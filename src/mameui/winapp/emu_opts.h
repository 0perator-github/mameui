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
	SOFTWARETYPE_COMPAT,
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

class emu_options_manager
{
public:
	struct dir_data { std::string dir_path; int which; };

	static const std::filesystem::path UI_FILEPATH;

	emu_options_manager();

	emu_options &GetMameOpts();
	ui_options &GetUIOpts();
	windows_options &GetGlobalOpts();

	std::string dir_get_value(size_t dir_index);
	void dir_set_value(size_t dir_index, std::string value);

	std::string emu_get_value(windows_options &options, std::string_view option_name);
	std::string emu_get_value(windows_options *options, std::string_view option_name);

	void emu_set_value(windows_options &options, std::string_view option_name, double option_value);
	void emu_set_value(windows_options &options, std::string_view option_name, float option_value);
	void emu_set_value(windows_options &options, std::string_view option_name, int value);
	void emu_set_value(windows_options &options, std::string_view option_name, long value);
	void emu_set_value(windows_options &options, std::string_view option_name, std::string_view option_value);

	void emu_set_value(windows_options *options, std::string_view option_name, double option_value);
	void emu_set_value(windows_options *options, std::string_view option_name, float option_value);
	void emu_set_value(windows_options *options, std::string_view option_name, int value);
	void emu_set_value(windows_options *options, std::string_view option_name, long value);
	void emu_set_value(windows_options *options, std::string_view option_name, std::string_view option_value);

	std::string ui_get_value(ui_options &options, std::string_view option_name);
	std::string ui_get_value(ui_options *options, std::string_view option_name);

	void ui_set_value(ui_options &options, std::string_view option_name, double option_value);
	void ui_set_value(ui_options &options, std::string_view option_name, float option_value);
	void ui_set_value(ui_options &options, std::string_view option_name, int value);
	void ui_set_value(ui_options &options, std::string_view option_name, long value);
	void ui_set_value(ui_options &options, std::string_view option_name, std::string_view option_value);

	void ui_set_value(ui_options *options, std::string_view option_name, double option_value);
	void ui_set_value(ui_options *options, std::string_view option_name, float option_value);
	void ui_set_value(ui_options *options, std::string_view option_name, int value);
	void ui_set_value(ui_options *options, std::string_view option_name, long value);
	void ui_set_value(ui_options *options, std::string_view option_name, std::string_view option_value);

	void load_options(windows_options &options, SOFTWARETYPE_OPTIONS software_type, int driver_index, bool set_system_name);
	void save_options(windows_options &options, SOFTWARETYPE_OPTIONS software_type, int driver_index);

	std::wstring get_exe_path();
	std::string get_exe_path_utf8();

	std::wstring get_ini_dir();
	std::string get_ini_dir_utf8();

	std::string GetSnapName();
	void SetSnapName(std::string_view name);

	bool AreOptionsEqual(windows_options &options1, windows_options &options2);
	bool DriverHasSoftware(int driver_index);
	bool GetEnablePlugins();
	bool GetSkipWarnings();
	const std::string GetLanguageUI();
	const std::string GetPluginDataPath();
	const std::string GetPlugins();
	void emu_opts_init(bool);
	void global_save_ini();
	void OptionsCopy(windows_options &source, windows_options &dest);
	void ResetGameDefaults();
	void SetDirectories(windows_options &options);
	void SetSelectedSoftware(int driver_index, std::string opt_name, std::string software);
	void SetSkipWarnings(bool);
	void SetSystemName(int driver_index);
	void ui_save_ini();

private:
	std::unique_ptr<emu_options> m_mame_opts;
	std::unique_ptr<ui_options> m_ui_opts;
	std::unique_ptr<windows_options> m_global_opts;

	std::map<int, dir_data> m_dir_map;

	std::string trim_end_of_path_utf8(std::string_view path);
	std::wstring trim_end_of_path(std::wstring_view path);

	void LoadSettingsFile(ui_options &options, std::string filename);
	void LoadSettingsFile(windows_options &options, std::string filename);
	void SaveSettingsFile(windows_options &options, std::string filename);
	void SaveSettingsFile(ui_options &options, std::string filename);
	void ResetToDefaults(windows_options &options, int priority);
};

extern emu_options_manager emu_opts;

#endif // MAMEUI_WINAPP_EMU_OPTS_H
