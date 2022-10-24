// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_HELP_H
#define MAMEUI_WINAPP_HELP_H

#pragma once

// standard windows headers
#if defined(__GNUC__)
constexpr auto HH_DISPLAY_TOPIC = 0;
constexpr auto HH_TP_HELP_CONTEXTMENU = 16;
constexpr auto HH_TP_HELP_WM_HELP = 17;
constexpr auto HH_CLOSE_ALL = 18;
constexpr auto HH_INITIALIZE = 28;
constexpr auto HH_UNINITIALIZE = 29;
#else
#include <htmlhelp.h>
#endif

using MAMEHELPINFO = struct mame_help_info
{
	int nMenuItem = -1;
	bool bIsHtmlHelp = false;
	std::wstring lpFile = L"";
};
using LPMAMEHELPINFO = MAMEHELPINFO*;
extern const MAMEHELPINFO g_helpInfo[];

#ifdef MESS
constexpr auto  MAMEUIHELP = L"messui.chm";
constexpr auto MAMEUICONTEXTHELP = L"messui.chm::/cntx_help.txt";
#else
constexpr auto  MAMEUIHELP = L"mameui.chm";
constexpr auto MAMEUICONTEXTHELP = L"mameui.chm::/cntx_help.txt";
#endif

extern int HelpInit(void);
extern void HelpExit(void);
extern HWND HelpFunction(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData);

#endif // MAMEUI_WINAPP_HELP_H
