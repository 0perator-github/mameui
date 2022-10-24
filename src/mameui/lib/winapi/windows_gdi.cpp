// license:BSD-3-Clause
// copyright-holders:0perator

//===========================================================================
//
// windows_gdi.cpp - Wrapper functions for Windows GDI
//
//===========================================================================

// standard windows headers
#include "winapi_common.h"

// MAMEUI headers
#include "windows_messages.h"

#include "windows_gdi.h"

using namespace mameui::winapi::windows;

//============================================================
// bit_blt
//============================================================

BOOL mameui::winapi::gdi::bit_blt(HDC destination_hdc, int destination_x, int destination_y, int rectangle_width, int rectangle_height, HDC source_hdc, int source_x, int source_y, DWORD raster_op_code)
{
	return BitBlt(destination_hdc, destination_x, destination_y, rectangle_width, rectangle_height, source_hdc, source_x, source_y, raster_op_code);
}

//============================================================
// client_to_screen
//============================================================

BOOL mameui::winapi::gdi::client_to_screen(HWND window_handle, LPPOINT point)
{
	return ClientToScreen(window_handle, point);
}

//============================================================
// combine_rgn
//============================================================

int mameui::winapi::gdi::combine_rgn(HRGN destination, HRGN primary_source, HRGN secondary_source, int combine_mode)
{
	return CombineRgn(destination, primary_source, secondary_source, combine_mode);
}

//============================================================
// create_bitmap
//============================================================

HBITMAP mameui::winapi::gdi::create_bitmap(int width,int height,UINT plane_count,UINT bits_per_pixel,const VOID *color_data)
{
	return CreateBitmap(width, height, plane_count, bits_per_pixel, color_data);
}

//============================================================
// create_compatible_dc
//============================================================

HDC mameui::winapi::gdi::create_compatible_dc(HDC device_context_handle)
{
	return CreateCompatibleDC(device_context_handle);
}

//============================================================
// create_font_indirect
//============================================================

HFONT mameui::winapi::gdi::create_font_indirect(const LPLOGFONTW logical_font)
{
	return CreateFontIndirectW(logical_font);
}

//============================================================
// create_half_tone_palette
//============================================================

HPALETTE mameui::winapi::gdi::create_half_tone_palette(HDC device_context_handle)
{
	return CreateHalftonePalette(device_context_handle);
}

//============================================================
// create_palette
//============================================================

HPALETTE mameui::winapi::gdi::create_palette(PLOGPALETTE logical_palette)
{
	return CreatePalette(logical_palette);
}

//============================================================
// create_rect_rgn_indirect
//============================================================

HRGN mameui::winapi::gdi::create_rect_rgn_indirect(LPCRECT rectangle)
{
	return CreateRectRgnIndirect(rectangle);
}

//============================================================
// create_solid_brush
//============================================================

HBRUSH mameui::winapi::gdi::create_solid_brush(COLORREF color)
{
	return CreateSolidBrush(color);
}

//============================================================
// delete_bitmap
//============================================================

BOOL mameui::winapi::gdi::delete_bitmap(HBITMAP bitmap_handle)
{
	return delete_object((HGDIOBJ)bitmap_handle);
}

//============================================================
// delete_brush
//============================================================

BOOL mameui::winapi::gdi::delete_brush(HBRUSH brush_handle)
{
	return delete_object((HGDIOBJ)brush_handle);
}

//============================================================
// delete_dc
//============================================================

BOOL mameui::winapi::gdi::delete_dc(HDC device_context_handle)
{
	return DeleteDC(device_context_handle);
}

//============================================================
// delete_font
//============================================================

BOOL mameui::winapi::gdi::delete_font(HFONT font_handle)
{
	return DeleteObject((HGDIOBJ)font_handle);
}

//============================================================
// delete_object
//============================================================

BOOL mameui::winapi::gdi::delete_object(HGDIOBJ gdi_object_handle)
{
	return DeleteObject(gdi_object_handle);
}

//============================================================
// delete_palette
//============================================================

BOOL mameui::winapi::gdi::delete_palette(HPALETTE palette_handle)
{
	return delete_object((HGDIOBJ)palette_handle);
}

//============================================================
// draw_focus_rect
//============================================================

BOOL mameui::winapi::gdi::draw_focus_rect(HDC device_context_handle, LPRECT rectangle)
{
	return DrawFocusRect(device_context_handle, rectangle);
}

//============================================================
// draw_text
//============================================================

int mameui::winapi::gdi::draw_text(HDC device_context_handle, const wchar_t *text, int text_length, LPRECT rectangle, UINT format)
{
	return DrawTextW(device_context_handle, text, text_length, rectangle, format);
}

//============================================================
// end_paint
//============================================================

BOOL  mameui::winapi::gdi::end_paint(HWND window_handle, const LPPAINTSTRUCT paint)
{
	return EndPaint(window_handle, paint);
}

//============================================================
// enum_display_devices
//============================================================

BOOL  mameui::winapi::gdi::enum_display_devices(const wchar_t *device_name, DWORD device_number, PDISPLAY_DEVICEW display_device, DWORD flags)
{
	return EnumDisplayDevicesW(device_name, device_number, display_device, flags);
}

//============================================================
// enum_display_settings
//============================================================

BOOL  mameui::winapi::gdi::enum_display_settings(const wchar_t* device_name, DWORD mode_number, PDEVMODEW device_mode)
{
	return EnumDisplaySettingsW(device_name, mode_number, device_mode);
}

//============================================================
// fill_rgn
//============================================================

BOOL mameui::winapi::gdi::fill_rgn(HDC device_context_handle, HRGN region_handle, HBRUSH brush_handle)
{
	return FillRgn(device_context_handle, region_handle, brush_handle);
}

//============================================================
// fill_rect
//============================================================

int mameui::winapi::gdi::fill_rect(HDC device_context_handle, LPCRECT rectangle, HBRUSH brush_handle)
{
	return FillRect(device_context_handle, rectangle, brush_handle);
}

//============================================================
// get_dc
//============================================================

HDC mameui::winapi::gdi::get_dc(HWND window_handle)
{
	return GetDC(window_handle);
}

//============================================================
// get_dc_org_ex
//============================================================

BOOL mameui::winapi::gdi::get_dc_org_ex(HDC device_context_handle, LPPOINT point)
{
	return GetDCOrgEx(device_context_handle, point);
}

//============================================================
// get_device_caps
//============================================================

int mameui::winapi::gdi::get_device_caps(HDC device_context_handle, int item_index)
{
	return GetDeviceCaps(device_context_handle, item_index);
}

//============================================================
// get_object
//============================================================

int mameui::winapi::gdi::get_object(HANDLE object_handle, int buffer_size, LPVOID buffer)
{
	return GetObjectW(object_handle, buffer_size, buffer);
}

//============================================================
//  get_sys_color_brush
//============================================================

HBRUSH mameui::winapi::gdi::get_sys_color_brush(int color_index)
{
	return GetSysColorBrush(color_index);
}

//============================================================
// get_text_extent_point_32
//============================================================

BOOL mameui::winapi::gdi::get_text_extent_point_32(HDC device_context_handle, const wchar_t *string,int string_length,LPSIZE logical_units)
{
	return GetTextExtentPoint32W(device_context_handle, string, string_length, logical_units);
}

//============================================================
// get_text_extent_point_32_utf8
//============================================================

BOOL mameui::winapi::gdi::get_text_extent_point_32_utf8(HDC device_context_handle, const char *string, int string_length, LPSIZE logical_units)
{
	return GetTextExtentPoint32A(device_context_handle, string, string_length, logical_units);
}

//============================================================
// get_text_metrics
//============================================================

BOOL mameui::winapi::gdi::get_text_metrics(HDC device_context_handle, LPTEXTMETRICW text_metric)
{
	return GetTextMetricsW(device_context_handle, text_metric);
}

//============================================================
//  invalidate_rect
//============================================================

BOOL mameui::winapi::gdi::invalidate_rect(HWND window_handle, LPRECT rectangle,BOOL erase)
{
	return InvalidateRect(window_handle, rectangle,erase);
}

//============================================================
//  load_bitmap
//============================================================

HBITMAP mameui::winapi::gdi::load_bitmap(HINSTANCE instance_handle, const wchar_t *bitmap_name)
{
	return LoadBitmapW(instance_handle, bitmap_name);
}

//============================================================
//  load_bitmap_utf8
//============================================================

HBITMAP mameui::winapi::gdi::load_bitmap_utf8(HINSTANCE instance_handle, const char *bitmap_name)
{
	return LoadBitmapA(instance_handle, bitmap_name);
}

//============================================================
//  make_points
//============================================================

POINTS mameui::winapi::gdi::make_points(DWORD cursor_pos)
{
	return MAKEPOINTS(cursor_pos);
}

//============================================================
//  map_window_points
//============================================================

int mameui::winapi::gdi::map_window_points(HWND from_handle, HWND to_handle, LPPOINT point, UINT count)
{
	return MapWindowPoints(from_handle, to_handle, point, count);
}

//============================================================
//  offset_rect
//============================================================

BOOL mameui::winapi::gdi::offset_rect(LPRECT rectangle, int dx, int dy)
{
	return OffsetRect(rectangle, dx, dy);
}

//============================================================
// pat_blt
//============================================================

BOOL mameui::winapi::gdi::pat_blt(HDC destination_hdc, int destination_x, int destination_y, int rectangle_width, int rectangle_height, DWORD raster_op_code)
{
	return PatBlt(destination_hdc, destination_x, destination_y, rectangle_width, rectangle_height, raster_op_code);
}

//============================================================
// pt_in_rect
//============================================================

BOOL mameui::winapi::gdi::pt_in_rect(LPCRECT rectangle, POINT point)
{
	return PtInRect(rectangle, point);
}

//============================================================
//  realize_palette
//============================================================

UINT mameui::winapi::gdi::realize_palette(HDC device_context_handle)
{
	return RealizePalette(device_context_handle);
}

//============================================================
//  redraw_window
//============================================================

BOOL mameui::winapi::gdi::redraw_window(HWND window_handle, LPCRECT update_rectangle,HRGN update_region,UINT flags)
{
	return RedrawWindow(window_handle, update_rectangle, update_region, flags);
}

//============================================================
//  release_dc
//============================================================

int mameui::winapi::gdi::release_dc(HWND window_handle, HDC device_context_handle)
{
	return ReleaseDC(window_handle, device_context_handle);
}

//============================================================
//  screen_to_client
//============================================================

BOOL mameui::winapi::gdi::screen_to_client(HWND window_handle, LPPOINT screen_point)
{
	return ScreenToClient(window_handle, screen_point);
}

//============================================================
//  select_clip_rgn
//============================================================

int mameui::winapi::gdi::select_clip_rgn(HDC device_context_handle,HRGN region_handle)
{
	return SelectClipRgn(device_context_handle, region_handle);
}

//============================================================
// select_object
//============================================================

HGDIOBJ mameui::winapi::gdi::select_object(HDC device_context_handle, HGDIOBJ gdi_object_handle)
{
	return SelectObject(device_context_handle, gdi_object_handle);
}

//============================================================
//  select_palette
//============================================================

HPALETTE mameui::winapi::gdi::select_palette(HDC device_context_handle, HPALETTE palette_to_select, BOOL force_to_background)
{
	return SelectPalette(device_context_handle, palette_to_select, force_to_background);
}

//============================================================
// set_bk_color
//============================================================

COLORREF mameui::winapi::gdi::set_bk_color(HDC device_context_handle, COLORREF color)
{
	return SetBkColor(device_context_handle, color);
}

//============================================================
// set_bk_mode
//============================================================

int mameui::winapi::gdi::set_bk_mode(HDC device_context_handle, int background_mode)
{
	return SetBkMode(device_context_handle, background_mode);
}

//============================================================
// set_stretch_blt_mode
//============================================================

int mameui::winapi::gdi::set_stretch_blt_mode(HDC device_context_handle, int stretching_mode)
{
	return SetStretchBltMode(device_context_handle, stretching_mode);
}

//============================================================
// set_text_color
//============================================================

COLORREF mameui::winapi::gdi::set_text_color(HDC device_context_handle, COLORREF color)
{
	return SetTextColor(device_context_handle, color);
}

//============================================================
// set_window_redraw
//============================================================

void mameui::winapi::gdi::set_window_redraw(HWND window_handle, BOOL redraw)
{
	(void)send_message(window_handle, WM_SETREDRAW, (WPARAM)redraw, 0);
}

//============================================================
// stretch_blt
//============================================================

BOOL mameui::winapi::gdi::stretch_blt(HDC destination, int destination_x, int destination_y, int destination_width,
	int destination_height, HDC source, int source_x, int source_y, int source_width, int source_height, DWORD raster_operation)
{
	return StretchBlt(destination, destination_x, destination_y, destination_width, destination_height, source, source_x, source_y, source_width, source_height, raster_operation);
}
// Windows GDI wrapper functions
