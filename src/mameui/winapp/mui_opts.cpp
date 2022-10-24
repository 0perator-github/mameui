// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

 /***************************************************************************

  mui_opts.cpp

  Stores global options and per-game options;

***************************************************************************/

// standard C++ headers
#include <filesystem>
#include <fstream>      // for *_opts.h (below)
#include <iostream>
#include <iomanip>

// standard windows headers
#include "winapi_common.h"

// MAME headers

// shlwapi.h pulls in objbase.h which defines interface as a struct which conflict with a template declaration in device.h
#ifdef interface
#undef interface // undef interface which is for COM interfaces
#endif
#include "emu.h"
#define interface struct

#include "main.h"

#include "drivenum.h"

#include "ui/info.h"
#include "ui/moptions.h"
#include "winopts.h"

// MAMEUI headers
#include "mui_cstr.h"
#include "mui_stringtokenizer.h"
#include "mui_padstr.h"
#include "mui_wcstr.h"
#include "mui_wcstrconv.h"

#include "windows_messages.h"

#include "bitmask.h"
#include "emu_opts.h"
#include "game_opts.h"
#include "resource.h"
#include "ui_opts.h"
#include "treeview.h"
#include "splitters.h"
#include "screenshot.h"
#include "winui.h"

#include "mui_opts.h"

using namespace mameui::util::string_util;
using namespace mameui::winapi;
using namespace std::string_literals;

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

/***************************************************************************
    Internal function prototypes
 ***************************************************************************/

// static void LoadFolderFilter(int folder_index,int filters);

static std::string CusColorEncodeString(const COLORREF *value);
static void CusColorDecodeString(std::string_view ss, COLORREF *value);

static std::string SplitterEncodeString(const int *value);
static void SplitterDecodeString(std::string_view ss, int *value);

static std::string FontEncodeString(const LOGFONT *f);
static void FontDecodeString(std::string_view ss, LOGFONT *f);

static std::string TabFlagsEncodeString(int data);
static void TabFlagsDecodeString(std::string_view ss, int *data, int fallback_tab_index = TAB_SCREENSHOT);

static std::string ColumnEncodeStringWithCount(const int *value, const size_t count);
static void ColumnDecodeStringWithCount(std::string_view ss, int *value, const size_t count);



/***************************************************************************
    Internal defines
 ***************************************************************************/

const std::filesystem::path GAMEINFO_INI_FILENAME = "mameui_g.ini";


/***************************************************************************
    Internal structures
 ***************************************************************************/

using ImageTabName = struct image_tab_name
{
	std::wstring_view short_name;
	std::wstring_view long_name;
};

 /***************************************************************************
    Internal variables
 ***************************************************************************/

constexpr int ALL_TABS_ON = (1 << MAX_TAB_TYPES) - 1;
static winui_ui_options settings; // mameui.ini
static winui_game_options game_opts;    // game stats

// Screen shot Page tab control text
// these must match the order of the options flags in options.h
// (TAB_...)

static ImageTabName image_tabnames[MAX_TAB_TYPES] =
{
	{L"artpreview",L"Artwork"},
	{L"boss",L"Boss"},
	{L"cabinet", L"Cabinet" },
	{L"cpanel",L"Control Panel"},
	{L"cover",L"Cover"},
	{L"end",L"End"},
	{L"flyer",L"Flyer"},
	{L"gameover",L"Game Over"},
	{L"howto",L"How To"},
	{L"logo",L"Logo"},
	{L"marquee",L"Marquee"},
	{L"pcb",L"PCB"},
	{L"scores",L"Scores"},
	{L"select",L"Select"},
	{L"snap",L"Snapshot"},
	{L"title",L"Title"},
	{L"versus",L"Versus"},
	{L"history",L"History"}
};


/***************************************************************************
    External functions
 ***************************************************************************/
std::string GetGameName(int driver_index)
{
	if (driver_index < driver_list::total())
		return driver_list::driver(driver_index).name;
	else
		return "0";
}

void OptionsInit()
{
	// set up global options
	std::cout << "OptionsInit: About to load " << MUI_INI_FILENAME << "\n";
	settings.load_file(MUI_INI_FILENAME.string());                    // parse MAMEUI.ini
	std::cout << "OptionsInit: About to load " << GAMEINFO_INI_FILENAME << "\n";
	game_opts.load_file(GAMEINFO_INI_FILENAME.string());             // parse MAME_g.ini
	std::cout << "OptionsInit: Finished" << "\n";
	return;
}

// Restore ui settings to factory
void ResetGUI(void)
{
	settings.reset_and_save(MUI_INI_FILENAME.string());
}

std::wstring_view  GetImageTabLongName(int tab_index)
{
	return image_tabnames[tab_index].long_name;
}

std::wstring_view  GetImageTabShortName(int tab_index)
{
	return image_tabnames[tab_index].short_name;
}

//============================================================
//  OPTIONS WRAPPERS
//============================================================

static COLORREF options_get_color(std::string option_name)
{
	COLORREF color_value;
	std::string option_value = settings.getter(std::move(option_name));
	std::istringstream decode_stream(option_value.c_str());

	decode_stream >> std::hex >> color_value;

	return color_value;
}

static void options_set_color(std::string option_name, COLORREF value)
{
	std::ostringstream encoded_stream;

	encoded_stream.fill('0');
	encoded_stream << std::setw(6) << std::hex << (value | 0);

	settings.setter(std::move(option_name), encoded_stream.str());
}

static COLORREF options_get_color_default(std::string option_name, int default_color)
{
	COLORREF color_value = options_get_color(std::move(option_name));
	if (color_value == (COLORREF) -1)
		color_value = (COLORREF)windows::get_sys_color(default_color);

	return color_value;
}

static void options_set_color_default(std::string option_name, COLORREF value, int default_color)
{
	if (value == (COLORREF)windows::get_sys_color(default_color))
		options_set_color(std::move(option_name), (COLORREF) -1);
	else
		options_set_color(std::move(option_name), value);
}

static input_seq *options_get_input_seq(std::string option_name)
{
/*
    static input_seq seq;
    std::string val = settings.getter(std::move(option_name));
    input_seq_from_tokens(nullptr, seq_string.c_str(), &seq);  // HACK
    return &seq;*/
	return nullptr;
}



//============================================================
//  OPTIONS CALLS
//============================================================

// ***************************************************************** MAMEUI.INI settings **************************************************************************
void SetViewMode(int val)
{
	settings.setter(MUIOPTION_LIST_MODE, val);
}

int GetViewMode(void)
{
	return settings.int_value(MUIOPTION_LIST_MODE);
}

void SetGameCheck(bool game_check)
{
	settings.setter(MUIOPTION_CHECK_GAME, game_check);
}

bool GetGameCheck(void)
{
	return settings.bool_value(MUIOPTION_CHECK_GAME);
}

void SetJoyGUI(bool use_joygui)
{
	settings.setter(MUIOPTION_JOYSTICK_IN_INTERFACE, use_joygui);
}

bool GetJoyGUI(void)
{
	return settings.bool_value( MUIOPTION_JOYSTICK_IN_INTERFACE);
}

void SetKeyGUI(bool use_keygui)
{
	settings.setter(MUIOPTION_KEYBOARD_IN_INTERFACE, use_keygui);
}

bool GetKeyGUI(void)
{
	return settings.bool_value(MUIOPTION_KEYBOARD_IN_INTERFACE);
}

void SetCycleScreenshot(int cycle_screenshot)
{
	settings.setter(MUIOPTION_CYCLE_SCREENSHOT, cycle_screenshot);
}

int GetCycleScreenshot(void)
{
	return settings.int_value(MUIOPTION_CYCLE_SCREENSHOT);
}

void SetStretchScreenShotLarger(bool stretch)
{
	settings.setter(MUIOPTION_STRETCH_SCREENSHOT_LARGER, stretch);
}

bool GetStretchScreenShotLarger(void)
{
	return settings.bool_value(MUIOPTION_STRETCH_SCREENSHOT_LARGER);
}

void SetScreenshotBorderSize(int size)
{
	settings.setter(MUIOPTION_SCREENSHOT_BORDER_SIZE, size);
}

int GetScreenshotBorderSize(void)
{
	return settings.int_value(MUIOPTION_SCREENSHOT_BORDER_SIZE);
}

void SetScreenshotBorderColor(COLORREF uColor)
{
	options_set_color_default(MUIOPTION_SCREENSHOT_BORDER_COLOR, uColor, COLOR_3DFACE);
}

COLORREF GetScreenshotBorderColor(void)
{
	return options_get_color_default(MUIOPTION_SCREENSHOT_BORDER_COLOR, COLOR_3DFACE);
}

void SetFilterInherit(bool inherit)
{
	settings.setter(MUIOPTION_INHERIT_FILTER, inherit);
}

bool GetFilterInherit(void)
{
	return settings.bool_value( MUIOPTION_INHERIT_FILTER);
}

void SetOffsetClones(bool offset)
{
	settings.setter(MUIOPTION_OFFSET_CLONES, offset);
}

bool GetOffsetClones(void)
{
	return settings.bool_value( MUIOPTION_OFFSET_CLONES);
}

void SetSavedFolderID(UINT val)
{
	settings.setter(MUIOPTION_DEFAULT_FOLDER_ID, (int) val);
}

UINT GetSavedFolderID(void)
{
	return (UINT) settings.int_value(MUIOPTION_DEFAULT_FOLDER_ID);
}

void SetOverrideRedX(bool val)
{
	settings.setter(MUIOPTION_OVERRIDE_REDX, val);
}

bool GetOverrideRedX(void)
{
	return settings.bool_value(MUIOPTION_OVERRIDE_REDX);
}

static LPBITS GetShowFolderFlags(LPBITS bits)
{
	std::string show_folder_option;
	SetAllBits(bits, true);

	show_folder_option = settings.getter(MUIOPTION_HIDE_FOLDERS);
	if (!show_folder_option.empty())
	{
		extern const FOLDERDATA g_folderData[];
		stringtokenizer tokenizer(show_folder_option, ",");
		for (const auto &short_name : tokenizer)
		{
			for (size_t j = 0; !g_folderData[j].m_lpTitle.empty(); j++)
			{
				if (short_name == g_folderData[j].short_name)
				{
					ClearBit(bits, g_folderData[j].m_nFolderId);
					break;
				}
			}
		}
	}

	return bits;
}

bool GetShowFolder(int folder)
{
	LPBITS show_folder_flags = NewBits(MAX_FOLDERS);
	show_folder_flags = GetShowFolderFlags(show_folder_flags);
	bool result = TestBit(show_folder_flags, folder);
	DeleteBits(std::move(show_folder_flags));
	return result;
}

void SetShowFolder(int folder, bool show)
{
	LPBITS show_folder_flags = NewBits(MAX_FOLDERS);
	bool do_append = false;
	std::string str;
	extern const FOLDERDATA g_folderData[];

	show_folder_flags = GetShowFolderFlags(show_folder_flags);

	if (show)
		SetBit(show_folder_flags, folder);
	else
		ClearBit(show_folder_flags, folder);

	// we save the ones that are NOT displayed, so we can add new ones
	// and upgraders will see them
	for (size_t i=0; i < MAX_FOLDERS; i++)
	{
		if (TestBit(show_folder_flags, i) == false)
		{
			for (size_t j = 0; !g_folderData[j].m_lpTitle.empty(); j++)
			{
				if (g_folderData[j].m_nFolderId == i)
				{
					if (do_append)
						str.append(",");
					else
						do_append = true;

					str.append(g_folderData[j].short_name);
					break;
				}
			}
		}
	}

	settings.setter(MUIOPTION_HIDE_FOLDERS, std::move(str));
	DeleteBits(std::move(show_folder_flags));
}

void SetShowStatusBar(bool val)
{
	settings.setter(MUIOPTION_SHOW_STATUS_BAR, val);
}

bool GetShowStatusBar(void)
{
	return settings.bool_value( MUIOPTION_SHOW_STATUS_BAR);
}

void SetShowTabCtrl (bool val)
{
	settings.setter(MUIOPTION_SHOW_TABS, val);
}

bool GetShowTabCtrl (void)
{
	return settings.bool_value( MUIOPTION_SHOW_TABS);
}

void SetShowToolBar(bool val)
{
	settings.setter(MUIOPTION_SHOW_TOOLBAR, val);
}

bool GetShowToolBar(void)
{
	return settings.bool_value( MUIOPTION_SHOW_TOOLBAR);
}

void SetCurrentTab(int val)
{
	settings.setter(MUIOPTION_CURRENT_TAB, val);
}

int GetCurrentTab(void)
{
	return settings.int_value(MUIOPTION_CURRENT_TAB);
}

// Need int here in case no games were in the list at exit
void SetDefaultGame(int val)
{
	if ((val < 0) || (val > driver_list::total()))
		settings.setter(MUIOPTION_DEFAULT_GAME, "");
	else
		settings.setter(MUIOPTION_DEFAULT_GAME, driver_list::driver(val).name);
}

uint32_t GetDefaultGame(void)
{
	std::string option_value = settings.getter(MUIOPTION_DEFAULT_GAME);
	if (option_value.empty())
		return 0;
	int val = driver_list::find(option_value.c_str());
	if (val == -1)
		val = 0;
	return val;
}

void SetWindowArea(const AREA *area)
{
	settings.setter(MUIOPTION_WINDOW_X, area->x);
	settings.setter(MUIOPTION_WINDOW_Y, area->y);
	settings.setter(MUIOPTION_WINDOW_WIDTH, area->width);
	settings.setter(MUIOPTION_WINDOW_HEIGHT, area->height);
}

void GetWindowArea(AREA *area)
{
	area->x = settings.int_value(MUIOPTION_WINDOW_X);
	area->y = settings.int_value(MUIOPTION_WINDOW_Y);
	area->width  = settings.int_value(MUIOPTION_WINDOW_WIDTH);
	area->height = settings.int_value(MUIOPTION_WINDOW_HEIGHT);
}

void SetWindowState(uint32_t state)
{
	settings.setter(MUIOPTION_WINDOW_STATE, static_cast<int32_t>(state));
}

uint32_t GetWindowState(void)
{
	return settings.int_value(MUIOPTION_WINDOW_STATE);
}

void SetWindowPanes(uint32_t val)
{
	settings.setter(MUIOPTION_WINDOW_PANES, static_cast<int32_t>(val & 15u));
}

uint32_t GetWindowPanes(void)
{
	return static_cast<uint32_t>(settings.int_value(MUIOPTION_WINDOW_PANES)) & 15u;
}

void SetCustomColor(int iIndex, COLORREF uColor)
{
	if ((iIndex < 0) || (iIndex > 15))
		return;

	COLORREF custom_color[16];

	CusColorDecodeString(settings.getter(MUIOPTION_CUSTOM_COLOR), custom_color);
	custom_color[iIndex] = uColor;
	settings.setter(MUIOPTION_CUSTOM_COLOR, CusColorEncodeString(custom_color));
}

COLORREF GetCustomColor(int iIndex)
{
	if ((iIndex < 0) || (iIndex > 15))
		return RGB(0,0,0);

	COLORREF custom_color[16];

	CusColorDecodeString(settings.getter(MUIOPTION_CUSTOM_COLOR), custom_color);

	if (custom_color[iIndex] == (COLORREF)-1)
		custom_color[iIndex] = RGB(0, 0, 0);

	return custom_color[iIndex];
}

void SetListFont(const LOGFONT *font)
{
	settings.setter(MUIOPTION_LIST_FONT, FontEncodeString(font));
}

void GetListFont(LOGFONT *font)
{
	FontDecodeString(settings.getter(MUIOPTION_LIST_FONT), font);
}

void SetListFontColor(COLORREF uColor)
{
	options_set_color_default(MUIOPTION_TEXT_COLOR, uColor, COLOR_WINDOWTEXT);
}

COLORREF GetListFontColor(void)
{
	return options_get_color_default(MUIOPTION_TEXT_COLOR, COLOR_WINDOWTEXT);
}

void SetListCloneColor(COLORREF uColor)
{
	options_set_color_default(MUIOPTION_CLONE_COLOR, uColor, COLOR_WINDOWTEXT);
}

COLORREF GetListCloneColor(void)
{
	return options_get_color_default(MUIOPTION_CLONE_COLOR, COLOR_WINDOWTEXT);
}

int GetShowTab(int tab)
{
	int show_tab_flags = 0;
	TabFlagsDecodeString(settings.getter(MUIOPTION_HIDE_TABS), &show_tab_flags);
	return (show_tab_flags & (1 << tab)) != 0;
}

void SetShowTab(int tab,bool show)
{
	int show_tab_flags = 0;
	TabFlagsDecodeString(settings.getter(MUIOPTION_HIDE_TABS), &show_tab_flags);

	if (show)
		show_tab_flags |= 1 << tab;
	else
		show_tab_flags &= ~(1 << tab);

	settings.setter(MUIOPTION_HIDE_TABS, TabFlagsEncodeString(show_tab_flags));
}

// don't delete the last one
bool AllowedToSetShowTab(int tab,bool show)
{
	int show_tab_flags = 0;

	if (show == true)
		return true;

	TabFlagsDecodeString(settings.getter(MUIOPTION_HIDE_TABS), &show_tab_flags);

	show_tab_flags &= ~(1 << tab);
	return show_tab_flags != 0;
}

int GetHistoryTab(void)
{
	return settings.int_value(MUIOPTION_HISTORY_TAB);
}

void SetHistoryTab(int tab, bool show)
{
	if (show)
		settings.setter(MUIOPTION_HISTORY_TAB, tab);
	else
		settings.setter(MUIOPTION_HISTORY_TAB, TAB_NONE);
}

void SetColumnWidths(int width[])
{
	settings.setter(MUIOPTION_COLUMN_WIDTHS, ColumnEncodeStringWithCount(width, COLUMN_COUNT));
}

void GetColumnWidths(int width[])
{
	ColumnDecodeStringWithCount(settings.getter(MUIOPTION_COLUMN_WIDTHS), width, COLUMN_COUNT);
}

void SetSplitterPos(int splitterId, int pos)
{
	int *splitter;

	if (splitterId < GetSplitterCount())
	{
		splitter = (int *) alloca(GetSplitterCount() * sizeof(*splitter));
		SplitterDecodeString(settings.getter(MUIOPTION_SPLITTERS), splitter);
		splitter[splitterId] = pos;
		settings.setter(MUIOPTION_SPLITTERS, SplitterEncodeString(splitter));
	}
}

int GetSplitterPos(int splitterId)
{
	int *splitter;
	splitter = (int *) alloca(GetSplitterCount() * sizeof(*splitter));
	SplitterDecodeString(settings.getter(MUIOPTION_SPLITTERS), splitter);

	if (splitterId < GetSplitterCount())
		return splitter[splitterId];

	return -1; // Error
}

void SetColumnOrder(int order[])
{
	settings.setter(MUIOPTION_COLUMN_ORDER, ColumnEncodeStringWithCount(order, COLUMN_COUNT));
}

void GetColumnOrder(int order[])
{
	ColumnDecodeStringWithCount(settings.getter(MUIOPTION_COLUMN_ORDER), order, COLUMN_COUNT);
}

void SetColumnShown(int shown[])
{
	settings.setter(MUIOPTION_COLUMN_SHOWN, ColumnEncodeStringWithCount(shown, COLUMN_COUNT));
}

void GetColumnShown(int shown[])
{
	ColumnDecodeStringWithCount(settings.getter(MUIOPTION_COLUMN_SHOWN), shown, COLUMN_COUNT);
}

void SetSortColumn(int column)
{
	settings.setter(MUIOPTION_SORT_COLUMN, column);
}

int GetSortColumn(void)
{
	return settings.int_value(MUIOPTION_SORT_COLUMN);
}

void SetSortReverse(bool reverse)
{
	settings.setter(MUIOPTION_SORT_REVERSED, reverse);
}

bool GetSortReverse(void)
{
	return settings.bool_value( MUIOPTION_SORT_REVERSED);
}

std::string GetBgDir (void)
{
	std::string option_value = settings.getter(MUIOPTION_BACKGROUND_DIRECTORY);
	if (!std::filesystem::exists(option_value))
		option_value = "bkground\\bkground.png";

	return option_value;
}

void SetBgDir (std::string_view path)
{
	settings.setter(MUIOPTION_BACKGROUND_DIRECTORY, std::string(path));
}

std::string GetVideoDir(void)
{
	std::string option_value = settings.getter(MUIOPTION_VIDEO_DIRECTORY);
	if (option_value.empty())
		option_value = "video";

	return option_value;
}

void SetVideoDir(std::string_view path)
{
	settings.setter(MUIOPTION_VIDEO_DIRECTORY, std::string(path));
}

std::string GetManualsDir(void)
{
	std::string option_value = settings.getter(MUIOPTION_MANUALS_DIRECTORY);
	if (option_value.empty())
		option_value = "manuals";

	return option_value;
}

void SetManualsDir(std::string_view path)
{
	settings.setter(MUIOPTION_MANUALS_DIRECTORY, std::string(path));
}

// ***************************************************************** MAME_g.INI settings **************************************************************************
int GetRomAuditResults(int driver_index)
{
	return game_opts.rom(driver_index);
}

void SetRomAuditResults(int driver_index, int audit_results)
{
	game_opts.rom(driver_index, audit_results);
}

int GetSampleAuditResults(int driver_index)
{
	return game_opts.sample(driver_index);
}

void SetSampleAuditResults(int driver_index, int audit_results)
{
	game_opts.sample(driver_index, audit_results);
}

static void IncrementPlayVariable(int driver_index, std::string play_variable, uint32_t increment)
{
	if (!mui_strcmp(play_variable, "count"))
		game_opts.play_count(driver_index, game_opts.play_count(driver_index) + increment);
	else
	if (!mui_strcmp(play_variable, "time"))
		game_opts.play_time(driver_index, game_opts.play_time(driver_index) + increment);
}

void IncrementPlayCount(int driver_index)
{
	IncrementPlayVariable(driver_index, "count", 1);
}

uint32_t GetPlayCount(int driver_index)
{
	return game_opts.play_count(driver_index);
}

// int needed here so we can reset all games
static void ResetPlayVariable(int driver_index, std::string play_variable)
{
	if (driver_index < 0)
		// all games
		for (uint32_t i = 0; i < driver_list::total(); i++)
			ResetPlayVariable(i, play_variable);
	else
	{
		if (!mui_strcmp(play_variable, "count"))
			game_opts.play_count(driver_index, 0);
		else
		if (!mui_strcmp(play_variable, "time"))
			game_opts.play_time(driver_index, 0);
	}
}

// int needed here so we can reset all games
void ResetPlayCount(int driver_index)
{
	ResetPlayVariable(driver_index, "count");
}

// int needed here so we can reset all games
void ResetPlayTime(int driver_index)
{
	ResetPlayVariable(driver_index, "time");
}

uint32_t GetPlayTime(int driver_index)
{
	return game_opts.play_time(driver_index);
}

void IncrementPlayTime(int driver_index, uint32_t playtime)
{
	IncrementPlayVariable(driver_index, "time", playtime);
}

std::wstring GetTextPlayTime(int driver_index)
{
	constexpr uint32_t sec_per_day = 86400;
	constexpr uint32_t sec_per_hour = 3600;
	constexpr uint32_t sec_per_min = 60;

	std::wostringstream oss;

	if (driver_index < driver_list::total())
	{
		uint32_t seconds = GetPlayTime(driver_index);
		uint32_t days = seconds / sec_per_day;
		seconds %= sec_per_day;

		uint32_t hours = seconds / sec_per_hour;
		seconds %= sec_per_hour;

		uint32_t minutes = seconds / sec_per_min;
		seconds %= sec_per_min;

		if (days > 0)
		{
			oss << days << L"days ";
		}

		if (hours > 0)
		{
			oss << hours << L":" << std::setw(2) << std::setfill(L'0') << minutes << L":" << std::setw(2) << std::setfill(L'0') << seconds;
		}
		else if (minutes > 0)
		{
			oss << minutes << L":" << std::setw(2) << std::setfill(L'0') << seconds;
		}
		else
		{
			oss << seconds << L"s";
		}
	}

	return oss.str();
}

input_seq* Get_ui_key_up(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_UP);
}

input_seq* Get_ui_key_down(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_DOWN);
}

input_seq* Get_ui_key_left(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_LEFT);
}

input_seq* Get_ui_key_right(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_RIGHT);
}

input_seq* Get_ui_key_start(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_START);
}

input_seq* Get_ui_key_pgup(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_PGUP);
}

input_seq* Get_ui_key_pgdwn(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_PGDWN);
}

input_seq* Get_ui_key_home(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_HOME);
}

input_seq* Get_ui_key_end(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_END);
}

input_seq* Get_ui_key_ss_change(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_SS_CHANGE);
}

input_seq* Get_ui_key_history_up(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_HISTORY_UP);
}

input_seq* Get_ui_key_history_down(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_HISTORY_DOWN);
}

input_seq* Get_ui_key_context_filters(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_CONTEXT_FILTERS);
}

input_seq* Get_ui_key_select_random(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_SELECT_RANDOM);
}

input_seq* Get_ui_key_game_audit(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_GAME_AUDIT);
}

input_seq* Get_ui_key_game_properties(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_GAME_PROPERTIES);
}

input_seq* Get_ui_key_help_contents(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_HELP_CONTENTS);
}

input_seq* Get_ui_key_update_gamelist(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_UPDATE_GAMELIST);
}

input_seq* Get_ui_key_view_folders(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_VIEW_FOLDERS);
}

input_seq* Get_ui_key_view_fullscreen(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_VIEW_FULLSCREEN);
}

input_seq* Get_ui_key_view_pagetab(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_VIEW_PAGETAB);
}

input_seq* Get_ui_key_view_picture_area(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_VIEW_PICTURE_AREA);
}

input_seq* Get_ui_key_view_software_area(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_VIEW_SOFTWARE_AREA);
}

input_seq* Get_ui_key_view_status(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_VIEW_STATUS);
}

input_seq* Get_ui_key_view_toolbars(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_VIEW_TOOLBARS);
}

input_seq* Get_ui_key_view_tab_cabinet(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_VIEW_TAB_CABINET);
}

input_seq* Get_ui_key_view_tab_cpanel(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_VIEW_TAB_CPANEL);
}

input_seq* Get_ui_key_view_tab_flyer(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_VIEW_TAB_FLYER);
}

input_seq* Get_ui_key_view_tab_history(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_VIEW_TAB_HISTORY);
}

input_seq* Get_ui_key_view_tab_marquee(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_VIEW_TAB_MARQUEE);
}

input_seq* Get_ui_key_view_tab_screenshot(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_VIEW_TAB_SCREENSHOT);
}

input_seq* Get_ui_key_view_tab_title(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_VIEW_TAB_TITLE);
}

input_seq* Get_ui_key_view_tab_pcb(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_VIEW_TAB_PCB);
}

input_seq* Get_ui_key_quit(void)
{
	return options_get_input_seq(MUIOPTION_UI_KEY_QUIT);
}

static int GetUIJoy(std::string option_name, int joycodeIndex)
{
	int joycodes[4];

	if ((joycodeIndex < 0) || (joycodeIndex > 3))
		joycodeIndex = 0;
	ColumnDecodeStringWithCount(settings.getter(std::move(option_name)), joycodes, std::size(joycodes));
	return joycodes[joycodeIndex];
}

static void SetUIJoy(std::string option_name, int joycodeIndex, int val)
{
	int joycodes[4];

	if ((joycodeIndex < 0) || (joycodeIndex > 3))
		joycodeIndex = 0;
	ColumnDecodeStringWithCount(settings.getter(option_name), joycodes, std::size(joycodes));
	joycodes[joycodeIndex] = val;
	settings.setter(std::move(option_name), ColumnEncodeStringWithCount(joycodes, std::size(joycodes)));
}

int GetUIJoyUp(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_UP, joycodeIndex);
}

void SetUIJoyUp(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_UP, joycodeIndex, val);
}

int GetUIJoyDown(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_DOWN, joycodeIndex);
}

void SetUIJoyDown(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_DOWN, joycodeIndex, val);
}

int GetUIJoyLeft(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_LEFT, joycodeIndex);
}

void SetUIJoyLeft(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_LEFT, joycodeIndex, val);
}

int GetUIJoyRight(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_RIGHT, joycodeIndex);
}

void SetUIJoyRight(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_RIGHT, joycodeIndex, val);
}

int GetUIJoyStart(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_START, joycodeIndex);
}

void SetUIJoyStart(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_START, joycodeIndex, val);
}

int GetUIJoyPageUp(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_PGUP, joycodeIndex);
}

void SetUIJoyPageUp(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_PGUP, joycodeIndex, val);
}

int GetUIJoyPageDown(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_PGDWN, joycodeIndex);
}

void SetUIJoyPageDown(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_PGDWN, joycodeIndex, val);
}

int GetUIJoyHome(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_HOME, joycodeIndex);
}

void SetUIJoyHome(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_HOME, joycodeIndex, val);
}

int GetUIJoyEnd(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_END, joycodeIndex);
}

void SetUIJoyEnd(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_END, joycodeIndex, val);
}

int GetUIJoySSChange(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_SS_CHANGE, joycodeIndex);
}

void SetUIJoySSChange(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_SS_CHANGE, joycodeIndex, val);
}

int GetUIJoyHistoryUp(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_HISTORY_UP, joycodeIndex);
}

void SetUIJoyHistoryUp(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_HISTORY_UP, joycodeIndex, val);
}

int GetUIJoyHistoryDown(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_HISTORY_DOWN, joycodeIndex);
}

void SetUIJoyHistoryDown(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_HISTORY_DOWN, joycodeIndex, val);
}

// exec functions start: these are unsupported
void SetUIJoyExec(int joycodeIndex, int val)
{
	SetUIJoy(MUIOPTION_UI_JOY_EXEC, joycodeIndex, val);
}

int GetUIJoyExec(int joycodeIndex)
{
	return GetUIJoy(MUIOPTION_UI_JOY_EXEC, joycodeIndex);
}

std::string GetExecCommand(void)
{
	return settings.getter(MUIOPTION_EXEC_COMMAND);
}

// not used
void SetExecCommand(char *cmd)
{
	settings.setter(MUIOPTION_EXEC_COMMAND, cmd);
}

int GetExecWait(void)
{
	return settings.int_value(MUIOPTION_EXEC_WAIT);
}

void SetExecWait(int wait)
{
	settings.setter(MUIOPTION_EXEC_WAIT, wait);
}
// exec functions end

bool GetHideMouseOnStartup(void)
{
	return settings.bool_value(MUIOPTION_HIDE_MOUSE);
}

void SetHideMouseOnStartup(bool hide)
{
	settings.setter(MUIOPTION_HIDE_MOUSE, hide);
}

bool GetRunFullScreen(void)
{
	return settings.bool_value( MUIOPTION_FULL_SCREEN);
}

void SetRunFullScreen(bool fullScreen)
{
	settings.setter(MUIOPTION_FULL_SCREEN, fullScreen);
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

static std::string CusColorEncodeString(const COLORREF *value)
{
	std::ostringstream encoded_stream;

	encoded_stream.fill('0');
	encoded_stream << std::setw(6) << std::hex << (value[0] | 0);
	for (size_t i = 1; i < 16; i++)
		encoded_stream << ","s << std::setw(6) << std::hex << (value[i] | 0);

	return encoded_stream.str();
}

static void CusColorDecodeString(std::string_view ss, COLORREF *value)
{
	stringtokenizer tokenizer(ss, ",");
	auto token_iterator = tokenizer.begin();

	for (size_t i = 0; i < 16; ++i)
	{
		const std::string &token = token_iterator.advance_as_string();
		if (!token.empty())
		{
			std::istringstream decode_stream(token);
			decode_stream >> std::hex >> value[i];
		}
	}
}

static std::string ColumnEncodeStringWithCount(const int *value, const size_t count)
{
	if (count == 0 || value == nullptr)
		return std::string{};

	std::ostringstream encoded_stream;

	encoded_stream << value[0];
	for (size_t i = 1; i < count; ++i)
		encoded_stream << ',' << value[i];

	return encoded_stream.str();
}

static void ColumnDecodeStringWithCount(std::string_view ss, int *value, const size_t count)
{
	stringtokenizer tokenizer(ss, ",");
	auto token_iterator = tokenizer.begin();
	for (size_t i = 0; i < count; ++i)
	{
		auto next_int = token_iterator.advance_as_int();
		value[i] = next_int.value_or(0);
	}
}

static std::string SplitterEncodeString(const int *value)
{
	const size_t count = GetSplitterCount();
	if (count == 0 || value == nullptr)
		return std::string{};

	std::ostringstream encoded_stream;

	encoded_stream << value[0];
	for (size_t i = 1; i < GetSplitterCount(); i++)
		encoded_stream << ',' << value[i];

	return encoded_stream.str();
}

static void SplitterDecodeString(std::string_view ss, int *value)
{
	const size_t count = GetSplitterCount();

	stringtokenizer tokenizer(ss, ",");
	auto token_iterator = tokenizer.begin();

	for (size_t i = 0; i < count; ++i)
	{
		auto next_int = token_iterator.advance_as_int();
		value[i] = next_int.value_or(0);
	}
}

// Parse the given comma-delimited string into a LOGFONT structure
static void FontDecodeString(std::string_view ss, LOGFONTW* f)
{
	stringtokenizer tokenizer(ss, ",");
	auto token_iterator = tokenizer.begin();

	auto next_long = token_iterator.advance_as_long();
	f->lfHeight = next_long.value_or(0);

	next_long = token_iterator.advance_as_long();
	f->lfWidth = next_long.value_or(0);

	next_long = token_iterator.advance_as_long();
	f->lfEscapement = next_long.value_or(0);

	next_long = token_iterator.advance_as_long();
	f->lfOrientation = next_long.value_or(0);

	next_long = token_iterator.advance_as_long();
	f->lfWeight = next_long.value_or(0);

	auto next_int = token_iterator.advance_as_int();
	f->lfItalic = next_int.value_or(0);

	next_int = token_iterator.advance_as_int();
	f->lfUnderline = next_int.value_or(0);

	next_int = token_iterator.advance_as_int();
	f->lfStrikeOut = next_int.value_or(0);

	next_int = token_iterator.advance_as_int();
	f->lfCharSet = next_int.value_or(0);

	next_int = token_iterator.advance_as_int();
	f->lfOutPrecision = next_int.value_or(0);

	next_int = token_iterator.advance_as_int();
	f->lfClipPrecision = next_int.value_or(0);

	next_int = token_iterator.advance_as_int();
	f->lfQuality = next_int.value_or(0);

	next_int = token_iterator.advance_as_int();
	f->lfPitchAndFamily = next_int.value_or(0);

	const auto &face = *token_iterator;
	if (face.empty())
		return;

	std::wstring face_name = mui_utf16_from_utf8string(face);
	mui_wcscpy(f->lfFaceName, face_name.c_str());

}

// Encode the given LOGFONT structure into a comma-delimited string
static std::string FontEncodeString(const LOGFONTW* f)
{
	std::ostringstream oss;
	oss << f->lfHeight << ','
		<< f->lfWidth << ','
		<< f->lfEscapement << ','
		<< f->lfOrientation << ','
		<< f->lfWeight << ','
		<< static_cast<int>(f->lfItalic) << ','
		<< static_cast<int>(f->lfUnderline) << ','
		<< static_cast<int>(f->lfStrikeOut) << ','
		<< static_cast<int>(f->lfCharSet) << ','
		<< static_cast<int>(f->lfOutPrecision) << ','
		<< static_cast<int>(f->lfClipPrecision) << ','
		<< static_cast<int>(f->lfQuality) << ','
		<< static_cast<int>(f->lfPitchAndFamily) << ','
		<< mui_utf8_from_utf16string(f->lfFaceName);

	return oss.str();
}

static std::string TabFlagsEncodeString(int data)
{
	bool first_tab = true;
	std::wstring str;

	// we save the ones that are NOT displayed, so we can add new ones
	// and upgraders will see them
	for (int i = 0; i < MAX_TAB_TYPES; i++)
	{
		if ((data & (1 << i)) != 0)
			continue;

		if (!first_tab)
			str.append(L",");
		else
			first_tab = false;

		str.append(GetImageTabShortName(i));
	}

	return mui_utf8_from_utf16string(str);
}

static void TabFlagsDecodeString(std::string_view ss, int *data, int fallback_tab_index)
{
	std::wstring disabled_tabnames = mui_utf16_from_utf8string(ss);

	if (fallback_tab_index < 0 || fallback_tab_index >= MAX_TAB_TYPES)
	{
		// Invalid fallback index; default to 0
		fallback_tab_index = 0;
	}

	// Start with all tabs enabled
	*data = ALL_TABS_ON;

	wstringtokenizer tokenizer(disabled_tabnames, L",");
	for (const auto &disabled_tabname : tokenizer)
	{
		for (size_t i = 0; i < MAX_TAB_TYPES; ++i)
		{
			if (disabled_tabname == GetImageTabShortName(i))
			{
				*data &= ~(1 << i); // Disable matched tab
				break;
			}
		}
	}

	// not allowed to hide all tabs, because then why even show the area?
	if (*data == 0)
		*data = (1 << fallback_tab_index);
}

DWORD GetFolderFlags(int folder_index)
{
	LPTREEFOLDER lpFolder = GetFolder(folder_index);

	if (lpFolder)
		return lpFolder->m_dwFlags & F_MASK;

	return 0;
}

// MSH 20080813
// Read the folder filters from MAMEui.ini.  This must only
// be called AFTER the folders have all been created.

void LoadFolderFlags()
{
	const size_t numFolders = GetNumFolders();

	for (size_t i = 0; i < numFolders; ++i)
	{
		LPTREEFOLDER lpFolder = GetFolder(i);
		if (!lpFolder)
			continue;

		// Make a copy of the folder title and sanitize it
		std::string option_name = lpFolder->m_lpTitle;

		std::transform(option_name.begin(), option_name.end(), option_name.begin(), [](char c) { return (c == ' ' || c == '-') ? '_' : c; });

		option_name += "_filters";

		// Apply the flags
		lpFolder->m_dwFlags |= (settings.int_value(option_name.c_str()) & F_MASK);
	}
}

// Adds our folder flags to winui_options, for saving.
static void AddFolderFlags()
{
	size_t numFolders = GetNumFolders();

	for (size_t i = 0; i < numFolders; ++i)
	{
		LPTREEFOLDER lpFolder = GetFolder(i);
		if (!lpFolder)
			continue;

		// Convert title to a sanitized name
		std::string folderName = lpFolder->m_lpTitle;
		for (char& ch : folderName)
		{
			if (ch == ' ' || ch == '-')
				ch = '_';
		}

		std::string optionName = folderName + "_filters";

		// Apply mask and store the flag
		settings.setter(optionName.c_str(), lpFolder->m_dwFlags & F_MASK);
	}
}

// Save MAMEUI.ini
void mui_save_ini(void)
{
	// Add the folder flag to settings.
	AddFolderFlags();
	std::string mui_inipath = MUI_INI_FILENAME.string();
	settings.save_file(std::move(mui_inipath));
}

void SaveGameListOptions(void)
{
	// Save GameInfo.ini - game options.
	std::string gameinfo_inipath = GAMEINFO_INI_FILENAME.string();
	game_opts.save_file(std::move(gameinfo_inipath));
}

std::string  GetVersionString(void)
{
	return emulator_info::get_build_version();
}

uint64_t GetDriverCacheLower(int driver_index)
{
	return game_opts.cache_lower(driver_index);
}

uint32_t GetDriverCacheUpper(int driver_index)
{
	return game_opts.cache_upper(driver_index);
}

void SetDriverCache(int driver_index, uint32_t val)
{
	game_opts.cache_upper(driver_index, val);
}

bool RequiredDriverCache(void)
{
	return game_opts.rebuild();
}

void ForceRebuild(void)
{
	game_opts.force_rebuild();
}

bool DriverIsComputer(int driver_index)
{
	assert(driver_index < driver_list::total());
	uint64_t system_type = GetDriverCacheLower(driver_index) & lower_cache::MASK_SYSTEMTYPE;
	return system_type == lower_cache::SYSTEMTYPE_COMPUTER;
}

bool DriverIsConsole(int driver_index)
{
	assert(driver_index < driver_list::total());
	uint64_t system_type = GetDriverCacheLower(driver_index) & lower_cache::MASK_SYSTEMTYPE;
	return system_type == lower_cache::SYSTEMTYPE_CONSOLE;
}

bool DriverIsModified(int driver_index)
{
	assert(driver_index < driver_list::total());
	return is_flag_set(game_opts.cache_lower(driver_index), lower_cache::UNOFFICIAL); // UNOFFICIAL
}

bool DriverIsImperfect(int driver_index)
{
	assert(driver_index < driver_list::total());
	constexpr uint64_t ImperfectOrMissingFlags = 0x00003FC800000000ULL;  // (WRONG_COLORS | NO_SOUND | NO_GRAPHICS | UNEMULATED_PROTECTION | IMPERFECT_COLOR | IMPERFECT_GRAPHICS | IMPERFECT_SOUND | IMPERFECT_CONTROLS | IMPERFECT_TIMING)
	return (game_opts.cache_lower(driver_index) & ImperfectOrMissingFlags) != 0;
}

// from optionsms.cpp (MESSUI)


#define LOG_SOFTWARE 1

void SetSLColumnOrder(int order[])
{
	settings.setter(MESSUI_SL_COLUMN_ORDER, ColumnEncodeStringWithCount(order, SL_COLUMN_COUNT));
}

void GetSLColumnOrder(int order[])
{
	ColumnDecodeStringWithCount(settings.getter(MESSUI_SL_COLUMN_ORDER), order, SL_COLUMN_COUNT);
}

void SetSLColumnShown(int shown[])
{
	settings.setter(MESSUI_SL_COLUMN_SHOWN, ColumnEncodeStringWithCount(shown, SL_COLUMN_COUNT));
}

void GetSLColumnShown(int shown[])
{
	ColumnDecodeStringWithCount(settings.getter(MESSUI_SL_COLUMN_SHOWN), shown, SL_COLUMN_COUNT);
}

void SetSLColumnWidths(int width[])
{
	settings.setter(MESSUI_SL_COLUMN_WIDTHS, ColumnEncodeStringWithCount(width, SL_COLUMN_COUNT));
}

void GetSLColumnWidths(int width[])
{
	ColumnDecodeStringWithCount(settings.getter(MESSUI_SL_COLUMN_WIDTHS), width, SL_COLUMN_COUNT);
}

void SetSLSortColumn(int column)
{
	settings.setter(MESSUI_SL_SORT_COLUMN, column);
}

int GetSLSortColumn(void)
{
	return settings.int_value(MESSUI_SL_SORT_COLUMN);
}

void SetSLSortReverse(bool reverse)
{
	settings.setter(MESSUI_SL_SORT_REVERSED, reverse);
}

bool GetSLSortReverse(void)
{
	return settings.bool_value(MESSUI_SL_SORT_REVERSED);
}

void SetSWColumnOrder(int order[])
{
	settings.setter(MESSUI_SW_COLUMN_ORDER, ColumnEncodeStringWithCount(order, SW_COLUMN_COUNT));
}

void GetSWColumnOrder(int order[])
{
	ColumnDecodeStringWithCount(settings.getter(MESSUI_SW_COLUMN_ORDER), order, SW_COLUMN_COUNT);
}

void SetSWColumnShown(int shown[])
{
	settings.setter(MESSUI_SW_COLUMN_SHOWN, ColumnEncodeStringWithCount(shown, SW_COLUMN_COUNT));
}

void GetSWColumnShown(int shown[])
{
	ColumnDecodeStringWithCount(settings.getter(MESSUI_SW_COLUMN_SHOWN), shown, SW_COLUMN_COUNT);
}

void SetSWColumnWidths(int width[])
{
	settings.setter(MESSUI_SW_COLUMN_WIDTHS, ColumnEncodeStringWithCount(width, SW_COLUMN_COUNT));
}

void GetSWColumnWidths(int width[])
{
	ColumnDecodeStringWithCount(settings.getter(MESSUI_SW_COLUMN_WIDTHS), width, SW_COLUMN_COUNT);
}

void SetSWSortColumn(int column)
{
	settings.setter(MESSUI_SW_SORT_COLUMN, column);
}

int GetSWSortColumn(void)
{
	return settings.int_value(MESSUI_SW_SORT_COLUMN);
}

void SetSWSortReverse(bool reverse)
{
	settings.setter( MESSUI_SW_SORT_REVERSED, reverse);
}

bool GetSWSortReverse(void)
{
	return settings.bool_value(MESSUI_SW_SORT_REVERSED);
}


void SetCurrentSoftwareTab(int val)
{
	settings.setter(MESSUI_SOFTWARE_TAB, val);
}

int GetCurrentSoftwareTab(void)
{
	return settings.int_value(MESSUI_SOFTWARE_TAB);
}

