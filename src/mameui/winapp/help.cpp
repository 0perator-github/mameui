// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

/***************************************************************************

  help.cpp

    Help wrapper code.

***************************************************************************/

// standard windows headers
#include <libloaderapi.h>

// MAMEUI headers
#include "help.h"

typedef HWND (WINAPI *HtmlHelpProc)(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData);

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
		g_hHelpLib = LoadLibraryW(L"hhctrl.ocx");
		if (g_hHelpLib)
		{
			FARPROC pProc = nullptr;
			pProc = GetProcAddress(g_hHelpLib, "HtmlHelpW");
			if (pProc)
			{
				g_pHtmlHelp = (HtmlHelpProc)pProc;
			}
			else
			{
				FreeLibrary(g_hHelpLib);
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
		FreeLibrary(g_hHelpLib);
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
