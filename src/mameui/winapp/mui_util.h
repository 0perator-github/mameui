// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_MUI_UTIL_H
#define MAMEUI_WINAPP_MUI_UTIL_H

#pragma once

// standard windows headers
#include <windows.h>
#include <windef.h>

// standard C++ headers

// MAME/MAMEUI headers
#include "emucore.h"

extern void __cdecl ErrorMsg(const char* fmt, ...);
extern void __cdecl dprintf(const char* fmt, ...);


extern UINT GetDepth(HWND hWnd);

// Open a text file
extern void DisplayTextFile(HWND hWnd, const char *cName);

#define PACKVERSION(major,minor) MAKELONG(minor,major)


void ShellExecuteCommon(HWND hWnd, const char *cName);
extern char *ConvertToWindowsNewlines(const char *source);
extern const char *GetDriverFilename(uint32_t nIndex);

BOOL DriverIsClone(uint32_t driver_index);
BOOL DriverIsBroken(uint32_t driver_index);
BOOL DriverIsHarddisk(uint32_t driver_index);
BOOL DriverHasOptionalBIOS(uint32_t driver_index);
BOOL DriverIsStereo(uint32_t driver_index);
BOOL DriverIsVector(uint32_t driver_index);
int DriverNumScreens(uint32_t driver_index);
BOOL DriverIsBios(uint32_t driver_index);
BOOL DriverUsesRoms(uint32_t driver_index);
BOOL DriverUsesSamples(uint32_t driver_index);
BOOL DriverUsesTrackball(uint32_t driver_index);
BOOL DriverUsesLightGun(uint32_t driver_index);
BOOL DriverUsesMouse(uint32_t driver_index);
BOOL DriverSupportsSaveState(uint32_t driver_index);
BOOL DriverIsVertical(uint32_t driver_index);
BOOL DriverIsMechanical(uint32_t driver_index);
BOOL DriverIsArcade(uint32_t driver_index);
BOOL DriverHasRam(uint32_t driver_index);

int isDriverVector(const machine_config *config);
int numberOfSpeakers(const machine_config *config);
int numberOfScreens(const machine_config *config);

void FlushFileCaches(void);

BOOL StringIsSuffixedBy(const char *s, const char *suffix);

BOOL SafeIsAppThemed(void);

// provides result of FormatMessage()
// resulting buffer must be free'd with LocalFree()
void GetSystemErrorMessage(DWORD dwErrorId, wchar_t **tErrorMessage);

#endif // MAMEUI_WINAPP_MUI_UTIL_H
