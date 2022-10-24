// license:BSD-3-Clause
// copyright-holders:0perator
// ============================================================================

#ifndef MAMEUI_WINAPP_WINAPI_INPUT_H
#define MAMEUI_WINAPP_WINAPI_INPUT_H

#pragma once


// standard windows headers
#include <windows.h>
#include <windef.h>
#include <dinput.h>

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

		using direct_input_create_proc = HRESULT(WINAPI*)(HINSTANCE instance, DWORD version, LPDIRECTINPUT *direct_input, LPUNKNOWN outer);
		HRESULT create_direct_input(direct_input_create_proc create_proc, HINSTANCE instance, DWORD version, LPDIRECTINPUT *direct_input, LPUNKNOWN outer);

		HRESULT direct_input_create_device(LPDIRECTINPUT direct_input, REFGUID device_guid, LPDIRECTINPUTDEVICE *device, LPUNKNOWN outer);

		HRESULT direct_input_enum_devices(LPDIRECTINPUT direct_input, DWORD device_type, LPDIENUMDEVICESCALLBACK callback, LPVOID context, DWORD flags);

		ULONG direct_input_release(LPDIRECTINPUT direct_input);

		HRESULT direct_input_device_enum_objects(LPDIRECTINPUTDEVICE device, LPDIENUMDEVICEOBJECTSCALLBACK callback, LPVOID context, DWORD flags);

		HRESULT direct_input_device_query_interface(LPDIRECTINPUTDEVICE device, REFIID interface_id, LPVOID *interface_pointer);

		ULONG direct_input_device_release(LPDIRECTINPUTDEVICE device);

		HRESULT direct_input_device_unacquire(LPDIRECTINPUTDEVICE device);

		HRESULT direct_input_device2_acquire(LPDIRECTINPUTDEVICE2 device);

		HRESULT direct_input_device2_get_device_state(LPDIRECTINPUTDEVICE2 device, DWORD data_size, LPVOID data);

		HRESULT direct_input_device2_poll(LPDIRECTINPUTDEVICE2 device);

		HRESULT direct_input_device2_set_cooperative_level(LPDIRECTINPUTDEVICE2 device, HWND window_handle, DWORD flags);

		HRESULT direct_input_device2_set_data_format(LPDIRECTINPUTDEVICE2 device, LPCDIDATAFORMAT data_format);

		HRESULT direct_input_device2_set_property(LPDIRECTINPUTDEVICE2 device, REFGUID property, LPCDIPROPHEADER property_header);
	}
}

#endif // MAMEUI_WINAPP_WINAPI_INPUT_H
