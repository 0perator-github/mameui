// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

 /***************************************************************************

  winui.cpp

  Win32 GUI code.

  Created 8/12/97 by Christopher Kirmse (ckirmse@ricochet.net)
  Additional code November 1997 by Jeff Miller (miller@aa.net)
  More July 1998 by Mike Haaland (mhaaland@hypertech.com)
  Added Spitters/Property Sheets/Removed Tabs/Added Tree Control in
  Nov/Dec 1998 - Mike Haaland

***************************************************************************/

// standard C++ headers
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>
#include <string>
#include <thread>

// standard windows headers
#include "windows.h"
#include "commdlg.h"

// MAME headers
#include "mameheaders.h"

// MAMEUI headers
//#include "mui_str.h"
//#include "mui_wcs.h"
//#include "mui_wcsconv.h"
//
#include "winapi_controls.h"
#include "winapi_dialog_boxes.h"
#include "winapi_gdi.h"
#include "winapi_menus.h"
//#include "winapi_shell.h"
//#include "winapi_system_services.h"
#include "winapi_windows.h"
//
//#include "bitmask.h"
//#include "columnedit.h"
//#include "dialogs.h"
//#include "dijoystick.h"     // For DIJoystick availability.
//#include "directinput.h"
//#include "directories.h"
//#include "dirwatch.h"
//#include "emu_opts.h"
//#include "help.h"
//#include "history.h"
//#include "mui_audit.h"
#include "mui_opts.h"
//#include "mui_util.h"
#include "picker.h"
//#include "properties.h"
#include "resource.h"
//#include "resource.hm"
//#include "splitters.h"
//#include "tabview.h"
//#include "treeview.h"
//
////#ifdef MESS
//#include "messui.h"
//#include "softwarelist.h"
////#endif

using namespace std::literals;

//#include "winui.h"

using namespace mameui::winapi;

/***************************************************************************
 externally defined global variables
 ***************************************************************************/

//#ifdef MESS // Set naming for MESSUI
//#ifndef PTR64
// std::wstring_view MAMEUINAME = L"MESSUI32"sv;
//#else
//std::wstring_view MAMEUINAME = L"MESSUI"sv;
//#endif
//std::string_view MUI_INI_FILENAME = "MESSUI.ini"sv;
//#else // or for MAMEUI
#ifndef PTR64
std::wstring_view MAMEUINAME(L"MAMEUI32"sv);
#else
std::wstring_view MAMEUINAME(L"MAMEUI"sv);
#endif
std::string_view MUI_INI_FILENAME("mameui.ini"sv);
//#endif

std::string_view SEARCH_PROMPT("<search here>"sv);

//
//	function prototypes
// ---------------------

HMENU GetMainMenu(void);

HWND GetListView(void);
HWND GetMainWindow(void);
HWND GetToolBar(void);
HWND GetTreeView(void);

bool IsStatusBarVisible(void);
bool IsToolBarVisible(void);

//
//    External functions
// ----------------------

HWND GetListView(void)
{
	static HWND listview_handle;

	if (!listview_handle)
		listview_handle = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_LIST);

	return listview_handle;
}

HMENU GetMainMenu(void)
{
	static HMENU mainmenu_handle;

	if (!mainmenu_handle)
		mainmenu_handle = menus::get_menu(GetMainWindow());

	return mainmenu_handle;
}

HWND GetMainWindow(void)
{
	static HWND mainwindow_handle;

	if (!mainwindow_handle)
		mainwindow_handle = windows::find_window(L"MainClass");

	return mainwindow_handle;
}

HWND GetStatusBar(void)
{
	static HWND statusbar_handle;

	if (!statusbar_handle)
		statusbar_handle = windows::find_window_ex(GetMainWindow(), NULL, STATUSCLASSNAMEW, NULL);

	return statusbar_handle;
}

HWND GetToolBar(void)
{
	static HWND toolbar_handle;

	if (!toolbar_handle)
		toolbar_handle = windows::find_window_ex(GetMainWindow(), NULL, TOOLBARCLASSNAMEW, NULL);

	return toolbar_handle;
}

HWND GetTreeView(void)
{
	static HWND treeview_handle;

	if (!treeview_handle)
		treeview_handle = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_TREE);

	return treeview_handle;
}

bool IsStatusBarVisible(void)
{
	return static_cast<bool>(windows::is_window_visible(GetStatusBar()));
}

bool IsToolBarVisible(void)
{
	return static_cast<bool>(windows::is_window_visible(GetToolBar()));
}
