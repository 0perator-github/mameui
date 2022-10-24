// license:BSD-3-Clause
// copyright-holders:0perator

//===========================================================================
//
// dialog_boxes.cpp - Wrapper functions for WIN32 dialog boxes
//
//===========================================================================

// standard C++ headers

// standard windows headers
#include "winapi_common.h"
#include <dlgs.h>

// MAME headers

// MAMEUI headers

#include "dialog_boxes.h"

//============================================================
// choose_color
//============================================================

BOOL mameui::winapi::dialog_boxes::choose_color(LPCHOOSECOLORW color)
{
	return ChooseColorW(color);
}

//============================================================
// choose_color_utf8
//============================================================

BOOL mameui::winapi::dialog_boxes::choose_color_utf8(LPCHOOSECOLORA color)
{
	return ChooseColorA(color);
}

//============================================================
// choose_font
//============================================================

BOOL mameui::winapi::dialog_boxes::choose_font(LPCHOOSEFONTW font)
{
	return ChooseFontW(font);
}

//============================================================
// choose_font_utf8
//============================================================

BOOL mameui::winapi::dialog_boxes::choose_font_utf8(LPCHOOSEFONTA font)
{
	return ChooseFontA(font);
}

//============================================================
// create_dialog
//============================================================

HWND mameui::winapi::dialog_boxes::create_dialog(HINSTANCE instance_handle, const wchar_t *template_name, HWND parent_window_handle, DLGPROC dialog_procedure, LPARAM init_dialog)
{
	return CreateDialogParamW(instance_handle, template_name, parent_window_handle, dialog_procedure, init_dialog);
}

//============================================================
// create_dialog_utf8
//============================================================

HWND mameui::winapi::dialog_boxes::create_dialog_utf8(HINSTANCE instance_handle, const char *template_name, HWND parent_window_handle, DLGPROC dialog_procedure, LPARAM init_dialog)
{
	return CreateDialogParamA(instance_handle, template_name, parent_window_handle, dialog_procedure, init_dialog);
}

//============================================================
// create_dialog_indirect
//============================================================

HWND mameui::winapi::dialog_boxes::create_dialog_indirect(HINSTANCE instance_handle, LPCDLGTEMPLATEW dialog_template, HWND parent_window_handle, DLGPROC dialog_procedure, LPARAM init_dialog)
{
	return CreateDialogIndirectParamW(instance_handle, dialog_template, parent_window_handle, dialog_procedure, init_dialog);
}

//============================================================
// create_dialog_indirect_utf8
//============================================================

HWND mameui::winapi::dialog_boxes::create_dialog_indirect_utf8(HINSTANCE instance_handle, LPCDLGTEMPLATEA dialog_template, HWND parent_window_handle, DLGPROC dialog_procedure, LPARAM init_dialog)
{
	return CreateDialogIndirectParamA(instance_handle, dialog_template, parent_window_handle, dialog_procedure, init_dialog);
}

//============================================================
// dialog_box
//============================================================

INT_PTR mameui::winapi::dialog_boxes::dialog_box(HINSTANCE instance_handle, const wchar_t *template_name, HWND parent_window_handle, DLGPROC dialog_procedure, LPARAM init_dialog)
{
	return DialogBoxParamW(instance_handle, template_name, parent_window_handle, dialog_procedure, init_dialog);
}

//============================================================
// dialog_box_utf8
//============================================================

INT_PTR mameui::winapi::dialog_boxes::dialog_box_utf8(HINSTANCE instance_handle, const char *template_name, HWND parent_window_handle, DLGPROC dialog_procedure, LPARAM init_dialog)
{
	return DialogBoxParamA(instance_handle, template_name, parent_window_handle, dialog_procedure, init_dialog);
}

//============================================================
// dialog_box_indirect
//============================================================

INT_PTR mameui::winapi::dialog_boxes::dialog_box_indirect(HINSTANCE instance_handle, LPCDLGTEMPLATEW dialog_template, HWND parent_window_handle, DLGPROC dialog_procedure, LPARAM init_dialog)
{
	return DialogBoxIndirectParamW(instance_handle, dialog_template, parent_window_handle, dialog_procedure, init_dialog);
}

//============================================================
// dialog_box_indirect_utf8
//============================================================

INT_PTR mameui::winapi::dialog_boxes::dialog_box_indirect_utf8(HINSTANCE instance_handle, LPCDLGTEMPLATEA dialog_template, HWND parent_window_handle, DLGPROC dialog_procedure, LPARAM init_dialog)
{
	return DialogBoxIndirectParamA(instance_handle, dialog_template, parent_window_handle, dialog_procedure, init_dialog);
}

//============================================================
// end_dialog
//============================================================

BOOL mameui::winapi::dialog_boxes::end_dialog(HWND window_handle, INT_PTR result)
{
	return EndDialog(window_handle, result);
}

//============================================================
// get_dlg_ctrl_id
//============================================================

int mameui::winapi::dialog_boxes::get_dlg_ctrl_id(HWND window_handle)
{
	return GetDlgCtrlID(window_handle);
}

//============================================================
// get_dlg_item
//============================================================

HWND mameui::winapi::dialog_boxes::get_dlg_item(HWND window_handle, int dialog_id)
{
	return GetDlgItem(window_handle, dialog_id);
}

//============================================================
// get_open_filename
//============================================================

BOOL mameui::winapi::dialog_boxes::get_open_filename(LPOPENFILENAMEW open_filename)
{
	return GetOpenFileNameW(open_filename);
}

//============================================================
// get_open_filename_utf8
//============================================================

BOOL mameui::winapi::dialog_boxes::get_open_filename_utf8(LPOPENFILENAMEA open_filename)
{
	return GetOpenFileNameA(open_filename);
}

//============================================================
// get_save_filename
//============================================================

BOOL mameui::winapi::dialog_boxes::get_save_filename(LPOPENFILENAMEW open_filename)
{
	return GetSaveFileNameW(open_filename);
}

//============================================================
// get_save_filename_utf8
//============================================================

BOOL mameui::winapi::dialog_boxes::get_save_filename_utf8(LPOPENFILENAMEA open_filename)
{
	return GetSaveFileNameA(open_filename);
}

//============================================================
// message_box
//============================================================

int mameui::winapi::dialog_boxes::message_box(HWND window_handle, const wchar_t *text, const wchar_t *caption, UINT type)
{
	return MessageBoxW(window_handle, text, caption, type);
}

//============================================================
// message_box_utf8
//============================================================

int mameui::winapi::dialog_boxes::message_box_utf8(HWND window_handle, const char *text, const char *caption, UINT type)
{
	return MessageBoxA(window_handle, text, caption, type);
}

//============================================================
// send_dlg_item_message
//============================================================

LRESULT mameui::winapi::dialog_boxes::send_dlg_item_message(HWND window_handle, int control_id, UINT message, WPARAM word_parameter, LPARAM long_parameter)
{
	return SendDlgItemMessageW(window_handle, control_id, message, word_parameter, long_parameter);
}

//============================================================
// send_dlg_item_message_utf8
//============================================================

LRESULT mameui::winapi::dialog_boxes::send_dlg_item_message_utf8(HWND window_handle, int control_id, UINT message, WPARAM word_parameter, LPARAM long_parameter)
{
	return SendDlgItemMessageA(window_handle, control_id, message, word_parameter, long_parameter);
}
