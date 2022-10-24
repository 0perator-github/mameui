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
#include <filesystem>
#include <iostream>
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
#include "zippath.h"

#include "ui/moptions.h"
#include "winopts.h"

// MAMEUI headers
#include "mui_cstr.h"
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
	const device_image_interface* dev;
	std::wstring ext;
	std::wstring dlgname;
};

using device_entry = struct device_entry
{
	int dline;
	std::string dev_type;
	INT resource;
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
	{ 19, "mout",  IDI_WIN_MIDI,    L"MIDI output" }
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
	bool (*pfnGetOpenFileName)(const device_image_interface *dev, std::wstring_view file_name);
	bool (*pfnGetCreateFileName)(const device_image_interface *dev, std::wstring_view file_name);
	void (*pfnSetSelectedSoftware)(int drvindex, const device_image_interface *dev, std::wstring_view file_name);
	std::wstring (*pfnGetSelectedSoftware)(int drvindex, const device_image_interface *dev);
	bool (*pfnGetOpenItemName)(const device_image_interface *dev, std::wstring_view file_name);
	bool (*pfnUnmount)(const device_image_interface *dev);
};

using MViewEntry = struct mview_entry
{
	const device_image_interface* dev;
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

static void SoftwarePicker_OnHeaderContextMenu(POINT pt, int nColumn);
static int SoftwarePicker_GetItemImage(HWND hwndPicker, int nItem);
static void SoftwarePicker_LeavingItem(HWND hwndSoftwarePicker, int nItem);
static void SoftwarePicker_EnteringItem(HWND hwndSoftwarePicker, int nItem);

static void SoftwareList_OnHeaderContextMenu(POINT pt, int nColumn);
static int SoftwareList_GetItemImage(HWND hwndPicker, int nItem);
static void SoftwareList_LeavingItem(HWND hwndSoftwareList, int nItem);
static void SoftwareList_EnteringItem(HWND hwndSoftwareList, int nItem);

static std::wstring_view SoftwareTabView_GetTabShortName(int tab_index);
static std::wstring_view SoftwareTabView_GetTabLongName(int tab_index);
static void SoftwareTabView_OnMoveSize(void);
static void SetupSoftwareTabView(void);

static void MessRefreshPicker(void);

static bool MView_GetOpenFileName(const device_image_interface *dev, std::wstring_view file_name);
static bool MView_GetOpenItemName(const device_image_interface *dev, std::wstring_view file_name);
static bool MView_GetCreateFileName(const device_image_interface *dev, std::wstring_view file_name);
static bool MView_Unmount(const device_image_interface *dev);
static void MView_SetSelectedSoftware(int drv_index, const device_image_interface *dev, std::wstring_view file_name);
static std::wstring MView_GetSelectedSoftware(int drv_index, const device_image_interface *dev);



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
	nullptr,                               // pfnSetViewMode
	GetViewMode,                        // pfnGetViewMode
	SetSWColumnWidths,                  // pfnSetColumnWidths
	GetSWColumnWidths,                  // pfnGetColumnWidths
	SetSWColumnOrder,                   // pfnSetColumnOrder
	GetSWColumnOrder,                   // pfnGetColumnOrder
	SetSWColumnShown,                   // pfnSetColumnShown
	GetSWColumnShown,                   // pfnGetColumnShown
	nullptr,                               // pfnGetOffsetChildren

	nullptr,                               // pfnCompare
	MamePlayGame,                       // pfnDoubleClick
	SoftwarePicker_GetItemString,       // pfnGetItemString
	SoftwarePicker_GetItemImage,        // pfnGetItemImage
	SoftwarePicker_LeavingItem,         // pfnLeavingItem
	SoftwarePicker_EnteringItem,        // pfnEnteringItem
	nullptr,                               // pfnBeginListViewDrag
	nullptr,                               // pfnFindItemParent
	SoftwarePicker_Idle,                // pfnIdle
	SoftwarePicker_OnHeaderContextMenu, // pfnOnHeaderContextMenu
	nullptr                                // pfnOnBodyContextMenu
};

// swlist
static const PickerCallbacks s_softwareListCallbacks =
{
	SetSLSortColumn,                    // pfnSetSortColumn
	GetSLSortColumn,                    // pfnGetSortColumn
	SetSLSortReverse,                   // pfnSetSortReverse
	GetSLSortReverse,                   // pfnGetSortReverse
	nullptr,                               // pfnSetViewMode
	GetViewMode,                        // pfnGetViewMode
	SetSLColumnWidths,                  // pfnSetColumnWidths
	GetSLColumnWidths,                  // pfnGetColumnWidths
	SetSLColumnOrder,                   // pfnSetColumnOrder
	GetSLColumnOrder,                   // pfnGetColumnOrder
	SetSLColumnShown,                   // pfnSetColumnShown
	GetSLColumnShown,                   // pfnGetColumnShown
	nullptr,                               // pfnGetOffsetChildren

	nullptr,                               // pfnCompare
	MamePlayGame,                       // pfnDoubleClick
	SoftwareList_GetItemString,         // pfnGetItemString
	SoftwareList_GetItemImage,          // pfnGetItemImage
	SoftwareList_LeavingItem,           // pfnLeavingItem
	SoftwareList_EnteringItem,          // pfnEnteringItem
	nullptr,                               // pfnBeginListViewDrag
	nullptr,                               // pfnFindItemParent
	SoftwareList_Idle,                  // pfnIdle
	SoftwareList_OnHeaderContextMenu,   // pfnOnHeaderContextMenu
	nullptr                                // pfnOnBodyContextMenu
};


static const TabViewCallbacks s_softwareTabViewCallbacks =
{
	nullptr,                               // pfnGetShowTabCtrl
	SetCurrentSoftwareTab,              // pfnSetCurrentTab
	GetCurrentSoftwareTab,              // pfnGetCurrentTab
	nullptr,                               // pfnSetShowTab
	nullptr,                               // pfnGetShowTab

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

	std::cout << "InitMessPicker: A" << std::endl;
	opts = {};  // zero initialize
	opts.pCallbacks = &s_softwarePickerCallbacks;
	opts.nColumnCount = SW_COLUMN_COUNT; // number of columns in picker
	opts.column_names = mess_column_names; // get picker column names
	std::cout << "InitMessPicker: B" << std::endl;
	SetupSoftwarePicker(hwndSoftware, &opts); // display them

	std::cout << "InitMessPicker: C" << std::endl;
	(void)windows::set_window_long_ptr(hwndSoftware, GWL_STYLE, windows::get_window_long_ptr(hwndSoftware, GWL_STYLE) | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDRAWFIXED);

	std::cout << "InitMessPicker: D" << std::endl;
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

	std::cout << "InitMessPicker: H" << std::endl;
	opts = {};
	opts.pCallbacks = &s_softwareListCallbacks;
	opts.nColumnCount = SL_COLUMN_COUNT; // number of columns in sw-list
	opts.column_names = softlist_column_names; // columns for sw-list
	std::cout << "InitMessPicker: I" << std::endl;
	SetupSoftwareList(hwndSoftwareList, &opts); // show them

	std::cout << "InitMessPicker: J" << std::endl;
	(void)windows::set_window_long_ptr(hwndSoftwareList, GWL_STYLE, windows::get_window_long_ptr(hwndSoftwareList, GWL_STYLE) | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDRAWFIXED);
	std::cout << "InitMessPicker: Finished" << std::endl;

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
		icon_name = software_type;
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
static std::string ProcessSWDir(int drvindex)
{
	std::string result;

	result = "1"s + emu_opts.get_exe_path_utf8();

	if (drvindex < 0)
		return result;

	bool b_dir = false;
	int nCloneIndex, nParentIndex;
	size_t find_pos;
	std::string dirmap_swpath, option_swpath;

	std::cout << "ProcessSWDir: A" << std::endl;
	dirmap_swpath = emu_opts.dir_get_value(DIRPATH_SWPATH);
	if (!dirmap_swpath.empty())
	{
		std::cout << "ProcessSWDir: B = " << dirmap_swpath << std::endl;
		find_pos = dirmap_swpath.find(';');
		if (find_pos != std::string::npos)
			dirmap_swpath.resize(find_pos);
		if (osd::directory::open(dirmap_swpath.c_str()))  // make sure its valid
			b_dir = true;
	}

	// Get the game's software path
	std::cout << "ProcessSWDir: C" << std::endl;
	windows_options o;
	emu_opts.load_options(o, SOFTWARETYPE_GAME, drvindex, 0);

	option_swpath = o.value(OPTION_SWPATH);
	find_pos = option_swpath.find(';');
	if (find_pos != std::string::npos)
		option_swpath.resize(find_pos);
	std::cout << "ProcessSWDir: D = " << option_swpath << " = " << o.value(OPTION_SWPATH) << std::endl;
	if (osd::directory::open(option_swpath.c_str()))  // make sure its valid
		if (b_dir && (dirmap_swpath != option_swpath))
		{
			result = "1"s + o.value(OPTION_SWPATH);
			return result;
		}

	// not specified in driver, try parent if it has one
	std::cout << "ProcessSWDir: E" << std::endl;
	nParentIndex = drvindex;
	if (DriverIsClone(drvindex))
	{
		std::cout << "ProcessSWDir: F" << std::endl;
		nParentIndex = GetParentIndex(&driver_list::driver(drvindex));
		if (nParentIndex >= 0)
		{
			std::cout << "ProcessSWDir: G" << std::endl;
			emu_opts.load_options(o, SOFTWARETYPE_PARENT, nParentIndex, 0);

			option_swpath = o.value(OPTION_SWPATH);

			find_pos = option_swpath.find(';');
			if (find_pos != std::string::npos)
				option_swpath.resize(find_pos);
			std::cout << "ProcessSWDir: GA = " << option_swpath << std::endl;
			if (osd::directory::open(option_swpath.c_str()))  // make sure its valid
			{
				std::cout << "ProcessSWDir: GB" << std::endl;
				if (b_dir && dirmap_swpath != option_swpath)
				{
					std::cout << "ProcessSWDir: GC" << std::endl;
					result = "1"s + o.value(OPTION_SWPATH);
					return result;
				}
			}
			else
				nParentIndex = drvindex; // don't pass -1 to compat check
		}
	}

	// Try compat if it has one
	std::cout << "ProcessSWDir: H = " << nParentIndex << std::endl;
	nCloneIndex = GetCompatIndex(&driver_list::driver(nParentIndex));
	std::cout << "ProcessSWDir: HA = " << nCloneIndex << std::endl;
	if (nCloneIndex >= 0)
	{
		std::cout << "ProcessSWDir: I" << std::endl;
		emu_opts.load_options(o, SOFTWARETYPE_PARENT, nCloneIndex, 0);
		option_swpath = o.value(OPTION_SWPATH);
		if (!option_swpath.empty())
		{
			find_pos = option_swpath.find(';');
			if (find_pos != std::string::npos)
			{
				option_swpath.resize(find_pos);
				if (osd::directory::open(option_swpath.c_str()))  // make sure its valid
					if (b_dir && (dirmap_swpath != option_swpath))
					{
						result = "1"s + o.value(OPTION_SWPATH);
						return result;
					}
			}
		}
	}

	// Try the global root
	std::cout << "ProcessSWDir: J" << std::endl;
	if (b_dir)
	{
		result = "0"s + dirmap_swpath;
		return result;
	}

	// nothing valid, drop to default emu directory
	std::cout << "ProcessSWDir: K" << std::endl;
	std::cout << "ProcessSWDir: L" << std::endl;
	return result;
}


// Split multi-directory for SW Files into separate directories, and ask the picker to add the files from each.
// pszSubDir path not used by any caller.
static bool AddSoftwarePickerDirs(HWND hwndPicker, std::wstring_view directories, std::wstring_view subdirectory = L""sv)
{
	bool result = false;

	if (directories.empty())
		return result;

	std::wistringstream tokenStream(&directories[0]);
	std::wstring directory_path, token;

	if (!subdirectory.empty())
	{
		while (std::getline(tokenStream, token, L';'))
		{
			directory_path = token + L"\\"s + &subdirectory[0];
			result = SoftwarePicker_AddDirectory(hwndPicker, directory_path);
			if (!result)
				break;
		}
	}
	else
	{
		while (std::getline(tokenStream, token, L';'))
		{
			directory_path = token;
			result = SoftwarePicker_AddDirectory(hwndPicker, directory_path);
			if (!result)
				break;
		}
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
	std::cout << "MView_Refresh: " << driver_list::driver(pMViewInfo->config->driver_index).name << std::endl;

	if (pMViewInfo->slots)
	{
		for (size_t i = 0; i < pMViewInfo->slots; i++)
		{
			pszSelection = pMViewInfo->pCallbacks->pfnGetSelectedSoftware(pMViewInfo->config->driver_index,
				pMViewInfo->pEntries[i].dev);
			std::cout << "MView_Refresh: Finished GetSelectSoftware" << std::endl;
			if (!&pszSelection[0])
				pszSelection = L"";

			pMViewInfo->bSurpressFilenameChanged = true;
			windows::set_window_text(pMViewInfo->pEntries[i].hwndEdit, pszSelection.c_str());
			pMViewInfo->bSurpressFilenameChanged = false;
		}
	}
}


//#ifdef __GNUC__
//#pragma GCC diagnostic ignored "-Wunused-variable"
//#endif
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
	std::cout << "MView_SetDriver: A" << std::endl;
	MView_Clear(hwndMView);

	// copy the config
	std::cout << "MView_SetDriver: B" << std::endl;
	pMViewInfo->config = sconfig;
	if (!sconfig || !has_software)
		return false;

	// count total amount of devices
	std::cout << "MView_SetDriver: C" << std::endl;
	for (device_image_interface &dev : image_interface_enumerator(pMViewInfo->config->mconfig->root_device()))
	{
		if (dev.user_loadable())
			pMViewInfo->slots++;
	}
	std::cout << "MView_SetDriver: Number of slots = " << pMViewInfo->slots << std::endl;

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
			ppszDevices.push_back(utf16_instance_name);
		}
		std::cout << "MView_SetDriver: Number of media slots = " << ppszDevices.size() << std::endl;

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
	std::cout << "MView_SetDriver: Calling MView_Refresh" << std::endl;
	MView_Refresh(hwndMView); // show names of already-loaded software
	std::cout << "MView_SetDriver: Finished" << std::endl;
	return true;
}
//#ifdef __GNUC__
//#pragma GCC diagnostic error "-Wunused-variable"
//#endif


bool MyFillSoftwareList(int drvindex, bool bForce)
{
	HWND hwndSoftwareList, hwndSoftwareMView, hwndSoftwarePicker;
	std::string paths;

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

	std::cout << "MyFillSoftwareList: Calling SoftwareList_Clear" << std::endl;
	SoftwareList_Clear(hwndSoftwareList);
	std::cout << "MyFillSoftwareList: Calling SoftwarePicker_Clear" << std::endl;
	SoftwarePicker_Clear(hwndSoftwarePicker);

	// set up the device view
	std::cout << "MyFillSoftwareList: Calling MView_SetDriver" << std::endl;
	MView_SetDriver(hwndSoftwareMView, s_config);

	// set up the software picker
	std::cout << "MyFillSoftwareList: Calling SoftwarePicker_SetDriver" << std::endl;
	SoftwarePicker_SetDriver(hwndSoftwarePicker, s_config);

	if (!s_config || !has_software)
		return has_software;

	// set up the Software Files by using swpath (can handle multiple paths)
	std::cout << "MyFillSoftwareList: Processing SWDir" << std::endl;
	paths = ProcessSWDir(drvindex);
	std::cout << "MyFillSoftwareList: Finished SWDir = " << paths << std::endl;
	if (paths[0] == '1')
	{
		std::cout << "MyFillSoftwareList: Calling AddSoftwarePickerDirs" << std::endl;
		paths.erase(0,1);
		AddSoftwarePickerDirs(hwndSoftwarePicker, mui_utf16_from_utf8string(paths));
	}

	// set up the Software List
	std::cout << "MyFillSoftwareList: Calling SoftwarePicker_SetDriver" << std::endl;
	SoftwareList_SetDriver(hwndSoftwareList, s_config);

	std::cout << "MyFillSoftwareList: Getting Softlist Information" << std::endl;
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
	std::cout << "MyFillSoftwareList: Finished" << std::endl;
	return has_software;
}


void MessUpdateSoftwareList(void)
{
	HWND hwndList = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_LIST);
	MyFillSoftwareList(Picker_GetSelectedItem(hwndList), true);
}


// Places the specified image in the specified slot - MUST be a valid filename, not blank
static void MessSpecifyImage(int drvindex, const device_image_interface *device_image, std::string file_name)
{
	
	if (device_image)
	{
		emu_opts.SetSelectedSoftware(drvindex, device_image->instance_name(), file_name);
		if (LOG_SOFTWARE)
			std::cout << "MessSpecifyImage(): instance_name = '" << device_image->instance_name() << "', file_name ='" << file_name << "'" << std::endl;

		return;
	}

	std::string file_extension, option_name;
	std::string::size_type dot_pos;

	// identify the file extension
	dot_pos = file_name.rfind('.');
	if (dot_pos != std::string::npos)
		file_extension = file_name.substr(dot_pos + 1, file_name.size());

	if (!file_extension.empty())
	{
		if (LOG_SOFTWARE)
			std::cout << "MessSpecifyImage(): file_extension = (" << file_extension << ")" << std::endl;

		for (device_image_interface &current_image : image_interface_enumerator(s_config->mconfig->root_device()))
		{
			if (!current_image.user_loadable())
				continue;

			if (uses_file_extension(current_image, file_extension))
			{
				std::string option_name = current_image.instance_name();
				if (!option_name.empty())
				{
					// place the image
					emu_opts.SetSelectedSoftware(drvindex, option_name, file_name);
					if (LOG_SOFTWARE)
						std::cout << "MessSpecifyImage(): option_name = '" << option_name << "', file_name ='" << file_name << "'" << std::endl;
				}
				else
				{
					// could not place the image
					if (LOG_SOFTWARE)
						std::cout << "MessSpecifyImage(): Failed to place image '" << file_name << "'" << std::endl;
				}

				break;
			}
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

	s_bIgnoreSoftwarePickerNotifies = true;

	// Now clear everything out; this may call back into us but it should not be problematic
	list_view::set_item_state(hwndSoftware, -1, 0, LVIS_SELECTED);

	// Get the game's options including slots & software
	windows_options o;
	emu_opts.load_options(o, SOFTWARETYPE_GAME, s_config->driver_index, 1);
	/* allocate the machine config */
	machine_config config(driver_list::driver(s_config->driver_index), o);

	for (device_image_interface &dev : image_interface_enumerator(config.root_device()))
	{
		std::string option_name, option_value;

		if (!dev.user_loadable())
			continue;

		option_name = dev.instance_name(); // get name of device slot
		option_value = o.value(&option_name[0]); // get name of software in the slot

		if (!option_value.empty()) // if software is loaded
		{
			int software_index = SoftwarePicker_LookupIndex(hwndSoftware, option_value); // see if its in the picker
			if (software_index < 0) // not there
			{
				// add already loaded software to picker, but not if its already there
//              SoftwarePicker_AddFile(hwndSoftware, option_value, 1);    // this adds the 'extra' loaded software item into the list - we don't need to see this
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
	}

	s_bIgnoreSoftwarePickerNotifies = false;
}

static std::wstring GetDialogFilter(std::vector<mess_image_type>& image_types)
{
	std::wstring dialog_filter;

	dialog_filter = L"Common image types (";
	for (size_t index = 0, last_pos = image_types.size() - 1; index < image_types.size(); index++)
	{
		mess_image_type& type = image_types[index];
		dialog_filter += L"*." + type.ext;
		if (index < last_pos)
			dialog_filter += L",";
		else
			dialog_filter += L")";
	}

	dialog_filter.append(L"\0", 1);
	for (size_t index = 0, last_pos = image_types.size() - 1; index < image_types.size(); index++)
	{
		mess_image_type& type = image_types[index];
		dialog_filter += L"*." + type.ext;
		if (index < last_pos)
			dialog_filter += L";";
		else
			dialog_filter.append(L"\0", 1);
	}

	dialog_filter.append(L"All files (*.*)\0*.*\0", 20);
	for (size_t index = 0; index < image_types.size(); index++)
	{
		mess_image_type& type = image_types[index];
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

/* Specify std::size(s_devices) for type if you want all types */
static std::vector<mess_image_type> SetupImageTypes(bool include_archives, const device_image_interface* dev)
{
	std::vector<mess_image_type> image_types;

	if (include_archives)
	{
		image_types.emplace_back(mess_image_type{ nullptr, L"7z",L"" });
		image_types.emplace_back(mess_image_type{ nullptr, L"zip",L"" });
	}

	if (dev != nullptr)
	{
		std::wistringstream tokenStream(mui_utf16_from_utf8string(dev->file_extensions()));
		std::wstring token;

		while (std::getline(tokenStream, token, L','))
		{
			const std::wstring &dlgname = lookupdevice(dev->image_brief_type_name())->dlgname;
			image_types.emplace_back(mess_image_type{ dev, std::move(token), dlgname });
		}
	}

	return image_types;
}


static void MessSetupDevice(common_file_dialog_proc cfd, const device_image_interface *dev)
{
	bool bResult;
	HWND hwndList;
	int drvindex;
	std::vector<mess_image_type> image_types;
	std::string software_path;
	std::string::size_type find_pos;
	wchar_t file_name[MAX_PATH];

	software_path = emu_opts.dir_get_value(DIRPATH_SWPATH);

	// We only want the first path; throw out the rest
	find_pos = software_path.find(';');
	if (find_pos != std::string_view::npos)
		software_path = software_path.substr(0, find_pos);

	//  begin_resource_tracking();
	hwndList = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_LIST);
	drvindex = Picker_GetSelectedItem(hwndList);
	if (drvindex < 0)
		return;

	// allocate the machine config
//  machine_config config(driver_list::driver(drvindex), MameUIGlobal());

	image_types = SetupImageTypes(true, dev);
	bResult = CommonFileImageDialog(mui_utf16_from_utf8string(software_path).c_str(), cfd, file_name, image_types);
	if (bResult)
	{
		SoftwarePicker_AddFile(dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWLIST), file_name, 0);
	}
}


// This is used to Unmount a file from the Media View.
// Unused fields: hwndMView, config, pszFilename, nFilenameLength
static bool MView_Unmount(const device_image_interface *dev)
{
	int drvindex = Picker_GetSelectedItem(dialog_boxes::get_dlg_item(GetMainWindow(), IDC_LIST));

	if (drvindex < 0)
		return false;
	emu_opts.SetSelectedSoftware(drvindex, dev->instance_name(), "");
	mvmap[dev->instance_name()] = 0;
	return true;
}


// This is used to Mount an existing software File in the Media View
static bool MView_GetOpenFileName(const device_image_interface *dev, std::wstring_view file_name)
{
	bool bResult;
	HWND hwndList;
	int drvindex;
	std::string initial_directory, option_name, option_value;
	std::string::size_type find_pos;
	windows_options options;
	std::vector<mess_image_type> image_types;

	hwndList = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_LIST);
	drvindex = Picker_GetSelectedItem(hwndList);
	if (drvindex < 0)
		return false;

	option_name = dev->instance_name();

	emu_opts.load_options(options, SOFTWARETYPE_GAME, drvindex, 1);
	option_value = options.value(option_name);

	/* Get the path to the currently mounted image */
	initial_directory = util::zippath_parent(option_value);

	find_pos = initial_directory.find(':');
	if ((!osd::directory::open(&initial_directory[0])) || (find_pos == std::string::npos))
	{
		std::cout << "MView_GetOpenFileName: initial_directory = " << initial_directory << std::endl;
		// no image loaded, use swpath
		initial_directory = ProcessSWDir(drvindex);
		std::cout << "MView_GetOpenFileName: initial_directory = " << initial_directory << std::endl;
		initial_directory.erase(0,1);
		/* We only want the first path; throw out the rest */
		find_pos = initial_directory.find(';');
		if (find_pos != std::string::npos)
			initial_directory = initial_directory.substr(0, find_pos);
	}

	image_types = SetupImageTypes(true, dev);
	bResult = CommonFileImageDialog(mui_utf16_from_utf8string(initial_directory).c_str(), dialog_boxes::get_open_filename, const_cast<wchar_t*>(&file_name[0]), image_types);
	if (bResult)
	{
		emu_opts.SetSelectedSoftware(drvindex, dev->instance_name(), mui_utf8_from_utf16string(file_name));
		mvmap[option_name] = 1;
	}

	return bResult;
}


// This is used to Mount a software-list Item in the Media View.
static bool MView_GetOpenItemName(const device_image_interface *dev,std::wstring_view file_name)
{
	bool bResult;
	HWND hwndList;
	int drvindex;
	std::vector<mess_image_type> imagetypes;
	std::string dst,opt_name;

	// sanity check - should never happen
	opt_name = dev->instance_name();
	if (slmap.find(opt_name) == slmap.end())
	{
		std::cout << "MView_GetOpenItemName used invalid device of " << opt_name << std::endl;
		return false;
	}

	hwndList = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_LIST);
	drvindex = Picker_GetSelectedItem(hwndList);
	if (drvindex < 0)
		return false;

	dst = slmap.find(opt_name)->second;

	if (!osd::directory::open(dst.c_str()))
		// Default to emu directory
		osd_get_full_path(dst, ".");

	imagetypes = SetupImageTypes(true, nullptr); // just get zip & 7z
	bResult = CommonFileImageDialog(mui_utf16_from_utf8string(dst).c_str(), dialog_boxes::get_open_filename, const_cast<wchar_t*>(file_name.data()), imagetypes);

	if (bResult)
	{
		std::string::size_type find_pos;
		std::wstring parsed_file_name;

		parsed_file_name = file_name;
		// Get the Item name out of the full path
		find_pos = parsed_file_name.find(L".zip"); // get rid of zip name and anything after
		if (find_pos != std::wstring::npos)
			parsed_file_name.erase(find_pos);
		else
		{
			find_pos = parsed_file_name.find(L".7z"); // get rid of 7zip name and anything after
			if (find_pos != std::wstring::npos)
				parsed_file_name.erase(find_pos);
		}
		find_pos = parsed_file_name.rfind(L"\\");   // put the swlist name in
		parsed_file_name[find_pos] = ':';
		find_pos = parsed_file_name.rfind(L"\\"); // get rid of path; we only want the item name
		parsed_file_name.erase(0, find_pos +1);

		std::wcout << L"MView_GetOpenItemName: parsed_file_name = " << parsed_file_name << std::endl;
		// set up editbox display text

		// set up inifile text to signify to MAME that a SW ITEM is to be used ************** will only load to the specified slot, multipart items are cut to the first
		emu_opts.SetSelectedSoftware(drvindex, dev->instance_name(), mui_utf8_from_utf16string(parsed_file_name));
		mvmap[opt_name] = 1;
	}

	return bResult;
}


// This is used to Create an image in the Media View.
static bool MView_GetCreateFileName(const device_image_interface *dev, std::wstring_view file_name)
{
	bool bResult;
	HWND hwndList;
	int drvindex;
	std::wstring initial_directory;
	std::string::size_type find_pos;
	std::vector<mess_image_type> image_types;

	hwndList = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_LIST);
	drvindex = Picker_GetSelectedItem(hwndList);
	if (drvindex < 0)
		return false;

	initial_directory = mui_utf16_from_utf8string(ProcessSWDir(drvindex));
	initial_directory.erase(0,1);
	/* We only want the first path; throw out the rest */
	find_pos = initial_directory.find(';');
	if (find_pos != std::wstring::npos)
		initial_directory = initial_directory.substr(0, find_pos);

	image_types = SetupImageTypes(true, dev);
	bResult = CommonFileImageDialog(initial_directory.data(), dialog_boxes::get_save_filename, const_cast<wchar_t*>(file_name.data()), image_types);
	if (bResult)
	{
		emu_opts.SetSelectedSoftware(drvindex, dev->instance_name(), mui_utf8_from_utf16string(file_name.data()));
		mvmap[dev->instance_name()] = 1;
	}

	return bResult;
}


// Unused fields: hwndMView, config
static void MView_SetSelectedSoftware(int drvindex, const device_image_interface *dev, std::wstring_view file_name)
{
	MessSpecifyImage(drvindex, dev, mui_utf8_from_utf16string(file_name.data()));
	MessRefreshPicker();
}


// Unused fields: config
static std::wstring MView_GetSelectedSoftware(int drvindex, const device_image_interface *dev)
{
	std::string opt_name, opt_value;

	if (dev && dev->user_loadable())
	{
		windows_options o;

		emu_opts.load_options(o, SOFTWARETYPE_GAME, drvindex, 1);

		opt_name = dev->instance_name();
		if (o.has_image_option(opt_name))
		{
			std::cout << "MView_GetSelectedSoftware: Got option '" << opt_name << "'" << std::endl;

			opt_value = o.image_option(opt_name).value();
			if (!opt_value.empty())
			{

				std::cout << "MView_GetSelectedSoftware: Got value '" << opt_value << "'" << std::endl;
				std::wstring utf8_to_wcs = mui_utf16_from_utf8string(opt_value);
				if (!utf8_to_wcs.empty())
				{
					mvmap[opt_name] = 1;
					return utf8_to_wcs;
				}
			}
			else
				std::cout << "MView_GetSelectedSoftware: Option '" << opt_name << "' has no value" << std::endl;
		}
		else
		{
			std::cout << "MView_GetSelectedSoftware: Option '" << opt_name << "' not found" << std::endl;
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
			INT resource = lookupdevice(nType)->resource;
			//if (!icon_name)
				//icon_name = nType.c_str();
			nIcon = FindIconIndex(resource);
			if (nIcon < 0)
				nIcon = FindIconIndex(IDI_WIN_UNKNOWN);
		}
	}
	return nIcon;
}


static void SoftwarePicker_LeavingItem(HWND hwndSoftwarePicker, int nItem)
{
#if 0
	if (!s_bIgnoreSoftwarePickerNotifies)
	{
		HWND hwndList = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_LIST);
		int drvindex = Picker_GetSelectedItem(hwndList);
		if (drvindex < 0)
			return;

		const char *pszFullName = SoftwarePicker_LookupFilename(hwndSoftwarePicker, nItem);
		MessRemoveImage(drvindex, pszFullName);
	}
#endif
}


static void SoftwarePicker_EnteringItem(HWND hwndSoftwarePicker, int nItem)
{
	HWND hwndList = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_LIST);

	if (!s_bIgnoreSoftwarePickerNotifies)
	{
		int drvindex;
		std::string full_name, part_name;

		drvindex = Picker_GetSelectedItem(hwndList);
		if (drvindex < 0)
		{
			g_szSelectedItem.clear();
			return;
		}

		// Get the fullname and partialname for this file
		full_name = SoftwarePicker_LookupFilename(hwndSoftwarePicker, nItem);
		std::cout << "SoftwarePicker_EnteringItem: full_name = \"" << full_name << "\"" << std::endl;
		part_name = SoftwarePicker_LookupBasename(hwndSoftwarePicker, nItem);
		std::cout << "SoftwarePicker_EnteringItem: part_name = \"" << part_name << "\"" << std::endl;
		// Do the dirty work
		MessSpecifyImage(drvindex, nullptr, full_name.c_str());
		part_name.resize(part_name.find('.') - 1);
		// Set up g_szSelectedItem, for the benefit of UpdateScreenShot()
		g_szSelectedItem = part_name;

		UpdateScreenShot();
	}
}


// ------------------------------------------------------------------------
// Software List Class - not used, swlist items don't have icons
// ------------------------------------------------------------------------

static int SoftwareList_GetItemImage(HWND hwndPicker, int nItem)
{
#if 0
	HWND hwndGamePicker = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_LIST);
	HWND hwndSoftwareList = dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SOFTLIST);
	int drvindex = Picker_GetSelectedItem(hwndGamePicker);
	if (drvindex < 0)
		return -1;

	iodevice_t nType = SoftwareList_GetImageType(hwndSoftwareList, nItem);
	int nIcon = GetMessIcon(drvindex, nType);
	if (!nIcon)
	{
		switch(nType)
		{
			case IO_UNKNOWN:
				nIcon = FindIconIndex(IDI_WIN_REDX);
				break;

			default:
				const char *icon_name = lookupdevice(nType)->icon_name;
				if (!icon_name)
					icon_name = device_image_interface::device_typename(nType);
				nIcon = FindIconIndexByName(icon_name);
				if (nIcon < 0)
					nIcon = FindIconIndex(IDI_WIN_UNKNOWN);
				break;
		}
	}
	return nIcon;
#endif
	return 0;
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
		const std::string &pszFullName = SoftwareList_LookupFullname(hwndSoftwareList, nItem); // for the screenshot and SetSoftware.

		// For UpdateScreenShot()
		g_szSelectedItem = pszFullName;
		UpdateScreenShot();
		// use SOFTWARENAME option to properly load a multipart set
		emu_opts.SetSelectedSoftware(drvindex, "", pszFullName);
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
			MView_Refresh(dialog_boxes::get_dlg_item(GetMainWindow(), IDC_SWDEVVIEW));
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

		std::cout << "MView_ButtonClick: dirmap_mediapath = " << dirmap_mediapath << std::endl;
		while (std::getline(tokenStream, sl_root, ';'))
		{
			DWORD dwAttrib;
			std::string sw_path = sl_root + std::string("\\") + slmap.find(opt_name)->second;
			dwAttrib = storage::get_file_attributes_utf8(sw_path.c_str());
			if ((dwAttrib != INVALID_FILE_ATTRIBUTES) && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
			{
				std::cout << L"MView_ButtonClick: sw_path = " << sw_path << std::endl;
				slmap[opt_name] = sw_path;
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

	if (mvmap.find(opt_name)->second == 1)
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
	MViewInfo *pMViewInfo;
	MViewEntry *pEnt;
	int nStaticPos, nStaticWidth, nEditPos, nEditWidth, nButtonPos, nButtonWidth;
	RECT r;
	LONG_PTR l = 0;
	LRESULT rc = 0;
	BOOL bHandled = false;
	HWND hwndButton;

	switch(nMessage)
	{
		case WM_DESTROY:
			MView_Free(hwndMView);
			break;

		case WM_SHOWWINDOW:
			if (wParam)
			{
				pMViewInfo = GetMViewInfo(hwndMView);
				MView_Refresh(hwndMView);
			}
			break;

		case WM_COMMAND:
			switch(HIWORD(wParam))
			{
				case BN_CLICKED:
					hwndButton = (HWND) lParam;
					l = windows::get_window_long_ptr(hwndButton, GWLP_USERDATA);
					pEnt = (MViewEntry *) l;
					MView_ButtonClick(hwndMView, pEnt, hwndButton);
					break;
			}
			break;
	}

	if (!bHandled)
		rc = windows::def_window_proc(hwndMView, nMessage, wParam, lParam);

	switch(nMessage)
	{
		case WM_CREATE:
			if (!MView_Setup(hwndMView))
				return -1;
			break;

		case WM_MOVE:
		case WM_SIZE:
			pMViewInfo = GetMViewInfo(hwndMView);
			pEnt = pMViewInfo->pEntries;
			if (pEnt)
			{
				MView_GetColumns(hwndMView, &nStaticPos, &nStaticWidth,
					&nEditPos, &nEditWidth, &nButtonPos, &nButtonWidth);
				while(pEnt->dev)
				{
					(void)windows::get_client_rect(pEnt->hwndStatic, &r);
					gdi::map_window_points(pEnt->hwndStatic, hwndMView, ((POINT *) &r), 2);
					//windows::move_window(pEnt->hwndStatic, nStaticPos, r.top, nStaticWidth, r.bottom - r.top, false); // has its own line, no need to move
					// On next line, so need MVIEW_SPACING to put them down there.
					windows::move_window(pEnt->hwndEdit, nEditPos, r.top+MVIEW_SPACING, nEditWidth, r.bottom - r.top, false);
					windows::move_window(pEnt->hwndBrowseButton, nButtonPos, r.top+MVIEW_SPACING, nButtonWidth, r.bottom - r.top, false);
					pEnt++;
				}
			}
			break;
	}
	return rc;
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

