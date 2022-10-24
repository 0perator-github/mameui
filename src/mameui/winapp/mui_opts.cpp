// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

 /***************************************************************************

  mui_opts.cpp

  Stores global options and per-game options;

***************************************************************************/

// standard C++ headers
#include <fstream>      // for *_opts.h (below)
#include <iostream>
#include <iomanip>
#include <string_view>

// standard windows headers
#include "windows.h"
#include "commctrl.h"
#include "commdlg.h"

// MAME headers
#include "mameheaders.h"

// MAMEUI headers
#include "mui_wcs.h"
#include "mui_wcsconv.h"

#include "winapi_windows.h"

#include "bitmask.h"
#include "emu_opts.h"
#include "game_opts.h"
#include "resource.h"
#include "screenshot.h"
#include "splitters.h"
#include "treeview.h"
#include "ui_opts.h"
#include "winui.h"

#include "mui_opts.h"

using namespace std::literals;
using namespace mameui::winapi;

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
static void TabFlagsDecodeString(std::string_view ss, int *data);

static std::string ColumnEncodeStringWithCount(const int *value, const size_t count);
static void ColumnDecodeStringWithCount(std::string_view ss, int *value, const size_t count);



/***************************************************************************
    Internal defines
 ***************************************************************************/

constexpr std::string_view GAMEINFO_INI_FILENAME = "mameui_g.ini";


/***************************************************************************
    Internal structures
 ***************************************************************************/

 /***************************************************************************
    Internal variables
 ***************************************************************************/

static winui_ui_options settings; // mameui.ini
static winui_game_options game_opts;    // game stats


// Screen shot Page tab control text
// these must match the order of the options flags in options.h
// (TAB_...)
static const char *const image_tabs_long_name[MAX_TAB_TYPES] =
{
	"Artwork",
	"Boss",
	"Cabinet",
	"Control Panel",
	"Cover",
	"End",
	"Flyer",
	"Game Over",
	"How To",
	"Logo",
	"Marquee",
	"PCB",
	"Scores",
	"Select",
	"Snapshot",
	"Title",
	"Versus",
	"History"
};

static const char *const image_tabs_short_name[MAX_TAB_TYPES] =
{
	"artpreview",
	"boss",
	"cabinet",
	"cpanel",
	"cover",
	"end",
	"flyer",
	"gameover",
	"howto",
	"logo",
	"marquee",
	"pcb",
	"scores",
	"select",
	"snap",
	"title",
	"versus",
	"history"
};


/***************************************************************************
    External functions
 ***************************************************************************/
std::string GetGameName(uint32_t driver_index)
{
	if (driver_index < driver_list::total())
		return driver_list::driver(driver_index).name;
	else
		return "0";
}

void OptionsInit()
{
	// set up global options
	std::cout << "OptionsInit: About to load " << MUI_INI_FILENAME << std::endl;
	settings.load_file(&MUI_INI_FILENAME[0]);                    // parse MAMEUI.ini
	std::cout << "OptionsInit: About to load " << GAMEINFO_INI_FILENAME << std::endl;
	game_opts.load_file(&GAMEINFO_INI_FILENAME[0]);             // parse MAME_g.ini
	std::cout << "OptionsInit: Finished" << std::endl;
	return;
}

// Restore ui settings to factory
void ResetGUI(void)
{
	settings.reset_and_save(&MUI_INI_FILENAME[0]);
}

const char * GetImageTabLongName(int tab_index)
{
	return image_tabs_long_name[tab_index];
}

const char * GetImageTabShortName(int tab_index)
{
	return image_tabs_short_name[tab_index];
}

//============================================================
//  OPTIONS WRAPPERS
//============================================================

static COLORREF options_get_color(const char* option_name)
{
	COLORREF color_value;
	std::string_view option_value = settings.getter(option_name);
	std::istringstream decode_stream(&option_value[0]);

	decode_stream >> std::hex >> color_value;

	return color_value;
}

static void options_set_color(const char *option_name, COLORREF value)
{
	std::ostringstream encoded_stream;

	encoded_stream.fill('0');
	encoded_stream << std::setw(6) << std::hex << (value | 0);

	settings.setter(option_name, encoded_stream.str());
}

static COLORREF options_get_color_default(const char *option_name, int default_color)
{
	COLORREF color_value = options_get_color(option_name);
	if (color_value == (COLORREF) -1)
		color_value = (COLORREF)windows::get_sys_color(default_color);

	return color_value;
}

static void options_set_color_default(const char *option_name, COLORREF value, int default_color)
{
	if (value == (COLORREF)windows::get_sys_color(default_color))
		options_set_color(option_name, (COLORREF) -1);
	else
		options_set_color(option_name, value);
}

static input_seq *options_get_input_seq(const char *option_name)
{
/*
    static input_seq seq;
    std::string val = settings.getter(option_name);
    input_seq_from_tokens(NULL, seq_string.c_str(), &seq);  // HACK
    return &seq;*/
	return NULL;
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
		std::istringstream tokenStream(show_folder_option);
		std::string token;

		while (std::getline(tokenStream, token, ','))
		{
			for (size_t j = 0; g_folderData[j].m_lpTitle; j++)
			{
				std::string short_name(g_folderData[j].short_name);
				if (short_name == token)
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
	DeleteBits(show_folder_flags);
	return result;
}

void SetShowFolder(int folder, bool show)
{
	LPBITS show_folder_flags = NewBits(MAX_FOLDERS);
	int num_saved = 0;
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
			if (num_saved != 0)
				str.append(",");

			for (size_t j=0; g_folderData[j].m_lpTitle; j++)
			{
				if (g_folderData[j].m_nFolderId == i)
				{
					str.append(g_folderData[j].short_name);
					num_saved++;
					break;
				}
			}
		}
	}
	settings.setter(MUIOPTION_HIDE_FOLDERS, str);
	DeleteBits(show_folder_flags);
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
	std::string_view option_value = settings.getter(MUIOPTION_DEFAULT_GAME);
	if (option_value.empty())
		return 0;
	int val = driver_list::find(&option_value[0]);
	if (val < 0)
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

void SetWindowState(UINT state)
{
	settings.setter(MUIOPTION_WINDOW_STATE, (int)state);
}

UINT GetWindowState(void)
{
	return settings.int_value(MUIOPTION_WINDOW_STATE);
}

void SetWindowPanes(int val)
{
	settings.setter(MUIOPTION_WINDOW_PANES, val & 15);
}

UINT GetWindowPanes(void)
{
	return settings.int_value(MUIOPTION_WINDOW_PANES) & 15;
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
	settings.setter(MUIOPTION_COLUMN_WIDTHS, ColumnEncodeStringWithCount(width, COLUMN_MAX));
}

void GetColumnWidths(int width[])
{
	ColumnDecodeStringWithCount(settings.getter(MUIOPTION_COLUMN_WIDTHS), width, COLUMN_MAX);
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
	settings.setter(MUIOPTION_COLUMN_ORDER, ColumnEncodeStringWithCount(order, COLUMN_MAX));
}

void GetColumnOrder(int order[])
{
	ColumnDecodeStringWithCount(settings.getter(MUIOPTION_COLUMN_ORDER), order, COLUMN_MAX);
}

void SetColumnShown(int shown[])
{
	settings.setter(MUIOPTION_COLUMN_SHOWN, ColumnEncodeStringWithCount(shown, COLUMN_MAX));
}

void GetColumnShown(int shown[])
{
	ColumnDecodeStringWithCount(settings.getter(MUIOPTION_COLUMN_SHOWN), shown, COLUMN_MAX);
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

std::string_view GetBgDir (void)
{
	std::string_view option_value;

	option_value = settings.getter(MUIOPTION_BACKGROUND_DIRECTORY);
	if (option_value.empty())
		option_value = "bkground\\bkground.png"sv;

	return option_value;
}

void SetBgDir (std::string path)
{
	settings.setter(MUIOPTION_BACKGROUND_DIRECTORY, path);
}

std::string_view GetVideoDir(void)
{
	std::string_view option_value;

	option_value = settings.getter(MUIOPTION_VIDEO_DIRECTORY);
	if (option_value.empty())
		option_value = "video"sv;

	return option_value;
}

void SetVideoDir(std::string_view path)
{
	settings.setter(MUIOPTION_VIDEO_DIRECTORY, path);
}

std::string_view GetManualsDir(void)
{
	std::string_view option_value;

	option_value = settings.getter(MUIOPTION_MANUALS_DIRECTORY);
	if (option_value.empty())
		option_value = "manuals"sv;

	return option_value;
}

void SetManualsDir(std::string_view path)
{
	settings.setter(MUIOPTION_MANUALS_DIRECTORY, path);
}

// ***************************************************************** MAME_g.INI settings **************************************************************************
int GetRomAuditResults(uint32_t driver_index)
{
	return game_opts.rom(driver_index);
}

void SetRomAuditResults(uint32_t driver_index, int audit_results)
{
	game_opts.rom(driver_index, audit_results);
}

int GetSampleAuditResults(uint32_t driver_index)
{
	return game_opts.sample(driver_index);
}

void SetSampleAuditResults(uint32_t driver_index, int audit_results)
{
	game_opts.sample(driver_index, audit_results);
}

static void IncrementPlayVariable(uint32_t driver_index, const char *play_variable, uint32_t increment)
{
	if (!mui_strcmp(play_variable, "count"))
		game_opts.play_count(driver_index, game_opts.play_count(driver_index) + increment);
	else
	if (!mui_strcmp(play_variable, "time"))
		game_opts.play_time(driver_index, game_opts.play_time(driver_index) + increment);
}

void IncrementPlayCount(uint32_t driver_index)
{
	IncrementPlayVariable(driver_index, "count", 1);
}

uint32_t GetPlayCount(uint32_t driver_index)
{
	return game_opts.play_count(driver_index);
}

// int needed here so we can reset all games
static void ResetPlayVariable(int driver_index, const char *play_variable)
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

uint32_t GetPlayTime(uint32_t driver_index)
{
	return game_opts.play_time(driver_index);
}

void IncrementPlayTime(uint32_t driver_index, uint32_t playtime)
{
	IncrementPlayVariable(driver_index, "time", playtime);
}

std::string GetTextPlayTime(uint32_t driver_index)
{
	constexpr uint32_t sec_per_day = 86400; // seconds per day
	constexpr uint32_t sec_per_hour = 3600; // seconds per hour
	constexpr uint32_t sec_per_min = 60; // seconds per minute
	std::string text_playtime;

	if (driver_index < driver_list::total())
	{
		uint32_t days, hours, minutes, seconds;

		seconds = GetPlayTime(driver_index); // get seconds played

		days = seconds / sec_per_day; // convert seconds to days
		seconds -= sec_per_day * days; // deducted the days

		hours = seconds / sec_per_hour; // convert seconds to hours
		seconds -= sec_per_hour * hours; // deducted the hours

		minutes = seconds / sec_per_min; // convert seconds to minutes
		seconds -= sec_per_min * minutes; // deduct the minutes

		if (hours > 0 || minutes > 0)
			text_playtime = util::string_format("%02d", seconds);
		else
			text_playtime = util::string_format("%ds", seconds);

		if (hours > 0)
			text_playtime = util::string_format("%02d:%s", minutes, text_playtime);
		else if (minutes > 0)
			text_playtime = util::string_format("%d:%s", minutes, text_playtime);

		if (days > 0)
			text_playtime = util::string_format("%days %s", hours, text_playtime);

	}
	return text_playtime;
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

static int GetUIJoy(const char *option_name, int joycodeIndex)
{
	int joycodes[4];

	if ((joycodeIndex < 0) || (joycodeIndex > 3))
		joycodeIndex = 0;
	ColumnDecodeStringWithCount(settings.getter(option_name), joycodes, std::size(joycodes));
	return joycodes[joycodeIndex];
}

static void SetUIJoy(const char *option_name, int joycodeIndex, int val)
{
	int joycodes[4];

	if ((joycodeIndex < 0) || (joycodeIndex > 3))
		joycodeIndex = 0;
	ColumnDecodeStringWithCount(settings.getter(option_name), joycodes, std::size(joycodes));
	joycodes[joycodeIndex] = val;
	settings.setter(option_name, ColumnEncodeStringWithCount(joycodes, std::size(joycodes)));
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

const std::string_view GetExecCommand(void)
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
	std::istringstream tokenStream(&ss[0]);

	for (size_t i = 0; i < 16; i++)
	{
		std::istringstream decode_stream;
		std::string token;

		if (std::getline(tokenStream, token, ','))
		{
			decode_stream.str(token);
			decode_stream >> std::hex >> value[i];
		}
	}
}


static std::string ColumnEncodeStringWithCount(const int *value, const size_t count)
{
	std::string str = std::to_string(value[0]);

	for (size_t i = 1; i < count; i++)
		str += ","s + std::to_string(value[i]);

	return str;
}

static void ColumnDecodeStringWithCount(std::string_view ss, int *value, const size_t count)
{
	std::istringstream tokenStream(&ss[0]);

	for (size_t i = 0; i < count; i++)
	{
		std::string token;

		if(std::getline(tokenStream, token, ','))
			value[i] = std::stoi(token);
	}
}

static std::string SplitterEncodeString(const int *value)
{
	std::string str = std::to_string(value[0]);

	for (size_t i = 1; i < GetSplitterCount(); i++)
		str += ","s + std::to_string(value[i]);

	return str;
}

static void SplitterDecodeString(std::string_view ss, int *value)
{
	const size_t count = GetSplitterCount();
	std::istringstream tokenStream(&ss[0]);

	for (size_t i = 0; i < count; i++)
	{
		std::string token;

		if(std::getline(tokenStream, token, ','))
			value[i] = std::stoi(token);
	}
}

// Parse the given comma-delimited string into a LOGFONT structure
static void FontDecodeString(std::string_view ss, LOGFONTW* f)
{
	std::istringstream tokenStream(&ss[0]);
	std::string token;

	if (std::getline(tokenStream, token, ','))
		f->lfHeight = std::stol(token);

	if (std::getline(tokenStream, token, ','))
		f->lfWidth = std::stol(token);

	if (std::getline(tokenStream, token, ','))
		f->lfEscapement = std::stol(token);

	if (std::getline(tokenStream, token, ','))
		f->lfOrientation = std::stol(token);

	if (std::getline(tokenStream, token, ','))
		f->lfWeight = std::stol(token);

	if (std::getline(tokenStream, token, ','))
		f->lfItalic = std::stoi(token);

	if (std::getline(tokenStream, token, ','))
		f->lfUnderline = std::stoi(token);

	if (std::getline(tokenStream, token, ','))
		f->lfStrikeOut = std::stoi(token);

	if (std::getline(tokenStream, token, ','))
		f->lfCharSet = std::stoi(token);

	if (std::getline(tokenStream, token, ','))
		f->lfOutPrecision = std::stoi(token);

	if (std::getline(tokenStream, token, ','))
		f->lfClipPrecision = std::stoi(token);

	if (std::getline(tokenStream, token, ','))
		f->lfQuality = std::stoi(token);

	if (std::getline(tokenStream, token, ','))
		f->lfPitchAndFamily = std::stoi(token);

	if (std::getline(tokenStream, token, ','))
	{
		std::unique_ptr<const wchar_t[]> utf8_to_wcs(mui_wcstring_from_utf8(token.c_str()));
		if (!utf8_to_wcs)
			return;
		mui_wcscpy(f->lfFaceName, utf8_to_wcs.get());
	}
}

// Encode the given LOGFONT structure into a comma-delimited string
static std::string FontEncodeString(const LOGFONTW *f)
{
	std::string encode_string = "";
	std::unique_ptr<const char[]> utf8_FaceName(mui_utf8_from_wcstring(f->lfFaceName));

	if( !utf8_FaceName )
		return encode_string;

	encode_string = util::string_format("%li,%li,%li,%li,%li,%i,%i,%i,%i,%i,%i,%i,%i,%s",
		f->lfHeight,
		f->lfWidth,
		f->lfEscapement,
		f->lfOrientation,
		f->lfWeight,
		f->lfItalic,
		f->lfUnderline,
		f->lfStrikeOut,
		f->lfCharSet,
		f->lfOutPrecision,
		f->lfClipPrecision,
		f->lfQuality,
		f->lfPitchAndFamily,
		utf8_FaceName.get());

	return encode_string;
}

static std::string TabFlagsEncodeString(int data)
{
	std::string str;

	// we save the ones that are NOT displayed, so we can add new ones
	// and upgraders will see them
	for ( int i=0; i<MAX_TAB_TYPES; i++)
	{
		if (((data & (1 << i)) == 0) && GetImageTabShortName(i))
		{
			if (i > 0)
				str.append(",");

			str.append(GetImageTabShortName(i));
		}
	}
	return str;
}

static void TabFlagsDecodeString(std::string_view ss, int *data)
{
	std::istringstream tokenStream(&ss[0]);
	std::string token;

	// simple way to set all tab bits "on"
	*data = (1 << MAX_TAB_TYPES) - 1;

	while (std::getline(tokenStream,token, ','))
	{
		//token.resize(token.size() - 3);
		for (size_t j=0; j < MAX_TAB_TYPES; j++)
		{
			std::string imagetab_shortname(GetImageTabShortName(j));
			if (imagetab_shortname.empty() || (token == imagetab_shortname))
			{
				// turn off this bit
				*data &= ~(1 << j);
				break;
			}
		}
	}

	if (*data == 0)
	{
		// not allowed to hide all tabs, because then why even show the area?
		*data = (1 << TAB_SCREENSHOT);
	}
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

void LoadFolderFlags(void)
{
	LPTREEFOLDER lpFolder;
	size_t numFolders = GetNumFolders();

	for (size_t i = 0; i < numFolders; i++)
	{
		lpFolder = GetFolder(i);

		if (lpFolder)
		{
			char folder_name[400];
			char *ptr;

			// Convert spaces to underscores
			(void)mui_strcpy(folder_name, lpFolder->m_lpTitle);
			ptr = folder_name;
			while (*ptr && *ptr != '\0')
			{
				if ((*ptr == ' ') || (*ptr == '-'))
					*ptr = '_';

				ptr++;
			}

			std::string option_name = std::string(folder_name) + "_filters";
		}
	}

	// These are added to our UI ini
	// The normal read will skip them.

	// retrieve the stored values
	for (size_t i = 0; i < numFolders; i++)
	{
		lpFolder = GetFolder(i);

		if (lpFolder)
		{
			char folder_name[400];

			// Convert spaces to underscores
			(void)mui_strcpy(folder_name, lpFolder->m_lpTitle);
			char *ptr = folder_name;
			while (*ptr && *ptr != '\0')
			{
				if ((*ptr == ' ') || (*ptr == '-'))
					*ptr = '_';

				ptr++;
			}
			std::string option_name = std::string(folder_name) + "_filters";

			// get entry and decode it
			lpFolder->m_dwFlags |= (settings.int_value(option_name.c_str()) & F_MASK);
		}
	}
}



// Adds our folder flags to winui_options, for saving.
static void AddFolderFlags()
{
	LPTREEFOLDER lpFolder;
	size_t numFolders = GetNumFolders();

	for (size_t i = 0; i < numFolders; i++)
	{
		lpFolder = GetFolder(i);
		if (lpFolder)
		{
			char folder_name[400];

			// Convert spaces to underscores
			(void)mui_strcpy(folder_name, lpFolder->m_lpTitle);
			char *ptr = folder_name;
			while (*ptr && *ptr != '\0')
			{
				if ((*ptr == ' ') || (*ptr == '-'))
					*ptr = '_';

				ptr++;
			}

			std::string option_name = std::string(folder_name) + "_filters";

			// store entry
			settings.setter(option_name.c_str(), lpFolder->m_dwFlags & F_MASK);
		}
	}
}

// Save MAMEUI.ini
void mui_save_ini(void)
{
	// Add the folder flag to settings.
	AddFolderFlags();
	settings.save_file(&MUI_INI_FILENAME[0]);
}

void SaveGameListOptions(void)
{
	// Save GameInfo.ini - game options.
	game_opts.save_file(&GAMEINFO_INI_FILENAME[0]);
}

const char * GetVersionString(void)
{
	return emulator_info::get_build_version();
}

uint32_t GetDriverCacheLower(uint32_t driver_index)
{
	return game_opts.cache_lower(driver_index);
}

uint32_t GetDriverCacheUpper(uint32_t driver_index)
{
	return game_opts.cache_upper(driver_index);
}

void SetDriverCache(uint32_t driver_index, uint32_t val)
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

bool DriverIsComputer(uint32_t driver_index)
{
	uint32_t cache = game_opts.cache_lower(driver_index) & 3;
	return (cache == 2) ? true : false;
}

bool DriverIsConsole(uint32_t driver_index)
{
	uint32_t cache = game_opts.cache_lower(driver_index) & 3;
	return (cache == 1) ? true : false;
}

bool DriverIsModified(uint32_t driver_index)
{
	return BIT(game_opts.cache_lower(driver_index), 12);
}

bool DriverIsImperfect(uint32_t driver_index)
{
	return (game_opts.cache_lower(driver_index) & 0xff0000) ? true : false; // (NO|IMPERFECT) (CONTROLS|PALETTE|SOUND|GRAPHICS)
}

// from optionsms.cpp (MESSUI)


#define LOG_SOFTWARE 1

void SetSLColumnOrder(int order[])
{
	settings.setter(MESSUI_SL_COLUMN_ORDER, ColumnEncodeStringWithCount(order, SL_COLUMN_MAX));
}

void GetSLColumnOrder(int order[])
{
	ColumnDecodeStringWithCount(settings.getter(MESSUI_SL_COLUMN_ORDER), order, SL_COLUMN_MAX);
}

void SetSLColumnShown(int shown[])
{
	settings.setter(MESSUI_SL_COLUMN_SHOWN, ColumnEncodeStringWithCount(shown, SL_COLUMN_MAX));
}

void GetSLColumnShown(int shown[])
{
	ColumnDecodeStringWithCount(settings.getter(MESSUI_SL_COLUMN_SHOWN), shown, SL_COLUMN_MAX);
}

void SetSLColumnWidths(int width[])
{
	settings.setter(MESSUI_SL_COLUMN_WIDTHS, ColumnEncodeStringWithCount(width, SL_COLUMN_MAX));
}

void GetSLColumnWidths(int width[])
{
	ColumnDecodeStringWithCount(settings.getter(MESSUI_SL_COLUMN_WIDTHS), width, SL_COLUMN_MAX);
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
	settings.setter(MESSUI_SW_COLUMN_ORDER, ColumnEncodeStringWithCount(order, SW_COLUMN_MAX));
}

void GetSWColumnOrder(int order[])
{
	ColumnDecodeStringWithCount(settings.getter(MESSUI_SW_COLUMN_ORDER), order, SW_COLUMN_MAX);
}

void SetSWColumnShown(int shown[])
{
	settings.setter(MESSUI_SW_COLUMN_SHOWN, ColumnEncodeStringWithCount(shown, SW_COLUMN_MAX));
}

void GetSWColumnShown(int shown[])
{
	ColumnDecodeStringWithCount(settings.getter(MESSUI_SW_COLUMN_SHOWN), shown, SW_COLUMN_MAX);
}

void SetSWColumnWidths(int width[])
{
	settings.setter(MESSUI_SW_COLUMN_WIDTHS, ColumnEncodeStringWithCount(width, SW_COLUMN_MAX));
}

void GetSWColumnWidths(int width[])
{
	ColumnDecodeStringWithCount(settings.getter(MESSUI_SW_COLUMN_WIDTHS), width, SW_COLUMN_MAX);
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

