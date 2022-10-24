// license:BSD-3-Clause
// copyright-holders:0perator

#ifndef MAMEUI_LIB_WINAPI_DIALOG_BOXES_H
#define MAMEUI_LIB_WINAPI_DIALOG_BOXES_H

namespace mameui::winapi::dialog_boxes
{
	BOOL choose_color(LPCHOOSECOLORW color);
	BOOL choose_color_utf8(LPCHOOSECOLORA color);

	BOOL choose_font(LPCHOOSEFONTW font);
	BOOL choose_font_utf8(LPCHOOSEFONTA font);

	HWND create_dialog(HINSTANCE instance_handle, const wchar_t* template_name, HWND parent_window_handle, DLGPROC dialog_procedure, LPARAM init_dialog);
	HWND create_dialog_utf8(HINSTANCE instance_handle, const char *template_name, HWND parent_window_handle, DLGPROC dialog_procedure, LPARAM init_dialog);

	HWND create_dialog_indirect(HINSTANCE instance_handle, LPCDLGTEMPLATEW dialog_template, HWND parent_window_handle, DLGPROC dialog_procedure, LPARAM init_dialog);
	HWND create_dialog_indirect_utf8(HINSTANCE instance_handle, LPCDLGTEMPLATEA dialog_template, HWND parent_window_handle, DLGPROC dialog_procedure, LPARAM init_dialog);

	INT_PTR dialog_box(HINSTANCE instance_handle, const wchar_t *template_name, HWND parent_window_handle, DLGPROC dialog_procedure, LPARAM init_dialog);
	INT_PTR dialog_box_utf8(HINSTANCE instance_handle, const char *template_name, HWND parent_window_handle, DLGPROC dialog_procedure, LPARAM init_dialog);

	INT_PTR dialog_box_indirect(HINSTANCE instance_handle, LPCDLGTEMPLATEW dialog_template, HWND parent_window_handle, DLGPROC dialog_procedure, LPARAM init_dialog);
	INT_PTR dialog_box_indirect_utf8(HINSTANCE instance_handle, LPCDLGTEMPLATEA dialog_template, HWND parent_window_handle, DLGPROC dialog_procedure, LPARAM init_dialog);

	BOOL end_dialog(HWND window_handle, INT_PTR result);

	int get_dlg_ctrl_id(HWND window_handle);

	HWND get_dlg_item(HWND window_handle, int dialog_id);

	BOOL get_open_filename(LPOPENFILENAMEW open_filename);
	BOOL get_open_filename_utf8(LPOPENFILENAMEA open_filename);

	BOOL get_save_filename(LPOPENFILENAMEW open_filename);
	BOOL get_save_filename_utf8(LPOPENFILENAMEA open_filename);

	int message_box(HWND window_handle, const wchar_t *text, const wchar_t* caption, UINT type);
	int message_box_utf8(HWND window_handle, const char *text, const char* caption, UINT type);

	LRESULT send_dlg_item_message(HWND window_handle, int control_id, UINT message, WPARAM word_parameter, LPARAM long_parameter);
	LRESULT send_dlg_item_message_utf8(HWND window_handle, int control_id, UINT message, WPARAM word_parameter, LPARAM long_parameter);
}

#endif // MAMEUI_LIB_WINAPI_DIALOG_BOXES_H
