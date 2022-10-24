// license:BSD-3-Clause
// copyright-holders:0perator

#ifndef MAMEUI_LIB_WINAPI_WINDOWS_CONTROLS_H
#define MAMEUI_LIB_WINAPI_WINDOWS_CONTROLS_H

namespace mameui::winapi
{
	namespace controls
	{
		namespace button_control
		{
			BOOL enable(HWND window_handle, BOOL enabled);

			int get_check(HWND window_handle);

			void set_check(HWND window_handle, int check_state);
		}

		namespace combo_box
		{
			int add_string(HWND window_handle, LPARAM add_string_data);

			int find_string(HWND window_handle, int start_index, LPARAM find_string_data);

			int get_count(HWND window_handle);

			int get_cur_sel(HWND window_handle);

			LRESULT get_item_data(HWND window_handle, int index);

			int insert_string(HWND window_handle, int index, LPARAM insert_string_data);

			int reset_content(HWND window_handle);

			BOOL set_cue_banner_text(HWND window_handle, const wchar_t *banner_text);

			int set_cur_sel(HWND window_handle, int index);

			int set_item_data(HWND window_handle, int index, LPARAM item_data);
		}

		namespace edit_control
		{
			int get_text(HWND window_handle, wchar_t *text, int max_count);
			wchar_t *get_text(HWND window_handle);

			int get_text_length(HWND window_handle);

			void replace_sel(HWND window_handle, const wchar_t *replacement);

			void set_sel(HWND window_handle, int begin_pos, int end_pos);
		}

		namespace header_control
		{
			int get_item_count(HWND window_handle);

			BOOL get_item_rect(HWND window_handle, int item_index, LPRECT rectangle);

			BOOL set_item(HWND window_handle, int item_index, LPHDITEMW header_item);
		}

		namespace image_list
		{
			int add_icon(HIMAGELIST image_list_handle, HICON icon);

			HIMAGELIST create(int image_width, int image_height, UINT flags, int initial_count, int grow_count);

			BOOL destroy(HIMAGELIST image_list_handle);

			BOOL draw(HIMAGELIST image_list_handle, int image_index, HDC device_context_handle, int x_coordinate, int y_coordinate, UINT style);

			BOOL draw_ex(HIMAGELIST image_list_handle, int image_index, HDC device_context_handle, int x_coordinate, int y_coordinate,
				int x_destination, int y_destination, COLORREF background_color, COLORREF foreground_color, UINT style);

			HICON get_icon(HIMAGELIST image_list_handle, int image_index, UINT flags);

			BOOL remove(HIMAGELIST image_list_handle, int image_index);
			BOOL remove_all(HIMAGELIST image_list_handle);

			int replace_icon(HIMAGELIST image_list_handle, int image_index, HICON icon);
		}

		namespace list_view
		{
			HIMAGELIST create_drag_image(HWND window_handle, int item_index, LPPOINT upper_left);

			BOOL delete_column(HWND window_handle, int column_index);

			BOOL delete_item(HWND window_handle, int index);

			BOOL delete_all_items(HWND window_handle);

			BOOL ensure_visible(HWND window_handle, int item_index, BOOL partial_visibility_ok);

			int find_item(HWND window_handle, int starting_index, const LPFINDINFOW find_info);

			BOOL get_bk_image(HWND window_handle, LPLVBKIMAGEW bk_image_struct);

			BOOL get_column(HWND window_handle, int column_index, LPLVCOLUMNW column);

			BOOL get_column_order_array(HWND window_handle, int column_count, LPINT column_order_array);

			int get_column_width(HWND window_handle, int column_index);

			int get_count_per_page(HWND window_handle);

			HWND get_header(HWND window_handle);

			HIMAGELIST get_image_list(HWND window_handle, int image_list);

			BOOL get_item(HWND window_handle, LPLVITEMW item);

			int get_item_count(HWND window_handle);

			BOOL get_item_rect(HWND window_handle, int item_index, LPRECT rectangle);

			UINT get_item_state(HWND window_handle, int item_index, UINT mask);

			int get_item_text(HWND window_handle, int index, LPLVITEMW item);
			int get_item_text(HWND window_handle, int index, int sub_item_index, wchar_t *text, int text_length);

			int get_item_text_utf8(HWND window_handle, int item_index, LPLVITEMA item);
			int get_item_text_utf8(HWND window_handle, int index, int sub_item_index, char* text, int text_length);

			int get_next_item(HWND window_handle, int index, UINT flags);

			int get_top_index(HWND window_handle);

			int hit_test(HWND window_handle, LPLVHITTESTINFO hit_test_info);

			int insert_column(HWND window_handle, int index, LPLVCOLUMNW column);

			int insert_item(HWND window_handle, LPLVITEMW item);

			BOOL redraw_items(HWND window_handle, int first_item, int last_item);

			BOOL set_bk_color(HWND window_handle, COLORREF background_color);

			BOOL set_bk_image(HWND window_handle, LPLVBKIMAGEW bk_image_struct);

			DWORD set_extended_list_view_style(HWND window_handle, DWORD style);

			HIMAGELIST set_image_list(HWND window_handle, HIMAGELIST image_list_handle, int image_list);

			int set_item_count(HWND window_handle, int item_count);

			BOOL set_item_state(HWND window_handle, int index, LPLVITEMW item);
			void set_item_state(HWND window_handle, int index, UINT data, UINT mask);

			BOOL set_text_color(HWND window_handle, COLORREF text_color);

			BOOL set_text_bk_color(HWND window_handle, COLORREF background_color);

			BOOL sort_item(HWND window_handle, PFNLVCOMPARE comparison_function, LPARAM comparable);
		}

		namespace progress_bar
		{
			int set_pos(HWND window_handle, int position);

			BOOL set_text(HWND window_handle, const wchar_t *text);
			BOOL set_text_utf8(HWND window_handle, const char *text);
		}

		namespace property_sheet
		{
			BOOL changed(HWND window_handle, HWND page_handle);

			HWND get_tab_control(HWND window_handle);

			void unchanged(HWND window_handle, HWND page_handle);
		}

		namespace scroll_bar
		{
			BOOL get_scroll_info(HWND window_handle, int scroll_bar, LPSCROLLINFO scroll_info);

			int get_scroll_pos(HWND window_handle, int scroll_bar);

			int scroll_window_ex(HWND window_handle, int dx, int dy, LPCRECT scroll_area_rectangle, LPCRECT clipping_rectangle, HRGN update_region_handle, LPRECT update_rectangle, UINT flags);

			int set_scroll_info(HWND window_handle, int scroll_bar_type, LPCSCROLLINFO scroll_info, BOOL redraw_control);

			int set_scroll_pos(HWND window_handle, int scroll_bar, int position, BOOL redraw);
		}

		namespace static_control
		{
			HBITMAP set_image(HWND window_handle, int image_type, HBITMAP image_handle);
		}

		namespace status_bar
		{
			BOOL get_rect(HWND window_handle, int part_index, LPRECT rectangle);

			BOOL set_text(HWND window_handle, int part_index, int drawing_op, const wchar_t *text);
			BOOL set_text_utf8(HWND window_handle, int part_index, int drawing_op, const char *text);
		}

		namespace tab_control
		{
			BOOL delete_all_items(HWND window_handle);

			int insert_item(HWND window_handle, int index, const LPTCITEMW item);
		}
		namespace tool_bar
		{
			BOOL check_button(HWND window_handle, UINT button_id, BOOL check_state);
		}

		namespace track_bar
		{
			int get_pos(HWND window_handle);
			void set_pos(HWND window_handle, BOOL redraw, int position);

			void set_range_max(HWND window_handle, BOOL redraw, int max_pos);

			void set_range_min(HWND window_handle, BOOL redraw, int min_pos);
		}

		namespace tree_view
		{
			BOOL delete_all_items(HWND window_handle);

			BOOL delete_item(HWND window_handle, HTREEITEM tree_item_handle);

			HWND edit_label(HWND window_handle, HTREEITEM tree_item_handle);

			HTREEITEM get_child(HWND window_handle, HTREEITEM tree_item_handle);

			HIMAGELIST get_image_list(HWND window_handle, int image_list_type);

			BOOL get_item(HWND window_handle, LPTVITEMW item);

			BOOL get_item_rect(HWND window_handle, HTREEITEM tree_item_handle, LPRECT bounding_rectangle, BOOL text_only);

			HTREEITEM get_next_item(HWND window_handle, HTREEITEM tree_item_handle, UINT flag);

			HTREEITEM get_next_sibling(HWND window_handle, HTREEITEM tree_item_handle);

			HTREEITEM get_parent(HWND window_handle, HTREEITEM tree_item_handle);

			HTREEITEM get_root(HWND window_handle);

			HTREEITEM get_selection(HWND window_handle);

			HTREEITEM hit_test(HWND window_handle, LPTV_HITTESTINFO hit_test_info);

			HTREEITEM insert_item(HWND window_handle, LPTVINSERTSTRUCTW insert_struct);

			BOOL select_drop_target(HWND window_handle, HTREEITEM item);

			BOOL select_item(HWND window_handle, HTREEITEM item);

			BOOL set_bk_color(HWND window_handle, COLORREF color);

			HIMAGELIST set_image_list(HWND window_handle, HIMAGELIST image_list, int image_list_type);

			COLORREF set_text_color(HWND window_handle, COLORREF color);
		}

		namespace window
		{
			HRESULT set_window_theme(HWND window_handle, const wchar_t *app_name, const wchar_t *id_list);
			HRESULT set_window_theme_utf8(HWND window_handle, const char *app_name, const char *id_list);
		}

	}
}

#endif // MAMEUI_LIB_WINAPI_WINDOWS_CONTROLS_H
