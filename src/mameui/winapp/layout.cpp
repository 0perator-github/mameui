// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

/***************************************************************************

  layout.cpp

  MAME specific TreeView definitions (and maybe more in the future)

***************************************************************************/

// standard C++ headers
#include <filesystem>
#include <memory>
#include <vector> // bitmask.h

// standard windows headers
#include "winapi_common.h"

// MAME headers

// shlwapi.h pulls in objbase.h which defines interface as a struct which conflict with a template declaration in device.h
#ifdef interface
#undef interface // undef interface which is for COM interfaces
#endif
#include "emu.h"
#define interface struct

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

static bool FilterAvailable(int driver_index)
{
	return !DriverUsesRoms(driver_index) || IsAuditResultYes(GetRomAuditResults(driver_index));
}

#ifdef MESS
extern const FOLDERDATA g_folderData[] =
{
	{"All Systems"s,     "allgames"s,          FOLDER_ALLGAMES,     IDI_FOLDER_ALLGAMES,      0,             0,                     false, nullptr,                     nullptr,                 true },
	{"Arcade"s,          "arcade"s,            FOLDER_ARCADE,       IDI_FOLDER,               F_ARCADE,      0,                     false, nullptr,                     DriverIsArcade,          true, SOFTWARETYPE_ARCADE },
	{"Available"s,       "available"s,         FOLDER_AVAILABLE,    IDI_FOLDER_AVAILABLE,     F_AVAILABLE,   0,                     false, nullptr,                     FilterAvailable,         true },
	{"BIOS"s,            "bios"s,              FOLDER_BIOS,         IDI_FOLDER_BIOS,          0,             0,                     true,  CreateBIOSFolders,           DriverIsBios,            true },
	{"CHD"s,             "harddisk"s,          FOLDER_HARDDISK,     IDI_FOLDER_HARDDISK,      0,             0,                     false, nullptr,                     DriverIsHarddisk,        true },
	{"Clones"s,          "clones"s,            FOLDER_CLONES,       IDI_FOLDER_CLONES,        F_CLONES,      F_ORIGINALS,           false, nullptr,                     DriverIsClone,           true },
	{"Computer"s,        "computer"s,          FOLDER_COMPUTER,     IDI_FOLDER,               F_COMPUTER,    F_CONSOLE,             false, nullptr,                     DriverIsComputer,        true, SOFTWARETYPE_COMPUTER },
	{"Console"s,         "console"s,           FOLDER_CONSOLE,      IDI_FOLDER,               F_CONSOLE,     F_COMPUTER,            false, nullptr,                     DriverIsConsole,         true, SOFTWARETYPE_CONSOLE },
	{"CPU"s,             "cpu"s,               FOLDER_CPU,          IDI_FOLDER_CPU,           0,             0,                     true,  CreateCPUFolders },
	{"Dumping Status"s,  "dumping"s,           FOLDER_DUMPING,      IDI_FOLDER_DUMP,          0,             0,                     true,  CreateDumpingFolders },
	{"FPS"s,             "fps"s,               FOLDER_FPS,          IDI_FOLDER_FPS,           0,             0,                     true,  CreateFPSFolders },
	{"Horizontal"s,      "horizontal"s,        FOLDER_HORIZONTAL,   IDI_FOLDER_HORIZONTAL,    F_HORIZONTAL,  F_VERTICAL,            false, nullptr,                     DriverIsVertical,        false, SOFTWARETYPE_HORIZONTAL },
	{"Imperfect"s,       "imperfect"s,         FOLDER_DEFICIENCY,   IDI_FOLDER_IMPERFECT,     0,             0,                     false, CreateDeficiencyFolders },
	{"Lightgun"s,        "Lightgun"s,          FOLDER_LIGHTGUN,     IDI_FOLDER_LIGHTGUN,      0,             0,                     false, nullptr,                     DriverUsesLightGun,      true },
	{"Manufacturer"s,    "manufacturer"s,      FOLDER_MANUFACTURER, IDI_FOLDER_MANUFACTURER,  0,             0,                     true,  CreateManufacturerFolders },
	{"Mechanical"s,      "mechanical"s,        FOLDER_MECHANICAL,   IDI_FOLDER_MECHANICAL,    0,             0,                     false, nullptr,                     DriverIsMechanical,      true },
	{"Modified/Hacked"s, "modified"s,          FOLDER_MODIFIED,     IDI_FOLDER,               0,             0,                     false, nullptr,                     DriverIsModified,        true },
	{"Mouse"s,           "mouse"s,             FOLDER_MOUSE,        IDI_FOLDER,               0,             0,                     false, nullptr,                     DriverUsesMouse,         true },
	{"Non Mechanical"s,  "nonmechanical"s,     FOLDER_NONMECHANICAL,IDI_FOLDER,               0,             0,                     false, nullptr,                     DriverIsMechanical,      false },
	{"Not Working"s,     "nonworking"s,        FOLDER_NONWORKING,   IDI_FOLDER_NONWORKING,    F_NONWORKING,  F_WORKING,             false, nullptr,                     DriverIsBroken,          true },
	{"Parents"s,         "parents"s,           FOLDER_ORIGINAL,     IDI_FOLDER_ORIGINALS,     F_ORIGINALS,   F_CLONES,              false, nullptr,                     DriverIsClone,           false },
	{"Raster"s,          "raster"s,            FOLDER_RASTER,       IDI_FOLDER_RASTER,        F_RASTER,      F_VECTOR,              false, nullptr,                     DriverIsVector,          false, SOFTWARETYPE_RASTER },
	{"Resolution"s,      "resolution"s,        FOLDER_RESOLUTION,   IDI_FOLDER_RESOL,         0,             0,                     true,  CreateResolutionFolders },
	{"Samples"s,         "samples"s,           FOLDER_SAMPLES,      IDI_FOLDER_SAMPLES,       0,             0,                     false, nullptr,                     DriverUsesSamples,       true },
	{"Save State"s,      "savestate"s,         FOLDER_SAVESTATE,    IDI_FOLDER_SAVESTATE,     0,             0,                     false, nullptr,                     DriverSupportsSaveState, true },
	{"Screens"s,         "screens"s,           FOLDER_SCREENS,      IDI_FOLDER,               0,             0,                     true,  CreateScreenFolders },
	{"Sound"s,           "sound"s,             FOLDER_SND,          IDI_FOLDER_SOUND,         0,             0,                     true,  CreateSoundFolders },
	{"Source"s,          "source"s,            FOLDER_SOURCE,       IDI_FOLDER_SOURCE,        0,             0,                     true,  CreateSourceFolders },
	{"Stereo"s,          "stereo"s,            FOLDER_STEREO,       IDI_FOLDER_SOUND,         0,             0,                     false, nullptr,                     DriverIsStereo,          true },
	{"Trackball"s,       "trackball"s,         FOLDER_TRACKBALL,    IDI_FOLDER_TRACKBALL,     0,             0,                     false, nullptr,                     DriverUsesTrackball,     true },
	{"Unavailable"s,     "unavailable"s,       FOLDER_UNAVAILABLE,  IDI_FOLDER_UNAVAILABLE,   0,             F_AVAILABLE,           false, nullptr,                     FilterAvailable,         false },
	{"Vector"s,          "vector"s,            FOLDER_VECTOR,       IDI_FOLDER_VECTOR,        F_VECTOR,      F_RASTER,              false, nullptr,                     DriverIsVector,          true, SOFTWARETYPE_VECTOR },
	{"Vertical"s,        "vertical"s,          FOLDER_VERTICAL,     IDI_FOLDER_VERTICAL,      F_VERTICAL,    F_HORIZONTAL,          false, nullptr,                     DriverIsVertical,        true, SOFTWARETYPE_VERTICAL },
	{"Working"s,         "working"s,           FOLDER_WORKING,      IDI_FOLDER_WORKING,       F_WORKING,     F_NONWORKING,          false, nullptr,                     DriverIsBroken,          false },
	{"Year"s,            "year"s,              FOLDER_YEAR,         IDI_FOLDER_YEAR,          0,             0,                     true,  CreateYearFolders },
	{}
};
#else
extern const FOLDERDATA g_folderData[] =
{
	{"All Systems"s,     "allgames"s,          FOLDER_ALLGAMES,     IDI_FOLDER_ALLGAMES,      0,             0,                     false, nullptr,                     nullptr,                 true },
	{"Arcade"s,          "arcade"s,            FOLDER_ARCADE,       IDI_FOLDER,               F_ARCADE,     F_CONSOLE | F_COMPUTER, false, nullptr,                     DriverIsArcade,          true, SOFTWARETYPE_ARCADE },
	{"Available"s,       "available"s,         FOLDER_AVAILABLE,    IDI_FOLDER_AVAILABLE,     F_AVAILABLE,   0,                     false, nullptr,                     FilterAvailable,         true },
	{"BIOS"s,            "bios"s,              FOLDER_BIOS,         IDI_FOLDER_BIOS,          0,             0,                     true,  CreateBIOSFolders,           DriverIsBios,            true },
	{"CHD"s,             "harddisk"s,          FOLDER_HARDDISK,     IDI_FOLDER_HARDDISK,      0,             0,                     false, nullptr,                     DriverIsHarddisk,        true },
	{"Clones"s,          "clones"s,            FOLDER_CLONES,       IDI_FOLDER_CLONES,        F_CLONES,      F_ORIGINALS,           false, nullptr,                     DriverIsClone,           true },
	{"CPU"s,             "cpu"s,               FOLDER_CPU,          IDI_FOLDER_CPU,           0,             0,                     true,  CreateCPUFolders },
	{"Dumping Status"s,  "dumping"s,           FOLDER_DUMPING,      IDI_FOLDER_DUMP,          0,             0,                     true,  CreateDumpingFolders },
	{"FPS"s,             "fps"s,               FOLDER_FPS,          IDI_FOLDER_FPS,           0,             0,                     true,  CreateFPSFolders },
	{"Horizontal"s,      "horizontal"s,        FOLDER_HORIZONTAL,   IDI_FOLDER_HORIZONTAL,    F_HORIZONTAL,  F_VERTICAL,            false, nullptr,                     DriverIsVertical,        false, SOFTWARETYPE_HORIZONTAL },
	{"Imperfect"s,       "imperfect"s,         FOLDER_DEFICIENCY,   IDI_FOLDER_IMPERFECT,     0,             0,                     false, CreateDeficiencyFolders },
	{"Lightgun"s,        "Lightgun"s,          FOLDER_LIGHTGUN,     IDI_FOLDER_LIGHTGUN,      0,             0,                     false, nullptr,                     DriverUsesLightGun,      true },
	{"Manufacturer"s,    "manufacturer"s,      FOLDER_MANUFACTURER, IDI_FOLDER_MANUFACTURER,  0,             0,                     true,  CreateManufacturerFolders },
	{"Mechanical"s,      "mechanical"s,        FOLDER_MECHANICAL,   IDI_FOLDER_MECHANICAL,    0,             0,                     false, nullptr,                     DriverIsMechanical,      true },
	{"Modified/Hacked"s, "modified"s,          FOLDER_MODIFIED,     IDI_FOLDER,               0,             0,                     false, nullptr,                     DriverIsModified,        true },
	{"Mouse"s,           "mouse"s,             FOLDER_MOUSE,        IDI_FOLDER,               0,             0,                     false, nullptr,                     DriverUsesMouse,         true },
	{"Non Mechanical"s,  "nonmechanical"s,     FOLDER_NONMECHANICAL,IDI_FOLDER,               0,             0,                     false, nullptr,                     DriverIsMechanical,      false },
	{"Not Working"s,     "nonworking"s,        FOLDER_NONWORKING,   IDI_FOLDER_NONWORKING,    F_NONWORKING,  F_WORKING,             false, nullptr,                     DriverIsBroken,          true },
	{"Parents"s,         "parents"s,           FOLDER_ORIGINAL,     IDI_FOLDER_ORIGINALS,     F_ORIGINALS,   F_CLONES,              false, nullptr,                     DriverIsClone,           false },
	{"Raster"s,          "raster"s,            FOLDER_RASTER,       IDI_FOLDER_RASTER,        F_RASTER,      F_VECTOR,              false, nullptr,                     DriverIsVector,          false, SOFTWARETYPE_RASTER },
	{"Resolution"s,      "resolution"s,        FOLDER_RESOLUTION,   IDI_FOLDER_RESOL,         0,             0,                     true,  CreateResolutionFolders },
	{"Samples"s,         "samples"s,           FOLDER_SAMPLES,      IDI_FOLDER_SAMPLES,       0,             0,                     false, nullptr,                     DriverUsesSamples,       true },
	{"Save State"s,      "savestate"s,         FOLDER_SAVESTATE,    IDI_FOLDER_SAVESTATE,     0,             0,                     false, nullptr,                     DriverSupportsSaveState, true },
	{"Screens"s,         "screens"s,           FOLDER_SCREENS,      IDI_FOLDER,               0,             0,                     true,  CreateScreenFolders },
	{"Sound"s,           "sound"s,             FOLDER_SND,          IDI_FOLDER_SOUND,         0,             0,                     true,  CreateSoundFolders },
	{"Source"s,          "source"s,            FOLDER_SOURCE,       IDI_FOLDER_SOURCE,        0,             0,                     true,  CreateSourceFolders },
	{"Stereo"s,          "stereo"s,            FOLDER_STEREO,       IDI_FOLDER_SOUND,         0,             0,                     false, nullptr,                     DriverIsStereo,          true },
	{"Trackball"s,       "trackball"s,         FOLDER_TRACKBALL,    IDI_FOLDER_TRACKBALL,     0,             0,                     false, nullptr,                     DriverUsesTrackball,     true },
	{"Unavailable"s,     "unavailable"s,       FOLDER_UNAVAILABLE,  IDI_FOLDER_UNAVAILABLE,   0,             F_AVAILABLE,           false, nullptr,                     FilterAvailable,         false },
	{"Vector"s,          "vector"s,            FOLDER_VECTOR,       IDI_FOLDER_VECTOR,        F_VECTOR,      F_RASTER,              false, nullptr,                     DriverIsVector,          true, SOFTWARETYPE_VECTOR },
	{"Vertical"s,        "vertical"s,          FOLDER_VERTICAL,     IDI_FOLDER_VERTICAL,      F_VERTICAL,    F_HORIZONTAL,          false, nullptr,                     DriverIsVertical,        true, SOFTWARETYPE_VERTICAL },
	{"Working"s,         "working"s,           FOLDER_WORKING,      IDI_FOLDER_WORKING,       F_WORKING,     F_NONWORKING,          false, nullptr,                     DriverIsBroken,          false },
	{"Year"s,            "year"s,              FOLDER_YEAR,         IDI_FOLDER_YEAR,          0,             0,                     true,  CreateYearFolders },
	{}
};
#endif

/* list of filter/control Id pairs */
extern constexpr FILTER_ITEM g_filterList[] =
{
	{ F_ARCADE,       IDC_FILTER_ARCADE,      DriverIsArcade, true },
	{ F_CLONES,       IDC_FILTER_CLONES,      DriverIsClone, true },
	{ F_HORIZONTAL,   IDC_FILTER_HORIZONTAL,  DriverIsVertical, false },
	{ F_MECHANICAL,   IDC_FILTER_MECHANICAL,  DriverIsMechanical, true },
	{ F_MESS,         IDC_FILTER_MESS,        DriverIsArcade, false },
//  { F_MODIFIED,     IDC_FILTER_MODIFIED,    DriverIsModified, true },
	{ F_NONWORKING,   IDC_FILTER_NONWORKING,  DriverIsBroken, true },
	{ F_ORIGINALS,    IDC_FILTER_ORIGINALS,   DriverIsClone, true },
	{ F_RASTER,       IDC_FILTER_RASTER,      DriverIsVector, true },
	{ F_UNAVAILABLE,  IDC_FILTER_UNAVAILABLE, FilterAvailable, false },
	{ F_VECTOR,       IDC_FILTER_VECTOR,      DriverIsVector, true },
	{ F_VERTICAL,     IDC_FILTER_VERTICAL,    DriverIsVertical, true },
	{ F_WORKING,      IDC_FILTER_WORKING,     DriverIsBroken, false },
	#ifndef MESS
	{ F_AVAILABLE,    IDC_FILTER_AVAILABLE,   FilterAvailable, true },
	#else
	{ F_COMPUTER,     IDC_FILTER_COMPUTER,    DriverIsComputer, true },
	{ F_CONSOLE,      IDC_FILTER_CONSOLE,     DriverIsConsole, true },
	#endif
		{}
};

extern const MAMEHELPINFO g_helpInfo[] =
{
//  { ID_HELP_CONTENTS,    true,  MAMEUIHELP+L"::/windows/main.htm" },
	{ ID_HELP_CONTENTS,    true,  MAMEUIHELP }, // 0 - call up CHM file
//  { ID_HELP_TROUBLE,     true,  MAMEUIHELP+L"::/html/mameui_support.htm"s },
//  { ID_HELP_RELEASE,     false, TEXT("windows.txt") },
#ifdef MESS
//  { ID_HELP_WHATS_NEW,   true,  MAMEUIHELP"::/messnew.txt" },
	{ ID_HELP_WHATS_NEW,   true,  L""s }, // 1 - call up whatsnew at mamedev.org
#else
//  { ID_HELP_WHATS_NEW, true,  MAMEUIHELP+L"::/html/mameui_changes.txt"s },
	{ ID_HELP_WHATS_NEW,   true,  MAMEUIHELP+L"::/docs/whatsnew.txt"s },
#endif
	{}
};

extern constexpr PROPERTYSHEETINFO g_propSheets[] =
{
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
	{ false, DriverHasRam,   IDD_PROP_CONFIGURATION, GameMessOptionsProc }, // PropSheetFilter_Config not needed
#endif
	{ true,  DriverIsVector, IDD_PROP_VECTOR,        GameOptionsProc },     // PropSheetFilter_Vector not needed
	{}
};

extern const ICONDATA g_iconData[] =
{
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
	{}
};
