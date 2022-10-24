// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

 /***************************************************************************

  emu_opts.cpp

  Interface to MAME's options and ini files.

***************************************************************************/

// standard C++ headers
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string_view>
#include <string>
#include <vector>

// standard windows headers
#include "winapi_common.h"

// MAME headers

// richedit.h pulls in objbase.h which defines interface as a struct which conflict with a template declaration in device.h
#ifdef interface
#undef interface // undef interface which is for COM interfaces
#endif
#include "emu.h"
#ifndef interface
#define interface struct // define interface as struct again
#endif

#include "drivenum.h"
#include "main.h"
#include "path.h"

#include "ui/moptions.h"
#include "winopts.h"

// MAMEUI headers
#include "system_services.h"

#include "mui_cstr.h"
#include "mui_stringtokenizer.h"
#include "mui_wcstr.h"
#include "mui_wcstrconv.h"

#include "emu_opts.h"

using namespace mameui::util::string_util;
using namespace mameui::winapi;
using namespace std::literals;

const std::filesystem::path emu_options_manager::UI_FILEPATH = std::filesystem::path("ini") / "ui.ini";
emu_options_manager emu_opts;

emu_options_manager::emu_options_manager()
{
}

emu_options &emu_options_manager::GetMameOpts()
{
	if (!m_mame_opts)
		m_mame_opts = std::make_unique<emu_options>();

	return *m_mame_opts;
}

ui_options &emu_options_manager::GetUIOpts()
{
	if (!m_ui_opts)
		m_ui_opts = std::make_unique<ui_options>();

	return *m_ui_opts;
}

windows_options &emu_options_manager::GetGlobalOpts()
{
	if (!m_global_opts)
		m_global_opts = std::make_unique<windows_options>();

	return *m_global_opts;
}

std::string emu_options_manager::emu_get_value(windows_options &options, std::string_view option_name)
{
	if (option_name.empty())
		return ""s;
	const char *value = options.value(option_name);
	return value ? value : ""s;
}

std::string emu_options_manager::emu_get_value(windows_options *options, std::string_view option_name)
{
	if (!options || option_name.empty())
		return ""s;
	const char *value = options->value(option_name);
	return value ? value : ""s;
}

void emu_options_manager::emu_set_value(windows_options &options, std::string_view option_name, double option_value)
{
	std::ostringstream ss;
	ss << std::fixed << option_value;
	options.set_value(option_name, ss.str(), OPTION_PRIORITY_HIGH);
}

void emu_options_manager::emu_set_value(windows_options &options, std::string_view option_name, float option_value)
{
	std::ostringstream ss;
	ss << std::fixed << option_value;
	options.set_value(option_name, ss.str(), OPTION_PRIORITY_HIGH);
}

void emu_options_manager::emu_set_value(windows_options &options, std::string_view option_name, int option_value)
{
	std::ostringstream ss;
	ss << option_value;
	std::cout << "Setting option " << option_name << " to value " << ss.str() << "\n";
	options.set_value(option_name, ss.str(), OPTION_PRIORITY_HIGH);
}

void emu_options_manager::emu_set_value(windows_options &options, std::string_view option_name, long option_value)
{
	std::ostringstream ss;
	ss << option_value;
	options.set_value(option_name, ss.str(), OPTION_PRIORITY_HIGH);
}

void emu_options_manager::emu_set_value(windows_options &options, std::string_view option_name, std::string_view option_value)
{
	options.set_value(option_name, option_value, OPTION_PRIORITY_HIGH);
}

void emu_options_manager::emu_set_value(windows_options *options, std::string_view option_name, double option_value)
{
	std::ostringstream ss;
	ss << std::fixed << option_value;
	options->set_value(option_name, ss.str(), OPTION_PRIORITY_HIGH);
}

void emu_options_manager::emu_set_value(windows_options *options, std::string_view option_name, float option_value)
{
	std::ostringstream ss;
	ss << std::fixed << option_value;
	options->set_value(option_name, ss.str(), OPTION_PRIORITY_HIGH);
}

void emu_options_manager::emu_set_value(windows_options *options, std::string_view option_name, int option_value)
{
	std::ostringstream ss;
	ss << option_value;
	options->set_value(option_name, ss.str(), OPTION_PRIORITY_HIGH);
}

void emu_options_manager::emu_set_value(windows_options *options, std::string_view option_name, long option_value)
{
	std::ostringstream ss;
	ss << option_value;
	options->set_value(option_name, ss.str(), OPTION_PRIORITY_HIGH);
}

void emu_options_manager::emu_set_value(windows_options *options, std::string_view option_name, std::string_view option_value)
{
	options->set_value(option_name, option_value, OPTION_PRIORITY_HIGH);
}

std::string emu_options_manager::ui_get_value(ui_options &options, std::string_view option_name)
{
	if (option_name.empty())
		return ""s;
	const char *value = options.value(option_name);
	return value ? value : ""s;
}

std::string emu_options_manager::ui_get_value(ui_options *options, std::string_view option_name)
{
	if (!options || option_name.empty())
		return ""s;
	const char *value = options->value(option_name);
	return value ? value : ""s;
}

void emu_options_manager::ui_set_value(ui_options &options, std::string_view option_name, double option_value)
{
	std::ostringstream ss;
	ss << option_value;
	options.set_value(option_name, ss.str(), OPTION_PRIORITY_HIGH);
}

void emu_options_manager::ui_set_value(ui_options &options, std::string_view option_name, float option_value)
{
	std::ostringstream ss;
	ss << option_value;
	options.set_value(option_name, ss.str(), OPTION_PRIORITY_HIGH);
}

void emu_options_manager::ui_set_value(ui_options &options, std::string_view option_name, int option_value)
{
	std::ostringstream ss;
	ss << option_value;
	options.set_value(option_name, ss.str(), OPTION_PRIORITY_HIGH);
}

void emu_options_manager::ui_set_value(ui_options &options, std::string_view option_name, long option_value)
{
	std::ostringstream ss;
	ss << option_value;
	options.set_value(option_name, ss.str(), OPTION_PRIORITY_HIGH);
}

void emu_options_manager::ui_set_value(ui_options &options, std::string_view option_name, std::string_view option_value)
{
	options.set_value(option_name, option_value, OPTION_PRIORITY_HIGH);
}

void emu_options_manager::ui_set_value(ui_options *options, std::string_view option_name, double option_value)
{
	std::ostringstream ss;
	ss << option_value;
	options->set_value(option_name, ss.str(), OPTION_PRIORITY_HIGH);
}

void emu_options_manager::ui_set_value(ui_options *options, std::string_view option_name, float option_value)
{
	std::ostringstream ss;
	ss << option_value;
	options->set_value(option_name, ss.str(), OPTION_PRIORITY_HIGH);
}

void emu_options_manager::ui_set_value(ui_options *options, std::string_view option_name, int option_value)
{
	std::ostringstream ss;
	ss << option_value;
	options->set_value(option_name, ss.str(), OPTION_PRIORITY_HIGH);
}

void emu_options_manager::ui_set_value(ui_options *options, std::string_view option_name, long option_value)
{
	std::ostringstream ss;
	ss << option_value;
	options->set_value(option_name, ss.str(), OPTION_PRIORITY_HIGH);
}

void emu_options_manager::ui_set_value(ui_options *options, std::string_view option_name, std::string_view option_value)
{
	options->set_value(option_name, option_value, OPTION_PRIORITY_HIGH);
}

std::string emu_options_manager::trim_end_of_path_utf8(std::string_view path)
{
	std::string fixed_path(path);

	while (!path.empty() && (path.back() != '\\' && path.back() != '/'))
		path.remove_suffix(1);

	if(path.empty())
		return fixed_path;

	while (!path.empty() && (path.back() == '\\' || path.back() == '/'))
		path.remove_suffix(1);

	if (!path.empty())
		fixed_path = std::string(path);

	return fixed_path;
}

std::wstring emu_options_manager::trim_end_of_path(std::wstring_view path)
{
	std::wstring fixed_path(path);

	while (!path.empty() && (path.back() != L'\\' && path.back() != L'/'))
		path.remove_suffix(1);

	if (path.empty())
		return fixed_path;

	while (!path.empty() && (path.back() == L'\\' || path.back() == L'/'))
		path.remove_suffix(1);

	if (!path.empty())
		fixed_path = std::wstring(path);

	return fixed_path;
}

std::wstring emu_options_manager::get_ini_dir()
{
	const char *option_value = GetMameOpts().ini_path();

	if (!option_value)
		return get_exe_path();

	stringtokenizer tokenizer(option_value, ";");
	for (const auto &token : tokenizer)
	{
		std::filesystem::path ini_path = token;
		if (std::filesystem::exists(ini_path))
			return trim_end_of_path(ini_path.wstring());
	}

	return get_exe_path();
}

std::string emu_options_manager::get_ini_dir_utf8()
{
	const char *option_value = GetMameOpts().ini_path();

	if (!option_value)
		return get_exe_path_utf8();

	stringtokenizer tokenizer(option_value, ";");
	for (const auto &token : tokenizer)
	{
		std::filesystem::path ini_path = token;
		if (std::filesystem::exists(ini_path))
			return trim_end_of_path_utf8(ini_path.string());
	}

	return get_exe_path_utf8();
}

std::wstring emu_options_manager::get_exe_path()
{
	wchar_t filename[MAX_PATH]{ L'\0' };
	if (system_services::get_module_filename(nullptr, filename, MAX_PATH) == 0)
		return L""s;
	return trim_end_of_path(filename);
}

std::string emu_options_manager::get_exe_path_utf8()
{
	char filename[MAX_PATH]{ '\0' };
	if (system_services::get_module_filename_utf8(nullptr, filename, MAX_PATH) == 0)
		return ""s;
	return trim_end_of_path_utf8(filename);
}

void emu_options_manager::LoadSettingsFile(ui_options &options, std::string filename)
{
	util::core_file::ptr file;
	std::error_condition filerr = util::core_file::open(filename.c_str(), OPEN_FLAG_READ, file);
	if (!filerr)
	{
		if (!file)
			return;

		options.parse_ini_file(*file, OPTION_PRIORITY_HIGH, true, true);
	}
}

void emu_options_manager::LoadSettingsFile(windows_options &options, std::string filename)
{
	util::core_file::ptr file;
	std::error_condition filerr = util::core_file::open(filename.c_str(), OPEN_FLAG_READ, file);
	if (!filerr)
	{
		if (!file)
			return;

		options.parse_ini_file(*file, OPTION_PRIORITY_HIGH, true, true);
	}
}

void emu_options_manager::SaveSettingsFile(windows_options &options, std::string filename)
{
	util::core_file::ptr file;
	std::error_condition filerr = util::core_file::open(filename.c_str(), OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS, file);
	if (!filerr)
	{
		if (!file)
			return;

		std::string inistring = options.output_ini();
		file->puts(inistring.c_str());
	}
	else
		std::cerr << __FILE__ << ": " << __LINE__ << ": Failed to open file: " << filename << " in " << __FUNCTION__ << "\n";

}

void emu_options_manager::SaveSettingsFile(ui_options &options, std::string filename)
{
	util::core_file::ptr file;
	std::error_condition filerr = util::core_file::open(filename.c_str(), OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS, file);
	if (!filerr)
	{
		if (!file)
			return;

		std::string inistring = options.output_ini();
		file->puts(inistring.c_str());
	}
	else
		std::cerr << __FILE__ << ": " << __LINE__ << ": Failed to open file: " << filename << " in " << __FUNCTION__ << "\n";
}

void emu_options_manager::load_options(windows_options &options, SOFTWARETYPE_OPTIONS software_type, int driver_index, bool set_system_name)
{

	if (software_type < 0 || software_type >= TOTAL_SOFTWARETYPE_OPTIONS)
	{
		std::cerr << __FILE__ << ": " << __LINE__ << ": Unknown software type " << software_type << " in " << __FUNCTION__ << "\n";
		return;
	}

	if (driver_index < 0 || driver_index >= driver_list::total())
	{
		std::cerr << __FILE__ << ": " << __LINE__ << ": Invalid driver index " << driver_index << " in " << __FUNCTION__ << "\n";
		return;
	}

	std::filesystem::path ini_directory = get_ini_dir_utf8(), load_inifile_path;

	// Always load global settings first
	load_inifile_path = ini_directory / (std::string(emulator_info::get_configname()) + ".ini");
		LoadSettingsFile(options, load_inifile_path.string());

	if (software_type == SOFTWARETYPE_GLOBAL)
		return;

	const game_driver *driver = &driver_list::driver(driver_index);

	switch (software_type)
	{
	case SOFTWARETYPE_ARCADE: load_inifile_path = ini_directory / "arcade.ini";
		LoadSettingsFile(options, load_inifile_path.string());
		break;
	case SOFTWARETYPE_COMPUTER: load_inifile_path = ini_directory / "computer.ini";
		LoadSettingsFile(options, load_inifile_path.string());
		break;
	case SOFTWARETYPE_CONSOLE: load_inifile_path = ini_directory / "console.ini";
		LoadSettingsFile(options, load_inifile_path.string());
		break;
	case SOFTWARETYPE_GAME:
		if (driver)
		{
			const char* driver_name = driver->name;
			if (driver_name && *driver_name)
			{
				load_inifile_path = ini_directory / (std::string(driver_name) + ".ini");
				if (set_system_name)
					options.set_value(OPTION_SYSTEMNAME, driver_name, OPTION_PRIORITY_HIGH);

				LoadSettingsFile(options, load_inifile_path.string());
			}
			SetDirectories(options);
		}
		break;
	case SOFTWARETYPE_HORIZONTAL: load_inifile_path = ini_directory / "horizontal.ini";
		LoadSettingsFile(options, load_inifile_path.string());
		break;
	case SOFTWARETYPE_PARENT:
	{
		int parent_index = driver_list::non_bios_clone(driver_index);
		if (parent_index >= 0 && parent_index < driver_list::total())
		{
			const game_driver* parent_driver = &driver_list::driver(parent_index);
			if (parent_driver)
			{
				const char* parent_name = parent_driver->name;
				if (parent_name && *parent_name)
				{
					load_inifile_path = ini_directory / (std::string(parent_name) + ".ini");
					if (set_system_name)
						options.set_value(OPTION_SYSTEMNAME, parent_name, OPTION_PRIORITY_HIGH);
					LoadSettingsFile(options, load_inifile_path.string());
				}
			}
		}
		break;
	}
	case SOFTWARETYPE_COMPAT:
	{
		int compat_index = driver_list::compatible_with(driver_index);
		if (compat_index >= 0 && compat_index < driver_list::total())
		{
			const game_driver* compat_driver = &driver_list::driver(compat_index);
			if (compat_driver)
			{
				const char* compat_name = compat_driver->name;
				if (compat_name && *compat_name)
				{
					load_inifile_path = ini_directory / (std::string(compat_name) + ".ini");
					if (set_system_name)
						options.set_value(OPTION_SYSTEMNAME, compat_name, OPTION_PRIORITY_HIGH);
					LoadSettingsFile(options, load_inifile_path.string());
				}
			}
		}
		break;
	}
	case SOFTWARETYPE_RASTER: load_inifile_path = ini_directory / "raster.ini";
		LoadSettingsFile(options, load_inifile_path.string());
		break;
	case SOFTWARETYPE_SOURCE:
		if (driver)
		{
			const char* type_source = driver->type.source();
			if (type_source && *type_source)
			{
				load_inifile_path = ini_directory / "source" / (std::filesystem::path(type_source).stem().concat(".ini"));
				LoadSettingsFile(options, load_inifile_path.string());
			}
		}
		break;
	case SOFTWARETYPE_VECTOR: load_inifile_path = ini_directory / "vector.ini";
		LoadSettingsFile(options, load_inifile_path.string());
		break;
	case SOFTWARETYPE_VERTICAL: load_inifile_path = ini_directory / "vertical.ini";
		LoadSettingsFile(options, load_inifile_path.string());
		break;

	// Note: SOFTWARETYPE_LCD and SOFTWARETYPE_OTHERSYS are not loaded by this function.
	// coverity[unreachable] - Suppress "unreachable code" warning by Coverity
	default:
		std::cerr << __FILE__ << ": " << __LINE__ << ": Unhandled software type " << software_type << " in " << __FUNCTION__ << "\n";
		break;
	}
}

void emu_options_manager::save_options(windows_options& options, SOFTWARETYPE_OPTIONS software_type, int driver_index)
{

	if (software_type < 0 || software_type >= TOTAL_SOFTWARETYPE_OPTIONS)
	{
		std::cerr << __FILE__ << ": " << __LINE__ << ": Unknown software type " << software_type << " in " << __FUNCTION__ << "\n";
		return;
	}

	if (driver_index < 0 || driver_index >= driver_list::total())
	{
		std::cerr << __FILE__ << ": " << __LINE__ << ": Invalid driver index " << driver_index << " in " << __FUNCTION__ << "\n";
		return;
	}

	std::filesystem::path ini_directory = get_ini_dir_utf8(), save_inifile_path;
	const game_driver *driver = &driver_list::driver(driver_index);

	switch (software_type)
	{
	case SOFTWARETYPE_ARCADE: save_inifile_path = ini_directory / "arcade.ini";
		SaveSettingsFile(options, save_inifile_path.string());
		break;
	case SOFTWARETYPE_COMPUTER: save_inifile_path = ini_directory / "computer.ini";
		SaveSettingsFile(options, save_inifile_path.string());
		break;
	case SOFTWARETYPE_CONSOLE: save_inifile_path = ini_directory / "console.ini";
		SaveSettingsFile(options, save_inifile_path.string());
		break;
	case SOFTWARETYPE_GAME:
		if (driver)
		{
			const char *driver_name = driver->name;
			if (driver_name && *driver_name)
			{
				save_inifile_path = ini_directory / (std::string(driver_name) + ".ini");
				SetDirectories(options);
				SaveSettingsFile(options, save_inifile_path.string());
			}
		}
		break;
	case SOFTWARETYPE_PARENT:
	{
		int parent_index = driver_list::non_bios_clone(driver_index);
		if (parent_index >= 0 && parent_index < driver_list::total())
		{
			const game_driver* parent_driver = &driver_list::driver(parent_index);
			if (parent_driver)
			{
				const char* parent_name = parent_driver->name;
				if (parent_name && *parent_name)
				{
					save_inifile_path = ini_directory / (std::string(parent_name) + ".ini");
					SaveSettingsFile(options, save_inifile_path.string());
				}
			}
		}
		break;
	}
	case SOFTWARETYPE_COMPAT:
	{
		int compat_index = driver_list::compatible_with(driver_index);
		if (compat_index >= 0 && compat_index < driver_list::total())
		{
			const game_driver* compat_driver = &driver_list::driver(compat_index);
			if (compat_driver)
			{
				const char* compat_name = compat_driver->name;
				if (compat_name && *compat_name)
				{
					save_inifile_path = ini_directory / (std::string(compat_name) + ".ini");
					SaveSettingsFile(options, save_inifile_path.string());
				}
			}
		}
		break;
	}
	case SOFTWARETYPE_GLOBAL: save_inifile_path = ini_directory / (std::string(emulator_info::get_configname()) + ".ini");
		SaveSettingsFile(options, save_inifile_path.string());
		break;
	case SOFTWARETYPE_HORIZONTAL: save_inifile_path = ini_directory / "horizontal.ini";
		SaveSettingsFile(options, save_inifile_path.string());
		break;
	case SOFTWARETYPE_RASTER: save_inifile_path = ini_directory / "raster.ini";
		SaveSettingsFile(options, save_inifile_path.string());
		break;
	case SOFTWARETYPE_SOURCE:
		if (driver)
		{
			const char *type_source = driver->type.source();
			if (type_source && *type_source)
			{
				save_inifile_path = ini_directory / "source" / (std::filesystem::path(type_source).stem().concat(".ini"));
				SaveSettingsFile(options, save_inifile_path.string());
			}
		}
		break;
	case SOFTWARETYPE_VECTOR: save_inifile_path = ini_directory / "vector.ini";
		SaveSettingsFile(options, save_inifile_path.string());
		break;
	case  SOFTWARETYPE_VERTICAL: save_inifile_path = ini_directory / "vertical.ini";
		SaveSettingsFile(options, save_inifile_path.string());
		break;

	// Note: SOFTWARETYPE_LCD and SOFTWARETYPE_OTHERSYS are not saved by this function.
	// coverity[unreachable] - Suppress "unreachable code" warning by Coverity
	default:
		std::cerr << __FILE__ << ": " << __LINE__ << ": Unhandled software type " << software_type << " in " << __FUNCTION__ << "\n";
		break;
	}
}


void emu_options_manager::emu_opts_init(bool b)
{
	if (!b)
	{
		m_dir_map[DIRPATH_PLUGINDATAPATH] = dir_data{ OPTION_PLUGINDATAPATH, 0 };
		m_dir_map[DIRPATH_MEDIAPATH] = dir_data{ OPTION_MEDIAPATH, 0 };
		m_dir_map[DIRPATH_HASHPATH] = dir_data{ OPTION_HASHPATH, 0 };
		m_dir_map[DIRPATH_SAMPLEPATH] = dir_data{ OPTION_SAMPLEPATH, 0 };
		m_dir_map[DIRPATH_ARTPATH] = dir_data{ OPTION_ARTPATH, 0 };
		m_dir_map[DIRPATH_CTRLRPATH] = dir_data{ OPTION_CTRLRPATH, 0 };
		m_dir_map[DIRPATH_INIPATH] = dir_data{ OPTION_INIPATH, 0 };
		m_dir_map[DIRPATH_FONTPATH] = dir_data{ OPTION_FONTPATH, 0 };
		m_dir_map[DIRPATH_CHEATPATH] = dir_data{ OPTION_CHEATPATH, 0 };
		m_dir_map[DIRPATH_CROSSHAIRPATH] = dir_data{ OPTION_CROSSHAIRPATH, 0 };
		m_dir_map[DIRPATH_PLUGINSPATH] = dir_data{ OPTION_PLUGINSPATH, 0 };
		m_dir_map[DIRPATH_LANGUAGEPATH] = dir_data{ OPTION_LANGUAGEPATH, 0 };
		m_dir_map[DIRPATH_SWPATH] = dir_data{ OPTION_SWPATH, 0 };
		m_dir_map[DIRPATH_CFG_DIRECTORY] = dir_data{ OPTION_CFG_DIRECTORY, 0 };
		m_dir_map[DIRPATH_NVRAM_DIRECTORY] = dir_data{ OPTION_NVRAM_DIRECTORY, 0 };
		m_dir_map[DIRPATH_INPUT_DIRECTORY] = dir_data{ OPTION_INPUT_DIRECTORY, 0 };
		m_dir_map[DIRPATH_STATE_DIRECTORY] = dir_data{ OPTION_STATE_DIRECTORY, 0 };
		m_dir_map[DIRPATH_SNAPSHOT_DIRECTORY] = dir_data{ OPTION_SNAPSHOT_DIRECTORY, 0 };
		m_dir_map[DIRPATH_DIFF_DIRECTORY] = dir_data{ OPTION_DIFF_DIRECTORY, 0 };
		m_dir_map[DIRPATH_COMMENT_DIRECTORY] = dir_data{ OPTION_COMMENT_DIRECTORY, 0 };
		m_dir_map[DIRPATH_BGFX_PATH] = dir_data{ OSDOPTION_BGFX_PATH, 0 };
		m_dir_map[DIRPATH_HLSLPATH] = dir_data{ WINOPTION_HLSLPATH, 0 };
		m_dir_map[DIRPATH_HISTORY_PATH] = dir_data{ OPTION_HISTORY_PATH, 1 };
		m_dir_map[DIRPATH_CATEGORYINI_PATH] = dir_data{ OPTION_CATEGORYINI_PATH, 1 };
		m_dir_map[DIRPATH_CABINETS_PATH] = dir_data{ OPTION_CABINETS_PATH, 1 };
		m_dir_map[DIRPATH_CPANELS_PATH] = dir_data{ OPTION_CPANELS_PATH, 1 };
		m_dir_map[DIRPATH_PCBS_PATH] = dir_data{ OPTION_PCBS_PATH, 1 };
		m_dir_map[DIRPATH_FLYERS_PATH] = dir_data{ OPTION_FLYERS_PATH, 1 };
		m_dir_map[DIRPATH_TITLES_PATH] = dir_data{ OPTION_TITLES_PATH, 1 };
		m_dir_map[DIRPATH_ENDS_PATH] = dir_data{ OPTION_ENDS_PATH, 1 };
		m_dir_map[DIRPATH_MARQUEES_PATH] = dir_data{ OPTION_MARQUEES_PATH, 1 };
		m_dir_map[DIRPATH_ARTPREV_PATH] = dir_data{ OPTION_ARTPREV_PATH, 1 };
		m_dir_map[DIRPATH_BOSSES_PATH] = dir_data{ OPTION_BOSSES_PATH, 1 };
		m_dir_map[DIRPATH_LOGOS_PATH] = dir_data{ OPTION_LOGOS_PATH, 1 };
		m_dir_map[DIRPATH_SCORES_PATH] = dir_data{ OPTION_SCORES_PATH, 1 };
		m_dir_map[DIRPATH_VERSUS_PATH] = dir_data{ OPTION_VERSUS_PATH, 1 };
		m_dir_map[DIRPATH_GAMEOVER_PATH] = dir_data{ OPTION_GAMEOVER_PATH, 1 };
		m_dir_map[DIRPATH_HOWTO_PATH] = dir_data{ OPTION_HOWTO_PATH, 1 };
		m_dir_map[DIRPATH_SELECT_PATH] = dir_data{ OPTION_SELECT_PATH, 1 };
		m_dir_map[DIRPATH_ICONS_PATH] = dir_data{ OPTION_ICONS_PATH, 1 };
		m_dir_map[DIRPATH_COVER_PATH] = dir_data{ OPTION_COVER_PATH, 1 };
		m_dir_map[DIRPATH_UI_PATH] = dir_data{ OPTION_UI_PATH, 1 };
	}

	std::cout << "emuOptsInit: About to load " << UI_FILEPATH.string() << "\n";
	LoadSettingsFile(GetUIOpts(), UI_FILEPATH.string());                // parse UI.INI
	std::cout << "emuOptsInit: About to load Global Options" << "\n";
	load_options(GetGlobalOpts(), SOFTWARETYPE_GLOBAL, 0, false);   // parse MAME.INI
	std::cout << "emuOptsInit: Finished" << "\n";

}

void emu_options_manager::dir_set_value(size_t dir_index, std::string value)
{
	if (dir_index)
	{
		if (m_dir_map.count(dir_index) > 0)
		{
			if (m_dir_map[dir_index].which)
				ui_set_value(GetUIOpts(), m_dir_map[dir_index].dir_path, value);
			else
				emu_set_value(GetGlobalOpts(), m_dir_map[dir_index].dir_path, value);
		}
	}
}

std::string emu_options_manager::dir_get_value(size_t dir_index)
{
	if (dir_index)
	{
		if (m_dir_map.count(dir_index) > 0)
		{
			if (m_dir_map[dir_index].which)
				return ui_get_value(GetUIOpts(), m_dir_map[dir_index].dir_path);
			else
				return emu_get_value(GetGlobalOpts(), m_dir_map[dir_index].dir_path);
		}
	}
	return ""s;
}

void emu_options_manager::ui_save_ini()
{
	SaveSettingsFile(GetUIOpts(), UI_FILEPATH.string());
}

void emu_options_manager::SetDirectories(windows_options &options)
{
	emu_set_value(options, OPTION_PLUGINDATAPATH, dir_get_value(DIRPATH_PLUGINDATAPATH));
	emu_set_value(options, OPTION_HASHPATH, dir_get_value(DIRPATH_HASHPATH));
	emu_set_value(options, OPTION_MEDIAPATH, dir_get_value(DIRPATH_MEDIAPATH));
	emu_set_value(options, OPTION_SAMPLEPATH, dir_get_value(DIRPATH_SAMPLEPATH));
	emu_set_value(options, OPTION_INIPATH, dir_get_value(DIRPATH_INIPATH));
	emu_set_value(options, OPTION_CFG_DIRECTORY, dir_get_value(DIRPATH_CFG_DIRECTORY));
	emu_set_value(options, OPTION_SNAPSHOT_DIRECTORY, dir_get_value(DIRPATH_SNAPSHOT_DIRECTORY));
	emu_set_value(options, OPTION_INPUT_DIRECTORY, dir_get_value(DIRPATH_INPUT_DIRECTORY));
	emu_set_value(options, OPTION_STATE_DIRECTORY, dir_get_value(DIRPATH_STATE_DIRECTORY));
	emu_set_value(options, OPTION_ARTPATH, dir_get_value(DIRPATH_ARTPATH));
	emu_set_value(options, OPTION_NVRAM_DIRECTORY, dir_get_value(DIRPATH_NVRAM_DIRECTORY));
	emu_set_value(options, OPTION_CTRLRPATH, dir_get_value(DIRPATH_CTRLRPATH));
	emu_set_value(options, OPTION_CHEATPATH, dir_get_value(DIRPATH_CHEATPATH));
	emu_set_value(options, OPTION_CROSSHAIRPATH, dir_get_value(DIRPATH_CROSSHAIRPATH));
	emu_set_value(options, OPTION_FONTPATH, dir_get_value(DIRPATH_FONTPATH));
	emu_set_value(options, OPTION_DIFF_DIRECTORY, dir_get_value(DIRPATH_DIFF_DIRECTORY));
	emu_set_value(options, OPTION_SNAPNAME, emu_get_value(GetGlobalOpts(), OPTION_SNAPNAME));
	emu_set_value(options, OPTION_DEBUG, "0");
	emu_set_value(options, OPTION_VERBOSE, "0");
	emu_set_value(options, OPTION_LOG, "0");
	emu_set_value(options, OPTION_OSLOG, "0");
}

std::string emu_options_manager::GetSnapName()
{
	const char *option_value = GetGlobalOpts().value(OPTION_SNAPNAME);
	return option_value ? option_value : "";
}

void emu_options_manager::SetSnapName(std::string_view name)
{
	emu_set_value(GetGlobalOpts(), OPTION_SNAPNAME, name);
	global_save_ini();
}

const std::string emu_options_manager::GetLanguageUI()
{
	return GetGlobalOpts().value(OPTION_LANGUAGE);
}

bool emu_options_manager::GetEnablePlugins()
{
	return GetGlobalOpts().bool_value(OPTION_PLUGINS);
}

const std::string emu_options_manager::GetPlugins()
{
	return GetGlobalOpts().value(OPTION_PLUGIN);
}

const std::string emu_options_manager::GetPluginDataPath()
{
	return dir_get_value(DIRPATH_PLUGINDATAPATH);
}

bool emu_options_manager::GetSkipWarnings()
{
	return GetUIOpts().bool_value(OPTION_SKIP_WARNINGS);
}

void emu_options_manager::SetSkipWarnings(bool skip)
{
	ui_set_value(GetUIOpts(), OPTION_SKIP_WARNINGS, (skip ? "1" : "0"));
}

void emu_options_manager::SetSelectedSoftware(int driver_index, std::string opt_name, std::string software)
{
	if (driver_index < 0 || driver_index >= driver_list::total())
	{
		std::cerr << __FILE__ << ": " << __LINE__ << ": Invalid driver index " << driver_index << " in " << __FUNCTION__ << "\n";
		return;
	}

	windows_options options;

	if (opt_name.empty())
	{
		// Software List Item, we write to SOFTWARENAME to ensure all parts of a multipart set are loaded
		std::cout << "About to write " << software << " to OPTION_SOFTWARENAME" << "\n";
		load_options(options, SOFTWARETYPE_GAME, driver_index, true);
		options.set_value(OPTION_SOFTWARENAME, software, OPTION_PRIORITY_HIGH);
		save_options(options, SOFTWARETYPE_GAME, driver_index);
	}
	else
	{
		// Loose software, we write the filename to the requested image device
		std::cout << __FUNCTION__ << ": slot=" << opt_name
			<< " driver=" << driver_list::driver(driver_index).name
			<< " software='" << software << "'" << "\n";

		std::cout << "About to load " << software << " into slot " << opt_name << "\n";
		load_options(options, SOFTWARETYPE_GAME, driver_index, true);
		options.set_value(opt_name.c_str(), software.c_str(), OPTION_PRIORITY_HIGH);
		//options.image_option(opt_name).specify(software);
		std::cout << "Done" << "\n";
		save_options(options, SOFTWARETYPE_GAME, driver_index);
	}
}

bool emu_options_manager::DriverHasSoftware(int driver_index)
{
	if (driver_index < driver_list::total())
	{
		windows_options o;
		load_options(o, SOFTWARETYPE_GAME, driver_index, true);
		machine_config config(driver_list::driver(driver_index), o);

		for (device_image_interface &img : image_interface_enumerator(config.root_device()))
			if (img.user_loadable())
				return true;
	}

	return false;
}

void emu_options_manager::global_save_ini()
{
	std::string filename = std::string(emulator_info::get_configname()) + ".ini";
	std::filesystem::path save_inifile_path = get_ini_dir_utf8() / std::filesystem::path(filename);
	SaveSettingsFile(GetGlobalOpts(), save_inifile_path.string());
}

bool emu_options_manager::AreOptionsEqual(windows_options &options1, windows_options &options2)
{
	for (const auto &cur_entry : options1.entries())
	{
		if (cur_entry->type() == core_options::option_type::HEADER)
			continue; // skip headers

		const char *opt1_cstr = cur_entry->value();
		const char *opt2_cstr = options2.value(cur_entry->name().c_str());

		std::string_view opt1_value = opt1_cstr ? opt1_cstr : "";
		std::string_view opt2_value = opt2_cstr ? opt2_cstr : "";
		if (opt1_value != opt2_value) // do proper compare
			return false;
	}
	return true;
}

void emu_options_manager::OptionsCopy(windows_options &source, windows_options &dest)
{
	for (const auto &source_entry : source.entries())
	{
		if (!source_entry->names().empty()) // check if there are names
		{
			// point to the source entry
			const char *value = source_entry->value();
			if (value)
			{
				// copy the value to the destination with the same priority
				auto dest_entry = dest.get_entry(source_entry->name());
				if (dest_entry)
				{
					dest_entry->set_value(value, source_entry->priority(), true);
				}
			}
		}
	}
}

void emu_options_manager::ResetToDefaults(windows_options &options, int priority)
{
	windows_options dummy;
	OptionsCopy(dummy, options);
}

void emu_options_manager::ResetGameDefaults()
{
	ResetToDefaults(GetGlobalOpts(), OPTION_PRIORITY_HIGH);
	save_options(GetGlobalOpts(), SOFTWARETYPE_GLOBAL, 0);
}

void emu_options_manager::SetSystemName(int driver_index)
{
	if (driver_index >= 0)
		GetMameOpts().set_system_name(driver_list::driver(driver_index).name);
}
