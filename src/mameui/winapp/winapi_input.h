// license:BSD-3-Clause
// copyright-holders:0perator

#ifndef MAMEUI_WINAPP_WINAPI_INPUT_H
#define MAMEUI_WINAPP_WINAPI_INPUT_H

#pragma once

// standard windows headers
#include <windows.h>
#include <windef.h>

namespace mameui::winapi
{
	namespace input
	{
		BOOL enable_window(HWND window_handle, BOOL enable_window);

		HWND get_active_window();

		HWND set_focus(HWND window_handle);
	}
}

#endif // MAMEUI_WINAPP_WINAPI_INPUT_H
