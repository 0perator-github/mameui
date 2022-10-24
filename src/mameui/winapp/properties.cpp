// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

/***************************************************************************

  Properties.cpp

    Properties Popup and Misc UI support routines.

    Created 8/29/98 by Mike Haaland (mhaaland@hypertech.com)

***************************************************************************/

/***************************************************************************

MSH - 20070809
--
Notes on properties and ini files, reset and reset to default.
----------------------------------------------------------------------------
Each ini contains a complete option set.

Priority order for option sets (Lowest to Highest):

built-in defaults
program    ini (executable root filename ini)
debug      ini (if running a debug build)
driver     ini (source code root filename in which this driver is found)
game       ini (where game is the driver name for this game)

To determine which option set to use, start at the top level (lowest
priority), and overlay all higher priority ini's until the desired level
is reached.

The 'default' option set is the next priority higher up the list from
the desired level. For the default (program.ini) level, it is also the
default.

When MAME is run, the desired level is game ini.

Expected Code behavior:
----------------------------------------------------------------------------
This approach requires 3 option sets, 'current', 'original' and 'default'.

'current': used to populate the property pages, and to initialize the
'original' set.

'original': used to evaluate if the 'Reset' button is enabled.
If 'current' matches 'original', the 'Reset' button is disabled,
otherwise it is enabled.

'default': used to evaluate if the 'Restore to Defaults' button is enabled.
If 'current' matches 'default', the 'Restore to Defaults' button is disabled,
otherwise it is enabled.

When editing any option set, the desired level is set to the one being
edited, the default set for that level, is the next lower priority set found.

Upon entering the properties dialog:
a) 'current' is initialized
b) 'original' is initialized by 'current'
c) 'default' is initialized
d) Populate Property pages with 'current'
e) 'Reset' and 'Restore to Defaults' buttons are evaluated.

After any change:
a) 'current' is updated
b) 'Reset' and 'Restore to Defaults' buttons are re-evaluated.

Reset:
a) 'current' is reinitialized to 'original'
b) Re-populate Property pages with 'current'
c) 'Reset' and 'Restore to Defaults' buttons are re-evaluated.

Restore to Defaults:
a) 'current' is reinitialized to 'default'
b) Re-populate Property pages with 'current'
b) 'Reset' and 'Restore to Defaults' buttons are re-evaluated.

Apply:
a) 'original' is reinitialized to 'current'
b) 'Reset' and 'Restore to defaults' are re-evaluated.
c) If they 'current' matches 'default', remove the ini from disk.
   Otherwise, write the ini to disk.

Cancel:
a) Exit the dialog.

OK:
a) If they 'current' matches 'default', remove the ini from disk.
   Otherwise, write the ini to disk.
b) Exit the dialog.


***************************************************************************/

// standard C++ headers
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <memory>
#include <string_view>

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
#include "modules/font/font_module.h"
#include "modules/input/input_module.h"
#include "modules/monitor/monitor_module.h"
#include "romload.h"
#include "screen.h"
#include "sound/samples.h"

#include "ui/info.h"
#include "ui/moptions.h"
#include "winopts.h"

// MAMEUI headers
#include "mui_cstr.h"
#include "mui_stringtokenizer.h"
#include "mui_wcstr.h"
#include "mui_wcstrconv.h"

#include "data_access_storage.h"
#include "dialog_boxes.h"
#include "menus_other_res.h"
#include "system_services.h"
#include "windows_controls.h"
#include "windows_gdi.h"
#include "windows_messages.h"
#include "windows_shell.h"


#include "bitmask.h"
#include "datamap.h"
#include "dijoystick.h"     /* For DIJoystick availability. */
#include "emu_opts.h"
#include "game_opts.h"
#include "mui_opts.h"
#include "directories.h"
#include "help.h"
#include "mui_audit.h"
#include "mui_util.h"
#include "resource.h"
#include "screenshot.h"
#include "winui.h"

#if defined(MAMEUI_NEWUI)
#include "newuires.h"
#endif

#include "properties.h"

using namespace mameui::util::string_util;
using namespace mameui::winapi::controls;
using namespace mameui::winapi;
using namespace std::literals;

#if defined(__GNUC__)
/* fix warning: cast does not match function type */
#undef  PropSheet_GetTabControl
#define PropSheet_GetTabControl(d) (HWND)(LRESULT)(int)windows::send_message((d),PSM_GETTABCONTROL,0,0)
#endif /* defined(__GNUC__) */

/***************************************************************
 * Imported function prototypes
 ***************************************************************/

/**************************************************************
 * Local function prototypes
 **************************************************************/

//static void SetSamplesEnabled(HWND hWnd, int nIndex, bool bSoundEnabled);
static void InitializeOptions(HWND hDlg);
static void InitializeMisc(HWND hDlg);
static void OptOnHScroll(HWND hWnd, HWND hwndCtl, UINT code, int pos);
static void NumScreensSelectionChange(HWND hwnd);
static void RefreshSelectionChange(HWND hWnd, HWND hWndCtrl);
static void InitializeSoundUI(HWND hwnd);
static void InitializeSkippingUI(HWND hwnd);
static void InitializeRotateUI(HWND hwnd);
static void UpdateSelectScreenUI(HWND hwnd);
static void InitializeVideoUI(HWND hwnd);
static void InitializeBIOSUI(HWND hwnd);
static void InitializeControllerMappingUI(HWND hwnd);
static void InitializeProviderMappingUI(HWND hwnd);
//static void InitializeLanguageUI(HWND hWnd);
static void InitializePluginsUI(HWND hWnd);
static void InitializeGLSLFilterUI(HWND hWnd);
static void InitializeBGFXBackendUI(HWND);
static void UpdateOptions(HWND hDlg, datamap *map, windows_options &options);
static void UpdateProperties(HWND hDlg, datamap *map, windows_options &options);
static void PropToOptions(HWND hWnd, windows_options &options);
static void OptionsToProp(HWND hWnd, windows_options &options);
static void SetPropEnabledControls(HWND hWnd);
static bool SelectLUAScript(HWND hWnd);
static bool ResetLUAScript(HWND hWnd);
static bool SelectGLSLShader(HWND, int, bool);
static bool ResetGLSLShader(HWND, int, bool);
static bool plugin_enabled(std::string_view plugin_list, std::string_view plugin_name);
static bool SelectPlugins(HWND hWnd);
static bool ResetPlugins(HWND hWnd);
static bool SelectBGFXChains(HWND hWnd);
static bool ResetBGFXChains(HWND hWnd);
static bool SelectEffect(HWND hWnd);
static bool ResetEffect(HWND hWnd);
static bool ChangeFallback(HWND hWnd);
static bool ChangeOverride(HWND hWnd);
static bool ChangeJoystickMap(HWND hWnd);
static bool ResetJoystickMap(HWND hWnd);
//static bool SelectDebugscript(HWND hWnd);
//static bool ResetDebugscript(HWND hWnd);

static void BuildDataMap(void);
static void ResetDataMap(HWND hWnd);

static std::string ResolutionSetOptionName(datamap* map, HWND dialog, HWND control);
static std::string ViewSetOptionName(datamap* map, HWND dialog, HWND control);

static void UpdateBackgroundBrush(HWND hwndTab);
static HBRUSH hBkBrush;

#ifdef MESS
static bool DirListReadControl(datamap *map, HWND dialog, HWND control, windows_options *options, std::string option_name);
static bool DirListPopulateControl(datamap *map, HWND dialog, HWND control, windows_options *options, std::string option_name);
static bool RamPopulateControl(datamap *map, HWND dialog, HWND control, windows_options *options, std::string option_name);
extern bool BrowseForDirectory(HWND hwnd, std::wstring_view pStartDir, std::wstring &selected_dirpath);
#endif

/**************************************************************
 * Local public variables
 **************************************************************/

bool g_bModifiedSoftwarePaths = false;

/**************************************************************
 * Local private variables
 **************************************************************/

/* No longer used by the core, but we need it to predefine configurable screens for all games. */
#ifndef MAX_SCREENS
/* maximum number of screens for one game */
#define MAX_SCREENS 4
#endif

std::unique_ptr<windows_options> m_CurrentOpts;
std::unique_ptr<windows_options> m_DefaultOpts;
std::unique_ptr<windows_options> m_OrigOpts;

windows_options &GetDefaultOpts()
{
	if (!m_DefaultOpts)
		m_DefaultOpts = std::make_unique<windows_options>();

	return *m_DefaultOpts;
}

windows_options &GetOrigOpts()
{
	if (!m_OrigOpts)
		m_OrigOpts = std::make_unique<windows_options>();

	return *m_OrigOpts;
}

windows_options &GetCurrentOpts()
{
	if (!m_CurrentOpts)
		m_CurrentOpts = std::make_unique<windows_options>();

	return *m_CurrentOpts;
}

static datamap *properties_datamap;

static int  g_nGame            = 0;
static int  g_nFolder          = 0;
static int m_currScreen = -1;
static SOFTWARETYPE_OPTIONS g_nPropertyMode = SOFTWARETYPE_GAME;
static bool  g_bAutoAspect[MAX_SCREENS+1] = {false, false, false, false, false}; // state of tick on keep-aspect checkbox on "Screen" pane, per screen
static bool  g_bAutoSnapSize = false;
static HICON g_hIcon = nullptr;
std::vector<std::string> plugin_names(32);

/* Property sheets */

#define HIGHLIGHT_COLOR RGB(0,196,0)
static HBRUSH highlight_brush = nullptr;
static HBRUSH background_brush = nullptr;

#define ORIENTATION_COLOR RGB( 190, 128, 0) //LIGHT BROWN
#define VECTOR_COLOR RGB( 190, 0, 0) //DARK RED
#define FOLDER_COLOR RGB( 0, 128, 0 ) // DARK GREEN
#define PARENT_COLOR RGB( 190, 128, 192 ) // PURPLE
#define GAME_COLOR RGB( 0, 128, 192 ) // DARK BLUE

bool PropSheetFilter_Vector(const machine_config *drv, const game_driver *gamedrv)
{
	return isDriverVector(drv);
}

/* Help IDs - moved to auto-generated helpids.c */
extern const DWORD dwHelpIDs[];

using DUALCOMBOSTR = struct dual_combo_string
{
	const wchar_t *m_pText;
	const char *m_pData;
};

using DUALCOMBOINT= struct dual_combo_integer
{
	const wchar_t *m_pText;
	const int m_pData;
};

const DUALCOMBOSTR m_cb_Video[] =
{
	{ L"Auto",        "auto"   },
	{ L"GDI",         "gdi"    },
	{ L"Direct3D",    "d3d"    },
	{ L"BGFX",        "bgfx"   },
	{ L"OpenGL",      "opengl" },
};
constexpr size_t NUMVIDEO(std::size(m_cb_Video));

const DUALCOMBOSTR m_cb_Sound[] =
{
	{ L"None",                       "none"      },
	{ L"Auto",                       "auto"      },
	{ L"DirectSound",                "dsound"    },
	{ L"PortAudio",                  "portaudio" },
	{ L"Wasapi",                     "wasapi" },
	{ L"XAudio2 (Win10+ only)",      "xaudio2" },
};
constexpr size_t NUMSOUND(std::size(m_cb_Sound));

const DUALCOMBOINT m_cb_SelectScreen[] =
{
	{ L"Default",     -1 },
	{ L"Screen 0",     0 },
	{ L"Screen 1",     1 },
	{ L"Screen 2",     2 },
	{ L"Screen 3",     3 },
};
constexpr size_t NUMSELECTSCREEN(std::size(m_cb_SelectScreen));

const DUALCOMBOSTR m_cb_View[] =
{
	{ L"Auto",            "auto"     },
	{ L"Standard",        "standard" },
	{ L"Pixel Aspect",    "pixel"    },
	{ L"Cocktail",        "cocktail" },
};
constexpr size_t NUMVIEW(std::size(m_cb_View));

const DUALCOMBOSTR m_cb_Device[] =
{
	{ L"None",        "none"     },
	{ L"Keyboard",    "keyboard" },
	{ L"Mouse",      "mouse"     },
	{ L"Joystick",    "joystick" },
	{ L"Lightgun",    "lightgun" },
};
constexpr size_t NUMDEVICES(std::size(m_cb_Device));

const DUALCOMBOSTR m_cb_ProvUifont[] =
{
	{ L"Auto",            "auto"   },
	{ L"Windows",         "win"    },
	{ L"Direct Write",    "dwrite" },
	{ L"None",            "none"   },
};
constexpr size_t NUMPROVUIFONT(std::size(m_cb_ProvUifont));

const DUALCOMBOSTR m_cb_ProvKeyboard[] =
{
	{ L"Auto",            "auto"     },
	{ L"Windows",         "win32"    },
	{ L"RAW",             "rawinput" },
	{ L"Direct Input",    "dinput"   },
	{ L"None",            "none"     },
};
constexpr size_t NUMPROVKEYBOARD(std::size(m_cb_ProvKeyboard));

const DUALCOMBOSTR m_cb_ProvMouse[] =
{
	{ L"Auto",            "auto"     },
	{ L"Windows",         "win32"    },
	{ L"RAW",             "rawinput" },
	{ L"Direct Input",    "dinput"   },
	{ L"None",            "none"     },
};
constexpr size_t NUMPROVMOUSE(std::size(m_cb_ProvMouse));

const DUALCOMBOSTR m_cb_ProvJoystick[] =
{
	{ L"Auto",            "auto"      },
	{ L"WinHybrid",       "winhybrid" },
	{ L"Xinput",          "xinput"    },
	{ L"Direct Input",    "dinput"    },
	{ L"None",            "none"      },
};
constexpr size_t NUMPROVJOYSTICK(std::size(m_cb_ProvJoystick));

const DUALCOMBOSTR m_cb_ProvLightgun[] =
{
	{ L"Auto",       "auto"     },
	{ L"Windows",    "win32"    },
	{ L"RAW",        "rawinput" },
	{ L"None",       "none"     },
};
constexpr size_t NUMPROVLIGHTGUN(std::size(m_cb_ProvLightgun));

const DUALCOMBOSTR m_cb_ProvMonitor[] =
{
	{ L"Auto",       "auto"  },
	{ L"Windows",    "win32" },
	{ L"DXGI",       "dxgi"  },
};
constexpr size_t NUMPROVMONITOR(std::size(m_cb_ProvMonitor));

const DUALCOMBOSTR m_cb_ProvOutput[] =
{
	{ L"Auto",       "auto"    },
	{ L"Console",    "console" },
	{ L"Network",    "network" },
	{ L"Windows",    "win32"   },
	{ L"None",       "none"    },
};
constexpr size_t NUMPROVOUTPUT(std::size(m_cb_ProvOutput));

const DUALCOMBOSTR m_cb_SnapView[] =
{
	{ L"Internal",         "internal" },
	{ L"Auto",             "auto"     },
	{ L"Standard",         "standard" },
	{ L"Pixel Aspect",     "pixel"    },
	{ L"Cocktail",         "cocktail" },
};
constexpr size_t NUMSNAPVIEW(std::size(m_cb_SnapView));

const DUALCOMBOSTR m_cb_GLSLFilter[] =
{
	{ L"Plain",       "0" },
	{ L"Bilinear",    "1" },
	{ L"Bicubic",     "2" },
};
constexpr size_t NUMGLSLFILTER(std::size(m_cb_GLSLFilter));

const DUALCOMBOSTR m_cb_BGFXBackend[] =
{
	{ L"Auto",                 "auto"   },
	{ L"DirectX9",             "dx9"    },
	{ L"DirectX11",            "dx11"   },
	{ L"DirectX12 (Win10)",    "dx12"   },
	{ L"GLES",                 "gles"   },
	{ L"GLSL",                 "glsl"   },
	{ L"Metal (Win10)",        "metal"  },
	{ L"Vulkan (Win10)",       "vulkan" },
};
constexpr size_t NUMBGFXBACKEND(std::size(m_cb_BGFXBackend));

/***************************************************************
 * Public functions
 ***************************************************************/

int PropertiesCurrentGame(HWND hDlg)
{
	return g_nGame;
}

DWORD_PTR GetHelpIDs(void)
{
	return (DWORD_PTR)dwHelpIDs;
}

static PROPSHEETPAGEW *CreatePropSheetPages(HINSTANCE hInst, int nGame, UINT* pnMaxPropSheets, bool isGame)
{
	PROPSHEETPAGEW *pspages;
	int maxPropSheets = 0;
	int possiblePropSheets;
	int i = (isGame) ? 0 : 2;

	while (g_propSheets[i].pfnDlgProc)
		i++;

	possiblePropSheets = (isGame) ? i + 1 : i - 1;

	pspages = new(std::nothrow) PROPSHEETPAGEW[possiblePropSheets]{};
	if (!pspages)
		return nullptr;

	i = (isGame) ? 0 : 2;

	while (g_propSheets[i].pfnDlgProc)
	{
		if (nGame < 0)
		{
			if (g_propSheets[i].bOnDefaultPage)
			{
				pspages[maxPropSheets].dwSize = sizeof(PROPSHEETPAGEW);
				pspages[maxPropSheets].dwFlags = 0;
				pspages[maxPropSheets].hInstance = hInst;
				pspages[maxPropSheets].pszTemplate = menus::make_int_resource(g_propSheets[i].dwDlgID);
				pspages[maxPropSheets].pfnCallback = nullptr;
				pspages[maxPropSheets].lParam = 0;
				pspages[maxPropSheets].pfnDlgProc = g_propSheets[i].pfnDlgProc;
				maxPropSheets++;
			}
		}
		else if (g_propSheets[i].bOnDefaultPage && (!g_propSheets[i].pfnFilterProc || g_propSheets[i].pfnFilterProc(nGame)))
		{
			pspages[maxPropSheets].dwSize = sizeof(PROPSHEETPAGEW);
			pspages[maxPropSheets].dwFlags = 0;
			pspages[maxPropSheets].hInstance = hInst;
			pspages[maxPropSheets].pszTemplate = menus::make_int_resource(g_propSheets[i].dwDlgID);
			pspages[maxPropSheets].pfnCallback = nullptr;
			pspages[maxPropSheets].lParam = 0;
			pspages[maxPropSheets].pfnDlgProc = g_propSheets[i].pfnDlgProc;
			maxPropSheets++;
		}
		i++;
	}

	if (pnMaxPropSheets)
		*pnMaxPropSheets = maxPropSheets;

	return pspages;
}

// This is for the DEFAULT property-page options only
void InitDefaultPropertyPage(HINSTANCE hInst, HWND hWnd)
{
	// clear globals
	g_nGame = 0;
//  windows_options dummy;
//  OptionsCopy(dummy,GetDefaultOpts());
//  OptionsCopy(dummy,GetOrigOpts());
//  OptionsCopy(dummy,GetCurrentOpts());

	/* Get default options to populate property sheets */
	emu_opts.load_options(GetCurrentOpts(), SOFTWARETYPE_GLOBAL, g_nGame, false);
	emu_opts.load_options(GetOrigOpts(), SOFTWARETYPE_GLOBAL, g_nGame, false);
	emu_opts.load_options(GetDefaultOpts(), SOFTWARETYPE_GLOBAL, g_nGame, false);

	g_nPropertyMode = SOFTWARETYPE_GLOBAL;
	BuildDataMap();

	PROPSHEETHEADERW pshead{};

	PROPSHEETPAGEW *pspages = CreatePropSheetPages(hInst, -1, &pshead.nPages, false);
	if (!pspages)
		return;

	/* Fill in the property sheet header */
	pshead.hwndParent   = hWnd;
	pshead.dwSize       = sizeof(PROPSHEETHEADERW);
	pshead.dwFlags      = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_PROPTITLE;
	pshead.hInstance    = hInst;
	pshead.pszCaption   = L"Default Game";
	pshead.nStartPage   = 0;
	pshead.pszIcon      = menus::make_int_resource(IDI_MAMEUI);
	pshead.ppsp         = pspages;

	/* Create the Property sheet and display it */
	if (PropertySheetW(&pshead) == -1)
	{
		std::ostringstream dialog_message;
		DWORD dwError = system_services::get_last_error();
		dialog_message << "Property Sheet Error " << dwError << " " << std::hex << dwError;
		dialog_boxes::message_box_utf8(0, dialog_message.str().c_str(), "Error", IDOK);
	}

	delete[] pspages;
}

/* Initialize the property pages for anything but the Default option set */
void InitPropertyPage(HINSTANCE hInst, HWND hWnd, HICON hIcon, SOFTWARETYPE_OPTIONS opt_type, int folder_id, int driver_index)
{
	InitPropertyPageToPage(hInst, hWnd, hIcon, opt_type, folder_id, driver_index, PROPERTIES_PAGE);
}

void InitPropertyPageToPage(HINSTANCE hInst, HWND hWnd, HICON hIcon, SOFTWARETYPE_OPTIONS opt_type, int folder_id, int driver_index, int start_page )
{
	if (!highlight_brush)
		highlight_brush = gdi::create_solid_brush(HIGHLIGHT_COLOR);

	if (!background_brush)
		background_brush = gdi::create_solid_brush(windows::get_sys_color(COLOR_3DFACE));

	// Initialize the options
	windows_options dummy;
	emu_opts.OptionsCopy(dummy,GetDefaultOpts());
	emu_opts.OptionsCopy(dummy,GetOrigOpts());
	emu_opts.OptionsCopy(dummy,GetCurrentOpts());

	emu_opts.load_options(GetCurrentOpts(), opt_type, driver_index, true);
	emu_opts.load_options(GetOrigOpts(), opt_type, driver_index, true);
	emu_opts.load_options(GetDefaultOpts(), opt_type, driver_index, false);

	// Copy icon to use for the property pages
	g_hIcon = CopyIcon(hIcon);

	// These MUST be valid, they are used as indicies
	g_nGame = driver_index;
	g_nFolder = folder_id;

	// Keep track of SOFTWARETYPE_OPTIONS that was passed in.
	g_nPropertyMode = opt_type;

	BuildDataMap();

	PROPSHEETHEADERW pshead{};

	// Set the game to audit to this game

	// Create the property sheets
	PROPSHEETPAGEW *pspages = nullptr;

	// Get the description use as the dialog caption.
	std::wstring description;
	switch( opt_type )
	{
	case SOFTWARETYPE_GAME:
	{
		InitGameAudit(driver_index);
		pspages = CreatePropSheetPages(hInst, driver_index, &pshead.nPages, true);
		if (driver_index >= 0)
		description = mui_utf16_from_utf8string(driver_list::driver(g_nGame).type.fullname());
	}
	break;
	case SOFTWARETYPE_SOURCE:
	{
		pspages = CreatePropSheetPages(hInst, -1, &pshead.nPages, false);
		description = GetDriverFilename(g_nGame);
	}
		break;
	case SOFTWARETYPE_GLOBAL:
	{
		pspages = CreatePropSheetPages(hInst, -1, &pshead.nPages, false);
		description = L"Default Settings";
	}
	break;
	case SOFTWARETYPE_COMPUTER:
	{
		pspages = CreatePropSheetPages(hInst, -1, &pshead.nPages, false);
		description = L"Default properties for computers";
	}
	break;
	case SOFTWARETYPE_CONSOLE:
	{
		pspages = CreatePropSheetPages(hInst, -1, &pshead.nPages, false);
		description = L"Default properties for consoles";
	}
	break;
	case SOFTWARETYPE_HORIZONTAL:
	{
		pspages = CreatePropSheetPages(hInst, -1, &pshead.nPages, false);
		description = L"Default properties for horizontal screens";
	}
	break;
	case SOFTWARETYPE_RASTER:
	{
		pspages = CreatePropSheetPages(hInst, -1, &pshead.nPages, false);
		description = L"Default properties for raster machines";
	}
	break;
	case SOFTWARETYPE_VECTOR:
	{
		pspages = CreatePropSheetPages(hInst, -1, &pshead.nPages, false);
		description = L"Default properties for vector machines";
	}
	break;
	case SOFTWARETYPE_VERTICAL:
	{
		pspages = CreatePropSheetPages(hInst, -1, &pshead.nPages, false);
		description = L"Default properties for vertical screens";
	}
	break;
	default:
		return;
	}

	// If we have no property pages, or no description, return.
	if (!pspages || pshead.nPages == 0 || description.empty())
	{
		delete[] pspages;
		return;
	}

	// Fill in the property sheet header
	pshead.pszCaption = description.c_str();
	pshead.hwndParent = hWnd;
	pshead.dwSize     = sizeof(PROPSHEETHEADERW);
	pshead.dwFlags    = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_PROPTITLE;
	pshead.hInstance  = hInst;
	pshead.nStartPage = start_page;
	pshead.pszIcon    = menus::make_int_resource(IDI_MAMEUI);
	pshead.ppsp       = pspages;

	// Create the Property sheet and display it
	if (PropertySheetW(&pshead) == -1)
	{
		std::ostringstream dialog_message;
		DWORD dwError = system_services::get_last_error();
		dialog_message << "Property Sheet Error " << dwError << " " << std::hex << dwError;
		dialog_boxes::message_box_utf8(0, dialog_message.str().c_str(), "Error", IDOK);
	}

	delete[] pspages;
}

/*********************************************************************
 * Local Functions
 *********************************************************************/

/* Build CPU info string */
static std::string GameInfoCPU(int nIndex)
{
	machine_config config(driver_list::driver(nIndex), GetDefaultOpts());
	execute_interface_enumerator cpuiter(config.root_device());
	std::ostringstream cpu_info;
	std::unordered_set<std::string> exectags;

	for (device_execute_interface& exec : cpuiter)
	{
		if (!exectags.insert(exec.device().tag()).second)
			continue;

		const char* cpu_name = exec.device().name();
		int  clock_factor;
		int clock = exec.device().clock();
		int count = 1;
		std::string frequency_unit;

		for (device_execute_interface& scan : cpuiter)
		{
			const char* scan_name = scan.device().name();
			if ((exec.device().type() == scan.device().type()) && mui_strcmp(cpu_name, scan_name) && (clock == scan.device().clock()))
				if (exectags.insert(scan.device().tag()).second)
					count++;
		}

		if (count > 1)
		{
			cpu_info << count << " x " << cpu_name << " ";
		}

		if (clock >= 1000000)
		{
			frequency_unit = "MHz\r";
			clock_factor = 1000000;
		}
		else
		{
			frequency_unit = "kHz\r";
			clock_factor = 1000;
		}

		cpu_info << (clock / clock_factor) << "." << std::setprecision(6) << std::fixed << (clock % clock_factor) << frequency_unit << "\n";
	}

	return cpu_info.str();
}

/* Build Sound system info string */
static std::string GameInfoSound(int nIndex)
{
	machine_config config(driver_list::driver(nIndex), GetDefaultOpts());
	sound_interface_enumerator sounditer(config.root_device());
	std::ostringstream sound_info;
	std::unordered_set<std::string> soundtags;

	for (device_sound_interface &sound : sounditer)
	{
		if (!soundtags.insert(sound.device().tag()).second)
				continue;

		const char *spu_name = sound.device().name();
		int clock = sound.device().clock();
		int count = 1;

		for (device_sound_interface &scan : sounditer)
		{
			if (sound.device().type() == scan.device().type() && mui_strcmp(spu_name, scan.device().name()) == 0 && clock == scan.device().clock())
				if (soundtags.insert(scan.device().tag()).second)
					count++;
		}

		if (count > 1)
			sound_info << count << " x " << spu_name << " ";

		if (clock)
		{
			int  clock_factor;
			std::string frequency_unit;

			if (clock >= 1000000)
			{
				frequency_unit = "MHz\r";
				clock_factor = 1000000;
			}
			else
			{
				frequency_unit = "kHz\r";
				clock_factor = 1000;
			}

			sound_info << (clock / clock_factor) << "." << std::setprecision(6) << std::fixed << (clock % clock_factor) << frequency_unit << "\n";
		}
	}

	return sound_info.str();
}

/* Build Display info string */
static std::string GameInfoScreen(UINT nIndex)
{
	std::ostringstream screen_info;
	machine_config config(driver_list::driver(nIndex),GetCurrentOpts());

	if (isDriverVector(&config))
		screen_info << "Vector Game";
	else
	{
		screen_device_enumerator iter(config.root_device());
		const screen_device *screen = iter.first();
		if (screen == nullptr)
			screen_info << "Screenless Game";
		else
		{
			for (screen_device &screen : screen_device_enumerator(config.root_device()))
			{
				const double screen_hertz = ATTOSECONDS_TO_HZ(screen.refresh_attoseconds());
				const rectangle &visarea = screen.visible_area();
				int visarea_height, visarea_width;
				std::string screen_orientation;

				if (is_flag_set(GetDriverCacheLower(nIndex), lower_cache_flags::SWAP_XY)) //ORIENTATION_SWAP_XY
				{
					// X and Y swapped
					visarea_height = visarea.max_x - visarea.min_x + 1;
					visarea_width = visarea.max_y - visarea.min_y + 1;
					screen_orientation = " (V) ";
				}
				else
				{
					visarea_height = visarea.max_x - visarea.min_x + 1;
					visarea_width = visarea.max_y - visarea.min_y + 1;
					screen_orientation = " (H) ";
				}
				screen_info << visarea_width << " x " << visarea_height << screen_orientation << screen_hertz << " Hz" << "\n";
			}
		}
	}
	return screen_info.str();
}

/* Build game status string */
std::string GameInfoStatus(int driver_index, bool bRomStatus)
{
	if (driver_index < 0)
		return "";

	if (bRomStatus)
	{
		int audit_result = GetRomAuditResults(driver_index);
		if (IsAuditResultKnown(audit_result) == false)
			return "Unknown";

		if (!IsAuditResultYes(audit_result))
			return "BIOS missing";
	}

	std::string status_info;
	if (DriverIsBroken(driver_index))
		status_info = "Not working";
	else
		status_info = "Working";

	uint64_t cache = GetDriverCacheLower(driver_index);
	if (is_flag_set(cache, lower_cache_flags::UNEMULATED_PROTECTION)) // UNEMULATED_PROTECTION
		status_info += "\nGame protection isn't fully emulated";

	if (is_flag_set(cache, lower_cache_flags::WRONG_COLORS)) // WRONG_COLORS
		status_info += "\nColors are completely wrong";

	if (is_flag_set(cache, lower_cache_flags::IMPERFECT_COLOR)) // IMPERFECT_COLORS
		status_info += "\nColors aren't 100% accurate";

	if (is_flag_set(cache, lower_cache_flags::IMPERFECT_GRAPHICS)) // IMPERFECT_GRAPHICS
		status_info += "\nVideo emulation isn't 100% accurate";

	if (is_flag_set(cache, lower_cache_flags::NO_SOUND)) // NO_SOUND
		status_info += "\nGame lacks sound";

	if (is_flag_set(cache, lower_cache_flags::IMPERFECT_SOUND)) // IMPERFECT_SOUND
		status_info += "\nSound emulation isn't 100% accurate";

	if (is_flag_set(cache, lower_cache_flags::NO_COCKTAIL)) // NO_COCKTAIL
		status_info += "\nScreen flipping is not supported";

	if (is_flag_set(cache, lower_cache_flags::REQUIRES_ARTWORK)) // REQUIRES_ARTWORK
		status_info += "\nGame requires artwork";

	return status_info;
}

/* Build game manufacturer string */
static std::string GameInfoManufacturer(int driver_index)
{
	assert(driver_index < driver_list::total());
	std::ostringstream manufacturer_info;
	const game_driver &drv = driver_list::driver(driver_index);
	manufacturer_info << drv.year << " " << drv.manufacturer;

	return manufacturer_info.str();
}

/* Build Game title string */
std::string GameInfoTitle(SOFTWARETYPE_OPTIONS software_type, int driver_index)
{
	std::string title_info;

	switch (software_type)
	{
	case SOFTWARETYPE_GLOBAL:
		title_info = "Global game options\nDefault options used by all games";
		break;
	case SOFTWARETYPE_SOURCE:
		title_info = std::string("Properties for machines in ").append(GetDriverFilename_utf8(driver_index));
		break;
	case SOFTWARETYPE_COMPUTER:
		title_info = "Default properties for computers";
		break;
	case SOFTWARETYPE_CONSOLE:
		title_info = "Default properties for consoles";
		break;
	case SOFTWARETYPE_HORIZONTAL:
		title_info = "Default properties for horizontal screens";
		break;
	case SOFTWARETYPE_RASTER:
		title_info = "Default properties for raster machines";
		break;
	case SOFTWARETYPE_VECTOR:
		title_info = "Default properties for vector machines";
		break;
	case SOFTWARETYPE_VERTICAL:
		title_info = "Default properties for vertical screens";
		break;
	case SOFTWARETYPE_GAME:
	{
		const auto& drv = driver_list::driver(driver_index);
		title_info = std::string(drv.type.fullname()) + "\n\"" + drv.name + "\"";
		break;
	}
	default:
		break;
	}
	return title_info;
}

/* Build game clone information string */
static std::string GameInfoCloneOf(int driver_index)
{
	if (!DriverIsClone(driver_index))
		return {};

	int parent_index = GetParentIndex(&driver_list::driver(driver_index));
	if (parent_index < 0 || parent_index >= driver_list::total())
		return {};

	const auto& parent = driver_list::driver(parent_index);
	const char* fullname = parent.type.fullname();
	const char* driver_name = parent.name;

	std::ostringstream clone_info;
	clone_info << ConvertAmpersandString(fullname)
		<< " - \"" << (driver_name ? driver_name : "") << "\""
		<< "\n";

	return clone_info.str();
}

static std::string GameInfoSource(int driver_index)
{
	return GetDriverFilename_utf8(driver_index);
}

/* Handle the information property page */
INT_PTR CALLBACK GamePropertiesDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
HWND hWnd;
	switch (Msg)
	{
	case WM_INITDIALOG:
		if (g_hIcon)
			(void)dialog_boxes::send_dlg_item_message(hDlg, IDC_GAME_ICON, STM_SETICON, (WPARAM) g_hIcon, 0);
#if defined(USE_SINGLELINE_TABCONTROL)
		{
			HWND hWnd = PropSheet_GetTabControl(windows::get_parent(hDlg));
			DWORD tabStyle = (windows::get_window_long_ptr(hWnd,GWL_STYLE) & ~TCS_MULTILINE);
			(void)windows::set_window_long_ptr(hWnd,GWL_STYLE,tabStyle | TCS_SINGLELINE);
		}
#endif

		std::string window_text = GameInfoTitle(g_nPropertyMode, g_nGame);
		windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_TITLE), window_text.c_str());
		window_text = GameInfoManufacturer(g_nGame);
		windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_MANUFACTURED), window_text.c_str());
		window_text = GameInfoStatus(g_nGame, false);
		windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_STATUS), window_text.c_str());
		window_text = GameInfoCPU(g_nGame);
		windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_CPU), window_text.c_str());
		window_text = GameInfoSound(g_nGame);
		windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_SOUND), window_text.c_str());
		window_text = GameInfoScreen(g_nGame);
		windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_SCREEN), window_text.c_str());
		window_text = GameInfoCloneOf(g_nGame);
		windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_CLONEOF), window_text.c_str());
		window_text = GameInfoSource(g_nGame);
		windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_SOURCE), window_text.c_str());

		if (DriverIsClone(g_nGame))
			(void)windows::show_window(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_CLONEOF_TEXT), SW_SHOW);
		else
			(void)windows::show_window(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_CLONEOF_TEXT), SW_HIDE);

		hWnd = PropSheet_GetTabControl(windows::get_parent(hDlg));
		UpdateBackgroundBrush(hWnd);
		(void)windows::show_window(hDlg, SW_SHOW);
		return 1;
	}
	return 0;
}

/* Handle all options property pages */
INT_PTR CALLBACK GameOptionsProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	bool g_bUseDefaults = false; //, g_bReset = false;

	switch (Msg)
	{
	case WM_INITDIALOG:
		/* Fill in the Game info at the top of the sheet */
		windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_TITLE), GameInfoTitle(g_nPropertyMode, g_nGame).c_str());
		InitializeOptions(hDlg);
		InitializeMisc(hDlg);

		UpdateProperties(hDlg, properties_datamap, GetCurrentOpts());

		g_bUseDefaults = emu_opts.AreOptionsEqual(GetCurrentOpts(), GetDefaultOpts()) ? false : true;
//      g_bReset = AreOptionsEqual(GetCurrentOpts(), GetOrigOpts()) ? false : true;

		// Default button doesn't exist on Default settings
		if (g_nPropertyMode == SOFTWARETYPE_GLOBAL)
			windows::show_window(dialog_boxes::get_dlg_item(hDlg, IDC_USE_DEFAULT), SW_HIDE);
		else
		{
			(void)windows::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_USE_DEFAULT), g_bUseDefaults);

			// Setup Reset button
//          (void)windows::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_RESET), g_bReset);
			windows::show_window(hDlg, SW_SHOW);
			//      PropSheet_Changed(windows::get_parent(hDlg), hDlg);
		}
		return 1;

	case WM_HSCROLL:
		/* slider changed */
		OptOnHScroll(hDlg, (HWND)lParam, (UINT)LOWORD(wParam), (int)(short)HIWORD(wParam)); //HANDLE_WM_HSCROLL(hDlg, wParam, lParam, OptOnHScroll);
		(void)windows::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_USE_DEFAULT), true);

		// Enable Apply button
		PropSheet_Changed(windows::get_parent(hDlg), hDlg);

		// make sure everything's copied over, to determine what's changed
		UpdateOptions(hDlg, properties_datamap, GetCurrentOpts());

		// redraw it, it might be a new color now
		gdi::invalidate_rect((HWND)lParam,nullptr,TRUE);

		break;

	case WM_COMMAND:
		{
			/* Below, 'changed' is used to signify the 'Apply' button should be enabled. */
			WORD wID = LOWORD(wParam); //GET_WM_COMMAND_ID(wParam, lParam);
			HWND hWndCtrl = (HWND)lParam; //GET_WM_COMMAND_HWND(wParam, lParam);
			WORD wNotifyCode = HIWORD(wParam); //GET_WM_COMMAND_CMD(wParam, lParam);
			bool changed     = false;
			int nCurSelection = 0;

			switch (wID)
			{
			case IDC_REFRESH:
				if (wNotifyCode == LBN_SELCHANGE)
				{
					RefreshSelectionChange(hDlg, hWndCtrl);
					changed = true;
				}
				break;

			case IDC_ASPECT:
				nCurSelection = button_control::get_check(dialog_boxes::get_dlg_item(hDlg, IDC_ASPECT));
				if( g_bAutoAspect[m_currScreen+1] != nCurSelection )
				{
					changed = true;
					g_bAutoAspect[m_currScreen+1] = nCurSelection;
				}
				break;

			case IDC_SNAPSIZE:
				nCurSelection = button_control::get_check(dialog_boxes::get_dlg_item(hDlg, IDC_SNAPSIZE));
				if( g_bAutoSnapSize != nCurSelection )
				{
					changed = true;
					g_bAutoSnapSize = nCurSelection;
				}
				break;

			case IDC_SELECT_EFFECT:
				changed = SelectEffect(hDlg);
				break;

			case IDC_RESET_EFFECT:
				changed = ResetEffect(hDlg);
				break;

			case IDC_SELECT_SHADER0:
			case IDC_SELECT_SHADER1:
			case IDC_SELECT_SHADER2:
			case IDC_SELECT_SHADER3:
			case IDC_SELECT_SHADER4:
				changed = SelectGLSLShader(hDlg, (wID - IDC_SELECT_SHADER0), 0);
				break;

			case IDC_RESET_SHADER0:
			case IDC_RESET_SHADER1:
			case IDC_RESET_SHADER2:
			case IDC_RESET_SHADER3:
			case IDC_RESET_SHADER4:
				changed = ResetGLSLShader(hDlg, (wID - IDC_RESET_SHADER0), 0);
				break;

			case IDC_SELECT_SCR_SHADER0:
			case IDC_SELECT_SCR_SHADER1:
			case IDC_SELECT_SCR_SHADER2:
			case IDC_SELECT_SCR_SHADER3:
			case IDC_SELECT_SCR_SHADER4:
				changed = SelectGLSLShader(hDlg, (wID - IDC_SELECT_SCR_SHADER0), 1);
				break;

			case IDC_RESET_SCR_SHADER0:
			case IDC_RESET_SCR_SHADER1:
			case IDC_RESET_SCR_SHADER2:
			case IDC_RESET_SCR_SHADER3:
			case IDC_RESET_SCR_SHADER4:
				changed = ResetGLSLShader(hDlg, (wID - IDC_RESET_SCR_SHADER0), 1);
				break;

			case IDC_SELECT_BGFX:
				changed = SelectBGFXChains(hDlg);
				break;

			case IDC_RESET_BGFX:
				changed = ResetBGFXChains(hDlg);
				break;

			case IDC_JOYSTICKMAP:
				changed = ChangeJoystickMap(hDlg);
				break;

			case IDC_RESET_JOYSTICKMAP:
				changed = ResetJoystickMap(hDlg);
				break;

			case IDC_ARTWORK_FALLBACK:
				changed = ChangeFallback(hDlg);
				break;

			case IDC_ARTWORK_OVERRIDE:
				changed = ChangeOverride(hDlg);
				break;

			case IDC_SELECT_LUASCRIPT:
				changed = SelectLUAScript(hDlg);
				break;

			case IDC_RESET_LUASCRIPT:
				changed = ResetLUAScript(hDlg);
				break;

			case IDC_SELECT_PLUGIN:
				changed = SelectPlugins(hDlg);
				break;

			case IDC_RESET_PLUGIN:
				changed = ResetPlugins(hDlg);
				break;

//          case IDC_PROP_RESET:
				// RESET Button - Only do it if mouse-clicked
//              if (wNotifyCode != BN_CLICKED)
//                  break;

				// Change settings in property sheets back to original
//              UpdateProperties(hDlg, properties_datamap, GetOrigOpts());
				// The original options become the current options.
//              UpdateOptions(hDlg, properties_datamap, GetCurrentOpts());

//              g_bUseDefaults = AreOptionsEqual(GetCurrentOpts(), GetDefaultOpts()) ? false : true;
//              g_bReset = AreOptionsEqual(GetCurrentOpts(), GetOrigOpts()) ? false : true;
				// Turn off Apply
//              PropSheet_UnChanged(windows::get_parent(hDlg), hDlg);
//              (void)windows::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_USE_DEFAULT), g_bUseDefaults);
//              (void)windows::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_RESET), g_bReset);
//              break;

			case IDC_USE_DEFAULT:
				// DEFAULT Button - Only do it if mouse-clicked
				if (wNotifyCode != BN_CLICKED)
					break;

				// Change settings to be the same as mame.ini
				UpdateProperties(hDlg, properties_datamap, GetDefaultOpts());
				// The original options become the current options.
				UpdateOptions(hDlg, properties_datamap, GetCurrentOpts());

				g_bUseDefaults = emu_opts.AreOptionsEqual(GetCurrentOpts(), GetDefaultOpts()) ? false : true;
//              g_bReset = AreOptionsEqual(GetCurrentOpts(), GetOrigOpts()) ? false : true;
				// Enable/Disable the Reset to Defaults button
				(void)windows::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_USE_DEFAULT), g_bUseDefaults);
//              (void)windows::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_RESET), g_bReset);
				// Tell the dialog to enable/disable the apply button.
//              if (g_nPropertyMode == SOFTWARETYPE_GLOBAL)
//              {
//                  if (g_bReset)
						PropSheet_Changed(windows::get_parent(hDlg), hDlg);
//                  else
//                      PropSheet_UnChanged(windows::get_parent(hDlg), hDlg);
//              }
				break;

				// MSH 20070813 - Update all related controls
			case IDC_SCREENSELECT:
				{
					HWND hCtrl = dialog_boxes::get_dlg_item(hDlg, wID);
					if (hCtrl)
						m_currScreen = combo_box::get_cur_sel(hCtrl)-1;
				}
				[[fallthrough]];

			case IDC_SCREEN:
				// NPW 3-Apr-2007:  Ugh I'm only perpetuating the vile hacks in this code
				if ((wNotifyCode == CBN_SELCHANGE) || (wNotifyCode == CBN_SELENDOK))
				{
					changed = datamap_read_control(properties_datamap, hDlg, GetCurrentOpts(), wID);
					datamap_populate_control(properties_datamap, hDlg, GetCurrentOpts(), IDC_SIZES);
					//MSH 20070814 - Hate to do this, but its either this, or update each individual
					// control on the SCREEN tab.
					UpdateProperties(hDlg, properties_datamap, GetCurrentOpts());
					changed = true;
					/*   NOT USED *************
					datamap_populate_control(properties_datamap, hDlg, GetCurrentOpts(), IDC_SCREENSELECT);
					datamap_populate_control(properties_datamap, hDlg, GetCurrentOpts(), IDC_SCREEN);
					datamap_populate_control(properties_datamap, hDlg, GetCurrentOpts(), IDC_REFRESH);
					datamap_populate_control(properties_datamap, hDlg, GetCurrentOpts(), IDC_SIZES);
					datamap_populate_control(properties_datamap, hDlg, GetCurrentOpts(), IDC_VIEW);
					datamap_populate_control(properties_datamap, hDlg, GetCurrentOpts(), IDC_SWITCHRES);

					if (mui_strcmp(options_get_string(GetCurrentOpts(), "screen0"), options_get_string(GetOrigOpts(), "screen0")!=0) ||
					mui_strcmp(options_get_string(GetCurrentOpts(), "screen1"), options_get_string(GetOrigOpts(), "screen1")!=0) ||
					mui_strcmp(options_get_string(GetCurrentOpts(), "screen2"), options_get_string(GetOrigOpts(), "screen2")!=0) ||
					mui_strcmp(options_get_string(GetCurrentOpts(), "screen3"), options_get_string(GetOrigOpts(), "screen3")!=0))
					    changed = true;
				*************************************  */
				}
				break;
			default:
#ifdef MESS
			if (MessPropertiesCommand(hDlg, wNotifyCode, wID, &changed))
				// To Do: add a hook to MessReadMountedSoftware(drvindex); so the software will update itself when the folder is configured
					break;
#endif

				// use default behavior; try to get the result out of the datamap if
				// appropriate

				wchar_t szClass[256];
				(void)windows::get_classname(hWndCtrl, szClass, std::size(szClass));
				if (!mui_wcscmp(szClass, WC_COMBOBOX))
				{
					// combo box
					if ((wNotifyCode == CBN_SELCHANGE) || (wNotifyCode == CBN_SELENDOK))
						changed = datamap_read_control(properties_datamap, hDlg, GetCurrentOpts(), wID);
				}
				else if (!mui_wcscmp(szClass, WC_BUTTON) && (windows::get_window_long_ptr(hWndCtrl, GWL_STYLE) & BS_CHECKBOX))
				{
					// check box
					changed = datamap_read_control(properties_datamap, hDlg, GetCurrentOpts(), wID);
				}
				break;
			}

			if (changed == true)
			{
				// make sure everything's copied over, to determine what's changed
				UpdateOptions(hDlg, properties_datamap, GetCurrentOpts());
				// enable the apply button
				PropSheet_Changed(windows::get_parent(hDlg), hDlg);
				g_bUseDefaults = emu_opts.AreOptionsEqual(GetCurrentOpts(), GetDefaultOpts()) ? false : true;
//              g_bReset = AreOptionsEqual(GetCurrentOpts(), GetOrigOpts()) ? false : true;
				(void)windows::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_USE_DEFAULT), g_bUseDefaults);
//              (void)windows::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_RESET), g_bReset);
			}
		}
		break;

	case WM_NOTIFY:
		{
			// Set to true if we are exiting the properties dialog
			//bool bClosing = ((LPPSHNOTIFY) lParam)->lParam; // indicates OK was clicked rather than APPLY

			switch (((NMHDR *) lParam)->code)
			{
				//We'll need to use a CheckState Table
				//Because this one gets called for all kinds of other things too, and not only if a check is set
			case PSN_SETACTIVE:
				/* Initialize the controls. */
				UpdateProperties(hDlg, properties_datamap, GetCurrentOpts());
				g_bUseDefaults = emu_opts.AreOptionsEqual(GetCurrentOpts(), GetDefaultOpts()) ? false : true;
//              g_bReset = AreOptionsEqual(GetCurrentOpts(), GetOrigOpts()) ? false : true;
				(void)windows::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_USE_DEFAULT), g_bUseDefaults);
//              (void)windows::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_RESET), g_bReset);
				break;

			case PSN_APPLY:
				// Handle more than one PSN_APPLY, since this proc handles more than one
				// property sheet and can be called multiple times when it's time to exit,
				// and we may have already freed the windows_options.
				//if (bClosing)
				//{
					//if (nullptr == GetCurrentOpts())
					//return true;
				//}

				// Read the datamap
				UpdateOptions(hDlg, properties_datamap, GetCurrentOpts());
				// The current options become the original options.
				UpdateOptions(hDlg, properties_datamap, GetOrigOpts());

				// Repopulate the controls?  WTF?  We just read them, they should be fine.
				UpdateProperties(hDlg, properties_datamap, GetCurrentOpts());

				g_bUseDefaults = emu_opts.AreOptionsEqual(GetCurrentOpts(), GetDefaultOpts()) ? false : true;
//              g_bReset = AreOptionsEqual(GetCurrentOpts(), GetOrigOpts()) ? false : true;
				(void)windows::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_USE_DEFAULT), g_bUseDefaults);
//              (void)windows::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_RESET), g_bReset);

				// Save the current options
				emu_opts.save_options(GetCurrentOpts(), g_nPropertyMode, g_nGame);

				// Disable apply button
				PropSheet_UnChanged(windows::get_parent(hDlg), hDlg);
				(void)windows::set_window_long_ptr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

				return true;

			case PSN_KILLACTIVE:
				/* Save Changes to the options here. */
				UpdateOptions(hDlg, properties_datamap, GetCurrentOpts());
				// Determine button states.
				g_bUseDefaults = emu_opts.AreOptionsEqual(GetCurrentOpts(), GetDefaultOpts()) ? false : true;
//              g_bReset = AreOptionsEqual(GetCurrentOpts(), GetOrigOpts()) ? false : true;
				(void)windows::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_USE_DEFAULT), g_bUseDefaults);
//              (void)windows::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_RESET), g_bReset);

				ResetDataMap(hDlg);
				(void)windows::set_window_long_ptr(hDlg, DWLP_MSGRESULT, false);
				return 1;

//          case PSN_RESET:
				// Reset to the original values. Disregard changes
				//GetCurrentOpts() = GetOrigOpts();
//              g_bUseDefaults = AreOptionsEqual(GetCurrentOpts(), GetDefaultOpts()) ? false : true;
//              g_bReset = AreOptionsEqual(GetCurrentOpts(), GetOrigOpts()) ? false : true;
//              (void)windows::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_USE_DEFAULT), g_bUseDefaults);
//              (void)windows::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_RESET), g_bReset);
//              (void)windows::set_window_long_ptr(hDlg, DWLP_MSGRESULT, false);
//              break;

			case PSN_HELP:
				// User wants help for this property page
				break;
			}
		}
		break;

	case WM_HELP:
		/* User clicked the ? from the upper right on a control */
		(void)HelpFunction((HWND)((LPHELPINFO)lParam)->hItemHandle, MAMEUICONTEXTHELP, HH_TP_HELP_WM_HELP, GetHelpIDs());
		break;

	case WM_CONTEXTMENU:
		(void)HelpFunction((HWND)wParam, MAMEUICONTEXTHELP, HH_TP_HELP_CONTEXTMENU, GetHelpIDs());
		break;

	}

	return 0;
}

/* Read controls that are not handled in the DataMap */
static void PropToOptions(HWND hWnd, windows_options &options)
{
	HWND hCtrl;
	HWND hCtrl2;
	HWND hCtrl3;
	int win_text_len;
	const int win_text_size = 200;
	wchar_t win_text_buf[win_text_size];
	wchar_t* win_text_end;

	/* aspect ratio */
	hCtrl  = dialog_boxes::get_dlg_item(hWnd, IDC_ASPECTRATION);
	hCtrl2 = dialog_boxes::get_dlg_item(hWnd, IDC_ASPECTRATIOD);
	hCtrl3 = dialog_boxes::get_dlg_item(hWnd, IDC_ASPECT);
	if (hCtrl && hCtrl2 && hCtrl3)
	{
		std::string aspect_option = std::string("aspect");
		if (m_currScreen >= 0)
			aspect_option += std::to_string(m_currScreen);

		if (button_control::get_check(hCtrl3))
		{
			emu_opts.emu_set_value(options, aspect_option, "auto");
		}
		else
		{
			int n = 0, d = 0;

			win_text_len = GetWindowTextW(hCtrl, win_text_buf, win_text_size);
			if (win_text_len)
			{
				win_text_end = win_text_buf + win_text_len;
				n = wcstol(win_text_buf, &win_text_end, 10);
			}

			win_text_len = GetWindowTextW(hCtrl2, win_text_buf, win_text_size);
			if (win_text_len)
			{
				win_text_end = win_text_buf + win_text_len;
				d = wcstol(win_text_buf, &win_text_end, 10);
			}

			if (n == 0 || d == 0)
			{
				n = 4;
				d = 3;
			}
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(2) << static_cast<double>(n) / d;
			emu_opts.emu_set_value(options, aspect_option, oss.str());
		}
	}
	/* snapshot size */
	hCtrl  = dialog_boxes::get_dlg_item(hWnd, IDC_SNAPSIZEWIDTH);
	hCtrl2 = dialog_boxes::get_dlg_item(hWnd, IDC_SNAPSIZEHEIGHT);
	hCtrl3 = dialog_boxes::get_dlg_item(hWnd, IDC_SNAPSIZE);
	if (hCtrl && hCtrl2 && hCtrl3)
	{
		if (button_control::get_check(hCtrl3))
		{
			emu_opts.emu_set_value(options, OPTION_SNAPSIZE, "auto");
		}
		else
		{
			int width = 0, height = 0, win_text_len;

			win_text_len = GetWindowTextW(hCtrl, win_text_buf, win_text_size);
			if (win_text_len)
			{
				win_text_end = win_text_buf + win_text_len;
				width = wcstol(win_text_buf, &win_text_end, 10);
			}

			win_text_len = GetWindowTextW(hCtrl2, win_text_buf, win_text_size);
			if (win_text_len)
			{
				win_text_end = win_text_buf + win_text_len;
				height = wcstol(win_text_buf, &win_text_end, 10);
			}

			if (width == 0 || height == 0)
			{
				width = 640;
				height = 480;
			}
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(0) << width << "x" << height;
			emu_opts.emu_set_value(options, OPTION_SNAPSIZE, oss.str());
		}
	}
}

/* Update options from the dialog */
static void UpdateOptions(HWND hDlg, datamap *map, windows_options &options)
{
	/* These are always called together, so make one convenient function. */
	datamap_read_all_controls(map, hDlg, options);
	PropToOptions(hDlg, options);
}

/* Update the dialog from the options */
static void UpdateProperties(HWND hDlg, datamap *map, windows_options &options)
{
	/* These are always called together, so make one convenient function. */
	datamap_populate_all_controls(map, hDlg, options);
	OptionsToProp(hDlg, options);
	SetPropEnabledControls(hDlg);
}

/* Note with shaders:
    MAME shader has *.vsh, plus *_rgb32_dir.fsh
    SCREEN shader has *.vsh, plus *.fsh
    In both cases, the vsh is entered into the ini-file, but without the extension */
static bool SelectGLSLShader(HWND hWnd, int slot, bool is_scr)
{
	wchar_t filename[MAX_PATH]{ L'\0' };
	bool changed = false;
	std::ostringstream shader;
	int dialog;

	if (is_scr)
	{
		dialog = IDC_SCREEN_SHADER0 + slot;
		shader << "glsl_shader_screen" << slot;
	}
	else
	{
		dialog = IDC_MAME_SHADER0 + slot;
		shader << "glsl_shader_mame" << slot;
	}

	if (CommonFileDialog(dialog_boxes::get_open_filename, filename, FILETYPE_SHADER_FILES))
	{
		const char* option_value = GetCurrentOpts().value(shader.str());
		std::string shader_name = std::filesystem::path(filename).stem().string();
		if (mui_strcmp(shader_name, option_value) != 0)
		{
			emu_opts.emu_set_value(GetCurrentOpts(), shader.str(), shader_name);
			windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hWnd, dialog), shader_name.c_str());
			changed = true;
		}
	}

	return changed;
}

static bool ResetGLSLShader(HWND hWnd, int slot, bool is_scr)
{
	bool changed = false;
	const char *new_value = "none";
	int dialog = (is_scr ? IDC_SCREEN_SHADER0 : IDC_MAME_SHADER0) + slot;

	std::ostringstream oss;
	oss << (is_scr ? "glsl_shader_screen" : "glsl_shader_mame") << slot;

	if (mui_strcmp(new_value, GetCurrentOpts().value(oss.str())) != 0)
	{
		emu_opts.emu_set_value(GetCurrentOpts(), oss.str(), new_value);
		windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hWnd, dialog), "None");
		changed = true;
	}

	return changed;
}

static void UpdateMameShader(HWND hWnd, int slot, windows_options &options)
{
	HWND hCtrl = dialog_boxes::get_dlg_item(hWnd, IDC_MAME_SHADER0 + slot);

	if (hCtrl)
	{
		std::ostringstream oss;
		oss << "glsl_shader_mame" << slot;
		const char* value = options.value(oss.str());

		if (mui_strcmp(value, "none") == 0)
			windows::set_window_text_utf8(hCtrl, "None");
		else
			windows::set_window_text_utf8(hCtrl, value);
	}
}

static void UpdateScreenShader(HWND hWnd, int slot, windows_options &options)
{
	HWND hCtrl = dialog_boxes::get_dlg_item(hWnd, IDC_SCREEN_SHADER0 + slot);

	if (hCtrl)
	{
		std::ostringstream oss;
		oss << "glsl_shader_screen" << slot;
		const char* value = options.value(oss.str());

		if (mui_strcmp(value, "none") == 0)
			windows::set_window_text_utf8(hCtrl, "None");
		else
			windows::set_window_text_utf8(hCtrl, value);
	}
}

/* Populate controls that are not handled in the DataMap */
static void OptionsToProp(HWND hWnd, windows_options& options)
{
	HWND hCtrl2;
	int  n = 0;
	int  d = 0;
	int  width = 0;
	int  height = 0;
	std::string c;

	/* video */

	/* Setup refresh list based on depth. */
	datamap_update_control(properties_datamap, hWnd, GetCurrentOpts(), IDC_REFRESH);
	/* Setup Select screen*/
	UpdateSelectScreenUI(hWnd );

	HWND hCtrl = dialog_boxes::get_dlg_item(hWnd, IDC_ASPECT);
	if (hCtrl)
		button_control::set_check(hCtrl, g_bAutoAspect[m_currScreen+1] );

	hCtrl = dialog_boxes::get_dlg_item(hWnd, IDC_SNAPSIZE);
	if (hCtrl)
		button_control::set_check(hCtrl, g_bAutoSnapSize );

	/* Bios select list */
	hCtrl = dialog_boxes::get_dlg_item(hWnd, IDC_BIOS);
	if (hCtrl)
	{
		const char* cBuffer;
		for( uint8_t i = 0; i <combo_box::get_count( hCtrl ); i++ )
		{
			cBuffer = (const char*)combo_box::get_item_data( hCtrl, i );
			if (mui_strcmp(cBuffer, GetCurrentOpts().value(OPTION_BIOS) ) == 0)
			{
				combo_box::set_cur_sel(hCtrl, i);
				break;
			}

		}
	}

	hCtrl = dialog_boxes::get_dlg_item(hWnd, IDC_ASPECT);
	if (hCtrl)
	{
		std::string aspect_option = "aspect";
		if (m_currScreen >= 0)
			aspect_option += std::to_string(m_currScreen);
		std::string aspect = emu_opts.emu_get_value(options, aspect_option);
		if( aspect == "auto")
		{
			button_control::set_check(hCtrl, true);
			g_bAutoAspect[m_currScreen+1] = true;
		}
		else
		{
			button_control::set_check(hCtrl, false);
			g_bAutoAspect[m_currScreen+1] = false;
		}
	}

	/* aspect ratio */
	hCtrl  = dialog_boxes::get_dlg_item(hWnd, IDC_ASPECTRATION);
	hCtrl2 = dialog_boxes::get_dlg_item(hWnd, IDC_ASPECTRATIOD);
	if (hCtrl && hCtrl2)
	{
		std::string aspect_option = "aspect";
		if (m_currScreen >= 0)
			aspect_option += std::to_string(m_currScreen);
		std::string aspect = emu_opts.emu_get_value(options, aspect_option);

		n = 0;
		d = 0;
		if (!aspect.empty())
		{
			std::istringstream iss(aspect);
			iss >> std::fixed >> n;
			if (n < 1 || n >100) n = 4;

			iss.ignore(1); // ignore the ':' character
			iss >> std::fixed >> d;
			if (d < 1 || d>100) d = 3;

			windows::set_window_text(hCtrl, std::to_wstring(n).c_str());
			windows::set_window_text(hCtrl2, std::to_wstring(d).c_str());
		}
		else
		{
			windows::set_window_text(hCtrl, L"4");
			windows::set_window_text(hCtrl2, L"3");
		}
	}

	hCtrl = dialog_boxes::get_dlg_item(hWnd, IDC_EFFECT);
	if (hCtrl)
	{
		c = emu_opts.emu_get_value(options, OPTION_EFFECT);
		if (c.empty())
		{
			c = "none";
			emu_opts.emu_set_value(options, OPTION_EFFECT, c);
		}
		windows::set_window_text_utf8(hCtrl, c.c_str());
	}

	hCtrl = dialog_boxes::get_dlg_item(hWnd, IDC_SNAPSIZE);
	if (hCtrl)
	{
		if(emu_opts.emu_get_value(options, OPTION_SNAPSIZE) == "auto")
		{
			button_control::set_check(hCtrl, true);
			g_bAutoSnapSize = true;
		}
		else
		{
			button_control::set_check(hCtrl, false);
			g_bAutoSnapSize = false;
		}
	}

	for (size_t i = 0; i < 5; i++)
	{
		UpdateMameShader(hWnd, i, options);
		UpdateScreenShader(hWnd, i, options);
	}

	hCtrl = dialog_boxes::get_dlg_item(hWnd, IDC_JOYSTICKMAP);

	if (hCtrl)
	{
		c = emu_opts.emu_get_value(options, OPTION_JOYSTICK_MAP);

		if (c.empty())
			windows::set_window_text_utf8(hCtrl, "Default");
		else
			windows::set_window_text_utf8(hCtrl, c.c_str());
	}

	hCtrl = dialog_boxes::get_dlg_item(hWnd, IDC_LUASCRIPT);

	if (hCtrl)
	{
		c = emu_opts.emu_get_value(options, OPTION_AUTOBOOT_SCRIPT);

		if (c.empty())
			windows::set_window_text_utf8(hCtrl, "None");
		else
		{
			std::filesystem::path file_path(c);
			std::string window_text = file_path.stem().string();
			windows::set_window_text_utf8(hCtrl, window_text.c_str());
		}
	}

	hCtrl = dialog_boxes::get_dlg_item(hWnd, IDC_PLUGIN);

	if (hCtrl)
	{
		c = emu_opts.emu_get_value(options, OPTION_PLUGIN);

		if (c.empty())
			windows::set_window_text_utf8(hCtrl, "None");
		else
			windows::set_window_text_utf8(hCtrl, c.c_str());
	}

	hCtrl = dialog_boxes::get_dlg_item(hWnd, IDC_BGFX_CHAINS);

	if (hCtrl)
	{
		c = emu_opts.emu_get_value(options, OSDOPTION_BGFX_SCREEN_CHAINS);

		if (c.empty())
			windows::set_window_text_utf8(hCtrl, "Default");
		else
			windows::set_window_text_utf8(hCtrl, c.c_str());
	}

	/* snapshot size */
	hCtrl  = dialog_boxes::get_dlg_item(hWnd, IDC_SNAPSIZEWIDTH);
	hCtrl2 = dialog_boxes::get_dlg_item(hWnd, IDC_SNAPSIZEHEIGHT);
	if (hCtrl && hCtrl2)
	{
		n = 0;
		d = 0;
		if (options.value(OPTION_SNAPSIZE))
		{
			std::istringstream iss(options.value(OPTION_SNAPSIZE));
			iss >> std::fixed >> n;
			iss.ignore(1); // ignore the 'x' character
			iss >> std::fixed >> d;
			if (n == 0 || d == 0)
			{
				n = 640;
				d = 480;
			}

			windows::set_window_text(hCtrl, std::to_wstring(width).c_str());
			windows::set_window_text(hCtrl2, std::to_wstring(height).c_str());
		}
		else
		{
			windows::set_window_text(hCtrl, L"640");
			windows::set_window_text(hCtrl2, L"480");
		}
	}
}

/* Adjust controls - tune them to the currently selected game */
static void SetPropEnabledControls(HWND hWnd)
{
#if 0
	bool useart = true;
	bool joystick_attached = true;
	bool in_window = false;
	int nIndex = g_nGame;

	// auto is a reserved word
	bool autov = (mui_stricmp(GetCurrentOpts().value(OSDOPTION_VIDEO), "auto")==0);
	bool d3d = (mui_stricmp(GetCurrentOpts().value(OSDOPTION_VIDEO), "d3d")==0) | autov;
	in_window = GetCurrentOpts().bool_value(OSDOPTION_WINDOW);
	button_control::set_check(dialog_boxes::get_dlg_item(hWnd, IDC_ASPECT), g_bAutoAspect[m_currScreen+1] );
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_WAITVSYNC), d3d);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_TRIPLE_BUFFER), d3d);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_PRESCALE), d3d);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_PRESCALEDISP), d3d);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_PRESCALETEXT), d3d);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_SWITCHRES), true);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_SYNCREFRESH), true);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_REFRESH), !in_window);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_REFRESHTEXT), !in_window);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_FSGAMMA), !in_window);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_FSGAMMATEXT), !in_window);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_FSGAMMADISP), !in_window);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_FSBRIGHTNESS), !in_window);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_FSBRIGHTNESSTEXT), !in_window);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_FSBRIGHTNESSDISP), !in_window);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_FSCONTRAST), !in_window);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_FSCONTRASTTEXT), !in_window);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_FSCONTRASTDISP), !in_window);

	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_ASPECTRATIOTEXT), !g_bAutoAspect[m_currScreen+1]);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_ASPECTRATION), !g_bAutoAspect[m_currScreen+1]);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_ASPECTRATIOD), !g_bAutoAspect[m_currScreen+1]);

	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_SNAPSIZETEXT), !g_bAutoSnapSize);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_SNAPSIZEHEIGHT), !g_bAutoSnapSize);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_SNAPSIZEWIDTH), !g_bAutoSnapSize);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_SNAPSIZEX), !g_bAutoSnapSize);

	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_D3D_FILTER),d3d);

	//Switchres and D3D or ddraw enable the per screen parameters
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_NUMSCREENS), ddraw | d3d);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_NUMSCREENSDISP), ddraw | d3d);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_SCREENSELECT), ddraw | d3d);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_SCREENSELECTTEXT), ddraw | d3d);

	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_ARTWORK_CROP), useart);
//  (void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_BACKDROPS), useart);
//  (void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_BEZELS), useart);
//  (void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_OVERLAYS), useart);
//  (void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_CPANELS), useart);
//  (void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_MARQUEES), useart);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_ARTMISCTEXT), useart);

	/* Joystick options */
	joystick_attached = DIJoystick.Available();

	button_control::enable(dialog_boxes::get_dlg_item(hWnd,IDC_JOYSTICK), joystick_attached);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_JDZTEXT),  joystick_attached);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_JDZDISP),  joystick_attached);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_JDZ),      joystick_attached);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_JSATTEXT), joystick_attached);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_JSATDISP), joystick_attached);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_JSAT),     joystick_attached);
	/* Trackball / Mouse options */
	if (nIndex <= -1 || DriverUsesTrackball(nIndex) || DriverUsesLightGun(nIndex))
		button_control::enable(dialog_boxes::get_dlg_item(hWnd,IDC_USE_MOUSE),TRUE);
	else
		button_control::enable(dialog_boxes::get_dlg_item(hWnd,IDC_USE_MOUSE),FALSE);

	if (!in_window && (nIndex <= -1 || DriverUsesLightGun(nIndex)))
	{
		// on WinXP the Lightgun and Dual Lightgun switches are no longer supported use mouse instead
		OSVERSIONINFOEX osvi;
		bool bOsVersionInfoEx;
		// Try calling GetVersionEx using the OSVERSIONINFOEX structure.
		// If that fails, try using the OSVERSIONINFO structure.

		osvi = {};
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

		if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
		{
			osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
			bOsVersionInfoEx = GetVersionEx ( (OSVERSIONINFO *) &osvi);
		}

	}
	else
	{
		button_control::enable(dialog_boxes::get_dlg_item(hWnd,IDC_LIGHTGUN),FALSE);
		button_control::enable(dialog_boxes::get_dlg_item(hWnd,IDC_DUAL_LIGHTGUN),FALSE);
	}


	/* Sound options */
	bool sound = (mui_stricmp(GetCurrentOpts().value(OSDOPTION_SOUND), "none") != 0);
	ComboBox_Enable(dialog_boxes::get_dlg_item(hWnd, IDC_SAMPLERATE), sound);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd,IDC_VOLUME),sound);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd,IDC_RATETEXT),sound);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd,IDC_VOLUMEDISP),sound);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd,IDC_VOLUMETEXT),sound);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd,IDC_AUDIO_LATENCY),sound);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd,IDC_AUDIO_LATENCY_DISP),sound);
	(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd,IDC_AUDIO_LATENCY_TEXT),sound);
	SetSamplesEnabled(hWnd, nIndex, sound);

	if (button_control::get_check(dialog_boxes::get_dlg_item(hWnd, IDC_AUTOFRAMESKIP)))
		(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_FRAMESKIP), false);
	else
		(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd, IDC_FRAMESKIP), 1);

	if (nIndex <= -1 || DriverHasOptionalBIOS(nIndex))
		(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd,IDC_BIOS),TRUE);
	else
		(void)windows::enable_window(dialog_boxes::get_dlg_item(hWnd,IDC_BIOS),FALSE);
#endif
}

//============================================================
//  CONTROL HELPER FUNCTIONS FOR DATA EXCHANGE
//============================================================

static bool RotateReadControl(datamap *map, HWND dialog, HWND control, windows_options *options, std::string option_name)
{
	int selected_index = combo_box::get_cur_sel(control);
	int original_selection = 0;

	// Figure out what the original selection value is
	if (options->bool_value(OPTION_ROR) && !options->bool_value(OPTION_ROL))
		original_selection = 1;
	else if (!options->bool_value(OPTION_ROR) && options->bool_value(OPTION_ROL))
		original_selection = 2;
	else if (!options->bool_value(OPTION_ROTATE))
		original_selection = 3;
	else if (options->bool_value(OPTION_AUTOROR))
		original_selection = 4;
	else if (options->bool_value(OPTION_AUTOROL))
		original_selection = 5;

	// Any work to do?  If so, make the changes and return true.
	if (selected_index != original_selection)
	{
		// Set the options based on the new selection.
		emu_opts.emu_set_value(options, OPTION_ROR, selected_index == 1);
		emu_opts.emu_set_value(options, OPTION_ROL, selected_index == 2);
		emu_opts.emu_set_value(options, OPTION_ROTATE, selected_index != 3);
		emu_opts.emu_set_value(options, OPTION_AUTOROR, selected_index == 4);
		emu_opts.emu_set_value(options, OPTION_AUTOROL, selected_index == 5);
		return true;
	}

	// No changes
	return false;
}



static bool RotatePopulateControl(datamap *map, HWND dialog, HWND control, windows_options *options, std::string option_name)
{
	int selected_index = 0;
	if (options->bool_value(OPTION_ROR) && !options->bool_value(OPTION_ROL))
		selected_index = 1;
	else if (!options->bool_value(OPTION_ROR) && options->bool_value(OPTION_ROL))
		selected_index = 2;
	else if (!options->bool_value(OPTION_ROTATE))
		selected_index = 3;
	else if (options->bool_value(OPTION_AUTOROR))
		selected_index = 4;
	else if (options->bool_value(OPTION_AUTOROL))
		selected_index = 5;

	combo_box::set_cur_sel(control, selected_index);
	return false;
}



static bool ScreenReadControl(datamap *map, HWND dialog, HWND control, windows_options *options, std::string option_name)
{
	std::string screen_option_name = "screen";
	if (m_currScreen >= 0)
		screen_option_name += std::to_string(m_currScreen);
	int screen_option_index = combo_box::get_cur_sel(control);
	LPCTSTR screen_option_value = (LPCTSTR) combo_box::get_item_data(control, screen_option_index);
	std::unique_ptr<char[]> op_val(mui_utf8_from_utf16cstring(screen_option_value));
	emu_opts.emu_set_value(options, screen_option_name, op_val.get());
	return false;
}



static bool ScreenPopulateControl(datamap *map, HWND dialog, HWND control, windows_options *options, std::string option_name)
{
	//int iMonitors;
	DISPLAY_DEVICEW dd;
	int i = 0;
	int nSelection = 0;

	/* Remove all items in the list. */
	combo_box::reset_content(control);
	combo_box::insert_string(control, 0, (LPARAM)L"Auto");
	combo_box::set_item_data(control, 0, (LPARAM)L"auto");

	//Dynamically populate it, by enumerating the Monitors
	//iMonitors = windows::get_system_metrics(SM_CMONITORS); // this gets the count of monitors attached
	dd = {};
	dd.cb = sizeof(dd);
	for(i=0; gdi::enum_display_devices(nullptr, i, &dd, 0); i++)
	{
		if( !(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) )
		{
			//we have to add 1 to account for the "auto" entry
			combo_box::insert_string(control, i+1, (LPARAM)dd.DeviceName);
			combo_box::set_item_data(control, i+1, (LPARAM)dd.DeviceName);

			std::string screen_option = "screen";
			if (m_currScreen >= 0)
				screen_option += std::to_string(m_currScreen);

			std::unique_ptr<char[]> utf8DeviceName(mui_utf8_from_utf16cstring(dd.DeviceName));
			std::string screen = emu_opts.emu_get_value(options, screen_option);
			if (!mui_strcmp(screen.c_str(), utf8DeviceName.get()))
				nSelection = i+1;
		}
	}
	combo_box::set_cur_sel(control, nSelection);
	return false;
}

static std::string ViewSetOptionName(datamap *map, HWND dialog, HWND control)
{
	std::string view_optionname = "view";

	if (m_currScreen >= 0)
		view_optionname += std::to_string(m_currScreen);

	return view_optionname;
}

static bool ViewPopulateControl(datamap *map, HWND dialog, HWND control, windows_options *options, std::string option_name)
{
	int selected_index = 0;

	// determine the view option value
	std::string view_option = "view";
	if (m_currScreen >= 0)
		view_option += std::to_string(m_currScreen);
	std::string view = emu_opts.emu_get_value(options, view_option);

	combo_box::reset_content(control);
	for (size_t i = 0; i < NUMVIEW; i++)
	{
		combo_box::insert_string(control, i, (LPARAM)m_cb_View[i].m_pText);
		combo_box::set_item_data(control, i, (LPARAM)m_cb_View[i].m_pData);

		if (mui_strcmp(view.c_str(), m_cb_View[i].m_pData)==0)
			selected_index = i;
	}
	combo_box::set_cur_sel(control, selected_index);
	return false;
}

static bool SnapViewPopulateControl(datamap *map, HWND dialog, HWND control, windows_options *options, std::string option_name)
{
	int selected_index = 0;

	// determine the snapview option value
	const char *snapview = options->value(OPTION_SNAPVIEW);

	combo_box::reset_content(control);
	for (size_t i = 0; i < NUMSNAPVIEW; i++)
	{
		combo_box::insert_string(control, i, (LPARAM)m_cb_SnapView[i].m_pText);
		combo_box::set_item_data(control, i, (LPARAM)m_cb_SnapView[i].m_pData);

		if (mui_strcmp(snapview, m_cb_SnapView[i].m_pData)==0)
			selected_index = i;
	}
	combo_box::set_cur_sel(control, selected_index);
	return false;
}

static bool DefaultInputReadControl(datamap *map, HWND dialog, HWND control, windows_options *options, std::string option_name)
{
	int input_option_index = combo_box::get_cur_sel(control);
	const char *input_option_value = (const char*)combo_box::get_item_data(control, input_option_index);
	emu_opts.emu_set_value(options, OPTION_CTRLR, input_option_index ? input_option_value : "");
	return false;
}

static bool DefaultInputPopulateControl(datamap *map, HWND dialog, HWND control, windows_options *options, std::string option_name)
{
	WIN32_FIND_DATAW FindFileData;
	int selected = 0;
	int index = 0;

	// determine the ctrlr option
	const char *ctrlr_option = options->value(OPTION_CTRLR);

	// reset the controllers dropdown
	(void)combo_box::reset_content(control);
	(void)combo_box::insert_string(control, index, (LPARAM)L"Default");
	(void)combo_box::set_item_data(control, index, (LPARAM)L"");
	index++;

	auto search_path = std::filesystem::path(emu_opts.dir_get_value(DIRPATH_CTRLRPATH)) / "*.*";
	std::wstring utf16_search_path = search_path.wstring();
	HANDLE hFind = storage::find_first_file(utf16_search_path.c_str(), &FindFileData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		while (storage::find_next_file(hFind, &FindFileData) != 0)
		{
			// copy the filename
			auto filename = std::filesystem::path(FindFileData.cFileName);

			if (filename.has_extension())
			{
				// check if it's a cfg file
				if (!mui_strcmp(filename.extension().string(), ".cfg"))
				{
					// set the option?
					if (!mui_strcmp(filename.stem().string(), ctrlr_option))
						selected = index;

					// add it as an option
					std::wstring utf16_stem = filename.stem().wstring();
					combo_box::insert_string(control, index, (LPARAM)utf16_stem.c_str());
					combo_box::set_item_data(control, index, (LPARAM)utf16_stem.c_str());
					index++;
				}
			}
		}

		FindClose (hFind);
	}

	combo_box::set_cur_sel(control, selected);

	return false;
}

static std::string ResolutionSetOptionName(datamap* map, HWND dialog, HWND control)
{
	std::string resolution_optionname = "resolution";

	if (m_currScreen >= 0)
		resolution_optionname += std::to_string(m_currScreen);

	return resolution_optionname;
}

static bool ResolutionReadControl(datamap *map, HWND dialog, HWND control, windows_options *options, std::string option_name)
{
	HWND refresh_control = dialog_boxes::get_dlg_item(dialog, IDC_REFRESH);
	HWND sizes_control = dialog_boxes::get_dlg_item(dialog, IDC_SIZES);
	int width = 0, height = 0;

	if (refresh_control && sizes_control)
	{
		std::unique_ptr<wchar_t[]> resolution_text = std::unique_ptr<wchar_t[]>(windows::get_window_text(sizes_control));
		std::wistringstream iss(resolution_text.get());
		iss >> std::fixed >> width;
		iss.ignore(1); // ignore the 'x' character
		iss >> std::fixed >> height;
		if (width == 0 || height == 0)
		{
			emu_opts.emu_set_value(options, option_name, "auto");
		}
		else
		{
			int refresh_index = combo_box::get_cur_sel(refresh_control);
			if (refresh_index == CB_ERR)
				refresh_index = 0;

			int refresh_value = combo_box::get_item_data(refresh_control, refresh_index);
			if (refresh_value == CB_ERR)
				return false;

			std::ostringstream oss;
			oss << width << "x" << height << "@" << refresh_value;
			emu_opts.emu_set_value(options, option_name, oss.str());
		}

	}
	return false;
}

static bool ResolutionPopulateControl(datamap *map, HWND dialog, HWND control_, windows_options *options, std::string option_name)
{
	HWND sizes_control = dialog_boxes::get_dlg_item(dialog, IDC_SIZES);
	HWND refresh_control = dialog_boxes::get_dlg_item(dialog, IDC_REFRESH);
	int width, height, refresh;
	const char *option_value;
	int sizes_index = 0;
	int refresh_index = 0;
	int sizes_selection = 0;
	int refresh_selection = 0;
	std::string screen_option;
	std::unique_ptr<wchar_t[]> wcs_screen;
	int i;
	DEVMODE devmode;

	if (sizes_control && refresh_control)
	{
		// determine the resolution
		option_value = options->value(option_name);
		if (sscanf(option_value, "%dx%d@%d", &width, &height, &refresh) != 3)
		{
			width = 0;
			height = 0;
			refresh = 0;
		}

		// reset sizes control
		combo_box::reset_content(sizes_control);
		combo_box::insert_string(sizes_control, sizes_index, (LPARAM)L"Auto");
		combo_box::set_item_data(sizes_control, sizes_index, 0);
		sizes_index++;

		// reset refresh control
		combo_box::reset_content(refresh_control);
		combo_box::insert_string(refresh_control, refresh_index, (LPARAM)L"Auto");
		combo_box::set_item_data(refresh_control, refresh_index, 0);
		refresh_index++;

		// determine which screen we're using
		std::string screen_option = "screen";
		if (m_currScreen >= 0)
			screen_option += std::to_string(m_currScreen);

		if (screen_option != "screen")
		{
			std::string screen = emu_opts.emu_get_value(options, screen_option);
			wcs_screen = std::unique_ptr<wchar_t[]>(mui_utf16_from_utf8cstring(screen.c_str()));
		}

		// retrieve screen information
		devmode.dmSize = sizeof(devmode);
		for (i = 0; gdi::enum_display_settings(wcs_screen.get(), i, &devmode); i++)
		{
			if ((devmode.dmBitsPerPel == 32 ) // Only 32 bit depth supported by core
				&&(devmode.dmDisplayFrequency == refresh || refresh == 0))
			{
				std::wostringstream wss_resolution;

				wss_resolution << devmode.dmPelsWidth << L" x " << devmode.dmPelsHeight;
				if (combo_box::find_string(sizes_control, 0, (LPARAM)&wss_resolution.str()[0]) == CB_ERR)
				{
					combo_box::insert_string(sizes_control, sizes_index, (LPARAM)&wss_resolution.str()[0]);

					if ((width == devmode.dmPelsWidth) && (height == devmode.dmPelsHeight))
						sizes_selection = sizes_index;
					sizes_index++;

				}
			}
			if (devmode.dmDisplayFrequency >= 10 )
			{
				// I have some devmode "vga" which specifes 1 Hz, which is probably bogus, so we filter it out
				std::wostringstream wss_refresh_rate;
				wss_refresh_rate << devmode.dmDisplayFrequency << L" Hz";

				if (combo_box::find_string(refresh_control, 0, (LPARAM)&wss_refresh_rate.str()[0]) == CB_ERR)
				{
					combo_box::insert_string(refresh_control, refresh_index, (LPARAM)&wss_refresh_rate.str()[0]);
					combo_box::set_item_data(refresh_control, refresh_index, (LPARAM)devmode.dmDisplayFrequency);

					if (refresh == devmode.dmDisplayFrequency)
						refresh_selection = refresh_index;

					refresh_index++;
				}
			}
		}

		combo_box::set_cur_sel(sizes_control, sizes_selection);
		combo_box::set_cur_sel(refresh_control, refresh_selection);
	}
	return false;
}

/************************************************************
 * DataMap initializers
 ************************************************************/

/* Initialize local helper variables */
static void ResetDataMap(HWND hWnd)
{
	std::string screen_option = "screen";
	if (m_currScreen >= 0)
		screen_option += std::to_string(m_currScreen);

	std::string screen = emu_opts.emu_get_value(GetCurrentOpts(), screen_option);

	if (screen.empty() || (screen == "auto"))
		emu_opts.emu_set_value(GetCurrentOpts(), screen_option, "auto");
}


/* Build the control mapping by adding all needed information to the DataMap */
static void BuildDataMap(void)
{
	properties_datamap = datamap_create();

	// core state options
	datamap_add(properties_datamap, IDC_ENABLE_AUTOSAVE,        DM_BOOL,    OPTION_AUTOSAVE);
	datamap_add(properties_datamap, IDC_SNAPVIEW,               DM_STRING,  OPTION_SNAPVIEW);
	datamap_add(properties_datamap, IDC_SNAPSIZEWIDTH,          DM_STRING,  "");
	datamap_add(properties_datamap, IDC_SNAPSIZEHEIGHT,         DM_STRING,  "");

	// core performance options
	datamap_add(properties_datamap, IDC_AUTOFRAMESKIP,          DM_BOOL,    OPTION_AUTOFRAMESKIP);
	datamap_add(properties_datamap, IDC_FRAMESKIP,              DM_INT,     OPTION_FRAMESKIP);
	datamap_add(properties_datamap, IDC_SECONDSTORUN,           DM_INT,     OPTION_SECONDS_TO_RUN);
	datamap_add(properties_datamap, IDC_SECONDSTORUNDISP,       DM_INT,     OPTION_SECONDS_TO_RUN);
	datamap_add(properties_datamap, IDC_THROTTLE,               DM_BOOL,    OPTION_THROTTLE);
	datamap_add(properties_datamap, IDC_SLEEP,                  DM_BOOL,    OPTION_SLEEP);
	datamap_add(properties_datamap, IDC_SPEED,                  DM_FLOAT,   OPTION_SPEED);
	datamap_add(properties_datamap, IDC_SPEEDDISP,              DM_FLOAT,   OPTION_SPEED);
	datamap_add(properties_datamap, IDC_REFRESHSPEED,           DM_BOOL,    OPTION_REFRESHSPEED);
	datamap_add(properties_datamap, IDC_LOWLATENCY,             DM_BOOL,    OPTION_LOWLATENCY);

	// core retation options
	datamap_add(properties_datamap, IDC_ROTATE,                 DM_INT,     "");
	// ror, rol, autoror, autorol handled by callback
	datamap_add(properties_datamap, IDC_FLIPX,                  DM_BOOL,    OPTION_FLIPX);
	datamap_add(properties_datamap, IDC_FLIPY,                  DM_BOOL,    OPTION_FLIPY);

	// core artwork options
	datamap_add(properties_datamap, IDC_ARTWORK_CROP,           DM_BOOL,    OPTION_ARTWORK_CROP);
	datamap_add(properties_datamap, IDC_ARTWORK_FALLBACK,       DM_STRING,  OPTION_FALLBACK_ARTWORK);
	datamap_add(properties_datamap, IDC_ARTWORK_OVERRIDE,       DM_STRING,  OPTION_OVERRIDE_ARTWORK);

	// core screen options
	datamap_add(properties_datamap, IDC_BRIGHTCORRECT,          DM_FLOAT,   OPTION_BRIGHTNESS);
	datamap_add(properties_datamap, IDC_BRIGHTCORRECTDISP,      DM_FLOAT,   OPTION_BRIGHTNESS);
	datamap_add(properties_datamap, IDC_CONTRAST,               DM_FLOAT,   OPTION_CONTRAST);
	datamap_add(properties_datamap, IDC_CONTRASTDISP,           DM_FLOAT,   OPTION_CONTRAST);
	datamap_add(properties_datamap, IDC_GAMMA,                  DM_FLOAT,   OPTION_GAMMA);
	datamap_add(properties_datamap, IDC_GAMMADISP,              DM_FLOAT,   OPTION_GAMMA);
	datamap_add(properties_datamap, IDC_PAUSEBRIGHT,            DM_FLOAT,   OPTION_PAUSE_BRIGHTNESS);
	datamap_add(properties_datamap, IDC_PAUSEBRIGHTDISP,        DM_FLOAT,   OPTION_PAUSE_BRIGHTNESS);
	datamap_add(properties_datamap, IDC_SNAPBURNIN,             DM_BOOL,    OPTION_BURNIN);
	datamap_add(properties_datamap, IDC_SNAPBILINEAR,           DM_BOOL,    OPTION_SNAPBILINEAR);
	datamap_add(properties_datamap, IDC_EXIT_PLAYBACK,          DM_BOOL,    OPTION_EXIT_AFTER_PLAYBACK);

	// core vector options
	datamap_add(properties_datamap, IDC_BEAM_MIN,               DM_FLOAT,   OPTION_BEAM_WIDTH_MIN);
	datamap_add(properties_datamap, IDC_BEAM_MINDISP,           DM_FLOAT,   OPTION_BEAM_WIDTH_MIN);
	datamap_add(properties_datamap, IDC_BEAM_MAX,               DM_FLOAT,   OPTION_BEAM_WIDTH_MAX);
	datamap_add(properties_datamap, IDC_BEAM_MAXDISP,           DM_FLOAT,   OPTION_BEAM_WIDTH_MAX);
	datamap_add(properties_datamap, IDC_BEAM_INTEN,             DM_FLOAT,   OPTION_BEAM_INTENSITY_WEIGHT);
	datamap_add(properties_datamap, IDC_BEAM_INTENDISP,         DM_FLOAT,   OPTION_BEAM_INTENSITY_WEIGHT);
	datamap_add(properties_datamap, IDC_BEAM_DOT,               DM_INT,     OPTION_BEAM_DOT_SIZE);
	datamap_add(properties_datamap, IDC_BEAM_DOTDISP,           DM_INT,     OPTION_BEAM_DOT_SIZE);
	datamap_add(properties_datamap, IDC_FLICKER,                DM_FLOAT,   OPTION_FLICKER);
	datamap_add(properties_datamap, IDC_FLICKERDISP,            DM_FLOAT,   OPTION_FLICKER);

	// core sound options
	datamap_add(properties_datamap, IDC_SAMPLERATE,             DM_INT,     OPTION_SAMPLERATE);
	datamap_add(properties_datamap, IDC_SAMPLES,                DM_BOOL,    OPTION_SAMPLES);
	datamap_add(properties_datamap, IDC_SOUND_MODE,             DM_STRING,  OSDOPTION_SOUND);
	datamap_add(properties_datamap, IDC_VOLUME,                 DM_INT,     OPTION_VOLUME);
	datamap_add(properties_datamap, IDC_VOLUMEDISP,             DM_INT,     OPTION_VOLUME);

	// core input options
	datamap_add(properties_datamap, IDC_COINLOCKOUT,            DM_BOOL,    OPTION_COIN_LOCKOUT);
	datamap_add(properties_datamap, IDC_DEFAULT_INPUT,          DM_STRING,  OPTION_CTRLR);
	datamap_add(properties_datamap, IDC_USE_MOUSE,              DM_BOOL,    OPTION_MOUSE);
	datamap_add(properties_datamap, IDC_JOYSTICK,               DM_BOOL,    OPTION_JOYSTICK);
	datamap_add(properties_datamap, IDC_LIGHTGUN,               DM_BOOL,    OPTION_LIGHTGUN);
	datamap_add(properties_datamap, IDC_STEADYKEY,              DM_BOOL,    OPTION_STEADYKEY);
	datamap_add(properties_datamap, IDC_MULTIKEYBOARD,          DM_BOOL,    OPTION_MULTIKEYBOARD);
	datamap_add(properties_datamap, IDC_MULTIMOUSE,             DM_BOOL,    OPTION_MULTIMOUSE);

	datamap_add(properties_datamap, IDC_JDZ,                    DM_FLOAT,   OPTION_JOYSTICK_DEADZONE);
	datamap_add(properties_datamap, IDC_JDZDISP,                DM_FLOAT,   OPTION_JOYSTICK_DEADZONE);
	datamap_add(properties_datamap, IDC_JSAT,                   DM_FLOAT,   OPTION_JOYSTICK_SATURATION);
	datamap_add(properties_datamap, IDC_JSATDISP,               DM_FLOAT,   OPTION_JOYSTICK_SATURATION);
	datamap_add(properties_datamap, IDC_JOYSTICKMAP,            DM_STRING,  OPTION_JOYSTICK_MAP);

	// core input automatic enable options
	datamap_add(properties_datamap, IDC_PADDLE,                 DM_STRING,  OPTION_PADDLE_DEVICE);
	datamap_add(properties_datamap, IDC_ADSTICK,                DM_STRING,  OPTION_ADSTICK_DEVICE);
	datamap_add(properties_datamap, IDC_PEDAL,                  DM_STRING,  OPTION_PEDAL_DEVICE);
	datamap_add(properties_datamap, IDC_DIAL,                   DM_STRING,  OPTION_DIAL_DEVICE);
	datamap_add(properties_datamap, IDC_TRACKBALL,              DM_STRING,  OPTION_TRACKBALL_DEVICE);
	datamap_add(properties_datamap, IDC_LIGHTGUNDEVICE,         DM_STRING,  OPTION_LIGHTGUN_DEVICE);
	datamap_add(properties_datamap, IDC_POSITIONAL,             DM_STRING,  OPTION_POSITIONAL_DEVICE);
	datamap_add(properties_datamap, IDC_MOUSE,                  DM_STRING,  OPTION_MOUSE_DEVICE);
	datamap_add(properties_datamap, IDC_PROV_UIFONT,            DM_STRING,  OSD_FONT_PROVIDER);
	datamap_add(properties_datamap, IDC_PROV_KEYBOARD,          DM_STRING,  OSD_KEYBOARDINPUT_PROVIDER);
	datamap_add(properties_datamap, IDC_PROV_MOUSE,             DM_STRING,  OSD_MOUSEINPUT_PROVIDER);
	datamap_add(properties_datamap, IDC_PROV_JOYSTICK,          DM_STRING,  OSD_JOYSTICKINPUT_PROVIDER);
	datamap_add(properties_datamap, IDC_PROV_LIGHTGUN,          DM_STRING,  OSD_LIGHTGUNINPUT_PROVIDER);
	datamap_add(properties_datamap, IDC_PROV_MONITOR,           DM_STRING,  OSD_MONITOR_PROVIDER);
	datamap_add(properties_datamap, IDC_PROV_OUTPUT,            DM_STRING,  OSD_OUTPUT_PROVIDER);

	// core debugging options
	datamap_add(properties_datamap, IDC_LOG,                    DM_BOOL,    OPTION_LOG);
	datamap_add(properties_datamap, IDC_UPDATEINPAUSE,          DM_BOOL,    OPTION_UPDATEINPAUSE);

	// core misc options
	datamap_add(properties_datamap, IDC_BIOS,                   DM_STRING,  OPTION_BIOS);
	datamap_add(properties_datamap, IDC_CHEAT,                  DM_BOOL,    OPTION_CHEAT);
	datamap_add(properties_datamap, IDC_SKIP_GAME_INFO,         DM_BOOL,    OPTION_SKIP_GAMEINFO);

	//datamap_add(properties_datamap, IDC_LANGUAGE,             DM_STRING,  OPTION_LANGUAGE);   broken
	datamap_add(properties_datamap, IDC_LUASCRIPT,              DM_STRING,  OPTION_AUTOBOOT_SCRIPT);
	datamap_add(properties_datamap, IDC_BOOTDELAY,              DM_INT,     OPTION_AUTOBOOT_DELAY);
	datamap_add(properties_datamap, IDC_BOOTDELAYDISP,          DM_INT,     OPTION_AUTOBOOT_DELAY);
	datamap_add(properties_datamap, IDC_PLUGINS,                DM_BOOL,    OPTION_PLUGINS);
	datamap_add(properties_datamap, IDC_PLUGIN,                 DM_STRING,  OPTION_PLUGIN);
	datamap_add(properties_datamap, IDC_NVRAM_SAVE,             DM_BOOL,    OPTION_NVRAM_SAVE);
	datamap_add(properties_datamap, IDC_REWIND,                 DM_BOOL,    OPTION_REWIND);
	datamap_add(properties_datamap, IDC_NATURAL,                DM_BOOL,    OPTION_NATURAL_KEYBOARD);
	datamap_add(properties_datamap, IDC_HLSL_ON,                DM_BOOL,    WINOPTION_HLSL_ENABLE);
	datamap_add(properties_datamap, IDC_SAVE_INI,               DM_BOOL,    OPTION_WRITECONFIG);
	datamap_add(properties_datamap, IDC_JOY_CONTRA,             DM_BOOL,    OPTION_JOYSTICK_CONTRADICTORY);

	// core opengl - bgfx options
	datamap_add(properties_datamap, IDC_GLSLPOW,                DM_BOOL,    OSDOPTION_GL_FORCEPOW2TEXTURE);
	datamap_add(properties_datamap, IDC_GLSLTEXTURE,            DM_BOOL,    OSDOPTION_GL_NOTEXTURERECT);
	datamap_add(properties_datamap, IDC_GLSLVBO,                DM_BOOL,    OSDOPTION_GL_VBO);
	datamap_add(properties_datamap, IDC_GLSLPBO,                DM_BOOL,    OSDOPTION_GL_PBO);
	datamap_add(properties_datamap, IDC_GLSL,                   DM_BOOL,    OSDOPTION_GL_GLSL);
	datamap_add(properties_datamap, IDC_GLSLFILTER,             DM_STRING,  OSDOPTION_GLSL_FILTER);
	//datamap_add(properties_datamap, IDC_GLSLSYNC,             DM_BOOL,    OSDOPTION_GLSL_SYNC);
	datamap_add(properties_datamap, IDC_BGFX_CHAINS,            DM_STRING,  OSDOPTION_BGFX_SCREEN_CHAINS);
	datamap_add(properties_datamap, IDC_BGFX_BACKEND,           DM_STRING,  OSDOPTION_BGFX_BACKEND);

	// opengl shaders
	datamap_add(properties_datamap, IDC_MAME_SHADER0,           DM_STRING,  OSDOPTION_SHADER_MAME "0");
	datamap_add(properties_datamap, IDC_MAME_SHADER1,           DM_STRING,  OSDOPTION_SHADER_MAME "1");
	datamap_add(properties_datamap, IDC_MAME_SHADER2,           DM_STRING,  OSDOPTION_SHADER_MAME "2");
	datamap_add(properties_datamap, IDC_MAME_SHADER3,           DM_STRING,  OSDOPTION_SHADER_MAME "3");
	datamap_add(properties_datamap, IDC_MAME_SHADER4,           DM_STRING,  OSDOPTION_SHADER_MAME "4");
	datamap_add(properties_datamap, IDC_SCREEN_SHADER0,         DM_STRING,  OSDOPTION_SHADER_SCREEN "0");
	datamap_add(properties_datamap, IDC_SCREEN_SHADER1,         DM_STRING,  OSDOPTION_SHADER_SCREEN "1");
	datamap_add(properties_datamap, IDC_SCREEN_SHADER2,         DM_STRING,  OSDOPTION_SHADER_SCREEN "2");
	datamap_add(properties_datamap, IDC_SCREEN_SHADER3,         DM_STRING,  OSDOPTION_SHADER_SCREEN "3");
	datamap_add(properties_datamap, IDC_SCREEN_SHADER4,         DM_STRING,  OSDOPTION_SHADER_SCREEN "4");

	// windows performance options
	datamap_add(properties_datamap, IDC_HIGH_PRIORITY,          DM_INT,     WINOPTION_PRIORITY);
	datamap_add(properties_datamap, IDC_HIGH_PRIORITYTXT,       DM_INT,     WINOPTION_PRIORITY);

	// windows video options
	datamap_add(properties_datamap, IDC_VIDEO_MODE,             DM_STRING,  OSDOPTION_VIDEO);
	datamap_add(properties_datamap, IDC_NUMSCREENS,             DM_INT,     OSDOPTION_NUMSCREENS);
	datamap_add(properties_datamap, IDC_NUMSCREENSDISP,         DM_INT,     OSDOPTION_NUMSCREENS);
	datamap_add(properties_datamap, IDC_WINDOWED,               DM_BOOL,    OSDOPTION_WINDOW);
	datamap_add(properties_datamap, IDC_MAXIMIZE,               DM_BOOL,    OSDOPTION_MAXIMIZE);
	datamap_add(properties_datamap, IDC_KEEPASPECT,             DM_BOOL,    OPTION_KEEPASPECT);
	datamap_add(properties_datamap, IDC_PRESCALE,               DM_INT,     OSDOPTION_PRESCALE);
	datamap_add(properties_datamap, IDC_PRESCALEDISP,           DM_INT,     OSDOPTION_PRESCALE);
	datamap_add(properties_datamap, IDC_EFFECT,                 DM_STRING,  OPTION_EFFECT);
	datamap_add(properties_datamap, IDC_WAITVSYNC,              DM_BOOL,    OSDOPTION_WAITVSYNC);
	datamap_add(properties_datamap, IDC_SYNCREFRESH,            DM_BOOL,    OSDOPTION_SYNCREFRESH);
//  datamap_add(properties_datamap, IDC_WIDESTRETCH,            DM_BOOL,    OPTION_WIDESTRETCH);
	datamap_add(properties_datamap, IDC_UNEVENSTRETCH,          DM_BOOL,    OPTION_UNEVENSTRETCH);
	datamap_add(properties_datamap, IDC_UNEVENSTRETCHX,         DM_BOOL,    OPTION_UNEVENSTRETCHX);
	datamap_add(properties_datamap, IDC_UNEVENSTRETCHY,         DM_BOOL,    OPTION_UNEVENSTRETCHY);
	datamap_add(properties_datamap, IDC_AUTOSTRETCHXY,          DM_BOOL,    OPTION_AUTOSTRETCHXY);
	datamap_add(properties_datamap, IDC_INTOVERSCAN,            DM_BOOL,    OPTION_INTOVERSCAN);
	datamap_add(properties_datamap, IDC_INTSCALEX,              DM_INT,     OPTION_INTSCALEX);
	datamap_add(properties_datamap, IDC_INTSCALEX_TXT,          DM_INT,     OPTION_INTSCALEX);
	datamap_add(properties_datamap, IDC_INTSCALEY,              DM_INT,     OPTION_INTSCALEY);
	datamap_add(properties_datamap, IDC_INTSCALEY_TXT,          DM_INT,     OPTION_INTSCALEY);

	// Direct3D specific options
	datamap_add(properties_datamap, IDC_D3D_FILTER,             DM_BOOL,    OSDOPTION_FILTER);

	// per window video options
	datamap_add(properties_datamap, IDC_SCREEN,                 DM_STRING,  "");
	datamap_add(properties_datamap, IDC_SCREENSELECT,           DM_STRING,  "");
	datamap_add(properties_datamap, IDC_VIEW,                   DM_STRING,  "");
	datamap_add(properties_datamap, IDC_ASPECTRATIOD,           DM_STRING,  "");
	datamap_add(properties_datamap, IDC_ASPECTRATION,           DM_STRING,  "");
	datamap_add(properties_datamap, IDC_REFRESH,                DM_STRING,  "");
	datamap_add(properties_datamap, IDC_SIZES,                  DM_STRING,  "");

	// full screen options
	datamap_add(properties_datamap, IDC_TRIPLE_BUFFER,          DM_BOOL,    WINOPTION_TRIPLEBUFFER);
	datamap_add(properties_datamap, IDC_SWITCHRES,              DM_BOOL,    OSDOPTION_SWITCHRES);
	datamap_add(properties_datamap, IDC_FSBRIGHTNESS,           DM_FLOAT,   WINOPTION_FULLSCREENBRIGHTNESS);
	datamap_add(properties_datamap, IDC_FSBRIGHTNESSDISP,       DM_FLOAT,   WINOPTION_FULLSCREENBRIGHTNESS);
	datamap_add(properties_datamap, IDC_FSCONTRAST,             DM_FLOAT,   WINOPTION_FULLSCREENCONTRAST);
	datamap_add(properties_datamap, IDC_FSCONTRASTDISP,         DM_FLOAT,   WINOPTION_FULLSCREENCONTRAST);
	datamap_add(properties_datamap, IDC_FSGAMMA,                DM_FLOAT,   WINOPTION_FULLSCREENGAMMA);
	datamap_add(properties_datamap, IDC_FSGAMMADISP,            DM_FLOAT,   WINOPTION_FULLSCREENGAMMA);

	// windows sound options
	datamap_add(properties_datamap, IDC_AUDIO_LATENCY,          DM_FLOAT,   OSDOPTION_AUDIO_LATENCY);
	datamap_add(properties_datamap, IDC_AUDIO_LATENCY_DISP,     DM_FLOAT,   OSDOPTION_AUDIO_LATENCY);

	// input device options
	datamap_add(properties_datamap, IDC_DUAL_LIGHTGUN,          DM_BOOL,    WINOPTION_DUAL_LIGHTGUN);

#if defined(MAMEUI_NEWUI)
	// show menu
	datamap_add(properties_datamap, IDC_SHOW_MENU,              DM_BOOL,    WINOPTION_SHOW_MENUBAR);
#endif

	// set up callbacks
	datamap_set_callback(properties_datamap, IDC_ROTATE,        DCT_READ_CONTROL,       RotateReadControl);
	datamap_set_callback(properties_datamap, IDC_ROTATE,        DCT_POPULATE_CONTROL,   RotatePopulateControl);
	datamap_set_callback(properties_datamap, IDC_SCREEN,        DCT_READ_CONTROL,       ScreenReadControl);
	datamap_set_callback(properties_datamap, IDC_SCREEN,        DCT_POPULATE_CONTROL,   ScreenPopulateControl);
	datamap_set_callback(properties_datamap, IDC_VIEW,          DCT_POPULATE_CONTROL,   ViewPopulateControl);
	datamap_set_callback(properties_datamap, IDC_REFRESH,       DCT_READ_CONTROL,       ResolutionReadControl);
	datamap_set_callback(properties_datamap, IDC_REFRESH,       DCT_POPULATE_CONTROL,   ResolutionPopulateControl);
	datamap_set_callback(properties_datamap, IDC_SIZES,         DCT_READ_CONTROL,       ResolutionReadControl);
	datamap_set_callback(properties_datamap, IDC_SIZES,         DCT_POPULATE_CONTROL,   ResolutionPopulateControl);
	datamap_set_callback(properties_datamap, IDC_DEFAULT_INPUT, DCT_READ_CONTROL,       DefaultInputReadControl);
	datamap_set_callback(properties_datamap, IDC_DEFAULT_INPUT, DCT_POPULATE_CONTROL,   DefaultInputPopulateControl);
	datamap_set_callback(properties_datamap, IDC_SNAPVIEW,      DCT_POPULATE_CONTROL,   SnapViewPopulateControl);

	datamap_set_option_name_callback(properties_datamap, IDC_VIEW,      ViewSetOptionName);
	//missing population of views with per game defined additional views
	datamap_set_option_name_callback(properties_datamap, IDC_REFRESH,   ResolutionSetOptionName);
	datamap_set_option_name_callback(properties_datamap, IDC_SIZES,     ResolutionSetOptionName);


	// formats
	datamap_set_int_format(properties_datamap, IDC_VOLUMEDISP,           "%ddB");
	datamap_set_float_format(properties_datamap, IDC_AUDIO_LATENCY_DISP, "%2.1f");
	datamap_set_float_format(properties_datamap, IDC_BEAM_MINDISP,       "%3.2f");
	datamap_set_float_format(properties_datamap, IDC_BEAM_MAXDISP,       "%3.2f");
	datamap_set_float_format(properties_datamap, IDC_BEAM_INTENDISP,     "%3.2f");
	datamap_set_float_format(properties_datamap, IDC_FLICKERDISP,        "%3.2f");
	datamap_set_float_format(properties_datamap, IDC_GAMMADISP,          "%3.2f");
	datamap_set_float_format(properties_datamap, IDC_BRIGHTCORRECTDISP,  "%3.2f");
	datamap_set_float_format(properties_datamap, IDC_CONTRASTDISP,       "%3.2f");
	datamap_set_float_format(properties_datamap, IDC_PAUSEBRIGHTDISP,    "%3.2f");
	datamap_set_float_format(properties_datamap, IDC_FSGAMMADISP,        "%3.2f");
	datamap_set_float_format(properties_datamap, IDC_FSBRIGHTNESSDISP,   "%3.2f");
	datamap_set_float_format(properties_datamap, IDC_FSCONTRASTDISP,     "%3.2f");
	datamap_set_float_format(properties_datamap, IDC_JDZDISP,            "%3.2f");
	datamap_set_float_format(properties_datamap, IDC_JSATDISP,           "%3.2f");
	datamap_set_float_format(properties_datamap, IDC_SPEEDDISP,          "%3.1f");

	// trackbar ranges - slider-name,start,end,step
	datamap_set_trackbar_range(properties_datamap, IDC_JDZ,           0.0, 1.0,  (float)0.05);
	datamap_set_trackbar_range(properties_datamap, IDC_JSAT,          0.0, 1.0,  (float)0.05);
	datamap_set_trackbar_range(properties_datamap, IDC_SPEED,         0.1, 100.0,  (float)0.1);
	datamap_set_trackbar_range(properties_datamap, IDC_BEAM_MIN,      0.0, 1.0, (float)0.01);
	datamap_set_trackbar_range(properties_datamap, IDC_BEAM_MAX,      1.0, 10.0, (float)0.01);
	datamap_set_trackbar_range(properties_datamap, IDC_BEAM_INTEN,    -10.0, 10.0, (float)0.01);
	datamap_set_trackbar_range(properties_datamap, IDC_BEAM_DOT,      1, 4, 1);
	datamap_set_trackbar_range(properties_datamap, IDC_FLICKER,       0.0, 1.0, (float)0.01);
	datamap_set_trackbar_range(properties_datamap, IDC_AUDIO_LATENCY, 0.0, 10.0, (float)0.1);
	datamap_set_trackbar_range(properties_datamap, IDC_VOLUME,        -32, 0, 1);
	datamap_set_trackbar_range(properties_datamap, IDC_SECONDSTORUN,  0, 60, 1);
	datamap_set_trackbar_range(properties_datamap, IDC_NUMSCREENS,    1, 4, 1);
	datamap_set_trackbar_range(properties_datamap, IDC_PRESCALE,      1, 20, 1);
	datamap_set_trackbar_range(properties_datamap, IDC_FSGAMMA,       0.1, 3.0, (float)0.5);
	datamap_set_trackbar_range(properties_datamap, IDC_FSBRIGHTNESS,  0.1, 2.0, (float)0.1);
	datamap_set_trackbar_range(properties_datamap, IDC_FSCONTRAST,    0.1, 2.0, (float)0.1);
	datamap_set_trackbar_range(properties_datamap, IDC_GAMMA,         0.1, 3.0, (float)0.1);
	datamap_set_trackbar_range(properties_datamap, IDC_BRIGHTCORRECT, 0.1, 2.0, (float)0.1);
	datamap_set_trackbar_range(properties_datamap, IDC_CONTRAST,      0.1, 2.0, (float)0.1);
	datamap_set_trackbar_range(properties_datamap, IDC_PAUSEBRIGHT,   0.0, 1.0, (float)0.05);
	datamap_set_trackbar_range(properties_datamap, IDC_BOOTDELAY,     0, 5, 1);
	datamap_set_trackbar_range(properties_datamap, IDC_INTSCALEX,     0, 4, 1);
	datamap_set_trackbar_range(properties_datamap, IDC_INTSCALEY,     0, 4, 1);
	datamap_set_trackbar_range(properties_datamap, IDC_HIGH_PRIORITY, -15, 1, 1);

#ifdef MESS
	// MESS specific stuff
	datamap_add(properties_datamap, IDC_DIR_LIST,                    DM_STRING, "");
	datamap_add(properties_datamap, IDC_RAM_COMBOBOX,                DM_INT, OPTION_RAMSIZE);

	// set up callbacks
	datamap_set_callback(properties_datamap, IDC_DIR_LIST,           DCT_READ_CONTROL,      DirListReadControl);
	datamap_set_callback(properties_datamap, IDC_DIR_LIST,           DCT_POPULATE_CONTROL,  DirListPopulateControl);
	datamap_set_callback(properties_datamap, IDC_RAM_COMBOBOX,       DCT_POPULATE_CONTROL,  RamPopulateControl);
#endif
}

#if 0
static void SetSamplesEnabled(HWND hWnd, int nIndex, bool bSoundEnabled)
{
	bool enabled = false;
	HWND hCtrl = dialog_boxes::get_dlg_item(hWnd, IDC_SAMPLES);


	if (hCtrl)
	{
		if ( nIndex > -1 )
		{
			machine_config config(driver_list::driver(nIndex),GetCurrentOpts());

			for (device_sound_interface &sound : sound_interface_enumerator(config.root_device()))
				if (sound.device().type() == SAMPLES)
					enabled = true;
		}
		enabled = enabled && bSoundEnabled;
		(void)windows::enable_window(hCtrl, enabled);
	}
}
#endif
/* Moved here cause it's called in a few places */
static void InitializeOptions(HWND hDlg)
{
// from FX
//  InitializeSampleRateUI(hDlg);
	InitializeSoundUI(hDlg);
//  InitializeSoundModeUI(hDlg);
	InitializeSkippingUI(hDlg);
	InitializeRotateUI(hDlg);
	InitializeBIOSUI(hDlg);
	InitializeControllerMappingUI(hDlg);
	InitializeProviderMappingUI(hDlg);
	InitializeVideoUI(hDlg);
//  InitializeSnapViewUI(hDlg);
//  InitializeLanguageUI(hDlg);
	InitializePluginsUI(hDlg);
	InitializeGLSLFilterUI(hDlg);
	InitializeBGFXBackendUI(hDlg);
}

/* Moved here because it is called in several places */
static void InitializeMisc(HWND hDlg)
{
	button_control::enable(dialog_boxes::get_dlg_item(hDlg, IDC_JOYSTICK), DIJoystick.Available());
}

static void OptOnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
	if (hwndCtl == dialog_boxes::get_dlg_item(hwnd, IDC_NUMSCREENS))
		NumScreensSelectionChange(hwnd);
}

/* Handle changes to the Numscreens slider */
static void NumScreensSelectionChange(HWND hwnd)
{
	//Also Update the ScreenSelect Combo with the new number of screens
	UpdateSelectScreenUI(hwnd );
}

/* Handle changes to the Refresh drop down */
static void RefreshSelectionChange(HWND hWnd, HWND hWndCtrl)
{
	int nCurSelection = combo_box::get_cur_sel(hWndCtrl);

	if (nCurSelection != CB_ERR)
	{
		datamap_read_control(properties_datamap, hWnd, GetCurrentOpts(), IDC_SIZES);
		datamap_populate_control(properties_datamap, hWnd, GetCurrentOpts(), IDC_SIZES);
	}
}

/* Initialize the sound options */
static void InitializeSoundUI(HWND hwnd)
{
	int i;

	HWND hCtrl = dialog_boxes::get_dlg_item(hwnd, IDC_SOUND_MODE);
	if (hCtrl)
	{
		for (i = 0; i < NUMSOUND; i++)
		{
			combo_box::insert_string(hCtrl, i, (LPARAM)m_cb_Sound[i].m_pText);
			combo_box::set_item_data( hCtrl, i, (LPARAM)m_cb_Sound[i].m_pData);
		}
	}

	i = 0;

	hCtrl = dialog_boxes::get_dlg_item(hwnd, IDC_SAMPLERATE);
	if (hCtrl)
	{
		combo_box::add_string(hCtrl, (LPARAM)L"11025");
		combo_box::set_item_data(hCtrl, i++, 11025);
		combo_box::add_string(hCtrl, (LPARAM)L"22050");
		combo_box::set_item_data(hCtrl, i++, 22050);
		combo_box::add_string(hCtrl, (LPARAM)L"44100");
		combo_box::set_item_data(hCtrl, i++, 44100);
		combo_box::add_string(hCtrl, (LPARAM)L"48000");
		combo_box::set_item_data(hCtrl, i++, 48000);
		combo_box::set_cur_sel(hCtrl, 1);
	}
}

/* Populate the Frame Skipping drop down */
static void InitializeSkippingUI(HWND hwnd)
{
	HWND hCtrl = dialog_boxes::get_dlg_item(hwnd, IDC_FRAMESKIP);
	int i = 0;

	if (hCtrl)
	{
		combo_box::add_string(hCtrl, (LPARAM)L"Draw every frame");
		combo_box::set_item_data(hCtrl, i++, 0);
		combo_box::add_string(hCtrl, (LPARAM)L"Skip 1 frame");
		combo_box::set_item_data(hCtrl, i++, 1);
		combo_box::add_string(hCtrl, (LPARAM)L"Skip 2 frames");
		combo_box::set_item_data(hCtrl, i++, 2);
		combo_box::add_string(hCtrl, (LPARAM)L"Skip 3 frames");
		combo_box::set_item_data(hCtrl, i++, 3);
		combo_box::add_string(hCtrl, (LPARAM)L"Skip 4 frames");
		combo_box::set_item_data(hCtrl, i++, 4);
		combo_box::add_string(hCtrl, (LPARAM)L"Skip 5 frames");
		combo_box::set_item_data(hCtrl, i++, 5);
		combo_box::add_string(hCtrl, (LPARAM)L"Skip 6 frames");
		combo_box::set_item_data(hCtrl, i++, 6);
		combo_box::add_string(hCtrl, (LPARAM)L"Skip 7 frames");
		combo_box::set_item_data(hCtrl, i++, 7);
		combo_box::add_string(hCtrl, (LPARAM)L"Skip 8 frames");
		combo_box::set_item_data(hCtrl, i++, 8);
		combo_box::add_string(hCtrl, (LPARAM)L"Skip 9 frames");
		combo_box::set_item_data(hCtrl, i++, 9);
		combo_box::add_string(hCtrl, (LPARAM)L"Skip 10 frames");
		combo_box::set_item_data(hCtrl, i++, 10);
	}
}

/* Populate the Rotate drop down */
static void InitializeRotateUI(HWND hwnd)
{
	HWND hCtrl = dialog_boxes::get_dlg_item(hwnd, IDC_ROTATE);

	if (hCtrl)
	{
		combo_box::add_string(hCtrl, (LPARAM)L"Default");             // 0
		combo_box::add_string(hCtrl, (LPARAM)L"Clockwise");           // 1
		combo_box::add_string(hCtrl, (LPARAM)L"Anti-clockwise");      // 2
		combo_box::add_string(hCtrl, (LPARAM)L"None");                // 3
		combo_box::add_string(hCtrl, (LPARAM)L"Auto clockwise");      // 4
		combo_box::add_string(hCtrl, (LPARAM)L"Auto anti-clockwise"); // 5
	}
}

/* Populate the Video Mode drop down */
static void InitializeVideoUI(HWND hwnd)
{
	HWND hCtrl = dialog_boxes::get_dlg_item(hwnd, IDC_VIDEO_MODE);
	if (hCtrl)
	{
		for (size_t i = 0; i < NUMVIDEO; i++)
		{
			combo_box::insert_string(hCtrl, i, (LPARAM)m_cb_Video[i].m_pText);
			combo_box::set_item_data( hCtrl, i, (LPARAM)m_cb_Video[i].m_pData);
		}
	}
}

static void UpdateSelectScreenUI(HWND hwnd)
{
	HWND hCtrl = dialog_boxes::get_dlg_item(hwnd, IDC_SCREENSELECT);
	if (hCtrl)
	{
		int i;
		int numscreens = GetCurrentOpts().int_value(OSDOPTION_NUMSCREENS);
		if (numscreens < 1)
			numscreens = 1;
		else
		if (numscreens > MAX_SCREENS)
			numscreens = MAX_SCREENS;
		numscreens += 1;  // account for default screen

		combo_box::reset_content(hCtrl);
		for (i = 0; i < NUMSELECTSCREEN && i < numscreens ; i++)
		{
			combo_box::insert_string(hCtrl, i, (LPARAM)m_cb_SelectScreen[i].m_pText);
			combo_box::set_item_data( hCtrl, i, (LPARAM)m_cb_SelectScreen[i].m_pData);
		}

		// Smaller Amount of screens was selected, so use 0
		if( i <= m_currScreen )
			combo_box::set_cur_sel(hCtrl, 0);
		else
			combo_box::set_cur_sel(hCtrl, m_currScreen+1);
	}
}

static void InitializeControllerMappingUI(HWND hwnd)
{
	HWND hCtrl  = dialog_boxes::get_dlg_item(hwnd,IDC_PADDLE);
	HWND hCtrl1 = dialog_boxes::get_dlg_item(hwnd,IDC_ADSTICK);
	HWND hCtrl2 = dialog_boxes::get_dlg_item(hwnd,IDC_PEDAL);
	HWND hCtrl3 = dialog_boxes::get_dlg_item(hwnd,IDC_MOUSE);
	HWND hCtrl4 = dialog_boxes::get_dlg_item(hwnd,IDC_DIAL);
	HWND hCtrl5 = dialog_boxes::get_dlg_item(hwnd,IDC_TRACKBALL);
	HWND hCtrl6 = dialog_boxes::get_dlg_item(hwnd,IDC_LIGHTGUNDEVICE);
	HWND hCtrl7 = dialog_boxes::get_dlg_item(hwnd,IDC_POSITIONAL);

	for (size_t i = 0; i < NUMDEVICES; i++)
	{
		if (hCtrl)
		{
			combo_box::insert_string(hCtrl, i, (LPARAM)m_cb_Device[i].m_pText);
			combo_box::set_item_data( hCtrl, i, (LPARAM)m_cb_Device[i].m_pData);
		}

		if (hCtrl1)
		{
			combo_box::insert_string(hCtrl1, i, (LPARAM)m_cb_Device[i].m_pText);
			combo_box::set_item_data( hCtrl1, i, (LPARAM)m_cb_Device[i].m_pData);
		}

		if (hCtrl2)
		{
			combo_box::insert_string(hCtrl2, i, (LPARAM)m_cb_Device[i].m_pText);
			combo_box::set_item_data( hCtrl2, i, (LPARAM)m_cb_Device[i].m_pData);
		}

		if (hCtrl3)
		{
			combo_box::insert_string(hCtrl3, i, (LPARAM)m_cb_Device[i].m_pText);
			combo_box::set_item_data( hCtrl3, i, (LPARAM)m_cb_Device[i].m_pData);
		}

		if (hCtrl4)
		{
			combo_box::insert_string(hCtrl4, i, (LPARAM)m_cb_Device[i].m_pText);
			combo_box::set_item_data( hCtrl4, i, (LPARAM)m_cb_Device[i].m_pData);
		}

		if (hCtrl5)
		{
			combo_box::insert_string(hCtrl5, i, (LPARAM)m_cb_Device[i].m_pText);
			combo_box::set_item_data( hCtrl5, i, (LPARAM)m_cb_Device[i].m_pData);
		}

		if (hCtrl6)
		{
			combo_box::insert_string(hCtrl6, i, (LPARAM)m_cb_Device[i].m_pText);
			combo_box::set_item_data( hCtrl6, i, (LPARAM)m_cb_Device[i].m_pData);
		}

		if (hCtrl7)
		{
			combo_box::insert_string(hCtrl7, i, (LPARAM)m_cb_Device[i].m_pText);
			combo_box::set_item_data( hCtrl7, i, (LPARAM)m_cb_Device[i].m_pData);
		}
	}
}

static void InitializeProviderMappingUI(HWND hwnd)
{
	int i;
	HWND hCtrl   = dialog_boxes::get_dlg_item(hwnd,IDC_PROV_UIFONT);
	HWND hCtrl1  = dialog_boxes::get_dlg_item(hwnd,IDC_PROV_KEYBOARD);
	HWND hCtrl2  = dialog_boxes::get_dlg_item(hwnd,IDC_PROV_MOUSE);
	HWND hCtrl3  = dialog_boxes::get_dlg_item(hwnd,IDC_PROV_JOYSTICK);
	HWND hCtrl4  = dialog_boxes::get_dlg_item(hwnd,IDC_PROV_LIGHTGUN);
	HWND hCtrl5  = dialog_boxes::get_dlg_item(hwnd,IDC_PROV_MONITOR);
	HWND hCtrl6  = dialog_boxes::get_dlg_item(hwnd,IDC_PROV_OUTPUT);

	if (hCtrl)
		for (i = 0; i < NUMPROVUIFONT; i++)
		{
			combo_box::insert_string(hCtrl, i, (LPARAM)m_cb_ProvUifont[i].m_pText);
			combo_box::set_item_data( hCtrl, i, (LPARAM)m_cb_ProvUifont[i].m_pData);
		}

	if (hCtrl1)
		for (i = 0; i < NUMPROVKEYBOARD; i++)
		{
			combo_box::insert_string(hCtrl1, i, (LPARAM)m_cb_ProvKeyboard[i].m_pText);
			combo_box::set_item_data( hCtrl1, i, (LPARAM)m_cb_ProvKeyboard[i].m_pData);
		}

	if (hCtrl2)
		for (i = 0; i < NUMPROVMOUSE; i++)
		{
			combo_box::insert_string(hCtrl2, i, (LPARAM)m_cb_ProvMouse[i].m_pText);
			combo_box::set_item_data( hCtrl2, i, (LPARAM)m_cb_ProvMouse[i].m_pData);
		}

	if (hCtrl3)
		for (i = 0; i < NUMPROVJOYSTICK; i++)
		{
			combo_box::insert_string(hCtrl3, i, (LPARAM)m_cb_ProvJoystick[i].m_pText);
			combo_box::set_item_data( hCtrl3, i, (LPARAM)m_cb_ProvJoystick[i].m_pData);
		}

	if (hCtrl4)
		for (i = 0; i < NUMPROVLIGHTGUN; i++)
		{
			combo_box::insert_string(hCtrl4, i, (LPARAM)m_cb_ProvLightgun[i].m_pText);
			combo_box::set_item_data( hCtrl4, i, (LPARAM)m_cb_ProvLightgun[i].m_pData);
		}

	if (hCtrl5)
		for (i = 0; i < NUMPROVMONITOR; i++)
		{
			combo_box::insert_string(hCtrl5, i, (LPARAM)m_cb_ProvMonitor[i].m_pText);
			combo_box::set_item_data( hCtrl5, i, (LPARAM)m_cb_ProvMonitor[i].m_pData);
		}

	if (hCtrl6)
		for (i = 0; i < NUMPROVOUTPUT; i++)
		{
			combo_box::insert_string(hCtrl6, i, (LPARAM)m_cb_ProvOutput[i].m_pText);
			combo_box::set_item_data( hCtrl6, i, (LPARAM)m_cb_ProvOutput[i].m_pData);
		}
}


static void InitializeBIOSUI(HWND hwnd)
{
	HWND hCtrl = dialog_boxes::get_dlg_item(hwnd,IDC_BIOS);
	int i = 0;

	if (hCtrl)
	{
		const game_driver *gamedrv = &driver_list::driver(g_nGame);
		const rom_entry *rom;

		if (g_nPropertyMode == SOFTWARETYPE_GLOBAL || DriverHasOptionalBIOS(g_nGame) == false)
		{
			combo_box::insert_string(hCtrl, i, (LPARAM)L"None");
			combo_box::set_item_data( hCtrl, i++, (LPARAM)"");
			return;
		}

		combo_box::insert_string(hCtrl, i, (LPARAM)L"Default");
		combo_box::set_item_data( hCtrl, i++, (LPARAM)"");

		if (gamedrv->rom)
		{
			auto rom_entries = rom_build_entries(gamedrv->rom);
			for (rom = rom_entries.data(); !ROMENTRY_ISEND(rom); rom++)
			{
				if (ROMENTRY_ISSYSTEM_BIOS(rom))
				{
					const char *name = rom->hashdata().c_str();
					const char *biosname = ROM_GETNAME(rom);
					std::unique_ptr<wchar_t[]> wcsName(mui_utf16_from_utf8cstring(name));
					if( !wcsName)
						return;
					combo_box::insert_string(hCtrl, i, (LPARAM)wcsName.get());
					combo_box::set_item_data( hCtrl, i++, (LPARAM)biosname);
				}
			}
		}
	}
}

#if 0
static void InitializeLanguageUI(HWND hWnd)
{
	HWND hCtrl = dialog_boxes::get_dlg_item(hWnd, IDC_LANGUAGE);

	if (hCtrl)
	{
		std::string c = emu_get_value(GetCurrentOpts(), OPTION_LANGUAGE);
		if (c.empty())
			c = "English";
		int match = -1;
		int english = -1;
		int count = 0;
		std::string t1 = dir_get_value(DIRPATH_LANGUAGEPATH);
		const char* t2 = t1.c_str();
		osd::directory::ptr directory = osd::directory::open(t2);

		if (directory == nullptr)
			return;

		combo_box::reset_content(hCtrl);
		for (const osd::directory::entry *entry = directory->read(); entry; entry = directory->read())
		{
			if (entry->type == osd::directory::entry::entry_type::DIR)
			{
				std::string name = entry->name;

				if (!(name == "." || name == ".."))
				{
					wchar_t *wcsName = mui_utf16_from_utf8cstring(entry->name);
					combo_box::insert_string(hCtrl, count, wcsName);
					combo_box::set_item_data(hCtrl, count, entry->name);
					if (!c.empty() && name == c)
						match = count;
					if (name == "English")
						english = count;
//                  std::cout << "Language: " << name << ",count: " << count << "\n";
					count++;
					delete[] wcsName;
				}
			}
		}

		directory.reset();

//      std::cout << "Language: " << c << ", match: " << match << ", english: " << english << "\n";
		if (match >= 0)
			combo_box::set_cur_sel(hCtrl, match);
		else
		if (english >= 0)
			combo_box::set_cur_sel(hCtrl, english);
		else
			combo_box::set_cur_sel(hCtrl, -1);
	}
}
#endif

static void InitializePluginsUI(HWND hWnd)
{
	HWND hCtrl = dialog_boxes::get_dlg_item(hWnd, IDC_SELECT_PLUGIN);

	if (hCtrl)
	{
		std::string option_value = emu_opts.dir_get_value(DIRPATH_PLUGINSPATH);
		if (option_value.empty())
			return;

		std::filesystem::path plugins_path(option_value);
		if (!std::filesystem::exists(plugins_path) || !std::filesystem::is_directory(plugins_path))
			return;

		int count = 0;

		for (auto const& entry : std::filesystem::directory_iterator{ plugins_path })
		{
			if (entry.is_directory())
			{
				std::string name = entry.path().stem().string();

				if (!(name == "." || name == ".." || name == "json"))
				{
					plugin_names[count] = std::move(name);
					std::wstring cbox_string = entry.path().stem().wstring();
					int insert_result = combo_box::insert_string(hCtrl, count++, reinterpret_cast<LPARAM>(cbox_string.c_str()));
					if (insert_result == CB_ERR)
						return;
				}
			}
		}
	}

	combo_box::set_cur_sel(hCtrl, -1);
	combo_box::set_cue_banner_text(hCtrl, L"Select a plugin");
}

static void InitializeGLSLFilterUI(HWND hWnd)
{
	HWND hCtrl = dialog_boxes::get_dlg_item(hWnd, IDC_GLSLFILTER);

	if (hCtrl)
	{
		for (size_t i = 0; i < NUMGLSLFILTER; i++)
		{
			combo_box::insert_string(hCtrl, i, (LPARAM)m_cb_GLSLFilter[i].m_pText);
			combo_box::set_item_data(hCtrl, i, (LPARAM)m_cb_GLSLFilter[i].m_pData);
		}
	}
}

static void InitializeBGFXBackendUI(HWND hWnd)
{
	HWND hCtrl = dialog_boxes::get_dlg_item(hWnd, IDC_BGFX_BACKEND);

	if (hCtrl)
	{
		for (size_t i = 0; i < NUMBGFXBACKEND; i++)
		{
			combo_box::insert_string(hCtrl, i, (LPARAM)m_cb_BGFXBackend[i].m_pText);
			combo_box::set_item_data(hCtrl, i, (LPARAM)m_cb_BGFXBackend[i].m_pData);
		}
	}
}

static bool SelectEffect(HWND hWnd)
{
	wchar_t filename[MAX_PATH]{ L'\0' };
	bool changed = false;

	if (CommonFileDialog(dialog_boxes::get_open_filename, filename, FILETYPE_EFFECT_FILES))
	{
		const char* option_value = GetCurrentOpts().value(OPTION_EFFECT);
		std::string effect_name = std::filesystem::path(filename).stem().string();
		if (mui_strcmp(effect_name, option_value) != 0) // strip the path and extension
		{
			HWND control = dialog_boxes::get_dlg_item(hWnd, IDC_EFFECT);
			emu_opts.emu_set_value(GetCurrentOpts(), OPTION_EFFECT, effect_name);
			windows::set_window_text_utf8(control, effect_name.c_str());
			changed = true;
		}
	}
	return changed;
}

static bool ResetEffect(HWND hWnd)
{
	bool changed = false;
	const char *new_value = "none";

	if (mui_strcmp(new_value, GetCurrentOpts().value(OPTION_EFFECT)))
	{
		HWND control = dialog_boxes::get_dlg_item(hWnd, IDC_EFFECT);
		emu_opts.emu_set_value(GetCurrentOpts(), OPTION_EFFECT, new_value);
		windows::set_window_text_utf8(control, new_value);
		changed = true;
	}
	return changed;
}

static bool ChangeFallback(HWND hWnd)
{
	bool changed = false;
	std::unique_ptr<char[]> data(windows::get_window_text_utf8(dialog_boxes::get_dlg_item(hWnd, IDC_ARTWORK_FALLBACK)));

	if (data && mui_strcmp(data.get(), GetCurrentOpts().value(OPTION_FALLBACK_ARTWORK)) != 0)
	{
		emu_opts.emu_set_value(GetCurrentOpts(), OPTION_FALLBACK_ARTWORK, data.get());
		changed = true;
	}

	return changed;
}

static bool ChangeOverride(HWND hWnd)
{
	bool changed = false;
	std::unique_ptr<char[]> data(windows::get_window_text_utf8(dialog_boxes::get_dlg_item(hWnd, IDC_ARTWORK_OVERRIDE)));

	if (data && mui_strcmp(data.get(), GetCurrentOpts().value(OPTION_OVERRIDE_ARTWORK)) != 0)
	{
		emu_opts.emu_set_value(GetCurrentOpts(), OPTION_OVERRIDE_ARTWORK, data.get());
		changed = true;
	}

	return changed;
}

static bool ChangeJoystickMap(HWND hWnd)
{
	bool changed = false;
	std::unique_ptr<char[]> joymap(windows::get_window_text_utf8(dialog_boxes::get_dlg_item(hWnd, IDC_JOYSTICKMAP)));

	if (joymap && mui_strcmp(joymap.get(), GetCurrentOpts().value(OPTION_JOYSTICK_MAP)))
	{
		emu_opts.emu_set_value(GetCurrentOpts(), OPTION_JOYSTICK_MAP, joymap.get());
		changed = true;
	}

	return changed;
}

static bool ResetJoystickMap(HWND hWnd)
{
	bool changed = false;
	const char *new_value = "auto";

	if (mui_strcmp(new_value, GetCurrentOpts().value(OPTION_JOYSTICK_MAP)) != 0)
	{
		HWND control = dialog_boxes::get_dlg_item(hWnd, IDC_JOYSTICKMAP);
		emu_opts.emu_set_value(GetCurrentOpts(), OPTION_JOYSTICK_MAP, new_value);
		windows::set_window_text_utf8(control, new_value);
		changed = true;
	}
	return changed;
}

static bool SelectLUAScript(HWND hWnd)
{
	wchar_t filename[MAX_PATH]{ L'\0' };
	bool changed = false;

	if (CommonFileDialog(dialog_boxes::get_open_filename, filename, FILETYPE_LUASCRIPT_FILES))
	{
		// Get the filename without path and extension
		std::filesystem::path file_path(filename);
		std::string window_text = file_path.stem().string();
		// Set the option if changed
		if (mui_strcmp(window_text, GetCurrentOpts().value(OPTION_AUTOBOOT_SCRIPT)) != 0)
		{
			emu_opts.emu_set_value(GetCurrentOpts(), OPTION_AUTOBOOT_SCRIPT, file_path.string());
			windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hWnd, IDC_LUASCRIPT), window_text.c_str());
			changed = true;
		}
	}

	return changed;
}

static bool ResetLUAScript(HWND hWnd)
{
	bool changed = false;
	const char *new_value = "";

	if (mui_strcmp(new_value, GetCurrentOpts().value(OPTION_AUTOBOOT_SCRIPT)) != 0)
	{
		emu_opts.emu_set_value(GetCurrentOpts(), OPTION_AUTOBOOT_SCRIPT, new_value);
		windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hWnd, IDC_LUASCRIPT), "None");
		changed = true;
	}

	return changed;
}

static bool plugin_enabled(std::string_view plugin_list, std::string_view plugin_name)
{
	stringtokenizer tokenizer(plugin_list, ",");
	for (const auto &token : tokenizer)
	{
		if (mui_strcmp(token, plugin_name) == 0)
			return true;
	}
	return false;
}

static bool SelectPlugins(HWND hWnd)
{
	bool changed = false;
	HWND hcontrol = dialog_boxes::get_dlg_item(hWnd, IDC_SELECT_PLUGIN);
	if (!hcontrol)
		return changed;

	int index = combo_box::get_cur_sel(hcontrol);
	if (index == CB_ERR)
		return changed;


	std::string new_value = plugin_names[index];
	std::string value = emu_opts.emu_get_value(GetCurrentOpts(), OPTION_PLUGIN);

	if (value.empty())
	{
		emu_opts.emu_set_value(GetCurrentOpts(), OPTION_PLUGIN, new_value);
		windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hWnd, IDC_PLUGIN), new_value.c_str());
		changed = true;
		combo_box::set_cur_sel(dialog_boxes::get_dlg_item(hWnd, IDC_SELECT_PLUGIN), -1);
		return changed;
	}

	bool not_enabled = !plugin_enabled(value, new_value);

	if (not_enabled)
	{
		new_value = (!value.empty()) ? value + "," + new_value : new_value;
		emu_opts.emu_set_value(GetCurrentOpts(), OPTION_PLUGIN, new_value);
		windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hWnd, IDC_PLUGIN), new_value.c_str());
		changed = true;
	}

	combo_box::set_cur_sel(dialog_boxes::get_dlg_item(hWnd, IDC_SELECT_PLUGIN), -1);
	return changed;
}

static bool ResetPlugins(HWND hWnd)
{
	emu_opts.emu_set_value(GetCurrentOpts(), OPTION_PLUGIN, "");
	windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hWnd, IDC_PLUGIN), "None");
	combo_box::set_cur_sel(dialog_boxes::get_dlg_item(hWnd, IDC_SELECT_PLUGIN), -1);
	return true;
}

static bool SelectBGFXChains(HWND hWnd)
{
	wchar_t filename[MAX_PATH]{ '\0' };
	bool changed = false;

	if (CommonFileDialog(dialog_boxes::get_open_filename, filename, FILETYPE_BGFX_FILES))
	{
		std::filesystem::path file_path(filename);
		std::string chain_name = file_path.stem().string();
		if (mui_strcmp(chain_name, GetCurrentOpts().value(OSDOPTION_BGFX_SCREEN_CHAINS)) != 0)
		{
			emu_opts.emu_set_value(GetCurrentOpts(), OSDOPTION_BGFX_SCREEN_CHAINS, chain_name);
			windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hWnd, IDC_BGFX_CHAINS), chain_name.c_str());
			changed = true;
		}
	}

	return changed;
}

static bool ResetBGFXChains(HWND hWnd)
{
	bool changed = false;
	const char *new_value = "default";
	if (mui_strcmp(new_value, GetCurrentOpts().value(OSDOPTION_BGFX_SCREEN_CHAINS)) != 0)
	{
		emu_opts.emu_set_value(GetCurrentOpts(), OSDOPTION_BGFX_SCREEN_CHAINS, new_value);
		windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hWnd, IDC_BGFX_CHAINS), "Default");
		changed = true;
	}

	return changed;
}

void UpdateBackgroundBrush(HWND hwndTab)
{
	// Destroy old brush
	if (hBkBrush)
		(void)gdi::delete_brush(hBkBrush);

	hBkBrush = nullptr;

	// Only do this if the theme is active
	if (SafeIsAppThemed())
	{
		// Get tab control dimensions
		RECT rc;
		(void)windows::get_window_rect( hwndTab, &rc);

		// Get the tab control DC
		HDC hDC = gdi::get_dc(hwndTab);

		// Create a compatible DC
		HDC hDCMem = gdi::create_compatible_dc(hDC);
		HBITMAP hBmp = CreateCompatibleBitmap(hDC, rc.right - rc.left, rc.bottom - rc.top);
		HBITMAP hBmpOld = (HBITMAP)(gdi::select_object(hDCMem, hBmp));

		// Tell the tab control to paint in our DC
		(void)windows::send_message(hwndTab, WM_PRINTCLIENT, (WPARAM)(hDCMem), (LPARAM)(PRF_ERASEBKGND | PRF_CLIENT | PRF_NONCLIENT));

		// Create a pattern brush from the bitmap selected in our DC
		hBkBrush = CreatePatternBrush(hBmp);

		// Restore the bitmap
		(void)gdi::select_object(hDCMem, hBmpOld);

		// Cleanup
		(void)gdi::delete_bitmap(hBmp);
		(void)gdi::delete_dc(hDCMem);
		(void)gdi::release_dc(hwndTab, hDC);
	}
}


// from properties.cpp (MESSUI)


//============================================================
#ifdef MESS
//============================================================
//  Functions to handle the SWPATH tab
//============================================================

static void AppendList(HWND hList, LPCWSTR lpItem, int nItem)
{
	LVITEMW Item{};
	Item.mask = LVIF_TEXT;
	Item.pszText = (LPWSTR) lpItem;
	Item.iItem = nItem;
	(void)list_view::insert_item(hList, &Item);
}

static bool DirListReadControl(datamap *map, HWND dialog, HWND control, windows_options *options, std::string option_name)
{
	// determine the directory count; note that one item is the "<    >" entry
	int directory_count = list_view::get_item_count(control);
	if (directory_count <= 0)
		return false;

	// don't include the last item, which is the "<    >" entry
	directory_count--;

	// allocate a buffer to hold the directory names
	std::wstring directories;

	LVITEMW lvi{};

	lvi.mask = LVIF_TEXT;

	std::unique_ptr<wchar_t[]> item_text;
	for (size_t index = 0; index < directory_count; index++)
	{
		item_text.reset(new wchar_t[MAX_PATH]);

		// retrieve the next entry
		lvi.iItem = index;
		lvi.pszText = item_text.get();
		lvi.cchTextMax = MAX_PATH;

		if (!list_view::get_item(control, &lvi))
			continue; // Skip this item if retrieval fails

		// append a semicolon, if we're past the first entry
		if (index > 0)
			directories.append(L";");

		directories.append(item_text.get());
	}

	if (directories.empty())
		emu_opts.emu_set_value(options, OPTION_SWPATH, "");
	else
	{
		std::string option_value = mui_utf8_from_utf16string(directories);
		emu_opts.emu_set_value(options, OPTION_SWPATH, option_value);
	}

	return true;
}


static bool DirListPopulateControl(datamap *map, HWND dialog, HWND control, windows_options *options, std::string option_name)
{
	std::string swpath_value = emu_opts.emu_get_value(options, OPTION_SWPATH);

	if (swpath_value.empty())
		return false;

	// delete all items in the list control
	(void)list_view::delete_all_items(control);

	// add the column
	RECT r;
	(void)windows::get_client_rect(control, &r);
	LV_COLUMNW lvc{};
	lvc.mask = LVCF_WIDTH;
	lvc.cx = r.right - r.left - windows::get_system_metrics(SM_CXHSCROLL);
	(void)list_view::insert_column(control, 0, &lvc);


	std::wstring directories = mui_utf16_from_utf8string(swpath_value);
	if (directories.empty())
		return false;

	int current_item = 0;
	wstringtokenizer tokenizer(directories, L";");
	// add each of the directories
	for (const auto &token : tokenizer)
	{
		// append this item
		AppendList(control, token.c_str(), current_item);
		++current_item;
	}

	// finish up
	AppendList(control, &DIRLIST_NEWENTRYTEXT[0], current_item);
	list_view::set_item_state(control, 0, LVIS_SELECTED, LVIS_SELECTED);

	return true;
}


static void MarkChanged(HWND hDlg)
{
	/* fake a CBN_SELCHANGE event from IDC_SIZES to force it to be changed */
	HWND hCtrl = dialog_boxes::get_dlg_item(hDlg, IDC_SIZES);
	windows::post_message(hDlg, WM_COMMAND, (CBN_SELCHANGE << 16) | IDC_SIZES, (LPARAM) hCtrl);
}


static bool SoftwareDirectories_OnInsertBrowse(HWND hDlg, bool bBrowse, LPCTSTR lpItem)
{
	wchar_t inbuf[MAX_PATH];
	std::wstring outbuf;
	LPWSTR lpIn;

	g_bModifiedSoftwarePaths = true;

	HWND hList = dialog_boxes::get_dlg_item(hDlg, IDC_DIR_LIST);
	int nItem = list_view::get_next_item(hList, -1, LVNI_SELECTED);

	if (nItem == -1)
		return false;

	/* Last item is placeholder for append */
	if (nItem == list_view::get_item_count(hList) - 1)
		bBrowse = false;

	if (!lpItem)
	{
//      if (bBrowse)
//      {
			ListView_GetItemText(hList, nItem, 0, inbuf, std::size(inbuf));
			lpIn = inbuf;
//      }
//      else
//          lpIn = nullptr;

		if (!BrowseForDirectory(hDlg, lpIn, outbuf))
			return false;

		lpItem = outbuf.c_str();
	}

	AppendList(hList, lpItem, nItem);
	if (bBrowse)
		(void)ListView_DeleteItem(hList, nItem+1);
	MarkChanged(hDlg);
	return true;
}



static bool SoftwareDirectories_OnDelete(HWND hDlg)
{
	int nSelect = 0;
	HWND hList = dialog_boxes::get_dlg_item(hDlg, IDC_DIR_LIST);

	g_bModifiedSoftwarePaths = true;

	int nItem = list_view::get_next_item(hList, -1, LVNI_SELECTED | LVNI_ALL);

	if (nItem == -1)
		return false;

	/* Don't delete "Append" placeholder. */
	if (nItem == list_view::get_item_count(hList) - 1)
		return false;

	(void)ListView_DeleteItem(hList, nItem);

	int nCount = list_view::get_item_count(hList);
	if (nCount <= 1)
		return false;

	/* If the last item was removed, select the item above. */
	if (nItem == nCount - 1)
		nSelect = nCount - 2;
	else
		nSelect = nItem;

	list_view::set_item_state(hList, nSelect, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	MarkChanged(hDlg);
	return true;
}



static bool SoftwareDirectories_OnBeginLabelEdit(HWND hDlg, NMHDR* pNMHDR)
{
	bool          bResult = false;
	NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pNMHDR;
	LVITEMW*       pItem = &pDispInfo->item;
	HWND          hList = dialog_boxes::get_dlg_item(hDlg, IDC_DIR_LIST);

	/* Last item is placeholder for append */
	if (pItem->iItem == list_view::get_item_count(hList) - 1)
	{
		HWND hEdit = (HWND) (uintptr_t) windows::send_message(hList, LVM_GETEDITCONTROL, 0, 0);
		windows::set_window_text_utf8(hEdit, "");
	}

	return bResult;
}



static bool SoftwareDirectories_OnEndLabelEdit(HWND hDlg, NMHDR* pNMHDR)
{
	NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pNMHDR;
	LVITEMW* pItem = &pDispInfo->item;

	if (!pItem->pszText || !*(pItem->pszText))
		return false; // no text entered

	// Check validity of edited directory.
	if (!std::filesystem::exists(pItem->pszText) || !std::filesystem::is_directory(pItem->pszText))
	{
		if (dialog_boxes::message_box(nullptr, L"Invalid entry. Do you want to continue anyway?", &MAMEUINAME[0], MB_OKCANCEL) == IDCANCEL)
			return false; // user cancelled
	}

	SoftwareDirectories_OnInsertBrowse(hDlg, true, pItem->pszText);

	return true;
}

#if 0
// only called by PropSheetFilter_Config (below)
static bool DriverHasDevice(const game_driver *gamedrv, iodevice_/t type)
{
	bool b = false;

	// allocate the machine config
	machine_config config(*gamedrv,MameUIGlobal());

	for (device_image_interface &dev : image_interface_enumerator(config.root_device()))
	{
		if (!dev.user_loadable())
			continue;
		if (dev.image_type() == type)
		{
			b = true;
			break;
		}
	}
	return b;
}

// not used
bool PropSheetFilter_Config(const machine_config *drv, const game_driver *gamedrv)
{
	ram_device_enumerator iter(drv->root_device());
	return (iter.first()) || DriverHasDevice(gamedrv, IO_PRINTER);
}
#endif
INT_PTR CALLBACK GameMessOptionsProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR rc = 0;
	bool bHandled = false;

	switch (Msg)
	{
	case WM_NOTIFY:
		switch (((NMHDR *) lParam)->code)
		{
		case LVN_ENDLABELEDIT:
			rc = SoftwareDirectories_OnEndLabelEdit(hDlg, (NMHDR *) lParam);
			bHandled = true;
			break;

		case LVN_BEGINLABELEDIT:
			rc = SoftwareDirectories_OnBeginLabelEdit(hDlg, (NMHDR *) lParam);
			bHandled = true;
			break;
		}
	}

	if (!bHandled)
		rc = GameOptionsProc(hDlg, Msg, wParam, lParam);

	return rc;
}



bool MessPropertiesCommand(HWND hWnd, WORD wNotifyCode, WORD wID, bool *changed)
{
	bool handled = true;

	switch(wID)
	{
		case IDC_DIR_BROWSE:
			if (wNotifyCode == BN_CLICKED)
				*changed = SoftwareDirectories_OnInsertBrowse(hWnd, true, nullptr);
			break;

		case IDC_DIR_INSERT:
			if (wNotifyCode == BN_CLICKED)
				*changed = SoftwareDirectories_OnInsertBrowse(hWnd, false, nullptr);
			break;

		case IDC_DIR_DELETE:
			if (wNotifyCode == BN_CLICKED)
				*changed = SoftwareDirectories_OnDelete(hWnd);
			break;

		default:
			handled = false;
			break;
	}
	return handled;
}

//============================================================
//  Functions to handle the RAM control
//============================================================

static std::string messram_string(UINT32 ram) // FIXME - limit of 4GB
{
	 std::string suffix = "", messram;

	if ((ram % (1024*1024)) == 0)
	{
		ram /= 1024*1024;
		suffix = "M";
	}
	else
	if ((ram % 1024) == 0)
	{
		ram /= 1024;
		suffix = "K";
	}

	messram = std::to_string(ram).append(suffix);
	return messram;
}

//-------------------------------------------------
//  parse_string - convert a ram string to an
//  integer value
//-------------------------------------------------

static uint32_t parse_string(const char *s)
{
	static const struct
	{
		const char *suffix;
		unsigned multiple;
	} s_suffixes[] =
	{
		{ "",       1 },
		{ "k",      1024 },
		{ "kb",     1024 },
		{ "kib",    1024 },
		{ "m",      1024 * 1024 },
		{ "mb",     1024 * 1024 },
		{ "mib",    1024 * 1024 },
		{ "K",      1024 },
		{ "KB",     1024 },
		{ "KiB",    1024 },
		{ "M",      1024 * 1024 },
		{ "MB",     1024 * 1024 },
		{ "MiB",    1024 * 1024 }
	};

	// parse the string
	unsigned ram = 0;
	char suffix[8] = { 0, };
	sscanf(s, "%u%7s", &ram, suffix);

	// perform the lookup
	auto iter = std::find_if(std::begin(s_suffixes), std::end(s_suffixes), [&suffix](const auto &potential_suffix)
	{ return !mui_stricmp(suffix, potential_suffix.suffix); } );

	// identify the multiplier (or 0 if not recognized, signalling a parse failure)
	unsigned multiple = iter != std::end(s_suffixes) ? iter->multiple : 0;

	// return the result
	return ram * multiple;
}

static bool RamPopulateControl(datamap *map, HWND dialog, HWND control, windows_options *options, std::string option_name)
{
	int i = 0, current_index = 0;

	// identify the driver
	int driver_index = PropertiesCurrentGame(dialog);
	const game_driver *gamedrv = &driver_list::driver(driver_index);

	// clear out the combo box
	combo_box::reset_content(control);

	// allocate the machine config
	machine_config cfg(*gamedrv,*options);

	// identify how many options that we have
	ram_device_enumerator iter(cfg.root_device());
	ram_device *device = iter.first();

	// we can only do something meaningful if there is more than one option
	if (device)
	{
		const ram_device *ramdev = dynamic_cast<const ram_device *>(device);
		if (!ramdev)
			return false;

		// identify the current amount of RAM
		const char *this_ram_string = options->value(OPTION_RAMSIZE);
		uint32_t current_ram = (this_ram_string) ? parse_string(this_ram_string) : 0;
		if (current_ram == 0)
			current_ram = ramdev->default_size();

		std::string ramtext = messram_string(current_ram);
		std::unique_ptr<const wchar_t[]> wcs_ramstring(mui_utf16_from_utf8cstring(ramtext.c_str()));
		if( !wcs_ramstring)
			return false;

		combo_box::insert_string(control, i, (LPARAM)wcs_ramstring.get());
		combo_box::set_item_data(control, i, (LPARAM)current_ram);

		if (!ramdev->extra_options().empty())
		{
			/* try to parse each option */
			for (ram_device::extra_option const &option : ramdev->extra_options())
			{
				// identify this option
				std::string ramtext2 = option.first;
				if (ramtext2 != ramtext)
				{
					wcs_ramstring.reset(mui_utf16_from_utf8cstring(ramtext2.c_str()));
					if(wcs_ramstring)
					{
						i++;
						// add this option to the combo box
						combo_box::insert_string(control, i, (LPARAM)wcs_ramstring.get());
						combo_box::set_item_data(control, i, (LPARAM)option.second);

						// is this the current option?  record the index if so
						if (option.second == current_ram)
							current_index = i;
					}
				}
			}
		}

		// set the combo box
		combo_box::set_cur_sel(control, current_index);
	}

	(void)windows::enable_window(control, i ? 1 : 0);

	return true;
}

#endif
