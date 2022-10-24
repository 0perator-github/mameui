// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_MUI_UTIL_H
#define MAMEUI_WINAPP_MUI_UTIL_H

#pragma once

extern void __cdecl ErrorMsg(const char* fmt, ...);
extern void __cdecl dprintf(const char* fmt, ...);


extern UINT GetDepth(HWND hWnd);

// Open a text file
extern void DisplayTextFile(HWND hWnd, std::wstring cName);

template<typename T1, typename T2>
constexpr auto PACKVERSION(T1 major, T2 minor) { return MAKELONG(minor,major); }


void ShellExecuteCommon(HWND hWnd, std::wstring cName);

std::string ConvertToWindowsNewlines(std::string_view source);

std::wstring GetDriverFilename(int driver_index);
std::string GetDriverFilename_utf8(int driver_index);

bool DriverIsClone(int driver_index);
bool DriverIsBroken(int driver_index);
bool DriverIsHarddisk(int driver_index);
bool DriverHasOptionalBIOS(int driver_index);
bool DriverIsStereo(int driver_index);
bool DriverIsVector(int driver_index);
int DriverNumScreens(int driver_index);
bool DriverIsBios(int driver_index);
bool DriverUsesRoms(int driver_index);
bool DriverUsesSamples(int driver_index);
bool DriverUsesTrackball(int driver_index);
bool DriverUsesLightGun(int driver_index);
bool DriverUsesMouse(int driver_index);
bool DriverSupportsSaveState(int driver_index);
bool DriverIsVertical(int driver_index);
bool DriverIsMechanical(int driver_index);
bool DriverIsArcade(int driver_index);
bool DriverHasRam(int driver_index);

bool isDriverVector(const machine_config *config);

std::wstring last_system_function_error_message();
std::string last_system_function_error_message_utf8();

int numberOfSpeakers(const machine_config *config);
int numberOfScreens(const machine_config *config);

void FlushFileCaches(void);

bool StringIsSuffixedBy(std::string s, std::string suffix);

bool SafeIsAppThemed(void);

// provides result of FormatMessage()
// resulting buffer must be free'd with LocalFree()
void GetSystemErrorMessage(DWORD dwErrorId, wchar_t **tErrorMessage);

#endif // MAMEUI_WINAPP_MUI_UTIL_H
