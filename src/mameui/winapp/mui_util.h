// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_MUI_UTIL_H
#define MAMEUI_WINAPP_MUI_UTIL_H

#pragma once

// MAME/MAMEUI headers

// standard windows headers
#include <windows.h>
#include <windef.h>

// standard C++ headers

extern void __cdecl ErrorMsg(const char* fmt, ...);
extern void __cdecl dprintf(const char* fmt, ...);


extern UINT GetDepth(HWND hWnd);

// Open a text file
extern void DisplayTextFile(HWND hWnd, const char *cName);

template<typename T1, typename T2>
constexpr auto PACKVERSION(T1 major, T2 minor) { return MAKELONG(minor,major); }


void ShellExecuteCommon(HWND hWnd, const char *cName);
extern char *ConvertToWindowsNewlines(const char *source);
extern const char *GetDriverFilename(uint32_t nIndex);

bool DriverIsClone(uint32_t driver_index);
bool DriverIsBroken(uint32_t driver_index);
bool DriverIsHarddisk(uint32_t driver_index);
bool DriverHasOptionalBIOS(uint32_t driver_index);
bool DriverIsStereo(uint32_t driver_index);
bool DriverIsVector(uint32_t driver_index);
int DriverNumScreens(uint32_t driver_index);
bool DriverIsBios(uint32_t driver_index);
bool DriverUsesRoms(uint32_t driver_index);
bool DriverUsesSamples(uint32_t driver_index);
bool DriverUsesTrackball(uint32_t driver_index);
bool DriverUsesLightGun(uint32_t driver_index);
bool DriverUsesMouse(uint32_t driver_index);
bool DriverSupportsSaveState(uint32_t driver_index);
bool DriverIsVertical(uint32_t driver_index);
bool DriverIsMechanical(uint32_t driver_index);
bool DriverIsArcade(uint32_t driver_index);
bool DriverHasRam(uint32_t driver_index);

bool isDriverVector(const machine_config *config);
int numberOfSpeakers(const machine_config *config);
int numberOfScreens(const machine_config *config);

void FlushFileCaches(void);

bool StringIsSuffixedBy(const char *s, const char *suffix);

bool SafeIsAppThemed(void);

// provides result of FormatMessage()
// resulting buffer must be free'd with LocalFree()
void GetSystemErrorMessage(DWORD dwErrorId, wchar_t **tErrorMessage);

#endif // MAMEUI_WINAPP_MUI_UTIL_H
