// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

/***************************************************************************

  mui_util.cpp

 ***************************************************************************/

// standard windows headers

// standard C++ headers
#include <fstream>
#include <iostream>
#include <string>

// MAME/MAMEUI headers
#include "emu.h"
#include "drivenum.h"
#include "machine/ram.h"
#include "path.h"
#include "romload.h"
#include "screen.h"
#include "sound/samples.h"
#include "speaker.h"
#include "unzip.h"

#include "winapi_dialog_boxes.h"
#include "winapi_gdi.h"
#include "winapi_input.h"
#include "winapi_system_services.h"
#include "winapi_shell.h"
#include "winapi_windows.h"

#include "mui_str.h"
#include "mui_wcs.h"
#include "mui_wcsconv.h"

#include "winui.h"
#include "emu_opts.h"
#include "mui_opts.h"

#include "mui_util.h"

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
struct DriversInfo
{
	int screenCount;
	bool isClone;
	bool isBroken;
	bool isHarddisk;
	bool hasOptionalBIOS;
	bool isStereo;
	bool isVector;
	bool usesRoms;
	bool usesSamples;
	bool usesTrackball;
	bool usesLightGun;
	bool usesMouse;
	bool supportsSaveState;
	bool isVertical;
	bool hasRam;
};

static std::vector<DriversInfo> drivers_info;
static bool bFirst = true;


enum
{
	DRIVER_CACHE_SCREEN     = 0x000F,
	DRIVER_CACHE_ROMS       = 0x0010,
	DRIVER_CACHE_CLONE      = 0x0020,
	DRIVER_CACHE_STEREO     = 0x0040,
	DRIVER_CACHE_BIOS       = 0x0080,
	DRIVER_CACHE_TRACKBALL  = 0x0100,
	DRIVER_CACHE_HARDDISK   = 0x0200,
	DRIVER_CACHE_SAMPLES    = 0x0400,
	DRIVER_CACHE_LIGHTGUN   = 0x0800,
	DRIVER_CACHE_VECTOR     = 0x1000,
	DRIVER_CACHE_MOUSE      = 0x2000,
	DRIVER_CACHE_RAM        = 0x4000,
};

/***************************************************************************
    External functions
 ***************************************************************************/

/*
    ErrorMsg
*/
void __cdecl ErrorMsg(const char* fmt, ...)
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

	std::unique_ptr<wchar_t[]> wcs_buffer(mui_wcstring_from_utf8(buffer.get()));
	dialog_boxes::message_box(input::get_active_window(), wcs_buffer.get(), &MAMEUINAME[0], MB_OK | MB_ICONERROR);
	std::wcout << MAMEUINAME<<": " << wcs_buffer.get() << std::endl;

	if (!outfile.is_open())
		outfile.open("debug.txt", std::ofstream::out | std::ofstream::app);
	if (!outfile.is_open())
	{
		outfile.write(log_text.c_str(), log_text.size());
		outfile.close();
	}
}

void __cdecl dprintf(const char* fmt, ...)
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

void ErrorMessageBox(const char *fmt, ...)
{
	int buffer_len;
	std::unique_ptr<char[]> buffer;
	va_list v_args = 0;

	va_start(v_args, fmt);
	buffer_len = _vsnprintf(0, 0, fmt, v_args);
	buffer = std::make_unique<char[]>(buffer_len + 1);
	vsnprintf(buffer.get(), buffer_len, fmt, v_args);
	va_end(v_args);

	std::unique_ptr<wchar_t[]> wcs_buffer(mui_wcstring_from_utf8(buffer.get()));
	dialog_boxes::message_box(input::get_active_window(), wcs_buffer.get(), &MAMEUINAME[0], MB_OK | MB_ICONERROR);
}

void ShellExecuteCommon(HWND hWnd, const char *cName)
{
	std::unique_ptr<wchar_t[]> wcs_name(mui_wcstring_from_utf8(cName));

	if(!wcs_name)
		return;

	HINSTANCE hErr = shell::shell_execute(hWnd, NULL, wcs_name.get(), NULL, NULL, SW_SHOWNORMAL);

	if ((uintptr_t)hErr > 32)
		return;

	const char *msg = NULL;
	switch((uintptr_t)hErr)
	{
	case 0:
		msg = "The Operating System is out of memory or resources.";
		break;

	case ERROR_FILE_NOT_FOUND:
		msg = "The specified file was not found.";
		break;

	case SE_ERR_NOASSOC :
		msg = "There is no application associated with the given filename extension.";
		break;

	case SE_ERR_OOM :
		msg = "There was not enough memory to complete the operation.";
		break;

	case SE_ERR_PNF :
		msg = "The specified path was not found.";
		break;

	case SE_ERR_SHARE :
		msg = "A sharing violation occurred.";
		break;

	default:
		msg = "Unknown error.";
	}

	ErrorMessageBox("%s\r\nPath: '%s'", msg, cName);
}

UINT GetDepth(HWND hWnd)
{
	UINT nBPP;
	HDC hDC;

	hDC = gdi::get_dc(hWnd);

	nBPP = gdi::get_device_caps(hDC, BITSPIXEL) * gdi::get_device_caps(hDC, PLANES);

	(void)gdi::release_dc(hWnd, hDC);

	return nBPP;
}

void DisplayTextFile(HWND hWnd, const char *cName)
{
	std::unique_ptr<wchar_t[]> wcs_name(mui_wcstring_from_utf8(cName));
	if( !wcs_name)
		return;

	HINSTANCE hErr = shell::shell_execute(hWnd, NULL, wcs_name.get(), NULL, NULL, SW_SHOWNORMAL);
	if ((uintptr_t)hErr > 32)
		return;

	LPCTSTR msg = 0;
	switch((uintptr_t)hErr)
	{
	case 0:
		msg = L"The operating system is out of memory or resources.";
		break;

	case ERROR_FILE_NOT_FOUND:
		msg = L"The specified file was not found.";
		break;

	case SE_ERR_NOASSOC :
		msg = L"There is no application associated with the given filename extension.";
		break;

	case SE_ERR_OOM :
		msg = L"There was not enough memory to complete the operation.";
		break;

	case SE_ERR_PNF :
		msg = L"The specified path was not found.";
		break;

	case SE_ERR_SHARE :
		msg = L"A sharing violation occurred.";
		break;

	default:
		msg = L"Unknown error.";
	}

	dialog_boxes::message_box(NULL, msg, wcs_name.get(), MB_OK);
}

char * ConvertToWindowsNewlines(const char *source)
{
	static char buf[2048 * 2048];
	char *dest;

	dest = buf;
	while (*source != 0)
	{
		if (*source == '\n')
		{
			*dest++ = '\r';
			*dest++ = '\n';
		}
		else
			*dest++ = *source;
		source++;
	}
	*dest = 0;

	return buf;
}

// Lop off path and extention from a source file name
// This assumes their is a pathname passed to the function
// like src\drivers\blah.c

const char *GetDriverFilename(uint32_t nIndex)
{
	std::string_view game_driver_name;
	game_driver_name = core_filename_extract_base(driver_list::driver(nIndex).type.source());

	return &game_driver_name[0];
}

BOOL isDriverVector(const machine_config *config)
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
	struct DriversInfo *gameinfo = NULL;

	for (uint32_t ndriver = 0; ndriver < total; ndriver++)
	{
		gameinfo = &drivers_info[ndriver];
		cache = gameinfo->screenCount & DRIVER_CACHE_SCREEN;

		if (gameinfo->isClone)
			cache += DRIVER_CACHE_CLONE;

		if (gameinfo->isHarddisk)
			cache += DRIVER_CACHE_HARDDISK;

		if (gameinfo->hasOptionalBIOS)
			cache += DRIVER_CACHE_BIOS;

		if (gameinfo->isStereo)
			cache += DRIVER_CACHE_STEREO;

		if (gameinfo->isVector)
			cache += DRIVER_CACHE_VECTOR;

		if (gameinfo->usesRoms)
			cache += DRIVER_CACHE_ROMS;

		if (gameinfo->usesSamples)
			cache += DRIVER_CACHE_SAMPLES;

		if (gameinfo->usesTrackball)
			cache += DRIVER_CACHE_TRACKBALL;

		if (gameinfo->usesLightGun)
			cache += DRIVER_CACHE_LIGHTGUN;

		if (gameinfo->usesMouse)
			cache += DRIVER_CACHE_MOUSE;

		if (gameinfo->hasRam)
			cache += DRIVER_CACHE_RAM;

		SetDriverCache(ndriver, cache);
	}
}

static void InitDriversInfo(void)
{
	std::cout << "InitDriversInfo: A" << std::endl;
	int num_speakers;
	uint32_t total = driver_list::total();
	const game_driver *gamedrv = NULL;
	struct DriversInfo *gameinfo = NULL;
	const rom_entry *region, *rom;

	for (uint32_t ndriver = 0; ndriver < total; ndriver++)
	{
		uint32_t cache = GetDriverCacheLower(ndriver);
		gamedrv = &driver_list::driver(ndriver);
		gameinfo = &drivers_info[ndriver];
		machine_config config(*gamedrv, MameUIGlobal());

		bool const have_parent(mui_strcmp(gamedrv->parent, "0"));
		auto const parent_idx(have_parent ? driver_list::find(gamedrv->parent) : -1);
		gameinfo->isClone = ( !have_parent || (0 > parent_idx) || BIT(GetDriverCacheLower(parent_idx),9)) ? false : true;
		gameinfo->isBroken = (cache & 0x4040) ? true : false;  // (MACHINE_NOT_WORKING | MACHINE_MECHANICAL)
		gameinfo->supportsSaveState = BIT(cache, 7) ^ 1;  //MACHINE_SUPPORTS_SAVE
		gameinfo->isHarddisk = false;
		gameinfo->isVertical = BIT(cache, 2);  //ORIENTATION_SWAP_XY

		ram_device_enumerator iter1(config.root_device());
		gameinfo->hasRam = (iter1.first() );

		for (device_t &device : device_enumerator(config.root_device()))
			for (region = rom_first_region(device); region; region = rom_next_region(region))
				if (ROMREGION_ISDISKDATA(region))
					gameinfo->isHarddisk = true;

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
			std::string errors;
			for (device_t &cfg : device_enumerator(config.root_device()))
				if (cfg.input_ports())
					portlist.append(cfg, errors);

			for (auto &port : portlist)
			{
				for (ioport_field &field : port.second->fields())
				{
					UINT32 type;
					type = field.type();
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
	std::cout << "InitDriversInfo: Finished" << std::endl;
}

static int InitDriversCache(void)
{
	std::cout << "InitDriversCache: A" << std::endl;
	if (RequiredDriverCache())
	{
		std::cout << "InitDriversCache: B" << std::endl;
		InitDriversInfo();
		return 0;
	}

	std::cout << "InitDriversCache: C" << std::endl;
	uint32_t cache_lower, cache_upper;
	uint32_t total = driver_list::total();
	struct DriversInfo *gameinfo = NULL;

	std::cout << "InitDriversCache: D" << std::endl;
	for (uint32_t ndriver = 0; ndriver < total; ndriver++)
	{
		gameinfo = &drivers_info[ndriver];
		cache_lower = GetDriverCacheLower(ndriver);
		cache_upper = GetDriverCacheUpper(ndriver);

		gameinfo->isBroken          =  (cache_lower & 0x4040) ? true : false; //MACHINE_NOT_WORKING | MACHINE_MECHANICAL
		gameinfo->supportsSaveState =  BIT(cache_lower, 7) ? false : true;  //MACHINE_SUPPORTS_SAVE
		gameinfo->isVertical        =  BIT(cache_lower, 2) ? true : false;  //ORIENTATION_XY
		gameinfo->screenCount       =   cache_upper & DRIVER_CACHE_SCREEN;
		gameinfo->isClone           = ((cache_upper & DRIVER_CACHE_CLONE)     != 0);
		gameinfo->isHarddisk        = ((cache_upper & DRIVER_CACHE_HARDDISK)  != 0);
		gameinfo->hasOptionalBIOS   = ((cache_upper & DRIVER_CACHE_BIOS)      != 0);
		gameinfo->isStereo          = ((cache_upper & DRIVER_CACHE_STEREO)    != 0);
		gameinfo->isVector          = ((cache_upper & DRIVER_CACHE_VECTOR)    != 0);
		gameinfo->usesRoms          = ((cache_upper & DRIVER_CACHE_ROMS)      != 0);
		gameinfo->usesSamples       = ((cache_upper & DRIVER_CACHE_SAMPLES)   != 0);
		gameinfo->usesTrackball     = ((cache_upper & DRIVER_CACHE_TRACKBALL) != 0);
		gameinfo->usesLightGun      = ((cache_upper & DRIVER_CACHE_LIGHTGUN)  != 0);
		gameinfo->usesMouse         = ((cache_upper & DRIVER_CACHE_MOUSE)     != 0);
		gameinfo->hasRam            = ((cache_upper & DRIVER_CACHE_RAM)       != 0);
	}

	std::cout << "InitDriversCache: Finished" << std::endl;

	return 0;
}

static struct DriversInfo* GetDriversInfo(uint32_t driver_index)
{
	if (bFirst)
	{
		bFirst = false;

		drivers_info.clear();
		drivers_info.resize(driver_list::total());
		std::fill(drivers_info.begin(), drivers_info.end(), DriversInfo{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
		std::cout << "DriversInfo: B" << std::endl;
		InitDriversCache();
	}

	return &drivers_info[driver_index];
}

BOOL DriverIsClone(uint32_t driver_index)
{
	DriversInfo* CloneInfo = GetDriversInfo(driver_index);
	 return CloneInfo->isClone;
}

BOOL DriverIsBroken(uint32_t driver_index)
{
	return GetDriversInfo(driver_index)->isBroken;
}

BOOL DriverIsHarddisk(uint32_t driver_index)
{
	return GetDriversInfo(driver_index)->isHarddisk;
}

BOOL DriverIsBios(uint32_t driver_index)
{
	return BIT(GetDriverCacheLower(driver_index), 9);
}

BOOL DriverIsMechanical(uint32_t driver_index)
{
	return BIT(GetDriverCacheLower(driver_index), 14);
}

BOOL DriverIsArcade(uint32_t driver_index)
{
	return ((GetDriverCacheLower(driver_index) & 3) == 0) ? true: false;  //TYPE_ARCADE
}

BOOL DriverHasOptionalBIOS(uint32_t driver_index)
{
	return GetDriversInfo(driver_index)->hasOptionalBIOS;
}

BOOL DriverIsStereo(uint32_t driver_index)
{
	return GetDriversInfo(driver_index)->isStereo;
}

int DriverNumScreens(uint32_t driver_index)
{
	return GetDriversInfo(driver_index)->screenCount;
}

BOOL DriverIsVector(uint32_t driver_index)
{
	return GetDriversInfo(driver_index)->isVector;
}

BOOL DriverUsesRoms(uint32_t driver_index)
{
	return GetDriversInfo(driver_index)->usesRoms;
}

BOOL DriverUsesSamples(uint32_t driver_index)
{
	return GetDriversInfo(driver_index)->usesSamples;
}

BOOL DriverUsesTrackball(uint32_t driver_index)
{
	return GetDriversInfo(driver_index)->usesTrackball;
}

BOOL DriverUsesLightGun(uint32_t driver_index)
{
	return GetDriversInfo(driver_index)->usesLightGun;
}

BOOL DriverUsesMouse(uint32_t driver_index)
{
	return GetDriversInfo(driver_index)->usesMouse;
}

BOOL DriverSupportsSaveState(uint32_t driver_index)
{
	return GetDriversInfo(driver_index)->supportsSaveState;
}

BOOL DriverIsVertical(uint32_t driver_index)
{
	return GetDriversInfo(driver_index)->isVertical;
}

BOOL DriverHasRam(uint32_t driver_index)
{
	return GetDriversInfo(driver_index)->hasRam;
}

void FlushFileCaches(void)
{
	util::archive_file::cache_clear();
}

BOOL StringIsSuffixedBy(const char *s, const char *suffix)
{
	return (mui_strlen(s) > mui_strlen(suffix)) && !mui_strcmp(s + mui_strlen(s) - mui_strlen(suffix), suffix);
}

/***************************************************************************
    Win32 wrappers
 ***************************************************************************/

BOOL SafeIsAppThemed(void)
{
	BOOL bResult = false;
	BOOL (WINAPI *pfnIsAppThemed)(void);
	HMODULE hThemes = system_services::load_library(L"uxtheme.dll");

	if (hThemes)
	{
		pfnIsAppThemed = (BOOL (WINAPI *)(void)) system_services::get_proc_address_utf8(hThemes, "IsAppThemed");
		if (pfnIsAppThemed)
			bResult = pfnIsAppThemed();
		(void)system_services::free_library(hThemes);
	}

	return bResult;
}


void GetSystemErrorMessage(DWORD dwErrorId, wchar_t **tErrorMessage)
{
	if (windows::format_message(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErrorId, 0, (LPWSTR)tErrorMessage, 0, NULL) == 0)
	{
		*tErrorMessage = new wchar_t[MAX_PATH];
		(void)mui_wcscpy(*tErrorMessage, L"Unknown Error");
	}
}
