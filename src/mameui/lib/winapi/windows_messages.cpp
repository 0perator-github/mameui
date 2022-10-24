//
// license:BSD-3-Clause
// copyright-holders:0perator

//===========================================================================
//
// windows_messages.cpp - Wrapper functions for Win32 API windows and messages
//
//===========================================================================

// standard C++ headers
#include <cstddef>
#include <memory>
#include <new>
#include <string_view>
#include <string>

// standard windows headers
#include "winapi_common.h"

// MAMEUI headers
#include "mui_wcstrconv.h"

#include "windows_messages.h"

//============================================================
//adjust_window_rect_ex
//============================================================

BOOL mameui::winapi::windows::adjust_window_rect_ex(LPRECT rectangle,DWORD style,BOOL menu,DWORD extended_style)
{
	return AdjustWindowRectEx(rectangle, style, menu, extended_style);
}

//============================================================
// call_window_proc
//============================================================

LRESULT mameui::winapi::windows::call_window_proc(WNDPROC previous_procedure, HWND window_handle, UINT message, WPARAM word_parameter, LPARAM long_parameter)
{
	return CallWindowProcW(previous_procedure, window_handle, message, word_parameter, long_parameter);
}

//============================================================
// create_window_ex
//============================================================

HWND mameui::winapi::windows::create_window_ex(DWORD extended_style, const wchar_t* class_name, const wchar_t* window_name, DWORD style, int x_position, int y_position, int width, int height, HWND parent, HMENU menu_handle, HINSTANCE instance_handle, LPVOID create_struct)
{
	return CreateWindowExW(extended_style, class_name, window_name,style,x_position,y_position,width,height,parent,menu_handle,instance_handle,create_struct);
}

//============================================================
// create_window_ex_utf8
//============================================================

HWND mameui::winapi::windows::create_window_ex_utf8(DWORD extended_style, const char* class_name, const char* window_name, DWORD style, int x_position, int y_position, int width, int height, HWND parent, HMENU menu_handle, HINSTANCE instance_handle, LPVOID create_struct)
{
	return CreateWindowExA(extended_style, class_name, window_name, style, x_position, y_position, width, height, parent, menu_handle, instance_handle, create_struct);
}

//============================================================
// def_window_proc
//============================================================

LRESULT mameui::winapi::windows::def_window_proc(HWND window_handle, UINT message, WPARAM word_parameter, LPARAM long_parameter)
{
	return DefWindowProcW(window_handle, message, word_parameter, long_parameter);
}

//============================================================
// destroy_window
//============================================================

BOOL mameui::winapi::windows::destroy_window(HWND window_handle)
{
	return DestroyWindow(window_handle);
}

//============================================================
// enable_window
//============================================================

BOOL mameui::winapi::windows::enable_window(HWND window_handle,BOOL enabled)
{
	return EnableWindow(window_handle, enabled);
}

//============================================================
// find_window
//============================================================

HWND mameui::winapi::windows::find_window(const wchar_t* class_name, const wchar_t* window_name)
{
	return FindWindowW(class_name, window_name);
}

//============================================================
// find_window_utf8
//============================================================

HWND mameui::winapi::windows::find_window_utf8(const char* class_name, const char* window_name)
{
	return FindWindowA(class_name, window_name);
}

//============================================================
// find_window_ex
//============================================================

HWND mameui::winapi::windows::find_window_ex(HWND parent_window_handle, HWND child_window_handle, const wchar_t *class_name, const wchar_t *window_name)
{
	return FindWindowExW(parent_window_handle, child_window_handle, class_name, window_name);
}

//============================================================
// find_window_ex_utf8
//============================================================

HWND mameui::winapi::windows::find_window_ex_utf8(HWND parent_window_handle, HWND child_window_handle, const char *class_name, const char *window_name)
{
	return FindWindowExA(parent_window_handle, child_window_handle, class_name, window_name);
}

//============================================================
//  format_message
//============================================================

DWORD mameui::winapi::windows::format_message(DWORD formatting_flags, const void *source, DWORD message_id, DWORD language_id, wchar_t *buffer, DWORD buffer_size, va_list *arguments)
{
	return FormatMessageW(formatting_flags, source, message_id, language_id, buffer, buffer_size, arguments);
}

//============================================================
//  format_message_utf8
//============================================================

DWORD mameui::winapi::windows::format_message_utf8(DWORD formatting_flags, const void *source, DWORD message_id, DWORD language_id, char *buffer, DWORD buffer_size, va_list *arguments)
{
	return FormatMessageA(formatting_flags, source, message_id, language_id, buffer, buffer_size, arguments);
}

//============================================================
//  get_classname
//============================================================

int mameui::winapi::windows::get_classname(HWND window_handle, wchar_t *classname, int max_count)
{
	return GetClassNameW(window_handle, classname, max_count);
}

//============================================================
//  get_classname_utf8
//============================================================

int mameui::winapi::windows::get_classname_utf8(HWND window_handle, char *classname, int max_count)
{
	return GetClassNameA(window_handle, classname, max_count);
}

//============================================================
//  get_client_rect
//============================================================

BOOL mameui::winapi::windows::get_client_rect(HWND window_handle, LPRECT rectangle)
{
	return GetClientRect(window_handle, rectangle);
}

//============================================================
//  get_desktop_window
//============================================================

HWND mameui::winapi::windows::get_desktop_window(void)
{
	return GetDesktopWindow();
}

//============================================================
//  get_parent
//============================================================

HWND mameui::winapi::windows::get_parent(HWND window_handle)
{
	return GetParent(window_handle);
}

//============================================================
//  get_prop
//============================================================

HANDLE mameui::winapi::windows::get_prop(HWND window_handle, const wchar_t *string)
{
	return GetPropW(window_handle, string);
}

//============================================================
//  get_prop_utf8
//============================================================

HANDLE mameui::winapi::windows::get_prop_utf8(HWND window_handle, const char *string)
{
	return GetPropA(window_handle, string);
}

//============================================================
//  get_message
//============================================================

BOOL mameui::winapi::windows::get_message(LPMSG message_struct, HWND window_handle, UINT filter_lowest_value, UINT filter_highest_value)
{
	return GetMessageW(message_struct, window_handle, filter_lowest_value, filter_highest_value);
}

//============================================================
//  get_message_utf8
//============================================================

BOOL mameui::winapi::windows::get_message_utf8(LPMSG message_struct, HWND window_handle, UINT filter_lowest_value, UINT filter_highest_value)
{
	return GetMessageA(message_struct, window_handle, filter_lowest_value, filter_highest_value);
}

//============================================================
//  get_message_pos
//============================================================

DWORD mameui::winapi::windows::get_message_pos(void)
{
	return GetMessagePos();
}

//============================================================
//  get_sys_color
//============================================================

DWORD mameui::winapi::windows::get_sys_color(int display_element)
{
	return GetSysColor(display_element);
}

//============================================================
//  get_system_metrics
//============================================================

int mameui::winapi::windows::get_system_metrics(int selected_metric)
{
	return GetSystemMetrics(selected_metric);
}

//============================================================
//  get_window
//============================================================

HWND mameui::winapi::windows::get_window(HWND window_handle, UINT relationship)
{
	return GetWindow(window_handle, relationship);
}

//============================================================
//  get_window_font
//============================================================

HFONT mameui::winapi::windows::get_window_font(HWND window_handle)
{
	return (HFONT)send_message(window_handle, WM_GETFONT, 0, 0L);
}

//============================================================
//  get_window_long_ptr
//============================================================

LONG_PTR mameui::winapi::windows::get_window_long_ptr(HWND window_handle, int value_index)
{
	return GetWindowLongPtrW(window_handle, value_index);
}

//============================================================
//  get_window_long_ptr_utf8
//============================================================

LONG_PTR mameui::winapi::windows::get_window_long_ptr_utf8(HWND window_handle, int value_index)
{
	return GetWindowLongPtrA(window_handle, value_index);
}

//============================================================
//  get_window_placement
//============================================================

BOOL mameui::winapi::windows::get_window_placement(HWND window_handle, LPWINDOWPLACEMENT window_placement)
{
	return GetWindowPlacement(window_handle, window_placement);
}

//============================================================
//  get_window_rect
//============================================================

BOOL mameui::winapi::windows::get_window_rect(HWND window_handle, LPRECT rectangle)
{
	return GetWindowRect(window_handle, rectangle);
}

//============================================================
// get_window_text
//============================================================

int mameui::winapi::windows::get_window_text(HWND window_handle, wchar_t* text, int max_count)
{
	return GetWindowTextW(window_handle, text, max_count);
}

wchar_t* mameui::winapi::windows::get_window_text(HWND window_handle)
{
	int text_length = get_window_text_length(window_handle);
	if (text_length <= 0)
		return nullptr;

	wchar_t *text = new(std::nothrow) wchar_t[text_length + 1];
	if (!text)
		return nullptr;

	int wchars_copied = get_window_text(window_handle, text, text_length + 1);
	if (wchars_copied <= 0)
	{
		delete[] text;
		return nullptr;
	}

	text[wchars_copied] = '\0';

	return text;
}

//============================================================
// get_window_text_utf8
//============================================================

int mameui::winapi::windows::get_window_text_utf8(HWND window_handle, char* text, int max_count)
{
	return GetWindowTextA(window_handle, text, max_count);
}

char *mameui::winapi::windows::get_window_text_utf8(HWND window_handle)
{
	int text_length = get_window_text_length_utf8(window_handle);
	if (text_length <= 0)
		return nullptr;

	char *text = new(std::nothrow) char[text_length + 1];
	if (!text)
		return nullptr;

	int chars_copied = get_window_text_utf8(window_handle, text, text_length + 1);
	if (chars_copied <= 0)
	{
		delete[] text;
		return nullptr;
	}

	text[chars_copied] = '\0';

	return text;
}

//============================================================
//  get_window_text_length
//============================================================

int mameui::winapi::windows::get_window_text_length(HWND window_handle)
{
	return GetWindowTextLengthW(window_handle);
}

//============================================================
//  get_window_text_length_utf8
//============================================================

int mameui::winapi::windows::get_window_text_length_utf8(HWND window_handle)
{
	return GetWindowTextLengthA(window_handle);
}

//============================================================
//  is_window
//============================================================

BOOL mameui::winapi::windows::is_window(HWND window_handle)
{
	return IsWindow(window_handle);
}

//============================================================
//  is_window_visible
//============================================================

BOOL mameui::winapi::windows::is_window_visible(HWND window_handle)
{
	return IsWindowVisible(window_handle);
}

//============================================================
//  kill_timer
//============================================================

BOOL mameui::winapi::windows::kill_timer(HWND window_handle, UINT_PTR timer_id)
{
	return KillTimer(window_handle, timer_id);
}

//============================================================
//  move_window
//============================================================

BOOL mameui::winapi::windows::move_window(HWND window_handle, int x_location, int y_location, int width, int height, BOOL redraw)
{
	return MoveWindow(window_handle, x_location, y_location, width, height, redraw);
}

//============================================================
//  post_message
//============================================================

BOOL mameui::winapi::windows::post_message(HWND window_handle, UINT message, WPARAM word_parameter, LPARAM long_parameter)
{
	return PostMessageW(window_handle, message, word_parameter, long_parameter);
}

//============================================================
//  post_message_utf8
//============================================================

BOOL mameui::winapi::windows::post_message_utf8(HWND window_handle, UINT message, WPARAM word_parameter, LPARAM long_parameter)
{
	return PostMessageA(window_handle, message, word_parameter, long_parameter);
}

//============================================================
//  post_quiet_message
//============================================================

void mameui::winapi::windows::post_quiet_message(int exit_code)
{
	PostQuitMessage(exit_code);
}

//============================================================
//  register_class
//============================================================

ATOM mameui::winapi::windows::register_class(const LPWNDCLASSW window_class_struct)
{
	return RegisterClassW(window_class_struct);
}

//============================================================
//  register_class_utf8
//============================================================

ATOM mameui::winapi::windows::register_class_utf8(const LPWNDCLASSA window_class_struct)
{
	return RegisterClassA(window_class_struct);
}

//============================================================
//  send_message
//============================================================

LRESULT mameui::winapi::windows::send_message(HWND window_handle, UINT message, WPARAM word_parameter, LPARAM long_parameter)
{
	return SendMessageW(window_handle, message, word_parameter, long_parameter);
}

//============================================================
//  send_message_utf8
//============================================================

LRESULT mameui::winapi::windows::send_message_utf8(HWND window_handle, UINT message, WPARAM word_parameter, LPARAM long_parameter)
{
	return SendMessageA(window_handle, message, word_parameter, long_parameter);
}

//============================================================
//  set_active_window
//============================================================

HWND mameui::winapi::windows::set_active_window(HWND window_handle)
{
	return SetActiveWindow(window_handle);
}

//============================================================
//  set_foreground_window
//============================================================

BOOL mameui::winapi::windows::set_foreground_window(HWND window_handle)
{
	return SetForegroundWindow(window_handle);
}

//============================================================
//  set_prop
//============================================================

BOOL mameui::winapi::windows::set_prop(HWND window_handle, const wchar_t *string, HANDLE data)
{
	return SetPropW(window_handle, string, data);
}

//============================================================
//  set_prop_utf8
//============================================================

BOOL mameui::winapi::windows::set_prop_utf8(HWND window_handle, const char *string, HANDLE data)
{
	return SetPropA(window_handle, string, data);
}

//============================================================
// set_timer
//============================================================

UINT_PTR mameui::winapi::windows::set_timer(HWND window_handle, UINT_PTR event_id, UINT time_out_value, TIMERPROC procedure)
{
	return SetTimer(window_handle, event_id, time_out_value, procedure);
}

//============================================================
//  set_window_font
//============================================================

void mameui::winapi::windows::set_window_font(HWND window_handle, HFONT font, BOOL redraw)
{
	(void)send_message(window_handle, WM_SETFONT, (WPARAM)font, MAKELPARAM(redraw, 0));
}

//============================================================
// set_window_long_ptr
//============================================================

LONG_PTR mameui::winapi::windows::set_window_long_ptr(HWND window_handle, int value_index, LONG_PTR new_value)
{
	return SetWindowLongPtrW(window_handle, value_index, new_value);
}

//============================================================
// set_window_long_ptr_utf8
//============================================================

LONG_PTR mameui::winapi::windows::set_window_long_ptr_utf8(HWND window_handle, int value_index, LONG_PTR new_value)
{
	return SetWindowLongPtrA(window_handle, value_index, new_value);
}

//============================================================
// set_window_pos
//============================================================

BOOL mameui::winapi::windows::set_window_pos(HWND window_handle, HWND preceding_window_handle, int x_position, int y_position, int width, int height, UINT flags)
{
	return SetWindowPos(window_handle, preceding_window_handle, x_position, y_position, width, height, flags);
}

//============================================================
// set_window_text
//============================================================

BOOL mameui::winapi::windows::set_window_text(HWND window_handle, const wchar_t* text)
{
	return SetWindowTextW(window_handle, text);
}

//============================================================
// set_window_text_utf8
//============================================================

BOOL mameui::winapi::windows::set_window_text_utf8(HWND window_handle, const char* text)
{
	return SetWindowTextA(window_handle, text);
}

//============================================================
// show_window
//============================================================

BOOL mameui::winapi::windows::show_window(HWND window_handle, int command_to_show)
{
	return ShowWindow(window_handle, command_to_show);
}

//============================================================
//  unregister_class
//============================================================

BOOL mameui::winapi::windows::unregister_class(const wchar_t *classname, HINSTANCE instance_handle)
{
	return UnregisterClassW(classname, instance_handle);
}

//============================================================
//  unregister_class_utf8
//============================================================

BOOL mameui::winapi::windows::unregister_class_utf8(const char* classname, HINSTANCE instance_handle)
{
	return UnregisterClassA(classname, instance_handle);
}
