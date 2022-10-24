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

// standard windows headers
#include "Windows.h"
#include "commctrl.h"
#include "commdlg.h"

// MAME headers
#include "mameheaders.h"

// MAMEUI headers
#include "mui_str.h"

#include "winapi_controls.h"
#include "winapi_dialog_boxes.h"
#include "winapi_input.h"
#include "winapi_menus.h"
#include "winapi_shell.h"
#include "winapi_storage.h"
#include "winapi_system_services.h"
#include "winapi_windows.h"

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

using namespace mameui::winapi;
using namespace mameui::winapi::controls;

struct combo_box_history_tab_t
{
	const wchar_t*  m_pText;
	const int       m_pData;
};

constexpr combo_box_history_tab_t g_ComboBoxHistoryTab[] =
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

static void DisableFilterControls(HWND hWnd, LPCFOLDERDATA lpFilterRecord, LPCFILTER_ITEM lpFilterItem, DWORD dwFlags);
static void EnableFilterExclusions(HWND hWnd, DWORD dwCtrlID);
static DWORD ValidateFilters(LPCFOLDERDATA lpFilterRecord, DWORD dwFlags);
static void OnHScroll(HWND hWnd, HWND hwndCtl, UINT code, int pos);

/***************************************************************************/

const char * GetFilterText(void)
{
	return g_FilterText.c_str();
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

				reset_message << MAMEUINAME << L" will now reset the following" << std::endl;
				reset_message << L"to the default settings:\n" << std::endl;

				if (resetDefaults)
					reset_message << L"Global game options" << std::endl;
				if (resetGames)
					reset_message << L"Individual game options" << std::endl;
				if (resetFilters)
					reset_message << L"Custom folder filters" << std::endl;
				if (resetUI)
				{
					reset_message << L"User interface settings\n" << std::endl;
					reset_message << L"Resetting the User Interface options" << std::endl;
					reset_message << L"requires exiting " << std::endl;
					reset_message << MAMEUINAME << L"." << std::endl;
				}
				reset_message << L"\nDo you wish to continue?" << std::flush;
				if (dialog_boxes::message_box(hDlg, reset_message.str().c_str(), L"Restore Settings", IDOK) == IDOK)
				{
					if (resetFilters)
						ResetFilters();

					if (resetGames)
						ResetAllGameOptions();

					if (resetDefaults)
						ResetGameDefaults();

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
		button_control::set_check(dialog_boxes::get_dlg_item(hDlg,IDC_UI_SKIP_WARNINGS),GetSkipWarnings());
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
			const char* snapname = GetSnapName();
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
			SetSkipWarnings(button_control::get_check(dialog_boxes::get_dlg_item(hDlg, IDC_UI_SKIP_WARNINGS)));
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
				windows::post_message(GetMainWindow(),WM_COMMAND, MAKEWPARAM(ID_VIEW_LINEUPICONS, false),(LPARAM)NULL);
			}
			checked = button_control::get_check(dialog_boxes::get_dlg_item(hDlg,IDC_NOOFFSET_CLONES));
			if (checked != GetOffsetClones())
			{
				SetOffsetClones(checked);
				// LineUpIcons does just a ResetListView(), which is what we want here
				windows::post_message(GetMainWindow(),WM_COMMAND, MAKEWPARAM(ID_VIEW_LINEUPICONS, false),(LPARAM)NULL);
			}
			nCurSelection = combo_box::get_cur_sel(dialog_boxes::get_dlg_item(hDlg,IDC_SNAPNAME));
			if (nCurSelection != CB_ERR)
			{
				const char* snapname_selection = (const char*)combo_box::get_item_data(dialog_boxes::get_dlg_item(hDlg,IDC_SNAPNAME), nCurSelection);
				if (snapname_selection)
					SetSnapName(snapname_selection);
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
	static LPCFOLDERDATA lpFilterRecord;
	std::string strText;
	int i = 0;

	switch (Msg)
	{
	case WM_INITDIALOG:
	{
		LPTREEFOLDER folder = GetCurrentFolder();
		LPTREEFOLDER lpParent = NULL;
		LPCFILTER_ITEM g_lpFilterList = GetFilterList();

		dwFilters = 0;

		if (folder)
		{
			char tmp[80];

			(void)windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_EDIT), g_FilterText.c_str());
			edit_control::set_sel(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_EDIT), 0, -1);
			// Mask out non filter flags
			dwFilters = folder->m_dwFlags & F_MASK;
			// Display current folder name in dialog titlebar
			snprintf(tmp,std::size(tmp),"Filters for %s Folder",folder->m_lpTitle);
			(void)windows::set_window_text_utf8(hDlg, tmp);
			if ( GetFilterInherit() )
			{
				bool bShowExplanation = false;
				lpParent = GetFolder( folder->m_nParent );
				if( lpParent )
				{
					/* Check the Parent Filters and inherit them on child,
					 * No need to promote all games to parent folder, works as is */
					dwpFilters = lpParent->m_dwFlags & F_MASK;
					/*Check all possible Filters if inherited solely from parent, e.g. not being set explicitly on our folder*/
					if( (dwpFilters & F_CLONES) && !(dwFilters & F_CLONES) )
					{
						/*Add a Specifier to the Checkbox to show it was inherited from the parent*/
						strText = windows::get_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_CLONES));
						strText.append(" (*)");
						(void)windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_CLONES), strText.c_str());
						bShowExplanation = true;
					}
					if( (dwpFilters & F_NONWORKING) && !(dwFilters & F_NONWORKING) )
					{
						/*Add a Specifier to the Checkbox to show it was inherited from the parent*/
						strText = windows::get_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_NONWORKING));
						strText.append(" (*)");
						(void)windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_NONWORKING), strText.c_str());
						bShowExplanation = true;
					}
					if( (dwpFilters & F_UNAVAILABLE) && !(dwFilters & F_UNAVAILABLE) )
					{
						/*Add a Specifier to the Checkbox to show it was inherited from the parent*/
						strText = windows::get_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_UNAVAILABLE));
						strText.append(" (*)");
						(void)windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_UNAVAILABLE), strText.c_str());
						bShowExplanation = true;
					}
					if( (dwpFilters & F_VECTOR) && !(dwFilters & F_VECTOR) )
					{
						/*Add a Specifier to the Checkbox to show it was inherited from the parent*/
						strText = windows::get_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_VECTOR));
						strText.append(" (*)");
						(void)windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_VECTOR), strText.c_str());
						bShowExplanation = true;
					}
					if( (dwpFilters & F_RASTER) && !(dwFilters & F_RASTER) )
					{
						/*Add a Specifier to the Checkbox to show it was inherited from the parent*/
						strText = windows::get_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_RASTER));
						strText.append(" (*)");
						(void)windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_RASTER), strText.c_str());
						bShowExplanation = true;
					}
					if( (dwpFilters & F_ORIGINALS) && !(dwFilters & F_ORIGINALS) )
					{
						/*Add a Specifier to the Checkbox to show it was inherited from the parent*/
						strText = windows::get_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_ORIGINALS));
						strText.append(" (*)");
						(void)windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_ORIGINALS), strText.c_str());
						bShowExplanation = true;
					}
					if( (dwpFilters & F_WORKING) && !(dwFilters & F_WORKING) )
					{
						/*Add a Specifier to the Checkbox to show it was inherited from the parent*/
						strText = windows::get_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_WORKING));
						strText.append(" (*)");
						(void)windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_WORKING), strText.c_str());
						bShowExplanation = true;
					}
					if( (dwpFilters & F_AVAILABLE) && !(dwFilters & F_AVAILABLE) )
					{
						/*Add a Specifier to the Checkbox to show it was inherited from the parent*/
						strText = windows::get_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_AVAILABLE));
						strText.append(" (*)");
						(void)windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_AVAILABLE), strText.c_str());
						bShowExplanation = true;
					}
					if( (dwpFilters & F_HORIZONTAL) && !(dwFilters & F_HORIZONTAL) )
					{
						/*Add a Specifier to the Checkbox to show it was inherited from the parent*/
						strText = windows::get_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_HORIZONTAL));
						strText.append(" (*)");
						(void)windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_HORIZONTAL), strText.c_str());
						bShowExplanation = true;
					}
					if( (dwpFilters & F_VERTICAL) && !(dwFilters & F_VERTICAL) )
					{
						/*Add a Specifier to the Checkbox to show it was inherited from the parent*/
						strText = windows::get_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_VERTICAL));
						strText.append(" (*)");
						(void)windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_VERTICAL), strText.c_str());
						bShowExplanation = true;
					}
					if( (dwpFilters & F_ARCADE) && !(dwFilters & F_ARCADE) )
					{
						/*Add a Specifier to the Checkbox to show it was inherited from the parent*/
						strText = windows::get_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_ARCADE));
						strText.append(" (*)");
						(void)windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_ARCADE), strText.c_str());
						bShowExplanation = true;
					}
					if( (dwpFilters & F_MESS) && !(dwFilters & F_MESS) )
					{
						/*Add a Specifier to the Checkbox to show it was inherited from the parent*/
						strText = windows::get_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_MESS));
						strText.append(" (*)");
						(void)windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_MESS), strText.c_str());
						bShowExplanation = true;
					}
					/*Do not or in the Values of the parent, so that the values of the folder still can be set*/
					//dwFilters |= dwpFilters;
				}
				if( ! bShowExplanation )
				{
					(void)windows::show_window(dialog_boxes::get_dlg_item(hDlg, IDC_INHERITED), false );
				}
			}
			else
				(void)windows::show_window(dialog_boxes::get_dlg_item(hDlg, IDC_INHERITED), false );

			// Find the matching filter record if it exists
			lpFilterRecord = FindFilter(folder->m_nFolderId);

			// initialize and disable appropriate controls
			for (i = 0; g_lpFilterList[i].m_dwFilterType; i++)
				DisableFilterControls(hDlg, lpFilterRecord, &g_lpFilterList[i], dwFilters);
		}
		(void)input::set_focus(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_EDIT));
		return false;
	}
	case WM_HELP:
		// User clicked the ? from the upper right on a control
		HelpFunction((HWND)((LPHELPINFO)lParam)->hItemHandle, MAMEUICONTEXTHELP,
			 HH_TP_HELP_WM_HELP, GetHelpIDs());
		break;

	case WM_CONTEXTMENU:
		HelpFunction((HWND)wParam, MAMEUICONTEXTHELP, HH_TP_HELP_CONTEXTMENU, GetHelpIDs());
		break;

	case WM_COMMAND:
	{
		WORD wID = LOWORD(wParam); //GET_WM_COMMAND_ID(wParam, lParam);
		WORD wNotifyCode = HIWORD(wParam); //GET_WM_COMMAND_CMD(wParam, lParam);
		LPTREEFOLDER folder = GetCurrentFolder();
		LPCFILTER_ITEM g_lpFilterList = GetFilterList();

		switch (wID)
		{
		case IDOK:
			dwFilters = 0;

			g_FilterText = windows::get_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_FILTER_EDIT));

			// see which buttons are checked
			for (i = 0; g_lpFilterList[i].m_dwFilterType; i++)
				if (button_control::get_check(dialog_boxes::get_dlg_item(hDlg, g_lpFilterList[i].m_dwCtrlID)))
					dwFilters |= g_lpFilterList[i].m_dwFilterType;

			// Mask out invalid filters
			dwFilters = ValidateFilters(lpFilterRecord, dwFilters);

			// Keep non filter flags
			folder->m_dwFlags &= ~F_MASK;

			// put in the set filters
			folder->m_dwFlags |= dwFilters;

			dialog_boxes::end_dialog(hDlg, 1);
			return true;

		case IDCANCEL:
			dialog_boxes::end_dialog(hDlg, 0);
			return true;

		default:
			// Handle unchecking mutually exclusive filters
			if (wNotifyCode == BN_CLICKED)
				EnableFilterExclusions(hDlg, wID);
		}
	}
	break;
	}
	return 0;
}

INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		{
			HBITMAP hBmp;
			hBmp = (HBITMAP)menus::load_image(system_services::get_module_handle(0), menus::make_int_resource(IDB_ABOUT), IMAGE_BITMAP, 0, 0, LR_SHARED);
			(void)dialog_boxes::send_dlg_item_message(hDlg, IDC_ABOUT, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBmp);
			(void)windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_VERSION), GetVersionString());
		}
		return 1;

	case WM_COMMAND:
		dialog_boxes::end_dialog(hDlg, 0);
		return 1;
	}
	return 0;
}

INT_PTR CALLBACK AddCustomFileDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HWND tree_view_handle = dialog_boxes::get_dlg_item(hDlg, IDC_CUSTOM_TREE);
	static LPTREEFOLDER default_selection = NULL;
	static int driver_index = 0;

	switch (Msg)
	{

	case WM_INITDIALOG:
	{
		TREEFOLDER **folders;
		int num_folders = 0;
		int i = 0;
		TVINSERTSTRUCTW tvis;
		TVITEM tvi;
		bool first_entry = true;
		HIMAGELIST treeview_icons = GetTreeViewIconList();

		// current game passed in using DialogBoxParam()
		driver_index = lParam;

		(void)tree_view::set_image_list(tree_view_handle, treeview_icons, LVSIL_NORMAL);

		GetFolders(&folders,&num_folders);

		// should add "New..."

		// insert custom folders into our tree view
		for (i=0;i<num_folders;i++)
		{
			if (folders[i]->m_dwFlags & F_CUSTOM)
			{
				HTREEITEM hti;
				int jj = 0;

				if (folders[i]->m_nParent == -1)
				{
					tvi = {};
					tvis.hParent = TVI_ROOT;
					tvis.hInsertAfter = TVI_SORT;
					tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
					tvi.pszText = folders[i]->m_lpwTitle;
					tvi.lParam = (LPARAM)folders[i];
					tvi.iImage = GetTreeViewIconIndex(folders[i]->m_nIconId);
					tvi.iSelectedImage = 0;
#if !defined(NONAMELESSUNION)
					tvis.item = tvi;
#else
					tvis.DUMMYUNIONNAME.item = tvi;
#endif

					hti = tree_view::insert_item(tree_view_handle,&tvis);

					/* look for children of this custom folder */
					for (jj=0; jj<num_folders; jj++)
					{
						if (folders[jj]->m_nParent == i)
						{
							HTREEITEM hti_child;
							tvis.hParent = hti;
							tvis.hInsertAfter = TVI_SORT;
							tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
							tvi.pszText = folders[jj]->m_lpwTitle;
							tvi.lParam = (LPARAM)folders[jj];
							tvi.iImage = GetTreeViewIconIndex(folders[jj]->m_nIconId);
							tvi.iSelectedImage = 0;
#if !defined(NONAMELESSUNION)
							tvis.item = tvi;
#else
							tvis.DUMMYUNIONNAME.item = tvi;
#endif
							hti_child = tree_view::insert_item(tree_view_handle,&tvis);
							if (folders[jj] == default_selection)
								(void)tree_view::select_item(tree_view_handle,hti_child);
						}
					}

					/*TreeView_Expand(tree_view_handle,hti,TVE_EXPAND);*/
					if (first_entry || folders[i] == default_selection)
					{
						(void)tree_view::select_item(tree_view_handle, hti);
						first_entry = false;
					}
				}
			}
		}

		(void)windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg,IDC_CUSTOMFILE_GAME), driver_list::driver(driver_index).type.fullname());

		return true;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			{
				TVITEM tvi;
				tvi.hItem = tree_view::get_selection(tree_view_handle);
				tvi.mask = TVIF_PARAM;
				if (tree_view::get_item(tree_view_handle,&tvi) == true)
				{
					/* should look for New... */
					default_selection = (LPTREEFOLDER)tvi.lParam; /* start here next time */
					AddToCustomFolder((LPTREEFOLDER)tvi.lParam,driver_index);
				}

				dialog_boxes::end_dialog(hDlg, 0);
				return true;
			}
		case IDCANCEL:
			dialog_boxes::end_dialog(hDlg, 0);
			return true;

		}
		break;
	}
	return 0;
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
			shell::shell_execute(GetMainWindow(), NULL, L"http://www.microsoft.com/directx", NULL, NULL, SW_SHOWNORMAL);

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

	if (lpFilterRecord != (LPFOLDERDATA)0)
	{
		// Mask out implied and excluded filters
		dwFilters = lpFilterRecord->m_dwSet | lpFilterRecord->m_dwUnset;
		return dwFlags & ~dwFilters;
	}

	// No special cases - all filters apply
	return dwFlags;
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
