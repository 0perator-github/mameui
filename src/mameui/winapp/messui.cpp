// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************
//============================================================
//
//  messui.cpp - MESS extensions to src/osd/winui/winui.c
//
//============================================================

// standard C++ headers
#include <cassert>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <optional>
#include <vector>

// standard windows headers
#include "winapi_common.h"

// MAME headers

// shlwapi.h pulls in objbase.h which defines interface as a struct which conflict with a template declaration in device.h
#ifdef interface
#undef interface // undef interface which is for COM interfaces
#endif
#include "emu.h"
#include "softlist_dev.h"
//#define interface struct // interface is defined in this sourcefile too.

#include "drivenum.h"

#include "image_handler.h"

//#include "formats/cassimg.h"
#include "formats/flopimg.h"
#include "formats/fsblk_vec.h"

#include "imagedev/floppy.h"

#include "zippath.h"

#include "ui/moptions.h"
#include "winopts.h"

// MAMEUI headers
#include "mui_cstr.h"
#include "mui_stringtokenizer.h"
#include "mui_wcstr.h"
#include "mui_wcstrconv.h"

#include "windows_controls.h"
#include "dialog_boxes.h"
#include "windows_gdi.h"
#include "menus_other_res.h"
#include "data_access_storage.h"
#include "system_services.h"
#include "windows_messages.h"

#include "bitmask.h"
#include "columnedit.h"
#include "emu_opts.h"
#include "mui_opts.h"
#include "mui_util.h"
#include "gamepicker.h"
#include "resource.h"
#include "screenshot.h"
#include "swconfig.h" // for softwarelist.h
#include "softwarelist.h"
#include "softwarepicker.h"
#include "tabview.h"
#include "winui.h"

#include "messui.h"

using namespace mameui::util::string_util;
using namespace mameui::winapi::controls;
using namespace mameui::winapi;

using namespace std::filesystem;
using namespace std::string_literals;
using namespace std::string_view_literals;

//============================================================
//  PARAMETERS
//============================================================

#define MVIEW_PADDING 10
#define MVIEW_SPACING 21
#define LOG_SOFTWARE 1


//============================================================
//  TYPEDEFS
//============================================================

using mess_image_type = struct mess_image_type
{
	device_image_interface *const dev = nullptr;
	std::wstring ext;
	std::wstring dlgname;
};

using device_entry = struct device_entry
{
	int dline = -1;
	std::string dev_type;
	int resource = -1;
	std::wstring dlgname;
};

//============================================================
//  GLOBAL VARIABLES
//============================================================

std::string g_szSelectedItem;


//============================================================
//  LOCAL VARIABLES
//============================================================

static software_config *s_config;
static BOOL s_bIgnoreSoftwarePickerNotifies = 0;
static HWND MyColumnDialogProc_hwndPicker;
static std::unique_ptr<int[]> MyColumnDialogProc_order;
static std::unique_ptr<int[]> MyColumnDialogProc_shown;
static std::unique_ptr<int[]> mess_icon_index;
static std::map<std::string,std::string> slmap; // store folder for Media View Mount Item
static std::map<std::string,int> mvmap;  // store indicator if Media View Unmount should be enabled

static const device_entry s_devices[] =
{
	{ 0,  "unkn",  IDI_WIN_UNKNOWN, L"Unknown" },
	{ 1,  "rom",   IDI_WIN_ROMS,    L"Cartridge images" },
	{ 2,  "prom",  IDI_WIN_ROMS,    L"Cartridge images" },
	{ 3,  "cart",  IDI_WIN_CART,    L"Cartridge images" },
	{ 4,  "flop",  IDI_WIN_FLOP,    L"Floppy disk images" },
	{ 5,  "disk",  IDI_WIN_FLOP,    L"Floppy disk images" },
	{ 6,  "hard",  IDI_WIN_HARD,    L"Hard disk images" },
	{ 7,  "cyln",  IDI_WIN_CYLN,    L"Cylinders" },
	{ 8,  "cass",  IDI_WIN_CASS,    L"Cassette images" },
	{ 9,  "card",  IDI_WIN_PCRD,    L"Punchcard images" },
	{ 10, "ptap",  IDI_WIN_PTAP,    L"Punchtape images" },
	{ 11, "prin",  IDI_WIN_PRIN,    L"Printer Output" },
	{ 12, "serl",  IDI_WIN_SERL,    L"Serial Output" },
	{ 13, "dump",  IDI_WIN_SNAP,    L"Snapshots" },
	{ 14, "quik",  IDI_WIN_SNAP,    L"Quickloads" },
	{ 15, "memc",  IDI_WIN_MEMC,    L"Memory cards" },
	{ 16, "cdrm",  IDI_WIN_CDRM,    L"CD-ROM images" },
	{ 17, "mtap",  IDI_WIN_MTAP,    L"Magnetic tapes" },
	{ 18, "min",   IDI_WIN_MIDI,    L"MIDI input" },
	{ 19, "mout",  IDI_WIN_MIDI,    L"MIDI output" },
	{ 20, "bitb",  IDI_WIN_BITB,    L"Bitbanger"},
};


// columns for software picker
static const std::wstring mess_column_names[] =
{
	L"Filename",
};

// columns for software list
static const std::wstring softlist_column_names[] =
{
	L"Name",
	L"List",
	L"Description",
	L"Year",
	L"Publisher",
	L"Usage",
};

static BOOL MVstate = 0;
static BOOL has_software = 0;

using MViewCallbacks = struct mview_callbacks
{
	bool (*pfnGetOpenFileName)(device_image_interface *const dev, const std::filesystem::path &file_name);
	bool (*pfnGetCreateFileName)(device_image_interface* const dev, const std::filesystem::path &file_name);
	void (*pfnSetSelectedSoftware)(int drvindex, device_image_interface *const dev, const std::filesystem::path &file_name);
	std::wstring (*pfnGetSelectedSoftware)(int drvindex, device_image_interface *const dev);
	bool (*pfnGetOpenItemName)(device_image_interface *const dev, const std::filesystem::path &file_name);
	bool (*pfnUnmount)(device_image_interface *const dev);
};

using MViewEntry = struct mview_entry
{
	device_image_interface* dev;
	HWND hwndStatic;
	HWND hwndEdit;
	HWND hwndBrowseButton;
	WNDPROC pfnEditWndProc;
};

using MViewInfo = struct mview_info
{
	HFONT hFont;
	UINT nWidth;
	UINT slots;
	BOOL bSurpressFilenameChanged;
	const software_config *config;
	const MViewCallbacks *pCallbacks;
	MViewEntry *pEntries;
};

//============================================================
//  PROTOTYPES
//============================================================

static bool SoftwarePicker_AddDirs(HWND hwndPicker, const std::wstring_view directories, const std::wstring_view subdirectory = L""sv);
static void SoftwarePicker_OnHeaderContextMenu(POINT pt, int nColumn);
static int SoftwarePicker_GetItemImage(HWND hwndPicker, int nItem);
//static void SoftwarePicker_LeavingItem(HWND hwndSoftwarePicker, int nItem); // Not used either
static void SoftwarePicker_EnteringItem(HWND hwndSoftwarePicker, int nItem);

static void SoftwareList_OnHeaderContextMenu(POINT pt, int nColumn);
//static int SoftwareList_GetItemImage(HWND hwndPicker, int nItem); // Nah!... We don't need a declaration or a definition for a long abandoned function
static void SoftwareList_LeavingItem(HWND hwndSoftwareList, int nItem);
static void SoftwareList_EnteringItem(HWND hwndSoftwareList, int nItem);

static std::wstring_view SoftwareTabView_GetTabShortName(int tab_index);
static std::wstring_view SoftwareTabView_GetTabLongName(int tab_index);
static void SoftwareTabView_OnMoveSize(void);
static void SetupSoftwareTabView(void);

static void MessRefreshPicker(void);

static bool MView_GetOpenFileName(device_image_interface *const dev, const std::filesystem::path &file_name);
static std::wstring GetSoftwareNameFromPath(const std::filesystem::path &full_path);
static bool MView_GetOpenItemName(device_image_interface *const dev, const std::filesystem::path &file_name);
static bool MView_GetCreateFileName(device_image_interface* const dev, const std::filesystem::path &file_name);
static bool MView_Unmount(device_image_interface *const dev);
static void MView_SetSelectedSoftware(int drv_index, device_image_interface *const dev, const std::filesystem::path &file_name);
static std::wstring MView_GetSelectedSoftware(int drv_index, device_image_interface *const dev);

static const floppy_image_format_t *FID_FindImgFileFormat(floppy_image_device *const flop_dev, const std::filesystem::path &file_path);
static std::vector<const floppy_image_format_t*> FID_GetWritableFormats(floppy_image_device *const flop_dev);
static const floppy_image_device::fs_info *FID_GetFsInfoByFormat(floppy_image_device *const flop_dev, const floppy_image_format_t *flop_fmt);
static bool MView_CreateFloppyDskImgFile(floppy_image_device *const flop_dev, const std::filesystem::path &file_path);

static bool DII_FindImgFileFormat(device_image_interface *const dev, const std::filesystem::path &file_path);
static bool MView_DeviceImgIfaceCreateFile(device_image_interface *const dev, const std::filesystem::path &file_path);

static void add_image_type(device_image_interface *const dev, std::wstring_view file_extensions, std::vector<mess_image_type> &image_types);
static const device_entry* lookupdevice(std::string device_type);
static bool images_type_ext_exists(const std::vector<mess_image_type> &image_types, std::wstring_view ext);

//============================================================
//  PICKER/TABVIEW CALLBACKS
//============================================================

// picker
static const PickerCallbacks s_softwarePickerCallbacks =
{
	SetSWSortColumn,                    // pfnSetSortColumn
	GetSWSortColumn,                    // pfnGetSortColumn
	SetSWSortReverse,                   // pfnSetSortReverse
	GetSWSortReverse,                   // pfnGetSortReverse
	nullptr,                            // pfnSetViewMode
	GetViewMode,                        // pfnGetViewMode
	SetSWColumnWidths,                  // pfnSetColumnWidths
	GetSWColumnWidths,                  // pfnGetColumnWidths
	SetSWColumnOrder,                   // pfnSetColumnOrder
	GetSWColumnOrder,                   // pfnGetColumnOrder
	SetSWColumnShown,                   // pfnSetColumnShown
	GetSWColumnShown,                   // pfnGetColumnShown
	nullptr,                            // pfnGetOffsetChildren

	nullptr,                            // pfnCompare
	MamePlayGame,                       // pfnDoubleClick
	SoftwarePicker_GetItemString,       // pfnGetItemString
	SoftwarePicker_GetItemImage,        // pfnGetItemImage
	nullptr,                            // pfnLeavingItem - unused
	SoftwarePicker_EnteringItem,        // pfnEnteringItem
	nullptr,                            // pfnBeginListViewDrag
	nullptr,                            // pfnFindItemParent
	SoftwarePicker_Idle,                // pfnIdle
	SoftwarePicker_OnHeaderContextMenu, // pfnOnHeaderContextMenu
	nullptr                             // pfnOnBodyContextMenu
};

// swlist
static const PickerCallbacks s_softwareListCallbacks =
{
	SetSLSortColumn,                    // pfnSetSortColumn
	GetSLSortColumn,                    // pfnGetSortColumn
	SetSLSortReverse,                   // pfnSetSortReverse
	GetSLSortReverse,                   // pfnGetSortReverse
	nullptr,                            // pfnSetViewMode
	GetViewMode,                        // pfnGetViewMode
	SetSLColumnWidths,                  // pfnSetColumnWidths
	GetSLColumnWidths,                  // pfnGetColumnWidths
	SetSLColumnOrder,                   // pfnSetColumnOrder
	GetSLColumnOrder,                   // pfnGetColumnOrder
	SetSLColumnShown,                   // pfnSetColumnShown
	GetSLColumnShown,                   // pfnGetColumnShown
	nullptr,                            // pfnGetOffsetChildren

	nullptr,                            // pfnCompare
	MamePlayGame,                       // pfnDoubleClick
	SoftwareList_GetItemString,         // pfnGetItemString
	nullptr,                            // pfnGetItemImage
	SoftwareList_LeavingItem,           // pfnLeavingItem
	SoftwareList_EnteringItem,          // pfnEnteringItem
	nullptr,                            // pfnBeginListViewDrag
	nullptr,                            // pfnFindItemParent
	SoftwareList_Idle,                  // pfnIdle
	SoftwareList_OnHeaderContextMenu,   // pfnOnHeaderContextMenu
	nullptr                             // pfnOnBodyContextMenu
};


static const TabViewCallbacks s_softwareTabViewCallbacks =
{
	nullptr,                            // pfnGetShowTabCtrl
	SetCurrentSoftwareTab,              // pfnSetCurrentTab
	GetCurrentSoftwareTab,              // pfnGetCurrentTab
	nullptr,                            // pfnSetShowTab
	nullptr,                            // pfnGetShowTab

	SoftwareTabView_GetTabShortName,    // pfnGetTabShortName
	SoftwareTabView_GetTabLongName,     // pfnGetTabLongName

	SoftwareTabView_OnSelectionChanged, // pfnOnSelectionChanged
	SoftwareTabView_OnMoveSize          // pfnOnMoveSize
};



//============================================================
//  IMPLEMENTATION
//============================================================

static const device_entry *lookupdevice(std::string device_type)
{
	if (!device_type.empty())
		for (size_t i = 0; i < std::size(s_devices); i++)
			if (s_devices[i].dev_type == device_type)
				return &s_devices[i];

	return &s_devices[0];
}


static MViewInfo *GetMViewInfo(HWND hwndMView)
{
	LONG_PTR l = windows::get_window_long_ptr(hwndMView, GWLP_USERDATA);
	return (MViewInfo *) l;
}


static void MView_SetCallbacks(HWND hwndMView, const MViewCallbacks *pCallbacks)
{
	MViewInfo *pMViewInfo;
	pMViewInfo = GetMViewInfo(hwndMView);
	pMViewInfo->pCallbacks = pCallbacks;
}


void InitMessPicker(void)
{
	HWND hwndSoftware;
	PickerOptions opts;

	s_config = nullptr;
	hwndSoftware = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWLIST);

	std::cout << "InitMessPicker: A" << "\n";
	opts = {};  // zero initialize
	opts.pCallbacks = &s_softwarePickerCallbacks;
	opts.nColumnCount = SW_COLUMN_COUNT; // number of columns in picker
	opts.column_names = mess_column_names; // get picker column names
	std::cout << "InitMessPicker: B" << "\n";
	SetupSoftwarePicker(hwndSoftware, &opts); // display them

	std::cout << "InitMessPicker: C" << "\n";
	(void)windows::set_window_long_ptr(hwndSoftware, GWL_STYLE, windows::get_window_long_ptr(hwndSoftware, GWL_STYLE) | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDRAWFIXED);

	std::cout << "InitMessPicker: D" << "\n";
	SetupSoftwareTabView();

	{
		static const MViewCallbacks s_MViewCallbacks =
		{
			MView_GetOpenFileName,
			MView_GetCreateFileName,
			MView_SetSelectedSoftware,
			MView_GetSelectedSoftware,
			MView_GetOpenItemName,
			MView_Unmount
		};
		MView_SetCallbacks(dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWDEVVIEW), &s_MViewCallbacks);
	}

	HWND hwndSoftwareList = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SOFTLIST);

	std::cout << "InitMessPicker: H" << "\n";
	opts = {};
	opts.pCallbacks = &s_softwareListCallbacks;
	opts.nColumnCount = SL_COLUMN_COUNT; // number of columns in sw-list
	opts.column_names = softlist_column_names; // columns for sw-list
	std::cout << "InitMessPicker: I" << "\n";
	SetupSoftwareList(hwndSoftwareList, &opts); // show them

	std::cout << "InitMessPicker: J" << "\n";
	(void)windows::set_window_long_ptr(hwndSoftwareList, GWL_STYLE, windows::get_window_long_ptr(hwndSoftwareList, GWL_STYLE) | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDRAWFIXED);
	std::cout << "InitMessPicker: Finished" << "\n";

	bool bShowSoftware = is_flag_set(GetWindowPanes(), window_pane::SOFTWARE_PANE);
	int swtab = GetCurrentSoftwareTab();
	if (!bShowSoftware)
		swtab = -1;
	 windows::show_window(dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWLIST), (swtab == 0) ? SW_SHOW : SW_HIDE);
	 windows::show_window(dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWDEVVIEW), (swtab == 1) ? SW_SHOW : SW_HIDE);
	 windows::show_window(dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SOFTLIST), (swtab == 2) ? SW_SHOW : SW_HIDE);
	 windows::show_window(dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWTAB), bShowSoftware ? SW_SHOW : SW_HIDE);
	menus::check_menu_item(menus::get_menu(GetMainWindow()), ID_VIEW_SOFTWARE_AREA, bShowSoftware ? MF_CHECKED : MF_UNCHECKED);
}


bool CreateMessIcons(void)
{
	bool bResult = false;
	// create the icon index, if we haven't already
	if (!mess_icon_index)
	{
		size_t icon_index_size = driver_list::total() * std::size(s_devices);
		mess_icon_index.reset(new int[icon_index_size]());
	}
	// Associate the image lists with the list view control.
	HWND hwndSoftwareList = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWLIST);
	if (hwndSoftwareList)
	{
		HIMAGELIST hSmall = GetSmallImageList();
		HIMAGELIST hLarge = GetLargeImageList();
		if (hSmall && hLarge)
		{
			(void)list_view::set_image_list(hwndSoftwareList, hSmall, LVSIL_SMALL);
			(void)list_view::set_image_list(hwndSoftwareList, hLarge, LVSIL_NORMAL);
			bResult = true;
		}
	}

	return bResult;
}


static int GetMessIcon(int driver_index, std::string software_type)
{
	const game_driver* driver;
	HICON hIcon;
	int device_index, icon_index, icon_pos;

	if (software_type.empty())
		return 0;

	icon_index = -1;
	icon_pos = -1;
	hIcon = 0;
	driver = 0;

	device_index = lookupdevice(software_type)->dline;
	icon_index = (driver_index * std::size(s_devices)) + device_index;

	icon_pos = mess_icon_index[icon_index];
	if (icon_pos >= 0)
	{
		int clone_index = 0;
		std::string file_path;
		std::string icon_name;

		driver = &driver_list::driver(driver_index);
		icon_name = std::move(software_type);
		while (driver)
		{
			file_path = std::string(driver->name) + "/"s + icon_name;
			hIcon = LoadIconFromFile(file_path);

			if (hIcon)
			{
				icon_pos = image_list::add_icon(GetSmallImageList(), hIcon);
				image_list::add_icon(GetLargeImageList(), hIcon);
				if (icon_pos != -1)
					mess_icon_index[icon_index] = icon_pos;

				return icon_pos;
			}

			clone_index = driver_list::clone(*driver);
			if (clone_index != -1)
				driver = &driver_list::driver(clone_index);
			else
				driver = 0;
		}

	}

	return icon_pos;
}


// Rules: if driver's path valid and not same as global, that's ok. Otherwise try parent, then compat.
// If still no good, use global if it was valid. Lastly, default to emu folder.
// Some callers regard the defaults as invalid so prepend a character to indicate validity
static std::pair<std::error_condition, std::string> ProcessSWDir(int drvindex)
{
	std::pair<std::error_condition, std::string> result;

	if (drvindex < 0 || drvindex >= driver_list::total())
	{
		std::cerr << __FILE__ << ": " << __LINE__ << ": Invalid driver index (" << drvindex << ") in " << __FUNCTION__ << "\n";
		result.first = std::errc::invalid_argument;
		return result;
	}

	bool map_path_exist = false;

	// Get the global software path
	std::string dirmap_path = emu_opts.dir_get_value(DIRPATH_SWPATH);
	if (dirmap_path.empty())
	{
		std::cerr << __FILE__ << ": " << __LINE__ << ": No value for directory map index (" << DIRPATH_SWPATH << ") in " << __FUNCTION__ << "\n";
		result.first = std::errc::no_such_file_or_directory;
		return result;
	}

	std::string::size_type find_pos = dirmap_path.find(';');
	if (find_pos != std::string::npos)
		dirmap_path.resize(find_pos);

	if (exists(dirmap_path))  // make sure its valid
		map_path_exist = true;

	// Get the system's software path
	windows_options o;
	emu_opts.load_options(o, SOFTWARETYPE_GAME, drvindex, false);

	std::string software_path;
	const char* option_value = o.value(OPTION_SWPATH);
	if (option_value && *option_value != '\0')
	{
		software_path = option_value;

		find_pos = software_path.find(';');
		if (find_pos != std::string::npos)
			software_path.resize(find_pos);

		if (exists(software_path))  // make sure its valid
		{
			if (map_path_exist && dirmap_path != software_path)
			{

				std::cout << __FUNCTION__ << ": Using " << driver_list::driver(drvindex).type.fullname() << "'s software path = (" << software_path << ")" << "\n";
				result.first = std::errc::is_a_directory; // show loose software
				result.second = std::move(software_path);
				return result;
			}
		}
	}

	// Try the parent, if the system it has one
	int nParentIndex = driver_list::non_bios_clone(drvindex);
	if (nParentIndex >= 0 && nParentIndex < driver_list::total())
	{
		emu_opts.load_options(o, SOFTWARETYPE_PARENT, drvindex, false);

		option_value = o.value(OPTION_SWPATH);
		if (option_value && *option_value != '\0')
		{
			software_path = option_value;

			find_pos = software_path.find(';');
			if (find_pos != std::string::npos)
				software_path.resize(find_pos);

			if (exists(software_path))  // make sure its valid
			{
				if (map_path_exist && dirmap_path != software_path)
				{
					std::cout << __FUNCTION__ << ": Using parent, " << driver_list::driver(nParentIndex).type.fullname() << ", with software path = (" << software_path << ")" << "\n";
					result.first = std::errc::is_a_directory;
					result.second = std::move(software_path);
					return result;
				}
			}
		}
	}

	// Try compat if it has one

	int nCloneIndex = driver_list::compatible_with(drvindex); // fills in the compatible cache
	if (nCloneIndex >= 0 && nCloneIndex < driver_list::total())
	{
		emu_opts.load_options(o, SOFTWARETYPE_COMPAT, drvindex, false);

		option_value = o.value(OPTION_SWPATH);
		if (option_value && *option_value != '\0')
		{
			software_path = option_value;

			find_pos = software_path.find(';');
			if (find_pos != std::string::npos)
				software_path.resize(find_pos);

			if (exists(software_path))  // make sure its valid
			{
				if (map_path_exist && dirmap_path != software_path)
				{
					std::cout << __FUNCTION__ << ": Using compatible system, " << driver_list::driver(nCloneIndex).type.fullname() << ", with software path = (" << software_path << ")" << "\n";
					result.first = std::errc::is_a_directory;
					result.second = std::move(software_path);
					return result;
				}
			}
		}
	}

	result.first = std::errc::no_such_file_or_directory; // show no loose software
	// Fallback to the global software path, if it was valid
	if (map_path_exist)
	{
		std::cout << __FUNCTION__ << ": Using global software path = " << dirmap_path << "\n";
		result.second = std::move(dirmap_path);
		return result;
	}

	// If all else fails, return the working dir
	result.second = emu_opts.get_exe_path_utf8();
	std::cout << __FUNCTION__ << ": Using working directory = " << result.second << "\n";

	return result;
}


// Split multi-directory for SW Files into separate directories, and ask the picker to add the files from each.
// pszSubDir path not used by any caller.
static bool SoftwarePicker_AddDirs(HWND hwndPicker, const std::wstring_view directories, const std::wstring_view subdirectory)
{
	if (directories.empty())
		return false;

	bool result = false, subdir_empty = subdirectory.empty();
	path dir_path;
	std::wstring add_path;

	wstringtokenizer tokenizer(directories, L";");
	for (const auto &token : tokenizer)
	{
		dir_path = token;
		if (!subdir_empty)
			dir_path /= subdirectory;

		add_path = dir_path.wstring();

		result = SoftwarePicker_AddDirectory(hwndPicker, add_path);
		if (!result)
			break;
	}

	return result;
}


void MySoftwareListClose(void)
{
	// free the machine config, if necessary
	if (s_config)
	{
		software_config_free(s_config);
		s_config = nullptr;
	}
}


void MView_Clear(HWND hwndMView)
{
	if (!hwndMView)
		return;
	MViewInfo *pMViewInfo;
	pMViewInfo = GetMViewInfo(hwndMView);

	if (pMViewInfo->pEntries)
	{
		for (size_t i = 0; pMViewInfo->pEntries[i].dev; i++)
		{
			windows::destroy_window(pMViewInfo->pEntries[i].hwndStatic);
			windows::destroy_window(pMViewInfo->pEntries[i].hwndEdit);
			windows::destroy_window(pMViewInfo->pEntries[i].hwndBrowseButton);
		}
		delete[] pMViewInfo->pEntries;
		pMViewInfo->pEntries = nullptr;
	}

	pMViewInfo->config = nullptr;
	MVstate = 0;
	pMViewInfo->slots = 0;
}


static void MView_GetColumns(HWND hwndMView, int *pnStaticPos, int *pnStaticWidth, int *pnEditPos, int *pnEditWidth, int *pnButtonPos, int *pnButtonWidth)
{
	MViewInfo *pMViewInfo;

	pMViewInfo = GetMViewInfo(hwndMView);

	RECT r;
	(void)windows::get_client_rect(hwndMView, &r);

	*pnStaticPos = MVIEW_PADDING;
	*pnStaticWidth = pMViewInfo->nWidth;

	*pnButtonWidth = 25;
	*pnButtonPos = (r.right - r.left) - *pnButtonWidth - MVIEW_PADDING;

//  *pnEditPos = *pnStaticPos + *pnStaticWidth + MVIEW_PADDING;
	*pnEditPos = MVIEW_PADDING;
	*pnEditWidth = *pnButtonPos - *pnEditPos - MVIEW_PADDING;
	if (*pnEditWidth < 0)
		*pnEditWidth = 0;
}


static void MView_TextChanged(HWND hwndMView, int nChangedEntry, wchar_t *pszFilename)
{
	MViewInfo *pMViewInfo;
	pMViewInfo = GetMViewInfo(hwndMView);
	if (!pMViewInfo->bSurpressFilenameChanged && pMViewInfo->pCallbacks->pfnSetSelectedSoftware)
		pMViewInfo->pCallbacks->pfnSetSelectedSoftware(pMViewInfo->config->driver_index,
			pMViewInfo->pEntries[nChangedEntry].dev, pszFilename);
}


static LRESULT CALLBACK MView_EditWndProc(HWND hwndEdit, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	LONG_PTR lp = windows::get_window_long_ptr(hwndEdit, GWLP_USERDATA);
	MViewEntry *pEnt = reinterpret_cast<MViewEntry*>(lp);

	if (nMessage == WM_SETTEXT)
	{
		HWND hwndMView = windows::get_parent(hwndEdit);
		MViewInfo *pMViewInfo;
		pMViewInfo = GetMViewInfo(hwndMView);
		MView_TextChanged(hwndMView, pEnt - pMViewInfo->pEntries, (wchar_t *) lParam);
	}
	return windows::call_window_proc(pEnt->pfnEditWndProc, hwndEdit, nMessage, wParam, lParam);
}


void MView_Refresh(HWND hwndMView)
{
	if (!MVstate)
		return;

	MViewInfo *pMViewInfo;
	std::wstring pszSelection;

	pMViewInfo = GetMViewInfo(hwndMView);
	std::cout << "MView_Refresh: " << driver_list::driver(pMViewInfo->config->driver_index).name << "\n";

	if (pMViewInfo->slots)
	{
		for (size_t i = 0; i < pMViewInfo->slots; i++)
		{
			pszSelection = pMViewInfo->pCallbacks->pfnGetSelectedSoftware(pMViewInfo->config->driver_index,
				pMViewInfo->pEntries[i].dev);
			std::cout << "MView_Refresh: Finished GetSelectSoftware" << "\n";
			if (!&pszSelection[0])
				pszSelection = L"";

			pMViewInfo->bSurpressFilenameChanged = true;
			windows::set_window_text(pMViewInfo->pEntries[i].hwndEdit, pszSelection.c_str());
			pMViewInfo->bSurpressFilenameChanged = false;
		}
	}
}


static BOOL MView_SetDriver(HWND hwndMView, const software_config *sconfig)
{
	if (!hwndMView || !sconfig)
		return false;

	MViewInfo *pMViewInfo = GetMViewInfo(hwndMView);
	if (!pMViewInfo)
		return false;

	int y = 0,
		nHeight = 0,
		nStaticPos = 0,
		nStaticWidth = 0,
		nEditPos = 0,
		nEditWidth = 0,
		nButtonPos = 0,
		nButtonWidth = 0;

	// clear out
	std::cout << "MView_SetDriver: A" << "\n";
	MView_Clear(hwndMView);

	// copy the config
	std::cout << "MView_SetDriver: B" << "\n";
	pMViewInfo->config = sconfig;
	if (!sconfig || !has_software)
		return false;

	// count total amount of devices
	std::cout << "MView_SetDriver: C" << "\n";
	for (device_image_interface &dev : image_interface_enumerator(pMViewInfo->config->mconfig->root_device()))
	{
		if (dev.user_loadable())
			pMViewInfo->slots++;
	}
	std::cout << "MView_SetDriver: Number of slots = " << pMViewInfo->slots << "\n";

	if (pMViewInfo->slots)
	{
		// get the names of all of the media-slots so we can then work out how much space is needed to display them
		std::string instance_name;
		std::vector<std::wstring> ppszDevices(pMViewInfo->slots);
		for (device_image_interface &dev : image_interface_enumerator(pMViewInfo->config->mconfig->root_device()))
		{
			if (!dev.user_loadable())
				continue;
			instance_name = dev.instance_name() + " ("s + dev.brief_instance_name() + ")"s;
			std::wstring utf16_instance_name = mui_utf16_from_utf8string(instance_name);
			ppszDevices.push_back(std::move(utf16_instance_name));
		}
		std::cout << "MView_SetDriver: Number of media slots = " << ppszDevices.size() << "\n";

		// Calculate the requisite size for the device column
		pMViewInfo->nWidth = 0;
		HDC hDc = gdi::get_dc(hwndMView);
		SIZE sz;
		for (const std::wstring &device_name : ppszDevices)
		{
			gdi::get_text_extent_point_32(hDc, device_name.c_str(), mui_wcslen(device_name.c_str()), &sz);
			if (sz.cx > pMViewInfo->nWidth)
				pMViewInfo->nWidth = sz.cx;
		}
		(void)gdi::release_dc(hwndMView, hDc);

		MViewEntry *pEnt;
		pEnt = new MViewEntry[pMViewInfo->slots + 1] {};
		if (!pEnt)
			return false;

		pMViewInfo->pEntries = pEnt;

		y = MVIEW_PADDING;
		nHeight = MVIEW_SPACING;
		MView_GetColumns(hwndMView, &nStaticPos, &nStaticWidth, &nEditPos, &nEditWidth, &nButtonPos, &nButtonWidth);

		// Now actually display the media-slot names, and show the empty boxes and the browse button
		LONG_PTR l = 0;
		for (device_image_interface &dev : image_interface_enumerator(pMViewInfo->config->mconfig->root_device()))
		{
			if (!dev.user_loadable())
				continue;
			pEnt->dev = &dev;
			instance_name = dev.instance_name() + std::string(" (") + dev.brief_instance_name() + std::string(")"); // get name of the slot (long and short)
			std::transform(instance_name.begin(), instance_name.begin()+1, instance_name.begin(), ::toupper); // turn first char to uppercase
			pEnt->hwndStatic = windows::create_window_ex_utf8(0, "STATIC", instance_name.c_str(), // display it
				WS_VISIBLE | WS_CHILD, nStaticPos, y, nStaticWidth, nHeight, hwndMView, nullptr, nullptr, nullptr);
			y += nHeight;

			pEnt->hwndEdit =windows::create_window_ex_utf8(0, "EDIT", "",
				WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, nEditPos,
				y, nEditWidth, nHeight, hwndMView, nullptr, nullptr, nullptr); // display blank edit box

			pEnt->hwndBrowseButton =windows::create_window_ex_utf8(0, "BUTTON", "...",
				WS_VISIBLE | WS_CHILD, nButtonPos, y, nButtonWidth, nHeight, hwndMView, nullptr, nullptr, nullptr); // display browse button

			if (pEnt->hwndStatic)
				(void) windows::send_message(pEnt->hwndStatic, WM_SETFONT, (WPARAM) pMViewInfo->hFont, true);

			if (pEnt->hwndEdit)
			{
				(void) windows::send_message(pEnt->hwndEdit, WM_SETFONT, (WPARAM) pMViewInfo->hFont, true);
				l = (LONG_PTR) MView_EditWndProc;
				l = windows::set_window_long_ptr(pEnt->hwndEdit, GWLP_WNDPROC, l);
				pEnt->pfnEditWndProc = (WNDPROC) l;
				(void)windows::set_window_long_ptr(pEnt->hwndEdit, GWLP_USERDATA, (LONG_PTR) pEnt);
			}

			if (pEnt->hwndBrowseButton)
				(void)windows::set_window_long_ptr(pEnt->hwndBrowseButton, GWLP_USERDATA, (LONG_PTR) pEnt);

			y += nHeight;
			y += nHeight;

			pEnt++;
		}
	}

	if (!pMViewInfo->slots)
		return false;

	MVstate = 1;
	std::cout << "MView_SetDriver: Calling MView_Refresh" << "\n";
	MView_Refresh(hwndMView); // show names of already-loaded software
	std::cout << "MView_SetDriver: Finished" << "\n";
	return true;
}


bool MyFillSoftwareList(int drvindex, bool bForce)
{
	HWND hwndSoftwareList, hwndSoftwareMView, hwndSoftwarePicker;
	std::string sw_paths;

	// do we have to do anything?
	if (!bForce)
	{
		bool is_same = false;

		if (s_config)
			is_same = (drvindex == s_config->driver_index);
		else
			is_same = (drvindex < 0);

		if (is_same)
			return has_software;
	}

	mvmap.clear();
	slmap.clear();
	has_software = false;

	// free the machine config, if necessary
	MySoftwareListClose();

	// allocate the machine config, if necessary
	if (drvindex >= 0)
	{
		s_config = software_config_alloc(drvindex);
		has_software = emu_opts.DriverHasSoftware(drvindex);
	}

	// locate key widgets
	hwndSoftwarePicker = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWLIST);
	hwndSoftwareList = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SOFTLIST);
	hwndSoftwareMView = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWDEVVIEW);

	std::cout << __FUNCTION__ << ": Calling SoftwareList_Clear" << "\n";
	SoftwareList_Clear(hwndSoftwareList);
	std::cout << __FUNCTION__ << ": Calling SoftwarePicker_Clear" << "\n";
	SoftwarePicker_Clear(hwndSoftwarePicker);

	// set up the device view
	std::cout << __FUNCTION__ << ": Calling MView_SetDriver" << "\n";
	MView_SetDriver(hwndSoftwareMView, s_config);

	// set up the software picker
	std::cout << __FUNCTION__ << ": Calling SoftwarePicker_SetDriver" << "\n";
	SoftwarePicker_SetDriver(hwndSoftwarePicker, s_config);

	if (!s_config || !has_software)
		return has_software;

	// set up the Software Files by using swpath (can handle multiple paths)
	std::cout << __FUNCTION__ << ": Processing software directories" << "\n";

	auto proc_sw = ProcessSWDir(drvindex);
	if (proc_sw.first == std::errc::invalid_argument)
		return false; // SWDir not valid

	sw_paths = proc_sw.second;

	std::cout << __FUNCTION__ << ": Finished processing, directory = (" << sw_paths << ")" << "\n";

	std::wstring utf16_sw_paths = mui_utf16_from_utf8string(sw_paths);
	std::cout << __FUNCTION__ << ": Calling AddSoftwarePickerDirs" << "\n";
	SoftwarePicker_AddDirs(hwndSoftwarePicker, utf16_sw_paths);

	// set up the Software List
	std::cout << "MyFillSoftwareList: Calling SoftwarePicker_SetDriver" << "\n";
	SoftwareList_SetDriver(hwndSoftwareList, s_config);

	std::cout << "MyFillSoftwareList: Getting Softlist Information" << "\n";
	for (software_list_device &swlistdev : software_list_device_enumerator(s_config->mconfig->root_device()))
	{
		if (swlistdev.is_compatible() || swlistdev.is_original())
		{
			for (const software_info &swinfo : swlistdev.get_info())
			{
				const software_part &swpart = swinfo.parts().front();

				// search for a device with the right interface
				for (const device_image_interface &image : image_interface_enumerator(s_config->mconfig->root_device()))
				{
					if (!image.user_loadable())
						continue;
					if (swlistdev.is_compatible(swpart) == SOFTWARE_IS_COMPATIBLE)
					{
						const char *interface = image.image_interface();
						if (interface)
						{
							if (swpart.matches_interface(interface))
							{
								// Extract the Usage data from the "info" fields.
								std::string usage;
								for (const software_info_item &flist : swinfo.info())
									if (flist.name() == "usage")
										usage = flist.value();
								// Now actually add the item
								SoftwareList_AddFile(hwndSoftwareList, swinfo.shortname(), swlistdev.list_name(), swinfo.longname(),
									swinfo.publisher(), swinfo.year(), usage, image.brief_instance_name());
								break;
							}
						}
					}
				}
			}
		}
	}
	std::cout << "MyFillSoftwareList: Finished" << "\n";
	return has_software;
}


void MessUpdateSoftwareList(void)
{
	HWND hwndList = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_LIST);
	MyFillSoftwareList(Picker_GetSelectedItem(hwndList), true);
}


// Places the specified image in the specified slot - MUST be a valid filename, not blank
static void MessSpecifyImage(int drvindex, device_image_interface *const device_image, const std::string &file_name)
{
	if (drvindex < 0 || drvindex >= driver_list::total() || file_name.empty())
		return;

	if (device_image)
	{
		emu_opts.SetSelectedSoftware(drvindex, device_image->instance_name(), file_name);
		if (LOG_SOFTWARE)
			std::cout << __FUNCTION__ << ": instance_name = '" << device_image->instance_name() << "', file_name ='" << file_name << "'" << "\n";

		return;
	}

	std::string file_extension, option_name;

	// identify the file extension
	std::string::size_type dot_pos = file_name.rfind('.');
	if (dot_pos != std::string::npos)
		file_extension = file_name.substr(dot_pos + 1, file_name.size());

	if (file_extension.empty())
	{
		if (LOG_SOFTWARE)
			std::cout << __FUNCTION__ << ": No file extension found in '" << file_name << "'" << "\n";
		return;
	}

	if (LOG_SOFTWARE)
		std::cout << __FUNCTION__ << ": file_extension = (" << file_extension << ")" << "\n";

	for (device_image_interface& current_image : image_interface_enumerator(s_config->mconfig->root_device()))
	{
		if (!current_image.user_loadable())
			continue;

		if (uses_file_extension(current_image, file_extension))
		{
			std::string option_name = current_image.instance_name();
			if (option_name.empty())
			{
				// could not place the image
				if (LOG_SOFTWARE)
					std::cout << __FUNCTION__ << ": Failed to place image '" << file_name << "'" << "\n";
			}
			else
			{
				// place the image
				emu_opts.SetSelectedSoftware(drvindex, option_name, file_name);
				if (LOG_SOFTWARE)
					std::cout << __FUNCTION__ << ": option_name = '" << option_name << "', file_name ='" << file_name << "'" << "\n";
			}
			break;
		}
	}

}


#if 0
// This is pointless because clicking on a new item overwrites the old one anyway
static void MessRemoveImage(int drvindex, const char *pszFilename)
{
	const char *s;
	windows_options o;
	//o.set_value(OPTION_SYSTEMNAME, driver_list::driver(drvindex).name, OPTION_PRIORITY_CMDLINE);
	load_options(o, SOFTWARETYPE_GAME, drvindex, 1);

	for (device_image_interface &dev : image_interface_enumerator(s_config->mconfig->root_device()))
	{
		if (!dev.user_loadable())
			continue;
		// search through all the slots looking for a matching software name and unload it
		std::string opt_name = dev.instance_name();
		s = o.value(opt_name.c_str());
		if (s && !mui_strcmp(pszFilename, s))
			SetSelectedSoftware(drvindex, dev, nullptr);
	}
}
#endif


void MessReadMountedSoftware(int drvindex)
{
	// First read stuff into device view
	if (TabView_GetCurrentTab(dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWTAB))==1)
		MView_Refresh(dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWDEVVIEW));

	// Now read stuff into picker
	if (TabView_GetCurrentTab(dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWTAB))==0)
		MessRefreshPicker();
}


static void MessRefreshPicker(void)
{
	HWND hwndSoftware = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWLIST);
	if (!hwndSoftware || !s_config)
		return;

	s_bIgnoreSoftwarePickerNotifies = true;

	// Now clear everything out; this may call back into us but it should not be problematic
	list_view::set_item_state(hwndSoftware, -1, 0, LVIS_SELECTED);

	// Get the game's options including slots & software
	windows_options o;
	emu_opts.load_options(o, SOFTWARETYPE_GAME, s_config->driver_index, false);
	/* allocate the machine config */
	machine_config config(driver_list::driver(s_config->driver_index), o);

	for (device_image_interface &dev : image_interface_enumerator(config.root_device()))
	{
		if (!dev.user_loadable())
			continue;

		std::string option_name = dev.instance_name(); // get name of device slot
		const char *option_value = o.value(option_name.c_str());// get name of software in the slot

		if (!option_value || *option_value == '\0')
			continue;

		// if software is loaded
		std::cout << __FUNCTION__ << ": option_name = '" << option_name << "', option_value ='" << option_value << "'" << "\n";
		int software_index = SoftwarePicker_LookupIndex(hwndSoftware, option_value); // see if its in the picker
		if (software_index < 0) // not there
		{
			std::wstring utf16_option_value = mui_utf16_from_utf8string(option_value);
			// add already loaded software to picker, but not if its already there
			SoftwarePicker_AddFile(hwndSoftware, utf16_option_value, true);    // this adds the 'extra' loaded software item into the list - we don't need to see this
			software_index = SoftwarePicker_LookupIndex(hwndSoftware, option_value); // refresh pointer
		}
		if (software_index >= 0) // is there
		{
			LVFINDINFOW lvfi;

			lvfi = {};
			lvfi.flags = LVFI_PARAM;
			lvfi.lParam = software_index;
			software_index = list_view::find_item(hwndSoftware, -1, &lvfi);
			list_view::set_item_state(hwndSoftware, software_index, LVIS_SELECTED, LVIS_SELECTED); // highlight it
		}
	}

	s_bIgnoreSoftwarePickerNotifies = false;
}

static std::wstring GetDialogFilter(std::vector<mess_image_type> &image_types)
{
	const wchar_t *all_files(L"All files (*.*)\0*.*\0");
	std::wstring dialog_filter;

	if (image_types.empty())
		return all_files;

	dialog_filter = L"Common image types (";
	for (size_t index = 0, last_pos = image_types.size() - 1; index < image_types.size(); index++)
	{
		mess_image_type &type = image_types[index];
		dialog_filter += L"*." + type.ext;
		if (index < last_pos)
			dialog_filter += L",";
		else
			dialog_filter += L")";
	}

	dialog_filter.append(L"\0", 1);
	for (size_t index = 0, last_pos = image_types.size() - 1; index < image_types.size(); index++)
	{
		mess_image_type &type = image_types[index];
		dialog_filter += L"*." + type.ext;
		if (index < last_pos)
			dialog_filter += L";";
		else
			dialog_filter.append(L"\0", 1);
	}

	dialog_filter.append(all_files, 20);
	for (size_t index = 0; index < image_types.size(); index++)
	{
		mess_image_type &type = image_types[index];
		if (!type.dev)
			dialog_filter += L"Compressed images";
		else
			dialog_filter += type.dlgname;

		dialog_filter += L" (*." + type.ext + std::wstring(L")\0*.", 4) + type.ext + std::wstring(L"\0", 1);
	}

	return dialog_filter;
}

// ------------------------------------------------------------------------
// Open others dialog
// ------------------------------------------------------------------------

static bool CommonFileImageDialog(LPCWSTR initial_directory, common_file_dialog_proc cfd, LPWSTR file_name, std::vector<mess_image_type> &image_types)
{
	bool success;
	OPENFILENAMEW of;
	std::wstring dialog_filter;

	dialog_filter = GetDialogFilter(image_types);

	of.lStructSize = sizeof(of);
	of.hwndOwner = GetMainWindow();
	of.hInstance = nullptr;
	of.lpstrFilter = &dialog_filter[0];
	of.lpstrCustomFilter = nullptr;
	of.nMaxCustFilter = 0;
	of.nFilterIndex = 1;
	of.lpstrFile = file_name;
	of.nMaxFile = MAX_PATH;
	of.lpstrFileTitle = nullptr;
	of.nMaxFileTitle = 0;
	of.lpstrInitialDir = initial_directory;
	of.lpstrTitle = nullptr;
	of.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	of.nFileOffset = 0;
	of.nFileExtension = 0;
	of.lpstrDefExt = L"rom";
	of.lCustData = 0;
	of.lpfnHook = nullptr;
	of.lpTemplateName = nullptr;

	success = cfd(&of);
#if 0
	if (success)
	{
		GetDirectory(filename, last_directory, sizeof(last_directory));
	}
#endif
	return success;
}

static std::vector<mess_image_type> ImageTypesForGenericDevices(device_image_interface *const dev, bool only_creatable)
{
	std::vector<mess_image_type> image_types;

	if (dev == nullptr)
		return image_types;

	const char *brief_type_name = dev->image_brief_type_name();
	if (!brief_type_name || *brief_type_name == '\0')
		return image_types;

	const device_entry *devinfo = lookupdevice(brief_type_name);
	if (!devinfo || devinfo->dlgname.empty())
		return image_types;

	const char* file_exts = dev->file_extensions();
	if (!file_exts || *file_exts == '\0')
	{
		if (!dev->image_interface() || *dev->image_interface() == '\0')
			std::cerr << __FUNCTION__ << ":" << __LINE__
			<< ": Error: Device has no image type name." << "\n";
		else
			std::cerr << __FUNCTION__ << ":" << __LINE__
			<< ": Error: Device '" << dev->image_interface() << "' has no file extensions." << "\n";

		return image_types;
	}
	else
	{
		if (only_creatable && !dev->is_creatable())
			return image_types;

		std::wstring dev_exts = mui_utf16_from_utf8string(file_exts);
		add_image_type(dev, dev_exts, image_types);
	}

	return image_types;
}

static bool images_type_ext_exists(const std::vector<mess_image_type> &image_types, std::wstring_view ext)
{
	for (const auto& img_type : image_types)
	{
		if (img_type.ext == ext)
			return true;
	}
	return false;
}

static void add_image_type(device_image_interface *const dev, std::wstring_view file_extensions, std::vector<mess_image_type> &image_types)
{

	const device_entry* devinfo = lookupdevice(dev->image_brief_type_name());
	if (!devinfo || devinfo->dlgname.empty())
		return;

	if (file_extensions.empty())
		return;

	wstringtokenizer tokenizer(file_extensions, L",");
	for (const auto& ext : tokenizer)
	{
		if (images_type_ext_exists(image_types, ext))
			continue;

		mess_image_type new_img_type{ dev,  ext,devinfo->dlgname };
		image_types.emplace_back(new_img_type);
	}
}

static std::vector<mess_image_type> ImageTypesForFloppyDevices(floppy_image_device *const flop_dev, bool only_creatable)
{
	std::vector<mess_image_type> image_types{};
	if (!flop_dev)
	{
		std::cerr << __FUNCTION__ << ":" << __LINE__
			<< ": Error: No image device provided" << "\n";
		return image_types;
	}

	std::vector<const floppy_image_format_t *> writable_fmts = FID_GetWritableFormats(flop_dev);
	for (const auto &format : writable_fmts)
	{
		const char *file_exts = format->extensions();
		if (!file_exts || *file_exts == '\0')
			continue;

//      std::cout << __FUNCTION__ << ": Format: " << format->name() << ": extensions: " << file_exts << "\n";
		std::wstring fmt_exts = mui_utf16_from_utf8string(file_exts);
		add_image_type(flop_dev, fmt_exts, image_types);
	}

	return image_types;
}

/* Specify std::size(s_devices) for type if you want all types */
static std::vector<mess_image_type> SetupImageTypes(device_image_interface *const dev, bool include_archives = true, bool only_creatable = false)
{
	std::vector<mess_image_type> image_types;

	if (!dev)
		return image_types;

	if (auto *dev_with_formats = dynamic_cast<floppy_image_device *const>(dev))
	{
		image_types = ImageTypesForFloppyDevices(dev_with_formats, only_creatable);
	}
	else
	{
		image_types = ImageTypesForGenericDevices(dev,only_creatable);
	}

	if (include_archives)
	{
		image_types.emplace_back(mess_image_type{ nullptr, L"zip",L"" });
		image_types.emplace_back(mess_image_type{ nullptr, L"7z",L"" });
	}

	return image_types;
}

static void MessSetupDevice(common_file_dialog_proc cfd, device_image_interface *const dev)
{
	if (!cfd || !dev)
		return;


	std::vector<mess_image_type> image_types = SetupImageTypes(dev);
	if (image_types.empty())
		return;

	std::string option_swpath = emu_opts.dir_get_value(DIRPATH_SWPATH);
	if (option_swpath.empty())
		return;

	// We only want the first path; throw out the rest
	std::string::size_type find_pos = option_swpath.find(';');
	if (find_pos != std::string::npos)
		option_swpath = option_swpath.substr(0, find_pos);

	std::wstring software_path = mui_utf16_from_utf8string(option_swpath);
	if (software_path.empty())
		return;

	std::vector<wchar_t> file_name(MUI_MAX_CHAR_COUNT);
	bool bResult = CommonFileImageDialog(software_path.c_str(), cfd, file_name.data(), image_types);
	if (bResult)
	{
		HWND hwndSoftList = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWLIST);
		if (!hwndSoftList)
			return;

		SoftwarePicker_AddFile(hwndSoftList, file_name.data(), false);
	}
}


// This is used to Unmount a file from the Media View.
// Unused fields: hwndMView, config, pszFilename, nFilenameLength
static bool MView_Unmount(device_image_interface *const dev)
{
	if (!dev)
		return false;

	int drvindex = Picker_GetSelectedItem(dialog_boxes::get_dlg_item(GetMainWindow(), IDC_LIST));
	if (drvindex < 0)
		return false;

	emu_opts.SetSelectedSoftware(drvindex, dev->instance_name(), "");
	mvmap[dev->instance_name()] = 0;
	return true;
}


// This is used to Mount an existing software File in the Media View
static bool MView_GetOpenFileName(device_image_interface *const dev, const std::filesystem::path &file_name)
{
	if (!dev || file_name.string().size() >= MUI_MAX_CHAR_COUNT)
		return false;

	HWND hwndList = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_LIST);
	if (!hwndList)
		return false;

	int drvindex = Picker_GetSelectedItem(hwndList);
	if (drvindex < 0 || drvindex >= driver_list::total())
		return false;

	windows_options options;
	emu_opts.load_options(options, SOFTWARETYPE_GAME, drvindex, true);

	std::string option_name = dev->instance_name();
	if (option_name.empty())
	{
		std::cout << __FUNCTION__ << ": used invalid device: " << "\n";
		return false;
	}

	// Get the path to the currently mounted image
	const char* option_value = options.value(option_name);
	std::string initial_directory = (!option_value || *option_value == '\0') ? "" : util::zippath_parent(option_value);

	std::string::size_type find_pos = initial_directory.find(':');
	if (!exists(initial_directory) || find_pos == std::string::npos)
	{
		// no image loaded, use swpath
		auto proc_sw = ProcessSWDir(drvindex);
		if (proc_sw.first == std::errc::invalid_argument)
			return false; // SWDir not valid

		initial_directory = proc_sw.second;
		// We only want the first path; throw out the rest
		find_pos = initial_directory.find(';');
		if (find_pos != std::string::npos)
			initial_directory = initial_directory.substr(0, find_pos);
	}

	std::cout << __FUNCTION__ << ": initial_directory = " << initial_directory << "\n";
	std::wstring utf16_filename = file_name.wstring();

	std::vector<wchar_t> filename_buffer(MUI_MAX_CHAR_COUNT);
	mui_wcscpy(filename_buffer.data(), utf16_filename.c_str());

	std::wstring utf16_initial_directory = mui_utf16_from_utf8string(initial_directory);
	std::vector<mess_image_type> image_types = SetupImageTypes(dev);
	bool bResult = CommonFileImageDialog(utf16_initial_directory.c_str(), dialog_boxes::get_open_filename, filename_buffer.data(), image_types);
	if (bResult)
	{
		utf16_filename = filename_buffer.data();
		std::wcout << L"MView_GetOpenFileName: file path = (" << utf16_filename << L")" << "\n";
		std::string utf8_filename = mui_utf8_from_utf16string(utf16_filename);
		// Notify system
		emu_opts.SetSelectedSoftware(drvindex, option_name, std::move(utf8_filename));
		mvmap[option_name] = 1;
	}

	return bResult;
}


static std::wstring GetSoftwareNameFromPath(const std::filesystem::path &full_path)
{
	std::wstring extension = full_path.extension().wstring();

	// Only accept .zip or .7z
	if (extension != L".zip" && extension != L".7z")
		return L""; // unsupported

	path parent = full_path.parent_path();
	std::wstring stem = full_path.stem().wstring(); // name without extension

	if (!parent.empty())
	{
		std::wstring swlist_name = parent.filename().wstring(); // last directory
		return swlist_name + L":" + stem;
	}

	return stem; // fallback
}


// This is used to Mount a software-list Item in the Media View.
static bool MView_GetOpenItemName(device_image_interface *const dev, const std::filesystem::path &file_name)
{
	if (!dev || file_name.string().size() >= MUI_MAX_CHAR_COUNT)
		return false;

	HWND hwndList = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_LIST);
	if (!hwndList)
		return false;

	int drvindex = Picker_GetSelectedItem(hwndList);
	if (drvindex < 0 || drvindex >= driver_list::total())
		return false;

	std::string option_name = dev->instance_name();
	if (option_name.empty())
	{
		std::cout << __FUNCTION__ << ": used invalid device: " << "\n";
		return false;
	}

	auto it = slmap.find(option_name);
	if (it == slmap.end())
	{
		std::cout << __FUNCTION__ << ": device, " << option_name << ", not found " << "\n";
		return false;
	}

	std::wstring initial_directory = mui_utf16_from_utf8string(it->second);

	// If directory doesn't exist, fall back to current dir
	if (!exists(initial_directory))
		initial_directory = current_path().wstring();

	std::wstring utf16_filename = file_name.wstring();

	std::vector<wchar_t> filename_buffer(MUI_MAX_CHAR_COUNT);
	mui_wcscpy(filename_buffer.data(), utf16_filename.c_str());

	std::vector<mess_image_type> imagetypes = SetupImageTypes(nullptr); // get zip & 7z
	bool bResult = CommonFileImageDialog(initial_directory.c_str(), dialog_boxes::get_open_filename, filename_buffer.data(), imagetypes);
	if (bResult)
	{
		std::wstring software_name = GetSoftwareNameFromPath(filename_buffer.data());
		if (software_name.empty())
		{
			std::wcerr << L"MView_GetOpenItemName: Failed to construct a valid software name, path = (" << std::wstring(filename_buffer.data()) << L")" << "\n";
			return false;
		}

		std::wcout << L"MView_GetOpenItemName: software_name = (" << software_name << L")" << "\n";

		// Notify system
		std::string utf8_software_name = mui_utf8_from_utf16string(software_name);
		emu_opts.SetSelectedSoftware(drvindex, option_name, std::move(utf8_software_name));
		mvmap[option_name] = 1;
	}

	return bResult;
}

// This is used to match a generic device to a file extension.
static bool DII_FindImgFileFormat(device_image_interface* const dev, const std::filesystem::path &file_path)
{
	if (!dev)
	{
		std::cerr << __FUNCTION__ << ":" << __LINE__
			<< ": Error: No image device provided" << "\n";
		return false;
	}

	if (file_path.empty())
	{
		std::cerr << __FUNCTION__ << ":" << __LINE__
			<< ": Error: The provided file path can not be empty" << "\n";
		return false;
	}

	std::string file_ext = file_path.extension().string();
	if (!file_ext.empty() && file_ext[0] == '.')
		file_ext.erase(0, 1);

	if (file_ext.empty())
	{
		std::cerr << __FUNCTION__ << ":" << __LINE__
			<< ": Error: No file extension found in the provided file path: '"
			<< file_path << "'" << "\n";
		return false;
	}

	const char *file_exts = dev->file_extensions();
	if (!file_exts || !*file_exts)
	{
		std::cerr << __FUNCTION__ << ":" << __LINE__
			<< ": Error: Device image interface '" << dev->instance_name()
			<< "' does not support any file extensions" << "\n";
		return false;
	}

	stringtokenizer tokenizer(file_exts, ",");
	for (const auto &ext : tokenizer)
	{
		if (mui_stricmp(ext, file_ext) == 0)
		{
			std::cout << __FUNCTION__ << ": File extension '" << file_ext
				<< "' matches known file extensions for device '"
				<< dev->instance_name() << "'" << "\n";
			return true;
		}
	}

	std::cout << __FUNCTION__ << ": File extension '" << file_ext
		<< "' does not match known file extensions for device '"
		<< dev->instance_name() << "'" << "\n";

	return false;
}

// This is used to Create a new generic image file in the Media View.
static bool MView_DeviceImgIfaceCreateFile(device_image_interface *const dev, const std::filesystem::path &file_path)
{
	if (!dev)
	{
		std::cerr << __FUNCTION__ << ":" << __LINE__
			<< ": Error: No image device provided" << "\n";
		return false;
	}

	if (file_path.empty())
	{
		std::cerr << __FUNCTION__ << ":" << __LINE__
			<< ": Error: The provided file path can not be empty" << "\n";
		return false;
	}

	if (!dev->is_creatable())
	{
		std::cerr << __FUNCTION__ << ": Image device '" << dev->instance_name()
			<< "' is not creatable'" << "\n";
		return false;
	}

	if (!DII_FindImgFileFormat(dev, file_path))
	{
		std::string file_ext = file_path.extension().string();

		std::cout << __FUNCTION__
			<< ": Info: File extension "
			<< (file_ext.empty() ? "for '" + file_path.string() + "'" : "'" + file_ext + "'(" + file_path.string() + ")")
			<< " does not match any known extensions for device image interface '"
			<< dev->instance_name() << "'" << "\n";

		return false;
	}

	std::ofstream file_creation(file_path, std::ios::trunc | std::ios::binary);
	if (!file_creation)
	{
		std::cerr << __FUNCTION__ << ":" << __LINE__
			<< ": Error: Failed to create file '" << file_path << "'" << "\n";
		return false;
	}

	return true;
}

// This is used to match a floppy format to a file extension.
static const floppy_image_format_t* FID_FindImgFileFormat(floppy_image_device *const flop_dev, const std::filesystem::path &file_path)
{
	if (!flop_dev)
	{
		std::cerr << __FUNCTION__ << ":" << __LINE__ << ": Error: No image device provided\n";
		return nullptr;
	}

	if (file_path.empty())
	{
		std::cerr << __FUNCTION__ << ":" << __LINE__ << ": Error: The provided file path is empty\n";
		return nullptr;
	}

	std::string file_ext = file_path.extension().string();
	//if (!file_ext.empty() && file_ext[0] == '.')
	//  file_ext.erase(0, 1);

	if (file_ext.empty())
	{
		std::cerr << __FUNCTION__ << ":" << __LINE__ << ": Error: No file extension found in '" << file_path << "\n";
		return nullptr;
	}

	const auto &writable_fmts = FID_GetWritableFormats(flop_dev); // ideally returns const&
	for (const auto *format : writable_fmts)
	{
		if (format->extension_matches(file_ext.c_str()))
		{
//          std::cout << __FUNCTION__ << ": Info: Matched format '" << format->name() << "' to extension '" << file_ext << "\n";
			return format;
		}
	}

	std::cout << __FUNCTION__ << ": Warning: No matching format for extension '" << file_ext << "\n";
	return nullptr;
}

// This is used to get all the floppy formats that can be created or written to.
static std::vector<const floppy_image_format_t *> FID_GetWritableFormats(floppy_image_device *const flop_dev)
{
	std::vector<const floppy_image_format_t *> writable_formats{};

	if (!flop_dev)
	{
		std::cerr << __FUNCTION__ << ":" << __LINE__
			<< ": Error: No image device provided" << "\n";
		return writable_formats;
	}

	const auto &all_fmts = flop_dev->get_formats();
	const auto &all_fs = flop_dev->get_fs();
	for (const auto &format : all_fmts)
	{
		if (!format || !format->supports_save())
			continue;

		for (const auto &fs : all_fs)
		{
			if (fs.m_manager && !fs.m_manager->can_format())
				continue;

			if (fs.m_type == format)
			{
				writable_formats.emplace_back(format);
			}
			else if (fs.m_key != 0)
			{
				writable_formats.emplace_back(format);
			}
		}
	}

	if (writable_formats.size() == 0)
		std::cout << __FUNCTION__
		<< ": Warning: No writable floppy image formats were found for image device '"
		<< flop_dev->instance_name() << "'" << "\n";

	return writable_formats;
}

// This is used to match a filesystem to a floppy format.
static const floppy_image_device::fs_info *FID_GetFsInfoByFormat(floppy_image_device *const flop_dev, const floppy_image_format_t *flop_fmt)
{
	if (!flop_dev)
	{
		std::cerr << __FUNCTION__ << ":" << __LINE__
			<< ": Error: No image device provided" << "\n";
		return nullptr;
	}

	if (!flop_fmt)
	{
		std::cerr << __FUNCTION__ << ":" << __LINE__
			<< ": Error: No floppy format provided" << "\n";
		return nullptr;
	}

	const floppy_image_device::fs_info *matched_fs = nullptr;

	const auto &all_fs = flop_dev->get_fs();
	for (const auto& fs : all_fs)
	{
		if (fs.m_manager && !fs.m_manager->can_format())
			continue;

		if (fs.m_type && mui_strcmp(fs.m_type->name(), flop_fmt->name()) == 0)
			return &fs;

		if (fs.m_key && !matched_fs)
		{
			uint32_t key_formfactor = 0;
			if (fs.m_key >= 1 && fs.m_key <= 3) // 8" Floppy
				key_formfactor = floppy_image::FF_8;
			else if (fs.m_key >= 4 && fs.m_key <= 14) // 5.25" Floppy
				key_formfactor = floppy_image::FF_525;
			else if (fs.m_key >= 15 && fs.m_key <= 18) // 3.5" Micro Floppy
				key_formfactor = floppy_image::FF_35;
			else if (fs.m_key >= 19 && fs.m_key <= 20) // 3" Compact Floppy
				key_formfactor = floppy_image::FF_3;

			if ((flop_dev->get_form_factor() & key_formfactor) != 0)
				matched_fs = &fs;
		}
	}

	if (!matched_fs)
		std::cerr << __FUNCTION__ << ": Warning: No matching filesystem found\n";
//  else
//      std::cout << __FUNCTION__ << ": Info: Matched filesystem '" << matched_fs->m_name << "'\n";

	return matched_fs;
}

// This is used to Create a new floppy disk image in the Media View.
static bool MView_CreateFloppyDskImgFile(floppy_image_device* const flop_dev, const std::filesystem::path& file_path)
{
	if (!flop_dev)
	{
		std::cerr << __FUNCTION__ << ":" << __LINE__ << ": Error: No image device provided\n";
		return false;
	}

	if (file_path.empty())
	{
		std::cerr << __FUNCTION__ << ":" << __LINE__ << ": Error: The file path cannot be empty\n";
		return false;
	}

	std::cout << __FUNCTION__ << ": Info: floppy image device = "
		<< flop_dev->instance_name() << "(" << flop_dev->image_interface()
		<< "), file_path = '" << file_path << "\n";

	// Find the format and filesystem info
	const auto* create_fmt = FID_FindImgFileFormat(flop_dev, file_path);
	if (!create_fmt)
	{
		std::cerr << __FUNCTION__ << ":" << __LINE__ << ": Error: No valid disk image format for '"
			<< file_path << "\n";
		return false;
	}

	const auto* create_fs = FID_GetFsInfoByFormat(flop_dev, create_fmt);
	if (!create_fs)
	{
		std::cerr << __FUNCTION__ << ":" << __LINE__ << ": Error: No valid disk image filesystem for '"
			<< file_path << "\n";
		return false;
	}

	// Initialize image handler with the disk path
	image_handler ih;
	ih.set_on_disk_path(file_path.string());

	// Initialize metadata in-place
	fs::meta_data meta{};
	meta.set(fs::meta_name::name, file_path.filename().string());

	// Construct floppy_create_info in-place with ternary operator
	floppy_create_info create_info = create_fs->m_type
		? floppy_create_info(create_fs->m_manager, create_fs->m_type, create_fs->m_image_size, create_fs->m_name, create_fs->m_description)
		: floppy_create_info(create_fs->m_name, create_fs->m_key, create_fs->m_description);

	std::cout << __FUNCTION__ << ": Info: "
		<< (create_fs->m_type ? "Creating formatted floppy image: " : "Creating unformatted floppy image: ")
		<< create_fs->m_name << "\n";

	// Create floppy image
	ih.floppy_create(create_info, std::move(meta));

	// Prepare format info and save
	return !ih.floppy_save({ create_fmt, create_fmt->name() }); // true on error
}

// This is used to Create an image in the Media View.
static bool MView_GetCreateFileName(device_image_interface *const dev, const std::filesystem::path &file_name)
{
	if (!dev || file_name.string().size() >= MUI_MAX_CHAR_COUNT)
		return false;

	HWND hwndList = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_LIST);
	if (!hwndList)
		return false;

	int drvindex = Picker_GetSelectedItem(hwndList);
	if (drvindex < 0 || drvindex >= driver_list::total())
		return false;

	std::string option_name = dev->instance_name();
	if (option_name.empty())
	{
		std::cerr << __FUNCTION__ << ":" << __LINE__
			<< ": Error: used invalid device: " << "\n";
		return false;
	}

	auto proc_sw = ProcessSWDir(drvindex);
	if (proc_sw.first == std::errc::invalid_argument)
		return false; // SWDir not valid

	std::string sw_path = proc_sw.second;

	// We only want the first path; throw out the rest
	size_t find_pos = sw_path.find(';');
	if (find_pos != std::string::npos)
		sw_path = sw_path.substr(0, find_pos);

	const std::wstring utf16_function_name = mui_utf16_from_utf8string(__FUNCTION__);

	std::wstring initial_directory = mui_utf16_from_utf8string(sw_path);
	std::wcout << utf16_function_name
		<< L": Info: initial_directory = '"
		<< initial_directory << "'" << "\n";

	std::wstring utf16_filename = file_name.filename().wstring();

	std::vector<wchar_t> filename_buffer(MUI_MAX_CHAR_COUNT);
	mui_wcscpy(filename_buffer.data(), utf16_filename.c_str());

	std::vector<mess_image_type> image_types = SetupImageTypes(dev, false, true);
	bool bResult = CommonFileImageDialog(initial_directory.data(), dialog_boxes::get_save_filename, filename_buffer.data(), image_types);
	if (bResult)
	{
		utf16_filename = filename_buffer.data();
		if (utf16_filename.empty())
		{
			std::wcerr << utf16_function_name << L":" << __LINE__
				<< L": Error: Failed to construct a valid software name, path = ("
				<< utf16_filename << L")" << "\n";
			return false;
		}

		std::wstring software_name = GetSoftwareNameFromPath(filename_buffer.data());
		std::wcout << utf16_function_name
			<< L": Info: software_name = '"
			<< software_name << L"'"
			<< "\n";

		std::string utf8_filename = mui_utf8_from_utf16string(utf16_filename);
		if (auto floppy_dev = dynamic_cast<floppy_image_device*>(dev))
		{
			// Floppy image file creation
			bResult = MView_CreateFloppyDskImgFile(floppy_dev, utf8_filename);
		}
		else
		{
			// Generic image file creation
			bResult = MView_DeviceImgIfaceCreateFile(dev, utf8_filename);
		}

		// Notify system
		emu_opts.SetSelectedSoftware(drvindex, option_name, std::move(utf8_filename));
		mvmap[option_name] = 1;

	}

	return bResult;
}

// Unused fields: hwndMView, config
static void MView_SetSelectedSoftware(int drvindex, device_image_interface *const dev, const std::filesystem::path &file_name)
{
	std::string utf8_filename = file_name.string();

	MessSpecifyImage(drvindex, dev, utf8_filename);
	MessRefreshPicker();
}


// Unused fields: config
static std::wstring MView_GetSelectedSoftware(int drvindex, device_image_interface *const dev)
{
	if (!dev)
	{
		std::cerr << __FUNCTION__ << ":" << __LINE__ << ": Error: No image device provided" << "\n";
		return L"";
	}
	if (drvindex < 0 || drvindex >= driver_list::total())
	{
		std::cerr << __FUNCTION__ << ":" << __LINE__ << ": Error: Game driver index is out of bounds" << "\n";
		return L"";
	}

	if (dev && dev->user_loadable())
	{
		windows_options o;
		emu_opts.load_options(o, SOFTWARETYPE_GAME, drvindex, true);

		std::string opt_name = dev->instance_name();
		if (o.has_image_option(opt_name))
		{
			//std::cout << __FUNCTION__
			//  << ": Info: Option '" << opt_name
			//  << "' found" << "\n";

			std::string opt_value = o.image_option(opt_name).value();
			if (!opt_value.empty())
			{
				std::wstring utf8_to_wcs = mui_utf16_from_utf8string(opt_value);
				if (!utf8_to_wcs.empty())
				{
					mvmap[opt_name] = 1;
					return utf8_to_wcs;
				}
			}
		}
		else
		{
			std::cout << __FUNCTION__
				<< ": Info: Option '" << opt_name
				<< "' not found" << "\n";
			mvmap[opt_name] = 0;
		}
	}

	return L""; // nothing loaded or error occurred
}


// ------------------------------------------------------------------------
// Software Picker Class
// ------------------------------------------------------------------------

static int SoftwarePicker_GetItemImage(HWND hwndPicker, int nItem)
{
	HWND hwndGamePicker = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_LIST);
	HWND hwndSoftwarePicker = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWLIST);
	int drvindex = Picker_GetSelectedItem(hwndGamePicker);
	if (drvindex < 0)
		return -1;
	std::string nType = SoftwarePicker_GetImageType(hwndSoftwarePicker, nItem);
	int nIcon = GetMessIcon(drvindex, nType);
	if (!nIcon)
	{
		if (nType == "unkn")
			nIcon = FindIconIndex(IDI_WIN_REDX);
		else
		{
			//const char *icon_name = lookupdevice(nType)->icon_name;
			INT resource = lookupdevice(std::move(nType))->resource;
			//if (!icon_name)
				//icon_name = nType.c_str();
			nIcon = FindIconIndex(resource);
			if (nIcon < 0)
				nIcon = FindIconIndex(IDI_WIN_UNKNOWN);
		}
	}
	return nIcon;
}


static void SoftwarePicker_EnteringItem(HWND hwndSoftwarePicker, int nItem)
{
	HWND hwndList = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_LIST);

	if (!s_bIgnoreSoftwarePickerNotifies)
	{
		int drvindex;

		drvindex = Picker_GetSelectedItem(hwndList);
		if (drvindex < 0)
		{
			g_szSelectedItem.clear();
			return;
		}

		// Get the fullname and partialname for this file
		if (auto full_name = SoftwarePicker_LookupFilename(hwndSoftwarePicker, nItem))
		{
			std::cout << "SoftwarePicker_EnteringItem: full_name = \"" << *full_name << "\"" << "\n";
			// Do the dirty work
			MessSpecifyImage(drvindex, nullptr, *full_name);

			if (auto part_name = SoftwarePicker_LookupBasename(hwndSoftwarePicker, nItem))
			{
				std::cout << "SoftwarePicker_EnteringItem: part_name = \"" << *part_name << "\"" << "\n";
				part_name->resize(part_name->find('.') - 1);
				// Set up g_szSelectedItem, for the benefit of UpdateScreenShot()
				g_szSelectedItem = *std::move(part_name);
				UpdateScreenShot();
			}
		}
	}
}


static void SoftwareList_LeavingItem(HWND hwndSoftwareList, int nItem)
{
	if (!s_bIgnoreSoftwarePickerNotifies)
		g_szSelectedItem.clear();
}



static void SoftwareList_EnteringItem(HWND hwndSoftwareList, int nItem)
{
	HWND hwndList = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_LIST);

	if (!s_bIgnoreSoftwarePickerNotifies)
	{
		int drvindex = Picker_GetSelectedItem(hwndList);
		if (drvindex < 0)
		{
			g_szSelectedItem.clear();
			return;
		}

		// Get the fullname for this file
		if (auto pszFullName = SoftwareList_LookupFullname(hwndSoftwareList, nItem))// for the screenshot and SetSoftware.
		{
			// For UpdateScreenShot()
			g_szSelectedItem = *pszFullName;
			UpdateScreenShot();
			// use SOFTWARENAME option to properly load a multipart set
			emu_opts.SetSelectedSoftware(drvindex, "", *std::move(pszFullName));
		}
	}
}


// ------------------------------------------------------------------------
// Header context menu stuff
// ------------------------------------------------------------------------

static void MyGetRealColumnOrder(int *order)
{
	int nColumnCount = Picker_GetColumnCount(MyColumnDialogProc_hwndPicker);
	for (size_t i = 0; i < nColumnCount; i++)
		order[i] = Picker_GetRealColumnFromViewColumn(MyColumnDialogProc_hwndPicker, i);
}


static void MyGetColumnInfo(int *order, int *shown)
{
	const PickerCallbacks *pCallbacks;
	pCallbacks = Picker_GetCallbacks(MyColumnDialogProc_hwndPicker);
	pCallbacks->pfnGetColumnOrder(order);
	pCallbacks->pfnGetColumnShown(shown);
}


static void MySetColumnInfo(int *order, int *shown)
{
	const PickerCallbacks *pCallbacks;
	pCallbacks = Picker_GetCallbacks(MyColumnDialogProc_hwndPicker);
	pCallbacks->pfnSetColumnOrder(order);
	pCallbacks->pfnSetColumnShown(shown);
}


static INT_PTR CALLBACK MyColumnDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	int nColumnCount = Picker_GetColumnCount(MyColumnDialogProc_hwndPicker);
	const std::wstring *column_names = Picker_GetColumnNames(MyColumnDialogProc_hwndPicker);

	INT_PTR result = InternalColumnDialogProc(hDlg, Msg, wParam, lParam, nColumnCount,
		MyColumnDialogProc_shown.get(), MyColumnDialogProc_order.get(), column_names,
		MyGetRealColumnOrder, MyGetColumnInfo, MySetColumnInfo);

	return result;
}


static BOOL RunColumnDialog(HWND hwndPicker)
{
	MyColumnDialogProc_hwndPicker = hwndPicker;
	int nColumnCount = Picker_GetColumnCount(MyColumnDialogProc_hwndPicker);

	MyColumnDialogProc_order.reset(new int[nColumnCount]());
	MyColumnDialogProc_shown.reset(new int[nColumnCount]());
	BOOL bResult = dialog_boxes::dialog_box(system_services::get_module_handle(nullptr), menus::make_int_resource(IDD_COLUMNS), GetMainWindow(), MyColumnDialogProc, 0L);
	return bResult;
}


static void SoftwarePicker_OnHeaderContextMenu(POINT pt, int nColumn)
{
	HMENU hMenuLoad = menus::load_menu(nullptr, menus::make_int_resource(IDR_CONTEXT_HEADER));
	HMENU hMenu = GetSubMenu(hMenuLoad, 0);

	int nMenuItem = (int) menus::track_popup_menu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, GetMainWindow(), nullptr);

	(void)DestroyMenu(hMenuLoad);

	HWND hwndPicker = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWLIST);

	switch(nMenuItem)
	{
		case ID_SORT_ASCENDING:
			SetSWSortReverse(false);
			SetSWSortColumn(Picker_GetRealColumnFromViewColumn(hwndPicker, nColumn));
			Picker_Sort(hwndPicker);
			break;

		case ID_SORT_DESCENDING:
			SetSWSortReverse(true);
			SetSWSortColumn(Picker_GetRealColumnFromViewColumn(hwndPicker, nColumn));
			Picker_Sort(hwndPicker);
			break;

		case ID_CUSTOMIZE_FIELDS:
			if (RunColumnDialog(hwndPicker))
				Picker_ResetColumnDisplay(hwndPicker);
			break;
	}
}


static void SoftwareList_OnHeaderContextMenu(POINT pt, int nColumn)
{
	HMENU hMenuLoad = menus::load_menu(nullptr, menus::make_int_resource(IDR_CONTEXT_HEADER));
	HMENU hMenu = GetSubMenu(hMenuLoad, 0);

	int nMenuItem = (int) menus::track_popup_menu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, GetMainWindow(), nullptr);

	(void)DestroyMenu(hMenuLoad);

	HWND hwndPicker = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SOFTLIST);

	switch(nMenuItem)
	{
		case ID_SORT_ASCENDING:
			SetSLSortReverse(false);
			SetSLSortColumn(Picker_GetRealColumnFromViewColumn(hwndPicker, nColumn));
			Picker_Sort(hwndPicker);
			break;

		case ID_SORT_DESCENDING:
			SetSLSortReverse(true);
			SetSLSortColumn(Picker_GetRealColumnFromViewColumn(hwndPicker, nColumn));
			Picker_Sort(hwndPicker);
			break;

		case ID_CUSTOMIZE_FIELDS:
			if (RunColumnDialog(hwndPicker))
				Picker_ResetColumnDisplay(hwndPicker);
			break;
	}
}


// ------------------------------------------------------------------------
// MessCommand
// ------------------------------------------------------------------------

BOOL MessCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
		case ID_MESS_OPEN_SOFTWARE:
			MessSetupDevice(dialog_boxes::get_open_filename, nullptr);
			break;

	}
	return FALSE;
}


// ------------------------------------------------------------------------
// Software Tab View
// ------------------------------------------------------------------------

using SoftwareTabName = struct software_tab_name
{
	std::wstring_view short_name;
	std::wstring_view long_name;
};

static SoftwareTabName soft_tabnames[] =
{
	{L"picker",L"SW Files"},
	{L"MVIEW",L"Media View"},
	{L"softlist", L"SW Items"},
};

static std::wstring_view SoftwareTabView_GetTabShortName(int tab_index)
{
	assert(tab_index >= 0 && tab_index < std::size(soft_tabnames));
	return soft_tabnames[tab_index].short_name;
}


static std::wstring_view SoftwareTabView_GetTabLongName(int tab_index)
{
	assert(tab_index >= 0 && tab_index < std::size(soft_tabnames));
	return soft_tabnames[tab_index].long_name;
}


void SoftwareTabView_OnSelectionChanged(void)
{
	HWND hwndSoftwarePicker = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWLIST);
	HWND hwndSoftwareMView = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWDEVVIEW);
	HWND hwndSoftwareList = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SOFTLIST);

	int nTab = TabView_GetCurrentTab(dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWTAB));

	switch(nTab)
	{
		case 0:
			windows::show_window(hwndSoftwarePicker, SW_SHOW);
			windows::show_window(hwndSoftwareMView, SW_HIDE);
			windows::show_window(hwndSoftwareList, SW_HIDE);
			//MessRefreshPicker(); // crashes MESSUI at start
			break;

		case 1:
			windows::show_window(hwndSoftwarePicker, SW_HIDE);
			windows::show_window(hwndSoftwareMView, SW_SHOW);
			windows::show_window(hwndSoftwareList, SW_HIDE);
			MView_Refresh(hwndSoftwareMView);
			break;
		case 2:
			windows::show_window(hwndSoftwarePicker, SW_HIDE);
			windows::show_window(hwndSoftwareMView, SW_HIDE);
			windows::show_window(hwndSoftwareList, SW_SHOW);
			break;
	}
}


static void SoftwareTabView_OnMoveSize(void)
{
	RECT rMain, rSoftwareTabView, rClient, rTab;

	HWND hwndSoftwareTabView = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWTAB);
	HWND hwndSoftwarePicker = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWLIST);
	HWND hwndSoftwareMView = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWDEVVIEW);
	HWND hwndSoftwareList = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SOFTLIST);

	(void)windows::get_window_rect(hwndSoftwareTabView, &rSoftwareTabView);
	(void)windows::get_client_rect(GetMainWindow(), &rMain);
	(void)gdi::client_to_screen(GetMainWindow(), &((POINT *) &rMain)[0]);
	(void)gdi::client_to_screen(GetMainWindow(), &((POINT *) &rMain)[1]);

	// Calculate rClient from rSoftwareTabView in terms of rMain coordinates
	rClient.left = rSoftwareTabView.left - rMain.left;
	rClient.top = rSoftwareTabView.top - rMain.top;
	rClient.right = rSoftwareTabView.right - rMain.left;
	rClient.bottom = rSoftwareTabView.bottom - rMain.top;

	// If the tabs are visible, then make sure that the tab view's tabs are
	// not being covered up
	if (windows::get_window_long_ptr(hwndSoftwareTabView, GWL_STYLE) & WS_VISIBLE)
	{
		(void)TabCtrl_GetItemRect(hwndSoftwareTabView, 0, &rTab);
		rClient.top += rTab.bottom - rTab.top + 2;
	}

	/* Now actually move the controls */
	windows::move_window(hwndSoftwarePicker, rClient.left, rClient.top, rClient.right - rClient.left, rClient.bottom - rClient.top, true);
	windows::move_window(hwndSoftwareMView, rClient.left + 3, rClient.top + 2, rClient.right - rClient.left - 6, rClient.bottom - rClient.top -4, true);
	windows::move_window(hwndSoftwareList, rClient.left, rClient.top, rClient.right - rClient.left, rClient.bottom - rClient.top, true);
}


static void SetupSoftwareTabView(void)
{
	TabViewOptions opts;

	opts = {};
	opts.pCallbacks = &s_softwareTabViewCallbacks;
	opts.nTabCount = std::size(soft_tabnames);

	SetupTabView(dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWTAB), &opts);
}


static void MView_ButtonClick(HWND hwndMView, MViewEntry *pEnt, HWND hwndButton)
{
	MViewInfo *pMViewInfo;

	bool passed_tests, result, software;
	HMENU hMenu;
	RECT r;
	std::string opt_name;
	wchar_t file_name[MAX_PATH];
	UINT rc;

	passed_tests = false;
	software = false;
	opt_name = pEnt->dev->instance_name();
	pMViewInfo = GetMViewInfo(hwndMView);

	hMenu = CreatePopupMenu();

	for (software_list_device &swlist : software_list_device_enumerator(pMViewInfo->config->mconfig->root_device()))
	{
		for (const software_info &swinfo : swlist.get_info())
		{
			const software_part &part = swinfo.parts().front();
			if (swlist.is_compatible(part) == SOFTWARE_IS_COMPATIBLE)
			{
				for (device_image_interface &image : image_interface_enumerator(pMViewInfo->config->mconfig->root_device()))
				{
					if (!image.user_loadable())
						continue;
					if (!software && (opt_name == image.instance_name()))
					{
						const char *interface = image.image_interface();
						if (interface && part.matches_interface(interface))
						{
							slmap[opt_name] = swlist.list_name();
							software = true;
						}
					}
				}
			}
		}
	}

	if (software)
	{
		std::string dirmap_mediapath, sl_root;

		dirmap_mediapath = emu_opts.dir_get_value(DIRPATH_MEDIAPATH);
		std::istringstream tokenStream(dirmap_mediapath);

		std::cout << "MView_ButtonClick: dirmap_mediapath = " << dirmap_mediapath << "\n";
		while (std::getline(tokenStream, sl_root, ';'))
		{
			DWORD dwAttrib;
			std::string sw_path = sl_root + std::string("\\") + slmap.find(opt_name)->second;
			dwAttrib = storage::get_file_attributes_utf8(sw_path.c_str());
			if ((dwAttrib != INVALID_FILE_ATTRIBUTES) && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
			{
				std::cout << "MView_ButtonClick: sw_path = " << sw_path << "\n";
				slmap[opt_name] = std::move(sw_path);
				passed_tests = true;
				break;
			}
		}
	}

	if (pMViewInfo->pCallbacks->pfnGetOpenFileName)
		menus::append_menu(hMenu, MF_STRING, 1, L"Mount File...");

	if (passed_tests && pMViewInfo->pCallbacks->pfnGetOpenItemName)
		menus::append_menu(hMenu, MF_STRING, 4, L"Mount Item...");

	if (pEnt->dev->is_creatable())
	{
		if (pMViewInfo->pCallbacks->pfnGetCreateFileName)
			menus::append_menu(hMenu, MF_STRING, 2, L"Create...");
	}

	auto find_option = mvmap.find(opt_name);
	if (find_option != mvmap.end() && find_option->second == 1)
		menus::append_menu(hMenu, MF_STRING, 3, L"Unmount");

	*file_name = '\0';
	(void)windows::get_window_rect(hwndButton, &r);

	rc = menus::track_popup_menu(hMenu, TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD, r.left, r.bottom, hwndMView, nullptr);
	switch(rc)
	{
		case 1:
			result = pMViewInfo->pCallbacks->pfnGetOpenFileName(pEnt->dev, file_name);
			break;
		case 2:
			result = pMViewInfo->pCallbacks->pfnGetCreateFileName(pEnt->dev, file_name);
			break;
		case 3:
			result = pMViewInfo->pCallbacks->pfnUnmount(pEnt->dev);
			break;
		case 4:
			result = pMViewInfo->pCallbacks->pfnGetOpenItemName(pEnt->dev, file_name);
			break;
		default:
			result = false;
			break;
	}

	if (result)
	{
		windows::set_window_text(pEnt->hwndEdit, file_name);
		MView_Refresh(dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWDEVVIEW));
	}
}


static BOOL MView_Setup(HWND hwndMView)
{
	MViewInfo *pMViewInfo;

	// allocate the device view info
	pMViewInfo = new MViewInfo();
	if (!pMViewInfo)
		return false;

	(void)windows::set_window_long_ptr(hwndMView, GWLP_USERDATA, (LONG_PTR) pMViewInfo);

	// create and specify the font
	pMViewInfo->hFont = CreateFontW(10, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, L"MS Sans Serif");
	(void)windows::send_message(hwndMView, WM_SETFONT, (WPARAM) pMViewInfo->hFont, false);
	return true;
}


static void MView_Free(HWND hwndMView)
{
	MViewInfo *pMViewInfo;

	MView_Clear(hwndMView);
	pMViewInfo = GetMViewInfo(hwndMView);
	(void)gdi::delete_object(pMViewInfo->hFont);
	delete pMViewInfo;
}


static LRESULT CALLBACK MView_WndProc(HWND hwndMView, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
	MViewInfo *pMViewInfo = GetMViewInfo(hwndMView);

	switch (nMessage)
	{
	case WM_CREATE:
		if (!MView_Setup(hwndMView))
			return -1; // Abort creation if setup failed
		return 0;

	case WM_DESTROY:
		MView_Free(hwndMView);
		return 0;

	case WM_SHOWWINDOW:
		if (wParam && pMViewInfo)
			MView_Refresh(hwndMView);
		return 0;

	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			HWND hwndButton = reinterpret_cast<HWND>(lParam);
			MViewEntry *pEnt = reinterpret_cast<MViewEntry*>(GetWindowLongPtr(hwndButton, GWLP_USERDATA));

			if (pEnt)
				MView_ButtonClick(hwndMView, pEnt, hwndButton);
		}
		return 0;

	case WM_MOVE:
	case WM_SIZE:
		if (pMViewInfo && pMViewInfo->pEntries)
		{
			int nStaticPos = 0, nStaticWidth = 0;
			int nEditPos = 0, nEditWidth = 0;
			int nButtonPos = 0, nButtonWidth = 0;

			MView_GetColumns(
				hwndMView,
				&nStaticPos, &nStaticWidth,
				&nEditPos, &nEditWidth,
				&nButtonPos, &nButtonWidth
			);

			for (auto *pEnt = pMViewInfo->pEntries; pEnt && pEnt->dev; ++pEnt)
			{
				RECT r{};
				if (GetClientRect(pEnt->hwndStatic, &r))
				{
					MapWindowPoints(pEnt->hwndStatic, hwndMView,
						reinterpret_cast<POINT*>(&r), 2);

					// Move edit and browse button controls relative to static label
					constexpr auto redraw = FALSE;
					MoveWindow(pEnt->hwndEdit, nEditPos, r.top + MVIEW_SPACING,
						nEditWidth, r.bottom - r.top, redraw);
					MoveWindow(pEnt->hwndBrowseButton, nButtonPos, r.top + MVIEW_SPACING,
						nButtonWidth, r.bottom - r.top, redraw);
				}
			}
		}
		return 0;

	default:
		return DefWindowProc(hwndMView, nMessage, wParam, lParam);
	}
}


void MView_RegisterClass(void)
{
	WNDCLASSW wc;
	wc = {};
	wc.lpszClassName = L"MessSoftwareMView";
	wc.hInstance = system_services::get_module_handle(nullptr);
	wc.lpfnWndProc = MView_WndProc;
	wc.hbrBackground = gdi::get_sys_color_brush(COLOR_BTNFACE);
	RegisterClassW(&wc);
}

