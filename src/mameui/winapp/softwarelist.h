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

#include "picker.h"
#include "swconfig.h"

//LPCSTR SoftwareList_LookupFilename(HWND hwndPicker, int nIndex); // returns file, eg adamlnk2 - NO LONGER USED
LPCSTR SoftwareList_LookupFullname(HWND hwndPicker, int nIndex); // returns list:file, eg adam_cart:adamlnk2
LPCSTR SoftwareList_LookupDevice(HWND hwndPicker, int nIndex); // returns the media slot in which the software is to be mounted
int SoftwareList_LookupIndex(HWND hwndPicker, LPCSTR pszFilename);
//iodevice_/t SoftwareList_GetImageType(HWND hwndPicker, int nIndex); // not used, swlist items don't have icons
BOOL SoftwareList_AddFile(HWND hwndPicker, std::string pszName, std::string pszListname, std::string pszDescription, std::string pszPublisher, std::string pszYear, std::string pszUsage, std::string pszDevice);
void SoftwareList_Clear(HWND hwndPicker);
void SoftwareList_SetDriver(HWND hwndPicker, const software_config *config);

// PickerOptions callbacks
LPCTSTR SoftwareList_GetItemString(HWND hwndPicker, int nRow, int nColumn, TCHAR *pszBuffer, UINT nBufferLength);
BOOL SoftwareList_Idle(HWND hwndPicker);

BOOL SetupSoftwareList(HWND hwndPicker, const PickerOptions *pOptions);
int SoftwareList_GetNumberOfItems();

#endif // MAMEUI_WINAPP_SOFTWARELIST_H
