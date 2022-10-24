// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

/***************************************************************************

  help.cpp

    Help wrapper code.

***************************************************************************/

// standard C++ headers
#include <cstddef>
#include <string>
#include <string_view>

// standard windows headers
#include <windows.h>
#include <libloaderapi.h>
#include <processthreadsapi.h>

// MAME headers

// MAMEUI headers
#include "system_services.h"

#include "help.h"

using namespace mameui::winapi;

using HtmlHelpProc = HWND(WINAPI*)(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData);

/***************************************************************************
 Internal function prototypes
***************************************************************************/

/***************************************************************************
 External function prototypes
***************************************************************************/

namespace
{

	/***************************************************************************
	 Internal structures
	***************************************************************************/

	/***************************************************************************
	 Internal variables
	***************************************************************************/

	HtmlHelpProc g_pHtmlHelp;
	HMODULE      g_hHelpLib;
	DWORD        g_dwCookie = 0UL;

	/***************************************************************************
	 Internal functions
	***************************************************************************/

	void Help_Load()
	{
		g_hHelpLib = system_services::load_library(L"hhctrl.ocx");
		if (g_hHelpLib)
		{
			FARPROC pProc = nullptr;
			pProc = system_services::get_proc_address(g_hHelpLib, L"HtmlHelpW");
			if (pProc)
			{
				g_pHtmlHelp = (HtmlHelpProc)pProc;
			}
			else
			{
				system_services::free_library(g_hHelpLib);
				g_hHelpLib = nullptr;
			}
		}
	}
}

/***************************************************************************
 External variables
***************************************************************************/

/**************************************************************************
 External functions
***************************************************************************/

int HelpInit()
{
	g_pHtmlHelp = nullptr;
	g_hHelpLib  = nullptr;

	g_dwCookie = 0;
	HelpFunction(nullptr, nullptr, HH_INITIALIZE, (DWORD_PTR)&g_dwCookie);
	return 0;
}

void HelpExit()
{
	HelpFunction(nullptr, nullptr, HH_CLOSE_ALL, 0);
	HelpFunction(nullptr, nullptr, HH_UNINITIALIZE, (DWORD_PTR)&g_dwCookie);

	g_dwCookie  = 0;
	g_pHtmlHelp = nullptr;

	if (g_hHelpLib)
	{
		system_services::free_library(g_hHelpLib);
		g_hHelpLib = nullptr;
	}
}

HWND HelpFunction(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData)
{
	if (g_pHtmlHelp == nullptr)
		Help_Load();

	if (g_pHtmlHelp)
		return g_pHtmlHelp(hwndCaller, pszFile, uCommand, dwData);
	else
		return nullptr;
}
