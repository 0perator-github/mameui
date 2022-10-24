// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************
//============================================================
//
//  softwarelist.cpp - MESS's softwarelist picker
//
//============================================================

// standard C++ headers
#include<algorithm>

// standard windows headers
#include "windows.h"
#include "commctrl.h"
#include "commdlg.h"

// MAME headers
#include "mameheaders.h"

// MAMEUI headers
#include "mui_str.h"
#include "mui_wcs.h"
#include "mui_wcsconv.h"

#include "winapi_controls.h"
#include "winapi_windows.h"

#include "picker.h"
#include "screenshot.h"
#include "swconfig.h"
#include "winui.h"

#include "softwarelist.h"

using namespace mameui::winapi;
using namespace mameui::winapi::controls;

//============================================================
//  TYPE DEFINITIONS
//============================================================

typedef struct _file_info file_info;
struct _file_info
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

typedef struct _software_list_info software_list_info;
struct _software_list_info
{
	WNDPROC old_window_proc;
	file_info **file_index;
	int file_index_length;
	const software_config *config;
};



//============================================================
//  CONSTANTS
//============================================================

static const wchar_t software_list_property_name[] = L"SWLIST";
static int software_numberofitems = 0;



static software_list_info *GetSoftwareListInfo(HWND hwndPicker)
{
	HANDLE h = GetProp(hwndPicker, software_list_property_name);
	//assert(h);
	return (software_list_info *) h;
}

#if 0
// return just the filename, to run a software in the software list - THIS IS NO LONGER USED
LPCSTR SoftwareList_LookupFilename(HWND hwndPicker, int nIndex)
{
	software_list_info *pPickerInfo;
	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	if ((nIndex < 0) || (nIndex >= pPickerInfo->file_index_length))
		return NULL;
	return pPickerInfo->file_index[nIndex]->file_name;
}
#endif

// return the list:file, for screenshot / history / inifile
std::string SoftwareList_LookupFullname(HWND hwndPicker, int nIndex)
{
	software_list_info *pPickerInfo;
	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	if ((nIndex < 0) || (nIndex >= pPickerInfo->file_index_length))
		return NULL;
	return pPickerInfo->file_index[nIndex]->full_name;
}

// return the media slot in which the software is mounted
std::string SoftwareList_LookupDevice(HWND hwndPicker, int nIndex)
{
	software_list_info *pPickerInfo;
	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	if ((nIndex < 0) || (nIndex >= pPickerInfo->file_index_length))
		return NULL;
	return pPickerInfo->file_index[nIndex]->device;
}


int SoftwareList_LookupIndex(HWND hwndPicker, LPCSTR pszFilename)
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
iodevice_//t SoftwareList_GetImageType(HWND hwndPicker, int nIndex)
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


bool SoftwareList_AddFile(HWND hwndPicker, std::string pszName, std::string pszListname, std::string pszDescription, std::string pszPublisher, std::string pszYear, std::string pszUsage, std::string pszDevice)
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
	{
		pszDescription = longdots(pszDescription, 200);
		pInfo->description = pszDescription;
	}

	if (!pszPublisher.empty())
	{
		pszPublisher = longdots(pszPublisher, 200);
		pInfo->publisher = pszPublisher;
	}
	if (!pszYear.empty())
	{
		pszYear = longdots(pszYear, 8);
		pInfo->year = pszYear;
	}
	if (!pszUsage.empty())
	{
		pszUsage = longdots(pszUsage, 200);
		pInfo->usage = pszUsage;
	}
	if (!pszDevice.empty())
		pInfo->device = pszDevice;

	full_name = util::string_format("%s:%s", pInfo->list_name, pInfo->file_name);
	pInfo->full_name = full_name;
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

	pPickerInfo->file_index = NULL;
	pPickerInfo->file_index_length = 0;
	software_numberofitems = 0;
}



void SoftwareList_Clear(HWND hwndPicker)
{
	software_list_info *pPickerInfo;

	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	SoftwareList_InternalClear(pPickerInfo);
	(void)ListView_DeleteAllItems(hwndPicker);
}


bool SoftwareList_Idle(HWND hwndPicker)
{
	return false;
}



LPCWSTR SoftwareList_GetItemString(HWND hwndPicker, int nRow, int nColumn, wchar_t *pszBuffer, UINT nBufferLength)
{
	software_list_info *pPickerInfo;
	const file_info *pFileInfo;
	LPCWSTR wcs_result = 0, wcs_buf = 0;

	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	if ((nRow < 0) || (nRow >= pPickerInfo->file_index_length))
		return NULL;

	pFileInfo = pPickerInfo->file_index[nRow];

	switch(nColumn)
	{
		case 0:
			wcs_buf = mui_wcstring_from_utf8(pFileInfo->file_name.c_str());
			break;
		case 1:
			wcs_buf = mui_wcstring_from_utf8(pFileInfo->list_name.c_str());
			break;
		case 2:
			wcs_buf = mui_wcstring_from_utf8(pFileInfo->description.c_str());
			break;
		case 3:
			wcs_buf = mui_wcstring_from_utf8(pFileInfo->year.c_str());
			break;
		case 4:
			wcs_buf = mui_wcstring_from_utf8(pFileInfo->publisher.c_str());
			break;
		case 5:
			wcs_buf = mui_wcstring_from_utf8(pFileInfo->usage.c_str());
			break;
	}

	if (wcs_buf)
	{
		(void)mui_wcsncpy(pszBuffer, wcs_buf, nBufferLength);
		wcs_result = pszBuffer;
		delete[] wcs_buf;
	}

	return wcs_result;
}



static LRESULT CALLBACK SoftwareList_WndProc(HWND hwndPicker, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	software_list_info *pPickerInfo;
	pPickerInfo = GetSoftwareListInfo(hwndPicker);
	LRESULT rc = windows::call_window_proc(pPickerInfo->old_window_proc, hwndPicker, nMessage, wParam, lParam);

	if (nMessage == WM_DESTROY)
	{
		SoftwareList_InternalClear(pPickerInfo);
		SoftwareList_SetDriver(hwndPicker, NULL);
		delete pPickerInfo;
	}

	return rc;
}



bool SetupSoftwareList(HWND hwndPicker, const PickerOptions *pOptions)
{
	software_list_info *pPickerInfo = NULL;
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
