// license:BSD-3-Clause
// copyright-holders:0perator

/***************************************************************************

  winapi_controls.cpp

 ***************************************************************************/

// standard windows headers
 
// standard C++ headers
#include <memory>

// MAME/MAMEUI headers
#include "winapi_controls.h"
#include "mui_wcsconv.h"
#include "winapi_windows.h"

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
	return (int)send_message(window_handle, BM_GETCHECK, 0, 0L);
}


//============================================================
// set_check
//============================================================

void mameui::winapi::controls::button_control::set_check(HWND window_handle, int check_state)
{
	(void)send_message(window_handle, BM_SETCHECK, (WPARAM)check_state, 0L);
}


//=============================================================================
// combo-box control
//=============================================================================

//============================================================
// add_string
//============================================================

int mameui::winapi::controls::combo_box::add_string(HWND window_handle, LPARAM add_string_data)
{
	return (int)send_message(window_handle, CB_ADDSTRING, 0, add_string_data);
}


//============================================================
// set_cue_banner_text
//============================================================

BOOL mameui::winapi::controls::combo_box::set_cue_banner_text(HWND window_handle, const wchar_t *banner_text)
{
	return (BOOL)send_message(window_handle, CB_SETCUEBANNER, 0, (LPARAM)banner_text);
}


//============================================================
// find_string
//============================================================

int mameui::winapi::controls::combo_box::find_string(HWND window_handle, int start_index, LPARAM find_string_data)
{
	return (int)send_message(window_handle, CB_FINDSTRING, (WPARAM)start_index, find_string_data);
}

//============================================================
// get_count
//============================================================

int mameui::winapi::controls::combo_box::get_count(HWND window_handle)
{
	return (int)send_message(window_handle, CB_GETCOUNT, 0, 0L);
}


//============================================================
// get_cur_sel
//============================================================

int mameui::winapi::controls::combo_box::get_cur_sel(HWND window_handle)
{
	return (int)send_message(window_handle, CB_GETCURSEL, 0, 0L);
}


//============================================================
// get_item_data
//============================================================

LRESULT mameui::winapi::controls::combo_box::get_item_data(HWND window_handle, int item_index)
{
	return send_message(window_handle, CB_GETITEMDATA, (WPARAM)item_index, 0L);
}


//============================================================
// insert_string
//============================================================

int mameui::winapi::controls::combo_box::insert_string(HWND window_handle, int item_index, LPARAM insert_string_data)
{
	return (int)send_message(window_handle, CB_INSERTSTRING, (WPARAM)item_index, insert_string_data);
}


//============================================================
// reset_content
//============================================================

int mameui::winapi::controls::combo_box::reset_content(HWND window_handle)
{
	return (int)send_message(window_handle, CB_RESETCONTENT, 0, 0L);
}


//============================================================
// set_cur_sel
//============================================================

int mameui::winapi::controls::combo_box::set_cur_sel(HWND window_handle, int item_index)
{
	return (int)send_message(window_handle, CB_SETCURSEL, (WPARAM)item_index, 0L);
}


//============================================================
// set_item_data
//============================================================

int mameui::winapi::controls::combo_box::set_item_data(HWND window_handle, int item_index,LPARAM item_data)
{
	return (int)send_message(window_handle, CB_SETITEMDATA, (WPARAM)item_index, item_data);
}


//=============================================================================
// edit control
//=============================================================================

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
	(void)send_message(window_handle, EM_REPLACESEL, 0, (LPARAM)replacement);
}


//=============================================================================
// set_sel
//=============================================================================

void mameui::winapi::controls::edit_control::set_sel(HWND window_handle, int begin_pos, int end_pos)
{
	(void)send_message(window_handle, EM_SETSEL, (WPARAM)begin_pos, (LPARAM)end_pos);
}


//=============================================================================
// header control
//=============================================================================

//=============================================================
// get_item_count
//=============================================================

int mameui::winapi::controls::header_control::get_item_count(HWND window_handle)
{
	return (int)send_message(window_handle, HDM_GETITEMCOUNT, 0, 0L);
}


//=============================================================
// get_item_rect
//=============================================================

BOOL mameui::winapi::controls::header_control::get_item_rect(HWND window_handle, int item_index, LPRECT rectangle)
{
	return (BOOL)send_message(window_handle, HDM_GETITEMRECT, (WPARAM)item_index, (LPARAM)rectangle);
}


//=============================================================
// set_item
//=============================================================

BOOL mameui::winapi::controls::header_control::set_item(HWND window_handle, int item_index, LPHDITEMW header_item)
{
	return (BOOL)send_message(window_handle, HDM_SETITEMW, (WPARAM)item_index, (LPARAM)header_item);
}


//=============================================================================
// image_list control
//=============================================================================

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
// add_icon
//=============================================================


int mameui::winapi::controls::image_list::add_icon(HIMAGELIST image_list_handle, HICON icon)
{
	return replace_icon(image_list_handle, -1, icon);
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

void mameui::winapi::controls::list_view::create_drag_image(HWND window_handle, int item_index, LPPOINT upper_left)
{
	(void)send_message(window_handle, LVM_CREATEDRAGIMAGE, (WPARAM)item_index, (LPARAM)upper_left);
}


//=============================================================
// delete_column
//=============================================================

BOOL mameui::winapi::controls::list_view::delete_column(HWND window_handle, int column_index)
{
	return (BOOL)send_message(window_handle, LVM_DELETECOLUMN, (WPARAM)column_index, 0L);
}


//=============================================================
// delete_all_items
//=============================================================

BOOL mameui::winapi::controls::list_view::delete_all_items(HWND window_handle)
{
	return (BOOL)send_message(window_handle, LVM_DELETEALLITEMS, 0, 0L);
}


//=============================================================
// delete_item
//=============================================================

BOOL mameui::winapi::controls::list_view::delete_item(HWND window_handle, int item_index)
{
	return (BOOL)send_message(window_handle, LVM_DELETEITEM, (WPARAM)item_index, 0L);
}


//=============================================================
// ensure_visible
//=============================================================

BOOL mameui::winapi::controls::list_view::ensure_visible(HWND window_handle, int item_index,BOOL partial_visibility_ok)
{
	return (BOOL)send_message(window_handle, LVM_ENSUREVISIBLE, (WPARAM)item_index, (LPARAM)partial_visibility_ok);
}


//=============================================================
// find_item
//=============================================================

int mameui::winapi::controls::list_view::find_item(HWND window_handle, int starting_index, const LPFINDINFOW find_info)
{
	return (int)send_message(window_handle, LVM_FINDITEM, (WPARAM)starting_index, (LPARAM)find_info);
}


//=============================================================
// get_bk_image
//=============================================================

BOOL mameui::winapi::controls::list_view::get_bk_image(HWND window_handle, LPLVBKIMAGEW bk_image_struct)
{
	return (BOOL)send_message(window_handle, LVM_GETBKIMAGEW, 0, (LPARAM)bk_image_struct);
}


//=============================================================
// get_column
//=============================================================

BOOL mameui::winapi::controls::list_view::get_column(HWND window_handle, int column_index, LPLVCOLUMNW column)
{
	return (BOOL)send_message(window_handle, LVM_GETCOLUMNW, (WPARAM)column_index, (LPARAM)column);
}


//=============================================================
// get_column_order_array
//=============================================================

BOOL mameui::winapi::controls::list_view::get_column_order_array(HWND window_handle, int column_count, LPINT column_order_array)
{
	return (BOOL)send_message(window_handle, LVM_GETCOLUMNORDERARRAY, (WPARAM)column_count, (LPARAM)column_order_array);
}


//=============================================================
// get_column_width
//=============================================================

int mameui::winapi::controls::list_view::get_column_width(HWND window_handle, int column_index)
{
	return (int)send_message(window_handle, LVM_GETCOLUMNWIDTH, (WPARAM)column_index, 0L);
}


//=============================================================
// get_count_per_page
//=============================================================

int mameui::winapi::controls::list_view::get_count_per_page(HWND window_handle)
{
	return (int)send_message(window_handle, LVM_GETCOUNTPERPAGE, 0, 0L);
}


//=============================================================
// get_header
//=============================================================

HWND mameui::winapi::controls::list_view::get_header(HWND window_handle)
{
	return (HWND)send_message(window_handle, LVM_GETHEADER, 0, 0L);
}


//=============================================================
// get_image_list
//=============================================================

HIMAGELIST mameui::winapi::controls::list_view::get_image_list(HWND window_handle, int image_list)
{
	return (HIMAGELIST)send_message(window_handle, LVM_GETIMAGELIST, (WPARAM)image_list, 0L);
}


//=============================================================
// get_item
//=============================================================

BOOL mameui::winapi::controls::list_view::get_item(HWND window_handle, LPLVITEMW item)
{
	return (BOOL)send_message(window_handle, LVM_GETITEM, 0, (LPARAM)item);
}


//=============================================================
// get_item_count
//=============================================================

int mameui::winapi::controls::list_view::get_item_count(HWND window_handle)
{
	return (int)send_message(window_handle, LVM_GETITEMCOUNT, 0, 0L);
}


//=============================================================
// get_item_rect
//=============================================================

BOOL mameui::winapi::controls::list_view::get_item_rect(HWND window_handle, int item_index, LPRECT rectangle)
{
	return (BOOL)send_message(window_handle, LVM_GETITEMRECT, (WPARAM)item_index, (LPARAM)rectangle);
}


//=============================================================
// get_item_text
//=============================================================

int mameui::winapi::controls::list_view::get_item_text(HWND window_handle, int item_index, LPLVITEMW item)
{
	return (int)send_message(window_handle, LVM_GETITEMTEXTW, (WPARAM)item_index, (LPARAM)item);
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
// get_next_item
//=============================================================

int mameui::winapi::controls::list_view::get_next_item(HWND window_handle, int item_index, UINT flags)
{
	return (int)send_message(window_handle, LVM_GETNEXTITEM, (WPARAM)item_index, MAKELPARAM(flags, 0));
}


//=============================================================
// get_top_index
//=============================================================

int mameui::winapi::controls::list_view::get_top_index(HWND window_handle)
{
	return (int)send_message(window_handle, LVM_GETTOPINDEX, 0, 0L);
}


//=============================================================
// hit_test
//=============================================================

int mameui::winapi::controls::list_view::hit_test(HWND window_handle, LPLVHITTESTINFO hit_test_info)
{
	return (int)send_message(window_handle, LVM_HITTEST, 0, (LPARAM)hit_test_info);
}


//=============================================================
// insert_column
//=============================================================

int mameui::winapi::controls::list_view::insert_column(HWND window_handle, int column_index, LPLVCOLUMNW column)
{
	return (int)send_message(window_handle, LVM_INSERTCOLUMN, (WPARAM)column_index, (LPARAM)column);
}


//=============================================================
// insert_item
//=============================================================

int mameui::winapi::controls::list_view::insert_item(HWND window_handle, LPLVITEMW item)
{
	return (int)send_message(window_handle, LVM_INSERTITEM, 0, (LPARAM)item);
}


//=============================================================
// redraw_items
//=============================================================

BOOL mameui::winapi::controls::list_view::redraw_items(HWND window_handle, int first_item, int last_item)
{
	return (BOOL)send_message(window_handle, LVM_REDRAWITEMS, (WPARAM)first_item, (LPARAM)last_item);
}


//=============================================================
// set_bk_image
//=============================================================

BOOL mameui::winapi::controls::list_view::set_bk_image(HWND window_handle, LPLVBKIMAGEW bk_image_struct)
{
	return (BOOL)send_message(window_handle, LVM_SETBKIMAGEW, 0, (LPARAM)bk_image_struct);
}


//=============================================================
// set_extended_list_view_style
//=============================================================

DWORD mameui::winapi::controls::list_view::set_extended_list_view_style(HWND window_handle, DWORD style)
{
	return (DWORD)send_message(window_handle, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, (LPARAM)style);
}


//=============================================================
// set_image_list
//=============================================================

HIMAGELIST mameui::winapi::controls::list_view::set_image_list(HWND window_handle, HIMAGELIST image_list_handle, int image_list)
{
	return (HIMAGELIST)send_message(window_handle, LVM_SETIMAGELIST, (WPARAM)image_list, (LPARAM)image_list_handle);
}


//=============================================================
// set_item_count
//=============================================================

int mameui::winapi::controls::list_view::set_item_count(HWND window_handle, int item_count)
{
	return (int)send_message(window_handle, LVM_SETITEMCOUNT, (WPARAM)item_count, 0L);
}


//=============================================================
// set_item_state
//=============================================================

BOOL mameui::winapi::controls::list_view::set_item_state(HWND window_handle, int item_index, LPLVITEMW item)
{
	return (BOOL)send_message(window_handle, LVM_SETITEMSTATE, (WPARAM)item_index, (LPARAM)item);
}

void mameui::winapi::controls::list_view::set_item_state(HWND window_handle, int item_index, UINT data, UINT mask)
{
	LVITEMW set_state_item{};

	set_state_item.stateMask = mask;
	set_state_item.state = data;

	return (void)set_item_state(window_handle, item_index, &set_state_item);
}


//=============================================================
// set_text_color
//=============================================================

BOOL mameui::winapi::controls::list_view::set_text_color(HWND window_handle, COLORREF text_color)
{
	return (BOOL)send_message(window_handle, LVM_SETTEXTCOLOR, 0, (LPARAM)text_color);
}


//=============================================================
// set_text_bk_color
//=============================================================

BOOL mameui::winapi::controls::list_view::set_text_bk_color(HWND window_handle, COLORREF background_color)
{
	return (BOOL)send_message(window_handle, LVM_SETTEXTBKCOLOR, 0, (LPARAM)background_color);
}


//=============================================================
// sort_item
//=============================================================

BOOL mameui::winapi::controls::list_view::sort_item(HWND window_handle, PFNLVCOMPARE comparison_function, LPARAM comparable)
{
	return (BOOL)send_message(window_handle, LVM_SORTITEMS, (WPARAM)comparable, (LPARAM)comparison_function);
}


//=============================================================================
// progressbar control
//=============================================================================

//=============================================================
// set_pos
//=============================================================

int mameui::winapi::controls::progress_bar::set_pos(HWND window_handle, int position)
{
	return (int)send_message(window_handle, PBM_SETPOS, (WPARAM)position, 0L);
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
// scroll bar
//=============================================================================

//=============================================================
// get_scroll_pos
//=============================================================


int mameui::winapi::controls::scroll_bar::get_scroll_pos(HWND window_handle, int scroll_bar)
{
	return GetScrollPos(window_handle, scroll_bar);
}


//=============================================================================
// static control
//=============================================================================

//=============================================================
// set_image
//=============================================================

HBITMAP mameui::winapi::controls::static_control::set_image(HWND window_handle, int image_type, HBITMAP image_handle)
{
	return (HBITMAP)send_message(window_handle, STM_SETIMAGE, (WPARAM)image_type, (LPARAM)image_handle);
}


//=============================================================================
// statusbar control
//=============================================================================


//=============================================================
// get_rect
//=============================================================

BOOL mameui::winapi::controls::status_bar::get_rect(HWND window_handle, int part_index, LPRECT rectangle)
{
	return (BOOL)send_message(window_handle, SB_GETRECT, (WPARAM)part_index, (LPARAM)rectangle);
}


//=============================================================
// set_text
//=============================================================

BOOL mameui::winapi::controls::status_bar::set_text(HWND window_handle, int part_index, int drawing_op, const wchar_t *text)
{
	return (BOOL)send_message(window_handle, SB_SETTEXTW, MAKEWPARAM(part_index, drawing_op), (LPARAM)text);
}


//=============================================================
// set_text_utf8
//=============================================================

BOOL mameui::winapi::controls::status_bar::set_text_utf8(HWND window_handle, int part_index, int drawing_op, const char *text)
{
	std::unique_ptr<wchar_t[]>  wcs_text(mui_wcstring_from_utf8(text));

	if (!wcs_text)
		return (BOOL)0;

	return set_text(window_handle, part_index, drawing_op, wcs_text.get());
}


//=============================================================================
// tab control
//=============================================================================

//=============================================================
// delete_all_items
//=============================================================

BOOL mameui::winapi::controls::tab_control::delete_all_items(HWND window_handle)
{
	return (BOOL)send_message(window_handle, TCM_DELETEALLITEMS, (WPARAM)0, (LPARAM)0L);
}

int mameui::winapi::controls::tab_control::insert_item(HWND window_handle, int item_index, const LPTCITEMW item)
{
	return (int)send_message(window_handle, TCM_INSERTITEM, (WPARAM)item_index, (LPARAM)item);
}

//=============================================================================
// toolbar control
//=============================================================================

//=============================================================
// check_button
//=============================================================

BOOL mameui::winapi::controls::tool_bar::check_button(HWND window_handle, UINT button_id, BOOL check_state)
{
	return (BOOL)send_message(window_handle, TB_CHECKBUTTON, (WPARAM)button_id, MAKELPARAM(check_state, 0));
}


//=============================================================================
// trackbar control
//=============================================================================

//=============================================================
// get_pos
//=============================================================

int mameui::winapi::controls::track_bar::get_pos(HWND window_handle)
{
	return (int)send_message(window_handle, TBM_GETPOS, (WPARAM)0, (LPARAM)0L);
}


//=============================================================
// set_pos
//=============================================================

void mameui::winapi::controls::track_bar::set_pos(HWND window_handle, BOOL redraw, int position)
{
	(void)send_message(window_handle, TBM_SETPOS, (WPARAM)redraw, (LPARAM)position);
}


//=============================================================
// set_range_max
//=============================================================

void mameui::winapi::controls::track_bar::set_range_max(HWND window_handle, BOOL redraw, int max_pos)
{
	(void)send_message(window_handle, TBM_SETRANGEMAX, (WPARAM)redraw, (LPARAM)max_pos);
}


//=============================================================
// set_range_min
//=============================================================

void mameui::winapi::controls::track_bar::set_range_min(HWND window_handle, BOOL redraw, int min_pos)
{
	(void)send_message(window_handle, TBM_SETRANGEMIN, (WPARAM)redraw, (LPARAM)min_pos);
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
	return (BOOL)send_message(window_handle, TVM_DELETEITEM, 0, (LPARAM)tree_item_handle);
}


//=============================================================
// edit_label
//=============================================================

HWND mameui::winapi::controls::tree_view::edit_label(HWND window_handle, HTREEITEM tree_item_handle)
{
	return (HWND)send_message(window_handle, TVM_EDITLABELW, 0, (LPARAM)tree_item_handle);
}


//=============================================================
// get_child
//=============================================================

HTREEITEM mameui::winapi::controls::tree_view::get_child(HWND window_handle, HTREEITEM tree_item_handle)
{
	return get_next_item(window_handle, tree_item_handle, TVGN_CHILD);
}


//=============================================================
// get_item
//=============================================================

BOOL mameui::winapi::controls::tree_view::get_item(HWND window_handle, LPTVITEMW item)
{
	return (BOOL)send_message(window_handle, TVM_GETITEMW, 0, (LPARAM)item);
}


//=============================================================
// get_item_rect
//=============================================================

BOOL mameui::winapi::controls::tree_view::get_item_rect(HWND window_handle, HTREEITEM tree_item_handle, LPRECT bounding_rectangle,BOOL text_only)
{
	*(HTREEITEM*)bounding_rectangle = tree_item_handle;
	return (BOOL)send_message(window_handle, TVM_GETITEMRECT, (WPARAM)text_only, (LPARAM)bounding_rectangle);
}


//=============================================================
// get_next_item
//=============================================================

HTREEITEM mameui::winapi::controls::tree_view::get_next_item(HWND window_handle, HTREEITEM tree_item_handle, UINT flag)
{
	return (HTREEITEM)send_message(window_handle, TVM_GETNEXTITEM, (WPARAM)flag, (LPARAM)tree_item_handle);
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
	return get_next_item(window_handle, 0, TVGN_ROOT);
}


//=============================================================
// get_selection
//=============================================================

HTREEITEM mameui::winapi::controls::tree_view::get_selection(HWND window_handle)
{
	return (HTREEITEM)send_message(window_handle, TVM_GETNEXTITEM, (WPARAM)TVGN_CARET, 0L);
}


//=============================================================
// hit_test
//=============================================================

HTREEITEM mameui::winapi::controls::tree_view::hit_test(HWND window_handle, LPTV_HITTESTINFO hit_test_info)
{
	return (HTREEITEM)send_message(window_handle, TVM_HITTEST, 0, (LPARAM)hit_test_info);
}


//=============================================================
// insert_item
//=============================================================

HTREEITEM mameui::winapi::controls::tree_view::insert_item(HWND window_handle, LPTVINSERTSTRUCTW insert_struct)
{
	return (HTREEITEM)send_message(window_handle, TVM_INSERTITEMW, 0, (LPARAM)insert_struct);
}


//=============================================================
// select_drop_target
//=============================================================

BOOL mameui::winapi::controls::tree_view::select_drop_target(HWND window_handle, HTREEITEM item)
{
	return (BOOL)send_message(window_handle, TVM_SELECTITEM, (WPARAM)TVGN_DROPHILITE, (LPARAM)item);
}


//=============================================================
// select_item
//=============================================================

BOOL mameui::winapi::controls::tree_view::select_item(HWND window_handle, HTREEITEM item)
{
	return (BOOL)send_message(window_handle, TVM_SELECTITEM, (WPARAM)TVGN_CARET, (LPARAM)item);
}


//=============================================================
// set_bk_color
//=============================================================

BOOL mameui::winapi::controls::tree_view::set_bk_color(HWND window_handle, COLORREF color)
{
	return (BOOL)send_message(window_handle, LVM_SETBKCOLOR, 0, (LPARAM)color);
}


//=============================================================
// set_image_list
//=============================================================

HIMAGELIST mameui::winapi::controls::tree_view::set_image_list(HWND window_handle, HIMAGELIST image_list, int image_list_type)
{
	return (HIMAGELIST)send_message(window_handle, LVM_SETIMAGELIST, (WPARAM)image_list_type, (LPARAM)image_list);
}


//=============================================================
// set_text_color
//=============================================================

COLORREF mameui::winapi::controls::tree_view::set_text_color(HWND window_handle, COLORREF color)
{
	return (COLORREF)send_message(window_handle, TVM_SETTEXTCOLOR, 0, (LPARAM)color);
}


//=============================================================================
// window control
//=============================================================================

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
	HRESULT result = 0L;
	std::unique_ptr<wchar_t[]>  wcs_app_name(mui_wcstring_from_utf8(app_name));

	if (!wcs_app_name)
		return result;

	std::unique_ptr<wchar_t[]>  wcs_id_list(mui_wcstring_from_utf8(id_list));

	if (!wcs_id_list)
		return result;

	result = set_window_theme(window_handle, wcs_app_name.get(), wcs_id_list.get());

	return result;
}
