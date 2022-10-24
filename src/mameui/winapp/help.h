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
	std::wstring_view lpFile = std::wstring_view{};
};
using LPMAMEHELPINFO = MAMEHELPINFO*;

// the size of the g_helpInfo array, See layout.cpp. Right now there's just 2 elements
constexpr std::size_t HELPINFO_COUNT = 2;

extern const std::array<MAMEHELPINFO, HELPINFO_COUNT> g_helpInfo;


//constexpr auto MAMEUIHELP_RELEASE = L"windows.txt";
#ifdef MESS
//constexpr auto MAMEUIHELP_CONTENTS = L"messui.chm::/windows/main.htm";
constexpr const std::wstring_view MAMEUIHELP_CONTENTS = L"messui.chm"; // 0 - call up CHM file
constexpr const std::wstring_view MAMEUIHELP_CONTEXT = L"messui.chm::/cntx_help.txt";
constexpr const std::wstring_view MAMEUIHELP_TROUBLE = L"messui.chm::/html/mameui_support.htm";
constexpr const std::wstring_view MAMEUIHELP_WHATSNEW = L""; // 1 - call up whatsnew at mamedev.org
#else
//constexpr auto MAMEUIHELP_CONTENTS = L"mameui.chm::/windows/main.htm";
constexpr const std::wstring_view MAMEUIHELP_CONTENTS = L"mameui.chm";
constexpr const std::wstring_view MAMEUIHELP_CONTEXT = L"mameui.chm::/cntx_help.txt";
constexpr const std::wstring_view MAMEUIHELP_WHATSNEW = L"mameui.chm::/docs/whatsnew.txt";
#endif

extern int HelpInit(void);
extern void HelpExit(void);
extern HWND HelpFunction(HWND hwndCaller, std::wstring_view pszFile, UINT uCommand, DWORD_PTR dwData);

#endif // MAMEUI_WINAPP_HELP_H
