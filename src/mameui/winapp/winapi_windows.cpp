// license:BSD-3-Clause
// copyright-holders:0perator

/***************************************************************************

  winapi_windows.cpp

 ***************************************************************************/

// standard windows headers

// standard C++ headers
#include <cstdarg>
#include <memory>

// MAME/MAMEUI headers
#include "mui_wcsconv.h"
#include "winapi_windows.h"


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
	HWND result = 0;
	std::unique_ptr<wchar_t[]>  wcs_class_name(mui_wcstring_from_utf8(class_name));

	if (!wcs_class_name)
		return result;

	std::unique_ptr<wchar_t[]>  wcs_window_name(mui_wcstring_from_utf8(window_name));

	if (!wcs_window_name)
		return result;

	result = create_window_ex(extended_style, wcs_class_name.get(), wcs_window_name.get(), style, x_position, y_position, width, height, parent, menu_handle, instance_handle, create_struct);

	return result;
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
//  format_message
//============================================================

DWORD mameui::winapi::windows::format_message(DWORD formatting_flags, const void *source, DWORD message_id, DWORD language_id, wchar_t *buffer, DWORD buffer_size, va_list *arguments)
{
	return FormatMessageW(formatting_flags, source, message_id, language_id, buffer, buffer_size, arguments);
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
	int result = 0;
	std::unique_ptr<wchar_t[]>  wcs_classname(mui_wcstring_from_utf8(classname));

	if (!wcs_classname)
		return result;

	result = get_classname(window_handle, wcs_classname.get(), max_count);

	if (result)
		result = mui_utf8_from_wcstring(classname, wcs_classname.get());

	return result;
}


//============================================================
//  get_client_rect
//============================================================

BOOL mameui::winapi::windows::get_client_rect(HWND window_handle, LPRECT rectangle)
{
	return GetClientRect(window_handle, rectangle);
}


//============================================================
//  get_message
//============================================================

BOOL mameui::winapi::windows::get_message(LPMSG message_struct, HWND window_handle, UINT filter_lowest_value, UINT filter_highest_value)
{
	return GetMessageW(message_struct, window_handle, filter_lowest_value, filter_highest_value);
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
//  get_window_rect
//============================================================

BOOL mameui::winapi::windows::get_window_rect(HWND window_handle, LPRECT rectangle)
{
	return GetWindowRect(window_handle, rectangle);
}


//============================================================
// get_window_text
//============================================================

int mameui::winapi::windows::get_window_text(HWND window_handle, wchar_t *text, int max_count)
{
	return GetWindowTextW(window_handle, text, max_count);
}

wchar_t *mameui::winapi::windows::get_window_text(HWND window_handle)
{
	int text_length = 0;
	wchar_t *text = 0;

	text_length = get_window_text_length(window_handle);
	if (!text_length)
		return text;
	else
		text_length++;

	text = new wchar_t[text_length];

	text_length = get_window_text(window_handle, text, text_length);

	return (!text_length) ? (wchar_t *)0 : text;
}

//============================================================
// get_window_text_utf8
//============================================================

int mameui::winapi::windows::get_window_text_utf8(HWND window_handle, char *text, int max_count)
{
	int result = 0;
	std::unique_ptr<wchar_t[]>wcs_text(mui_wcstring_from_utf8(text));

	if (!wcs_text)
		return result;

	result = get_window_text(window_handle, wcs_text.get(), max_count);
	if (!result)
		return result;

	result = mui_utf8_from_wcstring(text, wcs_text.get());

	return result;
}

char *mameui::winapi::windows::get_window_text_utf8(HWND window_handle)
{
	int text_length = 0;
	char* text = 0;

	text_length = get_window_text_length(window_handle);
	if (!text_length)
		return text;
	else
		text_length++;

	text = new char[text_length];

	text_length = get_window_text_utf8(window_handle, text, text_length);

	return (!text_length) ? (char *)0 : text;
}

//============================================================
//  get_window_text_length
//============================================================

int mameui::winapi::windows::get_window_text_length(HWND window_handle)
{
	return GetWindowTextLengthW(window_handle);
}


//============================================================
//  is_window_enabled
//============================================================

BOOL mameui::winapi::windows::is_window_enabled(HWND window_handle)
{
	return IsWindowEnabled(window_handle);
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
//  send_message
//============================================================

LRESULT mameui::winapi::windows::send_message(HWND window_handle, UINT message, WPARAM word_parameter, LPARAM long_parameter)
{
	return SendMessageW(window_handle, message, word_parameter, long_parameter);
}


//============================================================
//  set_window_font
//============================================================

void mameui::winapi::windows::set_window_font(HWND window_handle, HFONT font, BOOL redraw)
{
	(void)send_message(window_handle, WM_SETFONT, (WPARAM)font, MAKELPARAM(redraw, 0));
}


//============================================================
//  set_foreground_window
//============================================================

BOOL mameui::winapi::windows::set_foreground_window(HWND window_handle)
{
	return SetForegroundWindow(window_handle);
}


//============================================================
// set_timer
//============================================================

UINT_PTR mameui::winapi::windows::set_timer(HWND window_handle, UINT_PTR event_id, UINT time_out_value, TIMERPROC procedure)
{
	return SetTimer(window_handle, event_id, time_out_value, procedure);
}


//============================================================
// set_window_long_ptr
//============================================================

LONG_PTR mameui::winapi::windows::set_window_long_ptr(HWND window_handle, int value_index, LONG_PTR new_value)
{
	return SetWindowLongPtrW(window_handle, value_index, new_value);
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

BOOL mameui::winapi::windows::set_window_text(HWND window_handle, const wchar_t *text)
{
	return SetWindowTextW(window_handle, text);
}


//============================================================
// set_window_text_utf8
//============================================================

BOOL mameui::winapi::windows::set_window_text_utf8(HWND window_handle, const char* text)
{
	std::unique_ptr<wchar_t[]>wcs_text(mui_wcstring_from_utf8(text));

	if (!wcs_text)
		return (BOOL)0;

	return set_window_text(window_handle, wcs_text.get());
}


//============================================================
// show_window
//============================================================

BOOL mameui::winapi::windows::show_window(HWND window_handle, int command_to_show)
{
	return ShowWindow(window_handle, command_to_show);
}
