// For licensing and usage information, read docs/winui_license.txt
// MASTER
// ============================================================================

// ============================================================================
// Screenshot.cpp - Displays snapshots, control panels and other pictures.
// Files must be of type .PNG, .JPG or .JPEG. Background pictures must be a
// PNG file, and not compressed. (not in a zip file).
// ============================================================================

// standard C++ header
#include <filesystem>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

// standard windows headers
#include "winapi_common.h"
#include <setjmp.h>

// MAME headers
#include "emu.h"
#include "softlist_dev.h"

#include "png.h"
#include "unzip.h"
#include "drivenum.h"
#include "libjpeg/jpeglib.h"

#include "ui/moptions.h"
#include "winopts.h"

// MAMEUI headers
#include "mui_cstr.h"
#include "mui_stringtokenizer.h"

#include "windows_gdi.h"

#include "emu_opts.h"
#include "mui_opts.h"
#include "mui_util.h"  // for DriverIsClone

#include "screenshot.h"
#include "winui.h"

using namespace mameui::winapi;

// ============================================================================
// Static global variables
// ============================================================================

// these refer to the single image currently loaded by the ScreenShot functions
static HGLOBAL   m_hDIB = nullptr;
static HPALETTE  m_hPal = nullptr;
static HANDLE m_hDDB = nullptr;

// ============================================================================
// Functions
// ============================================================================

namespace
{

// ============================================================================
// JPEG graphics handling
// ============================================================================

	// error handler for JPEG library
	struct mameui_jpeg_error_mgr
	{
		struct jpeg_error_mgr pub; // "public" fields
		jmp_buf setjmp_buffer; // for return to caller
	};

	// ----------------------------------------------------
	// mameui_jpeg_error_exit - error handler for JPEG library
	// Parameters:
	//     cinfo - pointer to JPEG decompression structure
	// ----------------------------------------------------

	METHODDEF(void) mameui_jpeg_error_exit(j_common_ptr cinfo)
	{
		mameui_jpeg_error_mgr *myerr = (mameui_jpeg_error_mgr*)cinfo->err;
		(*cinfo->err->output_message) (cinfo);
		longjmp(myerr->setjmp_buffer, 1);
	}

	// ----------------------------------------------------
	// jpeg_read_bitmap_gui - reads a JPEG image from a core file and
	// creates a DIB and palette
	// Parameters:
	//     mfile - reference to the core file
	//     phDIB - pointer to the handle of the DIB
	//     pPAL - pointer to the handle of the palette
	// ----------------------------------------------------

	bool jpeg_read_bitmap_gui(util::core_file& mfile, HGLOBAL* phDIB, HPALETTE* pPAL)
	{
		uint64_t bytes;
		mfile.length(bytes);
		std::vector<uint8_t> content(bytes);
		size_t length;
		mfile.read_some(content.data(), bytes, length);
		if (length == 0)
			return false;

		*pPAL = nullptr;
		HGLOBAL hDIB = nullptr;
		jpeg_decompress_struct info{};
		mameui_jpeg_error_mgr err{};
		info.err = jpeg_std_error(&err.pub);
		err.pub.error_exit = mameui_jpeg_error_exit;

		if (setjmp(err.setjmp_buffer))
		{
			jpeg_destroy_decompress(&info);
			if (hDIB)
				::GlobalFree(hDIB);

			return false;
		}

		jpeg_create_decompress(&info);
		jpeg_mem_src(&info, content.data(), bytes);
		jpeg_read_header(&info, TRUE);
		if (info.num_components != 3 || info.out_color_space != JCS_RGB)
		{
			jpeg_destroy_decompress(&info);
			return false;
		}
		const std::size_t bitCount = 24;
		BITMAPINFOHEADER bi = {};
		bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.biWidth = info.image_width;
		bi.biHeight = -info.image_height; // top down bitmap
		bi.biPlanes = 1;
		bi.biBitCount = bitCount;
		bi.biCompression = BI_RGB;
		bi.biXPelsPerMeter = 2835;
		bi.biYPelsPerMeter = 2835;

		const std::size_t dibWidth = (((static_cast<std::size_t>(info.image_width) * bitCount) + 31) / 32) * 4;
		const std::size_t imageSize = dibWidth * static_cast<std::size_t>(info.image_height);
		hDIB = GlobalAlloc(GMEM_FIXED, static_cast<SIZE_T>(bi.biSize + imageSize));

		if (!hDIB)
			return false;

		jpeg_start_decompress(&info);

		LPBITMAPINFOHEADER lpbi = static_cast<LPBITMAPINFOHEADER>(hDIB);
		std::memcpy(lpbi, &bi, sizeof(BITMAPINFOHEADER));
		std::uint8_t *pRgb = reinterpret_cast<std::uint8_t *>(lpbi) + bi.biSize;

		while (info.output_scanline < info.output_height)
		{
			jpeg_read_scanlines(&info, &pRgb, 1);
			// rgb to win32 bgr
			for (JDIMENSION i = 0; i < info.output_width; ++i)
			{
				std::size_t r_idx = i * 3;
				std::size_t b_idx = r_idx + 2;
				std::swap(pRgb[r_idx], pRgb[b_idx]);
			}
			pRgb += dibWidth;
		}
		jpeg_finish_decompress(&info);
		jpeg_destroy_decompress(&info);
		*phDIB = hDIB;

		return true;
	}

	// ============================================================================
	// PNG graphics handling
	// ============================================================================

	// ----------------------------------------------------
	// png_read_bitmap_gui - reads a PNG file and creates a DIB and palette for display in the GUI
	// Parameters:
	//     mfile - reference to the core file containing the PNG data
	//     phDIB - pointer to the handle of the DIB
	//     pPAL - pointer to the handle of the palette
	// ----------------------------------------------------

	bool png_read_bitmap_gui(util::core_file &mfile, HGLOBAL *phDIB, HPALETTE *pPAL)
	{
		util::png_info p;

		if (p.read_file(mfile))
		{
			return false;
		}
		bool isPalettized = !(p.num_palette > 256 || p.num_palette == 0 || p.color_type != 3);
		bool isRgbNonAlpha = !(p.num_palette > 0 || p.color_type != 2);
		if (!(isPalettized || isRgbNonAlpha))
		{
			std::cout << "Unsupported PNG color type " << p.color_type << ", has to be 2(RGB non-alpha) or 3(8bpp palettized)" << "\n";
		}

		if (p.interlace_method != 0)
		{
			std::cout << "PNG Interlace unsupported" << "\n";
			return false;
		}

		p.expand_buffer_8bit();

		const std::size_t bitCount = isPalettized ? 8 : 24;
		DWORD nColors = isPalettized ? p.num_palette : 0;

		BITMAPINFOHEADER bi = {};
		bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.biWidth = p.width;
		bi.biHeight = p.height;
		bi.biPlanes = 1;
		bi.biBitCount = bitCount;
		bi.biCompression = BI_RGB;
		bi.biClrUsed = nColors;
		bi.biClrImportant = nColors;

		const std::size_t absHeight = static_cast<std::size_t>(std::abs(static_cast<int64_t>(p.height)));
		const std::size_t colorTableSize = static_cast<std::size_t>(nColors) * sizeof(RGBQUAD);
		const std::size_t dibWidth = (((static_cast<std::size_t>(p.width) * bitCount) + 31) / 32) * 4;
		const std::size_t imageSize = dibWidth * absHeight;

		HGLOBAL hDIB = GlobalAlloc(GMEM_FIXED, static_cast<std::size_t>(bi.biSize) + colorTableSize + imageSize);

		if (!hDIB)
		{
			return false;
		}

		LPBITMAPINFOHEADER lpbi = static_cast<LPBITMAPINFOHEADER>(hDIB);
		std::memcpy(lpbi, &bi, sizeof(BITMAPINFOHEADER));
		RGBQUAD* pRgb = reinterpret_cast<RGBQUAD*>(reinterpret_cast<std::uint8_t*>(lpbi) + bi.biSize);
		std::uint8_t *lpDIBBits = reinterpret_cast<std::uint8_t*>(lpbi) + bi.biSize + colorTableSize;

		LPBITMAPINFO bmInfo = (LPBITMAPINFO)hDIB;

		if (isPalettized)
		{
			for (int i = 0; i < nColors; ++i)
			{
				std::size_t r_idx = i * 3;
				std::size_t g_idx = r_idx + 1;
				std::size_t b_idx = r_idx + 2;
				pRgb[i] = { p.palette[b_idx], p.palette[g_idx], p.palette[r_idx], 0 };
			}

			const std::size_t logPalSize = sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * (nColors - 1));
			std::vector<std::byte> logPalBuffer(logPalSize);

			LOGPALETTE* pLP = reinterpret_cast<LOGPALETTE*>(logPalBuffer.data());
			pLP->palVersion = 0x300;
			pLP->palNumEntries = static_cast<WORD>(nColors);

			for (int i = 0; i < nColors; ++i)
			{
				pLP->palPalEntry[i] = { bmInfo->bmiColors[i].rgbRed, bmInfo->bmiColors[i].rgbGreen, bmInfo->bmiColors[i].rgbBlue, 0 };
			}

			*pPAL = gdi::create_palette(pLP);
		}
		else
		{
			HDC hDC = gdi::create_compatible_dc(0);
			*pPAL = gdi::create_half_tone_palette(hDC);
			gdi::delete_dc(hDC);
		}

		if (isPalettized)
		{
			const std::size_t pngWidth = static_cast<std::size_t>(p.width);
			const std::size_t rowPixelsBytes = pngWidth;
			const std::uint8_t *srcEnd = p.image.get() + (rowPixelsBytes * absHeight);
			std::uint8_t *dstRow = lpDIBBits + (absHeight - 1) * dibWidth;
			std::uint8_t *srcRow = p.image.get();

			while (srcRow != srcEnd)
			{
				// zero entire destination row (ensures padding is cleared)
				if (dibWidth > 0)
					std::memset(dstRow, 0, dibWidth);

				// copy pixel indices (no per-pixel transform needed)
				std::memcpy(dstRow, srcRow, rowPixelsBytes);

				srcRow += rowPixelsBytes;
				dstRow -= dibWidth;
			}
		}
		else
		{
			const std::size_t pngRowBytes = static_cast<size_t>(p.width) * 3; // 3 bytes per pixel (RGB)
			const std::uint8_t* srcRowEnd = p.image.get() + (pngRowBytes * absHeight);
			std::uint8_t *dstRow = lpDIBBits + (absHeight - 1) * dibWidth;
			std::uint8_t *srcRow = p.image.get();

			while (srcRow != srcRowEnd)
			{
				std::uint8_t* srcPixel = srcRow;
				const std::uint8_t* srcPixelEnd = srcRow + pngRowBytes;
				std::uint8_t* dstPixel = dstRow;

				// Unrolled copy 8 pixels at a time (24 bytes per 8 pixels)
				while (srcPixel + 24 <= srcPixelEnd)
				{
					// pixel 0
					dstPixel[0] = srcPixel[2]; dstPixel[1] = srcPixel[1]; dstPixel[2] = srcPixel[0];
					// pixel 1
					dstPixel[3] = srcPixel[5]; dstPixel[4] = srcPixel[4]; dstPixel[5] = srcPixel[3];
					// pixel 2
					dstPixel[6] = srcPixel[8]; dstPixel[7] = srcPixel[7]; dstPixel[8] = srcPixel[6];
					// pixel 3
					dstPixel[9] = srcPixel[11];dstPixel[10] = srcPixel[10];dstPixel[11] = srcPixel[9];
					// pixel 4
					dstPixel[12] = srcPixel[14];dstPixel[13] = srcPixel[13];dstPixel[14] = srcPixel[12];
					// pixel 5
					dstPixel[15] = srcPixel[17];dstPixel[16] = srcPixel[16];dstPixel[17] = srcPixel[15];
					// pixel 6
					dstPixel[18] = srcPixel[20];dstPixel[19] = srcPixel[19];dstPixel[20] = srcPixel[18];
					// pixel 7
					dstPixel[21] = srcPixel[23];dstPixel[22] = srcPixel[22];dstPixel[23] = srcPixel[21];

					srcPixel += 24;
					dstPixel += 24;
				}

				// remaining pixels
				while (srcPixel < srcPixelEnd)
				{
					dstPixel[0] = srcPixel[2]; dstPixel[1] = srcPixel[1]; dstPixel[2] = srcPixel[0];
					srcPixel += 3; dstPixel += 3;
				}

				// clear row padding bytes (if any)
				if (dibWidth > pngRowBytes)
					std::memset(dstRow + pngRowBytes, 0, dibWidth - pngRowBytes);

				srcRow += pngRowBytes;
				dstRow -= dibWidth;
			}
		}

		*phDIB = hDIB;

		return true;
	}

	// ============================================================================
	// Image file opening functions
	// ============================================================================

	// ----------------------------------------------------
	// OpenRawDIBFile - opens a raw DIB file from the specified directory and
	// file name
	// Parameters:
	//     dir_path - directory path where the raw DIB file is located
	//     file_name - name of the raw DIB file
	//     file_ptr - reference to a pointer that will hold the opened file
	// ----------------------------------------------------

	std::error_condition OpenRawDIBFile(std::string_view dir_path, std::string_view file_name, util::core_file::ptr& file_ptr)
	{
		// look for the raw file
		std::string file_path = std::string(dir_path) + PATH_SEPARATOR + std::string(file_name);
		return util::core_file::open(file_path, OPEN_FLAG_READ, file_ptr);
	}

	// Function to find DIB in an archive file
	std::error_condition FindDIBInArchiveFile(std::string_view graphic_filename, util::archive_file::ptr& archive_ptr, util::core_file::ptr& file_ptr, std::vector<uint8_t>& buffer)
	{
		int found = archive_ptr->search(graphic_filename, false);
		std::error_condition archive_err;

		if (found > -1) {
			buffer.resize(archive_ptr->current_uncompressed_length());
			archive_err = archive_ptr->decompress(buffer.data(), buffer.size());

			if (!archive_err) {
				archive_err = util::core_file::open_ram(buffer.data(), buffer.size(), OPEN_FLAG_READ, file_ptr);
			}

			if (archive_err) {
				buffer.clear();
				file_ptr.reset();
			}
		}
		archive_ptr.reset();

		return archive_err;
	}

	// ----------------------------------------------------
	// OpenDIBIn7ZipFile - opens a DIB file from a 7-Zip archive
	// Parameters:
	//     zip_filepath - path to the 7-Zip file
	//     graphic_filename - name of the graphic file to find within the 7-Zip archive
	//     file_ptr - reference to a pointer that will hold the opened file
	//     buffer - reference to a vector that will hold the decompressed data
	// ----------------------------------------------------

	std::error_condition OpenDIBIn7ZipFile(std::string_view zip_filepath, std::string_view graphic_filename, util::core_file::ptr& file_ptr, std::vector<uint8_t>& buffer)
	{
		util::archive_file::ptr archive_ptr;
		std::error_condition archive_err = util::archive_file::open_7z(zip_filepath, archive_ptr);

		if (!archive_err)
			archive_err = FindDIBInArchiveFile(graphic_filename, archive_ptr, file_ptr, buffer);

		return archive_err;
	}

	// ----------------------------------------------------
	// OpenDIBInZipFile - opens a DIB file from a ZIP archive
	// Parameters:
	//     zip_filepath - path to the ZIP file
	//     graphic_filename - name of the graphic file to find within the ZIP archive
	//     file_ptr - reference to a pointer that will hold the opened file
	//     buffer - reference to a vector that will hold the decompressed data
	// ----------------------------------------------------

	std::error_condition OpenDIBInZipFile(std::string_view zip_filepath, std::string_view graphic_filename, util::core_file::ptr& file_ptr, std::vector<uint8_t>& buffer)
	{
		util::archive_file::ptr archive_ptr;
		std::error_condition archive_err = util::archive_file::open_zip(zip_filepath, archive_ptr);

		if (!archive_err)
			archive_err = FindDIBInArchiveFile(graphic_filename, archive_ptr, file_ptr, buffer);

		return archive_err;
	}

	// ============================================================================
	// Device idependant bitmap handling functions
	// ============================================================================

	// ----------------------------------------------------
	// LoadDIB - loads a DIB from a file or archive based on the specified picture type
	// Parameters:
	//     full_name - full name of the graphic file to load
	//     phDIB - pointer to the handle of the DIB
	//     pPal - pointer to the handle of the palette
	//     pic_type - type of picture to load (e.g., TAB_ARTWORK, TAB_BOSSES, etc.)
	// ----------------------------------------------------

	bool LoadDIB(std::string_view full_name, HGLOBAL* phDIB, HPALETTE* pPal, int pic_type)
	{
		bool success = false;
		std::string directory_paths, directory_name;

		if (pPal)
		{
			(void)gdi::delete_palette(*pPal);
		}

		// Set directory paths and names based on pic_type
		switch (pic_type)
		{
		case TAB_ARTWORK:
			directory_paths = emu_opts.dir_get_value(DIRPATH_ARTPREV_PATH);
			directory_name = "artpreview";
			break;
		case TAB_BOSSES:
			directory_paths = emu_opts.dir_get_value(DIRPATH_BOSSES_PATH);
			directory_name = "bosses";
			break;
		case TAB_CABINET:
			directory_paths = emu_opts.dir_get_value(DIRPATH_CABINETS_PATH);
			directory_name = "cabinets";
			break;
		case TAB_CONTROL_PANEL:
			directory_paths = emu_opts.dir_get_value(DIRPATH_CPANELS_PATH);
			directory_name = "cpanel";
			break;
		case TAB_COVER:
			directory_paths = emu_opts.dir_get_value(DIRPATH_COVER_PATH);
			directory_name = "covers";
			break;
		case TAB_ENDS:
			directory_paths = emu_opts.dir_get_value(DIRPATH_ENDS_PATH);
			directory_name = "ends";
			break;
		case TAB_FLYER:
			directory_paths = emu_opts.dir_get_value(DIRPATH_FLYERS_PATH);
			directory_name = "flyers";
			break;
		case TAB_GAMEOVER:
			directory_paths = emu_opts.dir_get_value(DIRPATH_GAMEOVER_PATH);
			directory_name = "gameover";
			break;
		case TAB_HOWTO:
			directory_paths = emu_opts.dir_get_value(DIRPATH_HOWTO_PATH);
			directory_name = "howto";
			break;
		case TAB_LOGO:
			directory_paths = emu_opts.dir_get_value(DIRPATH_LOGOS_PATH);
			directory_name = "logo";
			break;
		case TAB_MARQUEE:
			directory_paths = emu_opts.dir_get_value(DIRPATH_MARQUEES_PATH);
			directory_name = "marquees";
			break;
		case TAB_PCB:
			directory_paths = emu_opts.dir_get_value(DIRPATH_PCBS_PATH);
			directory_name = "pcb";
			break;
		case TAB_SCORES:
			directory_paths = emu_opts.dir_get_value(DIRPATH_SCORES_PATH);
			directory_name = "scores";
			break;
		case TAB_SCREENSHOT:
			directory_paths = emu_opts.dir_get_value(DIRPATH_SNAPSHOT_DIRECTORY);
			directory_name = "snap";
			break;
		case TAB_SELECT:
			directory_paths = emu_opts.dir_get_value(DIRPATH_SELECT_PATH);
			directory_name = "select";
			break;
		case TAB_TITLE:
			directory_paths = emu_opts.dir_get_value(DIRPATH_TITLES_PATH);
			directory_name = "titles";
			break;
		case TAB_VERSUS:
			directory_paths = emu_opts.dir_get_value(DIRPATH_VERSUS_PATH);
			directory_name = "versus";
			break;
		default:
			// shouldn't get here
			return success;
		}

		// Loop through possible file extensions
		for (uint8_t extnum = 0; extnum < 3; extnum++)
		{
			std::istringstream tokenStream(directory_paths);
			std::string current_path, ext, soft_name, softlist_name;
			std::string::size_type delim_pos;
			util::core_file::ptr file_ptr = nullptr;
			std::vector<uint8_t> buffer; // Changed to unique_ptr for buffer management

			// Set file extension based on extnum
			switch (extnum)
			{
			case 0:
				ext = ".png";
				break;
			case 1:
				ext = ".jpg";
				break;
			case 2:
				ext = ".jpeg";
				break;
			default:
				return success;
			}

			// Split full_name into softlist_name and soft_name
			delim_pos = full_name.rfind(":");
			if (delim_pos != std::string_view::npos)
			{
				softlist_name = full_name.substr(0, delim_pos);
				soft_name = full_name.substr(delim_pos + 1);
			}
			else
			{
				soft_name = full_name;
			}

			// Support multiple paths
			while (std::getline(tokenStream, current_path, ';'))
			{
				std::error_condition filerr;
				std::string graphic_filename, zip_filepath;

				// Handle various file patterns and paths
				if (softlist_name.empty())
				{
					// Try dir/game.png
					graphic_filename = soft_name + ext;
					filerr = OpenRawDIBFile(current_path, graphic_filename, file_ptr);

					// Try dir/dir.zip/game.png
					if (filerr)
					{
						graphic_filename = soft_name + ext;
						zip_filepath = current_path + PATH_SEPARATOR + directory_name + ".7z";

						filerr = OpenDIBIn7ZipFile(zip_filepath, graphic_filename, file_ptr, buffer);
						if (filerr)
						{
							zip_filepath = current_path + PATH_SEPARATOR + directory_name + ".zip";
							filerr = OpenDIBInZipFile(zip_filepath, graphic_filename, file_ptr, buffer);
						}
					}

					// For SNAPS only, try filenames with 0000.png
					if ((pic_type == TAB_SCREENSHOT) && (extnum == 0))
					{
						if (filerr)
						{
							graphic_filename = soft_name + PATH_SEPARATOR + "0000.png";
							filerr = OpenRawDIBFile(current_path, graphic_filename, file_ptr);
						}

						if (filerr)
						{
							graphic_filename = soft_name + PATH_SEPARATOR + soft_name + "0000.png";
							filerr = OpenRawDIBFile(current_path, graphic_filename, file_ptr);
						}

						if (filerr)
						{
							graphic_filename = soft_name + "0000.png";
							filerr = OpenRawDIBFile(current_path, graphic_filename, file_ptr);
						}
					}
				}
				else
				{
					// Handle software list specific paths
					graphic_filename = softlist_name + ext;
					filerr = OpenRawDIBFile(current_path, graphic_filename, file_ptr);

					if (filerr)
					{
						graphic_filename = softlist_name + PATH_SEPARATOR + soft_name + ext;
						filerr = OpenRawDIBFile(current_path, graphic_filename, file_ptr);
					}

					if (filerr)
					{
						graphic_filename = softlist_name + PATH_SEPARATOR + soft_name + ext;
						filerr = OpenRawDIBFile(current_path, graphic_filename, file_ptr);
					}

					if (filerr)
					{
						graphic_filename = softlist_name + ext;
						zip_filepath = current_path + PATH_SEPARATOR + directory_name + ".7z";

						filerr = OpenDIBIn7ZipFile(zip_filepath, graphic_filename, file_ptr, buffer);
						if (filerr)
						{
							zip_filepath = current_path + PATH_SEPARATOR + directory_name + ".zip";
							filerr = OpenDIBInZipFile(zip_filepath, graphic_filename, file_ptr, buffer);
						}
					}

					if (filerr) {
						graphic_filename = softlist_name + PATH_SEPARATOR + soft_name + ext;
						zip_filepath = current_path + PATH_SEPARATOR + directory_name + ".7z";

						filerr = OpenDIBIn7ZipFile(zip_filepath, graphic_filename, file_ptr, buffer);
						if (filerr)
						{
							zip_filepath = current_path + PATH_SEPARATOR + directory_name + ".zip";
							filerr = OpenDIBInZipFile(zip_filepath, graphic_filename, file_ptr, buffer);
						}
					}

					if (filerr)
					{
						graphic_filename = softlist_name + PATH_SEPARATOR + softlist_name + ext;
						zip_filepath = current_path + PATH_SEPARATOR + directory_name + ".7z";

						filerr = OpenDIBIn7ZipFile(zip_filepath, graphic_filename, file_ptr, buffer);
						if (filerr)
						{
							zip_filepath = current_path + PATH_SEPARATOR + directory_name + ".zip";
							filerr = OpenDIBInZipFile(zip_filepath, graphic_filename, file_ptr, buffer);
						}
					}

					if (filerr)
					{
						graphic_filename = soft_name + ext;
						zip_filepath = softlist_name + ".7z";

						filerr = OpenDIBIn7ZipFile(zip_filepath, graphic_filename, file_ptr, buffer);
						if (filerr)
						{
							zip_filepath = softlist_name + ".zip";
							filerr = OpenDIBInZipFile(zip_filepath, graphic_filename, file_ptr, buffer);
						}
					}

					if (filerr)
					{
						graphic_filename = softlist_name + PATH_SEPARATOR + soft_name + ext;
						zip_filepath = softlist_name + ".7z";

						filerr = OpenDIBIn7ZipFile(zip_filepath, graphic_filename, file_ptr, buffer);
						if (filerr)
						{
							zip_filepath = softlist_name + ".zip";
							filerr = OpenDIBInZipFile(zip_filepath, graphic_filename, file_ptr, buffer);
						}
					}
				}

				if (!filerr)
				{
					break;
				}
			}

			if (file_ptr)
			{
				// Read the bitmap using the appropriate function based on the extension
				if (extnum)
				{
					success = jpeg_read_bitmap_gui(*file_ptr, phDIB, pPal);
				}
				else
				{
					success = png_read_bitmap_gui(*file_ptr, phDIB, pPal);
				}

				if (success)
				{
					// Free the buffer and release the file pointer
					buffer.clear(); // Changed to unique_ptr reset
					file_ptr.reset();
					break;
				}
			}
		}

		return success;
	}
}

// --------------------------------------------------------
// DIBToDDB - converts a DIB to a DDB
// Parameters:
//     hDC - handle to the device context
//     hDIB - handle to the DIB
//     desc - pointer to a MYBITMAPINFO structure to receive bitmap information
// --------------------------------------------------------

HBITMAP DIBToDDB(HDC hDC, HANDLE hDIB, LPMYBITMAPINFO desc)
{
	BITMAPINFO* bmInfo = (LPBITMAPINFO)hDIB;

	if (hDIB == nullptr)
		return nullptr;

	LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER)hDIB;
	int nColors = lpbi->biClrUsed ? lpbi->biClrUsed : 1 << lpbi->biBitCount;

	LPVOID lpDIBBits;
	if (bmInfo->bmiHeader.biBitCount > 8)
		lpDIBBits = (LPVOID)((LPDWORD)(bmInfo->bmiColors + bmInfo->bmiHeader.biClrUsed) +
			((bmInfo->bmiHeader.biCompression == BI_BITFIELDS) ? 3 : 0));
	else
		lpDIBBits = (LPVOID)(bmInfo->bmiColors + nColors);

	if (desc != 0)
	{
		// Store for easy retrieval later
		desc->bmWidth = bmInfo->bmiHeader.biWidth;
		desc->bmHeight = bmInfo->bmiHeader.biHeight;
		desc->bmColors = (nColors <= 256) ? nColors : 0;
	}

	HBITMAP hBM = CreateDIBitmap(hDC,  // handle to device context
		(LPBITMAPINFOHEADER)lpbi,      // pointer to bitmap info header
		(LONG)CBM_INIT,                // initialization flag
		lpDIBBits,                     // pointer to initialization data
		(LPBITMAPINFO)lpbi,            // pointer to bitmap info
		DIB_RGB_COLORS);               // color-data usage

	return hBM;
}

// --------------------------------------------------------
// LoadScreenShot - loads a screenshot for the specified driver index and software name
// Parameters:
//     driver_index - index of the game driver
//     lpSoftwareName - name of the software item (if applicable)
//     nType - type of picture to load (e.g., TAB_ARTWORK, TAB_BOSSES, etc.)
// --------------------------------------------------------

bool LoadScreenShot(int driver_index, const std::string& lpSoftwareName, int nType)
{
	bool loaded = false;
	const game_driver& game = driver_list::driver(driver_index);
	//std::cout << "LoadScreenShot: A" << "\n";
	// Delete the last ones
	if (ScreenShotLoaded())
		FreeScreenShot();

	//std::cout << "LoadScreenShot: B" << "\n";
	// If software item, see if picture exists (correct parent is passed in lpSoftwareName)
	if (!lpSoftwareName.empty())
	{
		//std::cout << "LoadScreenShot: C" << "\n";
		loaded = LoadDIB(lpSoftwareName, &m_hDIB, &m_hPal, nType);
	}

	//std::cout << "LoadScreenShot: D" << "\n";
	// If game, see if picture exists. Or, if no picture for the software, use game's picture.
	if (!loaded)
	{
		//std::cout << "LoadScreenShot: E" << "\n";
		loaded = LoadDIB(game.name, &m_hDIB, &m_hPal, nType);

	}

	//std::cout << "LoadScreenShot: F" << "\n";
	// None? Try parent
	if (!loaded && DriverIsClone(driver_index))
	{
		int nParentIndex = GetParentIndex(&game);
		if (nParentIndex > INVALID_INDEX)
		{
			const game_driver& clone = driver_list::driver(nParentIndex);
			//std::cout << "LoadScreenShot: G" << "\n";
			loaded = LoadDIB(clone.name, &m_hDIB, &m_hPal, nType);
		}
	}

	//std::cout << "LoadScreenShot: H" << "\n";
	if (loaded)
	{
		HWND mainWindow_handle = GetMainWindow();
		HDC hdc = gdi::get_dc(mainWindow_handle);
		//std::cout << "LoadScreenShot: I" << "\n";
		m_hDDB = DIBToDDB(hdc, m_hDIB, nullptr);
		gdi::release_dc(mainWindow_handle, hdc);
	}

	std::cout << "LoadScreenShot: Finished" << "\n";
	return loaded;
}

// --------------------------------------------------------
// LoadDIBBG - loads a background image
// Parameters:
//     phDIB - pointer to the DIB handle
//     pPal - pointer to the palette handle
// --------------------------------------------------------

bool LoadDIBBG(HGLOBAL* phDIB, HPALETTE* pPal)
{
	bool success = false;
	std::error_condition filerr;
	std::string file_path = GetBgDir();
	util::core_file::ptr background_file = nullptr;

	if (pPal)
		(void)gdi::delete_palette(*pPal);

	// look for the raw file
	filerr = util::core_file::open(file_path.c_str(), OPEN_FLAG_READ, background_file);
	if (!filerr)
	{
		if (!background_file)
			return success;

		success = png_read_bitmap_gui(*background_file, phDIB, pPal);
	}

	return success;
}

// ============================================================================
// Accessor functions
// ============================================================================

// called by winui.cpp twice
bool ScreenShotLoaded()
{
	return m_hDDB != nullptr;
}

// called by winui.cpp once
HANDLE GetScreenShotHandle()
{
	return m_hDDB;
}

// called by winui.cpp twice
int GetScreenShotWidth()
{
	if (!m_hDIB)
		return 0;

	// Get the absolute value of the width from the bitmap info header
	return std::abs(static_cast<LPBITMAPINFO>(m_hDIB)->bmiHeader.biWidth);
}

// called by winui.cpp twice
int GetScreenShotHeight(void)
{
	if (!m_hDIB)
		return 0;

	// Get the absolute value of the height from the bitmap info header
	return std::abs(static_cast<LPBITMAPINFO>(m_hDIB)->bmiHeader.biHeight);
}

// called by winui.cpp
// Delete the HPALETTE and Free the HDIB memory
void FreeScreenShot(void)
{
	if (m_hDIB != nullptr)
		(void)GlobalFree(m_hDIB);
	m_hDIB = nullptr;

	if (m_hPal != nullptr)
		(void)gdi::delete_palette(m_hPal);
	m_hPal = nullptr;

	if (m_hDDB != nullptr)
		(void)gdi::delete_object(m_hDDB);
	m_hDDB = nullptr;
}
