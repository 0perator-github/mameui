// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_WINUI_SHARED_H
#define MAMEUI_WINAPP_WINUI_SHARED_H

#pragma once

// Make sure all MESS features are included, and include software panes
//#define MESS // 0perator - I placed the definition in a more appropiate place. You'll find it in the build scripts.

/////////////////////// Next line must be commented out manually as there is no compile define
//#define BUILD_MESS // 0perator - Different features anyway. Why not use the same macro above?

extern std::wstring_view MAMEUINAME;
extern std::string_view MUI_INI_FILENAME;
extern std::string_view SEARCH_PROMPT;

HMENU GetMainMenu(void);

HWND GetListView(void);
HWND GetMainWindow(void);
HWND GetToolBar(void);
HWND GetTreeView(void);

bool IsStatusBarVisible(void);
bool IsToolBarVisible(void);

#endif // MAMEUI_WINAPP_WINUI_SHARED_H
