// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

/***************************************************************************

  columnedit.cpp

  Column Edit dialog

***************************************************************************/

// standard C++ headers
#include <filesystem>
#include <memory>
#include <string>

// standard windows headers
#include "winapi_common.h"
#include <uxtheme.h>

// MAME headers

// shlwapi.h pulls in objbase.h which defines interface as a struct which conflict with a template declaration in device.h
#ifdef interface
#undef interface // undef interface which is for COM interfaces
#endif
#include "emu.h"
#define interface struct

// MAMEUI headers
#include "windows_controls.h"
#include "dialog_boxes.h"
#include "windows_input.h"
#include "windows_messages.h"

#include "resource.h"
#include "mui_opts.h"
#include "screenshot.h"
#include "winui.h"

#include "columnedit.h"

using namespace mameui::winapi;
using namespace mameui::winapi::controls;

int shown[COLUMN_COUNT];
int order[COLUMN_COUNT];

// Returns true if successful
static int DoExchangeItem(HWND hFrom, HWND hTo, int nMinItem)
{
	LVITEMW lvi;
	wchar_t buf[80];

	lvi.iItem = list_view::get_next_item(hFrom, -1, LVIS_SELECTED | LVIS_FOCUSED);
	if (lvi.iItem < nMinItem)
	{
		if (lvi.iItem != -1) // Can't remove the first column
			(void)dialog_boxes::message_box(0, L"Cannot Move Selected Item", L"Move Item", IDOK);
		(void)input::set_focus(hFrom);
		return false;
	}
	lvi.iSubItem   = 0;
	lvi.mask       = LVIF_PARAM | LVIF_TEXT;
	lvi.pszText    = buf;
	lvi.cchTextMax = std::size(buf);
	if (list_view::get_item(hFrom, &lvi))
	{
		// Add this item to the Show and delete it from Available
		(void)list_view::delete_item(hFrom, lvi.iItem);
		lvi.iItem = list_view::get_item_count(hTo);
		(void)list_view::insert_item(hTo, &lvi);
		list_view::set_item_state(hTo, lvi.iItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		(void)input::set_focus(hTo);
		return lvi.iItem;
	}
	return false;
}

static void DoMoveItem( HWND hWnd, bool bDown)
{
	LVITEMW lvi;
	lvi.iItem = list_view::get_next_item(hWnd, -1, LVIS_SELECTED | LVIS_FOCUSED);
	int nMaxpos = list_view::get_item_count(hWnd);
	if (lvi.iItem == -1 ||
		(lvi.iItem <  2 && bDown == false) || // Disallow moving First column
		(lvi.iItem == 0 && bDown == true)  || // ""
		(lvi.iItem == nMaxpos - 1 && bDown == true))
	{
		(void)input::set_focus(hWnd);
		return;
	}

	wchar_t buf[80];
	lvi.iSubItem   = 0;
	lvi.mask       = LVIF_PARAM | LVIF_TEXT;
	lvi.pszText    = buf;
	lvi.cchTextMax = std::size(buf);
	if (list_view::get_item(hWnd, &lvi))
	{
		// Add this item to the Show and delete it from Available
		(void)list_view::delete_item(hWnd, lvi.iItem);
		lvi.iItem += (bDown) ? 1 : -1;
		(void)list_view::insert_item(hWnd,&lvi);
		list_view::set_item_state(hWnd, lvi.iItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		if (lvi.iItem == nMaxpos - 1)
			(void)input::enable_window(dialog_boxes::get_dlg_item(windows::get_parent(hWnd), IDC_BUTTONMOVEDOWN), false);
		else
			(void)input::enable_window(dialog_boxes::get_dlg_item(windows::get_parent(hWnd), IDC_BUTTONMOVEDOWN), true);

		if (lvi.iItem < 2)
			(void)input::enable_window(dialog_boxes::get_dlg_item(windows::get_parent(hWnd), IDC_BUTTONMOVEUP), false);
		else
			(void)input::enable_window(dialog_boxes::get_dlg_item(windows::get_parent(hWnd), IDC_BUTTONMOVEUP), true);

		(void)input::set_focus(hWnd);
	}
}

INT_PTR InternalColumnDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam,
	int nColumnMax, int *shown, int *order,
	const std::wstring *names, void (*pfnGetRealColumnOrder)(int *),
	void (*pfnGetColumnInfo)(int *pnOrder, int *pnShown),
	void (*pfnSetColumnInfo)(int *pnOrder, int *pnShown))
{
	static HWND hShown;
	static HWND hAvailable;
	static bool showMsg = false;
	int nShown = 0;
	int nAvail = 0;
	int i, nCount = 0;
	LVITEMW lvi;
	DWORD dwShowStyle, dwAvailableStyle, dwView = 0;

	switch (Msg)
	{
	case WM_INITDIALOG:
		hShown = dialog_boxes::get_dlg_item(hDlg, IDC_LISTSHOWCOLUMNS);
		hAvailable = dialog_boxes::get_dlg_item(hDlg, IDC_LISTAVAILABLECOLUMNS);
		/*Change Style to Always Show Selection */
		dwShowStyle = windows::get_window_long_ptr(hShown, GWL_STYLE);
		dwAvailableStyle = windows::get_window_long_ptr(hAvailable, GWL_STYLE);
		dwView = LVS_SHOWSELALWAYS | LVS_LIST;

		/* Only set the window style if the view bits have changed. */
		if ((dwShowStyle & LVS_TYPEMASK) != dwView)
		(void)windows::set_window_long_ptr(hShown, GWL_STYLE, (dwShowStyle & ~LVS_TYPEMASK) | dwView);
		if ((dwAvailableStyle & LVS_TYPEMASK) != dwView)
		(void)windows::set_window_long_ptr(hAvailable, GWL_STYLE, (dwAvailableStyle & ~LVS_TYPEMASK) | dwView);

		pfnGetColumnInfo(order, shown);

		showMsg = true;
		nShown = 0;
		nAvail = 0;

		lvi.mask = LVIF_TEXT | LVIF_PARAM;
		lvi.stateMask = 0;
		lvi.iSubItem  = 0;
		lvi.iImage = -1;

		/* Get the Column Order and save it */
		pfnGetRealColumnOrder(order);

		for (i = 0 ; i < nColumnMax; i++)
		{
			lvi.pszText = const_cast<wchar_t*>(names[order[i]].c_str());
			lvi.lParam = order[i];

			if (shown[order[i]])
			{
				lvi.iItem = nShown;
				(void)list_view::insert_item(hShown, &lvi);
				nShown++;
			}
			else
			{
				lvi.iItem = nAvail;
				(void)list_view::insert_item(hAvailable, &lvi);
				nAvail++;
			}
		}
		if( nShown > 0)
		{
			/*Set to Second, because first is not allowed*/
			list_view::set_item_state(hShown, 1, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		}
		if( nAvail > 0)
		{
			list_view::set_item_state(hAvailable, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		}
		(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONADD), true);
		return true;

	case WM_NOTIFY:
		{
			NMHDR *nm = (NMHDR *)lParam;
			NM_LISTVIEW *pnmv;
			int nPos = 0;

			switch (nm->code)
			{
			case NM_DBLCLK:
				// Do Data Exchange here, which ListView was double clicked?
				switch (nm->idFrom)
				{
				case IDC_LISTAVAILABLECOLUMNS:
					// Move selected Item from Available to Shown column
					nPos = DoExchangeItem(hAvailable, hShown, 0);
					if (nPos)
					{
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONADD),      false);
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONREMOVE),   true);
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEUP),   true);
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEDOWN), false);
					}
					break;

				case IDC_LISTSHOWCOLUMNS:
					// Move selected Item from Show to Available column
					if (DoExchangeItem(hShown, hAvailable, 1))
					{
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONADD),      true);
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONREMOVE),   false);
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEUP),   false);
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEDOWN), false);
					}
					break;
				}
				return true;

			case LVN_ITEMCHANGED:
				// Don't handle this message for now
				pnmv = (NM_LISTVIEW *)nm;
				if (//!(pnmv->uOldState & LVIS_SELECTED) &&
					(pnmv->uNewState  & LVIS_SELECTED))
				{
					if (pnmv->iItem == 0 && pnmv->hdr.idFrom == IDC_LISTSHOWCOLUMNS)
					{
						// Don't allow selecting the first item
						list_view::set_item_state(hShown, pnmv->iItem,
							LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
						if (showMsg)
						{
							(void)dialog_boxes::message_box(0, L"Changing this item is not permitted", L"Select Item", IDOK);
							showMsg = false;
						}
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONREMOVE),   false);
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEUP),   false);
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEDOWN), false);
						/*Leave Focus on Control*/
						//(void)input::set_focus(dialog_boxes::get_dlg_item(hDlg,IDOK));
						return true;
					}
					else
						showMsg = true;
				}
				if( pnmv->uOldState & LVIS_SELECTED && pnmv->iItem == 0 && pnmv->hdr.idFrom == IDC_LISTSHOWCOLUMNS )
				{
					/*we enable the buttons again, if the first Entry loses selection*/
					(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONREMOVE),   true);
					(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEUP),   true);
					(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEDOWN), true);
					//(void)input::set_focus(dialog_boxes::get_dlg_item(hDlg,IDOK));
				}
				break;
			case NM_SETFOCUS:
				{
					switch (nm->idFrom)
					{
					case IDC_LISTAVAILABLECOLUMNS:
						if (list_view::get_item_count(nm->hwndFrom) != 0)
						{
							(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONADD),      true);
							(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONREMOVE),   false);
							(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEDOWN), false);
							(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEUP),   false);
						}
						break;
					case IDC_LISTSHOWCOLUMNS:
						if (list_view::get_item_count(nm->hwndFrom) != 0)
						{
							(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONADD), false);

							if (list_view::get_next_item(hShown, -1, LVIS_SELECTED | LVIS_FOCUSED) == 0 )
							{
								(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONREMOVE),   false);
								(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEDOWN), false);
								(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEUP),   false);
							}
							else
							{
								(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONREMOVE),   true);
								(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEDOWN), true);
								(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEUP),   true);
							}
						}
						break;
					}
				}
				break;
			case LVN_KEYDOWN:
			case NM_CLICK:
				pnmv = (NM_LISTVIEW *)nm;
				if (//!(pnmv->uOldState & LVIS_SELECTED) &&
					(pnmv->uNewState  & LVIS_SELECTED))
				{
					if (pnmv->iItem == 0 && pnmv->hdr.idFrom == IDC_LISTSHOWCOLUMNS)
					{
					}
				}
				switch (nm->idFrom)
				{
				case IDC_LISTAVAILABLECOLUMNS:
					if (list_view::get_item_count(nm->hwndFrom) != 0)
					{
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONADD),      true);
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONREMOVE),   false);
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEDOWN), false);
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEUP),   false);
					}
					break;

				case IDC_LISTSHOWCOLUMNS:
					if (list_view::get_item_count(nm->hwndFrom) != 0)
					{
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONADD), false);
						if (list_view::get_next_item(hShown, -1, LVIS_SELECTED | LVIS_FOCUSED) == 0 )
						{
							(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONREMOVE),   false);
							(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEDOWN), false);
							(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEUP),   false);
						}
						else
						{
							(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONREMOVE),   true);
							(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEDOWN), true);
							(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEUP),   true);
						}
					}
					break;
				}
				//(void)input::set_focus( nm->hwndFrom );
				return true;
			}
		}
		return false;
	case WM_COMMAND:
		{
			WORD wID = LOWORD(wParam); //GET_WM_COMMAND_ID(wParam, lParam);
			HWND hWndCtrl = (HWND)lParam; //GET_WM_COMMAND_HWND(wParam, lParam);
			int  nPos = 0;

			switch (wID)
			{
				case IDC_LISTSHOWCOLUMNS:
					break;
				case IDC_BUTTONADD:
					// Move selected Item in Available to Shown
					nPos = DoExchangeItem(hAvailable, hShown, 0);
					if (nPos)
					{
						(void)input::enable_window(hWndCtrl,FALSE);
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONREMOVE),   true);
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEUP),   true);
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEDOWN), false);
					}
					break;

				case IDC_BUTTONREMOVE:
					// Move selected Item in Show to Available
					if (DoExchangeItem( hShown, hAvailable, 1))
					{
						(void)input::enable_window(hWndCtrl,FALSE);
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONADD),      true);
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEUP),   false);
						(void)input::enable_window(dialog_boxes::get_dlg_item(hDlg, IDC_BUTTONMOVEDOWN), false);
					}
					break;

				case IDC_BUTTONMOVEDOWN:
					// Move selected item in the Show window up 1 item
					DoMoveItem(hShown, true);
					break;

				case IDC_BUTTONMOVEUP:
					// Move selected item in the Show window down 1 item
					DoMoveItem(hShown, false);
					break;

				case IDOK:
					// Save users choices
					nShown = list_view::get_item_count(hShown);
					nAvail = list_view::get_item_count(hAvailable);
					nCount = 0;
					for (i = 0; i < nShown; i++)
					{
						lvi.iSubItem = 0;
						lvi.mask     = LVIF_PARAM;
						lvi.pszText  = 0;
						lvi.iItem    = i;
						(void)list_view::get_item(hShown, &lvi);
						order[nCount++]   = lvi.lParam;
						shown[lvi.lParam] = true;
					}
					for (i = 0; i < nAvail; i++)
					{
						lvi.iSubItem = 0;
						lvi.mask     = LVIF_PARAM;
						lvi.pszText  = 0;
						lvi.iItem    = i;
						(void)list_view::get_item(hAvailable, &lvi);
						order[nCount++]   = lvi.lParam;
						shown[lvi.lParam] = false;
					}
					pfnSetColumnInfo(order, shown);
					dialog_boxes::end_dialog(hDlg, 1);
					return true;

				case IDCANCEL:
					dialog_boxes::end_dialog(hDlg, 0);
					return true;
			}
		}
		break;
	}
	return 0;
}

static void GetColumnInfo(int *order, int *shown)
{
	GetColumnOrder(order);
	GetColumnShown(shown);
}

static void SetColumnInfo(int *order, int *shown)
{
	SetColumnOrder(order);
	SetColumnShown(shown);
}

INT_PTR CALLBACK ColumnDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	return InternalColumnDialogProc(hDlg, Msg, wParam, lParam, COLUMN_COUNT, shown, order, column_names, GetRealColumnOrder, GetColumnInfo, SetColumnInfo);
}

