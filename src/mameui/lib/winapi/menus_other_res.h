// license:BSD-3-Clause
// copyright-holders:0perator

#ifndef MAMEUI_LIB_WINAPI_MENUS_OTHER_RES_H
#define MAMEUI_LIB_WINAPI_MENUS_OTHER_RES_H

namespace mameui::winapi::menus
{
	BOOL append_menu(HMENU menu_handle, UINT flags, UINT_PTR item_id, const wchar_t* content);
	BOOL append_menu_utf8(HMENU menu_handle, UINT flags, UINT_PTR item_id, const char* content);

	DWORD check_menu_item(HMENU menu_handle, UINT item_id, UINT check);

	BOOL check_menu_radio_item(HMENU menu_handle, UINT first_item, UINT last_item, UINT check, UINT  flags);

	HMENU create_menu(void);

	HICON create_icon_from_resource_ex(PBYTE resource_bits, DWORD resource_size, BOOL for_icon, DWORD version, int x_desired = 0, int y_desired = 0, UINT flags = 0);

	BOOL destroy_icon(HICON icon_handle);

	BOOL destroy_menu(HMENU menu_handle);

	BOOL draw_menu_bar(HWND window_handle);

	BOOL enable_menu_item(HMENU menu_handle, UINT item_id, UINT flags = 0);

	BOOL get_cursor_pos(LPPOINT position);

	HMENU get_menu(HWND window_handle);

	int get_menu_item_count(HMENU menu_handle);

	BOOL get_menu_item_info(HMENU menu_handle, UINT item_id, BOOL by_position, LPMENUITEMINFOW menu_item_info);

	HMENU get_submenu(HMENU menu_handle, int position);

	BOOL insert_menu(HMENU menu_handle, UINT position, UINT flags, UINT_PTR item_id, const wchar_t *new_item);
	BOOL insert_menu_utf8(HMENU menu_handle, UINT position, UINT flags, UINT_PTR item_id, const char *new_item);

	BOOL insert_menu_item(HMENU menu_handle, UINT item_id, BOOL by_position, LPMENUITEMINFOW menu_item_info);

	HACCEL load_accelerators(HINSTANCE instance_handle, const wchar_t *table_name);
	HACCEL load_accelerators_utf8(HINSTANCE instance_handle, const char *table_name);

	HCURSOR load_cursor(HINSTANCE instance_handle, const wchar_t *cursor_name);
	HCURSOR load_cursor_utf8(HINSTANCE instance_handle, const char *cursor_name);

	HICON load_icon(HINSTANCE instance_handle, const wchar_t *icon_name);
	HICON load_icon_utf8(HINSTANCE instance_handle, const char *icon_name);

	HANDLE load_image(HINSTANCE instance_handle, const wchar_t *image_name, UINT image_type = 0, int x_position = 0, int y_position = 0, UINT flags = 0);
	HANDLE load_image_utf8(HINSTANCE instance_handle, const char *image_name, UINT image_type = 0, int x_position = 0, int y_position = 0, UINT flags = 0);

	HMENU load_menu(HINSTANCE instance_handle, const wchar_t *menu_name);
	HMENU load_menu_utf8(HINSTANCE instance_handle, const char *menu_name);

	wchar_t *make_int_resource(int resource_id);
	char *make_int_resource_utf8(int resource_id);

	BOOL remove_menu(HMENU menu_handle, UINT item_ref, UINT flags);

	HCURSOR set_cursor(HCURSOR cursor_handle);

	BOOL set_cursor_pos(int x_position, int y_position);

	BOOL set_menu(HWND window_handle, HMENU menu_handle);

	BOOL set_menu_item_info(HMENU menu_handle, UINT item_id, BOOL by_position, LPCMENUITEMINFOW menu_item_info);
	BOOL set_menu_item_info_utf8(HMENU menu_handle, UINT item_id, BOOL by_position, LPCMENUITEMINFOA menu_item_info);

	int show_cursor(BOOL show);

	BOOL track_popup_menu(HMENU menu_handle, UINT flags, int x_location, int y_location, int reserved, HWND window_handle, LPCRECT rectangle);
	BOOL track_popup_menu(HMENU menu_handle, UINT flags, int x_location, int y_location, HWND window_handle, LPCRECT rectangle);
}

#endif // MAMEUI_LIB_WINAPI_MENUS_OTHER_RES_H
