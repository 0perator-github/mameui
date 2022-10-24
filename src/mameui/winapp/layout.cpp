// For licensing and usage information, read docs/winui_license.txt
// MASTER
// ============================================================================

// ============================================================================
// layout.cpp - MAME specific TreeView definitions (and maybe more in the future)
// ============================================================================

// standard C++ headers
#include <array>
#include <filesystem>
#include <memory>
#include <vector> // bitmask.h

// standard windows headers
#include "winapi_common.h"

// MAME headers
#include "emu.h"

#include "ui/moptions.h"
#include "winopts.h"

// MAMEUI headers
#include "bitmask.h"
#include "emu_opts.h"
#include "help.h"
#include "mui_audit.h"
#include "mui_opts.h"
#include "mui_util.h"
#include "properties.h"
#include "resource.h"
#include "splitters.h"
#include "treeview.h"
#include "screenshot.h"
#include "winui.h"

using namespace std::string_literals;

static bool FilterAvailable(std::size_t driver_index)
{
	return !DriverUsesRoms(driver_index) || IsAuditResultYes(GetRomAuditResults(driver_index));
}

constexpr std::array<FOLDERDATA, TV_FOLDER_COUNT> g_folderData =
{
	FOLDERDATA{ "All Systems"s,     "allgames"s,          TV_FOLDER_ALLGAMES,     IDI_FOLDER_ALLGAMES,      0,            0,                      false, nullptr,                     nullptr,                 true },
#ifdef MESS
	FOLDERDATA{ "Arcade"s,          "arcade"s,            TV_FOLDER_ARCADE,       IDI_FOLDER,               F_ARCADE,     0,                      false, nullptr,                     DriverIsArcade,          true, SOFTWARETYPE_ARCADE },
	FOLDERDATA{ "Computer"s,        "computer"s,          TV_FOLDER_COMPUTER,     IDI_FOLDER,               F_COMPUTER,   F_CONSOLE,              false, nullptr,                     DriverIsComputer,        true, SOFTWARETYPE_COMPUTER },
	FOLDERDATA{ "Console"s,         "console"s,           TV_FOLDER_CONSOLE,      IDI_FOLDER,               F_CONSOLE,    F_COMPUTER,             false, nullptr,                     DriverIsConsole,         true, SOFTWARETYPE_CONSOLE },
#else
	FOLDERDATA{ "Arcade"s,          "arcade"s,            TV_FOLDER_ARCADE,       IDI_FOLDER,               F_ARCADE,     F_CONSOLE | F_COMPUTER, false, nullptr,                     DriverIsArcade,          true, SOFTWARETYPE_ARCADE },
	FOLDERDATA{ "Available"s,       "available"s,         TV_FOLDER_AVAILABLE,    IDI_FOLDER_AVAILABLE,     F_AVAILABLE,  0,                      false, nullptr,                     FilterAvailable,         true },
#endif
	FOLDERDATA{ "BIOS"s,            "bios"s,              TV_FOLDER_BIOS,         IDI_FOLDER_BIOS,          0,            0,                      true,  CreateBIOSFolders,           DriverIsBios,            true },
	FOLDERDATA{ "CHD"s,             "harddisk"s,          TV_FOLDER_HARDDISK,     IDI_FOLDER_HARDDISK,      0,            0,                      false, nullptr,                     DriverIsHarddisk,        true },
	FOLDERDATA{ "Clones"s,          "clones"s,            TV_FOLDER_CLONES,       IDI_FOLDER_CLONES,        F_CLONES,     F_ORIGINALS,            false, nullptr,                     DriverIsClone,           true },
	FOLDERDATA{ "CPU"s,             "cpu"s,               TV_FOLDER_CPU,          IDI_FOLDER_CPU,           0,            0,                      true,  CreateCPUFolders },
	FOLDERDATA{ "Dumping Status"s,  "dumping"s,           TV_FOLDER_DUMPING,      IDI_FOLDER_DUMP,          0,            0,                      true,  CreateDumpingFolders },
	FOLDERDATA{ "FPS"s,             "fps"s,               TV_FOLDER_FPS,          IDI_FOLDER_FPS,           0,            0,                      true,  CreateFPSFolders },
	FOLDERDATA{ "Horizontal"s,      "horizontal"s,        TV_FOLDER_HORIZONTAL,   IDI_FOLDER_HORIZONTAL,    F_HORIZONTAL, F_VERTICAL,             false, nullptr,                     DriverIsVertical,        false, SOFTWARETYPE_HORIZONTAL },
	FOLDERDATA{ "Imperfect"s,       "imperfect"s,         TV_FOLDER_DEFICIENCY,   IDI_FOLDER_IMPERFECT,     0,            0,                      false, CreateDeficiencyFolders },
	FOLDERDATA{ "Lightgun"s,        "Lightgun"s,          TV_FOLDER_LIGHTGUN,     IDI_FOLDER_LIGHTGUN,      0,            0,                      false, nullptr,                     DriverUsesLightGun,      true },
	FOLDERDATA{ "Manufacturer"s,    "manufacturer"s,      TV_FOLDER_MANUFACTURER, IDI_FOLDER_MANUFACTURER,  0,            0,                      true,  CreateManufacturerFolders },
	FOLDERDATA{ "Mechanical"s,      "mechanical"s,        TV_FOLDER_MECHANICAL,   IDI_FOLDER_MECHANICAL,    0,            0,                      false, nullptr,                     DriverIsMechanical,      true },
	FOLDERDATA{ "Modified/Hacked"s, "modified"s,          TV_FOLDER_MODIFIED,     IDI_FOLDER,               0,            0,                      false, nullptr,                     DriverIsModified,        true },
	FOLDERDATA{ "Mouse"s,           "mouse"s,             TV_FOLDER_MOUSE,        IDI_FOLDER,               0,            0,                      false, nullptr,                     DriverUsesMouse,         true },
	FOLDERDATA{ "Non Mechanical"s,  "nonmechanical"s,     TV_FOLDER_NONMECHANICAL,IDI_FOLDER,               0,            0,                      false, nullptr,                     DriverIsMechanical,      false },
	FOLDERDATA{ "Not Working"s,     "nonworking"s,        TV_FOLDER_NONWORKING,   IDI_FOLDER_NONWORKING,    F_NONWORKING, F_WORKING,              false, nullptr,                     DriverIsBroken,          true },
	FOLDERDATA{ "Parents"s,         "parents"s,           TV_FOLDER_ORIGINAL,     IDI_FOLDER_ORIGINALS,     F_ORIGINALS,  F_CLONES,               false, nullptr,                     DriverIsClone,           false },
	FOLDERDATA{ "Raster"s,          "raster"s,            TV_FOLDER_RASTER,       IDI_FOLDER_RASTER,        F_RASTER,     F_VECTOR,               false, nullptr,                     DriverIsVector,          false, SOFTWARETYPE_RASTER },
	FOLDERDATA{ "Resolution"s,      "resolution"s,        TV_FOLDER_RESOLUTION,   IDI_FOLDER_RESOL,         0,            0,                      true,  CreateResolutionFolders },
	FOLDERDATA{ "Samples"s,         "samples"s,           TV_FOLDER_SAMPLES,      IDI_FOLDER_SAMPLES,       0,            0,                      false, nullptr,                     DriverUsesSamples,       true },
	FOLDERDATA{ "Save State"s,      "savestate"s,         TV_FOLDER_SAVESTATE,    IDI_FOLDER_SAVESTATE,     0,            0,                      false, nullptr,                     DriverSupportsSaveState, true },
	FOLDERDATA{ "Screens"s,         "screens"s,           TV_FOLDER_SCREENS,      IDI_FOLDER,               0,            0,                      true,  CreateScreenFolders },
	FOLDERDATA{ "Sound"s,           "sound"s,             TV_FOLDER_SND,          IDI_FOLDER_SOUND,         0,            0,                      true,  CreateSoundFolders },
	FOLDERDATA{ "Source"s,          "source"s,            TV_FOLDER_SOURCE,       IDI_FOLDER_SOURCE,        0,            0,                      true,  CreateSourceFolders },
	FOLDERDATA{ "Stereo"s,          "stereo"s,            TV_FOLDER_STEREO,       IDI_FOLDER_SOUND,         0,            0,                      false, nullptr,                     DriverIsStereo,          true },
	FOLDERDATA{ "Trackball"s,       "trackball"s,         TV_FOLDER_TRACKBALL,    IDI_FOLDER_TRACKBALL,     0,            0,                      false, nullptr,                     DriverUsesTrackball,     true },
	FOLDERDATA{ "Unavailable"s,     "unavailable"s,       TV_FOLDER_UNAVAILABLE,  IDI_FOLDER_UNAVAILABLE,   0,            F_UNAVAILABLE,          false, nullptr,                     FilterAvailable,         false },
	FOLDERDATA{ "Vector"s,          "vector"s,            TV_FOLDER_VECTOR,       IDI_FOLDER_VECTOR,        F_VECTOR,     F_RASTER,               false, nullptr,                     DriverIsVector,          true, SOFTWARETYPE_VECTOR },
	FOLDERDATA{ "Vertical"s,        "vertical"s,          TV_FOLDER_VERTICAL,     IDI_FOLDER_VERTICAL,      F_VERTICAL,   F_HORIZONTAL,           false, nullptr,                     DriverIsVertical,        true, SOFTWARETYPE_VERTICAL },
	FOLDERDATA{ "Working"s,         "working"s,           TV_FOLDER_WORKING,      IDI_FOLDER_WORKING,       F_WORKING,    F_NONWORKING,           false, nullptr,                     DriverIsBroken,          false },
	FOLDERDATA{ "Year"s,            "year"s,              TV_FOLDER_YEAR,         IDI_FOLDER_YEAR,          0,            0,                      true,  CreateYearFolders },
};

// list of filter/control Id pairs
constexpr std::array<FILTER_ITEM, TV_FILTER_COUNT> g_filterList =
{{
	{ F_ARCADE,       IDC_FILTER_ARCADE,      DriverIsArcade,     true },
	{ F_CLONES,       IDC_FILTER_CLONES,      DriverIsClone,      true },
	{ F_HORIZONTAL,   IDC_FILTER_HORIZONTAL,  DriverIsVertical,   false },
	{ F_MECHANICAL,   IDC_FILTER_MECHANICAL,  DriverIsMechanical, true },
	{ F_MESS,         IDC_FILTER_MESS,        DriverIsArcade,     false },
//  { F_MODIFIED,     IDC_FILTER_MODIFIED,    DriverIsModified,   true },
	{ F_NONWORKING,   IDC_FILTER_NONWORKING,  DriverIsBroken,     true },
	{ F_ORIGINALS,    IDC_FILTER_ORIGINALS,   DriverIsClone,      false },
	{ F_RASTER,       IDC_FILTER_RASTER,      DriverIsVector,     false },
	{ F_UNAVAILABLE,  IDC_FILTER_UNAVAILABLE, FilterAvailable,    false },
	{ F_VECTOR,       IDC_FILTER_VECTOR,      DriverIsVector,     true },
	{ F_VERTICAL,     IDC_FILTER_VERTICAL,    DriverIsVertical,   true },
	{ F_WORKING,      IDC_FILTER_WORKING,     DriverIsBroken,     false },
#ifdef MESS
	{ F_COMPUTER,     IDC_FILTER_COMPUTER,    DriverIsComputer,   true },
	{ F_CONSOLE,      IDC_FILTER_CONSOLE,     DriverIsConsole,    true },
#else
	{ F_AVAILABLE,    IDC_FILTER_AVAILABLE,   FilterAvailable,    true },
#endif
}};

constexpr std::array<MAMEHELPINFO, HELPINFO_COUNT> g_helpInfo =
{{
	{ ID_HELP_CONTENTS,  true,  MAMEUIHELP_CONTENTS },
//  { ID_HELP_TROUBLE,   true,  MAMEUIHELP_TROUBLE },
//  { ID_HELP_RELEASE,   false, MAMEUIHELP_RELEASE },
	{ ID_HELP_WHATS_NEW, true,  MAMEUIHELP_WHATSNEW },
}};

constexpr std::array<PROPERTYSHEETINFO, SHEET_COUNT> g_propSheets =
{{
	{ false, nullptr,        IDD_PROP_GAME,          GamePropertiesDialogProc },
	{ false, nullptr,        IDD_PROP_AUDIT,         GameAuditDialogProc },
	{ true,  nullptr,        IDD_PROP_DISPLAY,       GameOptionsProc },
	{ true,  nullptr,        IDD_PROP_ADVANCED,      GameOptionsProc },
	{ true,  nullptr,        IDD_PROP_SCREEN,        GameOptionsProc },
	{ true,  nullptr,        IDD_PROP_SOUND,         GameOptionsProc },
	{ true,  nullptr,        IDD_PROP_INPUT,         GameOptionsProc },
	{ true,  nullptr,        IDD_PROP_CONTROLLER,    GameOptionsProc },
	{ true,  nullptr,        IDD_PROP_MISC,          GameOptionsProc },
	{ true,  nullptr,        IDD_PROP_LUA,           GameOptionsProc },
	{ true,  nullptr,        IDD_PROP_OPENGL,        GameOptionsProc },
	{ true,  nullptr,        IDD_PROP_SHADER,        GameOptionsProc },
	{ true,  nullptr,        IDD_PROP_SNAP,          GameOptionsProc },
#ifdef MESS
	{ false, nullptr,        IDD_PROP_SOFTWARE,      GameMessOptionsProc },
	// Commit 2d0a09b removed the ability to configure ":ram" from the command line & not having a UI
//  { false, DriverHasRam,   IDD_PROP_CONFIGURATION, GameMessOptionsProc }, // PropSheetFilter_Config not needed
#endif
	{ true,  DriverIsVector, IDD_PROP_VECTOR,        GameOptionsProc },     // PropSheetFilter_Vector not needed
}};

constexpr std::array<ICONDATA, ICON_COUNT> g_iconData =
{ {
	{ IDI_WIN_NOROMS,        "noroms"s },
	{ IDI_WIN_ROMS,          "roms"s },
	{ IDI_WIN_UNKNOWN,       "unknown"s },
	{ IDI_WIN_CLONE,         "clone"s },
	{ IDI_WIN_REDX,          "warning"s },
	{ IDI_WIN_IMPERFECT,     "imperfect"s },
#ifdef MESS
	{ IDI_WIN_NOROMSNEEDED,  "noromsneeded"s },
	{ IDI_WIN_MISSINGOPTROM, "missingoptrom"s },
	{ IDI_WIN_FLOP,          "floppy"s },
	{ IDI_WIN_CASS,          "cassette"s },
	{ IDI_WIN_SERL,          "serial"s },
	{ IDI_WIN_SNAP,          "snapshot"s },
	{ IDI_WIN_PRIN,          "printer"s },
	{ IDI_WIN_HARD,          "hard"s },
	{ IDI_WIN_MIDI,          "midi"s },
	{ IDI_WIN_CYLN,          "cyln"s },
	{ IDI_WIN_PTAP,          "ptap"s },
	{ IDI_WIN_PCRD,          "pcrd"s },
	{ IDI_WIN_MEMC,          "memc"s },
	{ IDI_WIN_CDRM,          "cdrm"s },
	{ IDI_WIN_MTAP,          "mtap"s },
	{ IDI_WIN_CART,          "cart"s },
#endif
}};
