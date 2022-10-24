// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

/***************************************************************************

  directories.cpp

***************************************************************************/

// standard C++ headers
#include <sys/stat.h>  // S_IFDIR
#include <iostream>

// MAME/MAMEUI headers
#include "directories.h"
#include "resource.h"
#include "emu_opts.h"
#include "mui_wcs.h"
#include "mui_wcsconv.h"

#include "winapi_controls.h"
#include "winapi_dialog_boxes.h"
//#include "winapi_gdi.h"
#include "winapi_input.h"
//#include "winapi_menus.h"
#include "winapi_shell.h"
//#include "winapi_storage.h"
//#include "winapi_system_services.h"
#include "winapi_windows.h"

// standard windows headers
//#include <windows.h>
//#include <windowsx.h>
#include <shlobj.h> // must be included last due to a conflicting definition.

using namespace mameui::winapi;
using namespace mameui::winapi::controls;


#define MAX_DIRS 256

/***************************************************************************
    Internal structures
 ***************************************************************************/

typedef struct
{
	std::wstring m_Directories[MAX_DIRS];
	int     m_NumDirectories;
	BOOL    m_bModified;
} tPath;

typedef struct
{
	tPath   *m_Path;
	std::wstring m_tDirectory;
} tDirInfo;

/***************************************************************************
    Function prototypes
 ***************************************************************************/

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
BOOL            BrowseForDirectory(HWND hwnd, LPCWSTR pStartDir, wchar_t* pResult);

static void     DirInfo_SetDir(tDirInfo *pInfo, int nType, int nItem, LPCWSTR pText);
static std::wstring_view DirInfo_Dir(tDirInfo *pInfo, int nType);
static std::wstring &DirInfo_Path(tDirInfo *pInfo, int nType, int nItem);
static void     DirInfo_SetModified(tDirInfo *pInfo, int nType, BOOL bModified);
static BOOL     DirInfo_Modified(tDirInfo *pInfo, int nType);
static std::wstring FixSlash(std::wstring_view s);

static void     UpdateDirectoryList(HWND hDlg);

static void     Directories_OnSelChange(HWND hDlg);
static BOOL     Directories_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam);
static void     Directories_OnDestroy(HWND hDlg);
static void     Directories_OnClose(HWND hDlg);
static void     Directories_OnOk(HWND hDlg);
static void     Directories_OnCancel(HWND hDlg);
static void     Directories_OnInsert(HWND hDlg);
static void     Directories_OnBrowse(HWND hDlg);
static void     Directories_OnDelete(HWND hDlg);
static BOOL     Directories_OnBeginLabelEdit(HWND hDlg, NMHDR* pNMHDR);
static BOOL     Directories_OnEndLabelEdit(HWND hDlg, NMHDR* pNMHDR);
static void     Directories_OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);
static BOOL     Directories_OnNotify(HWND hDlg, int id, NMHDR* pNMHDR);

/***************************************************************************
    External variables
 ***************************************************************************/

/***************************************************************************
    Internal variables
 ***************************************************************************/

static tDirInfo *g_pDirInfo;

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

static BOOL IsMultiDir(int nType)
{
	return g_directoryInfo[nType].bMulti;
}

static void DirInfo_SetDir(tDirInfo *pInfo, int nType, int nItem, LPCWSTR pText)
{
	if (IsMultiDir(nType))
	{
		assert(nItem >= 0);
		DirInfo_Path(pInfo, nType, nItem) = pText;
		DirInfo_SetModified(pInfo, nType, TRUE);
	}
	else
	{
		pInfo[nType].m_tDirectory = pText;
	}
}

static std::wstring_view DirInfo_Dir(tDirInfo *pInfo, int nType)
{
	// if a multipath exists in a single-path-only area then truncate it
	return mui_wcstok(pInfo[nType].m_tDirectory, L";");
}

static std::wstring &DirInfo_Path(tDirInfo *pInfo, int nType, int nItem)
{
	return pInfo[nType].m_Path->m_Directories[nItem];
}

// only used by Multiple directories; single dirs are always updated
static void DirInfo_SetModified(tDirInfo *pInfo, int nType, BOOL bModified)
{
	//assert(IsMultiDir(nType));
	pInfo[nType].m_Path->m_bModified = bModified;
}

static BOOL DirInfo_Modified(tDirInfo *pInfo, int nType)
{
	//assert(IsMultiDir(nType));
	return pInfo[nType].m_Path->m_bModified;
}

#define DirInfo_NumDir(pInfo, path) \
	((pInfo)[(path)].m_Path->m_NumDirectories)

/* lop off trailing backslash if it exists */
static std::wstring FixSlash(std::wstring_view s)
{
	std::wstring fixed_string;

	if (!s.empty())
	{
		std::wstring_view::const_iterator s_begin = s.begin();
		std::wstring_view::const_iterator s_end = (*s.end() != L'\\') ? s.end() : s.end() - 1;
		fixed_string = std::wstring(s_begin, s_end);
	}
	return fixed_string;
}

static void UpdateDirectoryList(HWND hDlg)
{
	HWND hList  = dialog_boxes::get_dlg_item(hDlg, IDC_DIR_LIST);
	HWND hCombo = dialog_boxes::get_dlg_item(hDlg, IDC_DIR_COMBO);

	/* Remove previous */
	(void)list_view::delete_all_items(hList);

	/* Update list */
	LVITEMW Item{};
	Item.mask = LVIF_TEXT;

	int nType = combo_box::get_cur_sel(hCombo);
	if (IsMultiDir(nType))
	{
		Item.pszText = const_cast<wchar_t*>(&DIRLIST_NEWENTRYTEXT[0]); // puts the < > empty entry in
		(void)list_view::insert_item(hList, &Item);
		int t = DirInfo_NumDir(g_pDirInfo, nType);
		// directories are inserted in reverse order
		for (int i = t; 0 < i; i--)
		{
			Item.pszText = &DirInfo_Path(g_pDirInfo, nType, i-1)[0];
			(void)list_view::insert_item(hList, &Item);
		}
	}
	else
	{
		Item.pszText = const_cast<wchar_t*>(&DirInfo_Dir(g_pDirInfo, nType)[0]);
		(void)list_view::insert_item(hList, &Item);
	}

	/* select first one */

	list_view::set_item_state(hList, 0, LVIS_SELECTED, LVIS_SELECTED);
}

static void Directories_OnSelChange(HWND hDlg)
{
	UpdateDirectoryList(hDlg);

	HWND hCombo = dialog_boxes::get_dlg_item(hDlg, IDC_DIR_COMBO);
	int nType = combo_box::get_cur_sel(hCombo);

	if (IsMultiDir(nType))
	{
		(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_DIR_DELETE), TRUE);
		(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_DIR_INSERT), TRUE);
	}
	else
	{
		(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_DIR_DELETE), FALSE);
		(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_DIR_INSERT), FALSE);
	}
}

static BOOL Directories_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
	RECT rectClient;
	LVCOLUMNW LVCol{};
	size_t nDirInfoCount;
	std::string s;
	std::wstring token;
	wchar_t multi_dir[MAX_PATH * MAX_DIRS];

	/* count how many dirinfos there are */
	for (nDirInfoCount = 0; g_directoryInfo[nDirInfoCount].lpName; nDirInfoCount++);

	g_pDirInfo = new tDirInfo[nDirInfoCount]{};

	if (!g_pDirInfo) /* bummer */
		goto error;

	for (int i = nDirInfoCount - 1; i >= 0; i--)
	{
		std::unique_ptr<wchar_t[]> utf8_to_wcs(mui_wcstring_from_utf8(g_directoryInfo[i].lpName));
		if( !utf8_to_wcs)
			return FALSE;

		(void)combo_box::insert_string(dialog_boxes::get_dlg_item(hDlg, IDC_DIR_COMBO), 0, (LPARAM)utf8_to_wcs.get());
	}

	(void)combo_box::set_cur_sel(dialog_boxes::get_dlg_item(hDlg, IDC_DIR_COMBO), 0);

	(void)windows::get_client_rect(dialog_boxes::get_dlg_item(hDlg, IDC_DIR_LIST), &rectClient);

	LVCol.mask = LVCF_WIDTH;
	LVCol.cx = rectClient.right - rectClient.left - windows::get_system_metrics(SM_CXHSCROLL);

	(void)list_view::insert_column(dialog_boxes::get_dlg_item(hDlg, IDC_DIR_LIST), 0, &LVCol);

	/* Keep a temporary copy of the directory strings in g_pDirInfo. */
	for (size_t i = 0; i < nDirInfoCount; i++)
	{
		if (g_directoryInfo[i].dir_index)
			s = dir_get_value(g_directoryInfo[i].dir_index);
		else
			s = g_directoryInfo[i].pfnGetTheseDirs();

		std::unique_ptr<wchar_t[]> utf8_to_wcs(mui_wcstring_from_utf8(s.c_str()));
		if( !utf8_to_wcs)
			return FALSE;
		/* Copy the string to our own buffer so that we can mutilate it */
		(void)mui_wcscpy(multi_dir, utf8_to_wcs.get());

		if (IsMultiDir(i))
		{
			g_pDirInfo[i].m_Path = new tPath{};
			if (!g_pDirInfo[i].m_Path)
				goto error;

			g_pDirInfo[i].m_Path->m_NumDirectories = 0;
			token = mui_wcstok(multi_dir, L";");
			while ((DirInfo_NumDir(g_pDirInfo, i) < MAX_DIRS) && !token.empty())
			{
				DirInfo_Path(g_pDirInfo, i, DirInfo_NumDir(g_pDirInfo, i)) = token;
				DirInfo_NumDir(g_pDirInfo, i)++;
				token = mui_wcstok(L"", L";");
			}
			DirInfo_SetModified(g_pDirInfo, i, FALSE);
		}
		else
		{
			// multi not supported so get first directory only
			token = mui_wcstok(multi_dir, L";");
			if (token.empty())
				token = utf8_to_wcs.get();

			DirInfo_SetDir(g_pDirInfo, i, -1, &token[0]);
		}
	}

	UpdateDirectoryList(hDlg);
	return true;

error:
	Directories_OnDestroy(hDlg);
	dialog_boxes::end_dialog(hDlg, -1);
	return false;
}

static void Directories_OnDestroy(HWND hDlg)
{
	if (g_pDirInfo)
	{
		int nDirInfoCount;

		// count how many dirinfos there are
		for (nDirInfoCount = 0; g_directoryInfo[nDirInfoCount].lpName; nDirInfoCount++);

		for (size_t i = 0; i < nDirInfoCount; i++)
		{
			if (g_pDirInfo[i].m_Path)
			{
				delete g_pDirInfo[i].m_Path;
				g_pDirInfo[i].m_Path = 0;
			}
		}
		delete[] g_pDirInfo;
		g_pDirInfo = 0;
	}
}

static void Directories_OnClose(HWND hDlg)
{
	dialog_boxes::end_dialog(hDlg, IDCANCEL);
}

// Only used by multi dir
static int RetrieveDirList(int nDir, int nFlagResult, void (*SetTheseDirs)(std::string_view s))
{
	int nResult = 0;

	if (DirInfo_Modified(g_pDirInfo, nDir))
	{
		std::wstring buf;
		int nPaths = DirInfo_NumDir(g_pDirInfo, nDir);

		for (size_t i = 0; i < nPaths; i++)
		{
			std::wstring& path = DirInfo_Path(g_pDirInfo, nDir, i);
			buf += FixSlash(path);
			if (i < nPaths - 1)
				buf += L";";
		}

		std::unique_ptr<char[]> wcs_to_utf8(mui_utf8_from_wcstring(&buf[0]));
		if (g_directoryInfo[nDir].dir_index)
			dir_set_value(g_directoryInfo[nDir].dir_index, wcs_to_utf8.get());
		else
			SetTheseDirs(wcs_to_utf8.get());

		nResult |= nFlagResult;
	}

	return nResult;
}

static void Directories_OnOk(HWND hDlg)
{
	int nResult = 0;

	for (size_t i = 0; g_directoryInfo[i].lpName; i++)
	{
		if (IsMultiDir(i))
			nResult |= RetrieveDirList(i, g_directoryInfo[i].nDirDlgFlags, g_directoryInfo[i].pfnSetTheseDirs);
		else
		//if (DirInfo_Modified(g_pDirInfo, i))   // this line only makes sense with multi - TODO - fix this up.
		{
			std::wstring fixed_slash = &DirInfo_Dir(g_pDirInfo, i)[0];
			fixed_slash = FixSlash(fixed_slash);
			std::unique_ptr<char[]> wcs_to_utf8(mui_utf8_from_wcstring(&fixed_slash[0]));

			if (g_directoryInfo[i].dir_index)
				dir_set_value(g_directoryInfo[i].dir_index, wcs_to_utf8.get());
			else
				g_directoryInfo[i].pfnSetTheseDirs(wcs_to_utf8.get());
		}
	}
	dialog_boxes::end_dialog(hDlg, nResult);
}

static void Directories_OnCancel(HWND hDlg)
{
	dialog_boxes::end_dialog(hDlg, IDCANCEL);
}

static void Directories_OnInsert(HWND hDlg)
{
	HWND hList = dialog_boxes::get_dlg_item(hDlg, IDC_DIR_LIST);
	int nItem = list_view::get_next_item(hList, -1, LVNI_SELECTED);

	wchar_t inbuf[MAX_PATH], outbuf[MAX_PATH];
	list_view::get_item_text(hList, nItem, 0, inbuf, MAX_PATH);
	if (BrowseForDirectory(hDlg, inbuf, outbuf) == true)
	{
		/* list was empty */
		if (nItem == -1)
			nItem = 0;

		int nType = combo_box::get_cur_sel(dialog_boxes::get_dlg_item(hDlg, IDC_DIR_COMBO));
		if (IsMultiDir(nType))
		{
			if (MAX_DIRS <= DirInfo_NumDir(g_pDirInfo, nType))
				return;

			for (size_t i = DirInfo_NumDir(g_pDirInfo, nType); nItem < i; i--)
				DirInfo_Path(g_pDirInfo, nType, i) = DirInfo_Path(g_pDirInfo, nType, i - 1);

			DirInfo_Path(g_pDirInfo, nType, nItem) = outbuf;
			DirInfo_NumDir(g_pDirInfo, nType)++;
			DirInfo_SetModified(g_pDirInfo, nType, true);
		}

		UpdateDirectoryList(hDlg);

		list_view::set_item_state(hList, nItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
}

static void Directories_OnBrowse(HWND hDlg)
{
	HWND hList = dialog_boxes::get_dlg_item(hDlg, IDC_DIR_LIST);
	int nItem = list_view::get_next_item(hList, -1, LVNI_SELECTED);

	if (nItem == -1)
		return;

	int nType = combo_box::get_cur_sel(dialog_boxes::get_dlg_item(hDlg, IDC_DIR_COMBO));
	if (IsMultiDir(nType))
	{
		/* Last item is placeholder for append */
		if (nItem == list_view::get_item_count(hList) - 1)
		{
			Directories_OnInsert(hDlg);
			return;
		}
	}

	wchar_t inbuf[MAX_PATH], outbuf[MAX_PATH];
	list_view::get_item_text(hList, nItem, 0, inbuf, MAX_PATH);

	if (BrowseForDirectory(hDlg, inbuf, outbuf) == true)
	{
		nType = combo_box::get_cur_sel(dialog_boxes::get_dlg_item(hDlg, IDC_DIR_COMBO));
		DirInfo_SetDir(g_pDirInfo, nType, nItem, outbuf);
		UpdateDirectoryList(hDlg);
	}
}

static void Directories_OnDelete(HWND hDlg)
{
	HWND hList = dialog_boxes::get_dlg_item(hDlg, IDC_DIR_LIST);
	int nItem = list_view::get_next_item(hList, -1, LVNI_SELECTED | LVNI_ALL);

	if (nItem == -1)
		return;

	/* Don't delete "Append" placeholder. */
	if (nItem == list_view::get_item_count(hList) - 1)
		return;

	int nType = combo_box::get_cur_sel(dialog_boxes::get_dlg_item(hDlg, IDC_DIR_COMBO));
	if (IsMultiDir(nType))
	{
		for (size_t i = nItem; i < DirInfo_NumDir(g_pDirInfo, nType) - 1; i++)
			DirInfo_Path(g_pDirInfo, nType, i) = DirInfo_Path(g_pDirInfo, nType, i + 1);

		DirInfo_Path(g_pDirInfo, nType, DirInfo_NumDir(g_pDirInfo, nType) - 1) = L"";
		DirInfo_NumDir(g_pDirInfo, nType)--;

		DirInfo_SetModified(g_pDirInfo, nType, TRUE);
	}

	UpdateDirectoryList(hDlg);

	int nCount = list_view::get_item_count(hList);
	if (nCount <= 1)
		return;

	/* If the last item was removed, select the item above. */
	int nSelect;
	if (nItem == nCount - 1)
		nSelect = nCount - 2;
	else
		nSelect = nItem;

	list_view::set_item_state(hList, nSelect, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
}

static BOOL Directories_OnBeginLabelEdit(HWND hDlg, NMHDR* pNMHDR)
{
	BOOL bResult = false;
	NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pNMHDR;
	LVITEMW* pItem = &pDispInfo->item;

	int nType = combo_box::get_cur_sel(dialog_boxes::get_dlg_item(hDlg, IDC_DIR_COMBO));
	if (IsMultiDir(nType))
	{
		/* Last item is placeholder for append */
		if (pItem->iItem == list_view::get_item_count(dialog_boxes::get_dlg_item(hDlg, IDC_DIR_LIST)) - 1)
		{
			if (MAX_DIRS <= DirInfo_NumDir(g_pDirInfo, nType))
				return true; /* don't edit */

			HWND hEdit = (HWND)dialog_boxes::send_dlg_item_message(hDlg, IDC_DIR_LIST, LVM_GETEDITCONTROL, 0, 0);
			windows::set_window_text(hEdit, L"");
		}
	}

	return bResult;
}

static BOOL Directories_OnEndLabelEdit(HWND hDlg, NMHDR* pNMHDR)
{
	BOOL bResult = false;
	NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pNMHDR;
	LVITEMW* pItem = &pDispInfo->item;

	if (pItem->pszText)
	{
		struct _stat file_stat;

		/* Don't allow empty entries. */
		if (!mui_wcscmp(pItem->pszText, L""))
			return false;

		/* Check validity of edited directory. */
		if (_wstat(pItem->pszText, &file_stat) == 0
		&& (file_stat.st_mode & S_IFDIR))
			bResult = true;
		else
		if (dialog_boxes::message_box(NULL, L"Directory does not exist, continue anyway?", &MAMEUINAME[0], MB_OKCANCEL) == IDOK)
			bResult = true;
	}

	if (bResult == true)
	{
		int nType = combo_box::get_cur_sel(dialog_boxes::get_dlg_item(hDlg, IDC_DIR_COMBO));
		if (IsMultiDir(nType))
		{
			/* Last item is placeholder for append */
			if (pItem->iItem == list_view::get_item_count(dialog_boxes::get_dlg_item(hDlg, IDC_DIR_LIST)) - 1)
			{
				if (MAX_DIRS <= DirInfo_NumDir(g_pDirInfo, nType))
					return false;

				for (size_t i = DirInfo_NumDir(g_pDirInfo, nType); pItem->iItem < i; i--)
					DirInfo_Path(g_pDirInfo, nType, i) = DirInfo_Path(g_pDirInfo, nType, i - 1);

				DirInfo_Path(g_pDirInfo, nType, pItem->iItem) = pItem->pszText;

				DirInfo_SetModified(g_pDirInfo, nType, true);
				DirInfo_NumDir(g_pDirInfo, nType)++;
			}
			else
				DirInfo_SetDir(g_pDirInfo, nType, pItem->iItem, pItem->pszText);
		}
		else
			DirInfo_SetDir(g_pDirInfo, nType, pItem->iItem, pItem->pszText);

		UpdateDirectoryList(hDlg);
		list_view::set_item_state(dialog_boxes::get_dlg_item(hDlg, IDC_DIR_LIST), pItem->iItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}

	return bResult;
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

static BOOL Directories_OnNotify(HWND hDlg, int id, NMHDR* pNMHDR)
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
		}
	}
	return FALSE;
}

/**************************************************************************

    Use the shell to select a Directory.

 **************************************************************************/

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	/*
	    Called just after the dialog is initialized
	    Select the dir passed in BROWSEINFO.lParam
	*/
	if (uMsg == BFFM_INITIALIZED)
	{
		if ((const char*)lpData)
			(void)windows::send_message(hwnd, BFFM_SETSELECTION, TRUE, lpData);
	}

	return 0;
}

BOOL BrowseForDirectory(HWND hwnd, LPCTSTR pStartDir, wchar_t *pResult)
{
	BOOL bResult = false;
	wchar_t buf[MAX_PATH];

	BROWSEINFOW Info;
	Info.hwndOwner = hwnd;
	Info.pidlRoot = NULL;
	Info.pszDisplayName = buf;
	Info.lpszTitle = L"Select a directory:";
	Info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
	Info.lpfn = BrowseCallbackProc;
	Info.lParam = (LPARAM)pStartDir;

	LPITEMIDLIST pItemIDList = SHBrowseForFolderW(&Info);

	if (pItemIDList)
	{
		if (shell::shell_get_path_from_id_list(pItemIDList, buf) == true)
		{
			mui_wcscpy(pResult, buf);
			bResult = true;
		}
	}
	else
		bResult = false;

	return bResult;
}

