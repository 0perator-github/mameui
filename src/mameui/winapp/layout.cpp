// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

/***************************************************************************

  layout.cpp

  MAME specific TreeView definitions (and maybe more in the future)

***************************************************************************/

// standard windows headers
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>

// MAME headers
#include "mameheaders.h"

// MAMEUI headers
#include "bitmask.h"
#include "emu_opts.h"
#include "help.h"
#include "mui_audit.h"
#include "mui_opts.h"
#include "mui_util.h"
#include "properties.h"
#include "resource.h"
#include "screenshot.h"
#include "splitters.h"
#include "treeview.h"
#include "winui.h"


static bool FilterAvailable(uint32_t driver_index)
{
	return !DriverUsesRoms(driver_index) || IsAuditResultYes(GetRomAuditResults(driver_index));
}

#ifdef MESS
extern constexpr FOLDERDATA g_folderData[] =
{
	{"All Systems",     "allgames",          FOLDER_ALLGAMES,     IDI_FOLDER_ALLGAMES,      0,             0,            0, NULL,                       NULL,                    TRUE },
	{"Available",       "available",         FOLDER_AVAILABLE,    IDI_FOLDER_AVAILABLE,     F_AVAILABLE,   0,            0, NULL,                       FilterAvailable,         TRUE },
	{"BIOS",            "bios",              FOLDER_BIOS,         IDI_FOLDER_BIOS,          0,             0,            1, CreateBIOSFolders,          DriverIsBios,            TRUE },
	{"CHD",             "harddisk",          FOLDER_HARDDISK,     IDI_FOLDER_HARDDISK,      0,             0,            0, NULL,                       DriverIsHarddisk,        TRUE },
	{"Clones",          "clones",            FOLDER_CLONES,       IDI_FOLDER_CLONES,        F_CLONES,      F_ORIGINALS,  0, NULL,                       DriverIsClone,           TRUE },
	{"Computer",        "computer",          FOLDER_COMPUTER,     IDI_FOLDER,               F_COMPUTER,    F_CONSOLE,    0, NULL,                       DriverIsComputer,        TRUE, OPTIONS_COMPUTER },
	{"Console",         "console",           FOLDER_CONSOLE,      IDI_FOLDER,               F_CONSOLE,     F_COMPUTER,   0, NULL,                       DriverIsConsole,         TRUE, OPTIONS_CONSOLE },
	{"CPU",             "cpu",               FOLDER_CPU,          IDI_FOLDER_CPU,           0,             0,            1, CreateCPUFolders },
	{"Dumping Status",  "dumping",           FOLDER_DUMPING,      IDI_FOLDER_DUMP,          0,             0,            1, CreateDumpingFolders },
	{"FPS",             "fps",               FOLDER_FPS,          IDI_FOLDER_FPS,           0,             0,            1, CreateFPSFolders },
	{"Horizontal",      "horizontal",        FOLDER_HORIZONTAL,   IDI_FOLDER_HORIZONTAL,    F_HORIZONTAL,  F_VERTICAL,   0, NULL,                       DriverIsVertical,        FALSE, OPTIONS_HORIZONTAL },
	{"Imperfect",       "imperfect",         FOLDER_DEFICIENCY,   IDI_FOLDER_IMPERFECT,     0,             0,            0, CreateDeficiencyFolders },
	{"Lightgun",        "Lightgun",          FOLDER_LIGHTGUN,     IDI_FOLDER_LIGHTGUN,      0,             0,            0, NULL,                       DriverUsesLightGun,      TRUE },
	{"Manufacturer",    "manufacturer",      FOLDER_MANUFACTURER, IDI_FOLDER_MANUFACTURER,  0,             0,            1, CreateManufacturerFolders },
	{"Mechanical",      "mechanical",        FOLDER_MECHANICAL,   IDI_FOLDER_MECHANICAL,    0,             0,            0, NULL,                       DriverIsMechanical,      TRUE },
	{"Modified/Hacked", "modified",          FOLDER_MODIFIED,     IDI_FOLDER,               0,             0,            0, NULL,                       DriverIsModified,        TRUE },
	{"Mouse",           "mouse",             FOLDER_MOUSE,        IDI_FOLDER,               0,             0,            0, NULL,                       DriverUsesMouse,         TRUE },
	{"Non Mechanical",  "nonmechanical",     FOLDER_NONMECHANICAL,IDI_FOLDER,               0,             0,            0, NULL,                       DriverIsMechanical,      FALSE },
	{"Not Working",     "nonworking",        FOLDER_NONWORKING,   IDI_FOLDER_NONWORKING,    F_NONWORKING,  F_WORKING,    0, NULL,                       DriverIsBroken,          TRUE },
	{"Parents",         "parents",           FOLDER_ORIGINAL,     IDI_FOLDER_ORIGINALS,     F_ORIGINALS,   F_CLONES,     0, NULL,                       DriverIsClone,           FALSE },
	{"Raster",          "raster",            FOLDER_RASTER,       IDI_FOLDER_RASTER,        F_RASTER,      F_VECTOR,     0, NULL,                       DriverIsVector,          FALSE, OPTIONS_RASTER },
	{"Resolution",      "resolution",        FOLDER_RESOLUTION,   IDI_FOLDER_RESOL,         0,             0,            1, CreateResolutionFolders },
	{"Samples",         "samples",           FOLDER_SAMPLES,      IDI_FOLDER_SAMPLES,       0,             0,            0, NULL,                       DriverUsesSamples,       TRUE },
	{"Save State",      "savestate",         FOLDER_SAVESTATE,    IDI_FOLDER_SAVESTATE,     0,             0,            0, NULL,                       DriverSupportsSaveState, TRUE },
	{"Screens",         "screens",           FOLDER_SCREENS,      IDI_FOLDER,               0,             0,            1, CreateScreenFolders },
	{"Sound",           "sound",             FOLDER_SND,          IDI_FOLDER_SOUND,         0,             0,            1, CreateSoundFolders },
	{"Source",          "source",            FOLDER_SOURCE,       IDI_FOLDER_SOURCE,        0,             0,            1, CreateSourceFolders },
	{"Stereo",          "stereo",            FOLDER_STEREO,       IDI_FOLDER_SOUND,         0,             0,            0, NULL,                       DriverIsStereo,          TRUE },
	{"Trackball",       "trackball",         FOLDER_TRACKBALL,    IDI_FOLDER_TRACKBALL,     0,             0,            0, NULL,                       DriverUsesTrackball,     TRUE },
	{"Unavailable",     "unavailable",       FOLDER_UNAVAILABLE,  IDI_FOLDER_UNAVAILABLE,   0,             F_AVAILABLE,  0, NULL,                       FilterAvailable,         FALSE },
	{"Vector",          "vector",            FOLDER_VECTOR,       IDI_FOLDER_VECTOR,        F_VECTOR,      F_RASTER,     0, NULL,                       DriverIsVector,          TRUE, OPTIONS_VECTOR },
	{"Vertical",        "vertical",          FOLDER_VERTICAL,     IDI_FOLDER_VERTICAL,      F_VERTICAL,    F_HORIZONTAL, 0, NULL,                       DriverIsVertical,        TRUE, OPTIONS_VERTICAL },
	{"Working",         "working",           FOLDER_WORKING,      IDI_FOLDER_WORKING,       F_WORKING,     F_NONWORKING, 0, NULL,                       DriverIsBroken,          FALSE },
	{"Year",            "year",              FOLDER_YEAR,         IDI_FOLDER_YEAR,          0,             0,            1, CreateYearFolders },
	{ NULL }
};
#else
extern constexpr FOLDERDATA g_folderData[] =
{
	{"All Games",       "allgames",          FOLDER_ALLGAMES,     IDI_FOLDER_ALLGAMES,      0,             0,            0, NULL,                       NULL,                    TRUE },
	{"Available",       "available",         FOLDER_AVAILABLE,    IDI_FOLDER_AVAILABLE,     F_AVAILABLE,   0,            0, NULL,                       FilterAvailable,         TRUE },
	{"BIOS",            "bios",              FOLDER_BIOS,         IDI_FOLDER_BIOS,          0,             0,            1, CreateBIOSFolders,          DriverIsBios,            TRUE },
	{"CHD",             "harddisk",          FOLDER_HARDDISK,     IDI_FOLDER_HARDDISK,      0,             0,            0, NULL,                       DriverIsHarddisk,        TRUE },
	{"Clones",          "clones",            FOLDER_CLONES,       IDI_FOLDER_CLONES,        F_CLONES,      F_ORIGINALS,  0, NULL,                       DriverIsClone,           TRUE },
	{"CPU",             "cpu",               FOLDER_CPU,          IDI_FOLDER_CPU,           0,             0,            1, CreateCPUFolders },
	{"Dumping Status",  "dumping",           FOLDER_DUMPING,      IDI_FOLDER_DUMP,          0,             0,            1, CreateDumpingFolders },
	{"FPS",             "fps",               FOLDER_FPS,          IDI_FOLDER_FPS,           0,             0,            1, CreateFPSFolders },
	{"Horizontal",      "horizontal",        FOLDER_HORIZONTAL,   IDI_FOLDER_HORIZONTAL,    F_HORIZONTAL,  F_VERTICAL,   0, NULL,                       DriverIsVertical,        FALSE, OPTIONS_HORIZONTAL },
	{"Imperfect",       "imperfect",         FOLDER_DEFICIENCY,   IDI_FOLDER_IMPERFECT,     0,             0,            0, CreateDeficiencyFolders },
	{"Lightgun",        "Lightgun",          FOLDER_LIGHTGUN,     IDI_FOLDER_LIGHTGUN,      0,             0,            0, NULL,                       DriverUsesLightGun,      TRUE },
	{"Manufacturer",    "manufacturer",      FOLDER_MANUFACTURER, IDI_FOLDER_MANUFACTURER,  0,             0,            1, CreateManufacturerFolders },
	{"Mechanical",      "mechanical",        FOLDER_MECHANICAL,   IDI_FOLDER_MECHANICAL,    0,             0,            0, NULL,                       DriverIsMechanical,      TRUE },
	{"Non Mechanical",  "nonmechanical",     FOLDER_NONMECHANICAL,IDI_FOLDER,               0,             0,            0, NULL,                       DriverIsMechanical,      FALSE },
	{"Not Working",     "nonworking",        FOLDER_NONWORKING,   IDI_FOLDER_NONWORKING,    F_NONWORKING,  F_WORKING,    0, NULL,                       DriverIsBroken,          TRUE },
	{"Parents",         "parents",           FOLDER_ORIGINAL,     IDI_FOLDER_ORIGINALS,     F_ORIGINALS,   F_CLONES,     0, NULL,                       DriverIsClone,           FALSE },
	{"Raster",          "raster",            FOLDER_RASTER,       IDI_FOLDER_RASTER,        F_RASTER,      F_VECTOR,     0, NULL,                       DriverIsVector,          FALSE, OPTIONS_RASTER },
	{"Resolution",      "resolution",        FOLDER_RESOLUTION,   IDI_FOLDER_RESOL,         0,             0,            1, CreateResolutionFolders },
	{"Samples",         "samples",           FOLDER_SAMPLES,      IDI_FOLDER_SAMPLES,       0,             0,            0, NULL,                       DriverUsesSamples,       TRUE },
	{"Save State",      "savestate",         FOLDER_SAVESTATE,    IDI_FOLDER_SAVESTATE,     0,             0,            0, NULL,                       DriverSupportsSaveState, TRUE },
	{"Screens",         "screens",           FOLDER_SCREENS,      IDI_FOLDER,               0,             0,            1, CreateScreenFolders },
	{"Sound",           "sound",             FOLDER_SND,          IDI_FOLDER_SOUND,         0,             0,            1, CreateSoundFolders },
	{"Source",          "source",            FOLDER_SOURCE,       IDI_FOLDER_SOURCE,        0,             0,            1, CreateSourceFolders },
	{"Stereo",          "stereo",            FOLDER_STEREO,       IDI_FOLDER_SOUND,         0,             0,            0, NULL,                       DriverIsStereo,          TRUE },
	{"Trackball",       "trackball",         FOLDER_TRACKBALL,    IDI_FOLDER_TRACKBALL,     0,             0,            0, NULL,                       DriverUsesTrackball,     TRUE },
	{"Unavailable",     "unavailable",       FOLDER_UNAVAILABLE,  IDI_FOLDER_UNAVAILABLE,   0,             F_AVAILABLE,  0, NULL,                       FilterAvailable,         FALSE },
	{"Vector",          "vector",            FOLDER_VECTOR,       IDI_FOLDER_VECTOR,        F_VECTOR,      F_RASTER,     0, NULL,                       DriverIsVector,          TRUE, OPTIONS_VECTOR },
	{"Vertical",        "vertical",          FOLDER_VERTICAL,     IDI_FOLDER_VERTICAL,      F_VERTICAL,    F_HORIZONTAL, 0, NULL,                       DriverIsVertical,        TRUE, OPTIONS_VERTICAL },
	{"Working",         "working",           FOLDER_WORKING,      IDI_FOLDER_WORKING,       F_WORKING,     F_NONWORKING, 0, NULL,                       DriverIsBroken,          FALSE },
	{"Year",            "year",              FOLDER_YEAR,         IDI_FOLDER_YEAR,          0,             0,            1, CreateYearFolders },
	{ NULL }
};
#endif

/* list of filter/control Id pairs */
extern constexpr FILTER_ITEM g_filterList[] =
{
	{ F_ARCADE,       IDC_FILTER_ARCADE,      DriverIsArcade, TRUE },
	{ F_CLONES,       IDC_FILTER_CLONES,      DriverIsClone, TRUE },
	{ F_HORIZONTAL,   IDC_FILTER_HORIZONTAL,  DriverIsVertical, FALSE },
	{ F_MECHANICAL,   IDC_FILTER_MECHANICAL,  DriverIsMechanical, TRUE },
	{ F_MESS,         IDC_FILTER_MESS,        DriverIsArcade, FALSE },
//  { F_MODIFIED,     IDC_FILTER_MODIFIED,    DriverIsModified, TRUE },
	{ F_NONWORKING,   IDC_FILTER_NONWORKING,  DriverIsBroken, TRUE },
	{ F_ORIGINALS,    IDC_FILTER_ORIGINALS,   DriverIsClone, FALSE },
	{ F_RASTER,       IDC_FILTER_RASTER,      DriverIsVector, FALSE },
	{ F_UNAVAILABLE,  IDC_FILTER_UNAVAILABLE, FilterAvailable, FALSE },
	{ F_VECTOR,       IDC_FILTER_VECTOR,      DriverIsVector, TRUE },
	{ F_VERTICAL,     IDC_FILTER_VERTICAL,    DriverIsVertical, TRUE },
	{ F_WORKING,      IDC_FILTER_WORKING,     DriverIsBroken, FALSE },
#ifndef MESS
	{ F_AVAILABLE,    IDC_FILTER_AVAILABLE,   FilterAvailable, TRUE },
#else
	{ F_COMPUTER,     IDC_FILTER_COMPUTER,    DriverIsComputer, TRUE },
	{ F_CONSOLE,      IDC_FILTER_CONSOLE,     DriverIsConsole, TRUE },
#endif
	{ 0 }
};

extern constexpr MAMEHELPINFO g_helpInfo[] =
{
//  { ID_HELP_CONTENTS,    TRUE,  MAMEUIHELP"::/windows/main.htm" },
	{ ID_HELP_CONTENTS,    TRUE,  MAMEUIHELP }, // 0 - call up CHM file
//  { ID_HELP_TROUBLE,     TRUE,  MAMEUIHELP"::/html/mameui_support.htm" },
//  { ID_HELP_RELEASE,     FALSE, TEXT("windows.txt") },
#ifdef MESS
//  { ID_HELP_WHATS_NEW,   TRUE,  MAMEUIHELP"::/messnew.txt" },
	{ ID_HELP_WHATS_NEW,   TRUE,  L"" }, // 1 - call up whatsnew at mamedev.org
#else
//  { ID_HELP_WHATS_NEW, TRUE,  MAMEUIHELP"::/html/mameui_changes.txt" },
	{ ID_HELP_WHATS_NEW,   TRUE,  MAMEUIHELP+L"::/docs/whatsnew.txt" },
#endif
	{ -1 }
};

extern constexpr PROPERTYSHEETINFO g_propSheets[] =
{
	{ FALSE, NULL,                   IDD_PROP_GAME,          GamePropertiesDialogProc },
	{ FALSE, NULL,                   IDD_PROP_AUDIT,         GameAuditDialogProc },
	{ TRUE,  NULL,                   IDD_PROP_DISPLAY,       GameOptionsProc },
	{ TRUE,  NULL,                   IDD_PROP_ADVANCED,      GameOptionsProc },
	{ TRUE,  NULL,                   IDD_PROP_SCREEN,        GameOptionsProc },
	{ TRUE,  NULL,                   IDD_PROP_SOUND,         GameOptionsProc },
	{ TRUE,  NULL,                   IDD_PROP_INPUT,         GameOptionsProc },
	{ TRUE,  NULL,                   IDD_PROP_CONTROLLER,    GameOptionsProc },
	{ TRUE,  NULL,                   IDD_PROP_MISC,          GameOptionsProc },
	{ TRUE,  NULL,                   IDD_PROP_LUA,           GameOptionsProc },
	{ TRUE,  NULL,                   IDD_PROP_OPENGL,        GameOptionsProc },
	{ TRUE,  NULL,                   IDD_PROP_SHADER,        GameOptionsProc },
	{ TRUE,  NULL,                   IDD_PROP_SNAP,          GameOptionsProc },
#ifdef MESS
	{ FALSE, NULL,                   IDD_PROP_SOFTWARE,      GameMessOptionsProc },
	{ FALSE, DriverHasRam,           IDD_PROP_CONFIGURATION, GameMessOptionsProc }, // PropSheetFilter_Config not needed
#endif
	{ TRUE,  DriverIsVector,         IDD_PROP_VECTOR,        GameOptionsProc },     // PropSheetFilter_Vector not needed
	{ FALSE }
};

extern constexpr ICONDATA g_iconData[] =
{
	{ IDI_WIN_NOROMS,        "noroms" },
	{ IDI_WIN_ROMS,          "roms" },
	{ IDI_WIN_UNKNOWN,       "unknown" },
	{ IDI_WIN_CLONE,         "clone" },
	{ IDI_WIN_REDX,          "warning" },
	{ IDI_WIN_IMPERFECT,     "imperfect" },
#ifdef MESS
	{ IDI_WIN_NOROMSNEEDED,  "noromsneeded" },
	{ IDI_WIN_MISSINGOPTROM, "missingoptrom" },
	{ IDI_WIN_FLOP,          "floppy" },
	{ IDI_WIN_CASS,          "cassette" },
	{ IDI_WIN_SERL,          "serial" },
	{ IDI_WIN_SNAP,          "snapshot" },
	{ IDI_WIN_PRIN,          "printer" },
	{ IDI_WIN_HARD,          "hard" },
	{ IDI_WIN_MIDI,          "midi" },
	{ IDI_WIN_CYLN,          "cyln" },
	{ IDI_WIN_PTAP,          "ptap" },
	{ IDI_WIN_PCRD,          "pcrd" },
	{ IDI_WIN_MEMC,          "memc" },
	{ IDI_WIN_CDRM,          "cdrm" },
	{ IDI_WIN_MTAP,          "mtap" },
	{ IDI_WIN_CART,          "cart" },
#endif
	{ 0 }
};
