// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

 /***************************************************************************

  ui_opts.cpp

  Stores ui options;

***************************************************************************/

// standard C++ headers
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

// standard windows headers

// MAME headers

// shlwapi.h pulls in objbase.h which defines interface as a struct which conflict with a template declaration in device.h
#ifdef interface
#undef interface // undef interface which is for COM interfaces
#endif
#include "emu.h"
#define interface struct

#include "ui/moptions.h"
#include "winopts.h"

// MAMEUI headers
#include "mui_cstr.h"
#include "mui_padstr.h"
#include "mui_stringtokenizer.h"

#include "ui_opts.h"

using namespace mameui::util::string_util;
using namespace std::literals;

// UI options in MAMEui.ini
const winui_ui_options_entry winui_ui_options::s_option_entries[] =
{
	{ MUIOPTION_OVERRIDE_REDX,                "0",        core_options::option_type::INTEGER,                 nullptr },
	{ MUIOPTION_DEFAULT_GAME,                 MUIDEFAULT_SELECTION, core_options::option_type::INTEGER,       nullptr },
	{ MUIOPTION_DEFAULT_FOLDER_ID,            "0",        core_options::option_type::INTEGER,                 nullptr },
	{ MUIOPTION_FULL_SCREEN,                  "0",        core_options::option_type::BOOLEAN,                 nullptr },
	{ MUIOPTION_CURRENT_TAB,                  "0",        core_options::option_type::STRING,                  nullptr },
	{ MESSUI_SOFTWARE_TAB,                    "0",        core_options::option_type::INTEGER,                 nullptr },
	{ MUIOPTION_SHOW_TOOLBAR,                 "1",        core_options::option_type::BOOLEAN,                 nullptr },
	{ MUIOPTION_SHOW_STATUS_BAR,              "1",        core_options::option_type::BOOLEAN,                 nullptr },
	{ MUIOPTION_HIDE_FOLDERS,                 "",         core_options::option_type::STRING,                  nullptr },
	{ MUIOPTION_SHOW_TABS,                    "1",        core_options::option_type::BOOLEAN,                 nullptr },
	{ MUIOPTION_HIDE_TABS,                    "artpreview,boss,cpanel,cover,end,flyer,gameover,howto,logo,marquee,pcb,scores,select,title,versus",  core_options::option_type::STRING, nullptr },
	{ MUIOPTION_HISTORY_TAB,                  "0",        core_options::option_type::INTEGER,                 nullptr },
	{ MUIOPTION_SORT_COLUMN,                  "0",        core_options::option_type::INTEGER,                 nullptr },
	{ MUIOPTION_SORT_REVERSED,                "0",        core_options::option_type::BOOLEAN,                 nullptr },
	{ MUIOPTION_WINDOW_X,                     "0",        core_options::option_type::INTEGER,                 nullptr },  // main window position, left
	{ MUIOPTION_WINDOW_Y,                     "0",        core_options::option_type::INTEGER,                 nullptr },  // main window position, top
	{ MUIOPTION_WINDOW_WIDTH,                 "2000",     core_options::option_type::INTEGER,                 nullptr },  // main window width
	{ MUIOPTION_WINDOW_HEIGHT,                "1000",     core_options::option_type::INTEGER,                 nullptr },  // main window height
	{ MUIOPTION_WINDOW_STATE,                 "1",        core_options::option_type::INTEGER,                 nullptr },
	{ MUIOPTION_WINDOW_PANES,                 "15",       core_options::option_type::INTEGER,                 nullptr },  // which windows are visible: bit 0 = tree, bit 1 = list, bit 2 = sw, bit 3 = images
	{ MUIOPTION_TEXT_COLOR,                   "-1",       core_options::option_type::INTEGER,                 nullptr },
	{ MUIOPTION_CLONE_COLOR,                  "-1",       core_options::option_type::INTEGER,                 nullptr },
	{ MUIOPTION_CUSTOM_COLOR,                 "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", core_options::option_type::STRING, nullptr }, // colour codes of the 16 custom colours (set in clone font dialog)
	/* ListMode needs to be before ColumnWidths settings */
	{ MUIOPTION_LIST_MODE,                    "5",       core_options::option_type::INTEGER,                 nullptr },
	{ MUIOPTION_SPLITTERS,                    MUIDEFAULT_SPLITTERS, core_options::option_type::STRING,       nullptr },
	{ MUIOPTION_LIST_FONT,                    "-11,0,0,0,400,0,0,0,0,1,2,1,34,MS Sans Serif", core_options::option_type::STRING, nullptr },
	{ MUIOPTION_COLUMN_WIDTHS,                "185,78,84,84,64,88,74,108,60,144,84,40,40", core_options::option_type::STRING, nullptr },
	{ MUIOPTION_COLUMN_ORDER,                 "0,1,2,3,4,5,6,7,8,9,10,11,12", core_options::option_type::STRING, nullptr },
	{ MUIOPTION_COLUMN_SHOWN,                 "1,1,1,1,1,1,1,1,1,1,1,1,0", core_options::option_type::STRING,  nullptr },
	{ MESSUI_SL_COLUMN_WIDTHS,                "100,75,223,46,120,120", core_options::option_type::STRING, nullptr },
	{ MESSUI_SL_COLUMN_ORDER,                 "0,1,2,3,4,5", core_options::option_type::STRING, nullptr }, // order of columns
	{ MESSUI_SL_COLUMN_SHOWN,                 "1,1,1,1,1,1", core_options::option_type::STRING, nullptr }, // 0=hide,1=show
	{ MESSUI_SL_SORT_COLUMN,                  "0", core_options::option_type::INTEGER, nullptr },
	{ MESSUI_SL_SORT_REVERSED,                "0", core_options::option_type::BOOLEAN, nullptr },
	{ MESSUI_SW_COLUMN_WIDTHS,                "400", core_options::option_type::STRING, nullptr },
	{ MESSUI_SW_COLUMN_ORDER,                 "0", core_options::option_type::STRING, nullptr }, // 1= dummy column
	{ MESSUI_SW_COLUMN_SHOWN,                 "1", core_options::option_type::STRING, nullptr }, // 0=don't show it
	{ MESSUI_SW_SORT_COLUMN,                  "0", core_options::option_type::INTEGER, nullptr },
	{ MESSUI_SW_SORT_REVERSED,                "0", core_options::option_type::BOOLEAN, nullptr },
	{ MUIOPTION_CHECK_GAME,                   "0",        core_options::option_type::BOOLEAN,    nullptr },
	{ MUIOPTION_JOYSTICK_IN_INTERFACE,        "1",        core_options::option_type::BOOLEAN,    nullptr },
	{ MUIOPTION_KEYBOARD_IN_INTERFACE,        "0",        core_options::option_type::BOOLEAN,    nullptr },
	{ MUIOPTION_HIDE_MOUSE,                   "0",        core_options::option_type::BOOLEAN,    nullptr },
	{ MUIOPTION_INHERIT_FILTER,               "0",        core_options::option_type::BOOLEAN,    nullptr },
	{ MUIOPTION_OFFSET_CLONES,                "0",        core_options::option_type::BOOLEAN,    nullptr },
	{ MUIOPTION_STRETCH_SCREENSHOT_LARGER,    "0",        core_options::option_type::BOOLEAN,    nullptr },
	{ MUIOPTION_CYCLE_SCREENSHOT,             "0",        core_options::option_type::INTEGER,                 nullptr },
	{ MUIOPTION_SCREENSHOT_BORDER_SIZE,       "11",       core_options::option_type::INTEGER,                 nullptr },
	{ MUIOPTION_SCREENSHOT_BORDER_COLOR,      "-1",       core_options::option_type::INTEGER,                 nullptr },
	{ MUIOPTION_EXEC_COMMAND,                 "",         core_options::option_type::STRING,                 nullptr },
	{ MUIOPTION_EXEC_WAIT,                    "0",        core_options::option_type::INTEGER,                 nullptr },
	{ MUIOPTION_BACKGROUND_DIRECTORY,         "bkground\\bkground.png", core_options::option_type::STRING,                 nullptr },
	{ MUIOPTION_DATS_DIRECTORY,               "dats",     core_options::option_type::STRING,                 nullptr },
	{ MUIOPTION_VIDEO_DIRECTORY,              "video",     core_options::option_type::STRING,                 nullptr },
	{ MUIOPTION_MANUALS_DIRECTORY,            "manuals",     core_options::option_type::STRING,                 nullptr },
	{ MUIOPTION_UI_KEY_UP,                    "KEYCODE_UP",                        core_options::option_type::STRING,          nullptr },
	{ MUIOPTION_UI_KEY_DOWN,                  "KEYCODE_DOWN",                     core_options::option_type::STRING,          nullptr },
	{ MUIOPTION_UI_KEY_LEFT,                  "KEYCODE_LEFT",                     core_options::option_type::STRING,          nullptr },
	{ MUIOPTION_UI_KEY_RIGHT,                 "KEYCODE_RIGHT",                    core_options::option_type::STRING,          nullptr },
	{ MUIOPTION_UI_KEY_START,                 "KEYCODE_ENTER NOT KEYCODE_LALT",    core_options::option_type::STRING,            nullptr },
	{ MUIOPTION_UI_KEY_PGUP,                  "KEYCODE_PGUP",                     core_options::option_type::STRING,          nullptr },
	{ MUIOPTION_UI_KEY_PGDWN,                 "KEYCODE_PGDN",                     core_options::option_type::STRING,          nullptr },
	{ MUIOPTION_UI_KEY_HOME,                  "KEYCODE_HOME",                     core_options::option_type::STRING,          nullptr },
	{ MUIOPTION_UI_KEY_END,                   "KEYCODE_END",                        core_options::option_type::STRING,          nullptr },
	{ MUIOPTION_UI_KEY_SS_CHANGE,             "KEYCODE_INSERT",                    core_options::option_type::STRING,          nullptr },
	{ MUIOPTION_UI_KEY_HISTORY_UP,            "KEYCODE_DEL",                        core_options::option_type::STRING,          nullptr },
	{ MUIOPTION_UI_KEY_HISTORY_DOWN,          "KEYCODE_LALT KEYCODE_0",            core_options::option_type::STRING,          nullptr },
	{ MUIOPTION_UI_KEY_CONTEXT_FILTERS,       "KEYCODE_LCONTROL KEYCODE_F", core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_KEY_SELECT_RANDOM,         "KEYCODE_LCONTROL KEYCODE_R", core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_KEY_GAME_AUDIT,            "KEYCODE_LALT KEYCODE_A",     core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_KEY_GAME_PROPERTIES,       "KEYCODE_LALT KEYCODE_ENTER", core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_KEY_HELP_CONTENTS,         "KEYCODE_F1",                 core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_KEY_UPDATE_GAMELIST,       "KEYCODE_F5",                 core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_KEY_VIEW_FOLDERS,          "KEYCODE_LALT KEYCODE_D",     core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_KEY_VIEW_FULLSCREEN,       "KEYCODE_F11",                core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_KEY_VIEW_PAGETAB,          "KEYCODE_LALT KEYCODE_B",     core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_KEY_VIEW_PICTURE_AREA,     "KEYCODE_LALT KEYCODE_P",     core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_KEY_VIEW_SOFTWARE_AREA,    "KEYCODE_LALT KEYCODE_W",     core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_KEY_VIEW_STATUS,           "KEYCODE_LALT KEYCODE_S",     core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_KEY_VIEW_TOOLBARS,         "KEYCODE_LALT KEYCODE_T",     core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_KEY_VIEW_TAB_CABINET,      "KEYCODE_LALT KEYCODE_3",     core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_KEY_VIEW_TAB_CPANEL,       "KEYCODE_LALT KEYCODE_6",     core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_KEY_VIEW_TAB_FLYER,        "KEYCODE_LALT KEYCODE_2",     core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_KEY_VIEW_TAB_HISTORY,      "KEYCODE_LALT KEYCODE_8",     core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_KEY_VIEW_TAB_MARQUEE,      "KEYCODE_LALT KEYCODE_4",     core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_KEY_VIEW_TAB_SCREENSHOT,   "KEYCODE_LALT KEYCODE_1",     core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_KEY_VIEW_TAB_TITLE,        "KEYCODE_LALT KEYCODE_5",     core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_KEY_VIEW_TAB_PCB,          "KEYCODE_LALT KEYCODE_7",     core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_KEY_QUIT,                  "KEYCODE_LALT KEYCODE_Q",     core_options::option_type::STRING, nullptr },
	{ MUIOPTION_UI_JOY_UP,                    "1,1,1,1",  core_options::option_type::STRING,                 nullptr },
	{ MUIOPTION_UI_JOY_DOWN,                  "1,1,1,2",  core_options::option_type::STRING,                 nullptr },
	{ MUIOPTION_UI_JOY_LEFT,                  "1,1,2,1",  core_options::option_type::STRING,                 nullptr },
	{ MUIOPTION_UI_JOY_RIGHT,                 "1,1,2,2",  core_options::option_type::STRING,                 nullptr },
	{ MUIOPTION_UI_JOY_START,                 "1,0,1,0",  core_options::option_type::STRING,                 nullptr },
	{ MUIOPTION_UI_JOY_PGUP,                  "2,1,2,1",  core_options::option_type::STRING,                 nullptr },
	{ MUIOPTION_UI_JOY_PGDWN,                 "2,1,2,2",  core_options::option_type::STRING,                 nullptr },
	{ MUIOPTION_UI_JOY_HOME,                  "0,0,0,0",  core_options::option_type::STRING,                 nullptr },
	{ MUIOPTION_UI_JOY_END,                   "0,0,0,0",  core_options::option_type::STRING,                 nullptr },
	{ MUIOPTION_UI_JOY_SS_CHANGE,             "2,0,3,0",  core_options::option_type::STRING,                 nullptr },
	{ MUIOPTION_UI_JOY_HISTORY_UP,            "2,0,4,0",  core_options::option_type::STRING,                 nullptr },
	{ MUIOPTION_UI_JOY_HISTORY_DOWN,          "2,0,1,0",  core_options::option_type::STRING,                 nullptr },
	{ MUIOPTION_UI_JOY_EXEC,                  "0,0,0,0",  core_options::option_type::STRING,                 nullptr },
	{ "$end" }
};

void winui_ui_options::create_index(std::ifstream &fp)
{
	bool is_ready = false;
	std::string file_line;

	if (!fp)
		return;

	m_list.reserve(107);

	while (std::getline(fp, file_line))
	{
		if (!is_ready)
		{
			if (file_line == "$start")
				is_ready = true;
			continue;
		}

		stringtokenizer tokenizer(file_line, "\t");

		auto token_iterator = tokenizer.begin();
		if (!token_iterator.peek())
			continue;

		const std::string &option_name = token_iterator.advance_as_string();

		if (!option_name.empty())
		{
			const std::string &option_data = token_iterator.advance_as_string();
			m_list[option_name] = option_data;
		}
	}

	fp.close();
}

bool winui_ui_options::ends_with_filters(std::string_view option_name) const
{
	if (option_name.empty())
		return false;

	constexpr std::string_view ending_part{ "_filters" };

	if (option_name.size() < ending_part.size())
		return false;

	return (option_name.compare(option_name.size() - ending_part.size(), ending_part.size(), ending_part) == 0);
}

std::string winui_ui_options::getter(std::string option_name) const
{
	auto found = m_list.find(option_name);
	return (found != m_list.end()) ? found->second : "";
}

void winui_ui_options::load_file(std::string filename)
{
	if (filename.empty() || m_filename == filename)
		return;

	m_filename = filename;

	std::ifstream infile(filename);
	create_index(infile);
}

void winui_ui_options::reset_and_save(std::string filename)
{
	// set up default values
	for (int i = 0; s_option_entries[i].option_name != "$end"; i++)
		m_list[s_option_entries[i].option_name] = s_option_entries[i].option_value;

	save_file(std::move(filename));
}

void winui_ui_options::save_file(std::string filename)
{
	if (filename.empty())
		return;

	std::string inistring = "YOU CAN SAFELY DELETE THIS FILE TO RESET THE EMULATOR BACK TO DEFAULTS.\n\n$start\n";

	for (const auto& it : m_list)
		inistring += it.first + "\t" + it.second + "\n";

	std::ofstream outfile(filename, std::ios::out | std::ios::trunc);
	if (!outfile.is_open())
		return;

	outfile.write(inistring.c_str(), inistring.size());
}

void winui_ui_options::setter(std::string option_name, int value)
{
	// filters: only want an entry if a filter is applied
	if (ends_with_filters(option_name) && (value == 0))
	{
		auto found_entry = m_list.find(option_name);
		if (found_entry != m_list.end())
			m_list.erase(found_entry); // delete
		else
			return;
	}

	m_list[option_name] = std::to_string(value); // add or update
	save_file(m_filename);
}

void winui_ui_options::setter(std::string option_name, std::string value)
{
	m_list[option_name] = std::move(value);
	save_file(m_filename);
}
