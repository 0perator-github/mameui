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

// shlwapi.h pulls in objbase.h which defines interface as a struct which conflict with a template declaration in device.h
#ifdef interface
#undef interface // undef interface which is for COM interfaces
#endif
#include "emu.h"
#define interface struct

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

/***************************************************************************
    External variables
 ***************************************************************************/

/***************************************************************************
    Internal structures
 ***************************************************************************/
using DriverInfo = struct driver_info_t
{
	int screenCount = 0;
	bool isClone = false;
	bool isBroken = false;
	bool isHarddisk = false;
	bool hasOptionalBIOS = false;
	bool isStereo = false;
	bool isVector = false;
	bool usesRoms = false;
	bool usesSamples = false;
	bool usesTrackball = false;
	bool usesLightGun = false;
	bool usesMouse = false;
	bool supportsSaveState = false;
	bool isVertical = false;
	bool hasRam = false;
};

static std::vector<DriverInfo> drivers_info;

struct upper_cache_flags
{
	enum type : uint32_t
	{
		SCREEN = 15u << 0, // bits 0-3: screen count
		ROMS = 1u << 4,  // bit 4: ROMs used
		CLONE = 1u << 5,  // bit 5: clone of another driver
		STEREO = 1u << 6,  // bit 6: stereo sound
		BIOS = 1u << 7,  // bit 7: BIOS or optional BIOS
		TRACKBALL = 1u << 8,  // bit 8: trackball used
		HARDDISK = 1u << 9,  // bit 9: hard disk used
		SAMPLES = 1u << 10, // bit 10: samples used
		LIGHTGUN = 1u << 11, // bit 11: lightgun used
		VECTOR = 1u << 12, // bit 12: vector graphics
		MOUSE = 1u << 13, // bit 13: mouse used
		RAM = 1u << 14, // bit 14: RAM used
	};
};

static bool first_time = true;
/***************************************************************************
    External functions
 ***************************************************************************/

/*
    ErrorMsg
*/
void __cdecl ErrorMsg(const char *fmt, ...)
{
	int buffer_len;
	std::ofstream outfile;
	std::string log_text;
	std::unique_ptr<char[]> buffer;
	va_list v_args;

	va_start(v_args, fmt);
	buffer_len = _vsnprintf(0, 0, fmt, v_args);
	buffer = std::make_unique<char[]>(buffer_len + 1);
	_vsnprintf(buffer.get(), buffer_len, fmt, v_args);
	va_end(v_args);

	std::unique_ptr<wchar_t[]> wcs_buffer(mui_utf16_from_utf8cstring(buffer.get()));
	dialog_boxes::message_box(input::get_active_window(), wcs_buffer.get(), &MAMEUINAME[0], MB_OK | MB_ICONERROR);
	std::wcout << MAMEUINAME<<": " << wcs_buffer.get() << "\n";

	if (!outfile.is_open())
		outfile.open("debug.txt", std::ofstream::out | std::ofstream::app);
	if (!outfile.is_open())
	{
		outfile.write(log_text.c_str(), log_text.size());
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
	buf = std::make_unique<char[]>(buffer_len + 1);
	_vsnprintf(buf.get(), buffer_len, fmt, v_args);
	va_end(v_args);

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
	_vsnwprintf(buffer.get(), buffer_len + 1, fmt, v_args);
	va_end(v_args);

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
	_vsnwprintf(buffer.get(), buffer_len + 1, fmt, v_args);
	va_end(v_args);

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
	DWORD error_code = GetLastError();

	DWORD result = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&message_buffer), 0, nullptr);
	std::wstring error_description = (!result || !message_buffer) ? L"FormatMessage failed." : message_buffer;
	if (message_buffer)
		LocalFree(message_buffer);

	while (!error_description.empty() && isspace(static_cast<unsigned char>(error_description.back())))
		error_description.pop_back();

	return L"Error code " + std::to_wstring(error_code) + L": " + error_description;
}

std::string last_system_function_error_message_utf8()
{
	char *message_buffer = nullptr;
	DWORD error_code = GetLastError();

	DWORD result = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&message_buffer), 0, nullptr);
	std::string error_description = (!result || !message_buffer) ? "FormatMessage failed." : message_buffer;
	if (message_buffer)
		LocalFree(message_buffer);

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

std::wstring GetDriverFilename(int driver_index)
{
	assert(driver_index < driver_list::total());
	std::filesystem::path driver_filename = driver_list::driver(driver_index).type.source();
	return driver_filename.filename().wstring();
}

std::string GetDriverFilename_utf8(int driver_index)
{
	assert(driver_index < driver_list::total());
	std::filesystem::path driver_filename = driver_list::driver(driver_index).type.source();
	return driver_filename.filename().string();
}

bool isDriverVector(const machine_config *config)
{
	const screen_device *screen = screen_device_enumerator(config->root_device()).first();

	if (screen)
		if (SCREEN_TYPE_VECTOR == screen->screen_type())
			return true;

	return false;
}

int numberOfScreens(const machine_config *config)
{
	screen_device_enumerator scriter(config->root_device());

	return scriter.count();
}

int numberOfSpeakers(const machine_config *config)
{
	speaker_device_enumerator iter(config->root_device());

	return iter.count();
}

static void SetDriversInfo(void)
{
	uint32_t cache;
	uint32_t total = driver_list::total();
	DriverInfo *gameinfo = nullptr;

	for (uint32_t ndriver = 0; ndriver < total; ndriver++)
	{
		gameinfo = &drivers_info[ndriver];
		cache = gameinfo->screenCount & upper_cache_flags::SCREEN;

		set_bit<uint32_t>(cache, gameinfo->isClone, upper_cache_flags::CLONE);
		set_bit<uint32_t>(cache, gameinfo->isHarddisk, upper_cache_flags::HARDDISK);
		set_bit<uint32_t>(cache, gameinfo->hasOptionalBIOS, upper_cache_flags::BIOS);
		set_bit<uint32_t>(cache, gameinfo->isStereo, upper_cache_flags::STEREO);
		set_bit<uint32_t>(cache, gameinfo->isVector, upper_cache_flags::VECTOR);
		set_bit<uint32_t>(cache, gameinfo->usesRoms, upper_cache_flags::ROMS);
		set_bit<uint32_t>(cache, gameinfo->usesSamples, upper_cache_flags::SAMPLES);
		set_bit<uint32_t>(cache, gameinfo->usesTrackball, upper_cache_flags::TRACKBALL);
		set_bit<uint32_t>(cache, gameinfo->usesLightGun, upper_cache_flags::LIGHTGUN);
		set_bit<uint32_t>(cache, gameinfo->usesMouse, upper_cache_flags::MOUSE);
		set_bit<uint32_t>(cache, gameinfo->hasRam, upper_cache_flags::RAM);

		SetDriverCache(ndriver, cache);
	}
}

static void InitDriversInfo(void)
{
	std::cout << "InitDriversInfo: A" << "\n";
	int num_speakers;
	uint32_t total = driver_list::total();
	const game_driver *gamedrv = nullptr;
	DriverInfo *gameinfo = nullptr;
	const rom_entry *region, *rom;

	for (uint32_t ndriver = 0; ndriver < total; ndriver++)
	{
		uint64_t cache = GetDriverCacheLower(ndriver);
		gamedrv = &driver_list::driver(ndriver);
		gameinfo = &drivers_info[ndriver];
		machine_config config(*gamedrv, emu_opts.GetGlobalOpts());

		gameinfo->isClone = driver_list::non_bios_clone(ndriver) != -1;

		gameinfo->isBroken = is_flag_set(cache, lower_cache::NOT_WORKING);
		gameinfo->supportsSaveState = is_flag_set(cache, lower_cache::SAVE_SUPPORTED); // SAVE_SUPPORTED

		gameinfo->isHarddisk = false;
		for (device_t& device : device_enumerator(config.root_device()))
			for (region = rom_first_region(device); region; region = rom_next_region(region))
				if (ROMREGION_ISDISKDATA(region))
					gameinfo->isHarddisk = true;

		gameinfo->isVertical = is_flag_set(cache, lower_cache::SWAP_XY); // SWAP_XY

		ram_device_enumerator iter1(config.root_device());
		gameinfo->hasRam = (iter1.first() );

		gameinfo->hasOptionalBIOS = false;
		if (gamedrv->rom)
		{
			auto rom_entries = rom_build_entries(gamedrv->rom);
			for (rom = rom_entries.data(); !ROMENTRY_ISEND(rom); rom++)
				if (ROMENTRY_ISSYSTEM_BIOS(rom))
					gameinfo->hasOptionalBIOS = true;
		}

		num_speakers = numberOfSpeakers(&config);

		gameinfo->isStereo = (num_speakers > 1);
		gameinfo->screenCount = numberOfScreens(&config);
		gameinfo->isVector = isDriverVector(&config); // ((drv.video_attributes & VIDEO_TYPE_VECTOR) != 0);
		gameinfo->usesRoms = false;
		for (device_t &device : device_enumerator(config.root_device()))
			for (region = rom_first_region(device); region; region = rom_next_region(region))
				for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
					gameinfo->usesRoms = true;

		samples_device_enumerator iter(config.root_device());
		gameinfo->usesSamples = iter.count() ? true : false;

		gameinfo->usesTrackball = false;
		gameinfo->usesLightGun = false;
		gameinfo->usesMouse = false;

		if (gamedrv->ipt)
		{
			ioport_list portlist;
			std::ostringstream errors;
			for (device_t &cfg : device_enumerator(config.root_device()))
				if (cfg.input_ports())
					portlist.append(cfg, errors);

			for (auto &port : portlist)
			{
				for (ioport_field &field : port.second->fields())
				{
					uint32_t type = field.type();
					if (type == IPT_END)
						break;
					if (type == IPT_DIAL || type == IPT_PADDLE || type == IPT_TRACKBALL_X || type == IPT_TRACKBALL_Y || type == IPT_AD_STICK_X || type == IPT_AD_STICK_Y)
						gameinfo->usesTrackball = true;
					if (type == IPT_LIGHTGUN_X || type == IPT_LIGHTGUN_Y)
						gameinfo->usesLightGun = true;
					if (type == IPT_MOUSE_X || type == IPT_MOUSE_Y)
						gameinfo->usesMouse = true;
				}
			}
		}
	}

	SetDriversInfo();
	std::cout << "InitDriversInfo: Finished" << "\n";
}

static int InitDriversCache(void)
{
	std::cout << "InitDriversCache: A" << "\n";
	if (RequiredDriverCache())
	{
		std::cout << "InitDriversCache: B" << "\n";
		InitDriversInfo();
		return 0;
	}

	std::cout << "InitDriversCache: C" << "\n";
	uint64_t cache_lower;
	uint32_t cache_upper;
	uint32_t total = driver_list::total();
	DriverInfo *gameinfo = nullptr;

	std::cout << "InitDriversCache: D" << "\n";
	for (uint32_t ndriver = 0; ndriver < total; ndriver++)
	{
		gameinfo = &drivers_info[ndriver];
		cache_lower = GetDriverCacheLower(ndriver);
		cache_upper = GetDriverCacheUpper(ndriver);

		gameinfo->isBroken          =  (cache_lower & lower_cache::NOT_WORKING);
		gameinfo->supportsSaveState = is_flag_set(cache_lower, lower_cache::SAVE_SUPPORTED); // SAVE_SUPPORTED
		gameinfo->isVertical        = is_flag_set(cache_lower, lower_cache::SWAP_XY); // SWAP_XY
		gameinfo->screenCount       = cache_upper & upper_cache_flags::SCREEN;
		gameinfo->isClone           = ((cache_upper & upper_cache_flags::CLONE)     != 0);
		gameinfo->isHarddisk        = ((cache_upper & upper_cache_flags::HARDDISK)  != 0);
		gameinfo->hasOptionalBIOS   = ((cache_upper & upper_cache_flags::BIOS)      != 0);
		gameinfo->isStereo          = ((cache_upper & upper_cache_flags::STEREO)    != 0);
		gameinfo->isVector          = ((cache_upper & upper_cache_flags::VECTOR)    != 0);
		gameinfo->usesRoms          = ((cache_upper & upper_cache_flags::ROMS)      != 0);
		gameinfo->usesSamples       = ((cache_upper & upper_cache_flags::SAMPLES)   != 0);
		gameinfo->usesTrackball     = ((cache_upper & upper_cache_flags::TRACKBALL) != 0);
		gameinfo->usesLightGun      = ((cache_upper & upper_cache_flags::LIGHTGUN)  != 0);
		gameinfo->usesMouse         = ((cache_upper & upper_cache_flags::MOUSE)     != 0);
		gameinfo->hasRam            = ((cache_upper & upper_cache_flags::RAM)       != 0);
	}

	std::cout << "InitDriversCache: Finished" << "\n";

	return 0;
}

static DriverInfo *GetDriversInfo(int driver_index)
{

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

bool DriverIsClone(int driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->isClone;
}

bool DriverIsBroken(int driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->isBroken;
}

bool DriverIsHarddisk(int driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->isHarddisk;
}

bool DriverIsBios(int driver_index)
{
	assert(driver_index < driver_list::total());
	return is_flag_set(GetDriverCacheLower(driver_index), lower_cache::IS_BIOS_ROOT); // IS_BIOS_ROOT
}

bool DriverIsMechanical(int driver_index)
{
	assert(driver_index < driver_list::total());
	return is_flag_set(GetDriverCacheLower(driver_index), lower_cache::MECHANICAL); // MECHANICAL
}

bool DriverIsArcade(int driver_index)
{
	assert(driver_index < driver_list::total());
	uint64_t system_type = GetDriverCacheLower(driver_index) & lower_cache::MASK_SYSTEMTYPE;
	return system_type == lower_cache::SYSTEMTYPE_ARCADE;
}

bool DriverHasOptionalBIOS(int driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->hasOptionalBIOS;
}

bool DriverIsStereo(int driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->isStereo;
}

int DriverNumScreens(int driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->screenCount;
}

bool DriverIsVector(int driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->isVector;
}

bool DriverUsesRoms(int driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->usesRoms;
}

bool DriverUsesSamples(int driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->usesSamples;
}

bool DriverUsesTrackball(int driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->usesTrackball;
}

bool DriverUsesLightGun(int driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->usesLightGun;
}

bool DriverUsesMouse(int driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->usesMouse;
}

bool DriverSupportsSaveState(int driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->supportsSaveState;
}

bool DriverIsVertical(int driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->isVertical;
}

bool DriverHasRam(int driver_index)
{
	assert(driver_index < driver_list::total());
	return GetDriversInfo(driver_index)->hasRam;
}

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
