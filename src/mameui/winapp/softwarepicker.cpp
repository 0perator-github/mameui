// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************
//============================================================
//
//  softwarepicker.cpp - MESS's software picker
//
//  Note: other parts of this code
//  optionsms.c, optionsms.h,
//  messui.cpp at static const LPCWSTR mess_column_names[] =
//  messui.cpp - LOTS of stuff about software
//
//
//============================================================

// standard C++ headers
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

// standard windows headers
#include "winapi_common.h"

// MAME headers

// shlwapi.h pulls in objbase.h which defines interface as a struct which conflict with a template declaration in device.h
#ifdef interface
#undef interface // undef interface which is for COM interfaces
#endif
#include "emu.h"
#define interface struct

#include "drivenum.h"
#include "hash.h"
#include "unzip.h"

// MAMEUI headers
#include "mui_cstr.h"
#include "mui_wcstr.h"
#include "mui_wcstrconv.h"

#include "windows_controls.h"
#include "data_access_storage.h"
#include "windows_messages.h"

#include "bitmask.h"
#include "mui_opts.h"
#include "mui_util.h"
#include "gamepicker.h"
#include "swconfig.h"
#include "screenshot.h"
#include "winui.h"

#include "softwarepicker.h"

using namespace mameui::util::string_util;
using namespace mameui::winapi::controls;
using namespace mameui::winapi;
using namespace std::string_literals;

//============================================================
//  TYPE DEFINITIONS
//============================================================

using FileInfo = struct file_info
{
	bool hash_realized;
	std::string zip_entry_name;
	std::string base_name;
	std::string file_name;
	const device_image_interface* device;
};

using DirectorySearchInfo = struct directory_search_info
{
	directory_search_info *next;
	HANDLE find_handle;
	WIN32_FIND_DATAW fd;
	std::wstring directory_name;
};

using SoftwarepickerInfo = struct software_picker_info
{
	WNDPROC old_window_proc;
	file_info **file_index;
	int file_index_length;
	int hashes_realized;
	int current_position;
	directory_search_info *first_search_info;
	directory_search_info *last_search_info;
	const software_config *config;
};

//============================================================
//  CONSTANTS
//============================================================

static const wchar_t software_picker_property_name[] = L"SWPICKER";

//============================================================
// FUNCTION DEFINITIONS
//============================================================

static std::wstring NormalizePath(std::wstring_view filename)
{
	std::filesystem::path file_path(filename);

	if (!file_path.has_root_path())
	{
		file_path = std::filesystem::current_path() /= file_path;
	}

	return file_path.wstring();
}



static software_picker_info *GetSoftwarePickerInfo(HWND hwndPicker)
{
	HANDLE h;
	h = GetProp(hwndPicker, software_picker_property_name);
	assert(h);
	return (software_picker_info *) h;
}



std::optional<std::string> SoftwarePicker_LookupBasename(HWND hwndPicker, int nIndex)
{
	software_picker_info *pPickerInfo;
	pPickerInfo = GetSoftwarePickerInfo(hwndPicker);
	if ((nIndex < 0) || !(pPickerInfo->file_index_length > nIndex))
		return std::nullopt;

	return pPickerInfo->file_index[nIndex]->base_name;
}

std::optional<std::string> SoftwarePicker_LookupFilename(HWND hwndPicker, int nIndex)
{
	software_picker_info *pPickerInfo;
	pPickerInfo = GetSoftwarePickerInfo(hwndPicker);
	if ((nIndex < 0) || !(pPickerInfo->file_index_length > nIndex))
		return std::nullopt;

	return pPickerInfo->file_index[nIndex]->file_name;
}

const device_image_interface *SoftwarePicker_LookupDevice(HWND hwndPicker, int nIndex)
{
	software_picker_info *pPickerInfo;
	pPickerInfo = GetSoftwarePickerInfo(hwndPicker);
	if ((nIndex < 0) || !(pPickerInfo->file_index_length > nIndex))
		return nullptr;

	return pPickerInfo->file_index[nIndex]->device;
}



int SoftwarePicker_LookupIndex(HWND hwndPicker, std::string_view file_name)
{
	software_picker_info *pPickerInfo = GetSoftwarePickerInfo(hwndPicker);
	if (pPickerInfo)
	{
		for (size_t i = 0; i < pPickerInfo->file_index_length; i++)
		{
			if (file_name == pPickerInfo->file_index[i]->file_name)
				return i;
		}
	}

	return -1;
}



std::string SoftwarePicker_GetImageType(HWND hwndPicker, int nIndex)
{
	std::string type = "unkn";
	const device_image_interface *device = SoftwarePicker_LookupDevice(hwndPicker, nIndex);

	if (device)
		type = std::string(device->image_brief_type_name());

	return type;
}



void SoftwarePicker_SetDriver(HWND hwndPicker, const software_config *config)
{
	int i;
	software_picker_info *pPickerInfo;

	pPickerInfo = GetSoftwarePickerInfo(hwndPicker);
	pPickerInfo->config = config;

	// invalidate the hash "realization"
	for (i = 0; i < pPickerInfo->file_index_length; i++)
	{
		//pPickerInfo->file_index[i]->hashinfo = nullptr;
		pPickerInfo->file_index[i]->hash_realized = false;
	}
	pPickerInfo->hashes_realized = 0;
}


#if 0
static void ComputeFileHash(software_picker_info *pPickerInfo, file_info *pFileInfo, const unsigned char *pBuffer, unsigned int nLength)
{
	const char *functions;

	 determine which functions to use
	functions = hashfile_functions_used(pPickerInfo->config->hashfile, pFileInfo->device->image_type());

	 compute the hash
	pFileInfo->device->device_compute_hash(pFileInfo->hashes, (const void *)pBuffer, (size_t)nLength, functions);
}



static bool SoftwarePicker_CalculateHash(HWND hwndPicker, int nIndex)
{
	software_picker_info *pPickerInfo;
	file_info *pFileInfo;
	LPSTR pszZipName;
	bool rc = false;
//    unsigned char *pBuffer;
	unsigned int nLength;
	HANDLE hFile, hFileMapping;
	LVFINDINFOW lvfi;
	bool res;

	pPickerInfo = GetSoftwarePickerInfo(hwndPicker);
	assert((nIndex >= 0) && (nIndex < pPickerInfo->file_index_length));
	pFileInfo = pPickerInfo->file_index[nIndex];

	if (pFileInfo->zip_entry_name)
	{
		// this is in a ZIP file
		util::archive_file *zip;
		zip_error ziperr;
		const zip_file_header *zipent;

		// open the ZIP file
		nLength = pFileInfo->zip_entry_name - pFileInfo->file_name;
		pszZipName = (LPSTR) alloca(nLength);
		memcpy(pszZipName, pFileInfo->file_name, nLength);
		pszZipName[nLength - 1] = '\0';

		// get the entry name
		ziperr = zip_file_open(pszZipName, &zip);
		if (ziperr == ZIPERR_NONE)
		{
			zipent = zip_file_first_file(zip);
			while(!rc && zipent)
			{
				if (mui_stricmp(zipent->filename, pFileInfo->zip_entry_name)==0)
				{
					std::unique_ptr<char[]> pBuffer(new char[zipent->uncompressed_length]);
					if (pBuffer)
					{
						ziperr = zip_file_decompress(zip, pBuffer.get(), zipent->uncompressed_length);
						if (ziperr == ZIPERR_NONE)
						{
							ComputeFileHash(pPickerInfo, pFileInfo, pBuffer.get(), zipent->uncompressed_length);
							rc = true;
						}
					}
				}
				zipent = zip_file_next_file(zip);
			}
			zip_file_close(zip);
		}
	}
	else
	{
		// plain open file; map it into memory and calculate the hash
		hFile = win_create_file_utf8(pFileInfo->file_name, GENERIC_READ, FILE_SHARE_READ, nullptr,
			OPEN_EXISTING, 0, nullptr);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			nLength = GetFileSize(hFile, nullptr);
			hFileMapping = CreateFileMapping(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
			if (hFileMapping)
			{
				pBuffer = (unsigned char *)MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
				if (pBuffer)
				{
					ComputeFileHash(pPickerInfo, pFileInfo, pBuffer, nLength);
					UnmapViewOfFile(pBuffer);
					rc = true;
				}
				system_services::close_handle(hFileMapping);
			}
			system_services::close_handle(hFile);
		}
	}

	if (rc)
	{
		lvfi = { LVFI_PARAM };
		lvfi.lParam = nIndex;
		nIndex = list_view::find_item(hwndPicker, -1, &lvfi);
		if (nIndex > 0)
			(void)list_view::redraw_items(hwndPicker, nIndex, nIndex);
	}

	return rc;
}
#endif


bool uses_file_extension(device_image_interface &dev, std::string_view file_extension)
{
	bool result = false;

	if (file_extension.empty())
		return result;

	// find the extensions
	std::istringstream tokenStream(dev.file_extensions());
	std::string curr_extension;

	while (std::getline(tokenStream, curr_extension, ','))
	{
		if (file_extension == curr_extension)
		{
			result = true;
			break;
		}
	}
	return result;
}


#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
static void SoftwarePicker_RealizeHash(HWND hwndPicker, int nIndex)
{
	software_picker_info *pPickerInfo;
	file_info *pFileInfo;
	//const char *nHashFunctionsUsed = nullptr;
	//unsigned int nCalculatedHashes = 0;
	//iodevice_/t type;

	pPickerInfo = GetSoftwarePickerInfo(hwndPicker);
	assert((nIndex >= 0) && (nIndex < pPickerInfo->file_index_length));
	pFileInfo = pPickerInfo->file_index[nIndex];

#if 0
	// Determine which hash functions we need to use for this file, and which hashes
	// have already been calculated
	if ((pPickerInfo->config->hashfile != nullptr) && (pFileInfo->device != nullptr))
	{
		type = pFileInfo->device->image_type();
		if (type < std::size(s_devices))
			nHashFunctionsUsed = hashfile_functions_used(pPickerInfo->config->hashfile, type);
		nCalculatedHashes = hash_data_used_functions(pFileInfo->hash_string);
	}

	// Did we fully compute all hashes?
	if ((nHashFunctionsUsed & ~nCalculatedHashes) == 0)
	{
		// We have calculated all hashs for this file; mark it as realized
		pPickerInfo->file_index[nIndex]->hash_realized = true;
		pPickerInfo->hashes_realized++;

		if (pPickerInfo->config->hashfile)
		{
			pPickerInfo->file_index[nIndex]->hashinfo = hashfile_lookup(pPickerInfo->config->hashfile,
				pPickerInfo->file_index[nIndex]->hashes);
		}
	}
#endif
}
#pragma GCC diagnostic error "-Wunused-but-set-variable"


static bool SoftwarePicker_AddFileEntry(HWND hwndPicker, std::string_view file_name, UINT nZipEntryNameLength, UINT32 nCrc, bool bForce, bool check)
{
	software_picker_info *pPickerInfo;
	file_info **ppNewIndex;
	file_info *pInfo;
	int nIndex;
	const device_image_interface *device = nullptr;
	std::string file_ext;
	size_t begin_pos, end_pos;

	// first check to see if it is already here - FIXME - only check when it could be a duplicate
	if (check && (SoftwarePicker_LookupIndex(hwndPicker, file_name) >= 0))
		return true;

	pPickerInfo = GetSoftwarePickerInfo(hwndPicker);

	// look up the device
	begin_pos = file_name.rfind('.');
	end_pos = file_name.size();
	if (begin_pos != std::string::npos)
		file_ext = file_name.substr(begin_pos, end_pos);

	if (!file_ext.empty() && pPickerInfo->config)
	{
		for (device_image_interface &dev : image_interface_enumerator(pPickerInfo->config->mconfig->root_device()))
		{
			if (!dev.user_loadable())
				continue;
			if (uses_file_extension(dev, file_ext))
			{
				device = &dev;
				break;
			}
		}
	}

	// no device?  cop out unless bForce is on
	if ((device == nullptr) && !bForce)
		return true;

	// create the FileInfo structure
	pInfo = new file_info{};
	if (!pInfo)
		return false;

	// copy the filename
	pInfo->file_name = file_name;

	// set up device and CRC, if specified
	pInfo->device = device;
	if ((device != nullptr))
		nCrc = 0;
	//if (nCrc != 0)
		//snprintf(pInfo->hash_string, std::size(pInfo->hash_string), "c:%08x#", nCrc);

	// set up zip entry name length, if specified
	if (nZipEntryNameLength > 0)
		pInfo->zip_entry_name = &(pInfo->file_name)[0] + file_name.size() - nZipEntryNameLength;

	// calculate the subname
	begin_pos = file_name.rfind('\\');
	if (begin_pos != std::string::npos)
		pInfo->base_name = file_name.substr(begin_pos + 1, end_pos);
	else
		pInfo->base_name = pInfo->file_name;

	const size_t index_size = pPickerInfo->file_index_length + 1;
	ppNewIndex = new file_info*[index_size];
	if (!ppNewIndex)
		return false;

	(void)std::copy_n(pPickerInfo->file_index, index_size, ppNewIndex);
	if (pPickerInfo->file_index)
		delete[] pPickerInfo->file_index;

	nIndex = pPickerInfo->file_index_length++;
	pPickerInfo->file_index = ppNewIndex;
	pPickerInfo->file_index[nIndex] = pInfo;

	// Realize the hash
	SoftwarePicker_RealizeHash(hwndPicker, nIndex);

	// Actually insert the item into the picker
	Picker_InsertItemSorted(hwndPicker, nIndex);

	return true;
}



static bool SoftwarePicker_AddZipEntFile(HWND hwndPicker, std::string_view zip_path, bool bForce, util::archive_file::ptr &pZip, bool check)
{
	std::string zip_filepath,zip_filename;

	zip_filename = pZip->current_name();

	// special case; skip first two characters if they are './'
	if (zip_filename[0] == '.' && zip_filename[1] == '/')
		zip_filename.erase(0, 2);

	zip_filepath = &zip_path[0] + "\\"s + pZip->current_name();

	return SoftwarePicker_AddFileEntry(hwndPicker, zip_filepath, zip_filepath.size(), pZip->current_crc(), bForce, check);
}

static bool SoftwarePicker_InternalAddFile(HWND hwndPicker, std::wstring_view filename, bool bForce, bool check)
{
	bool rc = true;
	std::error_condition ziperr = std::errc::function_not_supported;
	util::archive_file::ptr pZip;
	std::filesystem::path file_path(filename);
	std::string file_extension = file_path.extension().string();

	if (mui_stricmp(file_extension, ".zip") == 0)
		ziperr = util::archive_file::open_zip(file_path.string(), pZip);
	else if (mui_stricmp(file_extension, ".7z") == 0)
		ziperr = util::archive_file::open_7z(file_path.string(), pZip);

	if (!ziperr)
	{
		if (!pZip)
			return false;

		int result = pZip->first_file();
		while (rc && (result >= 0))
		{
			rc = SoftwarePicker_AddZipEntFile(hwndPicker, file_path.string(), bForce, pZip, check);
			result = pZip->next_file();
		}
	}
	else
		rc = SoftwarePicker_AddFileEntry(hwndPicker, file_path.string(), 0, 0, bForce, check);

	return rc;
}



bool SoftwarePicker_AddFile(HWND hwndPicker, std::wstring_view file_name, bool check)
{
	Picker_ResetIdle(hwndPicker);
	std::wstring normalized_path = NormalizePath(file_name);

	return SoftwarePicker_InternalAddFile(hwndPicker, normalized_path, true, check);
}



bool SoftwarePicker_AddDirectory(HWND hwndPicker, std::wstring_view directory_name)
{
	software_picker_info *pPickerInfo;
	directory_search_info *pSearchInfo;
	directory_search_info **ppLast;

	std::wstring normalized_path = NormalizePath(directory_name);

	Picker_ResetIdle(hwndPicker);
	pPickerInfo = GetSoftwarePickerInfo(hwndPicker);

	pSearchInfo = new directory_search_info{};
	if (!pSearchInfo)
		return false;

	pSearchInfo->find_handle = INVALID_HANDLE_VALUE;

	pSearchInfo->directory_name = std::move(normalized_path);

	// insert into linked list
	if (pPickerInfo->last_search_info)
		ppLast = &pPickerInfo->last_search_info->next;
	else
		ppLast = &pPickerInfo->first_search_info;
	*ppLast = pSearchInfo;
	pPickerInfo->last_search_info = pSearchInfo;
	return true;
}



static void SoftwarePicker_FreeSearchInfo(directory_search_info *pSearchInfo)
{
	if (pSearchInfo->find_handle != INVALID_HANDLE_VALUE)
		FindClose(pSearchInfo->find_handle);
	delete pSearchInfo;
}



static void SoftwarePicker_InternalClear(software_picker_info *pPickerInfo)
{
	directory_search_info *p;
	int i;

	for (i = 0; i < pPickerInfo->file_index_length; i++)
		delete pPickerInfo->file_index[i];

	while(pPickerInfo->first_search_info)
	{
		p = pPickerInfo->first_search_info->next;
		SoftwarePicker_FreeSearchInfo(pPickerInfo->first_search_info);
		pPickerInfo->first_search_info = p;
	}

	pPickerInfo->file_index = nullptr;
	pPickerInfo->file_index_length = 0;
	pPickerInfo->current_position = 0;
	pPickerInfo->last_search_info = nullptr;
}



void SoftwarePicker_Clear(HWND hwndPicker)
{
	software_picker_info *pPickerInfo;

	pPickerInfo = GetSoftwarePickerInfo(hwndPicker);
	SoftwarePicker_InternalClear(pPickerInfo);
	(void)list_view::delete_all_items(hwndPicker);
}



static bool SoftwarePicker_AddEntry(HWND hwndPicker, directory_search_info* pSearchInfo)
{
	//software_picker_info *pPickerInfo;
	bool rc = false;

	//pPickerInfo = GetSoftwarePickerInfo(hwndPicker);

	rc = !mui_wcscmp(pSearchInfo->fd.cFileName, L"..") || !mui_wcscmp(pSearchInfo->fd.cFileName, L".");
	if (!rc)
	{
		std::wstring file_path = pSearchInfo->directory_name + L"\\" + pSearchInfo->fd.cFileName;

		if (pSearchInfo->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			rc = SoftwarePicker_AddDirectory(hwndPicker, file_path);
		else
			rc = SoftwarePicker_InternalAddFile(hwndPicker, file_path, false, 1); // must check for dup
	}
	return rc;
}



bool SoftwarePicker_Idle(HWND hwndPicker)
{
	software_picker_info *pPickerInfo;
	file_info *pFileInfo;
	directory_search_info *pSearchInfo;
	std::wstring search_filter;
	bool bSuccess;
	int nCount;
	bool bDone = false;

	pPickerInfo = GetSoftwarePickerInfo(hwndPicker);

	pSearchInfo = pPickerInfo->first_search_info;

	if (pSearchInfo)
	{
		// searching through directories
		if (pSearchInfo->find_handle != INVALID_HANDLE_VALUE)
			bSuccess = FindNextFileW(pSearchInfo->find_handle, &pSearchInfo->fd);
		else
		{
			search_filter = pSearchInfo->directory_name + L"\\*.*";
			pSearchInfo->find_handle = storage::find_first_file(&search_filter[0], &pSearchInfo->fd);
			bSuccess = pSearchInfo->find_handle != INVALID_HANDLE_VALUE;
		}

		if (bSuccess)
			SoftwarePicker_AddEntry(hwndPicker, pSearchInfo);
		else
		{
			pPickerInfo->first_search_info = pSearchInfo->next;
			if (!pPickerInfo->first_search_info)
				pPickerInfo->last_search_info = nullptr;
			SoftwarePicker_FreeSearchInfo(pSearchInfo);
		}
	}
	else
	if (pPickerInfo->config!=nullptr  && (pPickerInfo->hashes_realized < pPickerInfo->file_index_length))
	{
		// time to realize some hashes
		nCount = 100;

		while((nCount > 0) && pPickerInfo->file_index[pPickerInfo->current_position]->hash_realized)
		{
			pPickerInfo->current_position++;
			pPickerInfo->current_position %= pPickerInfo->file_index_length;
			nCount--;
		}

		pFileInfo = pPickerInfo->file_index[pPickerInfo->current_position];
		if (!pFileInfo->hash_realized)
		{
#if 0
			if (hashfile_functions_used(pPickerInfo->config->hashfile, pFileInfo->device->image_type()))
			{
			// only calculate the hash if it is appropriate for this device
				if (!SoftwarePicker_CalculateHash(hwndPicker, pPickerInfo->current_position))
					return false;
			}
#endif
			SoftwarePicker_RealizeHash(hwndPicker, pPickerInfo->current_position);

			// under normal circumstances this will be redundant, but in the unlikely
			// event of a failure, we do not want to keep running into a brick wall
			// by calculating this hash over and over
			if (!pPickerInfo->file_index[pPickerInfo->current_position]->hash_realized)
			{
				pPickerInfo->file_index[pPickerInfo->current_position]->hash_realized = true;
				pPickerInfo->hashes_realized++;
			}
		}
	}
	else
	{
		// we are done!
		bDone = true;
	}

	return !bDone;
}



std::wstring SoftwarePicker_GetItemString(HWND hwndPicker, int nRow, int nColumn)
{
	software_picker_info *pPickerInfo = GetSoftwarePickerInfo(hwndPicker);

	if ((nRow >= 0) || (nRow < pPickerInfo->file_index_length))
	{
		switch (nColumn)
		{
		case SW_COLUMN_IMAGES:
			const file_info* pFileInfo = pPickerInfo->file_index[nRow];
			return mui_utf16_from_utf8string(pFileInfo->base_name);

#if 0   // MAMEUI: I probably should just remove these case satements too, but since a
			// good amount of code was used here before, I'll at least keep these here.
		case SW_COLUMN_GOODNAME:
			break;
		case SW_COLUMN_MANUFACTURER:
			break;
		case SW_COLUMN_YEAR:
			break;
		case SW_COLUMN_PLAYABLE:
			break;
		case SW_COLUMN_CRC:
			break;
		case SW_COLUMN_SHA1:
			switch (nColumn)
			{
			case SW_COLUMN_CRC:
				break;
			case SW_COLUMN_SHA1:
				break;
			}
			break;
#endif
		}
	}

	return std::wstring{};
}



static LRESULT CALLBACK SoftwarePicker_WndProc(HWND hwndPicker, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	software_picker_info *pPickerInfo;
	LRESULT rc;

	pPickerInfo = GetSoftwarePickerInfo(hwndPicker);
	rc = windows::call_window_proc(pPickerInfo->old_window_proc, hwndPicker, nMessage, wParam, lParam);

	if (nMessage == WM_DESTROY)
	{
		SoftwarePicker_InternalClear(pPickerInfo);
		SoftwarePicker_SetDriver(hwndPicker, nullptr);
		delete pPickerInfo;
	}

	return rc;
}



bool SetupSoftwarePicker(HWND hwndPicker, const PickerOptions *pOptions)
{
	software_picker_info *pPickerInfo = nullptr;
	LONG_PTR l;

	if (!SetupPicker(hwndPicker, pOptions))
		goto error;

	pPickerInfo = new software_picker_info{};
	if (!pPickerInfo)
		goto error;

	if (!SetProp(hwndPicker, software_picker_property_name, (HANDLE) pPickerInfo))
		goto error;

	l = (LONG_PTR) SoftwarePicker_WndProc;
	l = windows::set_window_long_ptr(hwndPicker, GWLP_WNDPROC, l);
	pPickerInfo->old_window_proc = (WNDPROC) l;
	return true;

error:
	if (pPickerInfo)
		delete pPickerInfo;
	return false;
}
