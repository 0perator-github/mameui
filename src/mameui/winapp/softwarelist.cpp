// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************
//============================================================
//
//  softwarelist.cpp - MESS's softwarelist picker
//
//============================================================

// standard C++ headers
#include<algorithm>
#include <filesystem>
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
#include "util/path.h"

// MAMEUI headers
#include "mui_cstr.h"
#include "mui_wcstr.h"
#include "mui_wcstrconv.h"

#include "windows_controls.h"
#include "windows_messages.h"

#include "gamepicker.h"
#include "screenshot.h"
#include "swconfig.h"
#include "winui.h"

#include "softwarelist.h"

using namespace mameui::util::string_util;
using namespace mameui::winapi::controls;
using namespace mameui::winapi;
using namespace std::string_literals;

//============================================================
//  TYPE DEFINITIONS
//============================================================

using FileInfo = struct file_info
{
	std::string list_name;
	std::string description;
	std::string file_name;
	std::string publisher;
	std::string year;
	std::string full_name;
	std::string usage;
	std::string device;
};

using SoftwareistInfo = struct software_list_info
{
	WNDPROC old_window_proc;
	file_info **file_index;
	int file_index_length;
	const software_config* config;
};

//============================================================
//  CONSTANTS
//============================================================

static const wchar_t software_list_property_name[] = L"SWLIST";
static int software_numberofitems = 0;

//============================================================
// FUNCTION DEFINITIONS
//============================================================

static software_list_info *GetSoftwareListInfo(HWND hwndPicker)
{
	HANDLE h = GetProp(hwndPicker, software_list_property_name);
	//assert(h);
	return (software_list_info *) h;
}

#if 0
// return just the filename, to run a software in the software list - THIS IS NO LONGER USED
std::optional<std::string> SoftwareList_LookupFilename(HWND hwndPicker, int nIndex)
{
	software_list_info *pPickerInfo;
	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	if ((nIndex < 0) || (nIndex >= pPickerInfo->file_index_length))
		return std::nullopt;
	return pPickerInfo->file_index[nIndex]->file_name;
}
#endif

// return the list:file, for screenshot / history / inifile
std::optional<std::string> SoftwareList_LookupFullname(HWND hwndPicker, int nIndex)
{
	software_list_info *pPickerInfo;
	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	if ((nIndex < 0) || (nIndex >= pPickerInfo->file_index_length))
		return std::nullopt;
	return pPickerInfo->file_index[nIndex]->full_name;
}

// return the media slot in which the software is mounted
std::optional<std::string> SoftwareList_LookupDevice(HWND hwndPicker, int nIndex)
{
	software_list_info *pPickerInfo;
	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	if ((nIndex < 0) || (nIndex >= pPickerInfo->file_index_length))
		return std::nullopt;
	return pPickerInfo->file_index[nIndex]->device;
}


int SoftwareList_LookupIndex(HWND hwndPicker, std::string_view pszFilename)
{
	software_list_info *pPickerInfo;
	pPickerInfo = GetSoftwareListInfo(hwndPicker);

	for (size_t i = 0; i < pPickerInfo->file_index_length; i++)
	{
		if (mui_stricmp(pPickerInfo->file_index[i]->file_name, pszFilename)==0)
			return i;
	}
	return -1;
}


#if 0
// not used, swlist items don't have icons
//iodevice_t SoftwareList_GetImageType(HWND hwndPicker, int nIndex)
{
	return IO_UNKNOWN;
}
#endif


void SoftwareList_SetDriver(HWND hwndPicker, const software_config *config)
{
	software_list_info *pPickerInfo;

	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	pPickerInfo->config = config;
}


bool SoftwareList_AddFile(HWND hwndPicker, std::string_view pszName, std::string_view pszListname, std::string_view pszDescription, std::string_view pszPublisher, std::string_view pszYear, std::string_view pszUsage, std::string_view pszDevice)
{
	Picker_ResetIdle(hwndPicker);

	software_list_info *pPickerInfo;
	file_info **ppNewIndex;
	file_info *pInfo;
	int nIndex;
	std::string full_name;

	pPickerInfo = GetSoftwareListInfo(hwndPicker);

	// create the FileInfo structure
	pInfo = new file_info {};
	if (!pInfo)
		return 0;

	// copy the filename
	pInfo->file_name = pszName;
	pInfo->list_name = pszListname;
	if (!pszDescription.empty())
		pInfo->description = longdots(pszDescription, 200);
	if (!pszPublisher.empty())
		pInfo->publisher = longdots(pszPublisher, 200);
	if (!pszYear.empty())
		pInfo->year = longdots(pszYear, 8);
	if (!pszUsage.empty())
		pInfo->usage = longdots(pszUsage, 200);
	if (!pszDevice.empty())
		pInfo->device = pszDevice;

	full_name = pInfo->list_name + ":"s + pInfo->file_name;
	pInfo->full_name = std::move(full_name);
	const size_t index_size = pPickerInfo->file_index_length + 1;
	ppNewIndex = new file_info *[index_size];
	(void)std::copy_n(pPickerInfo->file_index, index_size - 1, ppNewIndex);
	if (pPickerInfo->file_index)
		delete[] pPickerInfo->file_index;

	if (!ppNewIndex)
	{
		delete pInfo;
		return 0;
	}

	nIndex = pPickerInfo->file_index_length++;
	pPickerInfo->file_index = ppNewIndex;
	pPickerInfo->file_index[nIndex] = pInfo;

	// Actually insert the item into the picker
	Picker_InsertItemSorted(hwndPicker, nIndex);
	software_numberofitems++;

	return 1;
}


static void SoftwareList_InternalClear(software_list_info *pPickerInfo)
{
	for (size_t i = 0; i < pPickerInfo->file_index_length; i++)
		delete pPickerInfo->file_index[i];

	pPickerInfo->file_index = nullptr;
	pPickerInfo->file_index_length = 0;
	software_numberofitems = 0;
}



void SoftwareList_Clear(HWND hwndPicker)
{
	software_list_info *pPickerInfo;

	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	SoftwareList_InternalClear(pPickerInfo);
	(void)list_view::delete_all_items(hwndPicker);
}


bool SoftwareList_Idle(HWND hwndPicker)
{
	return false;
}



std::wstring SoftwareList_GetItemString(HWND hwndPicker, int nRow, int nColumn)
{
	software_list_info *pPickerInfo = GetSoftwareListInfo(hwndPicker);

		if ((nRow >= 0) && (nRow < pPickerInfo->file_index_length))
		{

			const file_info* pFileInfo = pPickerInfo->file_index[nRow];

			switch (nColumn)
			{
			case 0: return mui_utf16_from_utf8string(pFileInfo->file_name);
			case 1: return mui_utf16_from_utf8string(pFileInfo->list_name);
			case 2: return mui_utf16_from_utf8string(pFileInfo->description);
			case 3:return mui_utf16_from_utf8string(pFileInfo->year);
			case 4: return mui_utf16_from_utf8string(pFileInfo->publisher);
			case 5:return mui_utf16_from_utf8string(pFileInfo->usage);
			}
		}

	return std::wstring{};
}



static LRESULT CALLBACK SoftwareList_WndProc(HWND hwndPicker, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	software_list_info *pPickerInfo;
	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	LRESULT rc = windows::call_window_proc(pPickerInfo->old_window_proc, hwndPicker, nMessage, wParam, lParam);

	if (nMessage == WM_DESTROY)
	{
		SoftwareList_InternalClear(pPickerInfo);
		SoftwareList_SetDriver(hwndPicker, nullptr);
		delete pPickerInfo;
	}

	return rc;
}



bool SetupSoftwareList(HWND hwndPicker, const PickerOptions *pOptions)
{
	software_list_info *pPickerInfo = nullptr;
	LONG_PTR l;

	if (!SetupPicker(hwndPicker, pOptions))
		goto error;

	pPickerInfo = new software_list_info{};
	if (!pPickerInfo)
		goto error;

	if (!SetProp(hwndPicker, software_list_property_name, (HANDLE) pPickerInfo))
		goto error;

	l = (LONG_PTR) SoftwareList_WndProc;
	l = windows::set_window_long_ptr(hwndPicker, GWLP_WNDPROC, l);
	pPickerInfo->old_window_proc = (WNDPROC) l;
	return true;

error:
	if (pPickerInfo)
		delete pPickerInfo;
	return false;
}

int SoftwareList_GetNumberOfItems()
{
	return software_numberofitems;
}
