// license:BSD-3-Clause
// copyright-holders:0perator

#ifndef MAMEUI_WINAPP_WINAPI_MENUS_H
#define MAMEUI_WINAPP_WINAPI_MENUS_H

#pragma once

// standard windows headers
#include <windows.h>
#include <windef.h>
#include <winuser.h>

namespace mameui::winapi
{
	namespace menus
	{
		DWORD check_menu_item(HMENU menu_handle, UINT item_id, UINT check);

		HICON create_icon_from_resource_ex(PBYTE resource_bits, DWORD resource_size, BOOL for_icon, DWORD version, int x_desired, int y_desired, UINT flags);

		BOOL destroy_menu(HMENU menu_handle);

		BOOL enable_menu_item(HMENU menu_handle, UINT item_id, UINT flags);

		BOOL get_cursor_pos(LPPOINT position);

		HMENU get_menu(HWND window_handle);

		BOOL get_menu_item_info(HMENU menu_handle, UINT item_id, BOOL by_position, LPMENUITEMINFOW menu_item_info);

		HMENU get_submenu(HMENU menu_handle, int position);

		BOOL insert_menu_item(HMENU menu_handle, UINT item_id, BOOL by_position, LPMENUITEMINFOW menu_item_info);

		HACCEL load_accelerators(HINSTANCE instance_handle, const wchar_t *table_name);
		HACCEL load_accelerators_utf8(HINSTANCE instance_handle, const char *table_name);

		HCURSOR load_cursor(HINSTANCE instance_handle, const wchar_t *cursor_name);

		HICON load_icon(HINSTANCE instance_handle, const wchar_t *icon_name);
		HICON load_icon_utf8(HINSTANCE instance_handle, const char *icon_name);

		HANDLE load_image(HINSTANCE instance_handle, const wchar_t *image_name, UINT image_type, int x_position, int y_position, UINT flags);
		HANDLE load_image_utf8(HINSTANCE instance_handle, const char *image_name, UINT image_type, int x_position, int y_position, UINT flags);

		HMENU load_menu(HINSTANCE instance_handle, const wchar_t *menu_name);
		HMENU load_menu_utf8(HINSTANCE instance_handle, const char *menu_name);

		wchar_t *make_int_resource(int resource_id);

		BOOL set_cursor_pos(int x_position, int y_position);

		BOOL set_menu_item_info(HMENU menu_handle, UINT item_id, BOOL by_position, LPCMENUITEMINFOW menu_item_info);

		int show_cursor(BOOL show);

		BOOL track_popup_menu(HMENU menu_handle, UINT flags, int x_location, int y_location, HWND window_handle, LPCRECT rectangle);
	}
}

#endif // MAMEUI_WINAPP_WINAPI_MENUS_H
