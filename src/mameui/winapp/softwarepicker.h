// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************
//============================================================
//
//  softwarepicker.h - MESS's software picker
//
//============================================================

#ifndef MAMEUI_WINAPP_SOFTWAREPICKER_H
#define MAMEUI_WINAPP_SOFTWAREPICKER_H

#pragma once

//============================================================
//  TYPE DEFINITIONS
//============================================================

using softwarepicker_column_id = enum softwarepicker_column_id
{
	SW_COLUMN_IMAGES,
	SW_COLUMN_COUNT
};

using device_entry = struct device_entry
{
	int dline = -1;
	std::string dev_type;
	int resource = -1;
	std::wstring dlgname;
};

//============================================================
//  GLOBAL VARIABLES
//============================================================

extern const std::array<device_entry, 21> s_devices;
extern const std::array<std::wstring_view, SW_COLUMN_COUNT> softwarepicker_column_names;

//============================================================
//  FUNCTION PROTOTYPES
//============================================================

std::optional<std::string> SoftwarePicker_LookupBasename(HWND hwndPicker, int nIndex);
std::optional<std::string> SoftwarePicker_LookupFilename(HWND hwndPicker, int nIndex);
const device_image_interface *SoftwarePicker_LookupDevice(HWND hwndPicker, int nIndex);
int SoftwarePicker_LookupIndex(HWND hwndPicker, std::string_view file_name);
std::string SoftwarePicker_GetImageType(HWND hwndPicker, int nIndex);
bool SoftwarePicker_AddFile(HWND hwndPicker, std::wstring_view file_name, bool check);
bool SoftwarePicker_AddDirectory(HWND hwndPicker, std::wstring_view directory_name);
void SoftwarePicker_Clear(HWND hwndPicker);
void SoftwarePicker_SetDriver(HWND hwndPicker, const software_config *config);

// PickerOptions callbacks
std::wstring SoftwarePicker_GetItemString(HWND hwndPicker, int nRow, int nColumn);
bool SoftwarePicker_Idle(HWND hwndPicker);

bool SetupSoftwarePicker(HWND hwndPicker, const PickerOptions *pOptions);
bool uses_file_extension(device_image_interface& dev, std::string_view file_extension);

#endif // MAMEUI_WINAPP_SOFTWAREPICKER_H
