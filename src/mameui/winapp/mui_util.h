// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_MUI_UTIL_H
#define MAMEUI_WINAPP_MUI_UTIL_H

#pragma once

extern void __cdecl ErrorMessageBox(const char* fmt, ...);
extern void __cdecl dprintf(const char* fmt, ...);

extern void __cdecl ErrorMessageBox(const wchar_t *fmt, ...);
extern void __cdecl dprintf(const wchar_t *fmt, ...);

extern UINT GetDepth(HWND hWnd);

// Open a text file
extern void DisplayTextFile(HWND hWnd, std::wstring cName);

template<typename T1, typename T2>
constexpr auto PACKVERSION(T1 major, T2 minor) { return MAKELONG(minor,major); }


void ShellExecuteCommon(HWND hWnd, std::wstring cName);

std::string ConvertToWindowsNewlines(std::string_view source);

std::wstring GetDriverFilename(std::size_t driver_index);
std::string GetDriverFilename_utf8(std::size_t driver_index);

// Commit 2d0a09b removed the ability to configure ":ram" from the command line & not having a UI
//bool DriverHasRam(std::size_t driver_index);
//bool HasRam(const machine_config *config);

bool DriverHasOptionalBIOS(std::size_t driver_index);
bool DriverIsArcade(std::size_t driver_index);
bool DriverIsBios(std::size_t driver_index);
bool DriverIsBroken(std::size_t driver_index);
bool DriverIsClone(std::size_t driver_index);
bool DriverIsHarddisk(std::size_t driver_index);
bool DriverIsMechanical(std::size_t driver_index);
bool DriverIsStereo(std::size_t driver_index);
bool DriverIsVector(std::size_t driver_index);
bool DriverIsVertical(std::size_t driver_index);
bool DriverSupportsSaveState(std::size_t driver_index);
bool DriverUsesLightGun(std::size_t driver_index);
bool DriverUsesMouse(std::size_t driver_index);
bool DriverUsesRoms(std::size_t driver_index);
bool DriverUsesSamples(std::size_t driver_index);
bool DriverUsesTrackball(std::size_t driver_index);
int DriverScreenCount(std::size_t driver_index);

std::wstring last_system_function_error_message();
std::string last_system_function_error_message_utf8();

bool HasOptionalBios(const game_driver *game);
bool UsesLightGun(const game_driver *game);
bool UsesMouse(const game_driver *game);
bool UsesTrackball(const game_driver *game);

bool HasHardDisk(const machine_config *config);
bool UsesVectorGraphics(const machine_config *config);
bool UsesRoms(const machine_config *config);
bool UsesSamples(const machine_config *config);

int ScreenCount(const machine_config *config);
int SpeakerCount(const machine_config *config);

void FlushFileCaches(void);

bool StringIsSuffixedBy(std::string_view str, std::string_view suffix);

bool SafeIsAppThemed(void);

// provides result of FormatMessage()
// resulting buffer must be free'd with LocalFree()
void GetSystemErrorMessage(DWORD dwErrorId, wchar_t **tErrorMessage);

#endif // MAMEUI_WINAPP_MUI_UTIL_H
