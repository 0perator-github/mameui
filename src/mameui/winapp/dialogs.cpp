// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

/***************************************************************************

  dialogs.cpp

  Dialog box procedures go here

***************************************************************************/

#ifdef _MSC_VER
#ifndef NONAMELESSUNION
#define NONAMELESSUNION
#endif
#endif

// standard C++ headers
#include <filesystem>

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

#include "ui/moptions.h"
#include "winopts.h"

// MAMEUI headers
#include "mui_cstr.h"

#include "windows_controls.h"
#include "dialog_boxes.h"
#include "windows_input.h"
#include "menus_other_res.h"
#include "windows_shell.h"
#include "data_access_storage.h"
#include "system_services.h"
#include "windows_messages.h"

#include "bitmask.h"
#include "emu_opts.h"
#include "help.h"
#include "mui_opts.h"
#include "mui_util.h"
#include "properties.h"  // For GetHelpIDs
#include "resource.h"
#include "screenshot.h"
#include "treeview.h"
#include "winui.h"

#include "dialogs.h"

using namespace mameui::util::string_util;
using namespace mameui::winapi::controls;
using namespace mameui::winapi;

struct combo_box_history_tab
{
	const wchar_t*  m_pText;
	const int       m_pData;
};

constexpr combo_box_history_tab g_ComboBoxHistoryTab[] =
{
	{ L"Artwork",          TAB_ARTWORK },
	{ L"Boss",             TAB_BOSSES },
	{ L"Cabinet",          TAB_CABINET },
	{ L"Control Panel",    TAB_CONTROL_PANEL },
	{ L"Cover",            TAB_COVER },
	{ L"End",              TAB_ENDS },
	{ L"Flyer",            TAB_FLYER },
	{ L"Game Over",        TAB_GAMEOVER },
	{ L"How To",           TAB_HOWTO },
	{ L"Logo",             TAB_LOGO },
	{ L"Marquee",          TAB_MARQUEE },
	{ L"PCB",              TAB_PCB },
	{ L"Scores",           TAB_SCORES },
	{ L"Select",           TAB_SELECT },
	{ L"Snapshot",         TAB_SCREENSHOT },
	{ L"Title",            TAB_TITLE },
	{ L"Versus",           TAB_VERSUS },
	{ L"All",              TAB_ALL },
	{ L"None",             TAB_NONE }
};

static std::string g_FilterText;

#define NUM_EXCLUSIONS  12
#define NUMHISTORYTAB   std::size(g_ComboBoxHistoryTab)

/* Pairs of filters that exclude each other */
static DWORD filterExclusion[NUM_EXCLUSIONS] =
{
	IDC_FILTER_CLONES,      IDC_FILTER_ORIGINALS,
	IDC_FILTER_NONWORKING,  IDC_FILTER_WORKING,
	IDC_FILTER_UNAVAILABLE, IDC_FILTER_AVAILABLE,
	IDC_FILTER_RASTER,      IDC_FILTER_VECTOR,
	IDC_FILTER_HORIZONTAL,  IDC_FILTER_VERTICAL,
	IDC_FILTER_ARCADE,      IDC_FILTER_MESS,
};

static DWORD ValidateFilters(LPCFOLDERDATA lpFilterRecord, DWORD dwFlags);
static void DisableFilterControls(HWND hWnd, LPCFOLDERDATA lpFilterRecord, LPCFILTER_ITEM lpFilterItem, DWORD dwFlags);
static void EnableFilterExclusions(HWND hWnd, DWORD dwCtrlID);
static void OnHScroll(HWND hWnd, HWND hwndCtl, UINT code, int pos);
void apply_filter_settings(HWND hDlg, LPTREEFOLDER folder, LPCFILTER_ITEM filterList, LPCFOLDERDATA lpFilterRecord);
void set_inherited_checkbox_text(HWND hDlg, int controlId, DWORD parentFlags, DWORD folderFlags, DWORD flag, bool &shown);

/***************************************************************************/

std::string_view GetFilterText(void)
{
	return g_FilterText;
}

void apply_filter_settings(HWND hDlg, LPTREEFOLDER folder, LPCFILTER_ITEM filterList, LPCFOLDERDATA lpFilterRecord)
{
	DWORD dwFilters = 0;
	HWND hFilterEdit = dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_EDIT);
	std::unique_ptr<char[]> edit_text(windows::get_window_text_utf8(hFilterEdit));
	g_FilterText = (edit_text) ? edit_text.get() : "";

	for (int i = 0; filterList[i].m_dwFilterType; i++)
	{
		if (button_control::get_check(dialog_boxes::get_dlg_item(hDlg, filterList[i].m_dwCtrlID)))
			dwFilters |= filterList[i].m_dwFilterType;
	}

	dwFilters = ValidateFilters(lpFilterRecord, dwFilters);

	folder->m_dwFlags &= ~F_MASK;
	folder->m_dwFlags |= dwFilters;
}

void set_inherited_checkbox_text(HWND hDlg, int controlId, DWORD parentFlags, DWORD folderFlags, DWORD flag, bool& shown)
{
	if ((parentFlags & flag) && !(folderFlags & flag))
	{
		HWND hCtrl = dialog_boxes::get_dlg_item(hDlg, controlId);
		std::unique_ptr<char[]> text_ptr(windows::get_window_text_utf8(hCtrl));
		std::string text = text_ptr ? text_ptr.get() : "";
		if (text.empty()) return;

		text.append(" (*)");
		windows::set_window_text_utf8(hCtrl, text.c_str());
		shown = true;
	}
}

INT_PTR CALLBACK ResetDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	bool resetFilters  = false;
	bool resetGames    = false;
	bool resetUI       = false;
	bool resetDefaults = false;

	switch (Msg)
	{
	case WM_INITDIALOG:
		return true;

	case WM_HELP:
		/* User clicked the ? from the upper right on a control */
		HelpFunction((HWND)((LPHELPINFO)lParam)->hItemHandle, MAMEUICONTEXTHELP, HH_TP_HELP_WM_HELP, GetHelpIDs());
		break;

	case WM_CONTEXTMENU:
		HelpFunction((HWND)wParam, MAMEUICONTEXTHELP, HH_TP_HELP_CONTEXTMENU, GetHelpIDs());

		break;

	case WM_COMMAND :
		switch (LOWORD(wParam))
		{
		case IDOK :
			resetFilters  = button_control::get_check(dialog_boxes::get_dlg_item(hDlg, IDC_RESET_FILTERS));
			resetGames    = button_control::get_check(dialog_boxes::get_dlg_item(hDlg, IDC_RESET_GAMES));
			resetDefaults = button_control::get_check(dialog_boxes::get_dlg_item(hDlg, IDC_RESET_DEFAULT));
			resetUI       = button_control::get_check(dialog_boxes::get_dlg_item(hDlg, IDC_RESET_UI));
			if (resetFilters || resetGames || resetUI || resetDefaults)
			{
				std::wostringstream reset_message;

				reset_message << MAMEUINAME << L" will now reset the following" << "\n";
				reset_message << L"to the default settings:\n" << "\n";

				if (resetDefaults)
					reset_message << L"Global game options" << "\n";
				if (resetGames)
					reset_message << L"Individual game options" << "\n";
				if (resetFilters)
					reset_message << L"Custom folder filters" << "\n";
				if (resetUI)
				{
					reset_message << L"User interface settings\n" << "\n";
					reset_message << L"Resetting the User Interface options" << "\n";
					reset_message << L"requires exiting " << "\n";
					reset_message << MAMEUINAME << L"." << "\n";
				}
				reset_message << L"\nDo you wish to continue?" << std::flush;
				if (dialog_boxes::message_box(hDlg, reset_message.str().c_str(), L"Restore Settings", IDOK) == IDOK)
				{
					if (resetFilters)
						ResetFilters();

					if (resetDefaults)
						emu_opts.ResetGameDefaults();

					// This is the only case we need to exit and restart for.
					if (resetUI)
					{
						ResetGUI();
						dialog_boxes::end_dialog(hDlg, 1);
						return true;
					}
					else
					{
						dialog_boxes::end_dialog(hDlg, 0);
						return true;
					}
				}
				else
				{
					// Give the user a chance to change what they want to reset.
					break;
				}
			}
			[[fallthrough]];
		// Nothing was selected but OK, just fall through
		case IDCANCEL :
			dialog_boxes::end_dialog(hDlg, 0);
			return true;
		}
		break;
	}
	return 0;
}

INT_PTR CALLBACK InterfaceDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	CHOOSECOLOR cc;
	COLORREF choice_colors[16];
	int i = 0;
	bool bRedrawList = false;
	int nCurSelection = 0;
	int nHistoryTab = 0;
	int nPatternCount = 0;
	int value = 0;

	switch (Msg)
	{
	case WM_INITDIALOG:
		button_control::set_check(dialog_boxes::get_dlg_item(hDlg,IDC_START_GAME_CHECK),GetGameCheck());
		button_control::set_check(dialog_boxes::get_dlg_item(hDlg,IDC_JOY_GUI),GetJoyGUI());
		button_control::set_check(dialog_boxes::get_dlg_item(hDlg,IDC_KEY_GUI),GetKeyGUI());
		button_control::set_check(dialog_boxes::get_dlg_item(hDlg,IDC_UI_SKIP_WARNINGS),emu_opts.GetSkipWarnings());
		button_control::set_check(dialog_boxes::get_dlg_item(hDlg,IDC_OVERRIDE_REDX),GetOverrideRedX());
		button_control::set_check(dialog_boxes::get_dlg_item(hDlg,IDC_HIDE_MOUSE),GetHideMouseOnStartup());

		// Get the current value of the control
		(void)dialog_boxes::send_dlg_item_message(hDlg, IDC_CYCLETIMESEC, TBM_SETRANGE, (WPARAM)false, MAKELPARAM(0, 60)); /* [0, 60] */
		value = GetCycleScreenshot();
		(void)dialog_boxes::send_dlg_item_message(hDlg,IDC_CYCLETIMESEC, TBM_SETPOS, true, value);
		(void)dialog_boxes::send_dlg_item_message(hDlg,IDC_CYCLETIMESECTXT,WM_SETTEXT,0, (WPARAM)std::to_wstring(value).c_str());

		button_control::set_check(dialog_boxes::get_dlg_item(hDlg,IDC_STRETCH_SCREENSHOT_LARGER), GetStretchScreenShotLarger());
		button_control::set_check(dialog_boxes::get_dlg_item(hDlg,IDC_FILTER_INHERIT), GetFilterInherit());
		button_control::set_check(dialog_boxes::get_dlg_item(hDlg,IDC_NOOFFSET_CLONES), GetOffsetClones());

		for (size_t i = 0; i < NUMHISTORYTAB; i++)
		{
			(void)combo_box::insert_string(dialog_boxes::get_dlg_item(hDlg, IDC_HISTORY_TAB), i, (LPARAM)g_ComboBoxHistoryTab[i].m_pText);
			(void)combo_box::set_item_data(dialog_boxes::get_dlg_item(hDlg, IDC_HISTORY_TAB), i, (LPARAM)g_ComboBoxHistoryTab[i].m_pData);
		}

		if (GetHistoryTab() < MAX_TAB_TYPES)
			(void)combo_box::set_cur_sel(dialog_boxes::get_dlg_item(hDlg, IDC_HISTORY_TAB), GetHistoryTab());
		else
			(void)combo_box::set_cur_sel(dialog_boxes::get_dlg_item(hDlg, IDC_HISTORY_TAB), GetHistoryTab()-TAB_SUBTRACT);

		(void)combo_box::insert_string(dialog_boxes::get_dlg_item(hDlg, IDC_SNAPNAME), nPatternCount, (LPARAM)L"Gamename");
		(void)combo_box::set_item_data(dialog_boxes::get_dlg_item(hDlg, IDC_SNAPNAME), nPatternCount++, (LPARAM)"%g");
		(void)combo_box::insert_string(dialog_boxes::get_dlg_item(hDlg, IDC_SNAPNAME), nPatternCount, (LPARAM)L"Gamename + Increment");
		(void)combo_box::set_item_data(dialog_boxes::get_dlg_item(hDlg, IDC_SNAPNAME), nPatternCount++, (LPARAM)"%g%i");
		(void)combo_box::insert_string(dialog_boxes::get_dlg_item(hDlg, IDC_SNAPNAME), nPatternCount, (LPARAM)L"Gamename/Gamename");
		(void)combo_box::set_item_data(dialog_boxes::get_dlg_item(hDlg, IDC_SNAPNAME), nPatternCount++, (LPARAM)"%g/%g");
		(void)combo_box::insert_string(dialog_boxes::get_dlg_item(hDlg, IDC_SNAPNAME), nPatternCount, (LPARAM)L"Gamename/Gamename + Increment");
		(void)combo_box::set_item_data(dialog_boxes::get_dlg_item(hDlg, IDC_SNAPNAME), nPatternCount++, (LPARAM)"%g/%g%i");
		(void)combo_box::insert_string(dialog_boxes::get_dlg_item(hDlg, IDC_SNAPNAME), nPatternCount, (LPARAM)L"Gamename/Increment");
		(void)combo_box::set_item_data(dialog_boxes::get_dlg_item(hDlg, IDC_SNAPNAME), nPatternCount, (LPARAM)"%g/%i");
		//Default to this setting
		(void)combo_box::set_cur_sel(dialog_boxes::get_dlg_item(hDlg, IDC_SNAPNAME), nPatternCount++);

		{
			std::string snapname = emu_opts.GetSnapName();
			if (mui_stricmp(snapname,"%g" )==0)
				(void)combo_box::set_cur_sel(dialog_boxes::get_dlg_item(hDlg, IDC_SNAPNAME), 0);
			else
			if (mui_stricmp(snapname,"%g%i" )==0)
				(void)combo_box::set_cur_sel(dialog_boxes::get_dlg_item(hDlg, IDC_SNAPNAME), 1);
			else
			if (mui_stricmp(snapname,"%g/%g" )==0)
				(void)combo_box::set_cur_sel(dialog_boxes::get_dlg_item(hDlg, IDC_SNAPNAME), 2);
			else
			if (mui_stricmp(snapname,"%g/%g%i" )==0)
				(void)combo_box::set_cur_sel(dialog_boxes::get_dlg_item(hDlg, IDC_SNAPNAME), 3);
			else
			if (mui_stricmp(snapname,"%g/%i" )==0)
				(void)combo_box::set_cur_sel(dialog_boxes::get_dlg_item(hDlg, IDC_SNAPNAME), 4);
		}

		(void)dialog_boxes::send_dlg_item_message(hDlg, IDC_SCREENSHOT_BORDERSIZE, TBM_SETRANGE, (WPARAM)false, MAKELPARAM(0, 100)); /* [0, 100] */
		value = GetScreenshotBorderSize();
		(void)dialog_boxes::send_dlg_item_message(hDlg,IDC_SCREENSHOT_BORDERSIZE, TBM_SETPOS, true, value);
		(void)dialog_boxes::send_dlg_item_message(hDlg,IDC_SCREENSHOT_BORDERSIZETXT,WM_SETTEXT,0, (WPARAM)std::to_wstring(value).c_str());

		//return true;
		break;

	case WM_HELP:
		/* User clicked the ? from the upper right on a control */
		HelpFunction((HWND)((LPHELPINFO)lParam)->hItemHandle, MAMEUICONTEXTHELP, HH_TP_HELP_WM_HELP, GetHelpIDs());
		break;

	case WM_CONTEXTMENU:
		HelpFunction((HWND)wParam, MAMEUICONTEXTHELP, HH_TP_HELP_CONTEXTMENU, GetHelpIDs());
		break;
	case WM_HSCROLL:
		OnHScroll(hDlg, (HWND)lParam,(UINT)LOWORD(wParam),(int)HIWORD(wParam));
		break;
	case WM_COMMAND :
		switch (LOWORD(wParam))
		{
		case IDC_SCREENSHOT_BORDERCOLOR:
		{
			for (i=0;i<16;i++)
				choice_colors[i] = GetCustomColor(i);

			cc.lStructSize = sizeof(CHOOSECOLORW);
			cc.hwndOwner   = hDlg;
			cc.rgbResult   = GetScreenshotBorderColor();
			cc.lpCustColors = choice_colors;
			cc.Flags       = CC_ANYCOLOR | CC_RGBINIT | CC_SOLIDCOLOR;
			if (!dialog_boxes::choose_color(&cc))
				return true;
			for (i=0; i<16; i++)
				SetCustomColor(i,choice_colors[i]);
			SetScreenshotBorderColor(cc.rgbResult);
			UpdateScreenShot();
			return true;
		}
		case IDOK :
		{
			bool checked = false;

			SetGameCheck(button_control::get_check(dialog_boxes::get_dlg_item(hDlg, IDC_START_GAME_CHECK)));
			SetJoyGUI(button_control::get_check(dialog_boxes::get_dlg_item(hDlg, IDC_JOY_GUI)));
			SetKeyGUI(button_control::get_check(dialog_boxes::get_dlg_item(hDlg, IDC_KEY_GUI)));
			emu_opts.SetSkipWarnings(button_control::get_check(dialog_boxes::get_dlg_item(hDlg, IDC_UI_SKIP_WARNINGS)));
			SetOverrideRedX(button_control::get_check(dialog_boxes::get_dlg_item(hDlg, IDC_OVERRIDE_REDX)));
			SetHideMouseOnStartup(button_control::get_check(dialog_boxes::get_dlg_item(hDlg,IDC_HIDE_MOUSE)));

			if(button_control::get_check(dialog_boxes::get_dlg_item(hDlg,IDC_RESET_PLAYSTATS ) ) )
			{
				ResetPlayCount( -1 );
				ResetPlayTime( -1 );
				bRedrawList = true;
			}
			value = dialog_boxes::send_dlg_item_message(hDlg,IDC_CYCLETIMESEC, TBM_GETPOS, 0, 0);
			if( GetCycleScreenshot() != value )
			{
				SetCycleScreenshot(value);
			}
			value = dialog_boxes::send_dlg_item_message(hDlg,IDC_SCREENSHOT_BORDERSIZE, TBM_GETPOS, 0, 0);
			if( GetScreenshotBorderSize() != value )
			{
				SetScreenshotBorderSize(value);
				UpdateScreenShot();
			}
			value = dialog_boxes::send_dlg_item_message(hDlg,IDC_HIGH_PRIORITY, TBM_GETPOS, 0, 0);
			checked = button_control::get_check(dialog_boxes::get_dlg_item(hDlg,IDC_STRETCH_SCREENSHOT_LARGER));
			if (checked != GetStretchScreenShotLarger())
			{
				SetStretchScreenShotLarger(checked);
				UpdateScreenShot();
			}
			checked = button_control::get_check(dialog_boxes::get_dlg_item(hDlg,IDC_FILTER_INHERIT));
			if (checked != GetFilterInherit())
			{
				SetFilterInherit(checked);
				// LineUpIcons does just a ResetListView(), which is what we want here
				windows::post_message(GetMainWindow(),WM_COMMAND, MAKEWPARAM(ID_VIEW_LINEUPICONS, false),(LPARAM)nullptr);
			}
			checked = button_control::get_check(dialog_boxes::get_dlg_item(hDlg,IDC_NOOFFSET_CLONES));
			if (checked != GetOffsetClones())
			{
				SetOffsetClones(checked);
				// LineUpIcons does just a ResetListView(), which is what we want here
				windows::post_message(GetMainWindow(),WM_COMMAND, MAKEWPARAM(ID_VIEW_LINEUPICONS, false),(LPARAM)nullptr);
			}
			nCurSelection = combo_box::get_cur_sel(dialog_boxes::get_dlg_item(hDlg,IDC_SNAPNAME));
			if (nCurSelection != CB_ERR)
			{
				const char* snapname_selection = (const char*)combo_box::get_item_data(dialog_boxes::get_dlg_item(hDlg,IDC_SNAPNAME), nCurSelection);
				if (snapname_selection)
					emu_opts.SetSnapName(snapname_selection);
			}
			dialog_boxes::end_dialog(hDlg, 0);

			nCurSelection = combo_box::get_cur_sel(dialog_boxes::get_dlg_item(hDlg,IDC_HISTORY_TAB));
			if (nCurSelection != CB_ERR)
				nHistoryTab = combo_box::get_item_data(dialog_boxes::get_dlg_item(hDlg,IDC_HISTORY_TAB), nCurSelection);
			dialog_boxes::end_dialog(hDlg, 0);
			if( GetHistoryTab() != nHistoryTab )
			{
				SetHistoryTab(nHistoryTab, true);
				ResizePickerControls(GetMainWindow());
				UpdateScreenShot();
			}
			if( bRedrawList )
			{
				UpdateListView();
			}
			return true;
		}
		case IDCANCEL :
			dialog_boxes::end_dialog(hDlg, 0);
			return true;
		}
		break;
	}
	return 0;
}

INT_PTR CALLBACK FilterDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	static DWORD dwFilters;
	static DWORD dwpFilters;
	static LPCFILTER_ITEM filterList;
	static LPCFOLDERDATA lpFilterRecord;
	static LPTREEFOLDER folder;


	switch (Msg)
	{
	case WM_INITDIALOG:
	{
		dwFilters = 0;
		filterList = GetFilterList();
		folder = GetCurrentFolder();

		if (folder)
		{
			windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_EDIT), g_FilterText.c_str());
			edit_control::set_sel(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_EDIT), 0, -1);

			dwFilters = folder->m_dwFlags & F_MASK;

			std::ostringstream oss;
			oss << "Filters for " << folder->m_lpTitle << " Folder";
			windows::set_window_text_utf8(hDlg, oss.str().c_str());

			if (GetFilterInherit())
			{
				LPTREEFOLDER parent = GetFolder(folder->m_nParent);

				if (parent)
				{
					dwpFilters = parent->m_dwFlags & F_MASK;
					bool showExplanation = false;

					for (const FILTER_ITEM* f = filterList; f->m_dwFilterType; ++f)
					{
						set_inherited_checkbox_text(hDlg, f->m_dwCtrlID, dwpFilters, dwFilters, f->m_dwFilterType, showExplanation);
					}

					if (!showExplanation)
						windows::show_window(dialog_boxes::get_dlg_item(hDlg, IDC_INHERITED), false);
				}
			}
			else
			{
				windows::show_window(dialog_boxes::get_dlg_item(hDlg, IDC_INHERITED), false);
			}

			lpFilterRecord = FindFilter(folder->m_nFolderId);

			for (const FILTER_ITEM* f = filterList; f->m_dwFilterType; ++f)
				DisableFilterControls(hDlg, lpFilterRecord, f, dwFilters);
		}

		input::set_focus(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_EDIT));
		return static_cast<INT_PTR>(false);
	}

	case WM_HELP:
		HelpFunction(reinterpret_cast<HWND>(((LPHELPINFO)lParam)->hItemHandle), MAMEUICONTEXTHELP,
			HH_TP_HELP_WM_HELP, GetHelpIDs());
		break;

	case WM_CONTEXTMENU:
		HelpFunction(reinterpret_cast<HWND>(wParam), MAMEUICONTEXTHELP, HH_TP_HELP_CONTEXTMENU, GetHelpIDs());
		break;

	case WM_COMMAND:
	{
		WORD wID = LOWORD(wParam);
		WORD wNotifyCode = HIWORD(wParam);

		switch (wID)
		{
		case IDOK:
			apply_filter_settings(hDlg, folder, filterList, lpFilterRecord);
			dialog_boxes::end_dialog(hDlg, 1);
			return static_cast<INT_PTR>(true);

		case IDCANCEL:
			dialog_boxes::end_dialog(hDlg, 0);
			return static_cast<INT_PTR>(true);

		default:
			if (wNotifyCode == BN_CLICKED)
				EnableFilterExclusions(hDlg, wID);
		}
		break;
	}
	}

	return static_cast<INT_PTR>(false);
}

INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		{
			HBITMAP hBmp = (HBITMAP)menus::load_image(system_services::get_module_handle(0), menus::make_int_resource(IDB_ABOUT), IMAGE_BITMAP, 0, 0, LR_SHARED);
			(void)dialog_boxes::send_dlg_item_message(hDlg, IDC_ABOUT, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBmp);
			std::string mameui_version = GetVersionString();
			(void)windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_VERSION), mameui_version.c_str());
		}
		return 1;

	case WM_COMMAND:
		dialog_boxes::end_dialog(hDlg, 0);
		return 1;
	}
	return 0;
}

HTREEITEM InsertCustomFolder(HWND tree_view_handle, HTREEITEM parent_handle, LPTREEFOLDER folder,bool has_selection)
{
	TVINSERTSTRUCTW tvis{};
	TVITEMW tvi{};

	tvis.hParent = parent_handle;
	tvis.hInsertAfter = TVI_SORT;
	tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvi.pszText = const_cast<wchar_t*>(folder->m_lpwTitle.c_str());
	tvi.lParam = reinterpret_cast<LPARAM>(folder);
	tvi.iImage = GetTreeViewIconIndex(folder->m_nIconId);
	tvi.iSelectedImage = 0;
#if !defined(NONAMELESSUNION)
	tvis.item = tvi;
#else
	tvis.DUMMYUNIONNAME.item = tvi;
#endif

	HTREEITEM hti = tree_view::insert_item(tree_view_handle, &tvis);
	if (has_selection)
		(void)tree_view::select_item(tree_view_handle, hti);

	return hti;
}

HTREEITEM InsertChildCustomFolders(HWND tree_view_handle, LPTREEFOLDER default_selection, HTREEITEM parent_handle, int parent_index)
{
	int num_folders = 0;
	HTREEITEM hti_child = nullptr;
	LPTREEFOLDER* folders = nullptr;

	GetFolders(&folders, &num_folders);

	for (size_t i = 0; i < num_folders; i++)
	{
		if (folders[i]->m_dwFlags & F_CUSTOM && folders[i]->m_nParent == parent_index)
			hti_child = InsertCustomFolder(tree_view_handle, parent_handle, folders[i], (folders[i] == default_selection));
	}
	return hti_child;
}

HTREEITEM InsertCustomFolders(HWND tree_view_handle, LPTREEFOLDER default_selection)
{
	int num_folders = 0;
	HTREEITEM hti_root = nullptr;
	LPTREEFOLDER* folders = nullptr;

	GetFolders(&folders, &num_folders);

	for (size_t i = 0; i < num_folders; i++)
	{
			if (folders[i]->m_dwFlags & F_CUSTOM && folders[i]->m_nParent == -1)
			{
				hti_root = InsertCustomFolder(tree_view_handle, TVI_ROOT, folders[i], (i == 0 || folders[i] == default_selection));
				(void)InsertChildCustomFolders(tree_view_handle, default_selection, hti_root, i);
			}
	}
	return hti_root;
}

INT_PTR CALLBACK AddCustomFileDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HWND tree_view_handle = dialog_boxes::get_dlg_item(hDlg, IDC_CUSTOM_TREE);
	static LPTREEFOLDER default_selection = nullptr;
	static int driver_index = 0;

	switch (Msg)
	{

	case WM_INITDIALOG:
	{
		HIMAGELIST treeview_icons = GetTreeViewIconList();

		// current game passed in using DialogBoxParam()
		driver_index = lParam;

		(void)tree_view::set_image_list(tree_view_handle, treeview_icons, LVSIL_NORMAL);
		(void)InsertCustomFolders(tree_view_handle, default_selection);
		(void)windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_CUSTOMFILE_GAME), driver_list::driver(driver_index).type.fullname());

		return static_cast<INT_PTR>(true);
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			TVITEM tvi;
			tvi.hItem = tree_view::get_selection(tree_view_handle);
			if (!tvi.hItem)
			{
				dialog_boxes::message_box(hDlg, L"Please select a folder to add the file to.", L"Add Custom File", MB_OK);
				return static_cast<INT_PTR>(true);
			}
			tvi.mask = TVIF_PARAM;
			if (tree_view::get_item(tree_view_handle, &tvi) == true)
			{
				/* should look for New... */
				default_selection = (LPTREEFOLDER)tvi.lParam; /* start here next time */
				AddToCustomFolder((LPTREEFOLDER)tvi.lParam, driver_index);
			}

			dialog_boxes::end_dialog(hDlg, 0);
			return static_cast<INT_PTR>(true);
		}
		case IDCANCEL:
			dialog_boxes::end_dialog(hDlg, 0);
			return static_cast<INT_PTR>(true);

		}
		break;
	}
	return static_cast<INT_PTR>(false);
}

INT_PTR CALLBACK DirectXDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
	{
		HWND hEdit;
		int text_length;
		std::wstring directx_help = std::wstring(&MAMEUINAME[0]) + L" requires DirectX version 9 or later.\r\n";
		hEdit = dialog_boxes::get_dlg_item(hDlg, IDC_DIRECTX_HELP);
		text_length = windows::get_window_text_length(hEdit);
		edit_control::set_sel(hEdit, text_length, text_length);
		edit_control::replace_sel(hEdit, directx_help.c_str());
		return 1;
	}
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == IDB_WEB_PAGE)
			shell::shell_execute(GetMainWindow(), nullptr, L"http://www.microsoft.com/directx", nullptr, nullptr, SW_SHOWNORMAL);

		if (LOWORD(wParam) == IDCANCEL || LOWORD(wParam) == IDB_WEB_PAGE)
			dialog_boxes::end_dialog(hDlg, 0);
		return 1;
	}
	}
	return 0;
}

/***************************************************************************
    private functions
 ***************************************************************************/

static void DisableFilterControls(HWND hWnd, LPCFOLDERDATA lpFilterRecord, LPCFILTER_ITEM lpFilterItem, DWORD dwFlags)
{
	HWND hWndCtrl = dialog_boxes::get_dlg_item(hWnd, lpFilterItem->m_dwCtrlID);
	DWORD dwFilterType = lpFilterItem->m_dwFilterType;

	/* Check the appropriate control */
	if (dwFilterType & dwFlags)
		button_control::set_check(hWndCtrl, MF_CHECKED);

	/* No special rules for this folder? */
	if (!lpFilterRecord)
		return;

	/* If this is an excluded filter */
	if (lpFilterRecord->m_dwUnset & dwFilterType)
	{
		/* uncheck it and disable the control */
		button_control::set_check(hWndCtrl, MF_UNCHECKED);
		(void)input::enable_window(hWndCtrl, false);
	}

	/* If this is an implied filter, check it and disable the control */
	if (lpFilterRecord->m_dwSet & dwFilterType)
	{
		button_control::set_check(hWndCtrl, MF_CHECKED);
		(void)input::enable_window(hWndCtrl, false);
	}
}

// Handle disabling mutually exclusive controls
static void EnableFilterExclusions(HWND hWnd, DWORD dwCtrlID)
{
	int i;

	for (i = 0; i < NUM_EXCLUSIONS; i++)
	{
		// is this control in the list?
		if (filterExclusion[i] == dwCtrlID)
		{
			// found the control id
			break;
		}
	}

	// if the control was found
	if (i < NUM_EXCLUSIONS)
	{
		DWORD id;
		// find the opposing control id
		if (i % 2)
			id = filterExclusion[i - 1];
		else
			id = filterExclusion[i + 1];

		// Uncheck the other control
		button_control::set_check(dialog_boxes::get_dlg_item(hWnd, id), MF_UNCHECKED);
	}
}

// Validate filter setting, mask out inappropriate filters for this folder
static DWORD ValidateFilters(LPCFOLDERDATA lpFilterRecord, DWORD dwFlags)
{
	DWORD dwFilters = 0;

	// No special cases - all filters apply
	if (!lpFilterRecord)
		return dwFlags;

	// Mask out implied and excluded filters
	dwFilters = lpFilterRecord->m_dwSet | lpFilterRecord->m_dwUnset;
	return dwFlags & ~dwFilters;
}

static void OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
	int value = 0;

	if (hwndCtl == dialog_boxes::get_dlg_item(hwnd, IDC_CYCLETIMESEC))
	{
		value = dialog_boxes::send_dlg_item_message(hwnd,IDC_CYCLETIMESEC, TBM_GETPOS, 0, 0);
		(void)dialog_boxes::send_dlg_item_message(hwnd,IDC_CYCLETIMESECTXT,WM_SETTEXT,0, (WPARAM)std::to_wstring(value).c_str());
	}
	else
	if (hwndCtl == dialog_boxes::get_dlg_item(hwnd, IDC_SCREENSHOT_BORDERSIZE))
	{
		value = dialog_boxes::send_dlg_item_message(hwnd,IDC_SCREENSHOT_BORDERSIZE, TBM_GETPOS, 0, 0);
		(void)dialog_boxes::send_dlg_item_message(hwnd,IDC_SCREENSHOT_BORDERSIZETXT,WM_SETTEXT,0, (WPARAM)std::to_wstring(value).c_str());
	}
}
