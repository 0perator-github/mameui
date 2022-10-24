// license:BSD-3-Clause
// copyright-holders:0perator

/***************************************************************************

  winapi_dialog_boxes.cpp

 ***************************************************************************/

// standard windows headers

// standard C++ headers
#include <memory>

// MAME/MAMEUI headers
#include "mui_wcsconv.h"
#include "winapi_dialog_boxes.h"


//============================================================
// choose_color
//============================================================

BOOL mameui::winapi::dialog_boxes::choose_color(LPCHOOSECOLORW color)
{
	return ChooseColorW(color);
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
	std::unique_ptr<wchar_t[]> wcs_template_name(mui_wcstring_from_utf8(template_name));

	if (!wcs_template_name)
		return 0;

	return create_dialog(instance_handle, wcs_template_name.get(), parent_window_handle, dialog_procedure, init_dialog);
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
	std::unique_ptr<wchar_t[]> wcs_template_name(mui_wcstring_from_utf8(template_name));

	if (!wcs_template_name)
		return 0;

	return dialog_box(instance_handle, wcs_template_name.get(), parent_window_handle, dialog_procedure, init_dialog);
}


//============================================================
// end_dialog
//============================================================

BOOL mameui::winapi::dialog_boxes::end_dialog(HWND window_handle, INT_PTR result)
{
	return EndDialog(window_handle, result);
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
	int result = 0;
	std::unique_ptr<wchar_t[]> wcs_text, wcs_caption;

	wcs_text = std::unique_ptr<wchar_t[]>(mui_wcstring_from_utf8(text));
	if (!wcs_text)
		return result;

	wcs_caption = std::unique_ptr<wchar_t[]>(mui_wcstring_from_utf8(caption));
	if (!wcs_caption)
		return result;

	result = message_box(window_handle, wcs_text.get(), wcs_caption.get(), type);

	return result;
}


//============================================================
// send_dlg_item_message
//============================================================

LRESULT mameui::winapi::dialog_boxes::send_dlg_item_message(HWND window_handle, int control_id, UINT message, WPARAM word_parameter, LPARAM long_parameter)
{
	return SendDlgItemMessageW(window_handle, control_id, message, word_parameter, long_parameter);
}

