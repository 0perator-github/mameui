// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

/***************************************************************************

  directories.cpp

***************************************************************************/

// standard C++ headers
#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

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
#include "mui_wcstr.h"
#include "mui_wcstrconv.h"
#include "mui_stringtokenizer.h"

#include "windows_controls.h"
#include "dialog_boxes.h"
#include "windows_input.h"
#include "windows_shell.h"
#include "windows_messages.h"

#include "emu_opts.h"
#include "mui_opts.h"
#include "resource.h"
#include "screenshot.h"
#include "winui.h"

#include "directories.h"

using namespace mameui::winapi;
using namespace mameui::winapi::controls;

/***************************************************************************
    Internal structures
 ***************************************************************************/

using DirectoryOption = struct directory_option
{
	std::string option_name;                                    // name to display
	std::string(*pfnGetTheseDirs)(void);              // function to get existing setting
	void (*pfnSetTheseDirs)(std::string_view lpDirs); // function to save new setting
	int dir_index = -1;                                         // entry number in emu_opts:dir_map
	bool is_multi_path = false;                                    // true = it supports multiple directories
	int dir_dlg_flags = 0;                                       // if changed, a refresh needs to be done
};

// list view entries
class path_list
{
public:
	using value_type = std::filesystem::path;
	using size_type = std::vector<std::filesystem::path>::size_type;
	using reference = value_type&;
	using const_reference = const value_type&;
	using iterator = std::vector<value_type>::iterator;
	using const_iterator = std::vector<value_type>::const_iterator;
	using reverse_iterator = std::vector<value_type>::reverse_iterator;
	using const_reverse_iterator = std::vector<value_type>::const_reverse_iterator;

	// iterators
	const_iterator cbegin() const { return path.cbegin(); }
	const_iterator cend() const { return path.cend(); }
	iterator begin() { return path.begin(); }
	iterator end() { return path.end(); }

	// reverse iterators
	const_reverse_iterator const crbegin() { return path.crbegin(); }
	const_reverse_iterator const crend() { return path.crend(); }
	reverse_iterator rbegin() { return path.rbegin(); }
	reverse_iterator rend() { return path.rend(); }

	// copy and move semantics
	path_list(const path_list&) = default;
	path_list(path_list&&) noexcept = default;
	path_list& operator=(const path_list&) = delete;
	path_list& operator=(path_list&&) noexcept = delete;

	// constructor
	path_list() : value_modified(false) {}

	// destructor
	~path_list() = default;

	// subscript operator for const access
	[[nodiscard]] constexpr const_reference operator[](size_t index) const
	{
		return (index < path.size()) ? path[index] : throw out_of_range_exception;
	}

	// subscript operator for non-const access
	[[nodiscard]] constexpr reference operator[](size_t index)
	{
		return (index < path.size()) ? path[index] : throw out_of_range_exception;
	}

	// equality operator for const access
	[[nodiscard]] constexpr	bool operator==(const path_list& other) const noexcept
	{
		return (path == other.path) && (value_modified == other.value_modified);
	}

	// inequality operator
	[[nodiscard]] constexpr	bool operator!=(const path_list& other) const
	{
		return (path != other.path) || (value_modified != other.value_modified);
	}

	// clear the path list and reset modified state
	void clear()
	{
		path.clear();
		value_modified = false;
	}

	// get the number of paths in the list
	[[nodiscard]] constexpr size_t size() const noexcept
	{
		return path.size();
	}

	// get the number of paths in the list (non-const version)
	[[nodiscard]] constexpr size_t size() noexcept
	{
		return path.size();
	}

	// check if the path list is empty
	[[nodiscard]] constexpr bool empty() const
	{
		return path.empty();
	}

	// check if the path list is empty (non-const version)
	[[nodiscard]] constexpr bool empty() noexcept
	{
		return path.empty();
	}

	// get the first path in the list (const version)
	const std::filesystem::path &front() const
	{
		if (!path.empty())
			return path.front();
		throw out_of_range_exception;
	}

	// get the first path in the list (non-const version)
	std::filesystem::path &front()
	{
		if (!path.empty())
			return path.front();
		throw out_of_range_exception;
	}

	// get the last path in the list (const version)
	const std::filesystem::path &back() const
	{
		if (!path.empty())
			return path.back();
		throw out_of_range_exception;
	}

	// get the last path in the list (non-const version)
	std::filesystem::path &back()
	{
		if (!path.empty())
			return path.back();
		throw out_of_range_exception;
	}

	// clear the path list and reset modified state
	void reset()
	{
		path.clear();
		value_modified = false;
	}

	// check if the path list contains the specified path (const version)
	bool contains(const std::filesystem::path &p) const
	{
		return std::find(path.cbegin(), path.cend(), p) != path.cend();
	}

	// check if the path list contains the specified path (non-const version)
	bool contains(const std::filesystem::path &p)
	{
		return std::find(path.begin(), path.end(), p) != path.end();
	}

	// check if the path list contains the specified string (const version)
	bool contains(const std::string_view p) const
	{
		return std::find_if(path.cbegin(), path.cend(), [&p](const std::filesystem::path &item) { return item == p; }) != path.cend();
	}

	// check if the path list contains the specified string (non-const version)
	bool contains(const std::string_view p)
	{
		return std::find_if(path.begin(), path.end(), [&p](const std::filesystem::path &item) { return item == p; }) != path.end();
	}

	// check if the path list contains the specified wide string (const version)
	bool contains(const std::wstring_view p) const
	{
		return std::find_if(path.cbegin(), path.cend(), [&p](const std::filesystem::path &item) { return item == p; }) != path.cend();
	}

	// check if the path list contains the specified wide string (non-const version)
	bool contains(const std::wstring_view p)
	{
		return std::find_if(path.begin(), path.end(), [&p](const std::filesystem::path &item) { return item == p; }) != path.end();
	}
	// add a new path
	void add_path(const std::filesystem::path &new_path)
	{
		path.push_back(new_path);
		value_modified = true;
	}

	// add using a string view
	void add_path(const std::string_view new_path)
	{
		path.emplace_back(new_path);
		value_modified = true;
	}

	//add using a wide string view
	void add_path(const std::wstring_view new_path)
	{
		path.emplace_back(new_path);
		value_modified = true;
	}

	// get the path at the specified index (const version)
    const std::filesystem::path &get_path(size_t index) const
    {
        if (index < path.size())
            return path[index];
        throw out_of_range_exception;
    }

	// get the path at the specified index (non-const version)
	std::filesystem::path &get_path(size_t index)
	{
		if (index < path.size())
			return path[index];
		throw out_of_range_exception;
	}

	// insert a path at the specified index
	void insert_path(size_t index, const std::filesystem::path &new_path)
	{
		if (index <= path.size()) // allow insertion at the end
		{
			path.insert(path.begin() + index, new_path);
			value_modified = true;
		}
		else
			throw out_of_range_exception;
	}

	// insert a path at the specified index using a string view
	void insert_path(size_t index, const std::string_view new_path)
	{
		if (index <= path.size()) // allow insertion at the end
		{
			path.insert(path.begin() + index, std::filesystem::path(new_path));
			value_modified = true;
		}
		else
			throw out_of_range_exception;
	}

	// insert a path at the specified index using a wide string view
	void insert_path(size_t index, const std::wstring_view new_path)
	{
		if (index <= path.size()) // allow insertion at the end
		{
			path.insert(path.begin() + index, std::filesystem::path(new_path));
			value_modified = true;
		}
		else
			throw out_of_range_exception;
	}

	// remove the path at the specified index
	void remove_path(size_t index)
	{
		if (index < path.size())
		{
			path.erase(path.begin() + index);
			value_modified = true;
		}
		else
			throw out_of_range_exception;
	}

	// check if the value has been modified
	bool is_modified() const
	{
		return value_modified;
	}

private:
	std::vector<std::filesystem::path> path;
	bool value_modified = false;
	const std::out_of_range out_of_range_exception{ "Index out of range in path_list" };
};

// combo box entries
using DirInfo = struct dir_info
{
	path_list multi_path;
	std::wstring option_name;
};

/***************************************************************************
    Function prototypes
 ***************************************************************************/

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
bool BrowseForDirectory(HWND hwnd, std::wstring_view pStartDir, std::wstring &selected_dirpath);

static std::wstring FixSlash(std::wstring_view s);

static void     UpdateDirectoryList(HWND hDlg);

static void     Directories_OnSelChange(HWND hDlg);
static bool     Directories_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam);
static void     Directories_OnDestroy(HWND hDlg);
static void     Directories_OnClose(HWND hDlg);
static void     Directories_OnOk(HWND hDlg);
static void     Directories_OnCancel(HWND hDlg);
static void     Directories_OnInsert(HWND hDlg);
static bool     Directories_OnItemChanged(HWND hDlg, NMHDR* pNMHDR);
static void     Directories_OnBrowse(HWND hDlg);
static void     Directories_OnDelete(HWND hDlg);
static bool     Directories_OnBeginLabelEdit(HWND hDlg, NMHDR* pNMHDR);
static bool     Directories_OnEndLabelEdit(HWND hDlg, NMHDR* pNMHDR);
static void     Directories_OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);
static bool     Directories_OnNotify(HWND hDlg, int id, NMHDR* pNMHDR);

/***************************************************************************
    External variables
 ***************************************************************************/

/***************************************************************************
    Internal variables
 ***************************************************************************/

const DirectoryOption g_directory_options[] =
{
	{ "ROMs",                  nullptr,            nullptr,            2,  true,  DIRDLG_ROMS },
	{ "Samples",               nullptr,            nullptr,            4,  true,  DIRDLG_SAMPLES },
	{ "Software File Base",    nullptr,            nullptr,            13, false, DIRDLG_SW }, // core cannot handle multiple path, even though we can.
	{ "Artwork",               nullptr,            nullptr,            5,  true, 0 },
	{ "Artwork Previews",      nullptr,            nullptr,            32, true, 0 },
	{ "Bosses",                nullptr,            nullptr,            33, true, 0 },
	{ "Cabinets",              nullptr,            nullptr,            25, true, 0 },
	{ "Cheats",                nullptr,            nullptr,            9,  true, 0 }, //DIRDLG_CHEAT },  //not used anywhere
	{ "Config",                nullptr,            nullptr,            14, false, 0 }, //DIRDLG_CFG },  //not used anywhere
	{ "Control Panels",        nullptr,            nullptr,            26, true, 0 },
	{ "Controller Files",      nullptr,            nullptr,            6,  true, 0 }, //DIRDLG_CTRLR },  //not used anywhere
	{ "Covers",                nullptr,            nullptr,            41, true, 0 },
	{ "Crosshairs",            nullptr,            nullptr,            10, true, 0 },
	{ "DAT files",             nullptr,            nullptr,            23, false, 0 },
	{ "Ends",                  nullptr,            nullptr,            30, true, 0 },
	{ "Flyers",                nullptr,            nullptr,            28, true, 0 },
	{ "Folders",               nullptr,            nullptr,            24, false, 0 },
	{ "Fonts",                 nullptr,            nullptr,            8,  true, 0 },
	{ "Game Overs",            nullptr,            nullptr,            37, true, 0 },
	{ "Hash",                  nullptr,            nullptr,            3,  true, 0 },
	{ "Hard Drive Difference", nullptr,            nullptr,            19, true, 0 },
	{ "HLSL",                  nullptr,            nullptr,            22, false, 0 },
	{ "How To",                nullptr,            nullptr,            38, true, 0 },
	{ "Icons",                 nullptr,            nullptr,            40, true, 0 },
//  { "Ini Files",             GetIniDir,          nullptr,            7,  false, DIRDLG_INI },  // 2017-02-03 hardcoded to 'ini' now
	{ "Input files",           nullptr,            nullptr,            16, true, 0 }, //DIRDLG_INP },  //not used anywhere
	{ "Language",              nullptr,            nullptr,            12, false, 0 },
	{ "Logos",                 nullptr,            nullptr,            34, true, 0 },
	{ "Manuals (PDF)",         GetManualsDir,      SetManualsDir,      0,  false, 0 },
	{ "Marquees",              nullptr,            nullptr,            31, true, 0 },
	{ "NVRAM",                 nullptr,            nullptr,            15, true, 0 },
	{ "PCBs",                  nullptr,            nullptr,            27, true, 0 },
	{ "Plugins",               nullptr,            nullptr,            11, false, 0 },
	{ "Scores",                nullptr,            nullptr,            35, true, 0 },
	{ "Selects",               nullptr,            nullptr,            39, true, 0 },
	{ "Snapshots",             nullptr,            nullptr,            18, true, 0 }, //DIRDLG_IMG },  //not used anywhere
	{ "State",                 nullptr,            nullptr,            17, true, 0 },
	{ "Titles",                nullptr,            nullptr,            29, true, 0 },
	{ "Versus",                nullptr,            nullptr,            36, true, 0 },
	{ "Videos and Movies",     GetVideoDir,        SetVideoDir,        0,  false, 0 },
	{ "" }
};

const size_t g_directory_option_count = std::size(g_directory_options) - 1; // last entry is empty

static DirInfo *g_directories;
static HWND g_directories_dialog_handle = nullptr; // handle to the dialog box

/***************************************************************************
    External function definitions
 ***************************************************************************/

INT_PTR CALLBACK DirectoriesDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		return Directories_OnInitDialog(hDlg, (HWND)wParam, lParam);

	case WM_COMMAND:
		Directories_OnCommand(hDlg, LOWORD(wParam), (HWND)lParam, HIWORD(wParam));
		return 1;

	case WM_NOTIFY:
		return Directories_OnNotify(hDlg, (int)wParam, (LPNMHDR)lParam);

	case WM_CLOSE:
		Directories_OnClose(hDlg);
		break;

	case WM_DESTROY:
		Directories_OnDestroy(hDlg);
		break;

	default:
		break;
	}
	return 0;
}

/***************************************************************************
    Internal function definitions
 ***************************************************************************/

inline bool Is_Multi_Path(int dir_index) noexcept { return g_directory_options[dir_index].is_multi_path; }
inline bool Path_Modified(DirInfo *info_ptr, int dir_index) noexcept { return info_ptr[dir_index].multi_path.is_modified(); }
inline void Add_Path(DirInfo *info_ptr, int dir_index, std::filesystem::path new_path) { info_ptr[dir_index].multi_path.add_path(new_path); }
inline std::filesystem::path &Get_Path(DirInfo* info_ptr, int dir_index, int path_index) { return info_ptr[dir_index].multi_path[path_index]; }
inline void Insert_Path(DirInfo *info_ptr, int dir_index, int path_index, std::filesystem::path new_path) { info_ptr[dir_index].multi_path.insert_path(path_index, new_path); }
inline size_t Path_Count(DirInfo *info_ptr, int dir_index) noexcept { return info_ptr[dir_index].multi_path.size(); }
inline void Remove_Path(DirInfo *info_ptr, int dir_index, int path_index) { info_ptr[dir_index].multi_path.remove_path(path_index); }

HWND dirs_combo_box_handle()
{
	return (!g_directories_dialog_handle) ? nullptr : dialog_boxes::get_dlg_item(g_directories_dialog_handle, IDC_DIR_COMBO);
}

HWND dirs_dialog_box_handle()
{
	return g_directories_dialog_handle;
}

HWND dirs_list_view_handle()
{
	return (!g_directories_dialog_handle) ? nullptr : dialog_boxes::get_dlg_item(g_directories_dialog_handle, IDC_DIR_LIST);
}

HWND dirs_delete_button_handle()
{
	return (!g_directories_dialog_handle) ? nullptr : dialog_boxes::get_dlg_item(g_directories_dialog_handle, IDC_DIR_DELETE);
}

HWND dirs_insert_button_handle()
{
	return (!g_directories_dialog_handle) ? nullptr : dialog_boxes::get_dlg_item(g_directories_dialog_handle, IDC_DIR_INSERT);
}

/* lop off trailing backslash if it exists */
static std::wstring FixSlash(std::wstring_view s)
{
	if (!s.empty())
	{
		std::wstring_view::const_iterator s_begin = s.begin();
		std::wstring_view::const_iterator s_end = (*s.end() == L'\\' || *s.end() == L'/') ? s.end() - 1 : s.end();
		return std::wstring(s_begin, s_end);
	}

	return L"";
}

static void UpdateDirectoryList(HWND hDlg)
{
	HWND lview_handle  = dirs_list_view_handle();
	HWND cbox_handle = dirs_combo_box_handle();

	if (cbox_handle == nullptr || lview_handle == nullptr)
		return;

	/* Remove previous */
	if(list_view::get_item_count(lview_handle) > 0)
		(void)list_view::delete_all_items(lview_handle);

	/* Update list */
	LVITEMW Item{};
	Item.mask = LVIF_TEXT;

	int dir_index = combo_box::get_cur_sel(cbox_handle);
	if (Is_Multi_Path(dir_index))
	{
		std::wstring item_text(DIRLIST_NEWENTRYTEXT);
		Item.pszText = const_cast<wchar_t*>(item_text.c_str()); // puts the < > empty entry in
		(void)list_view::insert_item(lview_handle, &Item);
		size_t directory_count = Path_Count(g_directories, dir_index);
		// directories are inserted in reverse order
		for (int path_index = static_cast<int>(directory_count) - 1; path_index >= 0; path_index--)
		{
			item_text = Get_Path(g_directories, dir_index, path_index).wstring();
			Item.pszText = const_cast<wchar_t*>(item_text.c_str());
			(void)list_view::insert_item(lview_handle, &Item);
		}
	}
	else
	{
		std::wstring item_text = Get_Path(g_directories, dir_index, 0).wstring();
		Item.pszText = const_cast<wchar_t*>(item_text.c_str());
		(void)list_view::insert_item(lview_handle, &Item);
	}

	/* select first one */

	list_view::set_item_state(lview_handle, 0, LVIS_SELECTED, LVIS_SELECTED);
}

static void Directories_OnSelChange(HWND hDlg)
{
	UpdateDirectoryList(hDlg);

	HWND cbox_handle = dirs_combo_box_handle();
	HWND delbutton_handle = dirs_delete_button_handle();
	HWND insbutton_handle = dirs_insert_button_handle();
	int dir_index = combo_box::get_cur_sel(cbox_handle);

	if (Is_Multi_Path(dir_index))
	{
		(void)input::enable_window(delbutton_handle, true);
		(void)input::enable_window(insbutton_handle, true);
	}
	else
	{
		(void)input::enable_window(delbutton_handle, false);
		(void)input::enable_window(insbutton_handle, false);
	}
}

static bool directories_init()
{
	const size_t option_count = g_directory_option_count;
	g_directories = new(std::nothrow) DirInfo[option_count]{};
	if (!g_directories)
		return false;
	
	for (size_t i = 0; i < option_count; i++)
	{
		g_directories[i].option_name = mui_utf16_from_utf8string(g_directory_options[i].option_name);

		std::string initial_dirs = (g_directory_options[i].dir_index > 0) ? emu_opts.dir_get_value(g_directory_options[i].dir_index) : g_directory_options[i].pfnGetTheseDirs();
		mui_stringtokenizer tokenizer(initial_dirs, ";");
		for (auto current_path : tokenizer)
			Add_Path(g_directories,i, current_path);
	}

	return true;
}

static bool Directories_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
	g_directories_dialog_handle = hDlg;
	HWND lview_handle = dirs_list_view_handle();
	HWND cbox_handle = dirs_combo_box_handle();

	if (lview_handle == nullptr || cbox_handle == nullptr)
		return false;

	if (!directories_init()) /* bummer */
	{
		Directories_OnDestroy(hDlg);
		dialog_boxes::end_dialog(hDlg, -1);
		return false;
	}
	
	for (int i = 0; i < g_directory_option_count; i++)
	{
		(void)combo_box::insert_string(cbox_handle, i, (LPARAM)g_directories[i].option_name.c_str());
	}

	(void)combo_box::set_cur_sel(cbox_handle, 0);

	RECT rectClient;
	(void)windows::get_client_rect(lview_handle, &rectClient);

	LVCOLUMNW LVCol{};
	LVCol.mask = LVCF_WIDTH;
	LVCol.cx = rectClient.right - rectClient.left - windows::get_system_metrics(SM_CXHSCROLL);

	(void)list_view::insert_column(lview_handle, 0, &LVCol);

	/* Keep a temporary copy of the directory strings in g_directories. */

	UpdateDirectoryList(hDlg);
	return true;
}

static void Directories_OnDestroy(HWND hDlg)
{
	if (g_directories)
	{
		delete[] g_directories;
		g_directories = nullptr;
	}
}

static void Directories_OnClose(HWND hDlg)
{
	dialog_boxes::end_dialog(hDlg, IDCANCEL);
}

// Only used by multi dir
static int RetrieveDirList(int dirinfo_index, int nFlagResult, void (*SetTheseDirs)(std::string_view s))
{
	int nResult = 0;

	if (Path_Modified(g_directories, dirinfo_index))
	{
		std::wstring fixed_path;
		int path_count = Path_Count(g_directories, dirinfo_index);

		for (size_t i = 0; i < path_count; i++)
		{
			std::wstring path = Get_Path(g_directories, dirinfo_index, i).wstring();
			fixed_path += FixSlash(path);
			if (i < path_count - 1)
				fixed_path += L";";
		}

		std::string utf8_version = mui_utf8_from_utf16string(fixed_path);
		if (g_directory_options[dirinfo_index].dir_index)
			emu_opts.dir_set_value(g_directory_options[dirinfo_index].dir_index, utf8_version);
		else
			SetTheseDirs(utf8_version);

		nResult |= nFlagResult;
	}

	return nResult;
}

static void Directories_OnOk(HWND hDlg)
{
	int nResult = 0;

	for (size_t i = 0; i < g_directory_option_count; i++)
	{
		if (Is_Multi_Path(i))
			nResult |= RetrieveDirList(i, g_directory_options[i].dir_dlg_flags, g_directory_options[i].pfnSetTheseDirs);
		else
			//if (Path_Modified(g_directories, i))   // this line only makes sense with multi - TODO - fix this up.
		{
			std::wstring path = Get_Path(g_directories, i, 0).wstring();
			std::string utf8_version = mui_utf8_from_utf16string(FixSlash(path));
			if (g_directory_options[i].dir_index)

				emu_opts.dir_set_value(g_directory_options[i].dir_index, utf8_version);
			else
				g_directory_options[i].pfnSetTheseDirs(utf8_version);
		}
//		std::cout << "Directories_OnOk: Is_Multi_Path(i) = " << std::boolalpha << Is_Multi_Path(i) << ", i = " << i << ", dir_index = " << g_directory_options[i].dir_index << ", !g_directory_options[i].dir_index && g_directory_options[i].pfnSetTheseDirs = " << std::boolalpha << (g_directory_options[i].dir_index && g_directory_options[i].pfnSetTheseDirs) << std::endl;
	}

	dialog_boxes::end_dialog(hDlg, nResult);
}

static void Directories_OnCancel(HWND hDlg)
{
	dialog_boxes::end_dialog(hDlg, IDCANCEL);
}

static void Directories_OnInsert(HWND hDlg)
{
	HWND lview_handle = dirs_list_view_handle();
	if (lview_handle == nullptr)
		return;

	int item_index = list_view::get_next_item(lview_handle, -1, LVNI_SELECTED);
	if (item_index == -1)
	{
		item_index = list_view::get_item_count(lview_handle);
	}
	else
		++item_index;

	wchar_t inbuf[MAX_PATH];
	std::wstring outbuf;
	list_view::get_item_text(lview_handle, item_index, 0, inbuf, MAX_PATH);
	if (BrowseForDirectory(hDlg, inbuf, outbuf) == true)
	{
		int dir_index = combo_box::get_cur_sel(dirs_combo_box_handle());
		if (Is_Multi_Path(dir_index))
			Insert_Path(g_directories, dir_index, item_index, outbuf);
		
		UpdateDirectoryList(hDlg);

		list_view::set_item_state(lview_handle, item_index, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
}

// event handler for selection change in the list view
static bool     Directories_OnItemChanged(HWND hDlg, NMHDR* pNMHDR)
{
	bool bResult = false;
	LPNMLISTVIEW listview_info = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if (listview_info->uChanged & LVIF_STATE)
	{
		HWND lview_handle = dirs_list_view_handle();
		int item_index = listview_info->iItem;
		if (item_index == -1)
			return bResult;
		if (list_view::get_item_state(lview_handle, item_index, LVIS_SELECTED) != 0)
		{
			int dir_index = combo_box::get_cur_sel(dirs_combo_box_handle());
			if (Is_Multi_Path(dir_index))
			{
				wchar_t item_text[MAX_PATH]{ '\0' };
				list_view::get_item_text(lview_handle, item_index, 0, item_text, MAX_PATH-1);
				if (mui_wcscmp(item_text, DIRLIST_NEWENTRYTEXT))
				{
					if (input::is_window_enabled(dirs_delete_button_handle()) == false)
						(void)input::enable_window(dirs_delete_button_handle(), true);
//					if (input::is_window_enabled(dirs_insert_button_handle()) == false)
//						(void)input::enable_window(dirs_insert_button_handle(), true);
				}
				else
				{
					if (input::is_window_enabled(dirs_delete_button_handle()) == true)
						(void)input::enable_window(dirs_delete_button_handle(), false);
//					if (input::is_window_enabled(dirs_insert_button_handle()) == true)
//						(void)input::enable_window(dirs_insert_button_handle(), false);
				}
				bResult = true;
			}
		}
	}
	return bResult;
}

static void Directories_OnBrowse(HWND hDlg)
{
	HWND lview_handle = dirs_list_view_handle();
	HWND cbox_handle = dirs_combo_box_handle();

	if (lview_handle == nullptr || cbox_handle == nullptr)
		return;

	int item_index = list_view::get_next_item(lview_handle, -1, LVNI_SELECTED);

	if (item_index == -1)
		return;

	int dir_index = combo_box::get_cur_sel(cbox_handle);
	if (Is_Multi_Path(dir_index))
	{
		/* Last item is placeholder for append */
		if (item_index == list_view::get_item_count(lview_handle) - 1)
		{
			Directories_OnInsert(hDlg);
			return;
		}
	}

	wchar_t inbuf[MAX_PATH]{ '\0' };
	std::wstring outbuf;
	list_view::get_item_text(lview_handle, item_index, 0, inbuf, MAX_PATH);

	if (BrowseForDirectory(hDlg, inbuf, outbuf) == true)
	{
		dir_index = combo_box::get_cur_sel(dirs_combo_box_handle());
		Get_Path(g_directories, dir_index, item_index) = outbuf;
		UpdateDirectoryList(hDlg);
	}
}

static void Directories_OnDelete(HWND hDlg)
{
	HWND lview_handle = dirs_list_view_handle();
	HWND cbox_handle = dirs_combo_box_handle();

	if (lview_handle == nullptr || cbox_handle == nullptr)
		return;


	int item_index = list_view::get_next_item(lview_handle, -1, LVNI_SELECTED | LVNI_ALL);
	if (item_index == -1)
		return;

	//++item_index;
	/* Don't delete "Append" placeholder. */
	int item_count = list_view::get_item_count(lview_handle);
	if (item_index == item_count)
		return;

	int dir_index = combo_box::get_cur_sel(cbox_handle);
	if (Is_Multi_Path(dir_index))
	{
		Remove_Path(g_directories, dir_index, item_index);
		--item_count;
	}
	UpdateDirectoryList(hDlg);

	if (item_count <= 1)
		return;

	/* If the last item was removed, select the item above. */
	int nSelect;
	if (item_index == item_count-1)
		nSelect = item_count - 2;
	else
		nSelect = item_index;

	list_view::set_item_state(lview_handle, nSelect, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
}

static bool Directories_OnBeginLabelEdit(HWND hDlg, NMHDR* pNMHDR)
{
	bool bResult = false;
	NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pNMHDR;
	LVITEMW* pItem = &pDispInfo->item;

	int dir_index = combo_box::get_cur_sel(dirs_combo_box_handle());
	if (Is_Multi_Path(dir_index))
	{
		/* Last item is placeholder for append */
		if (pItem->iItem == list_view::get_item_count(dirs_list_view_handle()) - 1)
		{
			HWND hEdit = (HWND)dialog_boxes::send_dlg_item_message(hDlg, IDC_DIR_LIST, LVM_GETEDITCONTROL, 0, 0);
			windows::set_window_text(hEdit, L"");
		}
	}

	return bResult;
}
static void SetDirectoryEntry(int item_index, const std::wstring& fixed_dir_entry)
{
	int dir_index = combo_box::get_cur_sel(dirs_combo_box_handle());
	if (Is_Multi_Path(dir_index))
	{
		// Last item is placeholder for append
		if (item_index == list_view::get_item_count(dirs_list_view_handle()) - 1)
			Add_Path(g_directories, dir_index, fixed_dir_entry);
		else
			Get_Path(g_directories, dir_index, item_index) = fixed_dir_entry;
	}
	else
	{
		Get_Path(g_directories, dir_index, item_index) = fixed_dir_entry;
	}

	UpdateDirectoryList(dirs_dialog_box_handle());
	list_view::set_item_state(dirs_list_view_handle(), item_index, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
}

static bool Directories_OnEndLabelEdit(HWND hDlg, NMHDR* pNMHDR)
{
	NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pNMHDR;
	LVITEMW* pItem = &pDispInfo->item;

	// parse out any trailing backslash, if it exists
	std::wstring fixed_dir_entry = pItem->pszText? FixSlash(pItem->pszText): L"";
	// Don't allow empty entries.
	if (fixed_dir_entry.empty())
		return false;

	// Check validity of edited directory.
	if (std::filesystem::exists(fixed_dir_entry) && std::filesystem::is_directory(fixed_dir_entry))
	{
		SetDirectoryEntry(pItem->iItem, fixed_dir_entry);
		return true;
	}
	else
	{
		std::wstring err_msg = fixed_dir_entry + L" does not exist, continue anyway?";
		if (dialog_boxes::message_box(nullptr, err_msg.c_str(), &MAMEUINAME[0], MB_OKCANCEL) == IDOK)
		{
			SetDirectoryEntry(pItem->iItem, fixed_dir_entry);
			return true;
		}
	}

	return false;
}

static void Directories_OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
	case IDOK:
		if (codeNotify == BN_CLICKED)
			Directories_OnOk(hDlg);
		break;

	case IDCANCEL:
		if (codeNotify == BN_CLICKED)
			Directories_OnCancel(hDlg);
		break;

	case IDC_DIR_BROWSE:
		if (codeNotify == BN_CLICKED)
			Directories_OnBrowse(hDlg);
		break;

	case IDC_DIR_INSERT:
		if (codeNotify == BN_CLICKED)
			Directories_OnInsert(hDlg);
		break;

	case IDC_DIR_DELETE:
		if (codeNotify == BN_CLICKED)
			Directories_OnDelete(hDlg);
		break;

	case IDC_DIR_COMBO:
		switch (codeNotify)
		{
		case CBN_SELCHANGE:
			Directories_OnSelChange(hDlg);
			break;
		}
		break;
	}
}

static bool Directories_OnNotify(HWND hDlg, int id, NMHDR* pNMHDR)
{
	switch (id)
	{
	case IDC_DIR_LIST:
		switch (pNMHDR->code)
		{
		case LVN_ENDLABELEDIT:
			return Directories_OnEndLabelEdit(hDlg, pNMHDR);

		case LVN_BEGINLABELEDIT:
			return Directories_OnBeginLabelEdit(hDlg, pNMHDR);
		case LVN_ITEMCHANGED:
			return Directories_OnItemChanged(hDlg, pNMHDR);
		}
	}
	return false;
}

/**************************************************************************

    Use the shell to select a DirInfo.

 **************************************************************************/

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	/*
	    Called just after the dialog is initialized
	    Select the dir passed in BROWSEINFO.lParam
	*/
	if (uMsg == BFFM_INITIALIZED)
	{
		const wchar_t *ads_path = reinterpret_cast<const wchar_t*>(lpData);
		if (ads_path && *ads_path)
			(void)windows::send_message(hwnd, BFFM_SETSELECTION, true, lpData);
	}

	return 0;
}

bool BrowseForDirectory(HWND hwnd, std::wstring_view initial_dirpath, std::wstring &selected_dirpath)
{
	wchar_t buffer[MAX_PATH] = { '\0' };

	BROWSEINFOW Info;
	Info.hwndOwner = hwnd;
	Info.pidlRoot = nullptr;
	Info.pszDisplayName = buffer;
	Info.lpszTitle = L"Select a directory:";
	Info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
	Info.lpfn = BrowseCallbackProc;
	std::wstring ads_path(initial_dirpath);
	Info.lParam = reinterpret_cast<LPARAM>(ads_path.c_str());

	LPITEMIDLIST pItemIDList = shell::shell_browse_for_folder(&Info);

	if (pItemIDList)
	{
		if (shell::shell_get_path_from_id_list(pItemIDList, buffer) == true)
		{
			selected_dirpath = buffer; // copy the result to the output parameter
			return true;
		}
	}

	return false;
}

