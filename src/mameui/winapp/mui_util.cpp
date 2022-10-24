// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

/***************************************************************************

  mui_util.cpp

 ***************************************************************************/

// standard C++ headers
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

// standard windows headers
#include "winapi_common.h"

// MAME headers
#include "emu.h"

#include "drivenum.h"
#include "machine/ram.h"
#include "path.h"
#include "romload.h"
#include "screen.h"
#include "sound/samples.h"
#include "speaker.h"
#include "unzip.h"

#include "ui/info.h"
#include "ui/moptions.h"
#include "winopts.h"

//MAMEUI headers
#include "dialog_boxes.h"
#include "windows_gdi.h"
#include "windows_input.h"
#include "system_services.h"
#include "windows_shell.h"
#include "windows_messages.h"

#include "mui_cstr.h"
#include "mui_stringtokenizer.h"
#include "mui_wcstr.h"
#include "mui_wcstrconv.h"

#include "bitmask.h"
#include "emu_opts.h"
#include "game_opts.h"
#include "mui_opts.h"
#include "screenshot.h"
#include "winui.h"

#include "mui_util.h"

using namespace mameui::util::string_util;
using namespace mameui::winapi;

/***************************************************************************
    function prototypes
 ***************************************************************************/
static void SetDriversInfo(void);
static void InitDriversInfo(void);
static void InitDriversCache(void);

/***************************************************************************
    External variables
 ***************************************************************************/

/***************************************************************************
    Internal structures
 ***************************************************************************/
using DriverInfo = struct driver_info_t
{
	// most of these are saved in the upper cache
	int screenCount = 0;
	int speakerCount = 0;
	bool hasHarddisk = false;
	bool hasOptionalBIOS = false;
//	bool hasRam = false; // Commit 2d0a09b removed the ability to configure ":ram" from the command line & not having a UI
	bool isBroken = false; // value comes from lower cache
	bool isClone = false;
	bool isStereo = false; // derived from speaker count
	bool isVertical = false; // from lower cache too
	bool supportsSaveState = false; // from lower cache too
	bool usesLightGun = false;
	bool usesMouse = false;
	bool usesMultiChannelAudio = false; // derived from speaker count
	bool usesRoms = false;
	bool usesSamples = false;
	bool usesTrackball = false;
	bool usesVectorGraphics = false;
};

static std::vector<DriverInfo> drivers_info;

struct upper_cache_flags
{
	enum type : uint32_t
	{
		SCREENCOUNT = 15u << 0,   // bits 0-3: screen count
		SPEAKERCOUNT = 15u << 4,  // bit 4-8: speaker count
		HASHDD = 1u << 9,         // bit 9: has a hard disk
		HASOPTBIOS = 1u << 10,    // bit 10: has optional BIOS
//		HASRAM = 1u << 11,        // bit 11: has RAM // Commit 2d0a09b removed the ability to configure ":ram" from the command line & not having a UI
		ISCLONE = 1u << 12,       // bit 12, is a clone
		USESLIGHTGUN = 1u << 13,  // bit 13: lightgun used
		USESMOUSE = 1u << 14,     // bit 14: mouse used
		USESROMS = 1u << 15,      // bit 15: uses ROM chips
		USESSAMPLES = 1u << 16,   // bit 16: uses samples
		USESTRACKBALL = 1u << 17, // bit 17: uses a trackball
		USESVECTORGFX = 1u << 18, // bit 18: uses vector graphics
	};
};

static bool first_time = true;
/***************************************************************************
    External functions
 ***************************************************************************/

/*
    ErrorMsg
*/
void __cdecl ErrorMessageBox(const char *fmt, ...)
{
	int buffer_len;
	std::unique_ptr<char[]> buffer;
	va_list v_args;

	va_start(v_args, fmt);
	buffer_len = _vsnprintf(0, 0, fmt, v_args);
	va_end(v_args);
	if (buffer_len < 0)
		return;

	buffer = std::make_unique<char[]>(buffer_len + 1);
	va_start(v_args, fmt);
	buffer_len = _vsnprintf(buffer.get(), buffer_len + 1, fmt, v_args);
	va_end(v_args);
	if (buffer_len < 0)
		return;

	std::unique_ptr<wchar_t[]> wcs_buffer(mui_utf16_from_utf8cstring(buffer.get()));
	dialog_boxes::message_box(input::get_active_window(), wcs_buffer.get(), &MAMEUINAME[0], MB_OK | MB_ICONERROR);
	std::wcout << MAMEUINAME<<": " << wcs_buffer.get() << "\n";

	std::ofstream outfile("debug.txt", std::ofstream::out | std::ofstream::app);
	if (outfile.is_open())
	{
		outfile.write(buffer.get(), buffer_len);
		outfile.put('\n');
		outfile.close();
	}

}

void __cdecl dprintf(const char *fmt, ...)
{
	int buffer_len;
	std::unique_ptr<char[]> buf;
	va_list v_args = 0;

	va_start(v_args, fmt);
	buffer_len = _vsnprintf(0, 0, fmt, v_args);
	va_end(v_args);
	if (buffer_len < 0)
		return;

	buf = std::make_unique<char[]>(buffer_len + 1);

	va_start(v_args, fmt);
	buffer_len = _vsnprintf(buf.get(), buffer_len + 1, fmt, v_args);
	va_end(v_args);
	if (buffer_len < 0)
		return;

	system_services::output_debug_string_utf8(buf.get());
}

void __cdecl dprintf(const wchar_t* fmt, ...)
{
	va_list v_args = 0;

	va_start(v_args, fmt);
	int buffer_len = _vsnwprintf(nullptr, 0, fmt, v_args);
	va_end(v_args);

	if (buffer_len < 0)
		return;

	auto buffer = std::make_unique<wchar_t[]>(buffer_len + 1);

	va_start(v_args, fmt);
	buffer_len = _vsnwprintf(buffer.get(), buffer_len + 1, fmt, v_args);
	va_end(v_args);
	if (buffer_len < 0)
		return;

	system_services::output_debug_string(buffer.get());
}

void ErrorMessageBox(const wchar_t* fmt, ...)
{
	va_list v_args = 0;

	va_start(v_args, fmt);
	int buffer_len = _vsnwprintf(nullptr, 0, fmt, v_args);
	va_end(v_args);

	if (buffer_len < 0)
		return;

	auto buffer = std::make_unique<wchar_t[]>(buffer_len + 1);

	va_start(v_args, fmt);
	buffer_len = _vsnwprintf(buffer.get(), buffer_len + 1, fmt, v_args);
	va_end(v_args);
	if (buffer_len < 0)
		return;

	dialog_boxes::message_box(input::get_active_window(), buffer.get(), std::wstring(MAMEUINAME).c_str(), MB_OK | MB_ICONERROR);
}

void ShellExecuteCommon(HWND hWnd, std::wstring cName)
{
	if(cName.empty())
		return;

	HINSTANCE hErr = shell::shell_execute(hWnd, nullptr, cName.c_str(), nullptr, nullptr, SW_SHOWNORMAL);

	if ((uintptr_t)hErr > 32)
		return;

	std::wstring err_msg = last_system_function_error_message();
	ErrorMessageBox(L"%s\r\nPath: '%s'", err_msg.c_str(), cName.c_str());
}

UINT GetDepth(HWND hWnd)
{
	UINT nBPP;
	HDC hDC;

	hDC = gdi::get_dc(hWnd);

	nBPP = gdi::get_device_caps(hDC, BITSPIXEL) *gdi::get_device_caps(hDC, PLANES);

	(void)gdi::release_dc(hWnd, hDC);

	return nBPP;
}

std::wstring last_system_function_error_message()
{
	wchar_t *message_buffer = nullptr;
	DWORD error_code = system_services::get_last_error();

	DWORD result = windows::format_message(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&message_buffer), 0, nullptr);
	std::wstring error_description = (!result || !message_buffer) ? L"FormatMessage failed." : message_buffer;
	if (message_buffer)
		system_services::local_free(message_buffer);

	while (!error_description.empty() && isspace(static_cast<unsigned char>(error_description.back())))
		error_description.pop_back();

	return L"Error code " + std::to_wstring(error_code) + L": " + error_description;
}

std::string last_system_function_error_message_utf8()
{
	char *message_buffer = nullptr;
	DWORD error_code = system_services::get_last_error();

	DWORD result = windows::format_message_utf8(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&message_buffer), 0, nullptr);
	std::string error_description = (!result || !message_buffer) ? "FormatMessage failed." : message_buffer;
	if (message_buffer)
		system_services::local_free(message_buffer);

	while (!error_description.empty() && isspace(static_cast<unsigned char>(error_description.back())))
		error_description.pop_back();

	return "Error code " + std::to_string(error_code) + ": " + error_description;
}

void DisplayTextFile(HWND hWnd, std::wstring cName)
{
	if (cName.empty())
		return;

	HINSTANCE hErr = shell::shell_execute(hWnd, nullptr, cName.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	if ((uintptr_t)hErr > 32)
		return;

	std::wstring err_msg = last_system_function_error_message();
	dialog_boxes::message_box(nullptr, err_msg.c_str(), cName.c_str(), MB_OK);
}

std::string ConvertToWindowsNewlines(std::string_view source)
{
	std::string converted_lines;

	for(auto character : source)
	{
		if (character == '\n')
			converted_lines += "\r\n";
		else
			converted_lines += character;
	}
	return converted_lines;
}

// Lop off path and extention from a source file name
// This assumes their is a pathname passed to the function
// like src\drivers\blah.c

std::wstring GetDriverFilename(std::size_t driver_index)
{
	assert(driver_index < driver_list::total());
	std::filesystem::path driver_filename = driver_list::driver(driver_index).type.source();
	return driver_filename.filename().wstring();
}

std::string GetDriverFilename_utf8(std::size_t driver_index)
{
	assert(driver_index < driver_list::total());
	const std::filesystem::path driver_filename = driver_list::driver(driver_index).type.source();
	return driver_filename.filename().string();
}

bool HasOptionalBios(const game_driver *game)
{
	if (!game || !game->rom)
		return false;

	std::vector<rom_entry> rom_entries = rom_build_entries(game->rom);
	for (const rom_entry *rom = rom_entries.data(); rom && !ROMENTRY_ISEND(rom); rom++)
		if (ROMENTRY_ISSYSTEM_BIOS(rom) && ROM_ISOPTIONAL(rom))
			return true;

	return false;
}

bool UsesLightGun(const game_driver *game)
{
	if (!game || !game->ipt)
		return false;

	ioport_list portlist;
	std::ostringstream errors;

	// Enumerate all devices in the machine configuration and collect their input ports
	machine_config config(*game, emu_opts.GetGlobalOpts());
	for (device_t& dev : device_enumerator(config.root_device()))
		if (dev.input_ports())
			portlist.append(dev, errors);

	// Now check each port and its fields for trackball types
	for (auto& port : portlist)
	{
		for (ioport_field& field : port.second->fields())
		{
			std::uint32_t type = field.type(); // ioport_type is a typedef for std::uint32_t
			if (type == IPT_END)
				break; // End of fields for this port

			if (type == IPT_LIGHTGUN_X || type == IPT_LIGHTGUN_Y)
				return true;
		}
	}

	return false;
}

bool UsesMouse(const game_driver *game)
{
	if (!game || !game->ipt)
		return false;

	ioport_list portlist;
	std::ostringstream errors;

	// Enumerate all devices in the machine configuration and collect their input ports
	machine_config config(*game, emu_opts.GetGlobalOpts());
	for (device_t& dev : device_enumerator(config.root_device()))
		if (dev.input_ports())
			portlist.append(dev, errors);

	// Now check each port and its fields for trackball types
	for (auto &port : portlist)
	{
		for (ioport_field& field : port.second->fields())
		{
			std::uint32_t type = field.type(); // ioport_type is a typedef for std::uint32_t
			if (type == IPT_END)
				break; // End of fields for this port

			if (type == IPT_MOUSE_X || type == IPT_MOUSE_Y)
				return true;
		}
	}

	return false;
}

bool UsesTrackball(const game_driver *game)
{
	if (!game || !game->ipt)
		return false;

	ioport_list portlist;
	std::ostringstream errors;

	// Enumerate all devices in the machine configuration and collect their input ports
	machine_config config(*game, emu_opts.GetGlobalOpts());
	for (device_t &dev : device_enumerator(config.root_device()))
		if (dev.input_ports())
			portlist.append(dev, errors);

	// Now check each port and its fields for trackball types
	for (auto &port : portlist)
	{
		for (ioport_field &field : port.second->fields())
		{
			std::uint32_t type = field.type(); // ioport_type is a typedef for std::uint32_t
			if (type == IPT_END)
				break; // End of fields for this port

			if (type == IPT_TRACKBALL_X || type == IPT_TRACKBALL_Y)
				return true;
		}
	}

	return false;
}
#if 0 // Commit 2d0a09b removed the ability to configure ":ram" from the command line & not having a UI
bool HasRam(const machine_config *config)
{
	if (!config)
		return false;

	device_interface_enumerator<ram_device> ram_enum(config->root_device());
	if (ram_enum.count() == 0)
		return false;

	ram_device *ram = ram_enum.first();
	if (!ram)
		return false;

	return true;
}
#endif
bool UsesRoms(const machine_config *config)
{
	if (!config)
		return false;

	device_enumerator rom_enum(config->root_device());
	if (rom_enum.count() == 0)
		return false;

	for (device_t& device : rom_enum)
		for (const rom_entry* region = rom_first_region(device); region; region = rom_next_region(region))
			if (ROMREGION_ISROMDATA(region))
				return true;

	return false;
}

bool HasHardDisk(const machine_config *config)
{
	if (!config)
		return false;

	device_enumerator disk_enum(config->root_device());
	if (disk_enum.count() == 0)
		return false;

	for (device_t& device : disk_enum)
		for (const rom_entry* region = rom_first_region(device); region; region = rom_next_region(region))
			if (ROMREGION_ISDISKDATA(region))
				return true;

	return false;
}

bool UsesSamples(const machine_config *config)
{
	if (!config)
		return 0;

	samples_device_enumerator sample_enum(config->root_device());

	return sample_enum.count() > 0 ? true : false;
}

bool UsesVectorGraphics(const machine_config *config)
{
	if (!config)
		return false;

	video_output_interface_enumerator screen_enum(config->root_device());
	if (screen_enum.count() == 0)
		return false;

	device_video_output_interface *screen = screen_enum.first();
	if (!screen)
		return false;

	return screen->is_vector();
}

int ScreenCount(const machine_config *config)
{
	if (!config)
		return 0;

	video_output_interface_enumerator screen_enum(config->root_device());

	return screen_enum.count();
}

int SpeakerCount(const machine_config *config)
{
	if (!config)
		return 0;

	speaker_device_enumerator speaker_enum(config->root_device());

	return speaker_enum.count();
}

static void SetDriversInfo(void)
{
	uint32_t cache_upper;
	uint32_t total = driver_list::total();
	DriverInfo *gameinfo = nullptr;

	for (uint32_t ndriver = 0; ndriver < total; ndriver++)
	{
		gameinfo = &drivers_info[ndriver];
		cache_upper = gameinfo->screenCount & upper_cache_flags::SCREENCOUNT;
		cache_upper |= (gameinfo->speakerCount << 4) & upper_cache_flags::SPEAKERCOUNT;

		set_bit<uint32_t>(cache_upper, gameinfo->hasHarddisk, upper_cache_flags::HASHDD);
		set_bit<uint32_t>(cache_upper, gameinfo->hasOptionalBIOS, upper_cache_flags::HASOPTBIOS);
//		set_bit<uint32_t>(cache_upper, gameinfo->hasRam, upper_cache_flags::HASRAM); // Commit 2d0a09b removed the ability to configure ":ram" from the command line & not having a UI
		set_bit<uint32_t>(cache_upper, gameinfo->isClone, upper_cache_flags::ISCLONE);
		set_bit<uint32_t>(cache_upper, gameinfo->usesLightGun, upper_cache_flags::USESLIGHTGUN);
		set_bit<uint32_t>(cache_upper, gameinfo->usesMouse, upper_cache_flags::USESMOUSE);
		set_bit<uint32_t>(cache_upper, gameinfo->usesRoms, upper_cache_flags::USESROMS);
		set_bit<uint32_t>(cache_upper, gameinfo->usesSamples, upper_cache_flags::USESSAMPLES);
		set_bit<uint32_t>(cache_upper, gameinfo->usesTrackball, upper_cache_flags::USESTRACKBALL);
		set_bit<uint32_t>(cache_upper, gameinfo->usesVectorGraphics, upper_cache_flags::USESVECTORGFX);

		SetDriverCache(ndriver, cache_upper);
	}
}

static void InitDriversInfo(void)
{
	std::cout << "InitDriversInfo: A" << "\n";
	const game_driver *gamedrv = nullptr;
	DriverInfo *gameinfo = nullptr;
	uint64_t cache_lower = 0U;

	std::cout << "InitDriversInfo: B" << "\n";
	const std::size_t driver_total = driver_list::total();
	for (std::size_t driver_index = 0; driver_index < driver_total; driver_index++)
	{
		cache_lower = GetDriverCacheLower(driver_index);
		gamedrv = &driver_list::driver(driver_index);
		gameinfo = &drivers_info[driver_index];

		gameinfo->isBroken = is_flag_set(cache_lower, lower_cache::NOT_WORKING);
		gameinfo->supportsSaveState = is_flag_set(cache_lower, lower_cache::SAVE_SUPPORTED); // SAVE_SUPPORTED
		gameinfo->isVertical = is_flag_set(cache_lower, lower_cache::SWAP_XY); // SWAP_XY

		gameinfo->isClone = driver_list::non_bios_clone(driver_index) != -1;

		gameinfo->hasOptionalBIOS = HasOptionalBios(gamedrv);
		gameinfo->usesLightGun = UsesLightGun(gamedrv);
		gameinfo->usesMouse = UsesMouse(gamedrv);
		gameinfo->usesTrackball = UsesTrackball(gamedrv);

		machine_config config(*gamedrv, emu_opts.GetGlobalOpts());
		gameinfo->hasHarddisk = HasHardDisk(&config);
//		gameinfo->hasRam = HasRam(&config); // Commit 2d0a09b removed the ability to configure ":ram" from the command line & not having a UI
		gameinfo->screenCount = ScreenCount(&config);
		gameinfo->usesVectorGraphics = UsesVectorGraphics(&config);
		gameinfo->speakerCount = SpeakerCount(&config);
		gameinfo->usesSamples = UsesSamples(&config);
		gameinfo->usesRoms = UsesRoms(&config);

		gameinfo->isStereo = gameinfo->speakerCount == 2;
		gameinfo->usesMultiChannelAudio = gameinfo->speakerCount > 2;
	}

	SetDriversInfo();
	std::cout << "InitDriversInfo: Finished" << "\n";
}

static void InitDriversCache(void)
{
	std::cout << "InitDriversCache: A" << "\n";
	if (RequiredDriverCache())
	{
		std::cout << "InitDriversCache: B" << "\n";
		InitDriversInfo();
		return;
	}

	DriverInfo *gameinfo = nullptr;
	uint64_t cache_lower = 0U;
	uint32_t cache_upper = 0U;

	std::cout << "InitDriversCache: C" << "\n";
	const std::size_t total = driver_list::total();
	for (std::size_t driver_index = 0; driver_index < total; driver_index++)
	{
		gameinfo = &drivers_info[driver_index];
		cache_lower = GetDriverCacheLower(driver_index);
		cache_upper = GetDriverCacheUpper(driver_index);

		gameinfo->isBroken           = is_flag_set(cache_lower, lower_cache::NOT_WORKING);
		gameinfo->isVertical         = is_flag_set(cache_lower, lower_cache::SWAP_XY);
		gameinfo->supportsSaveState  = is_flag_set(cache_lower, lower_cache::SAVE_SUPPORTED);

		gameinfo->screenCount        = cache_upper & upper_cache_flags::SCREENCOUNT;
		gameinfo->speakerCount       = (cache_upper & upper_cache_flags::SPEAKERCOUNT) >> 4;

		gameinfo->hasHarddisk        = ((cache_upper & upper_cache_flags::HASHDD)        != 0);
		gameinfo->hasOptionalBIOS    = ((cache_upper & upper_cache_flags::HASOPTBIOS)    != 0);
//		gameinfo->hasRam             = ((cache_upper & upper_cache_flags::HASRAM)        != 0); // Commit 2d0a09b removed the ability to configure ":ram" from the command line & not having a UI
		gameinfo->isClone            = ((cache_upper & upper_cache_flags::ISCLONE)       != 0);
		gameinfo->usesLightGun       = ((cache_upper & upper_cache_flags::USESLIGHTGUN)  != 0);
		gameinfo->usesMouse          = ((cache_upper & upper_cache_flags::USESMOUSE)     != 0);
		gameinfo->usesRoms           = ((cache_upper & upper_cache_flags::USESROMS)      != 0);
		gameinfo->usesSamples        = ((cache_upper & upper_cache_flags::USESSAMPLES)   != 0);
		gameinfo->usesTrackball      = ((cache_upper & upper_cache_flags::USESTRACKBALL) != 0);
		gameinfo->usesVectorGraphics = ((cache_upper & upper_cache_flags::USESVECTORGFX) != 0);

		gameinfo->isStereo = gameinfo->speakerCount == 2;
		gameinfo->usesMultiChannelAudio = gameinfo->speakerCount > 2;
	}

	std::cout << "InitDriversCache: Finished" << "\n";
}

static DriverInfo *GetDriversInfo(std::size_t driver_index)
{
	assert(driver_index < driver_list::total());
	if (first_time)
	{
		first_time = false;
		drivers_info.clear();
		drivers_info.resize(driver_list::total());
		std::cout << "DriversInfo: B" << "\n";
		InitDriversCache();
	}

	return &drivers_info[driver_index];
}

bool DriverIsClone(std::size_t driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->isClone;
}

bool DriverIsBroken(std::size_t driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->isBroken;
}

bool DriverIsHarddisk(std::size_t driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->hasHarddisk;
}

bool DriverIsBios(std::size_t driver_index)
{
	assert(driver_index < driver_list::total());
	return is_flag_set(GetDriverCacheLower(driver_index), lower_cache::IS_BIOS_ROOT); // IS_BIOS_ROOT
}

bool DriverIsMechanical(std::size_t driver_index)
{
	assert(driver_index < driver_list::total());
	return is_flag_set(GetDriverCacheLower(driver_index), lower_cache::MECHANICAL); // MECHANICAL
}

bool DriverIsArcade(std::size_t driver_index)
{
	assert(driver_index < driver_list::total());
	uint64_t system_type = GetDriverCacheLower(driver_index) & lower_cache::MASK_SYSTEMTYPE;
	return system_type == lower_cache::SYSTEMTYPE_ARCADE;
}

bool DriverHasOptionalBIOS(std::size_t driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->hasOptionalBIOS;
}

bool DriverIsStereo(std::size_t driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->isStereo;
}

int DriverScreenCount(std::size_t driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->screenCount;
}

bool DriverIsVector(std::size_t driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->usesVectorGraphics;
}

bool DriverUsesRoms(std::size_t driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->usesRoms;
}

bool DriverUsesSamples(std::size_t driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->usesSamples;
}

bool DriverUsesTrackball(std::size_t driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->usesTrackball;
}

bool DriverUsesLightGun(std::size_t driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->usesLightGun;
}

bool DriverUsesMouse(std::size_t driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->usesMouse;
}

bool DriverSupportsSaveState(std::size_t driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->supportsSaveState;
}

bool DriverIsVertical(std::size_t driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->isVertical;
}
#if 0 // Commit 2d0a09b removed the ability to configure ":ram" from the command line & not having a UI
bool DriverHasRam(std::size_t driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->hasRam;
}
#endif
void FlushFileCaches(void)
{
	util::archive_file::cache_clear();
}

bool StringIsSuffixedBy(std::string_view str, std::string_view suffix)
{
	return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

/***************************************************************************
    Win32 wrappers
 ***************************************************************************/

bool SafeIsAppThemed(void)
{
	bool bResult = false;
	bool (WINAPI *pfnIsAppThemed)(void);
	HMODULE hThemes = system_services::load_library(L"uxtheme.dll");

	if (hThemes)
	{
		pfnIsAppThemed = (bool (WINAPI *)(void)) system_services::get_proc_address_utf8(hThemes, "IsAppThemed");
		if (pfnIsAppThemed)
			bResult = pfnIsAppThemed();
		(void)system_services::free_library(hThemes);
	}

	return bResult;
}


void GetSystemErrorMessage(DWORD dwErrorId, wchar_t **tErrorMessage)
{
	if (windows::format_message(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, dwErrorId, 0, (LPWSTR)tErrorMessage, 0, nullptr) == 0)
	{
		*tErrorMessage = new wchar_t[MAX_PATH];
		(void)mui_wcscpy(*tErrorMessage, L"Unknown Error");
	}
}
