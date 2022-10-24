// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_WINUI_H
#define MAMEUI_WINAPP_WINUI_H

#pragma once

// standard windows headers
#include <windef.h>
#include <wingdi.h>
#include <winuser.h>
#include <commctrl.h>
#include <commdlg.h>

// standard C++ headers

// MAME/MAMEUI headers
#include "emu.h"
#include "screenshot.h"

using namespace std::literals;

// Make sure all MESS features are included, and include software panes
//#define MESS // 0perator - I placed the definition in a more appropiate place. You'll find it in the build scripts.

/////////////////////// Next line must be commented out manually as there is no compile define
//#define BUILD_MESS // 0perator - Different features anyway. Why not use the same macro above?

//#ifdef MESS // BUILD_MESS
//#ifndef PTR64
// constexpr std::wstring_view MAMEUINAME = L"MESSUI32"sv;
//#else
// constexpr std::wstring_view MAMEUINAME = L"MESSUI"sv;
//#endif
// constexpr std::string_view MUI_INI_FILENAME = "MESSUI.ini"sv;
//#else
#ifndef PTR64
std::wstring_view MAMEUINAME(L"MAMEUI32"sv);
#else
std::wstring_view MAMEUINAME(L"MAMEUI"sv);
#endif
std::string_view MUI_INI_FILENAME("mameui.ini"sv);
//#endif

std::string_view SEARCH_PROMPT("<search here>"sv);

enum
{
	UNKNOWN = -1,
	TAB_PICKER = 0,
	TAB_DISPLAY,
	TAB_MISC,
	NUM_TABS
};

enum
{
	FILETYPE_INPUT_FILES = 1,
	FILETYPE_SAVESTATE_FILES,
	FILETYPE_WAVE_FILES,
	FILETYPE_AVI_FILES,
	FILETYPE_MNG_FILES,
	FILETYPE_EFFECT_FILES,
	FILETYPE_SHADER_FILES,
	FILETYPE_BGFX_FILES,
	FILETYPE_LUASCRIPT_FILES
};


typedef struct icon_data_t
{
	INT resource;
	const char *icon_name;
} ICONDATA;

typedef BOOL (WINAPI *common_file_dialog_proc)(LPOPENFILENAMEW lpofn);
BOOL CommonFileDialog(common_file_dialog_proc cfd, wchar_t *filename, int filetype);

HWND GetMainWindow(void);
HWND GetTreeView(void);
HIMAGELIST GetLargeImageList(void);
HIMAGELIST GetSmallImageList(void);
void SetNumOptionFolders(int count);
void GetRealColumnOrder(int order[]);
HICON LoadIconFromFile(const char *iconname);
void UpdateScreenShot(void);
void ResizePickerControls(HWND hWnd);
void MamePlayGame(void);
int FindIconIndex(int nIconResource);
int FindIconIndexByName(const char *icon_name);
int GetSelectedPick(void);

void UpdateListView(void);

// Move The in "The Title (notes)" to "Title, The (notes)"
const char *ModifyThe(const char *str);

// Convert Ampersand so it can display in a static control
const char *ConvertAmpersandString(const char *s);

// globalized for painting tree control
HBITMAP GetBackgroundBitmap(void);
HPALETTE GetBackgroundPalette(void);
MYBITMAPINFO* GetBackgroundInfo(void);

int GetMinimumScreenShotWindowWidth(void);

// we maintain an array of drivers sorted by name, useful all around
int GetParentIndex(const game_driver *driver);
int GetCompatIndex(const game_driver *driver);
int GetParentRomSetIndex(const game_driver *driver);
int GetGameNameIndex(const char *name);

// sets text in part of the status bar on the main window
void SetStatusBarText(int part_index, const char *message);
void SetStatusBarTextF(int part_index, const char *fmt, ...) ATTR_PRINTF(2,3);

int MameUIMain(HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow);

BOOL MouseHasBeenMoved(void);

const char * GetSearchText(void);

std::string longdots(std::string, uint16_t);

#endif // MAMEUI_WINAPP_WINUI_H
