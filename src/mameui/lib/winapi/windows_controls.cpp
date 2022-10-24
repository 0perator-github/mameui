// license:BSD-3-Clause
// copyright-holders:0perator

//===========================================================================
//
// windows_controls.cpp - Wrapper functions for handling WIN32 controls
//
//===========================================================================

// standard C++ headers
#include <memory>
#include <string>

// standard windows headers
#include "winapi_common.h"
#include <prsht.h>
#include <uxtheme.h>

// MAMEUI headers
#include "windows_messages.h"

#include "mui_wcstrconv.h"

#include "windows_controls.h"

using namespace mameui::util::string_util;
using namespace mameui::winapi::windows;

//=============================================================================
// button control
//=============================================================================


//============================================================
// enable
//============================================================

BOOL mameui::winapi::controls::button_control::enable(HWND window_handle, BOOL enabled)
{
	return enable_window(window_handle, enabled);
}

//============================================================
// get_check
//============================================================

int mameui::winapi::controls::button_control::get_check(HWND window_handle)
{
	return static_cast<int>(send_message(window_handle, BM_GETCHECK, 0U, 0L));
}

//============================================================
// set_check
//============================================================

void mameui::winapi::controls::button_control::set_check(HWND window_handle, int check_state)
{
	send_message(window_handle, BM_SETCHECK, static_cast<WPARAM>(check_state), 0L);
}


//=============================================================================
// combo-box control
//=============================================================================


//============================================================
// add_string
//============================================================

int mameui::winapi::controls::combo_box::add_string(HWND window_handle, LPARAM add_string_data)
{
	return static_cast<int>(send_message(window_handle, CB_ADDSTRING, 0U, add_string_data));
}

//============================================================
// set_cue_banner_text
//============================================================

BOOL mameui::winapi::controls::combo_box::set_cue_banner_text(HWND window_handle, const wchar_t *banner_text)
{
	return static_cast<BOOL>(send_message(window_handle, CB_SETCUEBANNER, 0U, reinterpret_cast<LPARAM>(banner_text)));
}

//============================================================
// find_string
//============================================================

int mameui::winapi::controls::combo_box::find_string(HWND window_handle, int start_index, LPARAM find_string_data)
{
	return static_cast<int>(send_message(window_handle, CB_FINDSTRING, static_cast<WPARAM>(start_index), find_string_data));
}

//============================================================
// get_count
//============================================================

int mameui::winapi::controls::combo_box::get_count(HWND window_handle)
{
	return static_cast<int>(send_message(window_handle, CB_GETCOUNT, 0U, 0L));
}

//============================================================
// get_cur_sel
//============================================================

int mameui::winapi::controls::combo_box::get_cur_sel(HWND window_handle)
{
	return static_cast<int>(send_message(window_handle, CB_GETCURSEL, 0U, 0L));
}

//============================================================
// get_item_data
//============================================================

LRESULT mameui::winapi::controls::combo_box::get_item_data(HWND window_handle, int item_index)
{
	return send_message(window_handle, CB_GETITEMDATA, static_cast<WPARAM>(item_index), 0L);
}

//============================================================
// insert_string
//============================================================

int mameui::winapi::controls::combo_box::insert_string(HWND window_handle, int item_index, LPARAM insert_string_data)
{
	return static_cast<int>(send_message(window_handle, CB_INSERTSTRING, static_cast<WPARAM>(item_index), insert_string_data));
}

//============================================================
// reset_content
//============================================================

int mameui::winapi::controls::combo_box::reset_content(HWND window_handle)
{
	return static_cast<int>(send_message(window_handle, CB_RESETCONTENT, 0, 0L));
}

//============================================================
// set_cur_sel
//============================================================

int mameui::winapi::controls::combo_box::set_cur_sel(HWND window_handle, int item_index)
{
	return static_cast<int>(send_message(window_handle, CB_SETCURSEL, static_cast<WPARAM>(item_index), 0L));
}

//============================================================
// set_item_data
//============================================================

int mameui::winapi::controls::combo_box::set_item_data(HWND window_handle, int item_index, LPARAM item_data)
{
	return static_cast<int>(send_message(window_handle, CB_SETITEMDATA, static_cast<WPARAM>(item_index), item_data));
}


//=============================================================================
// edit control
//=============================================================================


//=============================================================================
// get_text
//=============================================================================
int mameui::winapi::controls::edit_control::get_text(HWND window_handle, wchar_t *text, int max_count)
{
	return get_window_text(window_handle, text, max_count);
}

wchar_t *mameui::winapi::controls::edit_control::get_text(HWND window_handle)
{
	return get_window_text(window_handle);
}

//=============================================================================
// get_text_length
//=============================================================================

int mameui::winapi::controls::edit_control::get_text_length(HWND window_handle)
{
	return get_window_text_length(window_handle);
}

//=============================================================================
// replace_sel
//=============================================================================

void mameui::winapi::controls::edit_control::replace_sel(HWND window_handle, const wchar_t *replacement)
{
	send_message(window_handle, EM_REPLACESEL, 0U, reinterpret_cast<LPARAM>(replacement));
}

//=============================================================================
// set_sel
//=============================================================================

void mameui::winapi::controls::edit_control::set_sel(HWND window_handle, int begin_pos, int end_pos)
{
	send_message(window_handle, EM_SETSEL, static_cast<WPARAM>(begin_pos), static_cast<LPARAM>(end_pos));
}


//=============================================================================
// header control
//=============================================================================


//=============================================================
// get_item_count
//=============================================================

int mameui::winapi::controls::header_control::get_item_count(HWND window_handle)
{
	return static_cast<int>(send_message(window_handle, HDM_GETITEMCOUNT, 0U, 0L));
}

//=============================================================
// get_item_rect
//=============================================================

BOOL mameui::winapi::controls::header_control::get_item_rect(HWND window_handle, int item_index, LPRECT rectangle)
{
	return static_cast<BOOL>(send_message(window_handle, HDM_GETITEMRECT, static_cast<WPARAM>(item_index), reinterpret_cast<LPARAM>(rectangle)));
}

//=============================================================
// set_item
//=============================================================

BOOL mameui::winapi::controls::header_control::set_item(HWND window_handle, int item_index, LPHDITEMW header_item)
{
	return static_cast<BOOL>(send_message(window_handle, HDM_SETITEMW, static_cast<WPARAM>(item_index), reinterpret_cast<LPARAM>(header_item)));
}


//=============================================================================
// image_list control
//=============================================================================


//=============================================================
// add_icon
//=============================================================

int mameui::winapi::controls::image_list::add_icon(HIMAGELIST image_list_handle, HICON icon)
{
	return replace_icon(image_list_handle, -1, icon);
}

//=============================================================
// create
//=============================================================

HIMAGELIST mameui::winapi::controls::image_list::create(int image_width, int image_height, UINT flags, int initial_count, int grow_count)
{
	return ImageList_Create(image_width, image_height, flags, initial_count, grow_count);
}

//=============================================================
// destroy
//=============================================================


BOOL mameui::winapi::controls::image_list::destroy(HIMAGELIST image_list_handle)
{
	return ImageList_Destroy(image_list_handle);
}

//=============================================================
// draw
//=============================================================

BOOL mameui::winapi::controls::image_list::draw(HIMAGELIST image_list_handle, int image_index, HDC device_context_handle, int x_coordinate, int y_coordinate, UINT style)
{
	return ImageList_Draw(image_list_handle, image_index, device_context_handle, x_coordinate, y_coordinate, style);
}

//=============================================================
// draw_ex
//=============================================================

BOOL mameui::winapi::controls::image_list::draw_ex(HIMAGELIST image_list_handle, int image_index, HDC device_context_handle, int x_coordinate, int y_coordinate,
	int x_destination,int y_destination, COLORREF background_color,COLORREF foreground_color,UINT style)
{
	return ImageList_DrawEx(image_list_handle, image_index, device_context_handle, x_coordinate, y_coordinate, x_destination, y_destination, background_color,foreground_color,style);
}

//=============================================================
// get_icon
//=============================================================

HICON mameui::winapi::controls::image_list::get_icon(HIMAGELIST image_list_handle, int image_index, UINT flags)
{
	return ImageList_GetIcon(image_list_handle, image_index, flags);
}

//=============================================================
// remove
//=============================================================

BOOL mameui::winapi::controls::image_list::remove(HIMAGELIST image_list_handle, int image_index)
{
	return ImageList_Remove(image_list_handle, image_index);
}

//=============================================================
// remove_all
//=============================================================

BOOL mameui::winapi::controls::image_list::remove_all(HIMAGELIST image_list_handle)
{
	return remove(image_list_handle, -1);
}

//=============================================================
// replace_icon
//=============================================================

int mameui::winapi::controls::image_list::replace_icon(HIMAGELIST image_list_handle, int image_index, HICON icon)
{
	return ImageList_ReplaceIcon(image_list_handle, image_index, icon);
}


//=============================================================================
// list-view control
//=============================================================================

//=============================================================
// create_drag_image
//=============================================================
HIMAGELIST mameui::winapi::controls::list_view::create_drag_image(HWND window_handle, int item_index, LPPOINT upper_left)
{
	return reinterpret_cast<HIMAGELIST>(send_message(window_handle, LVM_CREATEDRAGIMAGE, static_cast<WPARAM>(item_index), reinterpret_cast<LPARAM>(upper_left)));
}

//=============================================================
// delete_column
//=============================================================

BOOL mameui::winapi::controls::list_view::delete_column(HWND window_handle, int column_index)
{
	return static_cast<BOOL>(send_message(window_handle, LVM_DELETECOLUMN, static_cast<WPARAM>(column_index), 0L));
}

//=============================================================
// delete_all_items
//=============================================================

BOOL mameui::winapi::controls::list_view::delete_all_items(HWND window_handle)
{
	return static_cast<BOOL>(send_message(window_handle, LVM_DELETEALLITEMS, 0U, 0L));
}

//=============================================================
// delete_item
//=============================================================

BOOL mameui::winapi::controls::list_view::delete_item(HWND window_handle, int item_index)
{
	return static_cast<BOOL>(send_message(window_handle, LVM_DELETEITEM, static_cast<WPARAM>(item_index), 0L));
}

//=============================================================
// ensure_visible
//=============================================================

BOOL mameui::winapi::controls::list_view::ensure_visible(HWND window_handle, int item_index, BOOL partial_visibility_ok)
{
	return static_cast<BOOL>(send_message(window_handle, LVM_ENSUREVISIBLE, static_cast<WPARAM>(item_index), static_cast<LPARAM>(partial_visibility_ok)));
}

//=============================================================
// find_item
//=============================================================

int mameui::winapi::controls::list_view::find_item(HWND window_handle, int starting_index, const LPFINDINFOW find_info)
{
	return static_cast<int>(send_message(window_handle, LVM_FINDITEM, static_cast<WPARAM>(starting_index), reinterpret_cast<LPARAM>(find_info)));
}

//=============================================================
// get_bk_image
//=============================================================

BOOL mameui::winapi::controls::list_view::get_bk_image(HWND window_handle, LPLVBKIMAGEW bk_image_struct)
{
	return static_cast<BOOL>(send_message(window_handle, LVM_GETBKIMAGEW, 0U, reinterpret_cast<LPARAM>(bk_image_struct)));
}

//=============================================================
// get_column
//=============================================================

BOOL mameui::winapi::controls::list_view::get_column(HWND window_handle, int column_index, LPLVCOLUMNW column)
{
	return static_cast<BOOL>(send_message(window_handle, LVM_GETCOLUMNW, static_cast<WPARAM>(column_index), reinterpret_cast<LPARAM>(column)));
}

//=============================================================
// get_column_order_array
//=============================================================

BOOL mameui::winapi::controls::list_view::get_column_order_array(HWND window_handle, int column_count, LPINT column_order_array)
{
	return static_cast<BOOL>(send_message(window_handle, LVM_GETCOLUMNORDERARRAY, static_cast<WPARAM>(column_count), reinterpret_cast<LPARAM>(column_order_array)));
}

//=============================================================
// get_column_width
//=============================================================

int mameui::winapi::controls::list_view::get_column_width(HWND window_handle, int column_index)
{
	return static_cast<int>(send_message(window_handle, LVM_GETCOLUMNWIDTH, static_cast<WPARAM>(column_index), 0L));
}

//=============================================================
// get_count_per_page
//=============================================================

int mameui::winapi::controls::list_view::get_count_per_page(HWND window_handle)
{
	return static_cast<int>(send_message(window_handle, LVM_GETCOUNTPERPAGE, 0U, 0L));
}

//=============================================================
// get_header
//=============================================================

HWND mameui::winapi::controls::list_view::get_header(HWND window_handle)
{
	return reinterpret_cast<HWND>(send_message(window_handle, LVM_GETHEADER, 0U, 0L));
}

//=============================================================
// get_image_list
//=============================================================

HIMAGELIST mameui::winapi::controls::list_view::get_image_list(HWND window_handle, int image_list)
{
	return reinterpret_cast<HIMAGELIST>(send_message(window_handle, LVM_GETIMAGELIST, static_cast<WPARAM>(image_list), 0L));
}

//=============================================================
// get_item
//=============================================================

BOOL mameui::winapi::controls::list_view::get_item(HWND window_handle, LPLVITEMW item)
{
	return static_cast<BOOL>(send_message(window_handle, LVM_GETITEM, 0U, reinterpret_cast<LPARAM>(item)));
}

//=============================================================
// get_item_count
//=============================================================

int mameui::winapi::controls::list_view::get_item_count(HWND window_handle)
{
	return static_cast<int>(send_message(window_handle, LVM_GETITEMCOUNT, 0U, 0L));
}

//=============================================================
// get_item_rect
//=============================================================

BOOL mameui::winapi::controls::list_view::get_item_rect(HWND window_handle, int item_index, LPRECT rectangle)
{
	return static_cast<BOOL>(send_message(window_handle, LVM_GETITEMRECT, static_cast<WPARAM>(item_index), reinterpret_cast<LPARAM>(rectangle)));
}

//=============================================================
// get_item_state
//=============================================================

UINT mameui::winapi::controls::list_view::get_item_state(HWND window_handle, int item_index, UINT mask)
{
	return static_cast<UINT>(send_message(window_handle, LVM_GETITEMSTATE, static_cast<WPARAM>(item_index), static_cast<LPARAM>(mask)));
}

//=============================================================
// get_item_text
//=============================================================

int mameui::winapi::controls::list_view::get_item_text(HWND window_handle, int item_index, LPLVITEMW item)
{
	return static_cast<int>(send_message(window_handle, LVM_GETITEMTEXTW, static_cast<WPARAM>(item_index), reinterpret_cast<LPARAM>(item)));
}

int mameui::winapi::controls::list_view::get_item_text(HWND window_handle, int item_index, int sub_item_index, wchar_t *text, int text_length)
{
	LVITEMW item{};
	item.iSubItem = sub_item_index;
	item.pszText = text;
	item.cchTextMax = text_length;

	return get_item_text(window_handle, item_index, &item);
}

//=============================================================
// get_item_text_utf8
//=============================================================

int mameui::winapi::controls::list_view::get_item_text_utf8(HWND window_handle, int item_index, LPLVITEMA item)
{
	return static_cast<int>(send_message(window_handle, LVM_GETITEMTEXTA, static_cast<WPARAM>(item_index), reinterpret_cast<LPARAM>(item)));
}

int mameui::winapi::controls::list_view::get_item_text_utf8(HWND window_handle, int item_index, int sub_item_index, char *text, int text_length)
{
	LVITEMA item{};
	item.iSubItem = sub_item_index;
	item.pszText = text;
	item.cchTextMax = text_length;

	return get_item_text_utf8(window_handle, item_index, &item);
}

//=============================================================
// get_next_item
//=============================================================

int mameui::winapi::controls::list_view::get_next_item(HWND window_handle, int item_index, UINT flags)
{
	return static_cast<int>(send_message(window_handle, LVM_GETNEXTITEM, static_cast<WPARAM>(item_index), MAKELPARAM(flags, 0)));
}

//=============================================================
// get_top_index
//=============================================================

int mameui::winapi::controls::list_view::get_top_index(HWND window_handle)
{
	return static_cast<int>(send_message(window_handle, LVM_GETTOPINDEX, 0U, 0L));
}

//=============================================================
// hit_test
//=============================================================

int mameui::winapi::controls::list_view::hit_test(HWND window_handle, LPLVHITTESTINFO hit_test_info)
{
	return static_cast<int>(send_message(window_handle, LVM_HITTEST, 0U, reinterpret_cast<LPARAM>(hit_test_info)));
}

//=============================================================
// insert_column
//=============================================================

int mameui::winapi::controls::list_view::insert_column(HWND window_handle, int column_index, LPLVCOLUMNW column)
{
	return static_cast<int>(send_message(window_handle, LVM_INSERTCOLUMN, static_cast<WPARAM>(column_index), reinterpret_cast<LPARAM>(column)));
}

//=============================================================
// insert_item
//=============================================================

int mameui::winapi::controls::list_view::insert_item(HWND window_handle, LPLVITEMW item)
{
	return static_cast<int>(send_message(window_handle, LVM_INSERTITEM, 0U, reinterpret_cast<LPARAM>(item)));
}

//=============================================================
// redraw_items
//=============================================================

BOOL mameui::winapi::controls::list_view::redraw_items(HWND window_handle, int first_item, int last_item)
{
	return static_cast<BOOL>(send_message(window_handle, LVM_REDRAWITEMS, static_cast<WPARAM>(first_item), static_cast<LPARAM>(last_item)));
}

//=============================================================
// set_bk_color
//=============================================================

BOOL mameui::winapi::controls::list_view::set_bk_color(HWND window_handle, COLORREF background_color)
{
	return static_cast<BOOL>(send_message(window_handle, LVM_SETBKCOLOR, 0U, static_cast<LPARAM>(background_color)));
}

//=============================================================
// set_bk_image
//=============================================================

BOOL mameui::winapi::controls::list_view::set_bk_image(HWND window_handle, LPLVBKIMAGEW bk_image_struct)
{
	return static_cast<BOOL>(send_message(window_handle, LVM_SETBKIMAGEW, 0U, reinterpret_cast<LPARAM>(bk_image_struct)));
}

//=============================================================
// set_extended_list_view_style
//=============================================================

DWORD mameui::winapi::controls::list_view::set_extended_list_view_style(HWND window_handle, DWORD style)
{
	return static_cast<DWORD>(send_message(window_handle, LVM_SETEXTENDEDLISTVIEWSTYLE, 0U, static_cast<LPARAM>(style)));
}

//=============================================================
// set_image_list
//=============================================================

HIMAGELIST mameui::winapi::controls::list_view::set_image_list(HWND window_handle, HIMAGELIST image_list_handle, int image_list)
{
	return reinterpret_cast<HIMAGELIST>(send_message(window_handle, LVM_SETIMAGELIST, static_cast<WPARAM>(image_list), reinterpret_cast<LPARAM>(image_list_handle)));
}

//=============================================================
// set_item_count
//=============================================================

int mameui::winapi::controls::list_view::set_item_count(HWND window_handle, int item_count)
{
	return static_cast<int>(send_message(window_handle, LVM_SETITEMCOUNT, static_cast<WPARAM>(item_count), 0L));
}

//=============================================================
// set_item_state
//=============================================================

BOOL mameui::winapi::controls::list_view::set_item_state(HWND window_handle, int item_index, LPLVITEMW item)
{
	return static_cast<BOOL>(send_message(window_handle, LVM_SETITEMSTATE, static_cast<WPARAM>(item_index), reinterpret_cast<LPARAM>(item)));
}

void mameui::winapi::controls::list_view::set_item_state(HWND window_handle, int item_index, UINT data, UINT mask)
{
	LVITEMW set_state_item{};
	set_state_item.stateMask = mask;
	set_state_item.state = data;

	set_item_state(window_handle, item_index, &set_state_item);
}

//=============================================================
// set_text_color
//=============================================================

BOOL mameui::winapi::controls::list_view::set_text_color(HWND window_handle, COLORREF text_color)
{
	return static_cast<BOOL>(send_message(window_handle, LVM_SETTEXTCOLOR, 0U, static_cast<LPARAM>(text_color)));
}

//=============================================================
// set_text_bk_color
//=============================================================

BOOL mameui::winapi::controls::list_view::set_text_bk_color(HWND window_handle, COLORREF background_color)
{
	return static_cast<BOOL>(send_message(window_handle, LVM_SETTEXTBKCOLOR, 0U, static_cast<LPARAM>(background_color)));
}

//=============================================================
// sort_item
//=============================================================

BOOL mameui::winapi::controls::list_view::sort_item(HWND window_handle, PFNLVCOMPARE comparison_function, LPARAM comparable)
{
	return static_cast<BOOL>(send_message(window_handle, LVM_SORTITEMS, static_cast<WPARAM>(comparable), reinterpret_cast<LPARAM>(comparison_function)));
}


//=============================================================================
// progressbar control
//=============================================================================

//=============================================================
// set_pos
//=============================================================

int mameui::winapi::controls::progress_bar::set_pos(HWND window_handle, int position)
{
	return static_cast<int>(send_message(window_handle, PBM_SETPOS, static_cast<WPARAM>(position), 0L));
}

//=============================================================
// set_text
//=============================================================

BOOL mameui::winapi::controls::progress_bar::set_text(HWND window_handle, const wchar_t *text)
{
	return set_window_text(window_handle, text);
}

//=============================================================
// set_text_utf8
//=============================================================

BOOL mameui::winapi::controls::progress_bar::set_text_utf8(HWND window_handle, const char *text)
{
	return set_window_text_utf8(window_handle, text);
}


//=============================================================================
// property sheet control
//=============================================================================

BOOL mameui::winapi::controls::property_sheet::changed(HWND window_handle, HWND page_handle)
{
	return static_cast<BOOL>(send_message(window_handle, PSM_CHANGED, reinterpret_cast<WPARAM>(window_handle), 0L));
}

HWND mameui::winapi::controls::property_sheet::get_tab_control(HWND window_handle)
{
	return reinterpret_cast<HWND>(send_message(window_handle, PSM_GETTABCONTROL, 0U, 0L));
}

void mameui::winapi::controls::property_sheet::unchanged(HWND window_handle, HWND page_handle)
{
	(void)send_message(window_handle, PSM_UNCHANGED, reinterpret_cast<WPARAM>(window_handle), 0L);
}

//=============================================================================
// scroll bar
//=============================================================================

//=============================================================
// get_scroll_info
//=============================================================

BOOL mameui::winapi::controls::scroll_bar::get_scroll_info(HWND window_handle, int scroll_bar, LPSCROLLINFO scroll_info)
{
	return GetScrollInfo(window_handle, scroll_bar, scroll_info);
}

//=============================================================
// get_scroll_pos
//=============================================================

int mameui::winapi::controls::scroll_bar::get_scroll_pos(HWND window_handle, int scroll_bar)
{
	return GetScrollPos(window_handle, scroll_bar);
}

//=============================================================
// scroll_window_ex
//=============================================================

int mameui::winapi::controls::scroll_bar::scroll_window_ex(HWND window_handle, int dx, int dy, LPCRECT scroll_area_rectangle, LPCRECT clipping_rectangle, HRGN update_region_handle, LPRECT update_rectangle, UINT flags)
{
	return ScrollWindowEx(window_handle, dx, dy, scroll_area_rectangle, clipping_rectangle, update_region_handle, update_rectangle, flags);
}

//=============================================================
// set_scroll_info
//=============================================================

int mameui::winapi::controls::scroll_bar::set_scroll_info(HWND window_handle, int scroll_bar_type, LPCSCROLLINFO scroll_info, BOOL redraw_control)
{
	return SetScrollInfo(window_handle, scroll_bar_type, scroll_info,redraw_control);
}

//=============================================================
// set_scroll_pos
//=============================================================

int mameui::winapi::controls::scroll_bar::set_scroll_pos(HWND window_handle, int scroll_bar, int position, BOOL redraw)
{
	return SetScrollPos(window_handle, scroll_bar, position, redraw);
}


//=============================================================================
// static control
//=============================================================================

//=============================================================
// set_image
//=============================================================

HBITMAP mameui::winapi::controls::static_control::set_image(HWND window_handle, int image_type, HBITMAP image_handle)
{
	return reinterpret_cast<HBITMAP>(send_message(window_handle, STM_SETIMAGE, static_cast<WPARAM>(image_type), reinterpret_cast<LPARAM>(image_handle)));
}


//=============================================================================
// statusbar control
//=============================================================================

//=============================================================
// get_rect
//=============================================================

BOOL mameui::winapi::controls::status_bar::get_rect(HWND window_handle, int part_index, LPRECT rectangle)
{
	return static_cast<BOOL>(send_message(window_handle, SB_GETRECT, static_cast<WPARAM>(part_index), reinterpret_cast<LPARAM>(rectangle)));
}

//=============================================================
// set_text
//=============================================================

BOOL mameui::winapi::controls::status_bar::set_text(HWND window_handle, int part_index, int drawing_op, const wchar_t *text)
{
	return static_cast<BOOL>(send_message(window_handle, SB_SETTEXTW, MAKEWPARAM(part_index, drawing_op), reinterpret_cast<LPARAM>(text)));
}

//=============================================================
// set_text_utf8
//=============================================================

BOOL mameui::winapi::controls::status_bar::set_text_utf8(HWND window_handle, int part_index, int drawing_op, const char *text)
{
	return static_cast<BOOL>(send_message(window_handle, SB_SETTEXTA, MAKEWPARAM(part_index, drawing_op), reinterpret_cast<LPARAM>(text)));
}


//=============================================================================
// tab control
//=============================================================================

//=============================================================
// delete_all_items
//=============================================================

BOOL mameui::winapi::controls::tab_control::delete_all_items(HWND window_handle)
{
	return static_cast<BOOL>(send_message(window_handle, TCM_DELETEALLITEMS, 0, 0L));
}

int mameui::winapi::controls::tab_control::insert_item(HWND window_handle, int item_index, const LPTCITEMW item)
{
	return static_cast<int>(send_message(window_handle, TCM_INSERTITEM, static_cast<WPARAM>(item_index), reinterpret_cast<LPARAM>(item)));
}


//=============================================================================
// toolbar control
//=============================================================================

//=============================================================
// check_button
//=============================================================

BOOL mameui::winapi::controls::tool_bar::check_button(HWND window_handle, UINT button_id, BOOL check_state)
{
	return static_cast<BOOL>(send_message(window_handle, TB_CHECKBUTTON, static_cast<WPARAM>(button_id), MAKELPARAM(check_state, 0)));
}


//=============================================================================
// trackbar control
//=============================================================================

//=============================================================
// get_pos
//=============================================================

int mameui::winapi::controls::track_bar::get_pos(HWND window_handle)
{
	return static_cast<int>(send_message(window_handle, TBM_GETPOS, 0U, 0L));
}

//=============================================================
// set_pos
//=============================================================

void mameui::winapi::controls::track_bar::set_pos(HWND window_handle, BOOL redraw, int position)
{
	send_message(window_handle, TBM_SETPOS, static_cast<WPARAM>(redraw), static_cast<LPARAM>(position));
}

//=============================================================
// set_range_max
//=============================================================

void mameui::winapi::controls::track_bar::set_range_max(HWND window_handle, BOOL redraw, int max_pos)
{
	send_message(window_handle, TBM_SETRANGEMAX, static_cast<WPARAM>(redraw), static_cast<LPARAM>(max_pos));
}

//=============================================================
// set_range_min
//=============================================================

void mameui::winapi::controls::track_bar::set_range_min(HWND window_handle, BOOL redraw, int min_pos)
{
	send_message(window_handle, TBM_SETRANGEMIN, static_cast<WPARAM>(redraw), static_cast<LPARAM>(min_pos));
}


//=============================================================================
// tree-view control
//=============================================================================


//=============================================================
// delete_all_items
//=============================================================

BOOL mameui::winapi::controls::tree_view::delete_all_items(HWND window_handle)
{
	return delete_item(window_handle, TVI_ROOT);
}

//=============================================================
// delete_item
//=============================================================

BOOL mameui::winapi::controls::tree_view::delete_item(HWND window_handle, HTREEITEM tree_item_handle)
{
	return static_cast<BOOL>(send_message(window_handle, TVM_DELETEITEM, 0U, reinterpret_cast<LPARAM>(tree_item_handle)));
}

//=============================================================
// edit_label
//=============================================================

HWND mameui::winapi::controls::tree_view::edit_label(HWND window_handle, HTREEITEM tree_item_handle)
{
	return reinterpret_cast<HWND>(send_message(window_handle, TVM_EDITLABELW, 0U, reinterpret_cast<LPARAM>(tree_item_handle)));
}

//=============================================================
// get_child
//=============================================================

HTREEITEM mameui::winapi::controls::tree_view::get_child(HWND window_handle, HTREEITEM tree_item_handle)
{
	return get_next_item(window_handle, tree_item_handle, TVGN_CHILD);
}

//=============================================================
// get_image_list
//=============================================================

HIMAGELIST mameui::winapi::controls::tree_view::get_image_list(HWND window_handle, int image_list_type)
{
	return reinterpret_cast<HIMAGELIST>(send_message(window_handle, TVM_GETIMAGELIST, static_cast<WPARAM>(image_list_type), 0L));
}

//=============================================================
// get_item
//=============================================================

BOOL mameui::winapi::controls::tree_view::get_item(HWND window_handle, LPTVITEMW item)
{
	return static_cast<BOOL>(send_message(window_handle, TVM_GETITEMW, 0U, reinterpret_cast<LPARAM>(item)));
}

//=============================================================
// get_item_rect
//=============================================================

BOOL mameui::winapi::controls::tree_view::get_item_rect(HWND window_handle, HTREEITEM tree_item_handle, LPRECT bounding_rectangle, BOOL text_only)
{
	*reinterpret_cast<HTREEITEM*>(bounding_rectangle) = tree_item_handle;
	return static_cast<BOOL>(send_message(window_handle, TVM_GETITEMRECT, static_cast<WPARAM>(text_only), reinterpret_cast<LPARAM>(bounding_rectangle)));
}

//=============================================================
// get_next_item
//=============================================================

HTREEITEM mameui::winapi::controls::tree_view::get_next_item(HWND window_handle, HTREEITEM tree_item_handle, UINT flag)
{
	return reinterpret_cast<HTREEITEM>(send_message(window_handle, TVM_GETNEXTITEM, static_cast<WPARAM>(flag), reinterpret_cast<LPARAM>(tree_item_handle)));
}

//=============================================================
// get_next_sibling
//=============================================================

HTREEITEM mameui::winapi::controls::tree_view::get_next_sibling(HWND window_handle, HTREEITEM tree_item_handle)
{
	return get_next_item(window_handle, tree_item_handle, TVGN_NEXT);
}

//=============================================================
// get_parent
//=============================================================

HTREEITEM mameui::winapi::controls::tree_view::get_parent(HWND window_handle, HTREEITEM tree_item_handle)
{
	return get_next_item(window_handle, tree_item_handle, TVGN_PARENT);
}

//=============================================================
// get_root
//=============================================================

HTREEITEM mameui::winapi::controls::tree_view::get_root(HWND window_handle)
{
	return get_next_item(window_handle, nullptr, TVGN_ROOT);
}

//=============================================================
// get_selection
//=============================================================

HTREEITEM mameui::winapi::controls::tree_view::get_selection(HWND window_handle)
{
	return reinterpret_cast<HTREEITEM>(send_message(window_handle, TVM_GETNEXTITEM, static_cast<WPARAM>(TVGN_CARET), 0L));
}

//=============================================================
// hit_test
//=============================================================

HTREEITEM mameui::winapi::controls::tree_view::hit_test(HWND window_handle, LPTV_HITTESTINFO hit_test_info)
{
	return reinterpret_cast<HTREEITEM>(send_message(window_handle, TVM_HITTEST, 0U, reinterpret_cast<LPARAM>(hit_test_info)));
}

//=============================================================
// insert_item
//=============================================================

HTREEITEM mameui::winapi::controls::tree_view::insert_item(HWND window_handle, LPTVINSERTSTRUCTW insert_struct)
{
	return reinterpret_cast<HTREEITEM>(send_message(window_handle, TVM_INSERTITEMW, 0U, reinterpret_cast<LPARAM>(insert_struct)));
}

//=============================================================
// select_drop_target
//=============================================================

BOOL mameui::winapi::controls::tree_view::select_drop_target(HWND window_handle, HTREEITEM item)
{
	return static_cast<BOOL>(send_message(window_handle, TVM_SELECTITEM, static_cast<WPARAM>(TVGN_DROPHILITE), reinterpret_cast<LPARAM>(item)));
}

//=============================================================
// select_item
//=============================================================

BOOL mameui::winapi::controls::tree_view::select_item(HWND window_handle, HTREEITEM item)
{
	return static_cast<BOOL>(send_message(window_handle, TVM_SELECTITEM, static_cast<WPARAM>(TVGN_CARET), reinterpret_cast<LPARAM>(item)));
}

//=============================================================
// set_bk_color
//=============================================================

BOOL mameui::winapi::controls::tree_view::set_bk_color(HWND window_handle, COLORREF color)
{
	return static_cast<BOOL>(send_message(window_handle, LVM_SETBKCOLOR, 0U, static_cast<LPARAM>(color)));
}

//=============================================================
// set_image_list
//=============================================================

HIMAGELIST mameui::winapi::controls::tree_view::set_image_list(HWND window_handle, HIMAGELIST image_list, int image_list_type)
{
	return reinterpret_cast<HIMAGELIST>(send_message(window_handle, LVM_SETIMAGELIST, static_cast<WPARAM>(image_list_type), reinterpret_cast<LPARAM>(image_list)));
}

//=============================================================
// set_text_color
//=============================================================

COLORREF mameui::winapi::controls::tree_view::set_text_color(HWND window_handle, COLORREF color)
{
	return static_cast<COLORREF>(send_message(window_handle, TVM_SETTEXTCOLOR, 0U, static_cast<LPARAM>(color)));
}

//=============================================================
// set_window_theme
//=============================================================

HRESULT mameui::winapi::controls::window::set_window_theme(HWND window_handle, const wchar_t* app_name, const wchar_t *id_list)
{
	return SetWindowTheme(window_handle, app_name, id_list);
}

//=============================================================
// set_window_theme_utf8
//=============================================================

HRESULT mameui::winapi::controls::window::set_window_theme_utf8(HWND window_handle, const char* app_name, const char *id_list)
{
	if (!window_handle || !app_name || !id_list)
		return E_INVALIDARG;

	std::unique_ptr<wchar_t[]> utf16_app_name(mui_utf16_from_utf8cstring(app_name));
	std::unique_ptr<wchar_t[]> utf16_id_list(mui_utf16_from_utf8cstring(id_list));

	return set_window_theme(window_handle, utf16_app_name.get(), utf16_id_list.get());
}
