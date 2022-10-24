// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

#ifndef MAMEUI_WINAPP_UI_OPTS_H
#define MAMEUI_WINAPP_UI_OPTS_H

#pragma once

namespace string_util = mameui::util::string_util;

#define MUIOPTION_OVERRIDE_REDX                  "override_redx"
#define MUIOPTION_LIST_MODE                      "list_mode"
#define MUIOPTION_CHECK_GAME                     "check_game"
#define MUIOPTION_JOYSTICK_IN_INTERFACE          "joystick_in_interface"
#define MUIOPTION_KEYBOARD_IN_INTERFACE          "keyboard_in_interface"
#define MUIOPTION_CYCLE_SCREENSHOT               "cycle_screenshot"
#define MUIOPTION_STRETCH_SCREENSHOT_LARGER      "stretch_screenshot_larger"
#define MUIOPTION_SCREENSHOT_BORDER_SIZE         "screenshot_bordersize"
#define MUIOPTION_SCREENSHOT_BORDER_COLOR        "screenshot_bordercolor"
#define MUIOPTION_INHERIT_FILTER                 "inherit_filter"
#define MUIOPTION_OFFSET_CLONES                  "offset_clones"
#define MUIOPTION_DEFAULT_FOLDER_ID              "default_folder_id"
#define MUIOPTION_HIDE_FOLDERS                   "hide_folders"
#define MUIOPTION_SHOW_STATUS_BAR                "show_status_bar"
#define MUIOPTION_SHOW_TABS                      "show_tabs"
#define MUIOPTION_SHOW_TOOLBAR                   "show_tool_bar"
#define MUIOPTION_CURRENT_TAB                    "current_tab"
#define MUIOPTION_WINDOW_X                       "window_x"
#define MUIOPTION_WINDOW_Y                       "window_y"
#define MUIOPTION_WINDOW_WIDTH                   "window_width"
#define MUIOPTION_WINDOW_HEIGHT                  "window_height"
#define MUIOPTION_WINDOW_STATE                   "window_state"
#define MUIOPTION_WINDOW_PANES                   "window_panes"
#define MUIOPTION_CUSTOM_COLOR                   "custom_color"
#define MUIOPTION_LIST_FONT                      "list_font"
#define MUIOPTION_TEXT_COLOR                     "text_color"
#define MUIOPTION_CLONE_COLOR                    "clone_color"
#define MUIOPTION_HIDE_TABS                      "hide_tabs"
#define MUIOPTION_HISTORY_TAB                    "history_tab"
#define MUIOPTION_COLUMN_WIDTHS                  "column_widths"
#define MUIOPTION_COLUMN_ORDER                   "column_order"
#define MUIOPTION_COLUMN_SHOWN                   "column_shown"
#define MUIOPTION_SPLITTERS                      "splitters"
#define MUIOPTION_SORT_COLUMN                    "sort_column"
#define MUIOPTION_SORT_REVERSED                  "sort_reversed"
#define MUIOPTION_BACKGROUND_DIRECTORY           "background_directory"
#define MUIOPTION_DATS_DIRECTORY                 "dats_directory"
#define MUIOPTION_VIDEO_DIRECTORY                "video_directory"
#define MUIOPTION_MANUALS_DIRECTORY              "manuals_directory"
#define MUIOPTION_UI_KEY_UP                      "ui_key_up"
#define MUIOPTION_UI_KEY_DOWN                    "ui_key_down"
#define MUIOPTION_UI_KEY_LEFT                    "ui_key_left"
#define MUIOPTION_UI_KEY_RIGHT                   "ui_key_right"
#define MUIOPTION_UI_KEY_START                   "ui_key_start"
#define MUIOPTION_UI_KEY_PGUP                    "ui_key_pgup"
#define MUIOPTION_UI_KEY_PGDWN                   "ui_key_pgdwn"
#define MUIOPTION_UI_KEY_HOME                    "ui_key_home"
#define MUIOPTION_UI_KEY_END                     "ui_key_end"
#define MUIOPTION_UI_KEY_SS_CHANGE               "ui_key_ss_change"
#define MUIOPTION_UI_KEY_HISTORY_UP              "ui_key_history_up"
#define MUIOPTION_UI_KEY_HISTORY_DOWN            "ui_key_history_down"
#define MUIOPTION_UI_KEY_CONTEXT_FILTERS         "ui_key_context_filters"
#define MUIOPTION_UI_KEY_SELECT_RANDOM           "ui_key_select_random"
#define MUIOPTION_UI_KEY_GAME_AUDIT              "ui_key_game_audit"
#define MUIOPTION_UI_KEY_GAME_PROPERTIES         "ui_key_game_properties"
#define MUIOPTION_UI_KEY_HELP_CONTENTS           "ui_key_help_contents"
#define MUIOPTION_UI_KEY_UPDATE_GAMELIST         "ui_key_update_gamelist"
#define MUIOPTION_UI_KEY_VIEW_FOLDERS            "ui_key_view_folders"
#define MUIOPTION_UI_KEY_VIEW_FULLSCREEN         "ui_key_view_fullscreen"
#define MUIOPTION_UI_KEY_VIEW_PAGETAB            "ui_key_view_pagetab"
#define MUIOPTION_UI_KEY_VIEW_PICTURE_AREA       "ui_key_view_picture_area"
#define MUIOPTION_UI_KEY_VIEW_STATUS             "ui_key_view_status"
#define MUIOPTION_UI_KEY_VIEW_TOOLBARS           "ui_key_view_toolbars"
#define MUIOPTION_UI_KEY_VIEW_TAB_CABINET        "ui_key_view_tab_cabinet"
#define MUIOPTION_UI_KEY_VIEW_TAB_CPANEL         "ui_key_view_tab_cpanel"
#define MUIOPTION_UI_KEY_VIEW_TAB_FLYER          "ui_key_view_tab_flyer"
#define MUIOPTION_UI_KEY_VIEW_TAB_HISTORY        "ui_key_view_tab_history"
#define MUIOPTION_UI_KEY_VIEW_TAB_MARQUEE        "ui_key_view_tab_marquee"
#define MUIOPTION_UI_KEY_VIEW_TAB_SCREENSHOT     "ui_key_view_tab_screenshot"
#define MUIOPTION_UI_KEY_VIEW_TAB_TITLE          "ui_key_view_tab_title"
#define MUIOPTION_UI_KEY_VIEW_TAB_PCB            "ui_key_view_tab_pcb"
#define MUIOPTION_UI_KEY_QUIT                    "ui_key_quit"
#define MUIOPTION_UI_JOY_UP                      "ui_joy_up"
#define MUIOPTION_UI_JOY_DOWN                    "ui_joy_down"
#define MUIOPTION_UI_JOY_LEFT                    "ui_joy_left"
#define MUIOPTION_UI_JOY_RIGHT                   "ui_joy_right"
#define MUIOPTION_UI_JOY_START                   "ui_joy_start"
#define MUIOPTION_UI_JOY_PGUP                    "ui_joy_pgup"
#define MUIOPTION_UI_JOY_PGDWN                   "ui_joy_pgdwn"
#define MUIOPTION_UI_JOY_HOME                    "ui_joy_home"
#define MUIOPTION_UI_JOY_END                     "ui_joy_end"
#define MUIOPTION_UI_JOY_SS_CHANGE               "ui_joy_ss_change"
#define MUIOPTION_UI_JOY_HISTORY_UP              "ui_joy_history_up"
#define MUIOPTION_UI_JOY_HISTORY_DOWN            "ui_joy_history_down"
#define MUIOPTION_UI_JOY_EXEC                    "ui_joy_exec"
#define MUIOPTION_EXEC_COMMAND                   "exec_command"
#define MUIOPTION_EXEC_WAIT                      "exec_wait"
#define MUIOPTION_HIDE_MOUSE                     "hide_mouse"
#define MUIOPTION_FULL_SCREEN                    "full_screen"
#define MUIOPTION_UI_KEY_VIEW_SOFTWARE_AREA      "ui_key_view_software_area"

#ifdef MESS
#define MUIOPTION_DEFAULT_GAME                   "default_system"
#define MUIDEFAULT_SELECTION                     "0"
#define MUIDEFAULT_SPLITTERS                     "133,1125,1706"
#else
#define MUIOPTION_DEFAULT_GAME                   "default_machine"
#define MUIDEFAULT_SELECTION                     "0"
#define MUIDEFAULT_SPLITTERS                     "164,1700"
#endif

#define MESSUI_SL_COLUMN_SHOWN        "sl_column_shown"
#define MESSUI_SL_COLUMN_WIDTHS       "sl_column_widths"
#define MESSUI_SL_COLUMN_ORDER        "sl_column_order"
#define MESSUI_SL_SORT_REVERSED       "sl_sort_reversed"
#define MESSUI_SL_SORT_COLUMN         "sl_sort_column"
#define MESSUI_SW_COLUMN_SHOWN        "sw_column_shown"
#define MESSUI_SW_COLUMN_WIDTHS       "sw_column_widths"
#define MESSUI_SW_COLUMN_ORDER        "sw_column_order"
#define MESSUI_SW_SORT_REVERSED       "sw_sort_reversed"
#define MESSUI_SW_SORT_COLUMN         "sw_sort_column"
#define MESSUI_SOFTWARE_TAB           "current_software_tab"

using winui_ui_options_entry = struct winui_ui_options_entry
{
	std::string option_name; // name of the option
	std::string option_value; // initial value if ini file not found
	const core_options::option_type unused1; // option type (unused)
	char *unused2; // help text (unused)
};

class winui_ui_options
{
public:
	// construction/destruction
	winui_ui_options()
	{
		// set up default values
		for (int i = 0; s_option_entries[i].option_name != "$end"; i++)
			m_list[s_option_entries[i].option_name] = s_option_entries[i].option_value;

//      constexpr size_t pad_length = 27;
//      std::cout << string_util::pad_to_center("START DUMP OF DEFAULT", pad_length, true) << "\n";

//      for (auto const &it : m_list)
//          std::cout << it.first << " = " << it.second << "\n";

//      std::cout << string_util::pad_to_center("END DUMP OF DEFAULT", pad_length, true) << "\n";
	}

	void load_file(std::string);
	void save_file(std::string);
	void reset_and_save(std::string);

	std::string getter(std::string) const;
	void setter(std::string, int);
	void setter(std::string, std::string);

	int int_value(std::string option_name) const
	{
		std::string option_value = getter(std::move(option_name));

		return (option_value.empty()) ? 0 : std::stoi(option_value.c_str());
	}

	bool bool_value(std::string option_name) const
	{
		return int_value(std::move(option_name)) ? true : false;
	}

private:
	// static list of ui option entries
	static const winui_ui_options_entry s_option_entries[];
	std::string m_filename;
	std::unordered_map<std::string, std::string> m_list;

	void create_index(std::ifstream& fp);

	bool ends_with_filters(std::string_view str) const;
};

#endif // MAMEUI_WINAPP_UI_OPTS_H
