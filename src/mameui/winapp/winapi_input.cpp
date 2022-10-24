// license:BSD-3-Clause
// copyright-holders:0perator

/***************************************************************************

  winapi_input.cpp

 ***************************************************************************/

// standard windows headers

// standard C++ headers

// MAME/MAMEUI headers
#include "mui_wcsconv.h"
#include "winapi_input.h"


//============================================================
// enable_window
//============================================================

BOOL mameui::winapi::input::enable_window(HWND window_handle, BOOL enable_window)
{
	return EnableWindow(window_handle, enable_window);
}


//============================================================
// get_active_window
//============================================================

HWND mameui::winapi::input::get_active_window()
{
	return GetActiveWindow();
}


//============================================================
//  set_focus
//============================================================

HWND mameui::winapi::input::set_focus(HWND window_handle)
{
	return SetFocus(window_handle);
}
