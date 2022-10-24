// license:BSD-3-Clause
// copyright-holders:0perator

#ifndef MAMEUI_LIB_WINAPI_WINDOWS_MESSAGES_H
#define MAMEUI_LIB_WINAPI_WINDOWS_MESSAGES_H

namespace mameui::winapi::windows
{
	BOOL adjust_window_rect_ex(LPRECT rectangle, DWORD style, BOOL menu, DWORD extended_style);

	LRESULT call_window_proc(WNDPROC previous_procedure, HWND window_handle, UINT message, WPARAM word_parameter, LPARAM long_parameter);

	HWND create_window_ex(DWORD extended_style, const wchar_t *class_name, const wchar_t *window_name, DWORD style, int x_position, int y_position, int width, int height, HWND parent = 0, HMENU menu_handle = 0, HINSTANCE instance_handle = 0, LPVOID create_struct = 0);
	HWND create_window_ex_utf8(DWORD extended_style, const char *class_name, const char *window_name, DWORD style, int x_position, int y_position, int width, int height, HWND parent = 0, HMENU menu_handle = 0, HINSTANCE instance_handle = 0, LPVOID create_struct = 0);

	LRESULT def_window_proc(HWND window_handle, UINT message, WPARAM word_parameter, LPARAM long_parameter);

	BOOL destroy_window(HWND window_handle);

	BOOL enable_window(HWND window_handle, BOOL enabled);

	HWND find_window(const wchar_t *class_name = 0, const wchar_t *window_name = 0);
	HWND find_window_utf8(const char *class_name = 0, const char *window_name = 0);

	HWND find_window_ex(HWND parent_window_handle = 0, HWND child_window_handle = 0, const wchar_t *class_name = 0, const wchar_t *window_name = 0);
	HWND find_window_ex_utf8(HWND parent_window_handle = 0, HWND child_window_handle = 0, const char* class_name = 0, const char *window_name = 0);

	DWORD format_message(DWORD formatting_flags, const void *source, DWORD message_id, DWORD language_id, wchar_t *buffer, DWORD buffer_size, va_list *arguments);
	DWORD format_message_utf8(DWORD formatting_flags, const void *source, DWORD message_id, DWORD language_id, char *buffer, DWORD buffer_size, va_list *arguments);

	int get_classname(HWND window_handle, wchar_t *class_name, int max_count);
	int get_classname_utf8(HWND window_handle, char *class_name, int max_count);

	BOOL get_client_rect(HWND window_handle, LPRECT rectangle);

	HWND get_desktop_window(void);

	HWND get_parent(HWND window_handle);

	HANDLE get_prop(HWND window_handle, const wchar_t *string);
	HANDLE get_prop_utf8(HWND window_handle, const char *string);

	BOOL get_message(LPMSG message_struct, HWND window_handle, UINT filter_lowest_value, UINT filter_highest_value);
	BOOL get_message_utf8(LPMSG message_struct, HWND window_handle, UINT filter_lowest_value, UINT filter_highest_value);

	DWORD get_message_pos(void);

	DWORD get_sys_color(int display_element);

	int get_system_metrics(int selected_metric);

	HWND get_window(HWND window_handle, UINT relationship);

	HFONT get_window_font(HWND window_handle);

	LONG_PTR get_window_long_ptr(HWND window_handle, int index);
	LONG_PTR get_window_long_ptr_utf8(HWND window_handle, int value_index);

	BOOL get_window_placement(HWND window_handle, LPWINDOWPLACEMENT window_placement);

	BOOL get_window_rect(HWND window_handle, LPRECT rectangle);

	int get_window_text(HWND window_handle, wchar_t *text, int max_count);
	wchar_t *get_window_text(HWND window_handle);

	int get_window_text_utf8(HWND window_handle, char *text, int max_count);
	char *get_window_text_utf8(HWND window_handle);

	int get_window_text_length(HWND window_handle);
	int get_window_text_length_utf8(HWND window_handle);

	BOOL is_window(HWND window_handle);

	BOOL is_window_visible(HWND window_handle);

	BOOL kill_timer(HWND window_handle, UINT_PTR timer_id);

	BOOL move_window(HWND window_handle, int x, int y, int width, int height, BOOL repaint);

	BOOL post_message(HWND window_handle, UINT message, WPARAM word_parameter, LPARAM long_parameter);
	BOOL post_message_utf8(HWND window_handle, UINT message, WPARAM word_parameter, LPARAM long_parameter);

	void post_quiet_message(int exit_code);

	ATOM register_class(const LPWNDCLASSW window_class_struct);
	ATOM register_class_utf8(const LPWNDCLASSA window_class_struct);

	LRESULT send_message(HWND window_handle, UINT message, WPARAM word_parameter, LPARAM long_parameter);
	LRESULT send_message_utf8(HWND window_handle, UINT message, WPARAM word_parameter, LPARAM long_parameter);

	HWND set_active_window(HWND window_handle);

	BOOL set_foreground_window(HWND window_handle);

	BOOL set_prop(HWND window_handle, const wchar_t *string, HANDLE data);
	BOOL set_prop_utf8(HWND window_handle, const char *string, HANDLE data);

	UINT_PTR set_timer(HWND window_handle, UINT_PTR timer_id, UINT time_out_value, TIMERPROC procedure);

	void set_window_font(HWND window_handle, HFONT font, BOOL redraw);

	LONG_PTR set_window_long_ptr(HWND window_handle, int index, LONG_PTR new_long);
	LONG_PTR set_window_long_ptr_utf8(HWND window_handle, int value_index, LONG_PTR new_value);

	BOOL set_window_pos(HWND window_handle, HWND preceding_window_handle, int x_position, int y_position, int width, int height, UINT flags);

	BOOL set_window_text(HWND window_handle, const wchar_t *text);
	BOOL set_window_text_utf8(HWND window_handle, const char *text);

	BOOL show_window(HWND window_handle, int command_to_show);

	BOOL unregister_class(const wchar_t *classname, HINSTANCE instance_handle);
	BOOL unregister_class_utf8(const char *classname, HINSTANCE instance_handle);
}

#endif // MAMEUI_LIB_WINAPI_WINDOWS_MESSAGES_H
