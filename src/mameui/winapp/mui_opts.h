// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

#ifndef MAMEUI_WINAPP_MUI_OPTS_H
#define MAMEUI_WINAPP_MUI_OPTS_H

#pragma once

// Because we have added the Options after MAX_TAB_TYPES, we have to subtract 3 here
// (that's how many options we have after MAX_TAB_TYPES)
constexpr auto TAB_SUBTRACT = 3;

constexpr auto OPTIONS_TYPE_GLOBAL = -1;
constexpr auto OPTIONS_TYPE_FOLDER = -2;

// List of artwork types to display in the screen shot area
using artwork_type = enum artwork_type
{
	// these must match array of strings image_tabs_long_name in mui_opts.cpp
	// if you add new Tabs, be sure to also add them to the ComboBox init in dialogs.cpp
	TAB_ARTWORK = 0,
	TAB_BOSSES,
	TAB_CABINET,
	TAB_CONTROL_PANEL,
	TAB_COVER,
	TAB_ENDS,
	TAB_FLYER,
	TAB_GAMEOVER,
	TAB_HOWTO,
	TAB_LOGO,
	TAB_MARQUEE,
	TAB_PCB,
	TAB_SCORES,
	TAB_SELECT,
	TAB_SCREENSHOT,
	TAB_TITLE,
	TAB_VERSUS,
	TAB_HISTORY,
	MAX_TAB_TYPES,
	BACKGROUND,
	TAB_ALL,
	TAB_NONE
};

// List of columns in the main game list
using machinepicker_column_id = enum machinepicker_column_id
{
	COLUMN_GAMES,
	COLUMN_SRCDRIVERS,
	COLUMN_DIRECTORY,
	COLUMN_TYPE,
	COLUMN_ORIENTATION,
	COLUMN_MANUFACTURER,
	COLUMN_YEAR,
	COLUMN_PLAYED,
	COLUMN_PLAYTIME,
	COLUMN_CLONE,
	COLUMN_TRACKBALL,
	COLUMN_SAMPLES,
	COLUMN_ROMS,
	COLUMN_COUNT
};

// from optionsms.h (MESSUI)

using softwarelist_column_id = enum softwarelist_column_id
{
	SL_COLUMN_IMAGES,
	SL_COLUMN_GOODNAME,
	SL_COLUMN_MANUFACTURER,
	SL_COLUMN_YEAR,
	SL_COLUMN_PLAYABLE,
	SL_COLUMN_USAGE,
	SL_COLUMN_COUNT
};

using softwarepicker_column_id = enum softwarepicker_column_id
{
	SW_COLUMN_IMAGES,
	SW_COLUMN_COUNT
};

using AREA = struct window_area
{
	int x, y, width, height;
};

using ScreenParams = struct screen_parameter
{
	char *screen;
	char *aspect;
	char *resolution;
	char *view;
};

// Keyboard control of ui
input_seq *Get_ui_key_down(void);
input_seq *Get_ui_key_end(void);
input_seq *Get_ui_key_history_down(void);
input_seq *Get_ui_key_history_up(void);
input_seq *Get_ui_key_home(void);
input_seq *Get_ui_key_left(void);
input_seq *Get_ui_key_pgdwn(void);
input_seq *Get_ui_key_pgup(void);
input_seq *Get_ui_key_right(void);
input_seq *Get_ui_key_ss_change(void);
input_seq *Get_ui_key_start(void);
input_seq *Get_ui_key_up(void);

input_seq *Get_ui_key_context_filters(void);
input_seq *Get_ui_key_game_audit(void);
input_seq *Get_ui_key_game_properties(void);
input_seq *Get_ui_key_help_contents(void);
input_seq *Get_ui_key_select_random(void);
input_seq *Get_ui_key_update_gamelist(void);
input_seq *Get_ui_key_view_folders(void);
input_seq *Get_ui_key_view_fullscreen(void);
input_seq *Get_ui_key_view_pagetab(void);
input_seq *Get_ui_key_view_picture_area(void);
input_seq *Get_ui_key_view_software_area(void);
input_seq *Get_ui_key_view_status(void);
input_seq *Get_ui_key_view_toolbars(void);

input_seq *Get_ui_key_quit(void);
input_seq *Get_ui_key_view_tab_cabinet(void);
input_seq *Get_ui_key_view_tab_cpanel(void);
input_seq *Get_ui_key_view_tab_flyer(void);
input_seq *Get_ui_key_view_tab_history(void);
input_seq *Get_ui_key_view_tab_marquee(void);
input_seq *Get_ui_key_view_tab_pcb(void);
input_seq *Get_ui_key_view_tab_screenshot(void);
input_seq *Get_ui_key_view_tab_title(void);

void OptionsInit(void);

void LoadFolderFlags(void);

// Start interface to directories.h

std::string GetManualsDir(void);
void SetManualsDir(std::string_view path);

std::string GetVideoDir(void);
void SetVideoDir(std::string_view path);

// End interface to directories.h

void mui_save_ini(void);
void SaveGameListOptions(void);

void ResetGUI(void);

std::wstring_view GetImageTabLongName(int tab_index);
std::wstring_view GetImageTabShortName(int tab_index);

void SetViewMode(int val);
int  GetViewMode(void);

void SetGameCheck(bool game_check);
bool GetGameCheck(void);

void SetJoyGUI(bool use_joygui);
bool GetJoyGUI(void);

void SetKeyGUI(bool use_keygui);
bool GetKeyGUI(void);

void SetCycleScreenshot(int cycle_screenshot);
int GetCycleScreenshot(void);

void SetStretchScreenShotLarger(bool stretch);
bool GetStretchScreenShotLarger(void);

void SetScreenshotBorderSize(int size);
int GetScreenshotBorderSize(void);

void SetScreenshotBorderColor(COLORREF uColor);
COLORREF GetScreenshotBorderColor(void);

void SetFilterInherit(bool inherit);
bool GetFilterInherit(void);

void SetOffsetClones(bool offset);
bool GetOffsetClones(void);

void SetSavedFolderID(UINT val);
UINT GetSavedFolderID(void);

void SetOverrideRedX(bool val);
bool GetOverrideRedX(void);

bool GetShowFolder(int folder);
void SetShowFolder(int folder,bool show);

void SetShowStatusBar(bool val);
bool GetShowStatusBar(void);

void SetShowToolBar(bool val);
bool GetShowToolBar(void);

void SetShowTabCtrl(bool val);
bool GetShowTabCtrl(void);

void SetCurrentTab(int val);
int GetCurrentTab(void);

void SetDefaultGame(int val);
uint32_t GetDefaultGame(void);

void SetWindowArea(const AREA *area);
void GetWindowArea(AREA *area);

void SetWindowState(uint32_t state);
uint32_t GetWindowState(void);

void SetWindowPanes(uint32_t val);
uint32_t GetWindowPanes(void);

void SetColumnWidths(int widths[]);
void GetColumnWidths(int widths[]);

void SetColumnOrder(int order[]);
void GetColumnOrder(int order[]);

void SetColumnShown(int shown[]);
void GetColumnShown(int shown[]);

void SetSplitterPos(int splitterId, int pos);
int  GetSplitterPos(int splitterId);

void SetCustomColor(int iIndex, COLORREF uColor);
COLORREF GetCustomColor(int iIndex);

void SetListFont(const LOGFONT *font);
void GetListFont(LOGFONT *font);

DWORD GetFolderFlags(int folder_index);

void SetListFontColor(COLORREF uColor);
COLORREF GetListFontColor(void);

void SetListCloneColor(COLORREF uColor);
COLORREF GetListCloneColor(void);

int GetHistoryTab(void);
void SetHistoryTab(int tab,bool show);

int GetShowTab(int tab);
void SetShowTab(int tab,bool show);
bool AllowedToSetShowTab(int tab,bool show);

void SetSortColumn(int column);
int  GetSortColumn(void);

void SetSortReverse(bool reverse);
bool GetSortReverse(void);

std::string GetBgDir(void);
void SetBgDir(std::string_view path);

int GetRomAuditResults(int driver_index);
void SetRomAuditResults(int driver_index, int audit_results);

int GetSampleAuditResults(int driver_index);
void SetSampleAuditResults(int driver_index, int audit_results);

void IncrementPlayCount(int driver_index);
uint32_t GetPlayCount(int driver_index);
void ResetPlayCount(int driver_index);

void IncrementPlayTime(int driver_index, uint32_t playtime);
uint32_t GetPlayTime(int driver_index);
std::wstring GetTextPlayTime(int driver_index);
void ResetPlayTime(int driver_index);

std::string GetVersionString(void);

int GetUIJoyUp(int joycodeIndex);
void SetUIJoyUp(int joycodeIndex, int val);

int GetUIJoyDown(int joycodeIndex);
void SetUIJoyDown(int joycodeIndex, int val);

int GetUIJoyLeft(int joycodeIndex);
void SetUIJoyLeft(int joycodeIndex, int val);

int GetUIJoyRight(int joycodeIndex);
void SetUIJoyRight(int joycodeIndex, int val);

int GetUIJoyStart(int joycodeIndex);
void SetUIJoyStart(int joycodeIndex, int val);

int GetUIJoyPageUp(int joycodeIndex);
void SetUIJoyPageUp(int joycodeIndex, int val);

int GetUIJoyPageDown(int joycodeIndex);
void SetUIJoyPageDown(int joycodeIndex, int val);

int GetUIJoyHome(int joycodeIndex);
void SetUIJoyHome(int joycodeIndex, int val);

int GetUIJoyEnd(int joycodeIndex);
void SetUIJoyEnd(int joycodeIndex, int val);

int GetUIJoySSChange(int joycodeIndex);
void SetUIJoySSChange(int joycodeIndex, int val);

int GetUIJoyHistoryUp(int joycodeIndex);
void SetUIJoyHistoryUp(int joycodeIndex, int val);

int GetUIJoyHistoryDown(int joycodeIndex);
void SetUIJoyHistoryDown(int joycodeIndex, int val);

int GetUIJoyExec(int joycodeIndex);
void SetUIJoyExec(int joycodeIndex, int val);

std::string GetExecCommand(void);
void SetExecCommand(char *cmd);

int GetExecWait(void);
void SetExecWait(int wait);

bool GetHideMouseOnStartup(void);
void SetHideMouseOnStartup(bool hide);

bool GetRunFullScreen(void);
void SetRunFullScreen(bool fullScreen);

uint64_t GetDriverCacheLower(int driver_index);
uint32_t GetDriverCacheUpper(int driver_index);

void SetDriverCache(int driver_index, uint32_t val);

bool RequiredDriverCache(void);

void ForceRebuild(void);

bool DriverIsComputer(int driver_index);
bool DriverIsConsole(int driver_index);
bool DriverIsModified(int driver_index);
bool DriverIsImperfect(int driver_index);

std::string GetGameName(int driver_index);

void SetSWColumnWidths(int widths[]);
void GetSWColumnWidths(int widths[]);

void SetSWColumnOrder(int order[]);
void GetSWColumnOrder(int order[]);

void SetSWColumnShown(int shown[]);
void GetSWColumnShown(int shown[]);

void SetSWSortColumn(int column);
int  GetSWSortColumn(void);

void SetSWSortReverse(bool reverse);
bool GetSWSortReverse(void);

void SetSLColumnWidths(int widths[]);
void GetSLColumnWidths(int widths[]);

void SetSLColumnOrder(int order[]);
void GetSLColumnOrder(int order[]);

void SetSLColumnShown(int shown[]);
void GetSLColumnShown(int shown[]);

void SetSLSortColumn(int column);
int  GetSLSortColumn(void);

void SetSLSortReverse(bool reverse);
bool GetSLSortReverse(void);

void SetCurrentSoftwareTab(int val);
int GetCurrentSoftwareTab(void);

#endif // MAMEUI_WINAPP_MUI_OPTS_H
