// For licensing and usage information, read docs/winui_license.txt
// ============================================================================

// ============================================================================
// treeview.cpp - TreeView support routines
// ============================================================================

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
#include "emu.h"

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

using namespace mameui::util::string_util;
using namespace mameui::winapi::controls;
using namespace mameui::winapi;
using namespace std::string_view_literals;

// ============================================================================
// public structures
// ============================================================================

// Name used for user-defined custom icons
// external *.ico file to look for.

using TREEICON = struct tree_icon
{
	DWORD nResourceID;
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
	{ IDI_FOLDER_MANUFACTURER, "manufact" },
	{ IDI_FOLDER_WORKING,      "working" },
	{ IDI_FOLDER_NONWORKING,   "nonwork" },
	{ IDI_FOLDER_YEAR,         "year" },
	{ IDI_FOLDER_SOUND,        "sound" },
	{ IDI_FOLDER_CPU,          "cpu" },
	{ IDI_FOLDER_HARDDISK,     "harddisk" },
	{ IDI_FOLDER_SOURCE,       "source" }
};

constexpr std::size_t ICON_MAX = std::size(treeIconNames);

// ============================================================================
// private variables
// ============================================================================

// this has an entry for every folder eventually in the UI, including subfolders
static LPTREEFOLDER *treeFolders = nullptr;
static std::size_t  numFolders  = 0;        // Number of folder in the folder array
static std::size_t  next_folder_id = MAX_FOLDERS;
static std::size_t  folderArrayLength = 0;  // Size of the folder array
static LPTREEFOLDER lpCurrentFolder = nullptr;    // Currently selected folder
static std::size_t  current_folder_id = 0;     // Current folder ID
static WNDPROC      g_lpTreeWndProc = 0;    // for subclassing the TreeView
static HIMAGELIST   hTreeSmall = 0;         // TreeView Image list of icons

// this only has an entry for each TOP LEVEL extra folder + SubFolders
LPEXFOLDERDATA     ExtraFolderData[EXTRAFOLDERDATA_SIZE]{};
static std::size_t numExtraFolders = 0;
static std::size_t numExtraIcons = 0;
static std::string ExtraFolderIcons[MAX_EXTRA_FOLDERS];

// built in folders and filters
static LPCFOLDERDATA  g_lpFolderData;
static LPCFILTER_ITEM g_lpFilterList;

// ============================================================================
// private function prototypes
// ============================================================================

extern bool InitFolders(void);
LPEXFOLDERDATA NewExtraFolderData(std::string title, std::size_t folderId, std::size_t parentId, std::size_t iconId, std::size_t subIconId = ICON_NONE, DWORD dwFlags = 0UL);
static bool AddFolder(LPTREEFOLDER lpFolder);
static bool ci_contains(std::string_view string, std::string_view sub_string);
static bool CreateTreeIcons(void);
static bool TryAddExtraFolderAndChildren(std::size_t parent_index);
static bool TrySaveExtraFolder(LPTREEFOLDER lpFolder);
static bool write_folder_contents(std::ofstream& out, TREEFOLDER* folder_data);
static std::size_t InitExtraFolders(void);
static LPTREEFOLDER NewFolder(std::string lpTitle, std::size_t nFolderId, std::size_t nParent, std::size_t nIconId, DWORD dwFlags = 0UL);
static LRESULT CALLBACK TreeWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static std::optional<std::filesystem::path> find_or_create_category_path(std::string_view option_value);
static std::string TrimManufacturer(std::string_view manufacturer_string);
static void DeleteFolder(LPTREEFOLDER &lpFolder);
static void FreeExtraFolders(void);
static void save_folder_sections(std::ofstream& out, std::size_t parent_index);
static void SaveExternalFolders(int parent_index, std::string_view fname);
static void SetExtraIcons(std::string_view name, std::size_t *id);
static void TreeCtrlOnPaint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
std::string ParseManufacturer(std::string_view input, int& parsedChars);

// ============================================================================
// public functions
// ============================================================================

// De-allocate all folder memory
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

// Reset folder filters
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

	// this will subclass the treeview (where WM_DRAWITEM gets sent for the header control)
	LONG_PTR l = windows::get_window_long_ptr(GetTreeView(), GWLP_WNDPROC);
	g_lpTreeWndProc = (WNDPROC)l;
	(void)windows::set_window_long_ptr(GetTreeView(), GWLP_WNDPROC, (LONG_PTR)TreeWndProc);
}

void SetCurrentFolder(LPTREEFOLDER lpFolder)
{
	lpCurrentFolder = (!lpFolder) ? treeFolders[0] : lpFolder;
	current_folder_id = (!lpCurrentFolder) ? 0 : lpCurrentFolder->m_nFolderId;
}

LPTREEFOLDER GetCurrentFolder(void)
{
	return lpCurrentFolder;
}

std::size_t GetCurrentFolderID(void)
{
	return current_folder_id;
}

std::size_t GetNumFolders(void)
{
	return numFolders;
}

LPTREEFOLDER GetFolder(std::size_t nFolder)
{
	return (nFolder < numFolders) ? treeFolders[nFolder] : nullptr;
}

LPTREEFOLDER GetFolderByID(std::size_t nID)
{
	if (nID == FOLDER_NONE)
		return nullptr;

	for (std::size_t i = 0; i < numFolders; i++)
	{
		LPTREEFOLDER folder_data = (!treeFolders[i]) ? nullptr : treeFolders[i];
		if (!folder_data || folder_data->m_nFolderId == FOLDER_NONE)
			continue;

		if (folder_data->m_nFolderId == nID)
			return folder_data;
	}

	return nullptr;
}

void AddGame(LPTREEFOLDER lpFolder, std::size_t driver_index)
{
	if (driver_index >= driver_list::total() || !lpFolder)
		return;

	lpFolder->m_lpGameBits.set(driver_index);
}

void RemoveGame(LPTREEFOLDER lpFolder, std::size_t driver_index)
{
	if (driver_index >= driver_list::total() || !lpFolder)
		return;

	lpFolder->m_lpGameBits.reset(driver_index);
}

std::size_t FindGame(LPTREEFOLDER lpFolder, std::size_t driver_index)
{
	if (driver_index >= driver_list::total() || !lpFolder)
		return INVALID_INDEX;

	return lpFolder->m_lpGameBits.find_next(driver_index, true);
}

static void RebuildGameList(LPTREEFOLDER folder, const FOLDERDATA& folderData, std::size_t totalGames)
{
	// Skip if the folder is null or if it is the BIOS folder.
	if (!folder || folder->m_lpTitle == "BIOS") return;

	folder->m_lpGameBits.set_all(false);

	for (std::size_t gameIndex = 0; gameIndex < totalGames; ++gameIndex)
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


// Used to build the GameList
bool GameFiltered(std::size_t nGame, DWORD dwMask)
{
	LPTREEFOLDER lpFolder = GetCurrentFolder();
	if(!lpFolder)
		return true;

	LPTREEFOLDER lpParent = nullptr;
	std::string driver_fullname, driver_name, driver_manufacturer, driver_source;

	if (nGame < 0 || nGame >= driver_list::total())
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
		lpParent = GetFolder(lpFolder->m_nParent);
		if (lpParent)
		{
			// Check the Parent Filters and inherit them on child,
			// The inherited filters don't display on the custom Filter Dialog for the Child folder
			// No need to promote all games to parent folder, works as is
			dwMask |= lpParent->m_dwFlags;
		}
	}

	if (!GetSearchText().empty() && mui_stricmp(GetSearchText(), SEARCH_PROMPT))
	{
		if (!ci_contains(driver_fullname, GetSearchText()) && !ci_contains(driver_name, GetSearchText()))
			return true;
	}

	// Filter Text is already global
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

// Get the parent of game in this view
bool GetParentFound(std::size_t nGame) // not used
{
	LPTREEFOLDER lpFolder = GetCurrentFolder();

	if( lpFolder )
	{
		std::size_t nParentIndex = GetParentIndex(&driver_list::driver(nGame));

		// return false if no parent is there in this view
		if( nParentIndex == INVALID_INDEX)
			return false;

		// return false if the folder should be HIDDEN in this view
		if (lpFolder->m_lpGameBits.test(nParentIndex) == 0)
			return false;

		// return false if the game should be HIDDEN in this view
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

// ============================================================================
// private functions
// ============================================================================

void CreateSourceFolders(std::size_t parent_index)
{
	if (parent_index >= numFolders)
		return; // invalid parent index

	if (!treeFolders[parent_index])
		return; // invalid parent folder

	LPTREEFOLDER parent_folder = treeFolders[parent_index];

	// no games in top level folder
	parent_folder->m_lpGameBits.set_all(false);

	const std::size_t start_pos = numFolders;
	const std::size_t driver_total = driver_list::total();
	for (std::size_t driver_index = 0; driver_index < driver_total; driver_index++)
	{
		std::string driver_filename = GetDriverFilename_utf8(driver_index);
		if (driver_filename.empty() || driver_filename == "empty")
			continue;

		// look for an existant source treefolder for this game
		// (likely to be the previous one, so start at the end)
		LPTREEFOLDER new_folder = nullptr;
		if (start_pos < numFolders)
		{
			std::size_t folder_index = numFolders;
			while (start_pos < folder_index)
			{
				--folder_index;
				if (!treeFolders[folder_index])
					continue;

				LPTREEFOLDER current_folder = treeFolders[folder_index];
				if (current_folder->m_lpTitle == driver_filename)
				{
					new_folder = current_folder;
					break;
				}
			}
		}

		if (!new_folder)
		{
			new_folder = NewFolder(driver_filename, next_folder_id, parent_index, IDI_FOLDER_SOURCE, GetFolderFlags(numFolders));
			if (!new_folder)
				continue; // failed to create new folder, skip this entry

			if (!AddFolder(new_folder) || next_folder_id >= EXTRAFOLDERDATA_SIZE)
			{
				delete new_folder;
				continue;
			}

			LPEXFOLDERDATA new_extraFolderData = NewExtraFolderData(std::move(driver_filename), next_folder_id, parent_folder->m_nFolderId, IDI_FOLDER_SOURCE);
			if (!new_extraFolderData)
			{
				delete new_folder;
				continue; // failed to create new extra folder data, skip this entry
			}

			ExtraFolderData[next_folder_id] = new_extraFolderData;
			next_folder_id++; // increment the next folder id for the next new folder
		}

		AddGame(new_folder, driver_index);
	}

	SaveExternalFolders(parent_index, "Source");
}

void CreateScreenFolders(std::size_t parent_index)
{
	if (parent_index < 0 || static_cast<std::size_t>(parent_index) >= numFolders)
		return; // invalid parent

	LPTREEFOLDER parent_folder = treeFolders[parent_index];
	if (!parent_folder)
		return;

	// no games in top level folder
	parent_folder->m_lpGameBits.set_all(false);

	const std::size_t start_pos = numFolders;
	const std::size_t driver_total = driver_list::total();
	for (std::size_t driver_index = 0; driver_index < driver_total; ++driver_index)
	{
		int screens = DriverScreenCount(driver_index);
		std::string screen_number = std::to_string(screens);
		if (screen_number.empty())
			continue;

		// look for an existing screen folder
		LPTREEFOLDER new_folder = nullptr;
		if (start_pos < numFolders)
		{
			std::size_t folder_index = numFolders;
			while (start_pos < folder_index)
			{
				--folder_index;
				if (!treeFolders[folder_index])
					continue;

				LPTREEFOLDER current_folder = treeFolders[folder_index];
				if (current_folder->m_lpTitle == screen_number)
				{
					new_folder = current_folder;
					break;
				}
			}
		}

		if (!new_folder)
		{
			new_folder = NewFolder(screen_number, next_folder_id, parent_index, IDI_FOLDER_SCREEN, GetFolderFlags(numFolders));
			if (!new_folder)
				continue;

			if (!AddFolder(new_folder) || next_folder_id >= EXTRAFOLDERDATA_SIZE)
				continue;

			LPEXFOLDERDATA new_extraFolderData = NewExtraFolderData(std::move(screen_number), next_folder_id, treeFolders[parent_index]->m_nFolderId, IDI_FOLDER_SCREEN);
			if (!new_extraFolderData)
			{
				delete new_folder;
				continue;
			}

			ExtraFolderData[next_folder_id] = new_extraFolderData;
			++next_folder_id;
		}

		AddGame(new_folder, driver_index);
	}

	SaveExternalFolders(parent_index, "Screen");
}

void CreateManufacturerFolders(std::size_t parent_index)
{
	if (parent_index >= numFolders)
		return;

	if (!treeFolders[parent_index])
		return;

	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	lpFolder->m_lpGameBits.set_all(false);

	const std::size_t start_pos = numFolders;
	const std::size_t driver_total = driver_list::total();
	for (std::size_t driver_index = 0; driver_index < driver_total; ++driver_index)
	{
		const char *mfr_iter = driver_list::driver(driver_index).manufacturer;
		int iChars = 0;

		while (mfr_iter && mfr_iter[0] != '\0')
		{
			std::string parsed_ref = ParseManufacturer(mfr_iter, iChars);
			mfr_iter += iChars;
			if (parsed_ref.empty())
				continue;

			std::string trimmed_ref = TrimManufacturer(parsed_ref);

			LPTREEFOLDER new_folder = nullptr;
			if (start_pos < numFolders)
			{
				std::size_t folder_index = numFolders;
				while (start_pos < folder_index)
				{
					--folder_index;
					if (!treeFolders[folder_index])
						continue;

					LPTREEFOLDER current_folder = treeFolders[folder_index];
					if (!mui_stricmp(current_folder->m_lpTitle, trimmed_ref))
					{
						AddGame(current_folder, driver_index);
						new_folder = current_folder;
						break;
					}
				}
			}

			if (!new_folder)
			{

				new_folder = NewFolder(std::move(trimmed_ref), next_folder_id, parent_index, IDI_FOLDER_MANUFACTURER, GetFolderFlags(numFolders));
				if (!new_folder)
					continue;

				if (!AddFolder(new_folder) || next_folder_id >= EXTRAFOLDERDATA_SIZE)
				{
					delete new_folder;
					continue;
				}

				LPEXFOLDERDATA new_extraFolderData = NewExtraFolderData(std::move(parsed_ref), next_folder_id, lpFolder->m_nFolderId, IDI_FOLDER_MANUFACTURER);
				if (!new_extraFolderData)
				{
					delete new_folder;
					continue;
				}

				ExtraFolderData[next_folder_id] = new_extraFolderData;
				++next_folder_id;
			}

			AddGame(new_folder, driver_index);
		}
	}

	SaveExternalFolders(parent_index, "Manufacturer");
}

// Make a reasonable name out of the one found in the driver array
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

// Analyze Manufacturer Names for typical patterns, that don't distinguish between companies (e.g. Co., Ltd., Inc., etc.
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

LPEXFOLDERDATA NewExtraFolderData(std::string title, std::size_t folderId, std::size_t parentId, std::size_t iconId, std::size_t subIconId, DWORD dwFlags)
{
	LPEXFOLDERDATA lpExtraFolderData = new(std::nothrow) EXFOLDERDATA;
	if (!lpExtraFolderData)
		return nullptr;

	lpExtraFolderData->m_szTitle = std::move(title);
	lpExtraFolderData->m_nFolderId = folderId;
	lpExtraFolderData->m_nParent = parentId;
	lpExtraFolderData->m_dwFlags = dwFlags;
	lpExtraFolderData->m_nIconId = iconId;
	lpExtraFolderData->m_nSubIconId = subIconId;

	return lpExtraFolderData;
}

void CreateBIOSFolders(std::size_t parent_index)
{
	if (parent_index >= numFolders)
		return;

	if (!treeFolders[parent_index])
		return;

	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	lpFolder->m_lpGameBits.set_all(false);

	const std::size_t start_pos = numFolders;
	const std::size_t driver_total = driver_list::total();
	for (std::size_t driver_index = 0; driver_index < driver_total; ++driver_index)
	{
		std::size_t nParentIndex = INVALID_INDEX;
		// If this is a clone, find its parent index to resolve the BIOS driver.
		if (DriverIsClone(driver_index))
		{
			nParentIndex = GetParentIndex(&driver_list::driver(driver_index));
			if (nParentIndex == INVALID_INDEX)
				continue;
		}
		else
			nParentIndex = driver_index;  // For non-clones, use the driver itself.

		// Lookup the BIOS driver for this entry.
		const game_driver *gamedrv = &driver_list::driver(nParentIndex);
		if (!gamedrv)
			continue; // No valid driver found, skip this entry.

		nParentIndex = GetParentIndex(gamedrv);
		if (nParentIndex == INVALID_INDEX)
			continue; // No parent found, skip this driver.

		// Find the actual driver for the BIOS.
		gamedrv = &driver_list::driver(nParentIndex);
		if (!gamedrv) continue;

		const char *driver_fullname = gamedrv->type.fullname();
		if (!driver_fullname || *driver_fullname == '\0')
			continue; // Skip if the fullname is empty or null

		// look for an existing folder
		LPTREEFOLDER new_folder = nullptr;
		if (start_pos < numFolders)
		{
			std::size_t folder_index = numFolders;
			while (start_pos < folder_index)
			{
				--folder_index;
				if (!treeFolders[folder_index])
					continue;

				LPTREEFOLDER current_folder = treeFolders[folder_index];
				if (current_folder->m_lpTitle == driver_fullname)
				{
					new_folder = current_folder;
					break;
				}
			}
		}

		if (!new_folder)
		{
			new_folder = NewFolder(driver_fullname, next_folder_id, parent_index, IDI_FOLDER_BIOS, GetFolderFlags(numFolders));
			if (!new_folder)
				continue; // Skip if folder creation failed or max extra folders reached

			if (!AddFolder(new_folder) || next_folder_id >= EXTRAFOLDERDATA_SIZE)
			{
				delete new_folder;
				continue;
			}

			LPEXFOLDERDATA new_extraFolderData = NewExtraFolderData(driver_fullname, next_folder_id, lpFolder->m_nFolderId, IDI_FOLDER_BIOS);
			if (!new_extraFolderData)
			{
				delete new_folder;
				continue;
			}

			ExtraFolderData[next_folder_id] = new_extraFolderData;
			++next_folder_id;
		}

		AddGame(new_folder, driver_index);
	}

	SaveExternalFolders(parent_index, "BIOS");
}

void CreateCPUFolders(std::size_t parent_index)
{
	if (parent_index >= numFolders)
		return;

	if (!treeFolders[parent_index])
		return;

	constexpr std::size_t device_folder_size = 1024;
	LPTREEFOLDER device_folders[device_folder_size]{};
	LPTREEFOLDER parent_folder = treeFolders[parent_index];
	std::size_t device_folder_count = 0;

	const std::size_t driver_total = driver_list::total();
	for (std::size_t driver_index = 0; driver_index < driver_total; ++driver_index)
	{
		machine_config config(driver_list::driver(driver_index), emu_opts.GetGlobalOpts());

		// enumerate through all devices
		for (device_execute_interface &device : execute_interface_enumerator(config.root_device()))
		{
			// get the name
			const char* dev_name = device.device().name();

			if (dev_name && *dev_name) // skip empty names
			{
				// do we have a folder for this device?
				LPTREEFOLDER new_folder = nullptr;
				for (const LPTREEFOLDER &current_folder : device_folders)
				{
					if (current_folder && current_folder->m_lpTitle == dev_name)
					{
						new_folder = current_folder;
						break;
					}
				}

				// are we forced to create a folder?
				if (new_folder == nullptr)
				{
					new_folder = NewFolder(dev_name, next_folder_id, parent_index, IDI_FOLDER_CPU, GetFolderFlags(numFolders));
					if (!new_folder)
						continue;

					if (!AddFolder(new_folder) || next_folder_id >= EXTRAFOLDERDATA_SIZE)
					{
						delete new_folder;
						continue;
					}

					LPEXFOLDERDATA new_extraFolderData = NewExtraFolderData(dev_name, next_folder_id, parent_folder->m_nFolderId, IDI_FOLDER_CPU);
					if (!new_extraFolderData)
					{
						delete new_folder;
						continue;
					}

					ExtraFolderData[next_folder_id] = new_extraFolderData;
					++next_folder_id;

					if (device_folder_count < device_folder_size)
					{
						device_folders[device_folder_count] = new_folder;
						device_folder_count++;
					}
					else
					{
						std::cout << "CreateCPUFolders buffer overrun: " << device_folder_count << "\n";
						break;
					}
				}

				AddGame(new_folder, driver_index);
			}
		}
	}

	SaveExternalFolders(parent_index, "CPU");
}

void CreateSoundFolders(std::size_t parent_index)
{
	if (parent_index >= numFolders)
		return;

	if (!treeFolders[parent_index])
		return;

	constexpr std::size_t device_folder_size = 512;
	LPTREEFOLDER device_folders[device_folder_size]{};
	LPTREEFOLDER parent_folder = treeFolders[parent_index];
	std::size_t device_folder_count = 0;

	const std::size_t driver_total = driver_list::total();
	for (std::size_t driver_index = 0; driver_index < driver_total; ++driver_index)
	{
		machine_config config(driver_list::driver(driver_index), emu_opts.GetGlobalOpts());

		// enumerate through all devices
		for (device_sound_interface &device : sound_interface_enumerator(config.root_device()))
		{
			// get the name
			const char *dev_name = device.device().name();

			// do we have a folder for this device?
			if (dev_name && *dev_name)
			{
				LPTREEFOLDER new_folder = nullptr;
				for (const LPTREEFOLDER &current_folder : device_folders)
				{
					if (current_folder && current_folder->m_lpTitle == dev_name)
					{
						new_folder = current_folder;
						break;
					}
				}

				// are we forced to create a folder?
				if (new_folder == nullptr)
				{
					new_folder = NewFolder(dev_name, next_folder_id, parent_index, IDI_FOLDER_SOUND, GetFolderFlags(numFolders));
					if (!new_folder)
						continue;

					if (!AddFolder(new_folder) || next_folder_id >= EXTRAFOLDERDATA_SIZE)
					{
						delete new_folder;
						continue;
					}

					LPEXFOLDERDATA new_extraFolderData = NewExtraFolderData(dev_name, next_folder_id, parent_folder->m_nFolderId, IDI_FOLDER_SOUND);
					if (!new_extraFolderData)
					{
						delete new_folder;
						continue;
					}

					ExtraFolderData[next_folder_id] = new_extraFolderData;
					++next_folder_id;

					// record that we found this folder
					if (device_folder_count < device_folder_size)
					{
						device_folders[device_folder_count] = new_folder;
						++device_folder_count;
					}
					else
					{
						std::cout << "CreateSoundFolders buffer overrun: " << device_folder_count << "\n";
						break;
					}
				}

				AddGame(new_folder, driver_index);
			}
		}
	}

	SaveExternalFolders(parent_index, "Sound");
}

void CreateDeficiencyFolders(std::size_t parent_index)
{
	if( parent_index >= numFolders)
		return;

	if (!treeFolders[parent_index])
		return;

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
		spec.folder = NewFolder(spec.title, next_folder_id, parent_index, IDI_FOLDER_IMPERFECT, GetFolderFlags(numFolders));
		if (!spec.folder)
			continue;

		if (!AddFolder(spec.folder) || next_folder_id >= EXTRAFOLDERDATA_SIZE)
		{
			delete spec.folder;
			continue;
		}

		LPEXFOLDERDATA new_extraFolderData = NewExtraFolderData(spec.title, next_folder_id, treeFolders[parent_index]->m_nFolderId, IDI_FOLDER_IMPERFECT);
		if (!new_extraFolderData)
		{
			delete spec.folder;
			continue;
		}

		ExtraFolderData[next_folder_id] = new_extraFolderData;
		++next_folder_id;
	}

	// Clear parent folder bits
	lpFolder->m_lpGameBits.set_all(false);

	// Repopulate
	const std::size_t driver_total = driver_list::total();
	for (std::size_t driver_index = 0; driver_index < driver_total; driver_index++)
	{
		uint64_t cache = GetDriverCacheLower(driver_index);
		for (const auto &spec : specs)
		{
			if (is_flag_set(cache, spec.cacheBit))
				AddGame(spec.folder, driver_index);
		}
	}
}

void CreateDumpingFolders(std::size_t parent_index)
{
	if (parent_index >= numFolders)
		return;

	if (!treeFolders[parent_index])
		return;

	bool bBadDump  = false;
	bool bNoDump = false;
	LPTREEFOLDER parent_folder = treeFolders[parent_index];
	const char *title;

	// create our two subfolders

	title = "Bad Dump";
	LPTREEFOLDER badDump_folder = NewFolder(title, next_folder_id, parent_index, IDI_FOLDER_DUMP, GetFolderFlags(numFolders));
	if (!badDump_folder)
		return;

	if (!AddFolder(badDump_folder) || next_folder_id >= EXTRAFOLDERDATA_SIZE)
	{
		delete badDump_folder;
		return;
	}

	LPEXFOLDERDATA badDump_extraFolderData = NewExtraFolderData(title, next_folder_id, parent_folder->m_nFolderId, IDI_FOLDER_DUMP);
	if (!badDump_extraFolderData)
	{
		delete badDump_folder;
		return;
	}

	ExtraFolderData[next_folder_id] = badDump_extraFolderData;
	++next_folder_id;

	title = "No Dump";
	LPTREEFOLDER noDump_folder = NewFolder(title, next_folder_id, parent_index, IDI_FOLDER_DUMP, GetFolderFlags(numFolders));
	if (!noDump_folder)
		return;

	if (!AddFolder(noDump_folder) || next_folder_id >= EXTRAFOLDERDATA_SIZE)
	{
		delete noDump_folder;
		return;
	}

	LPEXFOLDERDATA noDump_extraFolderData = NewExtraFolderData(title, next_folder_id, parent_folder->m_nFolderId, IDI_FOLDER_DUMP);
	if (!noDump_extraFolderData)
	{
		delete noDump_folder;
		return;
	};

	ExtraFolderData[next_folder_id] = noDump_extraFolderData;
	++next_folder_id;

	// no games in top level folder
	parent_folder->m_lpGameBits.set_all(false);

	const std::size_t driver_total = driver_list::total();
	for (std::size_t driver_index = 0; driver_index < driver_total; driver_index++)
	{
		const game_driver *gamedrv = &driver_list::driver(driver_index);
		if (!gamedrv || !gamedrv->rom)
			continue; // No valid driver found, skip this entry.

		bBadDump = false;
		bNoDump = false;
		// Allocate machine config
		machine_config config(*gamedrv, emu_opts.GetGlobalOpts());

		for (device_t &device : device_enumerator(config.root_device()))
		{
			for (const rom_entry *region = rom_first_region(device); region; region = rom_next_region(region))
			{
				for (const rom_entry *rom = rom_first_file(region); rom; rom = rom_next_file(rom))
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
			AddGame(badDump_folder,driver_index);

		if (bNoDump)
			AddGame(noDump_folder,driver_index);
	}

	SaveExternalFolders(parent_index, "Dumping");
}


void CreateYearFolders(std::size_t parent_index)
{
	if (parent_index >= numFolders)
		return;

	if (!treeFolders[parent_index])
		return;

	LPTREEFOLDER lpFolder = treeFolders[parent_index];

	// no games in top level folder
	lpFolder->m_lpGameBits.set_all(false);

	const std::size_t start_pos = numFolders;
	const std::size_t driver_total = driver_list::total();
	for (std::size_t driver_index = 0; driver_index < driver_total; driver_index++)
	{
		const game_driver* gamedrv = &driver_list::driver(driver_index);
		if (!gamedrv || !gamedrv->year || *gamedrv->year == '\0')
			continue; // No valid driver found, skip this entry.

		const char *year_str = gamedrv->year;
		std::size_t year_size = mui_strlen(year_str);
		if (year_size == MAX_PATH)
			continue; // skip invalid entry

		std::string temp_str;
		if (year_size > 4)
		{
			temp_str.reserve(year_size);
			temp_str.append(year_str, 4); // copy first 4 chars

			// append remaining chars until '?' or end
			for (std::size_t char_pos = 4; char_pos < year_size; ++char_pos)
			{
				if (year_str[char_pos] == '?')
					break;

				temp_str.push_back(year_str[char_pos]);
			}
		}
		else
		{
			temp_str.assign(year_str, year_size);
		}

		if (temp_str.empty())
			continue; // skip invalid entry

		// look for an extant year treefolder for this game
		// (likely to be the previous one, so start at the end)
		LPTREEFOLDER new_folder = nullptr;
		if (start_pos < numFolders)
		{
			std::size_t folder_index = numFolders;
			while (start_pos < folder_index)
			{
				--folder_index;
				if (!treeFolders[folder_index])
					continue;

				LPTREEFOLDER current_folder = treeFolders[folder_index];
				if (mui_strncmp(current_folder->m_lpTitle, temp_str, year_size) == 0)
				{
					AddGame(current_folder, driver_index);
					new_folder = current_folder;
					break;
				}
			}
		}

		if (!new_folder)
		{
			// nope, it's a year we haven't seen before, make it.
			new_folder = NewFolder(temp_str, next_folder_id, parent_index, IDI_FOLDER_YEAR, GetFolderFlags(numFolders));
			if (!new_folder) continue;

			if (!AddFolder(new_folder) || next_folder_id >= EXTRAFOLDERDATA_SIZE)
			{
				delete new_folder;
				continue;
			}

			LPEXFOLDERDATA new_extraFolderData = NewExtraFolderData(std::move(temp_str), next_folder_id, treeFolders[parent_index]->m_nFolderId, IDI_FOLDER_YEAR);
			if (!new_extraFolderData)
			{
				delete new_folder;
				continue;
			}

			ExtraFolderData[next_folder_id] = new_extraFolderData;
			++next_folder_id;
		}

		AddGame(new_folder, driver_index);
	}

	SaveExternalFolders(parent_index, "Year");
}

void CreateResolutionFolders(std::size_t parent_index)
{
	if (parent_index >= numFolders)
		return;

	if (!treeFolders[parent_index])
		return;

	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	std::string folder_title;

	// no games in top level folder
	lpFolder->m_lpGameBits.set_all(false);

	const std::size_t start_pos = numFolders;
	const std::size_t driver_total = driver_list::total();
	for (std::size_t driver_index = 0; driver_index < driver_total; driver_index++)
	{
		if (DriverIsVector(driver_index))
			folder_title = "Vector";
		else
		{
			if (DriverScreenCount(driver_index) == 0)
				folder_title = "Screenless Game";
			else
			{
				// Allocate machine config
				const game_driver &gamedrv = driver_list::driver(driver_index);
				machine_config config(gamedrv, emu_opts.GetGlobalOpts());
				for (screen_device &screen : screen_device_enumerator(config.root_device()))
				{
					const rectangle &visarea = screen.visible_area();
					int horizontal_size = visarea.max_x - visarea.min_x + 1,
						  vertical_size = visarea.max_y - visarea.min_y + 1;
					std::string_view orientation = (gamedrv.flags & ORIENTATION_SWAP_XY) ? "V" : "H";

					std::ostringstream oss;
					oss << horizontal_size << " x " << vertical_size << " (" << orientation << ")";
					folder_title = oss.str();

					// look for an existant screen treefolder for this game
					// (likely to be the previous one, so start at the end)
					LPTREEFOLDER new_folder = nullptr;
					if (start_pos < numFolders)
					{
						std::size_t folder_index = numFolders;
						while (start_pos < folder_index)
						{
							--folder_index;
							if (!treeFolders[folder_index])
								continue;

							LPTREEFOLDER current_folder = treeFolders[folder_index];
							if (current_folder->m_lpTitle == folder_title)
							{
								AddGame(current_folder, driver_index);
								new_folder = current_folder;
								break;
							}
						}
					}

					if (!new_folder)
					{
						// nope, it's a screen we haven't seen before, make it.
						new_folder = NewFolder(folder_title.c_str(), next_folder_id++, parent_index, IDI_FOLDER_RESOL, GetFolderFlags(numFolders));
						if (!new_folder)
							continue;

						if (!AddFolder(new_folder) || next_folder_id >= EXTRAFOLDERDATA_SIZE)
						{
							delete new_folder;
							continue;
						}

						LPEXFOLDERDATA new_extraFolderData = NewExtraFolderData(folder_title.c_str(), next_folder_id, treeFolders[parent_index]->m_nFolderId, IDI_FOLDER_RESOL);
						if (!new_extraFolderData)
						{
							delete new_folder;
							continue;
						}

						ExtraFolderData[next_folder_id] = new_extraFolderData;
						++next_folder_id;
					}

					AddGame(new_folder, driver_index);
				}
			}
		}
	}

	SaveExternalFolders(parent_index, "Resolution");
}

void CreateFPSFolders(std::size_t parent_index)
{
	if (parent_index >= numFolders)
		return;

	if (!treeFolders[parent_index])
		return;

	LPTREEFOLDER lpFolder = treeFolders[parent_index];
	std::string folder_title;

	// no games in top level folder
	lpFolder->m_lpGameBits.set_all(false);

	const std::size_t start_pos = numFolders;
	const std::size_t driver_total = driver_list::total();
	for (std::size_t driver_index = 0; driver_index < driver_total; driver_index++)
	{
		if (DriverIsVector(driver_index))
			folder_title = "Vector";
		else
		{
			if (DriverScreenCount(driver_index) == 0)
				folder_title = "Screenless Game";
			else
			{
				// Allocate machine config
				const game_driver &gamedrv = driver_list::driver(driver_index);
				machine_config config(gamedrv, emu_opts.GetGlobalOpts());
				for (screen_device &screen : screen_device_enumerator(config.root_device()))
				{
					std::ostringstream oss;

					oss << screen.frame_period().as_hz() << " Hz";
					folder_title = oss.str();

					// look for an existant screen treefolder for this game
					// (likely to be the previous one, so start at the end)
					LPTREEFOLDER new_folder = nullptr;
					if (start_pos < numFolders)
					{
						std::size_t folder_index = numFolders;
						while (start_pos < folder_index)
						{
							--folder_index;
							if (!treeFolders[folder_index])
								continue;

							LPTREEFOLDER current_folder = treeFolders[folder_index];
							if (current_folder->m_lpTitle == folder_title)
							{
								AddGame(current_folder, driver_index);
								new_folder = current_folder;
								break;
							}
						}
					}

					if (!new_folder)
					{
						// nope, it's a screen we haven't seen before, make it.
						new_folder = NewFolder(folder_title.c_str(), next_folder_id++, parent_index, IDI_FOLDER_FPS, GetFolderFlags(numFolders));
						if (!new_folder) continue;

						if (!AddFolder(new_folder) || next_folder_id >= EXTRAFOLDERDATA_SIZE)
						{
							delete new_folder;
							continue;
						}

						LPEXFOLDERDATA new_extraFolderData = NewExtraFolderData(folder_title.c_str(), next_folder_id, treeFolders[parent_index]->m_nFolderId, IDI_FOLDER_FPS);
						if (!new_extraFolderData) continue;

						ExtraFolderData[next_folder_id] = new_extraFolderData;
						++next_folder_id;
					}

					AddGame(new_folder, driver_index);
				}
			}
		}
	}

	SaveExternalFolders(parent_index, "Refresh");
}

// adds these folders to the treeview
void ResetTreeViewFolders()
{
	HWND hTreeView = GetTreeView();

	gdi::set_window_redraw(hTreeView, FALSE);
	tree_view::delete_all_items(hTreeView);

	TVINSERTSTRUCTW tvis{};
	tvis.hInsertAfter = TVI_SORT;

	TVITEMW tvi{};
	HTREEITEM hti_parent = nullptr;
	std::size_t index_parent = FOLDER_NONE;

	for (size_t i = 0; i < numFolders; ++i)
	{
		LPTREEFOLDER lpFolder = treeFolders[i];

		if (lpFolder->m_nFolderId >= MAX_FOLDERS || GetShowFolder(lpFolder->m_nFolderId))
		{
			tvi.iImage = GetTreeViewIconIndex(lpFolder->m_nIconId);
			tvi.iSelectedImage = 0;
			tvi.lParam = reinterpret_cast<LPARAM>(lpFolder);
			tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			tvi.pszText = const_cast<wchar_t*>(lpFolder->m_lpwTitle.c_str());

			if (lpFolder->m_nParent == FOLDER_NONE)
			{
				tvis.hParent = TVI_ROOT;
				tvis.item = tvi;

				hti_parent = tree_view::insert_item(hTreeView, &tvis);
				index_parent = FOLDER_NONE;
			}
			else
			{
				// Only search for parent if it changed
				if (lpFolder->m_nParent != index_parent)
				{
					// Find the parent's HTREEITEM
					HTREEITEM hti = tree_view::get_root(hTreeView);
					TVITEMW tvi_parent{ TVIF_PARAM };
					while (hti)
					{
						tvi_parent.hItem = hti;
						if (tree_view::get_item(hTreeView, &tvi_parent))
						{
							LPTREEFOLDER parentFolder = reinterpret_cast<LPTREEFOLDER>(tvi_parent.lParam);
							if (parentFolder == treeFolders[lpFolder->m_nParent])
							{
								hti_parent = hti;
								index_parent = lpFolder->m_nParent;
								break;
							}
						}
						hti = tree_view::get_next_sibling(hTreeView, hti);
					}
				}

				if (!hti_parent)
					continue;

				tvis.hParent = hti_parent;
				tvis.item = tvi;
				(void)tree_view::insert_item(hTreeView, &tvis);
			}
		}
	}

	gdi::set_window_redraw(hTreeView, TRUE);
}

void SelectTreeViewFolder(std::size_t folder_id)
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


// Does this folder have an INI associated with it?
// Currently only true for FOLDER_VECTOR and children
// of FOLDER_SOURCE.

static bool FolderHasIni(LPTREEFOLDER lpFolder)
{
	if (!lpFolder || lpFolder->m_nFolderId == FOLDER_NONE || lpFolder->m_nParent == FOLDER_NONE)
		return false;

	LPCFOLDERDATA data = FindFilter(lpFolder->m_nFolderId);
	if (!data || data->m_soft_type_opt == SOFTWARETYPE_UNSET)
		return false;

	if (data->m_soft_type_opt < TOTAL_SOFTWARETYPE_OPTIONS)
		return true;

	LPTREEFOLDER parent_folder = (!treeFolders[lpFolder->m_nParent]) ? nullptr : treeFolders[lpFolder->m_nParent];
	if (!parent_folder || parent_folder->m_nFolderId == FOLDER_NONE)
		return false;

	if (parent_folder->m_nFolderId == FOLDER_SOURCE)
			return true;

	return false;
}

// Add a folder to the list.  Does not allocate
static bool AddFolder(LPTREEFOLDER lpFolder)
{
	if (!lpFolder)
		return false;

	LPTREEFOLDER *tmpTree = nullptr;
	if (numFolders + 1 >= folderArrayLength)
	{
		std::size_t new_size = folderArrayLength + 500;
		tmpTree = new(std::nothrow) LPTREEFOLDER[new_size]{};
		if (!tmpTree)
			return false;

		for (std::size_t index = 0;index < numFolders;index++)
		{
			if (!treeFolders[index])
				continue;

			tmpTree[index] = treeFolders[index];
		}

		if (treeFolders)
			delete[] treeFolders;

		treeFolders = tmpTree;
		folderArrayLength = new_size;
	}

	// Is there an folder.ini that can be edited?
	if (FolderHasIni(lpFolder))
		lpFolder->m_dwFlags |= F_INIEDIT;

	treeFolders[numFolders] = lpFolder;
	numFolders++;

	return true;
}

// Allocate and initialize a NEW TREEFOLDER
static LPTREEFOLDER NewFolder(std::string lpTitle, std::size_t nFolderId, std::size_t nParent, std::size_t nIconId, DWORD dwFlags)
{
	LPTREEFOLDER lpFolder = new(std::nothrow) TREEFOLDER{};
	if (!lpFolder)
		return nullptr;

	lpFolder->m_lpTitle = std::move(lpTitle);
	lpFolder->m_lpwTitle = mui_utf16_from_utf8string(lpFolder->m_lpTitle);
	lpFolder->m_lpGameBits.resize(driver_list::total());
	lpFolder->m_nFolderId = nFolderId;
	lpFolder->m_nParent = nParent;
	lpFolder->m_nIconId = nIconId;
	lpFolder->m_dwFlags = dwFlags;

	return lpFolder;
}

// Deallocate the passed in LPTREEFOLDER
static void DeleteFolder(LPTREEFOLDER &lpFolder)
{
	if (lpFolder)
	{
		if (!lpFolder->m_lpGameBits.empty())
			lpFolder->m_lpGameBits.reset_all();

		delete lpFolder;
		lpFolder = nullptr;
	}
}

// Can be called to re-initialize the array of treeFolders
bool InitFolders(void)
{
	if (treeFolders != nullptr)
	{
		for (int i = 0; i < numFolders; i++)
			DeleteFolder(treeFolders[i]);

		delete[] treeFolders;
		numFolders = 0;
		treeFolders = nullptr;
	}

	folderArrayLength = 500;
	treeFolders = new(std::nothrow) LPTREEFOLDER[folderArrayLength];
	if (!treeFolders)
	{
		folderArrayLength = 0;
		return false;
	}

	// built-in top level folders
	for (size_t i = 0; !g_lpFolderData[i].m_lpTitle.empty(); i++)
	{
		if (!RequiredDriverCache() && g_lpFolderData[i].m_process)
			continue;

		LPCFOLDERDATA fData = &g_lpFolderData[i];
		// get the saved folder flags
		DWORD dwFolderFlags = GetFolderFlags(numFolders);
		// create the folder
		LPTREEFOLDER new_folder = NewFolder(fData->m_lpTitle.c_str(), fData->m_nFolderId, FOLDER_NONE, fData->m_nIconId, dwFolderFlags);
		if (new_folder != nullptr)
			(void)AddFolder(new_folder);

	}

	numExtraFolders = InitExtraFolders();

	for (size_t i = 0; i < numExtraFolders; i++)
	{
		LPEXFOLDERDATA fExData = ExtraFolderData[i];
		if (!fExData || fExData->m_szTitle.empty())
			continue;

		// create the folder, but if we are building the cache, the name must not be a pre-built one
		if (RequiredDriverCache())
		{
			bool folder_exists = false;
			for (size_t ii = 0; !g_lpFolderData[ii].m_lpTitle.empty(); ii++)
			{
				// check if the folder already exists
				folder_exists = (fExData->m_szTitle == g_lpFolderData[ii].m_lpTitle);
				if (folder_exists)
					break;
			}

			if (folder_exists)
				continue;
		}

		DWORD dwFolderFlags = fExData->m_dwFlags | GetFolderFlags(numFolders);
		LPTREEFOLDER new_folder = NewFolder(fExData->m_szTitle.c_str(), fExData->m_nFolderId, fExData->m_nParent, fExData->m_nIconId, dwFolderFlags);
		if (new_folder != nullptr)
			(void)AddFolder(new_folder);
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
				std::cout << "Internal inconsistency with non-built-in folder, but not custom" << "\n";
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
	std::size_t i;
	HINSTANCE hInst = system_services::get_module_handle(0);

	int numIcons = ICON_MAX + numExtraIcons;
	hTreeSmall = ImageList_Create(16, 16, ILC_COLORDDB | ILC_MASK, numIcons, numIcons);

// std::cout << "Trying to load " << ICON_MAX << " normal icons" << "\n";
	for (i = 0; i < ICON_MAX; i++)
	{
		hIcon = LoadIconFromFile(treeIconNames[i].lpName);
		if (!hIcon)
			hIcon = menus::load_icon(hInst, menus::make_int_resource(treeIconNames[i].nResourceID));

		if (image_list::add_icon(hTreeSmall, hIcon) == -1)
		{
			ErrorMessageBox("Error creating icon on regular folder, %i %i", i, hIcon != nullptr);
			return false;
		}
	}

// std::cout << "Trying to load " << numExtraIcons << " extra custom-folder icons" << "\n";
	for (i = 0; i < numExtraIcons; i++)
	{
		if ((hIcon = LoadIconFromFile(ExtraFolderIcons[i].c_str())) == 0)
			hIcon = menus::load_icon(hInst, menus::make_int_resource(IDI_FOLDER));

		if (image_list::add_icon(hTreeSmall, hIcon) == -1)
		{
			ErrorMessageBox("Error creating icon on extra folder, %i %i", i, hIcon != nullptr);
			return false;
		}
	}

	// Be sure that all the small icons were added.
	if (image_list::get_image_count(hTreeSmall) < numIcons)
	{
		ErrorMessageBox("Error with icon list--too few images.  %i %i", image_list::get_image_count(hTreeSmall), numIcons);
		return false;
	}

	// Be sure that all the small icons were added.

	if (image_list::get_image_count(hTreeSmall) < ICON_MAX)
	{
		ErrorMessageBox("Error with icon list--too few images.  %i < %i", image_list::get_image_count(hTreeSmall), (INT)ICON_MAX);
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

// Header code - Directional Arrows
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
				(void)gdi::update_window(hWnd);
				break;
		}
	}

	// message not handled
	return windows::call_window_proc(g_lpTreeWndProc, hWnd, uMsg, wParam, lParam);
}

// Filter code
// Added 01/09/99 - MSH <mhaaland@hypertech.com>

// find a FOLDERDATA by folderID
LPCFOLDERDATA FindFilter(std::size_t folderID)
{
	for (size_t i = 0; i < MAX_FOLDERS; i++)
		if (g_lpFolderData[i].m_nFolderId == folderID)
			return &g_lpFolderData[i];

	return nullptr;
}

LPTREEFOLDER GetFolderByName(std::size_t nParentId, const char *pszFolderName)
{
	// If the folder name is empty or the parent ID is invalid, return nullptr
	if (!treeFolders || !pszFolderName || nParentId == FOLDER_NONE || *pszFolderName == '\0')
		return nullptr;

	//First Get the Parent TreeviewItem
	//Enumerate Children
	for (int i = 0; i < numFolders; i++)
	{
		if (!treeFolders[i] || treeFolders[i]->m_nParent == FOLDER_NONE)
			continue;

		LPTREEFOLDER folder_data = treeFolders[i];
		if (folder_data->m_lpTitle == pszFolderName)
		{
			LPTREEFOLDER parent_folder = treeFolders[folder_data->m_nParent];
			if (parent_folder != nullptr && parent_folder->m_nFolderId == nParentId)
				return treeFolders[i];
		}
	}

	return nullptr;
}

static std::size_t InitExtraFolders(void)
{
	std::size_t count = 0;
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
		const std::string& file_ext = entry.path().extension().string();
		if (!entry.is_regular_file() || mui_stricmp(file_ext,".ini") != 0)
			continue;

		std::ifstream file_stream(entry.path());
		if (!file_stream.is_open())
			continue;

		std::size_t icon[2] = { ICON_NONE, ICON_NONE };
		std::string line;

		while (std::getline(file_stream, line))
		{
			if (line == "[FOLDER_SETTINGS]")
			{
				while (std::getline(file_stream, line))
				{
					if (!line.empty() && line.front() == '[')
						break; // Next section begins

					stringtokenizer tokenizer(line, " =\r\n");
					auto token_iterator = tokenizer.begin();

					const std::string &key = token_iterator.advance_as_string();
					const std::string &value = token_iterator.advance_as_string();

					if (key.empty() || value.empty())
						continue;

					if (key == "RootFolderIcon")
						SetExtraIcons(value, &icon[0]);
					else if (key == "SubFolderIcon")
						SetExtraIcons(value, &icon[1]);
				}
				break;
			}
		}

		file_stream.close();

		if (count >= MAX_EXTRA_FOLDERS)
			break;

		std::string stem = entry.path().stem().string();
		std::size_t icon_id = (icon[0] == ICON_NONE) ? IDI_FOLDER : icon[0];
		std::size_t subicon_id = (icon[1] == ICON_NONE) ? IDI_FOLDER : icon[1];
		ExtraFolderData[count] = NewExtraFolderData(std::move(stem), next_folder_id, FOLDER_NONE, icon_id, subicon_id, F_CUSTOM);
		if (ExtraFolderData[count] != nullptr)
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
			ExtraFolderData[i] = nullptr;
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


static void SetExtraIcons(std::string_view name, std::size_t* id)
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
bool TryAddExtraFolderAndChildren(std::size_t parent_index)
{
	if (parent_index >= numFolders || !treeFolders[parent_index])
		return false;

	LPTREEFOLDER lpFolder = treeFolders[parent_index], new_folder = nullptr;
	std::size_t extrafolder_id = lpFolder->m_nFolderId - MAX_FOLDERS;
	std::size_t current_id = lpFolder->m_nFolderId;
	std::string read_ini_buffer;

	// "folder\title.ini"
	auto ini_path = std::filesystem::path(emu_opts.dir_get_value(DIRPATH_CATEGORYINI_PATH)) / (std::string(ExtraFolderData[extrafolder_id]->m_szTitle) + ".ini");

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
				current_id = FOLDER_NONE;
				continue;
			}
			else if (name == "ROOT_FOLDER") // is it [ROOT_FOLDER]?
			{
				current_id = lpFolder->m_nFolderId;
				new_folder = lpFolder;
			}
			else
			{
				current_id = next_folder_id++;
				new_folder = NewFolder(name, current_id, parent_index,
					ExtraFolderData[extrafolder_id]->m_nSubIconId,
					GetFolderFlags(numFolders) | F_CUSTOM);

				ExtraFolderData[current_id] = NewExtraFolderData(
					name, current_id - MAX_EXTRA_FOLDERS,
					ExtraFolderData[extrafolder_id]->m_nFolderId, ExtraFolderData[extrafolder_id]->m_nSubIconId, ICON_NONE,
					ExtraFolderData[extrafolder_id]->m_dwFlags);

				(void)AddFolder(new_folder);
			}
		}
		// Process lines within a valid folder section
		else if (current_id != FOLDER_NONE)
		{
			stringtokenizer tokenizer(read_ini_buffer, " \t\r\n");
			auto token_iterator = tokenizer.begin();

			name = token_iterator.advance_as_string();

			if (name.empty())
			{
				current_id = FOLDER_NONE;// reset state if line is invalid
				continue;
			}

			// IMPORTANT: This assumes that all driver names are lowercase!
			std::transform(name.begin(), name.end(), name.begin(), ::tolower); // Convert to lowercase

			if (!new_folder)
			{
				const std::string &ini_path_str = ini_path.string();
				//ErrorMessageBox("Error parsing %s: missing [folder name] or [ROOT_FOLDER]", ini_path_str.c_str());
				current_id = lpFolder->m_nFolderId;
				new_folder = lpFolder;
			}

			AddGame(new_folder, GetGameNameIndex(name.c_str()));
		}
	}

	return true;
}


void GetFolders(TREEFOLDER ***folders, std::size_t *num_folders)
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
		std::filesystem::path new_path = std::move(ini_dirpath);

		if (lpFolder->m_nParent == FOLDER_NONE)

		{
			old_path /= old_name;
			new_path /= new_name;
		}
		else
		{
			LPTREEFOLDER lpParent = GetFolder(lpFolder->m_nParent);
			if (!lpParent)
				return false;

			old_path /= lpParent->m_lpTitle;
			old_path /= old_name;

			new_path /= lpParent->m_lpTitle;
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

	if (lpFolder->m_nParent == FOLDER_NONE)
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
	else
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
			lpFolder->m_lpTitle = std::move(old_title);
		}
	}

	return result;
}

void AddToCustomFolder(LPTREEFOLDER lpFolder, std::size_t driver_index)
{
	if ((lpFolder->m_dwFlags & F_CUSTOM) == 0)
	{
		dialog_boxes::message_box(GetMainWindow(),L"Unable to add game to non-custom folder", &MAMEUINAME[0],MB_OK | MB_ICONERROR);
		return;
	}

	if (lpFolder->m_lpGameBits.test(driver_index) == false)
	{
		AddGame(lpFolder,driver_index);
		if (TrySaveExtraFolder(lpFolder) == false)
			RemoveGame(lpFolder,driver_index); // undo on error
	}
}

void RemoveFromCustomFolder(LPTREEFOLDER lpFolder, std::size_t driver_index)
{
	if ((lpFolder->m_dwFlags & F_CUSTOM) == 0)
	{
		dialog_boxes::message_box(GetMainWindow(),L"Unable to remove game from non-custom folder", &MAMEUINAME[0],MB_OK | MB_ICONERROR);
		return;
	}

	if (lpFolder->m_lpGameBits.test(driver_index) != false)
	{
		RemoveGame(lpFolder,driver_index);
		if (TrySaveExtraFolder(lpFolder) == false)
			AddGame(lpFolder,driver_index); // undo on error
	}
}

bool TrySaveExtraFolder(LPTREEFOLDER lpFolder)
{
	if (!treeFolders || !lpFolder)
	{
		dialog_boxes::message_box(GetMainWindow(),L"Error encountered null folder data", &MAMEUINAME[0], MB_OK | MB_ICONERROR);
		return false;
	}

	std::ofstream ofsSaveExtraFolder;
	std::filesystem::path filename_path;
	const std::string ini_path = emu_opts.dir_get_value(DIRPATH_CATEGORYINI_PATH);

	LPTREEFOLDER root_folder = (lpFolder->m_nParent == FOLDER_NONE) ? lpFolder : treeFolders[lpFolder->m_nParent];
	LPEXFOLDERDATA extra_folder = nullptr;

	// find extra folder data
	for (size_t i = 0; i < numExtraFolders; i++)
	{
		if (!ExtraFolderData[i])
			continue;

		if (ExtraFolderData[i]->m_nFolderId == root_folder->m_nFolderId)
		{
			extra_folder = ExtraFolderData[i];
			break;
		}
	}

	if (extra_folder == nullptr || root_folder == nullptr)
	{
		dialog_boxes::message_box(GetMainWindow(), L"Error finding custom file name to save", &MAMEUINAME[0], MB_OK | MB_ICONERROR);
		return false;
	}
	// "folder\title.ini"

	filename_path = std::filesystem::path(ini_path) / (extra_folder->m_szTitle + ".ini");

	ofsSaveExtraFolder = std::ofstream(filename_path);

	if (!ofsSaveExtraFolder.is_open())
	{
		std::ostringstream oss;
		oss << "Error could not create custom folder file " << filename_path << "\n";
		std::string utf8_mameuiname = mui_utf8_from_utf16string(MAMEUINAME);
		dialog_boxes::message_box_utf8(GetMainWindow(), oss.str().c_str(), utf8_mameuiname.c_str(), MB_OK | MB_ICONERROR);
		return false;
	}

		TREEFOLDER* folder_data;

		ofsSaveExtraFolder << "[FOLDER_SETTINGS]" << "\n";
		// negative values for icons means it's custom, so save 'em
		if (extra_folder->m_nIconId != ICON_NONE)
		{
			std::string_view rootfolder_ico = ExtraFolderIcons[extra_folder->m_nIconId - ICON_MAX];
			ofsSaveExtraFolder << "RootFolderIcon " << rootfolder_ico << "\n";
		}
		if (extra_folder->m_nSubIconId != ICON_NONE)
		{
			std::string_view subfolder_ico = ExtraFolderIcons[extra_folder->m_nSubIconId - ICON_MAX];
			ofsSaveExtraFolder << "SubFolderIcon " << subfolder_ico << "\n";
		}

		// need to loop over all our TREEFOLDERs--first the root one, then each child. Start with the root

		folder_data = root_folder;

		ofsSaveExtraFolder << "\n" << "[ROOT_FOLDER]" << "\n";

		const size_t driver_total = driver_list::total();
		for (size_t i = 0; i < driver_total; i++)
		{
			if (folder_data->m_lpGameBits.test(i))
				ofsSaveExtraFolder << driver_list::driver(i).name << "\n";
		}

		// look through the custom folders for ones with our root as parent

		for (size_t i = 0; i < numFolders; i++)
		{
			if (!treeFolders[i])
				continue;

			folder_data = treeFolders[i];

			if (folder_data->m_nParent == FOLDER_NONE || treeFolders[folder_data->m_nParent] != root_folder)
				continue;

			ofsSaveExtraFolder << "\n" << "[" << folder_data->m_lpTitle << "]" << "\n";

			for (size_t ii = 0; ii < driver_total; ii++)
			{
				if (folder_data->m_lpGameBits.test(ii))
					ofsSaveExtraFolder << "\n" << driver_list::driver(ii).name;
			}

		}

		ofsSaveExtraFolder.close();
		if (!ofsSaveExtraFolder)
		{
			std::ostringstream oss;
			oss << "Error while saving custom file " << filename_path << "\n";
			std::string utf8_mameuiname = mui_utf8_from_utf16string(MAMEUINAME);
			dialog_boxes::message_box_utf8(GetMainWindow(), oss.str().c_str(), utf8_mameuiname.c_str(), MB_OK | MB_ICONERROR);
			return false;
		}

	return true;
}

HIMAGELIST GetTreeViewIconList(void)
{
	return hTreeSmall;
}

std::size_t GetTreeViewIconIndex(std::size_t icon_id)
{
	if (icon_id != ICON_NONE)
	{
		for (std::size_t i = 0; i < std::size(treeIconNames); i++)
			if (icon_id == treeIconNames[i].nResourceID)
				return i;
	}

	return ICON_NONE;
}

static std::optional<std::filesystem::path> find_or_create_category_path(std::string_view option_value)
{
	if (option_value.empty() || option_value.length() < 2)
	{
		std::cout << "SaveExternalFolders: Couldn't find the category ini folder. option_value = '" << option_value << "\n";
		return std::nullopt;
	}

	// Initialize tokenizer with ';' delimiter
	stringtokenizer tokenizer(option_value, ";");

	// Use the tokenizer to convert the list of directories to a vector of strings
	auto tokens = tokenizer.to_vector();

	// Try to find an existing directory from the list of directories
	for (const auto &token : tokens)
	{
		if (std::filesystem::exists(token) && std::filesystem::is_directory(token))
			return std::filesystem::path(token);
	}

	if (tokens.empty())
	{
		std::cout << "SaveExternalFolders: No valid directory path specified in \"" << option_value << "\"\n";
		return std::nullopt;
	}

	std::filesystem::path candidate = tokens.front();
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
	for (std::size_t i = 0; i < driver_list::total(); ++i)
	{
		if (folder_data->m_lpGameBits.test(i))
		{
			out << GetGameName(i) << '\n';
			found = true;
		}
	}
	return found;
}

static void save_folder_sections(std::ofstream &out, std::size_t parent_index)
{
	LPTREEFOLDER parent_folder = treeFolders[parent_index];

	// Write root section
	out << "\n[ROOT_FOLDER]\n";
	write_folder_contents(out, parent_folder);

	// Write each child folder section
	for (size_t i = 0; i < numFolders; i++)
	{
		if (!treeFolders[i])
			continue;

		LPTREEFOLDER folder_data = treeFolders[i];

		if (folder_data->m_nParent == FOLDER_NONE || treeFolders[folder_data->m_nParent] != parent_folder)
			continue;

		out << "\n[" << folder_data->m_lpTitle << "]\n";
		write_folder_contents(out, folder_data);

	}
}

static void SaveExternalFolders(int parent_index, std::string_view fname)
{
	const std::string option_value = emu_opts.dir_get_value(DIRPATH_CATEGORYINI_PATH);
	auto categoryini_path_opt = find_or_create_category_path(option_value);

	if (!categoryini_path_opt)
		return;

	const std::filesystem::path &categoryini_path = *categoryini_path_opt;
	const std::filesystem::path filename_path = categoryini_path / std::filesystem::path(fname).replace_extension(".ini");

	std::ofstream save_extern_ofs(filename_path);
	if (!save_extern_ofs.is_open())
	{
		std::cout << "SaveExternalFolders: Unable to open file " << filename_path.string() << " for writing.\n";
		return;
	}

	// Write static header
	save_extern_ofs << "[FOLDER_SETTINGS]\n";
	save_extern_ofs << "RootFolderIcon custom\n";
	save_extern_ofs << "SubFolderIcon custom\n";

	// Write folder and subfolder contents
	save_folder_sections(save_extern_ofs, parent_index);

	std::cout << "SaveExternalFolders: Saved file " << filename_path.string() << '\n';
}

bool ci_contains(std::string_view string, std::string_view sub_string)
{
	if (sub_string.empty()) return true;

	if (string.size() < sub_string.size()) return false;

	auto ci_equal = [](char a, char b) { return std::toupper(static_cast<unsigned char>(a)) == std::toupper(static_cast<unsigned char>(b)); };

	auto it = std::search(string.begin(), string.end(), sub_string.begin(), sub_string.end(), ci_equal);

	return it != string.end();
}

// End of source file
