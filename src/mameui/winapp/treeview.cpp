// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

/***************************************************************************

  treeview.cpp

  TreeView support routines - MSH 11/19/1998

***************************************************************************/

// standard C++ headers
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
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
#include "romload.h"
#include "screen.h"

#include "ui/moptions.h"
#include "winopts.h"

// MAMEUI headers
#include "windows_controls.h"
#include "dialog_boxes.h"
#include "windows_gdi.h"
#include "menus_other_res.h"
#include "data_access_storage.h"
#include "system_services.h"
#include "windows_messages.h"

#include "mui_cstr.h"
#include "mui_stringtokenizer.h"
#include "mui_wcstrconv.h"

#include "bitmask.h"
#include "dialogs.h"
#include "emu_opts.h"
#include "mui_opts.h"
#include "mui_util.h"
#include "resource.h"
#include "screenshot.h"
#include "winui.h"

#include "treeview.h"

//using namespace std::string_literals;
using namespace std::string_view_literals;

using namespace mameui::winapi;
using namespace mameui::winapi::controls;
using namespace mameui::util;

//#define MAX_EXTRA_FOLDERS 256

/***************************************************************************
    public structures
 ***************************************************************************/

#define ICON_MAX (std::size(treeIconNames))

/* Name used for user-defined custom icons */
/* external *.ico file to look for. */

using TREEICON = struct tree_icon
{
	int nResourceID;
	LPCSTR lpName;
};

static TREEICON treeIconNames[] =
{
	{ IDI_FOLDER_OPEN,         "foldopen" },
	{ IDI_FOLDER,              "folder" },
	{ IDI_FOLDER_AVAILABLE,    "foldavail" },
	{ IDI_FOLDER_MANUFACTURER, "foldmanu" },
	{ IDI_FOLDER_UNAVAILABLE,  "foldunav" },
	{ IDI_FOLDER_YEAR,         "foldyear" },
	{ IDI_FOLDER_SOURCE,       "foldsrc" },
	{ IDI_FOLDER_HORIZONTAL,   "horz" },
	{ IDI_FOLDER_VERTICAL,     "vert" },
	{ IDI_MANUFACTURER,        "manufact" },
	{ IDI_FOLDER_WORKING,      "working" },
	{ IDI_FOLDER_NONWORKING,   "nonwork" },
	{ IDI_YEAR,                "year" },
	{ IDI_SOUND,               "sound" },
	{ IDI_CPU,                 "cpu" },
	{ IDI_FOLDER_HARDDISK,     "harddisk" },
	{ IDI_SOURCE,              "source" }
};

/***************************************************************************
    private variables
 ***************************************************************************/

/* this has an entry for every folder eventually in the UI, including subfolders */
static TREEFOLDER **treeFolders = 0;
static int          numFolders  = 0;        /* Number of folder in the folder array */
static UINT         next_folder_id = MAX_FOLDERS;
static UINT         folderArrayLength = 0;  /* Size of the folder array */
static LPTREEFOLDER lpCurrentFolder = 0;    /* Currently selected folder */
static UINT         nCurrentFolder = 0;     /* Current folder ID */
static WNDPROC      g_lpTreeWndProc = 0;    /* for subclassing the TreeView */
static HIMAGELIST   hTreeSmall = 0;         /* TreeView Image list of icons */

/* this only has an entry for each TOP LEVEL extra folder + SubFolders*/
LPEXFOLDERDATA ExtraFolderData[MAX_EXTRA_FOLDERS * MAX_EXTRA_SUBFOLDERS];
static int numExtraFolders = 0;
static int numExtraIcons = 0;
static std::string ExtraFolderIcons[MAX_EXTRA_FOLDERS];

// built in folders and filters
static LPCFOLDERDATA  g_lpFolderData;
static LPCFILTER_ITEM g_lpFilterList;

/***************************************************************************
    private function prototypes
 ***************************************************************************/

extern bool InitFolders(void);
LPEXFOLDERDATA NewExtraFolderData(std::string title, int folderId, int parentId, int iconId, int subIconId = -1, DWORD dwFlags = 0UL);
static bool AddFolder(LPTREEFOLDER lpFolder);
static bool ci_contains(std::string_view string, std::string_view sub_string);
static bool CreateTreeIcons(void);
static bool TryAddExtraFolderAndChildren(int parent_index);
static bool TrySaveExtraFolder(LPTREEFOLDER lpFolder);
static bool write_folder_contents(std::ofstream& out, TREEFOLDER* folder_data);
static int InitExtraFolders(void);
static LPTREEFOLDER NewFolder(std::string lpTitle, UINT nFolderId, int nParent, UINT nIconId, DWORD dwFlags);
static LRESULT CALLBACK TreeWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static std::optional<std::filesystem::path> find_or_create_category_path(std::string_view option_value);
static std::string TrimManufacturer(std::string_view manufacturer_string);
static void DeleteFolder(LPTREEFOLDER &lpFolder);
static void FreeExtraFolders(void);
static void save_folder_sections(std::ofstream& out, int parent_index);
static void SaveExternalFolders(int parent_index, std::string_view fname);
static void SetExtraIcons(std::string_view name, int *id);
static void TreeCtrlOnPaint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
std::string ParseManufacturer(std::string_view input, int& parsedChars);


/***************************************************************************
    public functions
 ***************************************************************************/

/* De-allocate all folder memory */
void FreeFolders(void)
{
	FreeExtraFolders();
	if (treeFolders)
	{
		for (size_t i = 0; i < numFolders; i++)
		{
			if (treeFolders[i])
				DeleteFolder(treeFolders[i]);

		}

		delete[] treeFolders;
		treeFolders = 0;
		numFolders = 0;
	}
}

/* Reset folder filters */
void ResetFilters(void)
{
	if (treeFolders)
		for (size_t i = 0; i < (int)numFolders; i++)
			treeFolders[i]->m_dwFlags &= ~F_MASK;
}

void InitTreeView(LPCFOLDERDATA lpFolderData, LPCFILTER_ITEM lpFilterList)
{
	g_lpFolderData = lpFolderData;
	g_lpFilterList = lpFilterList;

	InitFolders();

	/* this will subclass the treeview (where WM_DRAWITEM gets sent for the header control) */
	LONG_PTR l = windows::get_window_long_ptr(GetTreeView(), GWLP_WNDPROC);
	g_lpTreeWndProc = (WNDPROC)l;
	(void)windows::set_window_long_ptr(GetTreeView(), GWLP_WNDPROC, (LONG_PTR)TreeWndProc);
}

void SetCurrentFolder(LPTREEFOLDER lpFolder)
{
	lpCurrentFolder = (!lpFolder) ? treeFolders[0] : lpFolder;
	nCurrentFolder = (!lpCurrentFolder) ? 0 : lpCurrentFolder->m_nFolderId;
}

LPTREEFOLDER GetCurrentFolder(void)
{
	return lpCurrentFolder;
}

UINT GetCurrentFolderID(void)
{
	return nCurrentFolder;
}

int GetNumFolders(void)
{
	return numFolders;
}

LPTREEFOLDER GetFolder(UINT nFolder)
{
	return (nFolder < numFolders) ? treeFolders[nFolder] : nullptr;
}

LPTREEFOLDER GetFolderByID(UINT nID)
{
	for (UINT i = 0; i < numFolders; i++)
		if (treeFolders[i]->m_nFolderId == nID)
			return treeFolders[i];

	return nullptr;
}

void AddGame(LPTREEFOLDER lpFolder, UINT nGame)
{
	if (lpFolder)
		SetBit(lpFolder->m_lpGameBits, nGame);
}

void RemoveGame(LPTREEFOLDER lpFolder, UINT nGame)
{
	ClearBit(lpFolder->m_lpGameBits, nGame);
}

int FindGame(LPTREEFOLDER lpFolder, int nGame)
{
	return FindBit(lpFolder->m_lpGameBits, nGame, true);
}

void RebuildGameList(LPTREEFOLDER folder, const FOLDERDATA& folderData, int totalGames)
{
	SetAllBits(folder->m_lpGameBits, false);

	for (int gameIndex = 0; gameIndex < totalGames; ++gameIndex)
	{
		// Evaluate the game using the query function, if available.
		bool result = folderData.m_pfnQuery ? folderData.m_pfnQuery(gameIndex) : true;

		// Invert the result if the expected result is false.
		if (!folderData.m_bExpectedResult)
			result = !result;

		// Add the game to the folder if the result is true.
		if (result)
			AddGame(folder, gameIndex);
	}
}

// Called to re-associate games with folders
void ResetWhichGamesInFolders()
{
	const int totalGames = driver_list::total();

	for (size_t i = 0; i < numFolders; ++i)
	{
		LPTREEFOLDER folder = treeFolders[i];

		for (size_t k = 0; !g_lpFolderData[k].m_lpTitle.empty(); ++k)
		{
			const auto& folderData = g_lpFolderData[k];

			if (folder->m_nFolderId != folderData.m_nFolderId)
				continue;

			if (folderData.m_pfnQuery || folderData.m_bExpectedResult)
				RebuildGameList(folder, folderData, totalGames);

			break;
		}
	}
}


/* Used to build the GameList */
bool GameFiltered(int nGame, DWORD dwMask)
{
	LPTREEFOLDER lpFolder = GetCurrentFolder();
	LPTREEFOLDER lpParent = nullptr;
	std::string driver_fullname, driver_name, driver_manufacturer, driver_source;

	if (!(nGame < driver_list::total()))
		return true;


	const char* driver_info = driver_list::driver(nGame).type.fullname();
	driver_fullname = (driver_info) ? driver_info : "";
	driver_info = driver_list::driver(nGame).name;
	driver_name = (driver_info) ? driver_info : "";
	driver_info = driver_list::driver(nGame).manufacturer;
	driver_manufacturer = (driver_info) ? driver_info : "";
	driver_info = driver_list::driver(nGame).type.source();
	driver_source = (driver_info) ? driver_info : "";

	//Filter out the Bioses on all Folders, except for the Bios Folder
	if (lpFolder->m_nFolderId != FOLDER_BIOS)
	{
		//      if( !( (driver_list::driver(nGame).flags & MACHINE_IS_BIOS_ROOT ) == 0) )
		//          return true;
		if (driver_name[0] == '_')
			return true;
	}
	// Filter games--return true if the game should be HIDDEN in this view
	if (GetFilterInherit())
	{
		if (lpFolder)
		{
			lpParent = GetFolder(lpFolder->m_nParent);
			if (lpParent)
			{
				/* Check the Parent Filters and inherit them on child,
				 * The inherited filters don't display on the custom Filter Dialog for the Child folder
				 * No need to promote all games to parent folder, works as is */
				dwMask |= lpParent->m_dwFlags;
			}
		}
	}

	if (!GetSearchText().empty() && mui_stricmp(GetSearchText(), SEARCH_PROMPT))
	{
		if (!ci_contains(driver_fullname, GetSearchText()) && !ci_contains(driver_name, GetSearchText()))
			return true;
	}

	/*Filter Text is already global*/
	if (!ci_contains(driver_fullname, GetFilterText()) && !ci_contains(driver_name, GetFilterText()) &&
		!ci_contains(driver_source, GetFilterText()) && !ci_contains(driver_manufacturer, GetFilterText()))
		return true;

	// Are there filters set on this folder?
	if ((dwMask & F_MASK) == 0)
		return false;

	// Filter out clones?
	if (dwMask & F_CLONES && DriverIsClone(nGame))
		return true;

	for (size_t i = 0; g_lpFilterList[i].m_dwFilterType; i++)
		if (dwMask & g_lpFilterList[i].m_dwFilterType)
			if (g_lpFilterList[i].m_pfnQuery(nGame) == g_lpFilterList[i].m_bExpectedResult)
				return true;

	return false;
}

/* Get the parent of game in this view */
bool GetParentFound(int nGame) // not used
{
	LPTREEFOLDER lpFolder = GetCurrentFolder();

	if( lpFolder )
	{
		int nParentIndex = GetParentIndex(&driver_list::driver(nGame));

		/* return false if no parent is there in this view */
		if( nParentIndex == -1)
			return false;

		/* return false if the folder should be HIDDEN in this view */
		if (TestBit(lpFolder->m_lpGameBits, nParentIndex) == 0)
			return false;

		/* return false if the game should be HIDDEN in this view */
		if (GameFiltered(nParentIndex, lpFolder->m_dwFlags))
			return false;

		return true;
	}

	return false;
}

LPCFILTER_ITEM GetFilterList(void)
{
	return g_lpFilterList;
}

/***************************************************************************
    private functions
 ***************************************************************************/

void CreateSourceFolders(int parent_index)
{
	int i, k=0;
	int nGames = driver_list::total();
	int start_folder = numFolders;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,false);
	for (size_t jj = 0; jj < nGames; jj++)
	{
		std::string driver_filename = GetDriverFilename_utf8(jj);

		if (driver_filename.empty())
			continue;

		// look for an existant source treefolder for this game
		// (likely to be the previous one, so start at the end)
		for (i=numFolders-1;i>=start_folder;i--)
		{
			if (!mui_strcmp(treeFolders[i]->m_lpTitle, driver_filename))
			{
				AddGame(treeFolders[i], jj);
				break;
			}
		}

		if (i == start_folder-1)
		{
			// nope, it's a source file we haven't seen before, make it.
			LPTREEFOLDER lpTemp = NewFolder(driver_filename, next_folder_id, parent_index, IDI_SOURCE, GetFolderFlags(numFolders));
			(void)NewExtraFolderData(driver_filename, next_folder_id, treeFolders[parent_index]->m_nFolderId, IDI_SOURCE);
			next_folder_id++;
			(void)AddFolder(lpTemp);
			AddGame(lpTemp, jj);
		}
	}
	SetNumOptionFolders(k-1);
	const char *fname = "Source";
	SaveExternalFolders(parent_index, fname);
}

void CreateScreenFolders(int parent_index)
{
	int i, k=0;
	int nGames = driver_list::total();
	int start_folder = numFolders;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,false);
	for (size_t jj = 0; jj < nGames; jj++)
	{
		int screens = DriverNumScreens(jj);
		char s[2];
		itoa(screens, s, 10);

		// look for an existant screens treefolder for this game
		// (likely to be the previous one, so start at the end)
		for (i=numFolders-1;i >= start_folder;i--)
		{
			if (!mui_strcmp(treeFolders[i]->m_lpTitle,s))
			{
				AddGame(treeFolders[i], jj);
				break;
			}
		}

		if (i == start_folder-1)
		{
			// nope, it's a screen file we haven't seen before, make it.
			LPTREEFOLDER lpTemp = NewFolder(s, next_folder_id, parent_index, IDI_SCREEN, GetFolderFlags(numFolders));
			(void)NewExtraFolderData(s, next_folder_id, treeFolders[parent_index]->m_nFolderId, IDI_SCREEN);
			next_folder_id++;
			(void)AddFolder(lpTemp);
			AddGame(lpTemp, jj);
		}
	}
	SetNumOptionFolders(k-1);
	const char *fname = "Screen";
	SaveExternalFolders(parent_index, fname);
}

void CreateManufacturerFolders(int parent_index)
{
	int i;
	const size_t nGames = driver_list::total();
	const size_t start_folder = numFolders;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,false);

	for (size_t jj = 0; jj < nGames; jj++)
	{
		const char *mfr_iter = 0;
		int iChars = 0;

		mfr_iter = driver_list::driver(jj).manufacturer;
		while(mfr_iter && mfr_iter[0] != '\0')
		{
			std::string parsed_ref = ParseManufacturer(mfr_iter, iChars);
			mfr_iter += iChars;
			//shift to next start char
			if(!parsed_ref.empty())
			{
				std::string trimmed_ref;
				
				trimmed_ref = TrimManufacturer(parsed_ref);
				for (i = numFolders - 1; i >= start_folder; i--)
				{
					//RS Made it case insensitive
					if (!mui_stricmp(treeFolders[i]->m_lpTitle, trimmed_ref))
					{
						AddGame(treeFolders[i], jj);
						break;
					}
				}

				if (i == start_folder - 1)
				{
					// nope, it's a manufacturer we haven't seen before, make it.
					LPTREEFOLDER lpTemp = NewFolder(trimmed_ref, next_folder_id, parent_index, IDI_MANUFACTURER, GetFolderFlags(numFolders));
					(void)NewExtraFolderData(parsed_ref, next_folder_id, treeFolders[parent_index]->m_nFolderId, IDI_MANUFACTURER);
					next_folder_id++;
					(void)AddFolder(lpTemp);
					AddGame(lpTemp, jj);
				}
			}
		}
	}
	const char *fname = "Manufacturer";
	SaveExternalFolders(parent_index, fname);
}

/* Make a reasonable name out of the one found in the driver array */
std::string ParseManufacturer(std::string_view input, int& parsedChars)
{
	parsedChars = 0;

	if (input.empty() || input.front() == '?' || input.front() == '<' || (input.size() > 3 && input[3] == '?'))
	{
		parsedChars = static_cast<int>(input.size());
		return "<unknown>";
	}

	if (input.front() == ' ')
	{
		input.remove_prefix(1);
		++parsedChars;
	}

	size_t i = 0;
	size_t len = input.length();
	std::string result;

	while (i < len)
	{
		char c = input[i];
		char next = (i + 1 < len) ? input[i + 1] : '\0';
		char next2 = (i + 2 < len) ? input[i + 2] : '\0';
		char next3 = (i + 3 < len) ? input[i + 3] : '\0';

		// Break conditions
		if ((c == ' ' && (next == '(' || next == '/' || next == '+')) || c == ']' || c == '/' || c == '?')
		{
			++parsedChars;
			if (next == '/' || next == '+') ++parsedChars;
			break;
		}

		if (c == ' ' && next == '?')
		{
			i += 2;
			parsedChars += 2;
			continue;
		}

		if (c != '[')
			result.push_back(c);

		++parsedChars;

		if (next == ',' && next2 == ' ' && (next3 == 's' || next3 == 'd'))
		{
			++i;
			break;
		}

		++i;
	}

	// Post-cleanup (remove prefix/suffix or special cases)
	auto view = std::string_view(result);

	if (!view.empty() && (view.front() == '(' || view.front() == ','))
		view.remove_prefix(1);
	if (!view.empty() && view.back() == ')')
		view.remove_suffix(1);

	static constexpr std::pair<std::string_view, size_t> prefixes[] = {
		{"licensed from ", 14},
		{"licenced from ", 14},
		{" supported by", 13},
		{" distributed by", 15}
	};

	for (const auto& [prefix, len] : prefixes)
	{
		if (view.substr(0, len) == prefix)
		{
			view.remove_prefix(len);
			break;
		}
	}

	size_t license_pos = view.find(" license");
	if (license_pos == std::string_view::npos)
		license_pos = view.find(" licence");

	if (license_pos != std::string_view::npos)
		view = view.substr(0, license_pos);

	return std::string(view);
}

/* Analyze Manufacturer Names for typical patterns, that don't distinguish between companies (e.g. Co., Ltd., Inc., etc. */
static std::string TrimManufacturer(std::string_view manufacturer_string)
{
	//Also remove Country specific suffixes (e.g. Japan, Italy, America, USA, ...)
	std::string trimmed_string;
	constexpr std::string_view string_suffix[] =
	{
		" INDUSTRIES JAPAN"sv,
		" ENTERPRISES, LTD"sv,
		" ENTERPRISES INC."sv,
		" IND. CO., LTD."sv,
		" GMBH & CO. KG"sv,
		" FRANCE S.A."sv,
		" ENTERPRISES"sv,
		" CORPORATION"sv,
		" OF AMERICA"sv,
		" INDUSTRIES"sv,
		" S.L. SPAIN"sv,
		" USA, INC."sv,
		" GMBH & CO"sv,
		" CO., LTD."sv,
		" CO., INC."sv,
		" USA, INC"sv,
		" UK, LTD."sv,
		" CO.,LTD."sv,
		" CO., LTD"sv,
		" CO. LTD."sv,
		" CO, LTD."sv,
		", S.R.L."sv,
		" ENGLAND"sv,
		" CO-LTD."sv,
		" CO.LTD."sv,
		" CO.,LTD"sv,
		" CO. LTD"sv,
		" CO LTD."sv,
		" AUSTRIA"sv,
		" AMERICA"sv,
		",S.R.L."sv,
		" S.R.L."sv,
		" S. L."sv,
		", LTD."sv,
		", INC."sv,
		" KOREA"sv,
		" JAPAN"sv,
		" ITALY"sv,
		" GAMES"sv,
		" CORP."sv,
		",INC."sv,
		", LTD"sv,
		", INC"sv,
		" S.L."sv,
		" S.A."sv,
		" P.L."sv,
		" LTD."sv,
		" I.S."sv,
		" INT."sv,
		" INC."sv,
		" GMBH"sv,
		" GAME"sv,
		" CORP"sv,
		" B.V."sv,
		" USA"sv,
		" SRL"sv,
		" LTD"sv,
		" INC"sv,
		" CO."sv,
		" UK"sv,
		" SL"sv,
		" SA"sv,
		" PL"sv,
		" KG"sv,
		" CO"sv,
		" AG"sv,
		" AB"sv
	};

	//start analyzing from the back, as these are usually suffixes
	for (const std::string_view &current_suffix : string_suffix)
	{
		const std::string_view::size_type trim_pos = manufacturer_string.size() - current_suffix.size();
		if (!mui_stricmp(&manufacturer_string[trim_pos],current_suffix))
		{
			manufacturer_string.remove_suffix(current_suffix.size());
			break;
		}
	}

	trimmed_string = manufacturer_string;

	return trimmed_string;
}

LPEXFOLDERDATA NewExtraFolderData(std::string title, int folderId, int parentId, int iconId, int subIconId, DWORD dwFlags)
{
	auto* extra = new EXFOLDERDATA;
	extra->m_szTitle = title;
	extra->m_nFolderId = folderId;
	extra->m_nParent = parentId;
	extra->m_dwFlags = dwFlags;
	extra->m_nIconId = iconId;
	extra->m_nSubIconId = subIconId;
	return extra;
}

void CreateBIOSFolders(int parent_index)
{
	int i, nGames = driver_list::total();
	int start_folder = numFolders;
	const game_driver *drv;
	int nParentIndex = -1;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits, false);

	for (size_t jj = 0; jj < nGames; jj++)
	{
		if ( DriverIsClone(jj) )
		{
			nParentIndex = GetParentIndex(&driver_list::driver(jj));
			if (nParentIndex < 0) return;
			drv = &driver_list::driver(nParentIndex);
		}
		else
			drv = &driver_list::driver(jj);
		nParentIndex = GetParentIndex(drv);

		if (nParentIndex < 0 || !driver_list::driver(nParentIndex).type.fullname())
			continue;

		for (i = numFolders-1; i >= start_folder; i--)
		{
			if (!mui_strcmp(treeFolders[i]->m_lpTitle, driver_list::driver(nParentIndex).type.fullname()))
			{
				AddGame(treeFolders[i], jj);
				break;
			}
		}

		if (i == start_folder-1)
		{
			LPTREEFOLDER lpTemp = NewFolder(driver_list::driver(nParentIndex).type.fullname(), next_folder_id++, parent_index, IDI_CPU, GetFolderFlags(numFolders));
			(void)AddFolder(lpTemp);
			AddGame(lpTemp, jj);
		}
	}
	const char *fname = "BIOS";
	SaveExternalFolders(parent_index, fname);
}

void CreateCPUFolders(int parent_index)
{
	int device_folder_count = 0;
	LPTREEFOLDER device_folders[1024];
	LPTREEFOLDER folder;
	int nFolder = numFolders;

	for (size_t i = 0; i < driver_list::total(); i++)
	{
		machine_config config(driver_list::driver(i), emu_opts.GetGlobalOpts());

		// enumerate through all devices
		for (device_execute_interface &device : execute_interface_enumerator(config.root_device()))
		{
			// get the name
			const char* dev_name = device.device().name();

			if (dev_name) // skip null
			{
				// do we have a folder for this device?
				folder = nullptr;
				for (size_t j = 0; j < device_folder_count; j++)
				{
					if (!mui_strcmp(dev_name, device_folders[j]->m_lpTitle))
					{
						folder = device_folders[j];
						break;
					}
				}

				// are we forced to create a folder?
				if (folder == nullptr)
				{
					LPTREEFOLDER lpTemp = NewFolder(dev_name, next_folder_id, parent_index, IDI_CPU, GetFolderFlags(numFolders));

					(void)NewExtraFolderData(dev_name, next_folder_id, treeFolders[parent_index]->m_nFolderId, IDI_CPU);
					next_folder_id++;
					(void)AddFolder(lpTemp);
					folder = treeFolders[nFolder++];

					// record that we found this folder
					device_folders[device_folder_count++] = folder;
					if (device_folder_count >= std::size(device_folders))
						std::cout << "CreateCPUFolders buffer overrun: " << device_folder_count << std::endl;
				}

				// cpu type #'s are one-based
				AddGame(folder, i);
			}
		}
	}
	const char *fname = "CPU";
	SaveExternalFolders(parent_index, fname);
}

void CreateSoundFolders(int parent_index)
{
	int device_folder_count = 0;
	LPTREEFOLDER device_folders[512];
	LPTREEFOLDER folder;
	int nFolder = numFolders;

	for (size_t i = 0; i < driver_list::total(); i++)
	{
		machine_config config(driver_list::driver(i), emu_opts.GetGlobalOpts());

		// enumerate through all devices

		for (device_sound_interface &device : sound_interface_enumerator(config.root_device()))
		{
			// get the name
			const char* dev_name = device.device().name();

			// do we have a folder for this device?
			if (dev_name)
			{
				folder = nullptr;
				for (size_t j = 0; j < device_folder_count; j++)
				{
					if (!mui_strcmp(dev_name, device_folders[j]->m_lpTitle))
					{
						folder = device_folders[j];
						break;
					}
				}

				// are we forced to create a folder?
				if (folder == nullptr)
				{
					LPTREEFOLDER lpTemp = NewFolder(dev_name, next_folder_id, parent_index, IDI_SOUND, GetFolderFlags(numFolders));
					(void)NewExtraFolderData(dev_name, next_folder_id, treeFolders[parent_index]->m_nFolderId, IDI_SOUND);
					next_folder_id++;
					(void)AddFolder(lpTemp);
					folder = treeFolders[nFolder++];

					// record that we found this folder
					device_folders[device_folder_count++] = folder;
					if (device_folder_count >= std::size(device_folders))
						std::cout << "CreateSoundFolders buffer overrun: " << device_folder_count << std::endl;
				}

				// cpu type #'s are one-based
				AddGame(folder, i);
			}
		}
	}
	const char *fname = "Sound";
	SaveExternalFolders(parent_index, fname);
}

void CreateDeficiencyFolders(int parent_index)
{
	int nGames = driver_list::total();
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// set up the deficiency folders
	struct DeficiencyFolderSpec
	{
		const char* title;
		int cacheBit;
		LPTREEFOLDER folder;
	} specs[] =
	{
		{ "Wrong Colors",      21 },
		{ "Unemulated Protection", 22 },
		{ "Imperfect Colors",  20 },
		{ "Imperfect Graphics",18 },
		{ "Missing Sound",     17 },
		{ "Imperfect Sound",   16 },
		{ "No Cocktail",        8 },
		{ "Requires Artwork",  10 }
	};

	// create all folders
	for (auto& spec : specs)
	{
		spec.folder = NewFolder(spec.title, next_folder_id, parent_index, IDI_FOLDER, GetFolderFlags(numFolders));
		(void)NewExtraFolderData(spec.title, next_folder_id, treeFolders[parent_index]->m_nFolderId, IDI_FOLDER);
		AddFolder(spec.folder);
		++next_folder_id;
	}

	// Clear parent folder bits
	SetAllBits(lpFolder->m_lpGameBits, false);

	// Clear all bits in the subfolders
	for (int jj = 0; jj < nGames; ++jj)
	{
		uint32_t cache = GetDriverCacheLower(jj);
		for (const auto& spec : specs)
		{
			if (BIT(cache, spec.cacheBit))
				AddGame(spec.folder, jj);
		}
	}
}

void CreateDumpingFolders(int parent_index)
{
	bool bBadDump  = false;
	bool bNoDump = false;
	int nGames = driver_list::total();
	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	const rom_entry *rom;
	const game_driver *gamedrv;
	const char *title;

	// create our two subfolders
	LPTREEFOLDER lpBad, lpNo;

	title = "Bad Dump";
	lpBad = NewFolder(title, next_folder_id, parent_index, IDI_FOLDER_DUMP, GetFolderFlags(numFolders));
	(void)NewExtraFolderData(title, next_folder_id, treeFolders[parent_index]->m_nFolderId, IDI_FOLDER_DUMP);
	next_folder_id++;
	(void)AddFolder(lpBad);

	title = "No Dump";
	lpNo = NewFolder(title, next_folder_id, parent_index, IDI_FOLDER_DUMP, GetFolderFlags(numFolders));
	(void)NewExtraFolderData(title, next_folder_id, treeFolders[parent_index]->m_nFolderId, IDI_FOLDER_DUMP);
	next_folder_id++;
	(void)AddFolder(lpNo);

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits,false);

	for (size_t jj = 0; jj < nGames; jj++)
	{
		gamedrv = &driver_list::driver(jj);

		if (!gamedrv->rom)
			continue;
		bBadDump = false;
		bNoDump = false;
		/* Allocate machine config */
		machine_config config(*gamedrv, emu_opts.GetGlobalOpts());

		for (device_t &device : device_enumerator(config.root_device()))
		{
			for (const rom_entry *region = rom_first_region(device); region; region = rom_next_region(region))
			{
				for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
				{
					if (ROMREGION_ISROMDATA(region) || ROMREGION_ISDISKDATA(region) )
					{
						//name = ROM_GETNAME(rom);
						util::hash_collection hashes(rom->hashdata());
						if (hashes.flag(util::hash_collection::FLAG_BAD_DUMP))
							bBadDump = true;
						if (hashes.flag(util::hash_collection::FLAG_NO_DUMP))
							bNoDump = true;
					}
				}
			}
		}
		if (bBadDump)
			AddGame(lpBad,jj);

		if (bNoDump)
			AddGame(lpNo,jj);
	}
	const char *fname = "Dumping";
	SaveExternalFolders(parent_index, fname);
}


void CreateYearFolders(int parent_index)
{
	int i,jj;
	int nGames = driver_list::total();
	int start_folder = numFolders;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits, false);

	for (jj = 0; jj < nGames; jj++)
	{
		std::size_t year_end, year_size;

		year_end = mui_strlen(driver_list::driver(jj).year);
		if (year_end == MAX_PATH) // skip invalid entry
			continue;

		year_size = year_end + 1;
		std::unique_ptr<char[]> s(new char [year_size]);
		(void)mui_strcpy(s.get(), driver_list::driver(jj).year);

		if (s[0] == '\0' || s[0] == '?')
			continue;

		if (s[year_end] == '?')
			s[year_end] = '\0';

		// look for an extant year treefolder for this game
		// (likely to be the previous one, so start at the end)
		for (i=numFolders-1;i>=start_folder;i--)
		{
			if (mui_strncmp(treeFolders[i]->m_lpTitle, s.get(), year_size) == 0)
			{
				AddGame(treeFolders[i], jj);
				break;
			}
		}
		if (i == start_folder-1)
		{
			// nope, it's a year we haven't seen before, make it.
			LPTREEFOLDER lpTemp;
			lpTemp = NewFolder(s.get(), next_folder_id, parent_index, IDI_YEAR, GetFolderFlags(numFolders));
			(void)NewExtraFolderData(s.get(), next_folder_id, treeFolders[parent_index]->m_nFolderId, IDI_YEAR);
			next_folder_id++;
			(void)AddFolder(lpTemp);
			AddGame(lpTemp, jj);
		}
	}
	const char *fname = "Year";
	SaveExternalFolders(parent_index, fname);
}

void CreateResolutionFolders(int parent_index)
{
	int i,jj;
	int nGames = driver_list::total();
	int start_folder = numFolders;
	std::string folder_title;
	const game_driver *gamedrv;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits, false);

	for (jj = 0; jj < nGames; jj++)
	{
		gamedrv = &driver_list::driver(jj);
		/* Allocate machine config */
		machine_config config(*gamedrv, emu_opts.GetGlobalOpts());

		if (isDriverVector(&config))
			folder_title = "Vector";
		else
		{
			screen_device_enumerator iter(config.root_device());
			const screen_device *screen = iter.first();
			if (screen == nullptr)
				folder_title = "Screenless Game";
			else
			{
				for (screen_device &screen : screen_device_enumerator(config.root_device()))
				{
					const rectangle &visarea = screen.visible_area();
					int horizontal_size = visarea.max_x - visarea.min_x + 1,
						  vertical_size = visarea.max_y - visarea.min_y + 1;
					std::ostringstream oss;

					oss << vertical_size << " x "<< horizontal_size <<" (" << ((driver_list::driver(jj).flags & ORIENTATION_SWAP_XY) ? "V" : "H") << ")";
					folder_title = oss.str();

					// look for an existant screen treefolder for this game
					// (likely to be the previous one, so start at the end)
					for (i=numFolders-1;i>=start_folder;i--)
					{
						if (!mui_strcmp(treeFolders[i]->m_lpTitle, folder_title.c_str()))
						{
							AddGame(treeFolders[i],jj);
							break;
						}
					}

					if (i == start_folder-1)
					{
						// nope, it's a screen we haven't seen before, make it.
						LPTREEFOLDER lpTemp = NewFolder(folder_title.c_str(), next_folder_id++, parent_index, IDI_SCREEN, GetFolderFlags(numFolders));
						(void)NewExtraFolderData(folder_title.c_str(), next_folder_id, treeFolders[parent_index]->m_nFolderId, IDI_SCREEN);
						next_folder_id++;
						(void)AddFolder(lpTemp);
						AddGame(lpTemp,jj);
					}
				}
			}
		}
	}
	const char *fname = "Resolution";
	SaveExternalFolders(parent_index, fname);
}

void CreateFPSFolders(int parent_index)
{
	int i,jj;
	int nGames = driver_list::total();
	int start_folder = numFolders;
	std::string folder_title;
	const game_driver *gamedrv;
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	SetAllBits(lpFolder->m_lpGameBits, false);

	for (jj = 0; jj < nGames; jj++)
	{
		gamedrv = &driver_list::driver(jj);
		/* Allocate machine config */
		machine_config config(*gamedrv, emu_opts.GetGlobalOpts());

		if (isDriverVector(&config))
			folder_title = "Vector";
		else
		{
			screen_device_enumerator iter(config.root_device());
			const screen_device *screen = iter.first();
			if (screen == nullptr)
				folder_title = "Screenless Game";
			else
			{
				for (screen_device &screen : screen_device_enumerator(config.root_device()))
				{
                    std::ostringstream oss;

					oss << ATTOSECONDS_TO_HZ(screen.refresh_attoseconds()) << " Hz";
					folder_title = oss.str();

					// look for an existant screen treefolder for this game
					// (likely to be the previous one, so start at the end)
					for (i=numFolders-1;i>=start_folder;i--)
					{
						if (!mui_strcmp(treeFolders[i]->m_lpTitle, folder_title.c_str()))
						{
							AddGame(treeFolders[i],jj);
							break;
						}
					}
					if (i == start_folder-1)
					{
						// nope, it's a screen we haven't seen before, make it.
						LPTREEFOLDER lpTemp = NewFolder(folder_title.c_str(), next_folder_id++, parent_index, IDI_SCREEN, GetFolderFlags(numFolders));
						(void)NewExtraFolderData(folder_title.c_str(), next_folder_id, treeFolders[parent_index]->m_nFolderId, IDI_SCREEN);
						next_folder_id++;
						(void)AddFolder(lpTemp);
						AddGame(lpTemp,jj);
					}
				}
			}
		}
	}
	const char *fname = "Refresh";
	SaveExternalFolders(parent_index, fname);
}


#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

// adds these folders to the treeview
void ResetTreeViewFolders(void)
{
	HWND hTreeView = GetTreeView();

	// currently "cached" parent
	HTREEITEM shti, hti_parent = nullptr;
	int index_parent = -1;

	(void)tree_view::delete_all_items(hTreeView);

	//printf("Adding folders to tree ui indices %i to %i\n",start_index,end_index);

	TVINSERTSTRUCTW tvs{};
	tvs.hInsertAfter = TVI_SORT;

	TVITEMW tvi{};
	for (size_t i=0; i<numFolders; i++)
	{
		LPTREEFOLDER lpFolder = treeFolders[i];

		if (lpFolder->m_nParent == -1)
		{
			if (lpFolder->m_nFolderId < MAX_FOLDERS)
				// it's a built in folder, let's see if we should show it
				if (GetShowFolder(lpFolder->m_nFolderId) == false)
					continue;

			tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			tvs.hParent = TVI_ROOT;
			tvi.pszText = const_cast<wchar_t*>(lpFolder->m_lpwTitle.c_str());
			tvi.lParam = (LPARAM)lpFolder;
			tvi.iImage = GetTreeViewIconIndex(lpFolder->m_nIconId);
			tvi.iSelectedImage = 0;
			tvs.item = tvi;

			// Add root branch
			hti_parent = tree_view::insert_item(hTreeView, &tvs);

			continue;
		}

		// not a top level branch, so look for parent
		if (treeFolders[i]->m_nParent != index_parent)
		{

			hti_parent = tree_view::get_root(hTreeView);
			while (true)
			{
				if (hti_parent == nullptr)
					// couldn't find parent folder, so it's a built-in but
					// not shown folder
					break;

				tvi.hItem = hti_parent;
				tvi.mask = TVIF_PARAM;
				(void)tree_view::get_item(hTreeView,&tvi);
				if (((LPTREEFOLDER)tvi.lParam) == treeFolders[treeFolders[i]->m_nParent])
					break;

				hti_parent = tree_view::get_next_sibling(hTreeView,hti_parent);
			}

			// if parent is not shown, then don't show the child either obviously!
			if (hti_parent == nullptr)
				continue;

			index_parent = treeFolders[i]->m_nParent;
		}

		tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvs.hParent = hti_parent;
		tvi.iImage = GetTreeViewIconIndex(treeFolders[i]->m_nIconId);
		tvi.iSelectedImage = 0;
		tvi.pszText = const_cast<wchar_t*>(lpFolder->m_lpwTitle.c_str());
		tvi.lParam = (LPARAM)treeFolders[i];
		tvs.item = tvi;

		// Add it to this tree branch
		shti = tree_view::insert_item(hTreeView, &tvs); // for current child branches
	}
}
#pragma GCC diagnostic error "-Wunused-but-set-variable"

void SelectTreeViewFolder(int folder_id)
{
	HWND hTreeView = GetTreeView();
	HTREEITEM hti = tree_view::get_root(hTreeView);
	TVITEMW tvi{ TVIF_PARAM };

	while (hti != nullptr)
	{
		HTREEITEM hti_next;

		tvi.hItem = hti;
		(void)tree_view::get_item(hTreeView,&tvi);

		if (((LPTREEFOLDER)tvi.lParam)->m_nFolderId == folder_id)
		{
			(void)tree_view::select_item(hTreeView,tvi.hItem);
			SetCurrentFolder((LPTREEFOLDER)tvi.lParam);
			return;
		}

		hti_next = tree_view::get_child(hTreeView,hti);
		if (hti_next == nullptr)
		{
			hti_next = tree_view::get_next_sibling(hTreeView,hti);
			if (hti_next == nullptr)
			{
				hti_next = tree_view::get_parent(hTreeView,hti);
				if (hti_next != nullptr)
					hti_next = tree_view::get_next_sibling(hTreeView,hti_next);
			}
		}
		hti = hti_next;
	}

	// could not find folder to select
	// make sure we select something
	tvi.hItem = tree_view::get_root(hTreeView);
	(void)tree_view::get_item(hTreeView,&tvi);
	(void)tree_view::select_item(hTreeView,tvi.hItem);
	SetCurrentFolder((LPTREEFOLDER)tvi.lParam);

}

/*
 * Does this folder have an INI associated with it?
 * Currently only true for FOLDER_VECTOR and children
 * of FOLDER_SOURCE.
 */
static bool FolderHasIni(LPTREEFOLDER lpFolder)
{
	LPCFOLDERDATA data = FindFilter(lpFolder->m_nFolderId);
	if (data)
		if (data->m_soft_type_opt < TOTAL_SOFTWARETYPE_OPTIONS)
			return true;

	if (lpFolder->m_nParent != -1 && FOLDER_SOURCE == treeFolders[lpFolder->m_nParent]->m_nFolderId)
		return true;

	return false;
}

/* Add a folder to the list.  Does not allocate */
static bool AddFolder(LPTREEFOLDER lpFolder)
{
	LPTREEFOLDER *tmpTree = nullptr;
	UINT oldFolderArrayLength = folderArrayLength;
	if (numFolders + 1 >= folderArrayLength)
	{
		folderArrayLength += 500;
		tmpTree = new LPTREEFOLDER[folderArrayLength];
		(void)std::copy_n(treeFolders, oldFolderArrayLength, tmpTree);
		if (treeFolders)
			delete[] treeFolders;
		treeFolders = tmpTree;
	}

	/* Is there an folder.ini that can be edited? */
	if (FolderHasIni(lpFolder))
		lpFolder->m_dwFlags |= F_INIEDIT;

	treeFolders[numFolders] = lpFolder;
	numFolders++;

	return true;
}

/* Allocate and initialize a NEW TREEFOLDER */
static LPTREEFOLDER NewFolder(std::string lpTitle, UINT nFolderId, int nParent, UINT nIconId, DWORD dwFlags)
{
	LPTREEFOLDER lpFolder = new TREEFOLDER();
	lpFolder->m_lpTitle = lpTitle;
	lpFolder->m_lpwTitle = mui_utf16_from_utf8string(lpFolder->m_lpTitle);
	lpFolder->m_lpGameBits = NewBits(driver_list::total());
	lpFolder->m_nFolderId = nFolderId;
	lpFolder->m_nParent = nParent;
	lpFolder->m_nIconId = nIconId;
	lpFolder->m_dwFlags = dwFlags;

	return lpFolder;
}

/* Deallocate the passed in LPTREEFOLDER */
static void DeleteFolder(LPTREEFOLDER &lpFolder)
{
	if (lpFolder)
	{
		if (lpFolder->m_lpGameBits)
			DeleteBits(lpFolder->m_lpGameBits);

		delete lpFolder;
		lpFolder = nullptr;
	}
}

/* Can be called to re-initialize the array of treeFolders */
bool InitFolders(void)
{
	if (treeFolders != 0)
	{		
		for (int i = 0; i < numFolders; i++)
			DeleteFolder(treeFolders[i]);

		delete[] treeFolders;
		treeFolders = nullptr;
		numFolders = 0;
	}

	if (folderArrayLength == 0)
	{
		folderArrayLength = 200;
		treeFolders = new LPTREEFOLDER[folderArrayLength];
		if (!treeFolders)
		{
			folderArrayLength = 0;
			return false;
		}
	}

	// built-in top level folders
	for (size_t i = 0; !g_lpFolderData[i].m_lpTitle.empty(); i++)
	{
		if (RequiredDriverCache() || (!RequiredDriverCache() && !g_lpFolderData[i].m_process))
		{
			LPCFOLDERDATA fData = &g_lpFolderData[i];
			/* get the saved folder flags */
			DWORD dwFolderFlags = GetFolderFlags(numFolders);
			/* create the folder */
			(void)AddFolder(NewFolder(fData->m_lpTitle.c_str(), fData->m_nFolderId, -1, fData->m_nIconId, dwFolderFlags));
		}
	}

	numExtraFolders = InitExtraFolders();

	for (size_t i = 0; i < numExtraFolders; i++)
	{
		LPEXFOLDERDATA fExData = ExtraFolderData[i];
		// OR in the saved folder flags
		DWORD dwFolderFlags = fExData->m_dwFlags | GetFolderFlags(numFolders);
		// create the folder, but if we are building the cache, the name must not be a pre-built one
		if (RequiredDriverCache())
		{
			bool folder_exists = false;
			for (size_t ii = 0; !g_lpFolderData[ii].m_lpTitle.empty(); ii++)
			{
				folder_exists = !mui_strcmp(fExData->m_szTitle, g_lpFolderData[ii].m_lpTitle);
				if (folder_exists)
					break;
			}
			if (folder_exists)
				continue;
		}

		(void)AddFolder(NewFolder(fExData->m_szTitle.c_str(), fExData->m_nFolderId, fExData->m_nParent, fExData->m_nIconId, dwFolderFlags));
	}

// creates child folders of all the top level folders, including custom ones
	int num_top_level_folders = numFolders;

	for (size_t i = 0; i < num_top_level_folders; i++)
	{
		LPTREEFOLDER lpFolder = treeFolders[i];
		LPCFOLDERDATA lpFolderData = nullptr;

		for (size_t ii = 0; !g_lpFolderData[ii].m_lpTitle.empty(); ii++)
		{
			if (g_lpFolderData[ii].m_nFolderId == lpFolder->m_nFolderId)
			{
				lpFolderData = &g_lpFolderData[ii];
				break;
			}
		}

		if (lpFolderData)
		{
			if (lpFolderData->m_pfnCreateFolders)
			{
				if (RequiredDriverCache() && lpFolderData->m_process) // rebuild cache
					lpFolderData->m_pfnCreateFolders(i);
				else
				if (!lpFolderData->m_process) // build every time (CreateDeficiencyFolders)
					lpFolderData->m_pfnCreateFolders(i);
			}
		}
		else
		{
			if ((lpFolder->m_dwFlags & F_CUSTOM) == 0)
			{
				std::cout << "Internal inconsistency with non-built-in folder, but not custom" << std::endl;
				continue;
			}

			// load the extra folder files, which also adds children
			if (TryAddExtraFolderAndChildren(i) == false)
				lpFolder->m_nFolderId = FOLDER_NONE;
		}
	}

	CreateTreeIcons();
	ResetWhichGamesInFolders();
	ResetTreeViewFolders();
	SelectTreeViewFolder(GetSavedFolderID());
	LoadFolderFlags();
	return true;
}

// create iconlist and Treeview control
static bool CreateTreeIcons()
{
	HICON hIcon;
	INT i;
	HINSTANCE hInst = system_services::get_module_handle(0);

	int numIcons = ICON_MAX + numExtraIcons;
	hTreeSmall = ImageList_Create(16, 16, ILC_COLORDDB | ILC_MASK, numIcons, numIcons);

	//printf("Trying to load %i normal icons\n",ICON_MAX);
	for (i = 0; i < ICON_MAX; i++)
	{
		hIcon = LoadIconFromFile(treeIconNames[i].lpName);
		if (!hIcon)
			hIcon = menus::load_icon(hInst, menus::make_int_resource(treeIconNames[i].nResourceID));

		if (image_list::add_icon(hTreeSmall, hIcon) == -1)
		{
			ErrorMsg("Error creating icon on regular folder, %i %i",i,hIcon != nullptr);
			return false;
		}
	}

	//printf("Trying to load %i extra custom-folder icons\n",numExtraIcons);
	for (i = 0; i < numExtraIcons; i++)
	{
		if ((hIcon = LoadIconFromFile(ExtraFolderIcons[i].c_str())) == 0)
			hIcon = LoadIconW (hInst, menus::make_int_resource(IDI_FOLDER));

		if (image_list::add_icon(hTreeSmall, hIcon) == -1)
		{
			ErrorMsg("Error creating icon on extra folder, %i %i",i,hIcon != nullptr);
			return false;
		}
	}

	// Be sure that all the small icons were added.
	if (ImageList_GetImageCount(hTreeSmall) < numIcons)
	{
		ErrorMsg("Error with icon list--too few images.  %i %i", ImageList_GetImageCount(hTreeSmall),numIcons);
		return false;
	}

	// Be sure that all the small icons were added.

	if (ImageList_GetImageCount (hTreeSmall) < ICON_MAX)
	{
		ErrorMsg("Error with icon list--too few images.  %i < %i", ImageList_GetImageCount(hTreeSmall),(INT)ICON_MAX);
		return false;
	}

	// Associate the image lists with the list view control.
	(void)tree_view::set_image_list(GetTreeView(), hTreeSmall, TVSIL_NORMAL);

	return true;
}


static void TreeCtrlOnPaint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	RECT rcClip, rcClient;

	HBITMAP hBackground = GetBackgroundBitmap();
	MYBITMAPINFO *bmDesc = GetBackgroundInfo();

	HDC hDC = BeginPaint(hWnd, &ps);

	(void)GetClipBox(hDC, &rcClip);
	(void)windows::get_client_rect(hWnd, &rcClient);

	// Create a compatible memory DC
	HDC memDC = gdi::create_compatible_dc(hDC);

	// Select a compatible bitmap into the memory DC
	HBITMAP bitmap = CreateCompatibleBitmap(hDC, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
	HBITMAP hOldBitmap = (HBITMAP)gdi::select_object(memDC, bitmap);

	// First let the control do its default drawing.
	(void)windows::call_window_proc(g_lpTreeWndProc, hWnd, uMsg, (WPARAM)memDC, 0);

	// Draw bitmap in the background
	// Now create a mask
	HDC maskDC = gdi::create_compatible_dc(hDC);

	// Create monochrome bitmap for the mask
	HBITMAP maskBitmap = gdi::create_bitmap(rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, 1, 1, nullptr);

	HBITMAP hOldMaskBitmap = (HBITMAP)gdi::select_object(maskDC, maskBitmap);
	(void)gdi::set_bk_color(memDC, windows::get_sys_color(COLOR_WINDOW));

	// Create the mask from the memory DC
	(void)gdi::bit_blt(maskDC, 0, 0, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, memDC, rcClient.left, rcClient.top, SRCCOPY);

	HDC tempDC = gdi::create_compatible_dc(hDC);
	HBITMAP hOldHBitmap = (HBITMAP)gdi::select_object(tempDC, hBackground);

	HDC imageDC = gdi::create_compatible_dc(hDC);
	HBITMAP bmpImage = CreateCompatibleBitmap(hDC, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
	HBITMAP hOldBmpImage = (HBITMAP)gdi::select_object(imageDC, bmpImage);

	HPALETTE hPAL = GetBackgroundPalette();
	if (hPAL == nullptr)
		hPAL = gdi::create_half_tone_palette(hDC);

	if (gdi::get_device_caps(hDC, RASTERCAPS) & RC_PALETTE && hPAL != nullptr)
	{
		gdi::select_palette(hDC, hPAL, false);
		gdi::realize_palette(hDC);
		gdi::select_palette(imageDC, hPAL, false);
	}

	// Get x and y offset
	RECT rcRoot;
	tree_view::get_item_rect(hWnd, tree_view::get_root(hWnd), &rcRoot, false);
	rcRoot.left = -scroll_bar::get_scroll_pos(hWnd, SB_HORZ);

	// Draw bitmap in tiled manner to imageDC
	for (LONG i = rcRoot.left; i < rcClient.right; i += bmDesc->bmWidth)
		for (LONG j = rcRoot.top; j < rcClient.bottom; j += bmDesc->bmHeight)
			(void)gdi::bit_blt(imageDC,  i, j, bmDesc->bmWidth, bmDesc->bmHeight, tempDC, 0, 0, SRCCOPY);

	// Set the background in memDC to black. Using SRCPAINT with black and any other
	// color results in the other color, thus making black the transparent color
	(void)gdi::set_bk_color(memDC, RGB(0,0,0));
	(void)gdi::set_text_color(memDC, RGB(255,255,255));
	(void)gdi::bit_blt(memDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left, rcClip.bottom - rcClip.top, maskDC, rcClip.left, rcClip.top, SRCAND);

	// Set the foreground to black. See comment above.
	(void)gdi::set_bk_color(imageDC, RGB(255,255,255));
	(void)gdi::set_text_color(imageDC, RGB(0,0,0));
	(void)gdi::bit_blt(imageDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left, rcClip.bottom - rcClip.top, maskDC, rcClip.left, rcClip.top, SRCAND);

	// Combine the foreground with the background
	(void)gdi::bit_blt(imageDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left, rcClip.bottom - rcClip.top, memDC, rcClip.left, rcClip.top, SRCPAINT);

	// Draw the final image to the screen
	(void)gdi::bit_blt(hDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left, rcClip.bottom - rcClip.top, imageDC, rcClip.left, rcClip.top, SRCCOPY);

	(void)gdi::select_object(maskDC, hOldMaskBitmap);
	(void)gdi::select_object(tempDC, hOldHBitmap);
	(void)gdi::select_object(imageDC, hOldBmpImage);

	(void)gdi::delete_dc(maskDC);
	(void)gdi::delete_dc(imageDC);
	(void)gdi::delete_dc(tempDC);
	(void)gdi::delete_bitmap(bmpImage);
	(void)gdi::delete_bitmap(maskBitmap);

	if (GetBackgroundPalette() == nullptr)
	{
		(void)gdi::delete_palette(hPAL);
		hPAL = nullptr;
	}

	(void)gdi::select_object(memDC, hOldBitmap);
	(void)gdi::delete_bitmap(bitmap);
	(void)gdi::delete_dc(memDC);
	(void)EndPaint(hWnd, &ps);
	(void)gdi::release_dc(hWnd, hDC);
}

/* Header code - Directional Arrows */
static LRESULT CALLBACK TreeWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (GetBackgroundBitmap() != nullptr)
	{
		switch (uMsg)
		{
			case WM_MOUSEMOVE:
			{
				if (MouseHasBeenMoved())
					menus::show_cursor(true);
				break;
			}

			case WM_KEYDOWN :
				if (wParam == VK_F2)
				{
					if (lpCurrentFolder->m_dwFlags & F_CUSTOM)
					{
						tree_view::edit_label(hWnd,tree_view::get_selection(hWnd));
						return true;
					}
				}
				break;

			case WM_ERASEBKGND:
				return true;

			case WM_PAINT:
				TreeCtrlOnPaint(hWnd, uMsg, wParam, lParam);
				UpdateWindow(hWnd);
				break;
		}
	}

	/* message not handled */
	return windows::call_window_proc(g_lpTreeWndProc, hWnd, uMsg, wParam, lParam);
}

/*
 * Filter code
 * Added 01/09/99 - MSH <mhaaland@hypertech.com>
 */

/* find a FOLDERDATA by folderID */
LPCFOLDERDATA FindFilter(DWORD folderID)
{
	for (size_t i = 0; !g_lpFolderData[i].m_lpTitle.empty(); i++)
		if (g_lpFolderData[i].m_nFolderId == folderID)
			return &g_lpFolderData[i];

	return nullptr;
}

LPTREEFOLDER GetFolderByName(int nParentId, const char *pszFolderName)
{
	//First Get the Parent TreeviewItem
	//Enumerate Children
	for(int i = 0; i < numFolders/* ||treeFolders[i] != nullptr*/; i++)
	{
		if (!mui_strcmp(treeFolders[i]->m_lpTitle, pszFolderName))
		{
			int nParent = treeFolders[i]->m_nParent;
			if ((nParent >= 0) && treeFolders[nParent]->m_nFolderId == nParentId)
				return treeFolders[i];
		}
	}
	return nullptr;
}

static int InitExtraFolders(void)
{
	int count = 0;
	std::string ini_path = emu_opts.dir_get_value(DIRPATH_CATEGORYINI_PATH);
	std::filesystem::path folder_path(ini_path);

	// Reset extra folder data array
	std::fill(std::begin(ExtraFolderData), std::end(ExtraFolderData), nullptr);
	numExtraIcons = 0;

	// Bail if directory doesn't exist
	if (!std::filesystem::exists(folder_path) || !std::filesystem::is_directory(folder_path))
		return 0;

	for (const auto& entry : std::filesystem::directory_iterator(folder_path))
	{
		if (!entry.is_regular_file() || entry.path().extension() != ".ini")
			continue;

		std::ifstream file_stream(entry.path());
		if (!file_stream.is_open())
			continue;

		int icon[2] = { 0, 0 };
		std::string line;

		while (std::getline(file_stream, line))
		{
			if (line == "[FOLDER_SETTINGS]")
			{
				while (std::getline(file_stream, line))
				{
					if (!line.empty() && line.front() == '[')
						break; // Next section begins

					stringtokenizer tokenizer(line, " =\t\r\n");
					auto cursor = tokenizer.begin();

					auto key = cursor.next_token();
					auto value = cursor.next_token();

					if (!key || !value)
						continue;

					if (*key == "RootFolderIcon")
						SetExtraIcons(*value, &icon[0]);
					else if (*key == "SubFolderIcon")
						SetExtraIcons(*value, &icon[1]);
				}
				break;
			}
		}

		file_stream.close();

		std::string stem = entry.path().stem().string();

		if (count >= MAX_EXTRA_FOLDERS)
			break;

		ExtraFolderData[count] = NewExtraFolderData(stem.c_str(), next_folder_id, -1, icon[0] ? -icon[0] : IDI_FOLDER, icon[1] ? -icon[1] : IDI_FOLDER, F_CUSTOM);
		if (ExtraFolderData[count])
		{
			next_folder_id++;
			count++;
		}
	}

	return count;
}

void FreeExtraFolders(void)
{
	for (size_t i = 0; i < numExtraFolders; i++)
	{
		if (ExtraFolderData[i])
		{
			delete ExtraFolderData[i];
			ExtraFolderData[i] = 0;
		}
	}
	numFolders -= numExtraFolders;
	numExtraFolders = 0;

	for (size_t i = 0; i < numExtraIcons; i++)
	{
		if (!ExtraFolderIcons[i].empty())
			ExtraFolderIcons[i].clear();
	}
	numExtraIcons = 0;
}


static void SetExtraIcons(std::string_view name, int* id)
{
	std::size_t find_pos;

	find_pos = name.find('.');

	if (find_pos != std::string_view::npos)
	{
		*id = ICON_MAX + numExtraIcons;
		ExtraFolderIcons[numExtraIcons] = name.substr(find_pos - 1);
		numExtraIcons++;
	}
}


// Called to add child folders of the top level extra folders already created
bool TryAddExtraFolderAndChildren(int parent_index)
{
	int current_id, id;
	LPTREEFOLDER lpFolder, lpTemp = 0;
	std::string read_ini_buffer;

	lpFolder = treeFolders[parent_index];

	current_id = lpFolder->m_nFolderId;
	id = lpFolder->m_nFolderId - MAX_FOLDERS;

	// "folder\title.ini"
	auto ini_path = std::filesystem::path(emu_opts.dir_get_value(DIRPATH_CATEGORYINI_PATH)) / (std::string(ExtraFolderData[id]->m_szTitle) + ".ini");

	std::ifstream file_stream(ini_path.string());
	if (!file_stream.is_open())
		return false;

	while (std::getline(file_stream, read_ini_buffer))
	{
		std::string name;

		// Handle [Section] headers
		if (!read_ini_buffer.empty() && read_ini_buffer.front() == '[')
		{
			std::size_t start_pos = 1;
			std::size_t end_pos = read_ini_buffer.find(']', start_pos);
			if (end_pos == std::string::npos)
				continue;

			name = read_ini_buffer.substr(start_pos, end_pos - start_pos);
			if (name == "FOLDER_SETTINGS")
			{
				current_id = -1;
				continue;
			}
			else if (name == "ROOT_FOLDER") // is it [ROOT_FOLDER]?
			{
				current_id = lpFolder->m_nFolderId;
				lpTemp = lpFolder;
			}
			else
			{
				current_id = next_folder_id++;

				lpTemp = NewFolder(name.c_str(), current_id, parent_index,
					ExtraFolderData[id]->m_nSubIconId,
					GetFolderFlags(numFolders) | F_CUSTOM);

				ExtraFolderData[current_id] = NewExtraFolderData(
					name.c_str(), current_id - MAX_EXTRA_FOLDERS,
					ExtraFolderData[id]->m_nFolderId, -1, -1,
					ExtraFolderData[id]->m_dwFlags);

				(void)AddFolder(lpTemp);
			}
		}
		// Process lines within a valid folder section
		else if (current_id != -1)
		{
			stringtokenizer tokenizer(read_ini_buffer, " \t\r\n");
			auto cursor = tokenizer.begin();

			auto token = cursor.next_token();
			if (token && !token->empty())
			{
				name = *token;

				// IMPORTANT: This assumes that all driver names are lowercase!
				std::transform(name.begin(), name.end(), name.begin(), ::tolower); // Convert to lowercase

				if (!lpTemp)
				{
					ErrorMsg("Error parsing %s: missing [folder name] or [ROOT_FOLDER]",
						ini_path.string().c_str());
					current_id = lpFolder->m_nFolderId;
					lpTemp = lpFolder;
				}

				AddGame(lpTemp, GetGameNameIndex(name.c_str()));
			}
			else
				current_id = -1; // reset state if line is invalid
		}
	}

	return true;
}


void GetFolders(TREEFOLDER ***folders,int *num_folders)
{
	*folders = treeFolders;
	*num_folders = numFolders;
}

static bool TryRenameCustomFolderIni(LPTREEFOLDER lpFolder, std::string_view old_name, std::string_view new_name)
{
	try
	{
		std::filesystem::path ini_dirpath = emu_opts.get_ini_dir_utf8();
		std::filesystem::path ini_ext = ".ini";

		std::filesystem::path old_path = ini_dirpath;
		std::filesystem::path new_path = ini_dirpath;

		if (lpFolder->m_nParent >= 0)
		{
			LPTREEFOLDER lpParent = GetFolder(lpFolder->m_nParent);
			if (!lpParent)
				return false;

			old_path /= lpParent->m_lpTitle;
			old_path /= old_name;

			new_path /= lpParent->m_lpTitle;
			new_path /= new_name;
		}
		else
		{
			old_path /= old_name;
			new_path /= new_name;
		}

		std::filesystem::path old_ini = old_path;
		old_ini += ini_ext;

		std::filesystem::path new_ini = new_path;
		new_ini += ini_ext;

		if (std::filesystem::exists(old_ini))
		{
			std::filesystem::rename(old_ini, new_ini);
			return true;
		}
		if (std::filesystem::exists(old_path))
		{
			std::filesystem::rename(old_path, new_path);
			return true;
		}
	}
	catch (const std::filesystem::filesystem_error& fs_ex)
	{
		std::ostringstream error_message;
		error_message << "Error code: " << fs_ex.code() << ": " << fs_ex.what();

		auto utf8_name = mui_utf8_from_utf16string(MAMEUINAME);
		dialog_boxes::message_box_utf8(GetMainWindow(), error_message.str().c_str(), utf8_name.c_str(), MB_OK | MB_ICONERROR);
	}

	return false;
}

bool TryRenameCustomFolder(LPTREEFOLDER lpFolder, std::string_view new_name)
{
	bool result = false;
	const std::string ini_dirpath = emu_opts.dir_get_value(DIRPATH_CATEGORYINI_PATH);
	const std::string ini_ext = ".ini";

	if (lpFolder->m_nParent >= 0)
	{
		// Subfolder rename: Save under new title and rename associated files
		std::string old_title = lpFolder->m_lpTitle;

		lpFolder->m_lpTitle = std::string(new_name); // Convert view to owning string

		if (TrySaveExtraFolder(lpFolder))
		{
			result = true;
			TryRenameCustomFolderIni(lpFolder, old_title, new_name);
		}
		else
		{
			// Restore on failure
			lpFolder->m_lpTitle = old_title;
		}
	}
	else
	{
		// Parent folder rename: Rename file directly
		const std::filesystem::path old_path = std::filesystem::path(ini_dirpath) / (lpFolder->m_lpTitle + ini_ext);
		const std::filesystem::path new_path = std::filesystem::path(ini_dirpath) / (std::string(new_name) + ini_ext);

		try
		{
			std::filesystem::rename(old_path, new_path);
			result = true;
		}
		catch (const std::filesystem::filesystem_error& fs_ex)
		{
			std::ostringstream error_message;
			error_message << "Error code: " << fs_ex.code() << ": " << fs_ex.what();

			auto utf8_name = mui_utf8_from_utf16string(MAMEUINAME);
			dialog_boxes::message_box_utf8(GetMainWindow(), error_message.str().c_str(), utf8_name.c_str(), MB_OK | MB_ICONERROR);
		}

		if (result)
		{
			TryRenameCustomFolderIni(lpFolder, lpFolder->m_lpTitle, new_name);
			lpFolder->m_lpTitle = std::string(new_name);
		}
	}

	return result;
}

void AddToCustomFolder(LPTREEFOLDER lpFolder,int driver_index)
{
	if ((lpFolder->m_dwFlags & F_CUSTOM) == 0)
	{
		dialog_boxes::message_box(GetMainWindow(),L"Unable to add game to non-custom folder", &MAMEUINAME[0],MB_OK | MB_ICONERROR);
		return;
	}

	if (TestBit(lpFolder->m_lpGameBits,driver_index) == 0)
	{
		AddGame(lpFolder,driver_index);
		if (TrySaveExtraFolder(lpFolder) == false)
			RemoveGame(lpFolder,driver_index); // undo on error
	}
}

void RemoveFromCustomFolder(LPTREEFOLDER lpFolder,int driver_index)
{
	if ((lpFolder->m_dwFlags & F_CUSTOM) == 0)
	{
		dialog_boxes::message_box(GetMainWindow(),L"Unable to remove game from non-custom folder", &MAMEUINAME[0],MB_OK | MB_ICONERROR);
		return;
	}

	if (TestBit(lpFolder->m_lpGameBits,driver_index) != 0)
	{
		RemoveGame(lpFolder,driver_index);
		if (TrySaveExtraFolder(lpFolder) == false)
			AddGame(lpFolder,driver_index); // undo on error
	}
}

bool TrySaveExtraFolder(LPTREEFOLDER lpFolder)
{
	bool error = false;
	std::ofstream ofsSaveExtraFolder;
	std::filesystem::path filename_path;
	const std::string ini_path = emu_opts.dir_get_value(DIRPATH_CATEGORYINI_PATH);

	LPTREEFOLDER root_folder = 0;
	LPEXFOLDERDATA extra_folder = 0;

	for (size_t i = 0; i < numExtraFolders; i++)
	{
		if (ExtraFolderData[i]->m_nFolderId == lpFolder->m_nFolderId)
		{
			root_folder = lpFolder;
			extra_folder = ExtraFolderData[i];
			break;
		}

		if (lpFolder->m_nParent >= 0 &&
			ExtraFolderData[i]->m_nFolderId == treeFolders[lpFolder->m_nParent]->m_nFolderId)
		{
			root_folder = treeFolders[lpFolder->m_nParent];
			extra_folder = ExtraFolderData[i];
			break;
		}
	}

	if (extra_folder == 0 || root_folder == 0)
	{
		dialog_boxes::message_box(GetMainWindow(), L"Error finding custom file name to save", &MAMEUINAME[0], MB_OK | MB_ICONERROR);
		return false;
	}
	/* "folder\title.ini" */

	filename_path = std::filesystem::path(ini_path) / extra_folder->m_szTitle;

	ofsSaveExtraFolder = std::ofstream(filename_path);

	if (ofsSaveExtraFolder.is_open())
	{
		TREEFOLDER* folder_data;

		ofsSaveExtraFolder << "[FOLDER_SETTINGS]" << std::endl;
		// negative values for icons means it's custom, so save 'em
		if (extra_folder->m_nIconId < 0)
		{
			std::string_view rootfolder_ico = ExtraFolderIcons[(-extra_folder->m_nIconId) - ICON_MAX];
			ofsSaveExtraFolder << "RootFolderIcon " << rootfolder_ico << std::endl;
		}
		if (extra_folder->m_nSubIconId < 0)
		{
			std::string_view subfolder_ico = ExtraFolderIcons[(-extra_folder->m_nSubIconId) - ICON_MAX];
			ofsSaveExtraFolder << "SubFolderIcon " << subfolder_ico << std::endl;
		}

		/* need to loop over all our TREEFOLDERs--first the root one, then each child. Start with the root */

		folder_data = root_folder;

		ofsSaveExtraFolder << "\n[ROOT_FOLDER]" << std::endl;
		const size_t driver_total = driver_list::total();
		for (size_t i = 0; i < driver_total; i++)
		{
			if (TestBit(folder_data->m_lpGameBits, i))
				ofsSaveExtraFolder << driver_list::driver(i).name << std::endl;
		}

		/* look through the custom folders for ones with our root as parent */
		for (size_t i = 0; i < numFolders; i++)
		{
			folder_data = treeFolders[i];

			if (folder_data->m_nParent >= 0 &&
				treeFolders[folder_data->m_nParent] == root_folder)
			{
				ofsSaveExtraFolder << "\n[" << folder_data->m_lpTitle << "]" << std::endl;

				for (size_t ii = 0; ii < driver_total; ii++)
				{
					if (TestBit(folder_data->m_lpGameBits, ii))
						ofsSaveExtraFolder << driver_list::driver(ii).name << std::endl;
				}
			}
		}
	}

	error = !ofsSaveExtraFolder.good();
	if (error)
	{
		std::ostringstream oss;
		oss << "Error while saving custom file " << filename_path << std::endl;
		std::string utf8_mameuiname = mui_utf8_from_utf16string(MAMEUINAME);
		dialog_boxes::message_box_utf8(GetMainWindow(), oss.str().c_str(), utf8_mameuiname.c_str(), MB_OK | MB_ICONERROR);
	}

	ofsSaveExtraFolder.close();

	return !error;
}

HIMAGELIST GetTreeViewIconList(void)
{
	return hTreeSmall;
}

int GetTreeViewIconIndex(int icon_id)
{
	if (icon_id < 0)
		return -icon_id;

	for (size_t i = 0; i < std::size(treeIconNames); i++)
		if (icon_id == treeIconNames[i].nResourceID)
			return i;

	return -1;
}

static std::optional<std::filesystem::path> find_or_create_category_path(std::string_view option_value)
{
    if (option_value.empty() || option_value.length() < 2)
    {
        std::cout << "SaveExternalFolders: Couldn't find the category ini folder. option_value = '" << option_value << "'\n";
        return std::nullopt;
    }

    // Initialize tokenizer with ';' delimiter
    mameui::util::stringtokenizer tokenizer(option_value, ";");
    
    // Use a cursor to check tokens for existing directories
    auto cursor = tokenizer.begin();

    // Try to find an existing directory among tokens
    for (const auto& token : cursor.remaining_tokens())
    {
        if (std::filesystem::exists(token) && std::filesystem::is_directory(token))
            return std::filesystem::path(token);
    }

    // Instead of reset(), just create a fresh cursor to get the first token again
    auto first_cursor = tokenizer.begin();
    auto first_token_opt = first_cursor.next_token();

    if (!first_token_opt || first_token_opt->empty())
    {
        std::cout << "SaveExternalFolders: No valid directory path specified in \"" << option_value << "\"\n";
        return std::nullopt;
    }

    std::filesystem::path candidate = *first_token_opt;

    // Attempt to create the directory if it doesn't exist
    std::error_code ec;
    if (!std::filesystem::exists(candidate))
    {
        if (!std::filesystem::create_directory(candidate, ec))
        {
            std::cout << "SaveExternalFolders: Unable to create the directory \"" << candidate
                      << "\" (" << ec.message() << ")\n";
            return std::nullopt;
        }
    }

    return candidate;
}

static bool write_folder_contents(std::ofstream &out, TREEFOLDER *folder_data)
{
	bool found = false;
	for (size_t i = 0; i < driver_list::total(); ++i)
	{
		if (TestBit(folder_data->m_lpGameBits, i))
		{
			out << GetGameName(i) << '\n';
			found = true;
		}
	}
	return found;
}

static void save_folder_sections(std::ofstream &out, int parent_index)
{
	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// Write root section
	out << "\n[ROOT_FOLDER]\n";
	write_folder_contents(out, lpFolder);

	// Write each child folder section
	for (size_t i = 0; i < numFolders; i++)
	{
		TREEFOLDER* folder_data = treeFolders[i];

		if (folder_data->m_nParent >= 0 && treeFolders[folder_data->m_nParent] == lpFolder)
		{
			out << "\n[" << folder_data->m_lpTitle << "]\n";
			write_folder_contents(out, folder_data);
		}
	}
}

static void SaveExternalFolders(int parent_index, std::string_view fname)
{
	const std::string option_value = emu_opts.dir_get_value(DIRPATH_CATEGORYINI_PATH);
	auto categoryini_path_opt = find_or_create_category_path(option_value);

	if (!categoryini_path_opt)
		return;

	const std::filesystem::path& categoryini_path = *categoryini_path_opt;
	const std::filesystem::path filename_path = categoryini_path / std::filesystem::path(fname).replace_extension(".ini");

	std::ofstream save_extern_ofs(filename_path);
	if (!save_extern_ofs.is_open())
	{
		std::cout << "SaveExternalFolders: Unable to open file " << filename_path << " for writing.\n";
		return;
	}

	// Write static header
	save_extern_ofs << "[FOLDER_SETTINGS]\n";
	save_extern_ofs << "RootFolderIcon custom\n";
	save_extern_ofs << "SubFolderIcon custom\n";

	// Write folder and subfolder contents
	save_folder_sections(save_extern_ofs, parent_index);

	std::cout << "SaveExternalFolders: Saved file " << filename_path << '\n';
}

bool ci_contains(std::string_view string, std::string_view sub_string)
{
	if (sub_string.empty()) return true;

	if (string.size() < sub_string.size()) return false;

	auto ci_equal = [](char a, char b) { return std::toupper(static_cast<unsigned char>(a)) == std::toupper(static_cast<unsigned char>(b)); };

	auto it = std::search(string.begin(), string.end(), sub_string.begin(), sub_string.end(), ci_equal);

	return it != string.end();
}

/* End of source file */
