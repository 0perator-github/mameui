// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

#ifndef MAMEUI_WINAPP_EMU_OPTS_H
#define MAMEUI_WINAPP_EMU_OPTS_H

#pragma once
// standard C++ headers

// standard windows headers

// MAME headers
#include "winmain.h"
#include "ui/moptions.h"

// These help categorise the folders on the left side
// This list is mainly for documentation, although a few are used in code
typedef enum {
	// Global types
	OPTIONS_GLOBAL = 0,
	OPTIONS_HORIZONTAL,
	OPTIONS_VERTICAL,
	OPTIONS_RASTER,
	OPTIONS_VECTOR,
	OPTIONS_LCD,
	OPTIONS_ARCADE,
	OPTIONS_CONSOLE,
	OPTIONS_COMPUTER,
	OPTIONS_OTHERSYS,
	// Local types
	OPTIONS_SOURCE,
	OPTIONS_GPARENT,
	OPTIONS_PARENT,
	OPTIONS_GAME,
	// EOF marker
	OPTIONS_MAX
} OPTIONS_TYPE;

typedef enum {
	// Global types
	DIRMAP_PLUGINDATAPATH = 1,
	DIRMAP_MEDIAPATH,
	DIRMAP_HASHPATH,
	DIRMAP_SAMPLEPATH,
	DIRMAP_ARTPATH,
	DIRMAP_CTRLRPATH,
	DIRMAP_INIPATH,
	DIRMAP_FONTPATH,
	DIRMAP_CHEATPATH,
	DIRMAP_CROSSHAIRPATH,
	DIRMAP_PLUGINSPATH,
	DIRMAP_LANGUAGEPATH,
	DIRMAP_SWPATH,
	DIRMAP_CFG_DIRECTORY,
	DIRMAP_NVRAM_DIRECTORY,
	DIRMAP_INPUT_DIRECTORY,
	DIRMAP_STATE_DIRECTORY,
	DIRMAP_SNAPSHOT_DIRECTORY,
	DIRMAP_DIFF_DIRECTORY,
	DIRMAP_COMMENT_DIRECTORY,
	DIRMAP_BGFX_PATH,
	DIRMAP_HLSLPATH,
	DIRMAP_HISTORY_PATH,
	DIRMAP_CATEGORYINI_PATH,
	DIRMAP_CABINETS_PATH,
	DIRMAP_CPANELS_PATH,
	DIRMAP_PCBS_PATH,
	DIRMAP_FLYERS_PATH,
	DIRMAP_TITLES_PATH,
	DIRMAP_ENDS_PATH,
	DIRMAP_MARQUEES_PATH,
	DIRMAP_ARTPREV_PATH,
	DIRMAP_BOSSES_PATH,
	DIRMAP_LOGOS_PATH,
	DIRMAP_SCORES_PATH,
	DIRMAP_VERSUS_PATH,
	DIRMAP_GAMEOVER_PATH,
	DIRMAP_HOWTO_PATH,
	DIRMAP_SELECT_PATH,
	DIRMAP_ICONS_PATH,
	DIRMAP_COVER_PATH,
	DIRMAP_UI_PATH,
	// EOF marker
	DIRMAP_TOTAL
} emuopts_dirmap_idx_type;

constexpr auto GLOBAL_OPTIONS = -1;
constexpr auto GLOBAL_DEFAULT_OPTIONS = -2;

std::string emu_get_value(windows_options& options, std::string_view option_name);
std::string emu_get_value(windows_options *options, std::string_view option_name);

void emu_set_value(windows_options &options, std::string_view option_name, double option_value);
void emu_set_value(windows_options &options, std::string_view option_name, float option_value);
void emu_set_value(windows_options &options, std::string_view option_name, int value);
void emu_set_value(windows_options &options, std::string_view option_name, long value);
void emu_set_value(windows_options &options, std::string_view option_name, std::string option_value);

void emu_set_value(windows_options *options, std::string_view option_name, double option_value);
void emu_set_value(windows_options *options, std::string_view option_name, float option_value);
void emu_set_value(windows_options *options, std::string_view option_name, int value);
void emu_set_value(windows_options *options, std::string_view option_name, long value);
void emu_set_value(windows_options *options, std::string_view option_name, std::string option_value);

std::string ui_get_value(ui_options &options, std::string_view option_name);
std::string ui_get_value(ui_options *options, std::string_view option_name);


void dir_set_value(uint32_t dir_index, std::string value);
std::string dir_get_value(uint32_t dir_index);
void emu_opts_init(bool);
void ui_save_ini();
std::wstring get_exe_path(void);
std::string get_exe_path_utf8(void);
std::wstring get_ini_dir(void);
std::string get_ini_dir_utf8(void);
const char* GetSnapName();
void SetSnapName(const char*);
const std::string GetLanguageUI();
bool GetEnablePlugins();
bool GetSkipWarnings();
void SetSkipWarnings(bool);
const std::string GetPlugins();
void SetSelectedSoftware(uint32_t driver_index, std::string opt_name, std::string software);
void global_save_ini(void);
bool DriverHasSoftware(uint32_t drvindex);
void ResetGameDefaults(void);
void ResetAllGameOptions(void);
windows_options &MameUIGlobal(void);
void SetSystemName(uint32_t driver_index);
bool AreOptionsEqual(windows_options &options1, windows_options &options2);
void OptionsCopy(windows_options &source, windows_options &dest);
void SetDirectories(windows_options &options);
void load_options(windows_options &options, OPTIONS_TYPE option_type, int game_number, bool set_system_name);
void save_options(windows_options &options, OPTIONS_TYPE option_type, int game_number);


#endif // MAMEUI_WINAPP_EMU_OPTS_H
