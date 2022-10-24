// license:BSD-3-Clause
// copyright-holders:0perator

// ============================================================================
// windows_input.cpp - Wrapper functions for WIN32 input
// ============================================================================

// standard C++ headers
#include <string>

// standard windows headers
#include "winapi_common.h"

// MAMEUI headers

#include "windows_input.h"

// --------------------------------------------------------
// enable_window
// --------------------------------------------------------

BOOL mameui::winapi::input::enable_window(HWND window_handle, BOOL enable_window)
{
	return EnableWindow(window_handle, enable_window);
}

// --------------------------------------------------------
// get_active_window
// --------------------------------------------------------

HWND mameui::winapi::input::get_active_window()
{
	return GetActiveWindow();
}

// --------------------------------------------------------
// get_focus
// --------------------------------------------------------

HWND mameui::winapi::input::get_focus()
{
	return GetFocus();
}

// --------------------------------------------------------
// is_window_enabled
// --------------------------------------------------------

BOOL mameui::winapi::input::is_window_enabled(HWND window_handle)
{
	return IsWindowEnabled(window_handle);
}

// --------------------------------------------------------
// release_capture
// --------------------------------------------------------

BOOL mameui::winapi::input::release_capture(void)
{
	return ReleaseCapture();
}

// --------------------------------------------------------
// set_active_window
// --------------------------------------------------------

HWND mameui::winapi::input::set_active_window(HWND window_handle)
{
	return SetActiveWindow(window_handle);
}

// --------------------------------------------------------
// set_capture
// --------------------------------------------------------

HWND mameui::winapi::input::set_capture(HWND window_handle)
{
	return SetCapture(window_handle);
}

// --------------------------------------------------------
// set_focus
// --------------------------------------------------------

HWND mameui::winapi::input::set_focus(HWND window_handle)
{
	return SetFocus(window_handle);
}

// --------------------------------------------------------
// create_direct_input
// --------------------------------------------------------

HRESULT mameui::winapi::input::create_direct_input(direct_input_create_proc create_proc, HINSTANCE instance, DWORD version, LPDIRECTINPUT *direct_input, LPUNKNOWN outer)
{
	return create_proc(instance, version, direct_input, outer);
}

// --------------------------------------------------------
// direct_input_create_device
// --------------------------------------------------------

HRESULT mameui::winapi::input::direct_input_create_device(LPDIRECTINPUT direct_input, REFGUID device_guid, LPDIRECTINPUTDEVICE *device, LPUNKNOWN outer)
{
	return IDirectInput_CreateDevice(direct_input, device_guid, device, outer);
}

// --------------------------------------------------------
// direct_input_enum_devices
// --------------------------------------------------------

HRESULT mameui::winapi::input::direct_input_enum_devices(LPDIRECTINPUT direct_input, DWORD device_type, LPDIENUMDEVICESCALLBACK callback, LPVOID context, DWORD flags)
{
	return IDirectInput_EnumDevices(direct_input, device_type, callback, context, flags);
}

// --------------------------------------------------------
// direct_input_release
// --------------------------------------------------------

ULONG mameui::winapi::input::direct_input_release(LPDIRECTINPUT direct_input)
{
	return IDirectInput_Release(direct_input);
}

// --------------------------------------------------------
// direct_input_device_enum_objects
// --------------------------------------------------------

HRESULT mameui::winapi::input::direct_input_device_enum_objects(LPDIRECTINPUTDEVICE device, LPDIENUMDEVICEOBJECTSCALLBACK callback, LPVOID context, DWORD flags)
{
	return IDirectInputDevice_EnumObjects(device, callback, context, flags);
}

// --------------------------------------------------------
// direct_input_device_query_interface
// --------------------------------------------------------

HRESULT mameui::winapi::input::direct_input_device_query_interface(LPDIRECTINPUTDEVICE device, REFIID interface_id, LPVOID *interface_pointer)
{
	return IDirectInputDevice_QueryInterface(device, interface_id, interface_pointer);
}

// --------------------------------------------------------
// direct_input_device_release
// --------------------------------------------------------

ULONG mameui::winapi::input::direct_input_device_release(LPDIRECTINPUTDEVICE device)
{
	return IDirectInputDevice_Release(device);
}

// --------------------------------------------------------
// direct_input_device_unacquire
// --------------------------------------------------------

HRESULT mameui::winapi::input::direct_input_device_unacquire(LPDIRECTINPUTDEVICE device)
{
	return IDirectInputDevice_Unacquire(device);
}

// --------------------------------------------------------
// direct_input_device2_acquire
// --------------------------------------------------------

HRESULT mameui::winapi::input::direct_input_device2_acquire(LPDIRECTINPUTDEVICE2 device)
{
	return IDirectInputDevice2_Acquire(device);
}

// --------------------------------------------------------
// direct_input_device2_get_device_state
// --------------------------------------------------------

HRESULT mameui::winapi::input::direct_input_device2_get_device_state(LPDIRECTINPUTDEVICE2 device, DWORD data_size, LPVOID data)
{
	return IDirectInputDevice2_GetDeviceState(device, data_size, data);
}

// --------------------------------------------------------
// direct_input_device2_poll
// --------------------------------------------------------

HRESULT mameui::winapi::input::direct_input_device2_poll(LPDIRECTINPUTDEVICE2 device)
{
	return IDirectInputDevice2_Poll(device);
}

// --------------------------------------------------------
// direct_input_device2_set_cooperative_level
// --------------------------------------------------------

HRESULT mameui::winapi::input::direct_input_device2_set_cooperative_level(LPDIRECTINPUTDEVICE2 device, HWND window_handle, DWORD flags)
{
	return IDirectInputDevice2_SetCooperativeLevel(device, window_handle, flags);
}

// --------------------------------------------------------
// direct_input_device2_set_data_format
// --------------------------------------------------------

HRESULT mameui::winapi::input::direct_input_device2_set_data_format(LPDIRECTINPUTDEVICE2 device, LPCDIDATAFORMAT data_format)
{
	return IDirectInputDevice2_SetDataFormat(device, data_format);
}

// --------------------------------------------------------
// direct_input_device2_set_property
// --------------------------------------------------------

HRESULT mameui::winapi::input::direct_input_device2_set_property(LPDIRECTINPUTDEVICE2 device, REFGUID property, LPCDIPROPHEADER property_header)
{
	return IDirectInputDevice2_SetProperty(device, property, property_header);
}
