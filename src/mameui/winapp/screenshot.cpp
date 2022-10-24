// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

/***************************************************************************

  Screenshot.cpp

  Displays snapshots, control panels and other pictures.
  Files must be of type .PNG, .JPG or .JPEG .
  If Software tab is chosen, software-specific pictures can be displayed.
  Background picture must be PNG, and can be chosen from anywhere, but
    must be uncompressed (not in a zip file).

***************************************************************************/

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

// shlwapi.h pulls in objbase.h which defines interface as a struct which conflict with a template declaration in device.h
#ifdef interface
#undef interface // undef interface which is for COM interfaces
#endif
#include "emu.h"
#include "softlist_dev.h"
#define interface struct

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

/***************************************************************************
    Static global variables
***************************************************************************/

/* these refer to the single image currently loaded by the ScreenShot functions */
static HGLOBAL   m_hDIB = nullptr;
static HPALETTE  m_hPal = nullptr;
static HANDLE m_hDDB = nullptr;

/* PNG variables */

static int   copy_size = 0;
static char* pixel_ptr = nullptr;
static int   row = 0;
static int   effWidth;

/***************************************************************************
    Functions
***************************************************************************/

namespace
{

	/***************************************************************************
	    JPEG graphics handling functions
	***************************************************************************/

	struct mameui_jpeg_error_mgr
	{
		struct jpeg_error_mgr pub; /* "public" fields */
		jmp_buf setjmp_buffer; /* for return to caller */
	};

	METHODDEF(void) mameui_jpeg_error_exit(j_common_ptr cinfo)
	{
		mameui_jpeg_error_mgr* myerr = (mameui_jpeg_error_mgr*)cinfo->err;
		(*cinfo->err->output_message) (cinfo);
		longjmp(myerr->setjmp_buffer, 1);
	}

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
		jpeg_decompress_struct info;
		mameui_jpeg_error_mgr err;
		info.err = jpeg_std_error(&err.pub);
		err.pub.error_exit = mameui_jpeg_error_exit;

		if (setjmp(err.setjmp_buffer)) {
			jpeg_destroy_decompress(&info);
			copy_size = 0;
			pixel_ptr = nullptr;
			effWidth = 0;
			row = 0;
			if (hDIB)
				::GlobalFree(hDIB);
			return false;
		}

		jpeg_create_decompress(&info);
		jpeg_mem_src(&info, content.data(), bytes);
		jpeg_read_header(&info, TRUE);
		if (info.num_components != 3 || info.out_color_space != JCS_RGB) {
			jpeg_destroy_decompress(&info);
			return false;
		}

		BITMAPINFOHEADER bi = {};
		bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.biWidth = info.image_width;
		bi.biHeight = -info.image_height; // top down bitmap
		bi.biPlanes = 1;
		bi.biBitCount = 24;
		bi.biCompression = BI_RGB;
		bi.biXPelsPerMeter = 2835;
		bi.biYPelsPerMeter = 2835;

		effWidth = ((info.image_width * bi.biBitCount + 31) / 32) * 4;
		int dibSize = effWidth * info.image_height;
		hDIB = ::GlobalAlloc(GMEM_FIXED, bi.biSize + dibSize);

		if (!hDIB) {
			return false;
		}

		jpeg_start_decompress(&info);

		auto lpbi = static_cast<LPBITMAPINFOHEADER>(hDIB);
		std::memcpy(lpbi, &bi, sizeof(BITMAPINFOHEADER));
		auto pRgb = reinterpret_cast<LPSTR>(lpbi) + bi.biSize;
		auto lpDIBBits = reinterpret_cast<LPVOID>(reinterpret_cast<LPSTR>(lpbi) + bi.biSize);

		while (info.output_scanline < info.output_height) {
			unsigned char* cacheRow[1] = { reinterpret_cast<unsigned char*>(pRgb) };
			jpeg_read_scanlines(&info, cacheRow, 1);
			// rgb to win32 bgr
			for (JDIMENSION i = 0; i < info.output_width; ++i)
				std::swap(cacheRow[0][i * 3], cacheRow[0][i * 3 + 2]);
			pRgb += effWidth;
		}
		jpeg_finish_decompress(&info);
		jpeg_destroy_decompress(&info);
		copy_size = dibSize;
		pixel_ptr = static_cast<char*>(lpDIBBits);
		*phDIB = hDIB;
		return true;
	}

	/***************************************************************************
	    PNG graphics handling functions
	***************************************************************************/

	inline void store_pixels(UINT8* buf, int len)
	{
		if (pixel_ptr && copy_size)
		{
			memcpy(&pixel_ptr[row * effWidth], buf, len);
			row--;
			copy_size -= len;
		}
	}

	bool AllocatePNG(util::png_info *p, HGLOBAL *phDIB, HPALETTE* pPal)
	{
		int nColors = (p->color_type != 2 && p->num_palette <= 256) ? p->num_palette : 0;
		copy_size = 0;
		pixel_ptr = nullptr;
		row = p->height - 1;

		BITMAPINFOHEADER bi = {};
		bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.biWidth = p->width;
		bi.biHeight = p->height;
		bi.biPlanes = 1;
		bi.biBitCount = (p->color_type == 3) ? 8 : 24;
		bi.biCompression = BI_RGB;
		bi.biClrUsed = nColors;
		bi.biClrImportant = nColors;

		effWidth = ((p->width * bi.biBitCount + 31) / 32) * 4;
		int dibSize = effWidth * bi.biHeight;
		HGLOBAL hDIB = GlobalAlloc(GMEM_FIXED, bi.biSize + (nColors * sizeof(RGBQUAD)) + dibSize);

		if (!hDIB)
		{
			return false;
		}

		auto lpbi = static_cast<LPBITMAPINFOHEADER>(hDIB);
		std::memcpy(lpbi, &bi, sizeof(BITMAPINFOHEADER));
		auto pRgb = reinterpret_cast<RGBQUAD*>(reinterpret_cast<LPSTR>(lpbi) + bi.biSize);
		auto lpDIBBits = reinterpret_cast<LPVOID>(reinterpret_cast<LPSTR>(lpbi) + bi.biSize + (nColors * sizeof(RGBQUAD)));

		if (nColors)
		{
			for (int i = 0; i < nColors; ++i)
			{
				pRgb[i] = { p->palette[i * 3 + 2], p->palette[i * 3 + 1], p->palette[i * 3], 0 };
			}
		}

		LPBITMAPINFO bmInfo = (LPBITMAPINFO)hDIB;

		if (0 == nColors || nColors > 256)
		{
			HDC hDC = gdi::create_compatible_dc(0);
			*pPal = gdi::create_half_tone_palette(hDC);
			gdi::delete_dc(hDC);
		}
		else
		{
			LOGPALETTE* pLP = static_cast<LOGPALETTE*>(malloc(sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * nColors)));
			pLP->palVersion = 0x300;
			pLP->palNumEntries = nColors;
			for (int i = 0; i < nColors; ++i)
			{
				pLP->palPalEntry[i] = { bmInfo->bmiColors[i].rgbRed, bmInfo->bmiColors[i].rgbGreen, bmInfo->bmiColors[i].rgbBlue, 0 };
			}
			*pPal = gdi::create_palette(pLP);
			free(pLP);
		}

		copy_size = dibSize;
		pixel_ptr = static_cast<char*>(lpDIBBits);
		*phDIB = hDIB;
		return true;
	}

	bool png_read_bitmap_gui(util::core_file& mfile, HGLOBAL* phDIB, HPALETTE* pPAL)
	{
		util::png_info p;

		if (p.read_file(mfile))
		{
			return false;
		}

		if (p.color_type != 3 && p.color_type != 2)
		{
			std::cout << "PNG Unsupported color type " << p.color_type << " (has to be 2 or 3)" << "\n";
		}

		if (p.interlace_method != 0)
		{
			std::cout << "PNG Interlace unsupported" << "\n";
			return false;
		}

		p.expand_buffer_8bit();

		if (!AllocatePNG(&p, phDIB, pPAL))
		{
			std::cout << "PNG Unable to allocate memory to display screenshot" << "\n";
			return false;
		}

		int bytespp = (p.color_type == 2) ? 3 : 1;

		for (uint32_t i = 0; i < p.height; ++i)
		{
			uint8_t* ptr = &p.image[i * (p.width * bytespp)];

			if (p.color_type == 2)
			{
				for (uint32_t j = 0; j < p.width; ++j)
				{
					std::swap(ptr[0], ptr[2]);
					ptr += 3;
				}
			}
			store_pixels(&p.image[i * (p.width * bytespp)], p.width * bytespp);
		}

		return true;
	}

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

	// Function to open DIB in a 7Zip file
	std::error_condition OpenDIBIn7ZipFile(std::string_view zip_filepath, std::string_view graphic_filename, util::core_file::ptr& file_ptr, std::vector<uint8_t>& buffer)
	{
		util::archive_file::ptr archive_ptr;
		std::error_condition archive_err = util::archive_file::open_7z(zip_filepath, archive_ptr);

		if (!archive_err)
		{
			archive_err = FindDIBInArchiveFile(graphic_filename, archive_ptr, file_ptr, buffer);
		}

		return archive_err;
	}

	// Function to open DIB in a Zip file
	std::error_condition OpenDIBInZipFile(std::string_view zip_filepath, std::string_view graphic_filename, util::core_file::ptr& file_ptr, std::vector<uint8_t>& buffer)
	{
		util::archive_file::ptr archive_ptr;
		std::error_condition archive_err = util::archive_file::open_zip(zip_filepath, archive_ptr);

		if (!archive_err)
		{
			archive_err = FindDIBInArchiveFile(graphic_filename, archive_ptr, file_ptr, buffer);
		}

		return archive_err;
	}

	// display a snap, cabinet, title, flyer, marquee, pcb, control panel
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
/* Delete the HPALETTE and Free the HDIB memory */
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

/***************************************************************************
    File search functions
***************************************************************************/

// called from winui.cpp and here
HBITMAP DIBToDDB(HDC hDC, HANDLE hDIB, LPMYBITMAPINFO desc)
{
	BITMAPINFO * bmInfo = (LPBITMAPINFO)hDIB;

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
		/* Store for easy retrieval later */
		desc->bmWidth  = bmInfo->bmiHeader.biWidth;
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



// main call from winui to display a picture
bool LoadScreenShot(int driver_index, const std::string& lpSoftwareName, int nType)
{
	bool loaded = false;
	const game_driver &game = driver_list::driver(driver_index);
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
		if (nParentIndex > -1)
		{
			const game_driver &clone = driver_list::driver(nParentIndex);
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

// called from winui.cpp to display the background
bool LoadDIBBG(HGLOBAL *phDIB, HPALETTE *pPal)
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
