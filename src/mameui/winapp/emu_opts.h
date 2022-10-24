// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

#ifndef MAMEUI_WINAPP_EMU_OPTS_H
#define MAMEUI_WINAPP_EMU_OPTS_H

#pragma once

// These help categorise the folders on the left side
// This list is mainly for documentation, although a few are used in code
using SOFTWARETYPE_OPTIONS = enum softwaretype_options {
	// Global types
	SOFTWARETYPE_GLOBAL = 0,
	SOFTWARETYPE_HORIZONTAL,
	SOFTWARETYPE_VERTICAL,
	SOFTWARETYPE_RASTER,
	SOFTWARETYPE_VECTOR,
	SOFTWARETYPE_LCD,
	SOFTWARETYPE_ARCADE,
	SOFTWARETYPE_CONSOLE,
	SOFTWARETYPE_COMPUTER,
	SOFTWARETYPE_OTHERSYS,
	// Local types
	SOFTWARETYPE_SOURCE,
	SOFTWARETYPE_GPARENT,
	SOFTWARETYPE_PARENT,
	SOFTWARETYPE_GAME,
	// EOF marker
	TOTAL_SOFTWARETYPE_OPTIONS
};

using DIRPATH_OPTIONS = enum dirpath_options {
	// Global types
	DIRPATH_PLUGINDATAPATH = 1,
	DIRPATH_MEDIAPATH,
	DIRPATH_HASHPATH,
	DIRPATH_SAMPLEPATH,
	DIRPATH_ARTPATH,
	DIRPATH_CTRLRPATH,
	DIRPATH_INIPATH,
	DIRPATH_FONTPATH,
	DIRPATH_CHEATPATH,
	DIRPATH_CROSSHAIRPATH,
	DIRPATH_PLUGINSPATH,
	DIRPATH_LANGUAGEPATH,
	DIRPATH_SWPATH,
	DIRPATH_CFG_DIRECTORY,
	DIRPATH_NVRAM_DIRECTORY,
	DIRPATH_INPUT_DIRECTORY,
	DIRPATH_STATE_DIRECTORY,
	DIRPATH_SNAPSHOT_DIRECTORY,
	DIRPATH_DIFF_DIRECTORY,
	DIRPATH_COMMENT_DIRECTORY,
	DIRPATH_BGFX_PATH,
	DIRPATH_HLSLPATH,
	DIRPATH_HISTORY_PATH,
	DIRPATH_CATEGORYINI_PATH,
	DIRPATH_CABINETS_PATH,
	DIRPATH_CPANELS_PATH,
	DIRPATH_PCBS_PATH,
	DIRPATH_FLYERS_PATH,
	DIRPATH_TITLES_PATH,
	DIRPATH_ENDS_PATH,
	DIRPATH_MARQUEES_PATH,
	DIRPATH_ARTPREV_PATH,
	DIRPATH_BOSSES_PATH,
	DIRPATH_LOGOS_PATH,
	DIRPATH_SCORES_PATH,
	DIRPATH_VERSUS_PATH,
	DIRPATH_GAMEOVER_PATH,
	DIRPATH_HOWTO_PATH,
	DIRPATH_SELECT_PATH,
	DIRPATH_ICONS_PATH,
	DIRPATH_COVER_PATH,
	DIRPATH_UI_PATH,
	// EOF marker
	TOTAL_DIRPATH_OPTIONS
};

constexpr auto GLOBAL_OPTIONS = -1;
constexpr auto GLOBAL_DEFAULT_OPTIONS = -2;

std::string emu_get_value(windows_options &options, std::string_view option_name);
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
void load_options(windows_options &options, SOFTWARETYPE_OPTIONS software_type, int game_number, bool set_system_name);
void save_options(windows_options &options, SOFTWARETYPE_OPTIONS software_type, int game_number);


#endif // MAMEUI_WINAPP_EMU_OPTS_H
