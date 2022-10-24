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
