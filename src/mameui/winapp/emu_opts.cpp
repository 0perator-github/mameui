// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

 /***************************************************************************

  emu_opts.cpp

  Interface to MAME's options and ini files.

***************************************************************************/

// standard windows headers

// standard C headers
#include <iostream>
#include <memory>
#include <string_view>

// MAME/MAMEUI headers
#include "emu.h"
#include "drivenum.h"
#include "main.h"
#include "path.h"

#include "winapi_system_services.h"

#include "mui_str.h"
#include "mui_wcs.h"
#include "mui_wcsconv.h"

#include "emu_opts.h"

using namespace std::literals;
using namespace mameui::winapi;

static emu_options mameopts; // core options
static ui_options emu_ui; // ui.ini
static windows_options emu_global; // Global 'default' options
constexpr std::string_view UI_FILEPATH("ini\\ui.ini"sv);

std::string emu_get_value(windows_options& options, std::string_view option_name)
{
	const char* value = options.value(&option_name[0]);
	if (!value)
		return ""s;
	else
		return value;
}

std::string emu_get_value(windows_options *options, std::string_view option_name)
{
	const char *value = options->value(&option_name[0]);
	if (!value)
		return ""s;
	else
		return value;
}

// char names

void emu_set_value(windows_options &options, std::string_view option_name, double option_value)
{
	std::ostringstream ss;

	ss << std::fixed << option_value;
	options.set_value(option_name, ss.str(), OPTION_PRIORITY_CMDLINE);
}

void emu_set_value(windows_options &options, std::string_view option_name, float option_value)
{
	std::ostringstream ss;

	ss << std::fixed << option_value;
	options.set_value(option_name, ss.str(), OPTION_PRIORITY_CMDLINE);
}

void emu_set_value(windows_options &options, std::string_view option_name, int option_value)
{
	std::ostringstream ss;

	ss << option_value;
	options.set_value(option_name, ss.str(), OPTION_PRIORITY_CMDLINE);
}

void emu_set_value(windows_options &options, std::string_view option_name, long option_value)
{
	std::ostringstream ss;

	ss << option_value;
	options.set_value(option_name, ss.str(), OPTION_PRIORITY_CMDLINE);
}

void emu_set_value(windows_options &options, std::string_view option_name, std::string option_value)
{
	options.set_value(option_name, option_value, OPTION_PRIORITY_CMDLINE);
}

void emu_set_value(windows_options *options, std::string_view option_name, double option_value)
{
	std::ostringstream ss;

	ss << std::fixed << option_value;
	options->set_value(option_name, ss.str(), OPTION_PRIORITY_CMDLINE);
}

void emu_set_value(windows_options *options, std::string_view option_name, float option_value)
{
	std::ostringstream ss;

	ss << std::fixed << option_value;
	options->set_value(option_name, ss.str(), OPTION_PRIORITY_CMDLINE);
}

void emu_set_value(windows_options *options, std::string_view option_name, int option_value)
{
	std::ostringstream ss;

	ss << option_value;
	options->set_value(option_name, ss.str(), OPTION_PRIORITY_CMDLINE);
}

void emu_set_value(windows_options *options, std::string_view option_name, long option_value)
{
	std::ostringstream ss;

	ss << option_value;
	options->set_value(option_name, ss.str(), OPTION_PRIORITY_CMDLINE);
}

void emu_set_value(windows_options *options, std::string_view option_name, std::string option_value)
{
	options->set_value(option_name, option_value, OPTION_PRIORITY_CMDLINE);
}

std::string ui_get_value(ui_options &options, std::string_view option_name)
{
	const char* value = options.value(&option_name[0]);
	if (!value)
		return "";
	else
		return value;
}

std::string ui_get_value(ui_options *options, std::string_view option_name)
{
	const char* value = options->value(&option_name[0]);
	if (!value)
		return "";
	else
		return value;
}

void ui_set_value(ui_options &options, std::string_view option_name, double option_value)
{
	std::ostringstream ss;

	ss << option_value;
	options.set_value(option_name, ss.str(), OPTION_PRIORITY_CMDLINE);
}

void ui_set_value(ui_options &options, std::string_view option_name, float option_value)
{
	std::ostringstream ss;

	ss << option_value;
	options.set_value(option_name, ss.str(), OPTION_PRIORITY_CMDLINE);
}

void ui_set_value(ui_options &options, std::string_view option_name, int option_value)
{
	std::ostringstream ss;

	ss << option_value;
	options.set_value(option_name, ss.str(), OPTION_PRIORITY_CMDLINE);
}

void ui_set_value(ui_options &options, std::string_view option_name, long option_value)
{
	std::ostringstream ss;

	ss << option_value;
	options.set_value(option_name, ss.str(), OPTION_PRIORITY_CMDLINE);
}

void ui_set_value(ui_options &options, std::string_view option_name, std::string option_value)
{
	options.set_value(option_name, option_value, OPTION_PRIORITY_CMDLINE);
}

void ui_set_value(ui_options *options, std::string_view option_name, double option_value)
{
	std::ostringstream ss;

	ss << option_value;
	options->set_value(option_name, ss.str(), OPTION_PRIORITY_CMDLINE);
}

void ui_set_value(ui_options *options, std::string_view option_name, float option_value)
{
	std::ostringstream ss;

	ss << option_value;
	options->set_value(option_name, ss.str(), OPTION_PRIORITY_CMDLINE);
}

void ui_set_value(ui_options *options, std::string_view option_name, int option_value)
{
	std::ostringstream ss;

	ss << option_value;
	options->set_value(option_name, ss.str(), OPTION_PRIORITY_CMDLINE);
}

void ui_set_value(ui_options *options, std::string_view option_name, long option_value)
{
	std::ostringstream ss;

	ss << option_value;
	options->set_value(option_name, ss.str(), OPTION_PRIORITY_CMDLINE);
}

void ui_set_value(ui_options *options, std::string_view option_name, std::string option_value)
{
	options->set_value(option_name, option_value, OPTION_PRIORITY_CMDLINE);
}

struct dir_data { std::string dir_path; int which; };
static std::map<int, dir_data> dir_map;

std::wstring get_ini_dir(void)
{
	std::wstring ini_path;
	std::wstring_view option_value;

	ini_path = get_exe_path();
	option_value = std::unique_ptr<wchar_t[]>(mui_wcstring_from_utf8(mameopts.ini_path())).get();

	if (option_value[0] != L'.')
	{
		std::wstring_view::size_type semicolon_pos = option_value.find(L';');

		if (std::wstring_view::npos != semicolon_pos)
			ini_path = ini_path + L"\\"s + std::wstring(&option_value[0], semicolon_pos);
	}

	return ini_path;
}

std::string get_ini_dir_utf8(void)
{
	std::string ini_path;

	std::unique_ptr<char[]> utf8_ini_path(mui_utf8_from_wcstring(&get_ini_dir()[0]));
	if (utf8_ini_path)
		ini_path = utf8_ini_path.get();

	return ini_path;
}

std::wstring get_exe_path(void)
{
	std::wstring exe_path(MAX_PATH, L'\0');
	std::wstring::size_type last_backslash;

	GetModuleFileNameW(0, &exe_path[0], MAX_PATH);

	last_backslash = exe_path.rfind(L'\\');
	if (std::wstring::npos != last_backslash)
		exe_path.resize(last_backslash);

	return exe_path;
}

std::string get_exe_path_utf8(void)
{
	std::string exe_path;
	std::unique_ptr<char[]> utf8_exe_path(mui_utf8_from_wcstring(&get_exe_path()[0]));

	if (utf8_exe_path)
		exe_path = utf8_exe_path.get();

	return exe_path;
}

// load newui settings
static void LoadSettingsFile(ui_options &options, const char *filename)
{
	util::core_file::ptr file;

	std::error_condition filerr = util::core_file::open(filename, OPEN_FLAG_READ, file);
	if (!filerr)
	{
		options.parse_ini_file(*file, OPTION_PRIORITY_CMDLINE, true, true);
		file.reset();
	}
}

// load a game ini
static void LoadSettingsFile(windows_options &options, const char *filename)
{
	util::core_file::ptr file;

	std::error_condition filerr = util::core_file::open(filename, OPEN_FLAG_READ, file);
	if (!filerr)
	{
		options.parse_ini_file(*file, OPTION_PRIORITY_CMDLINE, true, true);
		file.reset();
	}
}

// This saves changes to <game>.INI or MAME.INI only
static void SaveSettingsFile(windows_options &options, const char *filename)
{
	util::core_file::ptr file;

	std::error_condition filerr = util::core_file::open(filename, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS, file);

	if (!filerr)
	{
		std::string inistring = options.output_ini();
		// printf("=====%s=====\n%s\n",filename,&inistring[0]);  // for debugging
		file->puts(&inistring[0]);
		file.reset();
	}
}

/*  get options, based on passed in game number. */
void load_options(windows_options& options, OPTIONS_TYPE option_type, int game_number, bool set_system_name)
{
	const game_driver* driver = 0;
	if (game_number > -1)
		driver = &driver_list::driver(game_number);

	// Try base ini first
	std::string filename = std::string(emulator_info::get_configname()) + ".ini"s;
	LoadSettingsFile(options, &filename[0]);

	if (option_type == OPTIONS_SOURCE)
	{
		std::string type_source(core_filename_extract_base(driver->type.source(), true));
		filename = get_ini_dir_utf8() + PATH_SEPARATOR + "source"s + PATH_SEPARATOR + type_source + ".ini"s;
	}
	else
	if (option_type == OPTIONS_COMPUTER)
		filename = get_ini_dir_utf8() + PATH_SEPARATOR + "computer.ini"s;
	else
	if (option_type == OPTIONS_CONSOLE)
		filename = get_ini_dir_utf8() + PATH_SEPARATOR + "console.ini"s;
	else
	if (option_type == OPTIONS_HORIZONTAL)
		filename = get_ini_dir_utf8() + PATH_SEPARATOR + "horizontal.ini"s;
	else
	if (option_type == OPTIONS_RASTER)
		filename = get_ini_dir_utf8() + PATH_SEPARATOR + "raster.ini"s;
	else
	if (option_type == OPTIONS_VECTOR)
		filename = get_ini_dir_utf8() + PATH_SEPARATOR + "vector.ini"s;
	else
	if (option_type == OPTIONS_VERTICAL)
		filename = get_ini_dir_utf8() + PATH_SEPARATOR + "vertical.ini"s;
	else
		filename.clear();

	if (!filename.empty())
	{
		LoadSettingsFile(options, &filename[0]);
		return;
	}

	if (game_number > -2)
	{
		// Now try global ini
		filename = get_ini_dir_utf8() + PATH_SEPARATOR + std::string(emulator_info::get_configname()) + ".ini"s;
		LoadSettingsFile(options, &filename[0]);

		if (game_number > -1)
		{
			// Lastly, gamename.ini
			if (driver)
			{
				filename = get_ini_dir_utf8() + PATH_SEPARATOR + std::string(driver->name) + ".ini"s;
				if (set_system_name)
					options.set_value(OPTION_SYSTEMNAME, driver->name, OPTION_PRIORITY_CMDLINE);
				LoadSettingsFile(options, &filename[0]);
			}
		}
	}
	if (game_number > -1)
		SetDirectories(options);
}

/* Save ini file based on game_number. */
void save_options(windows_options& options, OPTIONS_TYPE option_type, int game_number)
{
	const game_driver* driver = 0;
	std::string filename, filepath;

	if (option_type == OPTIONS_COMPUTER)
		filename = get_ini_dir_utf8() + PATH_SEPARATOR + "computer.ini"s;
	else
	if (option_type == OPTIONS_CONSOLE)
		filename = get_ini_dir_utf8() + PATH_SEPARATOR + "console.ini"s;
	else
	if (option_type == OPTIONS_HORIZONTAL)
		filename = get_ini_dir_utf8() + PATH_SEPARATOR + "horizontal.ini"s;
	else
	if (option_type == OPTIONS_RASTER)
		filename = get_ini_dir_utf8() + PATH_SEPARATOR + "raster.ini"s;
	else
	if (option_type == OPTIONS_VECTOR)
		filename = get_ini_dir_utf8() + PATH_SEPARATOR + "vector.ini"s;
	else
	if (option_type == OPTIONS_VERTICAL)
		filename = get_ini_dir_utf8() + PATH_SEPARATOR + "vertical.ini"s;

	if (!filename.empty())
	{
		SaveSettingsFile(options, &filename[0]);
		return;
	}

	if (game_number >= 0)
	{
		driver = &driver_list::driver(game_number);
		if (driver)
		{
			filename.assign(driver->name);
			if (option_type == OPTIONS_SOURCE)
			{
				std::string type_source(core_filename_extract_base(driver->type.source(), true));
				filepath = get_ini_dir_utf8() + PATH_SEPARATOR + "source"s + PATH_SEPARATOR + type_source + ".ini"s;
			}
		}
	}
	else
		if (game_number == -1)
			filename = std::string(emulator_info::get_configname());

	if (!filename.empty() && filepath.empty())
		filepath = get_ini_dir_utf8() + PATH_SEPARATOR + filename + ".ini"s;

	if (game_number == -2)
		filepath = std::string(emulator_info::get_configname()) + ".ini"s;

	if (!filepath.empty())
	{
		if (game_number > -1)
			SetDirectories(options);
		SaveSettingsFile(options, &filepath[0]);
		//      printf("Settings saved to %s\n",&filepath[0]);
	}
	//  else
	//      printf("Unable to save settings\n");
}

void emu_opts_init(bool b)
{
	std::cout << "emuOptsInit: About to load " << UI_FILEPATH << std::endl;
	LoadSettingsFile(emu_ui, &UI_FILEPATH[0]);                // parse UI.INI
	std::cout << "emuOptsInit: About to load Global Options" << std::endl;
	load_options(emu_global, OPTIONS_GLOBAL, -1, 0);   // parse MAME.INI
	std::cout << "emuOptsInit: Finished" << std::endl;

	if (b)
		return;

	dir_map[DIRMAP_PLUGINDATAPATH] = dir_data { OPTION_PLUGINDATAPATH, 0 };
	dir_map[DIRMAP_MEDIAPATH] = dir_data { OPTION_MEDIAPATH, 0 };
	dir_map[DIRMAP_HASHPATH] = dir_data { OPTION_HASHPATH, 0 };
	dir_map[DIRMAP_SAMPLEPATH] = dir_data { OPTION_SAMPLEPATH, 0 };
	dir_map[DIRMAP_ARTPATH] = dir_data { OPTION_ARTPATH, 0 };
	dir_map[DIRMAP_CTRLRPATH] = dir_data { OPTION_CTRLRPATH, 0 };
	dir_map[DIRMAP_INIPATH] = dir_data { OPTION_INIPATH, 0 };
	dir_map[DIRMAP_FONTPATH] = dir_data { OPTION_FONTPATH, 0 };
	dir_map[DIRMAP_CHEATPATH] = dir_data { OPTION_CHEATPATH, 0 };
	dir_map[DIRMAP_CROSSHAIRPATH] = dir_data { OPTION_CROSSHAIRPATH, 0 };
	dir_map[DIRMAP_PLUGINSPATH] = dir_data { OPTION_PLUGINSPATH, 0 };
	dir_map[DIRMAP_LANGUAGEPATH] = dir_data { OPTION_LANGUAGEPATH, 0 };
	dir_map[DIRMAP_SWPATH] = dir_data { OPTION_SWPATH, 0 };
	dir_map[DIRMAP_CFG_DIRECTORY] = dir_data { OPTION_CFG_DIRECTORY, 0 };
	dir_map[DIRMAP_NVRAM_DIRECTORY] = dir_data { OPTION_NVRAM_DIRECTORY, 0 };
	dir_map[DIRMAP_INPUT_DIRECTORY] = dir_data { OPTION_INPUT_DIRECTORY, 0 };
	dir_map[DIRMAP_STATE_DIRECTORY] = dir_data { OPTION_STATE_DIRECTORY, 0 };
	dir_map[DIRMAP_SNAPSHOT_DIRECTORY] = dir_data { OPTION_SNAPSHOT_DIRECTORY, 0 };
	dir_map[DIRMAP_DIFF_DIRECTORY] = dir_data { OPTION_DIFF_DIRECTORY, 0 };
	dir_map[DIRMAP_COMMENT_DIRECTORY] = dir_data { OPTION_COMMENT_DIRECTORY, 0 };
	dir_map[DIRMAP_BGFX_PATH] = dir_data { OSDOPTION_BGFX_PATH, 0 };
	dir_map[DIRMAP_HLSLPATH] = dir_data { WINOPTION_HLSLPATH, 0 };
	dir_map[DIRMAP_HISTORY_PATH] = dir_data { OPTION_HISTORY_PATH, 1 };
	dir_map[DIRMAP_CATEGORYINI_PATH] = dir_data { OPTION_CATEGORYINI_PATH, 1 };
	dir_map[DIRMAP_CABINETS_PATH] = dir_data { OPTION_CABINETS_PATH, 1 };
	dir_map[DIRMAP_CPANELS_PATH] = dir_data { OPTION_CPANELS_PATH, 1 };
	dir_map[DIRMAP_PCBS_PATH] = dir_data { OPTION_PCBS_PATH, 1 };
	dir_map[DIRMAP_FLYERS_PATH] = dir_data { OPTION_FLYERS_PATH, 1 };
	dir_map[DIRMAP_TITLES_PATH] = dir_data { OPTION_TITLES_PATH, 1 };
	dir_map[DIRMAP_ENDS_PATH] = dir_data { OPTION_ENDS_PATH, 1 };
	dir_map[DIRMAP_MARQUEES_PATH] = dir_data { OPTION_MARQUEES_PATH, 1 };
	dir_map[DIRMAP_ARTPREV_PATH] = dir_data { OPTION_ARTPREV_PATH, 1 };
	dir_map[DIRMAP_BOSSES_PATH] = dir_data { OPTION_BOSSES_PATH, 1 };
	dir_map[DIRMAP_LOGOS_PATH] = dir_data { OPTION_LOGOS_PATH, 1 };
	dir_map[DIRMAP_SCORES_PATH] = dir_data { OPTION_SCORES_PATH, 1 };
	dir_map[DIRMAP_VERSUS_PATH] = dir_data { OPTION_VERSUS_PATH, 1 };
	dir_map[DIRMAP_GAMEOVER_PATH] = dir_data { OPTION_GAMEOVER_PATH, 1 };
	dir_map[DIRMAP_HOWTO_PATH] = dir_data { OPTION_HOWTO_PATH, 1 };
	dir_map[DIRMAP_SELECT_PATH] = dir_data { OPTION_SELECT_PATH, 1 };
	dir_map[DIRMAP_ICONS_PATH] = dir_data { OPTION_ICONS_PATH, 1 };
	dir_map[DIRMAP_COVER_PATH] = dir_data { OPTION_COVER_PATH, 1 };
	dir_map[DIRMAP_UI_PATH] = dir_data { OPTION_UI_PATH, 1 };
}

void dir_set_value(int dir_index, std::string value)
{
	if (dir_index)
	{
		if (dir_map.count(dir_index) > 0)
		{
			std::string sname = dir_map[dir_index].dir_path;
			int which = dir_map[dir_index].which;
			if (which)
				ui_set_value(emu_ui, sname, value);
			else
				emu_set_value(emu_global, sname, value);
		}
	}
}

std::string dir_get_value(int dir_index)
{
	if (dir_index)
	{
		if (dir_map.count(dir_index) > 0)
		{
			std::string sname = dir_map[dir_index].dir_path;
			int which = dir_map[dir_index].which;
			if (which)
				return ui_get_value(emu_ui, sname);
			else
				return emu_get_value(emu_global, sname);
		}
	}
	return "";
}

// This saves changes to UI.INI only
static void SaveSettingsFile(ui_options &options, const char *filename)
{
	util::core_file::ptr file;

	std::error_condition filerr = util::core_file::open(filename, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS, file);

	if (!filerr)
	{
		std::string inistring = options.output_ini();
		file->puts(&inistring[0]);
		file.reset();
	}
}

void ui_save_ini()
{
	SaveSettingsFile(emu_ui, &UI_FILEPATH[0]);
}

void SetDirectories(windows_options &o)
{
	emu_set_value(o, OPTION_MEDIAPATH, dir_get_value(DIRMAP_MEDIAPATH));
	emu_set_value(o, OPTION_SAMPLEPATH, dir_get_value(DIRMAP_SAMPLEPATH));
	emu_set_value(o, OPTION_INIPATH, dir_get_value(DIRMAP_INIPATH));
	emu_set_value(o, OPTION_CFG_DIRECTORY, dir_get_value(DIRMAP_CFG_DIRECTORY));
	emu_set_value(o, OPTION_SNAPSHOT_DIRECTORY, dir_get_value(DIRMAP_SNAPSHOT_DIRECTORY));
	emu_set_value(o, OPTION_INPUT_DIRECTORY, dir_get_value(DIRMAP_INPUT_DIRECTORY));
	emu_set_value(o, OPTION_STATE_DIRECTORY, dir_get_value(DIRMAP_STATE_DIRECTORY));
	emu_set_value(o, OPTION_ARTPATH, dir_get_value(DIRMAP_ARTPATH));
	emu_set_value(o, OPTION_NVRAM_DIRECTORY, dir_get_value(DIRMAP_NVRAM_DIRECTORY));
	emu_set_value(o, OPTION_CTRLRPATH, dir_get_value(DIRMAP_CTRLRPATH));
	emu_set_value(o, OPTION_CHEATPATH, dir_get_value(DIRMAP_CHEATPATH));
	emu_set_value(o, OPTION_CROSSHAIRPATH, dir_get_value(DIRMAP_CROSSHAIRPATH));
	emu_set_value(o, OPTION_FONTPATH, dir_get_value(DIRMAP_FONTPATH));
	emu_set_value(o, OPTION_DIFF_DIRECTORY, dir_get_value(DIRMAP_DIFF_DIRECTORY));
	emu_set_value(o, OPTION_SNAPNAME, emu_get_value(emu_global, OPTION_SNAPNAME));
	emu_set_value(o, OPTION_DEBUG, "0");
	emu_set_value(o, OPTION_SPEAKER_REPORT, "0");
	emu_set_value(o, OPTION_VERBOSE, "0");
	emu_set_value(o, OPTION_LOG, "0");
	emu_set_value(o, OPTION_OSLOG, "0");
}

// For dialogs.cpp
const char* GetSnapName(void)
{
	return emu_global.value(OPTION_SNAPNAME);
}

void SetSnapName(const char* value)
{
	std::string nvalue = value ? std::string(value) : "";
	emu_set_value(emu_global, OPTION_SNAPNAME, nvalue);
	global_save_ini();
}

// For winui.cpp
const std::string GetLanguageUI(void)
{
	return emu_global.value(OPTION_LANGUAGE);
}

bool GetEnablePlugins(void)
{
	return emu_global.bool_value(OPTION_PLUGINS);
}

const std::string GetPlugins(void)
{
	return emu_global.value(OPTION_PLUGIN);
}

bool GetSkipWarnings(void)
{
	return emu_ui.bool_value(OPTION_SKIP_WARNINGS);
}

void SetSkipWarnings(bool skip)
{
	ui_set_value(emu_ui, OPTION_SKIP_WARNINGS, (skip ? "1" : "0"));
}

void SetSelectedSoftware(int driver_index, std::string opt_name, std::string software)
{
		windows_options options;

	std::cout << "SetSelectedSoftware: opt_name" << opt_name << " software = " << software << std::endl;
	if (opt_name.empty())
	{
		// Software List Item, we write to SOFTWARENAME to ensure all parts of a multipart set are loaded
		std::cout << "About to write " << software << " to OPTION_SOFTWARENAME" << std::endl;
		load_options(options, OPTIONS_GAME, driver_index, 1);
		options.set_value(OPTION_SOFTWARENAME, software, OPTION_PRIORITY_CMDLINE);
		save_options(options, OPTIONS_GAME, driver_index);
	}
	else
	{
		// Loose software, we write the filename to the requested image device
		std::cout << "SetSelectedSoftware(): slot=" << opt_name
			<< " driver=" << driver_list::driver(driver_index).name
			<< " software='" << software << "'" << std::endl;

		std::cout << "About to load " << software << " into slot " << opt_name << std::endl;
		load_options(options, OPTIONS_GAME, driver_index, 1);
		options.set_value(&opt_name[0], &software[0], OPTION_PRIORITY_CMDLINE);
		//options.image_option(opt_name).specify(software);
		std::cout << "Done" << std::endl;
		save_options(options, OPTIONS_GAME, driver_index);
	}
}

// See if this driver has software support
bool DriverHasSoftware(uint32_t drvindex)
{
	if (drvindex < driver_list::total())
	{
		windows_options o;
		load_options(o, OPTIONS_GAME, drvindex, 1);
		machine_config config(driver_list::driver(drvindex), o);

		for (device_image_interface &img : image_interface_enumerator(config.root_device()))
			if (img.user_loadable())
				return 1;
	}

	return 0;
}

void global_save_ini(void)
{
	std::string filename = get_ini_dir_utf8() + PATH_SEPARATOR + std::string(emulator_info::get_configname()) + ".ini";
	SaveSettingsFile(emu_global, &filename[0]);
}

bool AreOptionsEqual(windows_options &opts1, windows_options &opts2)
{
	for (auto &cur_entry : opts1.entries())
	{
		if (cur_entry->type() != core_options::option_type::HEADER)
		{
			const char *value = cur_entry->value();
			const char *comp = opts2.value(&cur_entry->name()[0]);
			if (!value && !comp) // both empty, they are the same
			{}
			else
			if (!value || !comp) // only one empty, they are different
				return false;
			else
			if (mui_strcmp(value, comp) != 0) // both not empty, do proper compare
				return false;
		}
	}
	return true;
}

void OptionsCopy(windows_options &source, windows_options &dest)
{
	for (auto &dest_entry : source.entries())
	{
		if (dest_entry->names().size() > 0)
		{
			// identify the source entry
			const core_options::entry::shared_ptr source_entry = source.get_entry(dest_entry->name());
			if (source_entry)
			{
				const char *value = source_entry->value();
				if (value)
					dest_entry->set_value(value, source_entry->priority(), true);
			}
		}
	}
}

// Reset the given windows_options to their default settings.
static void ResetToDefaults(windows_options &options, int priority)
{
	// iterate through the options setting each one back to the default value.
	windows_options dummy;
	OptionsCopy(dummy, options);
}

void ResetGameOptions(int driver_index)
{
	//save_options(0, OPTIONS_GAME, driver_index);
}

void ResetGameDefaults(void)
{
	// Walk the global settings and reset everything to defaults;
	ResetToDefaults(emu_global, OPTION_PRIORITY_CMDLINE);
	save_options(emu_global, OPTIONS_GLOBAL, GLOBAL_OPTIONS);
}

/*
 * Reset all game, vector and source options to defaults.
 * No reason to reboot if this is done.
 */
void ResetAllGameOptions(void)
{
	for (size_t i = 0; i < driver_list::total(); i++)
		ResetGameOptions(i);
}

windows_options & MameUIGlobal(void)
{
	return emu_global;
}

void SetSystemName(int driver_index)
{
	if (driver_index >= 0)
		mameopts.set_system_name(driver_list::driver(driver_index).name);
}

