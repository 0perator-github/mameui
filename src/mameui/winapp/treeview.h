// For licensing and usage information, read docs/winui_license.txt
// ============================================================================
// NOTE: ifdef MESS doesn't work here

#ifndef MAMEUI_WINAPP_TREEVIEW_H
#define MAMEUI_WINAPP_TREEVIEW_H

constexpr std::size_t MAX_EXTRA_FOLDERS = 256;
constexpr std::size_t MAX_EXTRA_SUBFOLDERS = 256;
constexpr std::size_t EXTRAFOLDERDATA_SIZE = 65536; // MAX_EXTRA_FOLDERS * MAX_EXTRA_SUBFOLDERS

constexpr DWORD ICON_NONE = std::numeric_limits<DWORD>::max();

// TreeView structures
using FOLDER_ID = enum : std::size_t
{
	FOLDER_NONE = std::numeric_limits<std::size_t>::max(),
	FOLDER_ALLGAMES = 1,
	FOLDER_ARCADE,
	FOLDER_AVAILABLE,
	FOLDER_BIOS,
	FOLDER_CLONES,
	FOLDER_COMPUTER,
	FOLDER_CONSOLE,
	FOLDER_CPU,
	FOLDER_DEFICIENCY,
	FOLDER_DUMPING,
	FOLDER_FPS,
	FOLDER_HARDDISK,
	FOLDER_HORIZONTAL,
	FOLDER_LIGHTGUN,
	FOLDER_MANUFACTURER,
	FOLDER_MECHANICAL,
	FOLDER_MODIFIED,
	FOLDER_MOUSE,
	FOLDER_NONMECHANICAL,
	FOLDER_NONWORKING,
	FOLDER_ORIGINAL,
	FOLDER_RASTER,
	FOLDER_RESOLUTION,
	FOLDER_SAMPLES,
	FOLDER_SAVESTATE,
	FOLDER_SCREENS,
	FOLDER_SND,
	FOLDER_SOURCE,
	FOLDER_STEREO,
	FOLDER_TRACKBALL,
	FOLDER_UNAVAILABLE,
	FOLDER_VECTOR,
	FOLDER_VERTICAL,
	FOLDER_WORKING,
	FOLDER_YEAR,
	MAX_FOLDERS,
};

using FILTER_TYPE = enum
{
	F_CLONES = 0x00000001,
	F_NONWORKING = 0x00000002,
	F_UNAVAILABLE = 0x00000004,
	F_VECTOR = 0x00000008,
	F_RASTER = 0x00000010,
	F_ORIGINALS = 0x00000020,
	F_WORKING = 0x00000040,
	F_AVAILABLE = 0x00000080,
	F_HORIZONTAL = 0x00000100,
	F_VERTICAL = 0x00000200,
	F_MECHANICAL = 0x00000400,
	F_ARCADE = 0x00000800,
	F_MESS = 0x00001000,
	F_COMPUTER = 0x00002000,
	F_CONSOLE = 0x00004000,
	F_MODIFIED = 0x00008000,
	F_MASK = 0x0000FFFF,
	F_INIEDIT = 0x00010000, // There is an .ini that can be edited. MSH 20070811
	F_CUSTOM = 0x01000000  // for current .ini custom folders
};

// ============================================================================
// Folder And Filter Definitions
// ============================================================================

using FOLDERDATA = struct folder_data
{
	std::string m_lpTitle = ""; // Folder Title
	std::string short_name = "";  // for saving in the .ini
	std::size_t m_nFolderId = FOLDER_NONE; // ID
	DWORD       m_nIconId = ICON_NONE; // if >= 0, resource id of icon (IDI_xxx), otherwise index in image list
	DWORD       m_dwUnset = 0UL; // Excluded filters
	DWORD       m_dwSet = 0UL; // Implied filters
	bool        m_process = false; // 1 = process only if rebuilding the cache
	void        (*m_pfnCreateFolders)(std::size_t parent_index) = nullptr; // Constructor for special folders
	bool        (*m_pfnQuery)(std::size_t driver_index) = nullptr; // Query function
	bool        m_bExpectedResult = false; // Expected query result
	SOFTWARETYPE_OPTIONS m_soft_type_opt = TOTAL_SOFTWARETYPE_OPTIONS; // Has an ini file (vector.ini, etc)
};
using LPFOLDERDATA = FOLDERDATA*;
using LPCFOLDERDATA = const FOLDERDATA*;

// used to build the filter list for the treeview
using FILTER_ITEM = struct filter_item
{
	DWORD m_dwFilterType = 0UL;                 // Filter value
	DWORD m_dwCtrlID = 0UL;                     // Control ID that represents it
	bool (*m_pfnQuery)(std::size_t driver_index) = nullptr; // Query function
	bool m_bExpectedResult = false;                 // Expected query result
};
using LPFILTER_ITEM = FILTER_ITEM*;
using LPCFILTER_ITEM = const FILTER_ITEM*;

using TREEFOLDER = struct tree_folder
{
	std::string  m_lpTitle = "";    // String contains the folder name
	std::wstring m_lpwTitle = L"";  // String contains the folder name as WCHAR*
	std::size_t  m_nFolderId = FOLDER_NONE; // Index / Folder ID number
	std::size_t  m_nParent = FOLDER_NONE;    // Parent folder index in treeFolders[]
	DWORD        m_dwFlags = 0UL;   // Misc flags
	DWORD        m_nIconId = ICON_NONE;   // negative icon index into the ImageList, or IDI_xxx resource id
	BitBuffer    m_lpGameBits;      // Game bits, represent game indices
};
using LPTREEFOLDER = TREEFOLDER*;

using EXFOLDERDATA = struct extra_folder_data
{
	std::string m_szTitle = "";    // Folder Title
	std::size_t m_nFolderId = FOLDER_NONE; // ID
	std::size_t m_nParent = FOLDER_NONE;    // Parent Folder index in treeFolders[]
	DWORD       m_dwFlags = 0UL;   // Flags - Customisable and Filters
	DWORD       m_nIconId = ICON_NONE;     // negative icon index into the ImageList, or IDI_xxx resource id
	DWORD       m_nSubIconId = ICON_NONE;  // negative icon index into the ImageList, or IDI_xxx resource id
};
using LPEXFOLDERDATA = EXFOLDERDATA*;

// ============================================================================
// Functions to build builtin folder lists
// ============================================================================

bool GameFiltered(std::size_t nGame, DWORD dwFlags);
bool GetParentFound(std::size_t nGame);
bool TryRenameCustomFolder(LPTREEFOLDER lpFolder, std::string_view new_name);
DWORD GetTreeViewIconIndex(int icon_id);
HIMAGELIST GetTreeViewIconList(void);
std::size_t GetNumFolders(void);
LPCFILTER_ITEM GetFilterList(void);
LPCFOLDERDATA FindFilter(std::size_t folderID);
LPTREEFOLDER GetCurrentFolder(void);
LPTREEFOLDER GetFolder(std::size_t nFolder);
LPTREEFOLDER GetFolderByID(std::size_t nID);
LPTREEFOLDER GetFolderByName(std::size_t nParentId, const char *pszFolderName);
std::size_t FindGame(LPTREEFOLDER lpFolder, std::size_t nGame);
std::size_t GetCurrentFolderID(void);
void AddGame(LPTREEFOLDER lpFolder, std::size_t nGame);
void AddToCustomFolder(LPTREEFOLDER lpFolder,std::size_t driver_index);
void CreateBIOSFolders(std::size_t parent_index);
void CreateCPUFolders(std::size_t parent_index);
void CreateDeficiencyFolders(std::size_t parent_index);
void CreateDumpingFolders(std::size_t parent_index);
void CreateFPSFolders(std::size_t parent_index);
void CreateManufacturerFolders(std::size_t parent_index);
void CreateResolutionFolders(std::size_t parent_index);
void CreateScreenFolders(std::size_t parent_index);
void CreateSoundFolders(std::size_t parent_index);
void CreateSourceFolders(std::size_t parent_index);
void CreateYearFolders(std::size_t parent_index);
void FreeFolders(void);
void GetFolders(TREEFOLDER ***folders, std::size_t *num_folders);
void InitTreeView(LPCFOLDERDATA lpFolderData, LPCFILTER_ITEM lpFilterList);
void RemoveFromCustomFolder(LPTREEFOLDER lpFolder,std::size_t driver_index);
void RemoveGame(LPTREEFOLDER lpFolder, std::size_t nGame);
void ResetFilters(void);
void ResetTreeViewFolders(void);
void ResetWhichGamesInFolders(void);
void SelectTreeViewFolder(std::size_t folder_id);
void SetCurrentFolder(LPTREEFOLDER lpFolder);

#endif // MAMEUI_WINAPP_TREEVIEW_H
