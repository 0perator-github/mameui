// license:BSD-3-Clause
// copyright-holders:0perator

/***************************************************************************

  winapi_menus.cpp

 ***************************************************************************/

// standard windows headers

// standard C++ headers
#include <memory>

// MAME/MAMEUI headers
#include "mui_wcsconv.h"
#include "winapi_menus.h"


//============================================================
//check_menu_item
//============================================================

DWORD mameui::winapi::menus::check_menu_item(HMENU menu_handle,UINT item_id, UINT check)
{
	return CheckMenuItem(menu_handle, item_id, check);
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
// destroy_menu
//============================================================

BOOL mameui::winapi::menus::destroy_menu(HMENU menu_handle)
{
	return DestroyMenu(menu_handle);
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
	std::unique_ptr<wchar_t[]>  wcs_table_name(mui_wcstring_from_utf8(table_name));

	if (!wcs_table_name)
		return 0;

	return load_accelerators(instance_handle, wcs_table_name.get());
}


//============================================================
//  load_cursor
//============================================================

HCURSOR mameui::winapi::menus::load_cursor(HINSTANCE instance_handle, const wchar_t* cursor_name)
{
	return LoadCursorW(instance_handle, cursor_name);
}


//============================================================
//  load_icon
//============================================================

HICON mameui::winapi::menus::load_icon(HINSTANCE instance_handle, const wchar_t* icon_name)
{
	return LoadIconW(instance_handle, icon_name);
}


//============================================================
//  load_icon_utf8
//============================================================

HICON mameui::winapi::menus::load_icon_utf8(HINSTANCE instance_handle, const char* icon_name)
{
	std::unique_ptr<wchar_t[]>  wcs_icon_name(mui_wcstring_from_utf8(icon_name));

	if (!wcs_icon_name)
		return 0;

	return load_icon(instance_handle, wcs_icon_name.get());
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
	std::unique_ptr<wchar_t[]>  wcs_image_name(mui_wcstring_from_utf8(image_name));

	if (!wcs_image_name)
		return 0;

	return load_image(instance_handle, wcs_image_name.get(), image_type, x_position, y_position, flags);
}


//============================================================
//  load_menu
//============================================================

HMENU mameui::winapi::menus::load_menu(HINSTANCE instance_handle, const wchar_t* menu_name)
{
	return LoadMenuW(instance_handle, menu_name);
}


//============================================================
//  load_menu_utf8
//============================================================

HMENU mameui::winapi::menus::load_menu_utf8(HINSTANCE instance_handle, const char* menu_name)
{
	std::unique_ptr<wchar_t[]>  wcs_menu_name(mui_wcstring_from_utf8(menu_name));

	if (!wcs_menu_name)
		return 0;

	return load_menu(instance_handle, wcs_menu_name.get());
}


//============================================================
//  make_int_resource
//============================================================

wchar_t *mameui::winapi::menus::make_int_resource(int resource_id)
{
	return MAKEINTRESOURCEW(resource_id);
}


//============================================================
//  set_cursor_pos
//============================================================

BOOL mameui::winapi::menus::set_cursor_pos(int x_position, int y_position)
{
	return SetCursorPos(x_position, y_position);
}


//============================================================
//  set_menu_item_info
//============================================================

BOOL mameui::winapi::menus::set_menu_item_info(HMENU menu_handle, UINT item_id, BOOL by_position, LPCMENUITEMINFOW menu_item_info)
{
	return SetMenuItemInfoW(menu_handle, item_id, by_position, menu_item_info);
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

BOOL mameui::winapi::menus::track_popup_menu(HMENU menu_handle, UINT flags, int x_location, int y_location, HWND window_handle, LPCRECT rectangle)
{
	return TrackPopupMenu(menu_handle, flags, x_location, y_location, 0, window_handle, rectangle);
}
