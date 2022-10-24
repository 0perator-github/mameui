// license:BSD-3-Clause
// copyright-holders:0perator

//===========================================================================
//
// menus_other_res.cpp - WIN32 functions to handle menus and other resources
//
//===========================================================================

// standard C++ headers

// standard windows headers
#include "winapi_common.h"

// MAME headers

// MAMEUI headers

#include "menus_other_res.h"

//============================================================
// append_menu
//============================================================

BOOL mameui::winapi::menus::append_menu(HMENU menu_handle, UINT flags, UINT_PTR item_id, const wchar_t *content)
{
	return AppendMenuW(menu_handle, flags, item_id, content);
}

//============================================================
// append_menu_utf8
//============================================================

BOOL mameui::winapi::menus::append_menu_utf8(HMENU menu_handle, UINT flags, UINT_PTR item_id, const char *content)
{
	return AppendMenuA(menu_handle, flags, item_id, content);
}

//============================================================
// check_menu_item
//============================================================

DWORD mameui::winapi::menus::check_menu_item(HMENU menu_handle,UINT item_id, UINT check)
{
	return CheckMenuItem(menu_handle, item_id, check);
}

//============================================================
// check_menu_radio_item
//============================================================

BOOL mameui::winapi::menus::check_menu_radio_item(HMENU menu_handle, UINT first_item, UINT last_item, UINT check, UINT  flags)
{
	return  CheckMenuRadioItem(menu_handle, first_item, last_item, check, flags);
}

//============================================================
// create_menu
//============================================================

HMENU mameui::winapi::menus::create_menu(void)
{
	return CreateMenu();
}

//============================================================
// create_icon_from_resource_ex
//============================================================

HICON mameui::winapi::menus::create_icon_from_resource_ex(PBYTE resource_bits, DWORD resource_size, BOOL for_icon,
	DWORD version, int x_desired, int y_desired, UINT flags)
{
	return CreateIconFromResourceEx(resource_bits, resource_size, for_icon, version, x_desired, y_desired, flags);
}

//============================================================
// destroy_icon
//============================================================

BOOL mameui::winapi::menus::destroy_icon(HICON icon_handle)
{
	return DestroyIcon(icon_handle);
}

//============================================================
// destroy_menu
//============================================================

BOOL mameui::winapi::menus::destroy_menu(HMENU menu_handle)
{
	return DestroyMenu(menu_handle);
}

//============================================================
// draw_menu_bar
//============================================================

BOOL mameui::winapi::menus::draw_menu_bar(HWND window_handle)
{
	return DrawMenuBar(window_handle);
}

//============================================================
// enable_menu_item
//============================================================

BOOL mameui::winapi::menus::enable_menu_item(HMENU menu_handle,UINT item_id, UINT flags)
{
	return EnableMenuItem(menu_handle, item_id, flags);
}

//============================================================
//  get_cursor_pos
//============================================================

BOOL mameui::winapi::menus::get_cursor_pos(LPPOINT position)
{
	return GetCursorPos(position);
}

//============================================================
//  get_menu
//============================================================

HMENU mameui::winapi::menus::get_menu(HWND window_handle)
{
	return GetMenu(window_handle);
}

//============================================================
//  get_menu_item_count
//============================================================

int mameui::winapi::menus::get_menu_item_count(HMENU menu_handle)
{
	return GetMenuItemCount(menu_handle);
}

//============================================================
//  get_menu_item_info
//============================================================

BOOL mameui::winapi::menus::get_menu_item_info(HMENU menu_handle, UINT item_id, BOOL by_position, LPMENUITEMINFOW menu_item_info)
{
	return GetMenuItemInfoW(menu_handle, item_id, by_position, menu_item_info);
}

//============================================================
//  get_submenu
//============================================================

HMENU mameui::winapi::menus::get_submenu(HMENU menu_handle, int position)
{
	return GetSubMenu(menu_handle, position);
}

//============================================================
//  insert_menu
//============================================================

BOOL mameui::winapi::menus::insert_menu(HMENU menu_handle, UINT position, UINT flags, UINT_PTR item_id, const wchar_t *new_item)
{
	return InsertMenuW(menu_handle, position, flags, item_id, new_item);
}

//============================================================
//  insert_menu_utf8
//============================================================

BOOL mameui::winapi::menus::insert_menu_utf8(HMENU menu_handle, UINT position, UINT flags, UINT_PTR item_id, const char* new_item)
{
	return InsertMenuA(menu_handle, position, flags, item_id, new_item);
}

//============================================================
//  insert_menu_item
//============================================================

BOOL mameui::winapi::menus::insert_menu_item(HMENU menu_handle, UINT item_id, BOOL by_position, LPMENUITEMINFOW menu_item_info)
{
	return InsertMenuItemW(menu_handle, item_id, by_position, menu_item_info);
}

//============================================================
//  load_accelerators
//============================================================

HACCEL mameui::winapi::menus::load_accelerators(HINSTANCE instance_handle, const wchar_t *table_name)
{
	return LoadAcceleratorsW(instance_handle, table_name);
}

//============================================================
//  load_accelerators_utf8
//============================================================

HACCEL mameui::winapi::menus::load_accelerators_utf8(HINSTANCE instance_handle, const char *table_name)
{
	return LoadAcceleratorsA(instance_handle, table_name);
}

//============================================================
//  load_cursor
//============================================================

HCURSOR mameui::winapi::menus::load_cursor(HINSTANCE instance_handle, const wchar_t *cursor_name)
{
	return LoadCursorW(instance_handle, cursor_name);
}

//============================================================
//  load_cursor_utf8
//============================================================

HCURSOR mameui::winapi::menus::load_cursor_utf8(HINSTANCE instance_handle, const char *cursor_name)
{
	return LoadCursorA(instance_handle, cursor_name);
}

//============================================================
//  load_icon
//============================================================

HICON mameui::winapi::menus::load_icon(HINSTANCE instance_handle, const wchar_t *icon_name)
{
	return LoadIconW(instance_handle, icon_name);
}

//============================================================
//  load_icon_utf8
//============================================================

HICON mameui::winapi::menus::load_icon_utf8(HINSTANCE instance_handle, const char *icon_name)
{
	return LoadIconA(instance_handle, icon_name);
}

//============================================================
//  load_image
//============================================================

HANDLE mameui::winapi::menus::load_image(HINSTANCE instance_handle, const wchar_t *image_name, UINT image_type, int x_position, int y_position, UINT flags)
{
	return LoadImageW(instance_handle, image_name, image_type, x_position, y_position, flags);
}

//============================================================
//  load_image_utf8
//============================================================

HANDLE mameui::winapi::menus::load_image_utf8(HINSTANCE instance_handle, const char *image_name, UINT image_type, int x_position, int y_position, UINT flags)
{
	return LoadImageA(instance_handle, image_name, image_type, x_position, y_position, flags);
}

//============================================================
//  load_menu
//============================================================

HMENU mameui::winapi::menus::load_menu(HINSTANCE instance_handle, const wchar_t *menu_name)
{
	return LoadMenuW(instance_handle, menu_name);
}

//============================================================
//  load_menu_utf8
//============================================================

HMENU mameui::winapi::menus::load_menu_utf8(HINSTANCE instance_handle, const char *menu_name)
{
	return LoadMenuA(instance_handle, menu_name);
}

//============================================================
//  make_int_resource
//============================================================

wchar_t *mameui::winapi::menus::make_int_resource(int resource_id)
{
	return MAKEINTRESOURCEW(resource_id);
}

//============================================================
//  make_int_resource_utf8
//============================================================

char *mameui::winapi::menus::make_int_resource_utf8(int resource_id)
{
	return MAKEINTRESOURCEA(resource_id);
}

//============================================================
//  remove_menu
//============================================================

BOOL mameui::winapi::menus::remove_menu(HMENU menu_handle, UINT item_ref, UINT flags)
{
	return RemoveMenu(menu_handle, item_ref, flags);
}

//============================================================
//  set_cursor
//============================================================

HCURSOR mameui::winapi::menus::set_cursor(HCURSOR cursor_handle)
{
	return SetCursor(cursor_handle);
}

//============================================================
//  set_cursor_pos
//============================================================

BOOL mameui::winapi::menus::set_cursor_pos(int x_position, int y_position)
{
	return SetCursorPos(x_position, y_position);
}

//============================================================
//  set_menu
//============================================================

BOOL mameui::winapi::menus::set_menu(HWND window_handle, HMENU menu_handle)
{
	return SetMenu(window_handle, menu_handle);
}

//============================================================
//  set_menu_item_info
//============================================================

BOOL mameui::winapi::menus::set_menu_item_info(HMENU menu_handle, UINT item_id, BOOL by_position, LPCMENUITEMINFOW menu_item_info)
{
	return SetMenuItemInfoW(menu_handle, item_id, by_position, menu_item_info);
}

//============================================================
//  set_menu_item_info_utf8
//============================================================

BOOL mameui::winapi::menus::set_menu_item_info_utf8(HMENU menu_handle, UINT item_id, BOOL by_position, LPCMENUITEMINFOA menu_item_info)
{
	return SetMenuItemInfoA(menu_handle, item_id, by_position, menu_item_info);
}

//============================================================
//  show_cursor
//============================================================

int mameui::winapi::menus::show_cursor(BOOL show)
{
	return ShowCursor(show);
}

//============================================================
// track_popup_menu
//============================================================

BOOL mameui::winapi::menus::track_popup_menu(HMENU menu_handle, UINT flags, int x_location, int y_location, int reserved, HWND window_handle, LPCRECT rectangle)
{
	return TrackPopupMenu(menu_handle, flags, x_location, y_location, reserved, window_handle, rectangle);
}

BOOL mameui::winapi::menus::track_popup_menu(HMENU menu_handle, UINT flags, int x_location, int y_location, HWND window_handle, LPCRECT rectangle)
{
	return TrackPopupMenu(menu_handle, flags, x_location, y_location, 0, window_handle, rectangle);
}
