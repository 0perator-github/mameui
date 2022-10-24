// license:BSD-3-Clause
// copyright-holders:0perator

#ifndef MAMEUI_LIB_WINAPI_WINDOWS_GDI_H
#define MAMEUI_LIB_WINAPI_WINDOWS_GDI_H

namespace mameui::winapi
{
	namespace gdi
	{
		BOOL bit_blt(HDC destination_hdc, int destination_x, int destination_y, int rectangle_width, int rectangle_height, HDC source_hdc, int source_x, int source_y, DWORD raster_op_code);

		BOOL client_to_screen(HWND window_handle, LPPOINT point);

		int combine_rgn(HRGN destination, HRGN primary_source, HRGN secondary_source, int combine_mode);

		HBITMAP create_bitmap(int width, int height, UINT plane_count, UINT bits_per_pixel, const VOID* color_data);

		HDC create_compatible_dc(HDC device_context_handle);

		HFONT create_font_indirect(const LPLOGFONTW logical_font);

		HPALETTE create_half_tone_palette(HDC device_context_handle);

		HPALETTE create_palette(PLOGPALETTE logical_palette);

		HRGN create_rect_rgn_indirect(LPCRECT rectangle);

		HBRUSH create_solid_brush(COLORREF color);

		BOOL delete_bitmap(HBITMAP bitmap_handle);

		BOOL delete_brush(HBRUSH brush);

		BOOL delete_dc(HDC device_context_handle);

		BOOL delete_font(HFONT font_handle);

		BOOL delete_object(HGDIOBJ gdi_object_handle);

		BOOL delete_palette(HPALETTE palette_handle);

		BOOL draw_focus_rect(HDC device_context_handle, LPRECT rectangle);

		int draw_text(HDC device_context_handle, const wchar_t* text, int text_length, LPRECT rectangle, UINT format);

		BOOL end_paint(HWND window_handle, const LPPAINTSTRUCT paint);

		BOOL  enum_display_devices(const wchar_t *device_name, DWORD device_number, PDISPLAY_DEVICEW display_device, DWORD flags);

		BOOL  enum_display_settings(const wchar_t *device_name, DWORD mode_number, PDEVMODEW device_mode);

		int fill_rect(HDC device_context_handle, LPCRECT rectangle, HBRUSH brush_handle);

		BOOL fill_rgn(HDC device_context_handle, HRGN region_handle, HBRUSH brush_handle);

		HDC get_dc(HWND window_handle);

		BOOL get_dc_org_ex(HDC device_context_handle, LPPOINT point);

		int get_device_caps(HDC device_context_handle, int index);

		int get_object(HANDLE object_handle, int buffer_size, LPVOID buffer);

		HBRUSH get_sys_color_brush(int color_index);

		BOOL get_text_extent_point_32(HDC device_context_handle, const wchar_t* string, int string_length, LPSIZE logical_units);
		BOOL get_text_extent_point_32_utf8(HDC device_context_handle, const char* string, int string_length, LPSIZE logical_units);

		BOOL get_text_metrics(HDC device_context_handle, LPTEXTMETRICW text_metric);

		BOOL invalidate_rect(HWND window_handle, LPRECT rectangle, BOOL erase);

		HBITMAP load_bitmap(HINSTANCE instance_handle, const wchar_t *bitmap_name);
		HBITMAP load_bitmap_utf8(HINSTANCE instance_handle, const char *bitmap_name);

		POINTS make_points(DWORD cursor_pos);

		int map_window_points(HWND from_handle, HWND to_handle, LPPOINT point, UINT count);

		BOOL offset_rect(LPRECT rectangle, int dx, int dy);

		BOOL pat_blt(HDC destination_hdc, int destination_x, int destination_y, int rectangle_width, int rectangle_height, DWORD raster_op_code);

		BOOL pt_in_rect(LPCRECT rectangle, POINT point);

		UINT realize_palette(HDC device_context_handle);

		BOOL redraw_window(HWND window_handle, LPCRECT update_rectangle, HRGN update_region, UINT flags);

		int release_dc(HWND window_handle, HDC device_context_handle);

		BOOL screen_to_client(HWND window_handle, LPPOINT screen_point);

		int select_clip_rgn(HDC device_context_handle, HRGN region_handle);

		HGDIOBJ select_object(HDC device_context_handle, HGDIOBJ gdi_object_handle);

		HPALETTE select_palette(HDC device_context_handle, HPALETTE palette_to_select, BOOL force_to_background);

		COLORREF set_bk_color(HDC device_context_handle, COLORREF color);

		int set_bk_mode(HDC device_context_handle, int background_mode);

		int set_stretch_blt_mode(HDC device_context_handle, int stretching_mode);

		COLORREF set_text_color(HDC device_context_handle, COLORREF color);

		void set_window_redraw(HWND window_handle, BOOL redraw);

		BOOL stretch_blt(HDC destination, int destination_x, int destination_y, int destination_width, int destination_height, HDC source, int source_x, int source_y, int source_width, int source_height, DWORD raster_operation);
	}
}

#endif // MAMEUI_LIB_WINAPI_WINDOWS_GDI_H
