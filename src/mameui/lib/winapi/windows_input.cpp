// license:BSD-3-Clause
// copyright-holders:0perator

/***************************************************************************

  windows_input.cpp

 ***************************************************************************/

// standard C++ headers
#include <string>

// standard windows headers
#include "winapi_common.h"

// MAMEUI headers

#include "windows_input.h"

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
//  get_focus
//============================================================

HWND mameui::winapi::input::get_focus()
{
	return GetFocus();
}

//============================================================
//  is_window_enabled
//============================================================

BOOL mameui::winapi::input::is_window_enabled(HWND window_handle)
{
	return IsWindowEnabled(window_handle);
}

//============================================================
// release_capture
//============================================================

BOOL mameui::winapi::input::release_capture(void)
{
	return ReleaseCapture();
}

//============================================================
// set_active_window
//============================================================

HWND mameui::winapi::input::set_active_window(HWND window_handle)
{
	return SetActiveWindow(window_handle);
}

//============================================================
// set_capture
//============================================================

HWND mameui::winapi::input::set_capture(HWND window_handle)
{
	return SetCapture(window_handle);
}

//============================================================
//  set_focus
//============================================================

HWND mameui::winapi::input::set_focus(HWND window_handle)
{
	return SetFocus(window_handle);
}
