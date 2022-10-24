// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************
//============================================================
//
//  softwarelist.h - MESS's software
//
//============================================================

#ifndef MAMEUI_WINAPP_SOFTWARELIST_H
#define MAMEUI_WINAPP_SOFTWARELIST_H

#pragma once


//std::optional<std::string> SoftwareList_LookupFilename(HWND hwndPicker, int nIndex); // returns file, eg adamlnk2 - NO LONGER USED
std::optional<std::string> SoftwareList_LookupFullname(HWND hwndPicker, int nIndex); // returns list:file, eg adam_cart:adamlnk2
std::optional<std::string> SoftwareList_LookupDevice(HWND hwndPicker, int nIndex); // returns the media slot in which the software is to be mounted
int SoftwareList_LookupIndex(HWND hwndPicker, std::string_view pszFilename);
//iodevice_t SoftwareList_GetImageType(HWND hwndPicker, int nIndex); // not used, swlist items don't have icons
bool SoftwareList_AddFile(HWND hwndPicker, std::string_view pszName, std::string_view pszListname, std::string_view pszDescription, std::string_view pszPublisher, std::string_view pszYear, std::string_view pszUsage, std::string_view pszDevice);
void SoftwareList_Clear(HWND hwndPicker);
void SoftwareList_SetDriver(HWND hwndPicker, const software_config *config);

// PickerOptions callbacks
std::wstring SoftwareList_GetItemString(HWND hwndPicker, int nRow, int nColumn);
bool SoftwareList_Idle(HWND hwndPicker);

bool SetupSoftwareList(HWND hwndPicker, const PickerOptions *pOptions);
int SoftwareList_GetNumberOfItems();

#endif // MAMEUI_WINAPP_SOFTWARELIST_H
