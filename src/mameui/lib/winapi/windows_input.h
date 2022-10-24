// license:BSD-3-Clause
// copyright-holders:0perator

#ifndef MAMEUI_WINAPP_WINAPI_INPUT_H
#define MAMEUI_WINAPP_WINAPI_INPUT_H



// standard windows headers
#include <windows.h>
#include <windef.h>

namespace mameui::winapi
{
	namespace input
	{
		BOOL enable_window(HWND window_handle, BOOL enable_window);

		HWND get_active_window();

		HWND get_focus();

		BOOL is_window_enabled(HWND window_handle);

		BOOL release_capture(void);

		HWND set_active_window(HWND window_handle);

		HWND set_capture(HWND window_handle);

		HWND set_focus(HWND window_handle);
	}
}

#endif // MAMEUI_WINAPP_WINAPI_INPUT_H
