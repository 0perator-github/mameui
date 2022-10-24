// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

/***************************************************************************

  picker.cpp

 ***************************************************************************/

// standard C++ headers
#include <filesystem>
#include <iostream>
#include <string>

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
#include "mui_cstr.h"
#include "mui_wcstr.h"

#include "menus_other_res.h"
#include "system_services.h"
#include "windows_controls.h"
#include "windows_gdi.h"
#include "windows_input.h"
#include "windows_messages.h"

#include "bitmask.h"
#include "emu_opts.h"
#include "mui_opts.h"
#include "screenshot.h"
#include "treeview.h"
#include "winui.h"

#include "gamepicker.h"

using namespace mameui::util::string_util;
using namespace mameui::winapi::controls;
using namespace mameui::winapi;

// fix warning: cast does not match function type
#if defined(__GNUC__) && defined(ListView_GetHeader)
#undef ListView_GetHeader
#endif
#if defined(__GNUC__) && defined(ListView_GetImageList)
#undef ListView_GetImageList
#undef ListView_GetItemRect
#endif

#ifndef ListView_GetItemRect
template<typename T1, typename T2, typename T3, typename T4>
constexpr auto ListView_GetItemRect(T1 hwnd, T2  i, T3  prc, T4  code) {
	return (BOOL)windows::send_message((hwnd), LVM_GETITEMRECT, (WPARAM)(int)(i), ((prc) ? (((RECT*)(prc))->left = (code), (LPARAM)(RECT*)(prc)) : (LPARAM)(RECT*)nullptr));
}
#endif

#ifndef ListView_GetImageList
template<typename T1, typename T2>
constexpr auto ListView_GetImageList(T1 w, T2 i) {
	return (HIMAGELIST)(LRESULT)(int)windows::send_message((w), LVM_GETIMAGELIST, (i), 0);
}

#endif // ListView_GetImageList

#ifndef ListView_GetHeader
template<typename T1>
constexpr auto ListView_GetImageList(T1 w) {
	return (HWND)(LRESULT)(int)windows::send_message((w), LVM_GETHEADER, 0, 0);
}
#endif // ListView_GetHeader

#ifndef HDM_SETIMAGELIST
constexpr auto HDM_SETIMAGELIST = (HDM_FIRST + 8);
#endif // HDM_SETIMAGELIST

#ifndef Header_SetImageList
#define Header_SetImageList(h,i) (HIMAGELIST)(LRESULT)(int)SNDMSG((h), HDM_SETIMAGELIST, 0, (LPARAM)i)
template<typename T1, typename T2>
constexpr auto Header_SetImageList(T1 w, T2 i) {
	return (HIMAGELIST)(LRESULT)(int)windows::send_message((h), HDM_SETIMAGELIST, 0, (LPARAM)i);
}
#endif // Header_SetImageList

#ifndef HDF_SORTUP
constexpr auto HDF_SORTUP = 0x400;
#endif

#ifndef HDF_SORTDOWN
constexpr auto  HDF_SORTDOWN = 0x200;
#endif

using PickerInfo = struct picker_info
{
	const PickerCallbacks *pCallbacks;
	WNDPROC pfnParentWndProc;
	int nCurrentViewID;
	int nLastItem;
	int nColumnCount;
	int *pnColumnsShown;
	int *pnColumnsOrder;
	UINT_PTR nTimer;
	const std::wstring *column_names;
};

using CompareProcParams = struct compare_proc_params
{
	HWND hwndPicker;
	PickerInfo *pPickerInfo;
	int nSortColumn;
	int nViewMode;
	bool bReverse;
};

static PickerInfo *GetPickerInfo(HWND hWnd)
{
	PickerInfo *lpPickerInfo = reinterpret_cast<PickerInfo*>(windows::get_window_long_ptr(hWnd, GWLP_USERDATA));
	return lpPickerInfo;
}


static LRESULT CallParentWndProc(WNDPROC pfnParentWndProc, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT rc;

	if (!pfnParentWndProc)
		pfnParentWndProc = GetPickerInfo(hWnd)->pfnParentWndProc;

	rc = windows::call_window_proc(pfnParentWndProc, hWnd, message, wParam, lParam);
	return rc;
}

static bool ListViewOnErase(HWND hWnd, HDC hDC)
{
	MYBITMAPINFO *pbmDesc = GetBackgroundInfo();
	HBITMAP hBackground = GetBackgroundBitmap();
	HPALETTE hPALbg = GetBackgroundPalette();

	// this does not draw the background properly in report view

	RECT rcClient;
	(void)windows::get_client_rect(hWnd, &rcClient);

	HDC htempDC = gdi::create_compatible_dc(hDC);
	HBITMAP hOldBitmap = (HBITMAP)gdi::select_object(htempDC, hBackground);

	HRGN rgnBitmap = gdi::create_rect_rgn_indirect(&rcClient);
	gdi::select_clip_rgn(hDC, rgnBitmap);
	(void)gdi::delete_bitmap((HBITMAP)rgnBitmap);

	HPALETTE hPAL = (!hPALbg) ? gdi::create_half_tone_palette(hDC) : hPALbg;

	if (gdi::get_device_caps(htempDC, RASTERCAPS) & RC_PALETTE && hPAL != nullptr)
	{
		gdi::select_palette(htempDC, hPAL, false);
		gdi::realize_palette(htempDC);
	}

	// Get x and y offset
	POINT pt = {0,0};
	gdi::map_window_points(hWnd, GetTreeView(), &pt, 1);
	POINT ptOrigin;
	gdi::get_dc_org_ex(hDC, &ptOrigin);
	ptOrigin.x -= pt.x;
	ptOrigin.y -= pt.y;
	ptOrigin.x = -scroll_bar::get_scroll_pos(hWnd, SB_HORZ);
	ptOrigin.y = -scroll_bar::get_scroll_pos(hWnd, SB_VERT);

	if (pbmDesc->bmWidth && pbmDesc->bmHeight)
		for (LONG i = ptOrigin.x; i < rcClient.right; i += pbmDesc->bmWidth)
			for (LONG j = ptOrigin.y; j < rcClient.bottom; j += pbmDesc->bmHeight)
				(void)gdi::bit_blt(hDC, i, j, pbmDesc->bmWidth, pbmDesc->bmHeight, htempDC, 0, 0, SRCCOPY);

	(void)gdi::select_object(htempDC, hOldBitmap);
	(void)gdi::delete_dc(htempDC);

	if (!pbmDesc->bmColors)
	{
		(void)gdi::delete_palette(hPAL);
		hPAL = 0;
	}

	return true;
}



static bool ListViewNotify(HWND hWnd, LPNMHDR lpNmHdr)
{
	// This code is for using bitmap in the background
	// Invalidate the right side of the control when a column is resized
	if (lpNmHdr->code == HDN_ITEMCHANGINGA || lpNmHdr->code == HDN_ITEMCHANGINGW)
	{
		DWORD dwPos = windows::get_message_pos();
		POINT pt;
		pt.x = LOWORD(dwPos);
		pt.y = HIWORD(dwPos);

		RECT rcClient;
		(void)windows::get_client_rect(hWnd, &rcClient);
		(void)gdi::screen_to_client(hWnd, &pt);
		rcClient.left = pt.x;
		gdi::invalidate_rect(hWnd, &rcClient, false);
	}
	return false;
}



static bool ListViewContextMenu(HWND hwndPicker, LPARAM lParam)
{
	PickerInfo *pPickerInfo;
	pPickerInfo = GetPickerInfo(hwndPicker);

	// Extract the point out of the lparam
	POINT pt;
	pt.x = (int)(short)LOWORD(lParam); //GET_X_LPARAM(lParam);
	pt.y = (int)(short)HIWORD(lParam); //GET_Y_LPARAM(lParam);
	if (pt.x < 0 && pt.y < 0)
		menus::get_cursor_pos(&pt);

	// Figure out which header column was clicked, if at all
	int nViewID = Picker_GetViewID(hwndPicker);
	int nColumn = -1;

	if ((nViewID == VIEW_REPORT) || (nViewID == VIEW_GROUPED))
	{
		HWND hwndHeader = list_view::get_header(hwndPicker);
		POINT headerPt = pt;
		(void)gdi::screen_to_client(hwndHeader, &headerPt);

		RECT rcCol;
		for (size_t i = 0; header_control::get_item_rect(hwndHeader, i, &rcCol); i++)
		{
			if (gdi::pt_in_rect(&rcCol, headerPt))
			{
				nColumn = i;
				break;
			}
		}
	}

	if (nColumn >= 0)
	{
		// A column header was clicked
		if (pPickerInfo->pCallbacks->pfnOnHeaderContextMenu)
			pPickerInfo->pCallbacks->pfnOnHeaderContextMenu(pt, nColumn);
	}
	else
	{
		// The body was clicked
		if (pPickerInfo->pCallbacks->pfnOnBodyContextMenu)
			pPickerInfo->pCallbacks->pfnOnBodyContextMenu(pt);
	}
	return true;
}



static void Picker_Free(PickerInfo *pPickerInfo)
{
	// Free up all resources associated with this picker structure
	if (pPickerInfo->pnColumnsShown)
		delete[] pPickerInfo->pnColumnsShown;
	if (pPickerInfo->pnColumnsOrder)
		delete[] pPickerInfo->pnColumnsOrder;
	delete pPickerInfo;
}



static LRESULT CALLBACK ListViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PickerInfo *pPickerInfo;
	LRESULT rc = 0;
	bool bHandled = false;
	WNDPROC pfnParentWndProc;
	HWND hwndHeaderCtrl  = nullptr;
	HFONT hHeaderCtrlFont = nullptr;

	pPickerInfo = GetPickerInfo(hWnd);
	pfnParentWndProc = pPickerInfo->pfnParentWndProc;

	switch(message)
	{
		case WM_MOUSEMOVE:
			if (MouseHasBeenMoved())
				menus::show_cursor(true);
			break;

		case WM_ERASEBKGND:
			if (GetBackgroundBitmap())
			{
				rc = ListViewOnErase(hWnd, (HDC) wParam);
				bHandled = true;
			}
			break;

		case WM_NOTIFY:
			bHandled = ListViewNotify(hWnd, (LPNMHDR) lParam);
			break;

		case WM_SETFONT:
			hwndHeaderCtrl = list_view::get_header(hWnd);
			if (hwndHeaderCtrl)
				hHeaderCtrlFont = windows::get_window_font(hwndHeaderCtrl);
			break;

		case WM_CONTEXTMENU:
			bHandled = ListViewContextMenu(hWnd, lParam);
			break;

		case WM_DESTROY:
			// Received WM_DESTROY; time to clean up
			if (pPickerInfo->pCallbacks->pfnSetViewMode)
				pPickerInfo->pCallbacks->pfnSetViewMode(pPickerInfo->nCurrentViewID);
			Picker_Free(pPickerInfo);
			(void)windows::set_window_long_ptr(hWnd, GWLP_WNDPROC, (LONG_PTR) pfnParentWndProc);
			(void)windows::set_window_long_ptr(hWnd, GWLP_USERDATA, (LONG_PTR)nullptr);
			break;
	}

	if (!bHandled)
		rc = CallParentWndProc(pfnParentWndProc, hWnd, message, wParam, lParam);

	 // If we received WM_SETFONT, reset header ctrl font back to original font
	if (hwndHeaderCtrl)
		windows::set_window_font(hwndHeaderCtrl, hHeaderCtrlFont, true);

	return rc;
}



// Re/initialize the ListControl Columns
static void Picker_InternalResetColumnDisplay(HWND hWnd, bool bFirstTime)
{
	LVCOLUMNW   lvc;
	int         i = 0;
	int         nColumn = 0;
	LVCOLUMNW col;
	PickerInfo *pPickerInfo;

	pPickerInfo = GetPickerInfo(hWnd);

	int *widths, *order, *shown;
	widths = new int[pPickerInfo->nColumnCount];
	order = new int[pPickerInfo->nColumnCount];
	shown = new int[pPickerInfo->nColumnCount];
	if (!widths || !order || !shown)
		goto done;

	pPickerInfo->pCallbacks->pfnGetColumnWidths(widths);
	pPickerInfo->pCallbacks->pfnGetColumnOrder(order);
	pPickerInfo->pCallbacks->pfnGetColumnShown(shown);

	if (!bFirstTime)
	{
		DWORD style = windows::get_window_long_ptr(hWnd, GWL_STYLE);

		// switch the list view to LVS_REPORT style so column widths reported correctly
		(void)windows::set_window_long_ptr(hWnd, GWL_STYLE, (windows::get_window_long_ptr(hWnd, GWL_STYLE) & ~LVS_TYPEMASK) | LVS_REPORT);

		// Retrieve each of the column widths
		i = 0;
		col = {};
		col.mask = LVCF_WIDTH;
		while(list_view::get_column(hWnd, 0, &col))
		{
			nColumn = Picker_GetRealColumnFromViewColumn(hWnd, i++);
			widths[nColumn] = col.cx;
			(void)list_view::delete_column(hWnd, 0);
		}

		pPickerInfo->pCallbacks->pfnSetColumnWidths(widths);

		// restore old style
		(void)windows::set_window_long_ptr(hWnd, GWL_STYLE, style);
	}

	nColumn = 0;
	for (i = 0; i < pPickerInfo->nColumnCount; i++)
	{
		if (shown[order[i]])
		{
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_TEXT;
			lvc.pszText = const_cast<wchar_t*>(pPickerInfo->column_names[order[i]].c_str());
			lvc.iSubItem = nColumn;
			lvc.cx = widths[order[i]];
			lvc.fmt = LVCFMT_LEFT;
			if (lvc.pszText && *lvc.pszText) // column name cannot be blank
			{
				(void)list_view::insert_column(hWnd, nColumn, &lvc);
				pPickerInfo->pnColumnsOrder[nColumn] = order[i];
//              std::cout << "Visible column " << nColumn << ": Logical column " << order[i] << "; Width=" << widths[order[i]] << "\n";
				nColumn++;
			}
		}
	}

	//shown_columns = nColumn;

	// Fill this in so we can still sort on columns NOT shown
	for (i = 0; i < pPickerInfo->nColumnCount && nColumn < pPickerInfo->nColumnCount; i++)
	{
		if (!shown[order[i]])
		{
			pPickerInfo->pnColumnsOrder[nColumn] = order[i];
			nColumn++;
		}
	}

	if (GetListFontColor() == RGB(255, 255, 255))
		(void)list_view::set_text_color(hWnd, RGB(240, 240, 240));
	else
		(void)list_view::set_text_color(hWnd, GetListFontColor());

done:
	if (widths)
		delete[] widths;
	if (order)
		delete[] order;
	if (shown)
		delete[] shown;
}



void Picker_ResetColumnDisplay(HWND hWnd)
{
	Picker_InternalResetColumnDisplay(hWnd, false);
}



void Picker_ClearIdle(HWND hwndPicker)
{
	PickerInfo *pPickerInfo;

	pPickerInfo = GetPickerInfo(hwndPicker);
	if (pPickerInfo->nTimer)
	{
		KillTimer(hwndPicker, 0);
		pPickerInfo->nTimer = 0;
	}
}



static void CALLBACK Picker_TimerProc(HWND hwndPicker, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	PickerInfo *pPickerInfo;
	DWORD nTickCount = 0;
	DWORD nMaxIdleTicks = 50;

	pPickerInfo = GetPickerInfo(hwndPicker);
	bool bContinueIdle = false;
	DWORD nBaseTickCount = system_services::get_tick_count();

	// This idle procedure will loop until either idling is over, or until
	// a specified amount of time elapses (in this case, 50ms).  This frees
	// idle callbacks of any responsibility for balancing their workloads; the
	// picker code will
	do
	{
		if (pPickerInfo->pCallbacks->pfnOnIdle)
			bContinueIdle = pPickerInfo->pCallbacks->pfnOnIdle(hwndPicker);
		nTickCount = system_services::get_tick_count();
	}
	while(bContinueIdle && ((nTickCount - nBaseTickCount) < nMaxIdleTicks));

	if (!bContinueIdle)
		Picker_ClearIdle(hwndPicker);
}



// Instructs this picker to reset idling; idling will continue until the
// idle function returns false
void Picker_ResetIdle(HWND hwndPicker)
{
	PickerInfo *pPickerInfo;

	pPickerInfo = GetPickerInfo(hwndPicker);

	Picker_ClearIdle(hwndPicker);
	if (pPickerInfo->pCallbacks->pfnOnIdle)
		pPickerInfo->nTimer = SetTimer(hwndPicker, 0, 0, Picker_TimerProc);
}



bool Picker_IsIdling(HWND hwndPicker)
{
	PickerInfo *pPickerInfo;
	pPickerInfo = GetPickerInfo(hwndPicker);
	return pPickerInfo->nTimer != 0;
}



bool SetupPicker(HWND hwndPicker, const PickerOptions *pOptions)
{
	PickerInfo *pPickerInfo;
	int i = 0;
	LONG_PTR l = 0;

	//assert(hwndPicker);

	// Allocate the list view struct
	pPickerInfo = new PickerInfo{};
	if (!pPickerInfo)
		return false;

	// And fill it out
	pPickerInfo->pCallbacks = pOptions->pCallbacks;
	pPickerInfo->nColumnCount = pOptions->nColumnCount;
	pPickerInfo->column_names = pOptions->column_names;
	pPickerInfo->nLastItem = -1;

	if (pPickerInfo->nColumnCount)
	{
		// Allocate space for the column order and columns shown array
		pPickerInfo->pnColumnsOrder = new int[pPickerInfo->nColumnCount];
		pPickerInfo->pnColumnsShown = new int[pPickerInfo->nColumnCount];
		if (!pPickerInfo->pnColumnsOrder || !pPickerInfo->pnColumnsShown)
			goto error;

		// set up initial values
		for (i = 0; i < pPickerInfo->nColumnCount; i++)
		{
			pPickerInfo->pnColumnsOrder[i] = i;
			pPickerInfo->pnColumnsShown[i] = true;
		}

		if (pPickerInfo->pCallbacks->pfnGetColumnOrder)
			pPickerInfo->pCallbacks->pfnGetColumnOrder(pPickerInfo->pnColumnsOrder);
		if (pPickerInfo->pCallbacks->pfnGetColumnShown)
			pPickerInfo->pCallbacks->pfnGetColumnShown(pPickerInfo->pnColumnsShown);
	}

	// Hook in our wndproc and userdata pointer
	l = windows::get_window_long_ptr(hwndPicker, GWLP_WNDPROC);
	pPickerInfo->pfnParentWndProc = (WNDPROC) l;
	(void)windows::set_window_long_ptr(hwndPicker, GWLP_USERDATA, (LONG_PTR)pPickerInfo);
	(void)windows::set_window_long_ptr(hwndPicker, GWLP_WNDPROC, (LONG_PTR)ListViewWndProc);

	(void)list_view::set_extended_list_view_style(hwndPicker, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP |
		LVS_EX_UNDERLINEHOT | LVS_EX_UNDERLINECOLD | LVS_EX_LABELTIP);

	Picker_InternalResetColumnDisplay(hwndPicker, true);
	Picker_ResetIdle(hwndPicker);
	return true;

error:
	if (pPickerInfo)
		Picker_Free(pPickerInfo);
	return false;
}



int Picker_GetViewID(HWND hwndPicker)
{
	PickerInfo *pPickerInfo;
	pPickerInfo = GetPickerInfo(hwndPicker);
	return pPickerInfo->nCurrentViewID;
}



void Picker_SetViewID(HWND hwndPicker, int nViewID)
{
	LONG_PTR lpIconStyle,lpNewStyle, lpListViewStyle;
	PickerInfo *pPickerInfo;

	pPickerInfo = GetPickerInfo(hwndPicker);

	// Change the nCurrentViewID member
	pPickerInfo->nCurrentViewID = nViewID;
	if (pPickerInfo->pCallbacks->pfnSetViewMode)
		pPickerInfo->pCallbacks->pfnSetViewMode(pPickerInfo->nCurrentViewID);

	switch(nViewID)
	{
		case VIEW_LARGE_ICONS:
			lpNewStyle = LVS_ICON;
			break;
		case VIEW_SMALL_ICONS:
			lpNewStyle = LVS_SMALLICON;
			break;
		case VIEW_INLIST:
			lpNewStyle = LVS_LIST;
			break;
		case VIEW_GROUPED:
			[[fallthrough]];
		case VIEW_REPORT:
			[[fallthrough]];
		default:
			lpNewStyle = LVS_REPORT;
			break;
	}

	// Change the ListView flags in accordance
	lpListViewStyle = windows::get_window_long_ptr(hwndPicker, GWL_STYLE);
	lpIconStyle = lpListViewStyle;
	if (nViewID == VIEW_LARGE_ICONS || nViewID == VIEW_SMALL_ICONS)
	{
		// remove Ownerdraw style for Icon views
		lpIconStyle &= ~LVS_OWNERDRAWFIXED;
		if( nViewID == VIEW_SMALL_ICONS )
		{
			// to properly get them to arrange, otherwise the entries might overlap
			// we have to call SetWindowLong to get it into effect !!
			// It's no use just setting the Style, as it's changed again further down...
			(void)windows::set_window_long_ptr(hwndPicker, GWL_STYLE, (lpListViewStyle & ~LVS_TYPEMASK) | LVS_ICON);
		}
	}
	else
	{
		// add again..
		lpIconStyle |= LVS_OWNERDRAWFIXED;
	}
	lpIconStyle &= ~LVS_TYPEMASK;
	lpIconStyle |= lpNewStyle;
	(void)windows::set_window_long_ptr(hwndPicker, GWL_STYLE, lpIconStyle);
	(void)gdi::redraw_window(hwndPicker, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME);
}


static bool PickerHitTest(HWND hWnd)
{
	// Get the mouse position in screen coordinates
	DWORD cursor_pos = windows::get_message_pos();
	POINTS screen_pt = gdi::make_points(cursor_pos);

	// Convert mouse position from screen to client coordinates
	POINT client_pt = { screen_pt.x, screen_pt.y };
	(void)gdi::screen_to_client(hWnd, &client_pt);

	// Prepare and perform hit test
	LVHITTESTINFO htInfo = {};
	htInfo.pt = client_pt;
	(void)list_view::hit_test(hWnd, &htInfo);

	// Return true if mouse is over a list item
	return !(htInfo.flags & LVHT_NOWHERE);
}


int Picker_GetSelectedItem(HWND hWnd)
{
	int item_index = list_view::get_next_item(hWnd, -1, LVIS_SELECTED | LVIS_FOCUSED);
	if (item_index < 0)
		return item_index;

	LVITEMW lvi;
	lvi = {};
	lvi.iItem = item_index;
	lvi.mask = LVIF_PARAM;
	(void)list_view::get_item(hWnd, &lvi);
	return static_cast<int>(lvi.lParam);
}


void Picker_SetSelectedPick(HWND hWnd, int item_index)
{
	// item_count is one more than number of last game
	int item_count = list_view::get_item_count(hWnd);
	if (item_count <= 0)
		return; // early exit. no games to show

	// bounds check
	if (item_index < 0)
		item_index = 0;
	else if (item_index >= item_count)
		item_index = item_count - 1;

	// Highlight a game
	list_view::set_item_state(hWnd, item_index, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	// Bring the game into view
	(void)list_view::ensure_visible(hWnd, item_index, false);
}



void Picker_SetSelectedItem(HWND hWnd, int item_index)
{
	LVFINDINFOW lvfi;
	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = static_cast<LPARAM>(item_index);

	int item_pos = list_view::find_item(hWnd, -1, &lvfi);
	if (item_pos == -1)
	{
		POINT p = {};
		lvfi.flags = LVFI_NEARESTXY;
		lvfi.pt = p;
		item_pos = list_view::find_item(hWnd, -1, &lvfi);
	}
	Picker_SetSelectedPick(hWnd, item_pos);
}



static std::wstring Picker_CallGetItemString(HWND hwndPicker,  int item_index, int nColumn)
{
	// this call wraps the pfnGetItemString callback to properly set up the
	// buffers, and normalize the results
	PickerInfo *pPickerInfo = GetPickerInfo(hwndPicker);

	if (!pPickerInfo || !pPickerInfo->pCallbacks || !pPickerInfo->pCallbacks->pfnGetItemString)
		return std::wstring{};

	return pPickerInfo->pCallbacks->pfnGetItemString(hwndPicker, item_index, nColumn);
}



// put the arrow on the new sort column
static void Picker_ResetHeaderSortIcon(HWND hwndPicker)
{
	PickerInfo* pPickerInfo;
	pPickerInfo = GetPickerInfo(hwndPicker);
	HWND hwndHeader = list_view::get_header(hwndPicker);

	// take arrow off non-current columns
	HDITEMW hdi;
	hdi.mask = HDI_FORMAT;
	hdi.fmt = HDF_STRING;
	for (size_t i = 0; i < pPickerInfo->nColumnCount; i++)
		if (i != pPickerInfo->pCallbacks->pfnGetSortColumn())
			(void)header_control::set_item(hwndHeader, Picker_GetViewColumnFromRealColumn(hwndPicker, i), &hdi);

	// put our arrow icon next to the text
	hdi.mask = HDI_FORMAT | HDI_IMAGE;
	hdi.fmt = HDF_STRING | HDF_IMAGE | HDF_BITMAP_ON_RIGHT;
	hdi.iImage = pPickerInfo->pCallbacks->pfnGetSortReverse() ? 1 : 0;

	int nViewColumn = Picker_GetViewColumnFromRealColumn(hwndPicker, pPickerInfo->pCallbacks->pfnGetSortColumn());
	(void)header_control::set_item(hwndHeader, nViewColumn, &hdi);
}




static void Picker_PopulateCompareProcParams(HWND hwndPicker, CompareProcParams *pParams)
{
	PickerInfo *pPickerInfo;

	pPickerInfo = GetPickerInfo(hwndPicker);

	// populate the CompareProcParams structure
	*pParams = {};
	pParams->hwndPicker = hwndPicker;
	pParams->pPickerInfo = pPickerInfo;
	pParams->nViewMode = pPickerInfo->pCallbacks->pfnGetViewMode();
	if (pPickerInfo->pCallbacks->pfnGetSortColumn)
		pParams->nSortColumn = pPickerInfo->pCallbacks->pfnGetSortColumn();
	if (pPickerInfo->pCallbacks->pfnGetSortReverse)
		pParams->bReverse = pPickerInfo->pCallbacks->pfnGetSortReverse();
}



static int CALLBACK Picker_CompareProc(LPARAM index1, LPARAM index2, LPARAM nParamSort)
{
	CompareProcParams *pcpp = (CompareProcParams *) nParamSort;
	PickerInfo *pPickerInfo = pcpp->pPickerInfo;
	bool bCallCompare = true;
	int nResult = 0, nParent1 = 0, nParent2 = 0;

	if (pcpp->nViewMode == VIEW_GROUPED)
	{
		// do our fancy compare, with clones grouped under parents
		// first thing we need to do is identify both item's parents
		if (pPickerInfo->pCallbacks->pfnFindItemParent)
		{
			nParent1 = pPickerInfo->pCallbacks->pfnFindItemParent(pcpp->hwndPicker, index1);
			nParent2 = pPickerInfo->pCallbacks->pfnFindItemParent(pcpp->hwndPicker, index2);
		}
		else
		{
			nParent1 = nParent2 = -1;
		}

		if ((nParent1 < 0) && (nParent2 < 0))
		{
			// if both entries are both parents, we just do a basic sort
		}
		else if ((nParent1 >= 0) && (nParent2 >= 0))
		{
			// if both entries are children and the parents are different,
			// sort on their parents
			if (nParent1 != nParent2)
			{
				index1 = nParent1;
				index2 = nParent2;
			}
		}
		else
		{
			// one parent, one child
			if (nParent1 >= 0)
			{
				// first one is a child
				if (nParent1 == index2)
				{
					// if this is a child and its parent, put child after
					nResult = 1;
					bCallCompare = false;
				}
				else
				{
					// sort on parent
					index1 = nParent1;
				}
			}
			else
			{
				// second one is a child
				if (nParent2 == index1)
				{
					// if this is a child and its parent, put child after
					nResult = -1;
					bCallCompare = false;
				}
				else
				{
					// sort on parent
					index2 = nParent2;
				}
			}
		}
	}

	if (bCallCompare)
	{
		if (pPickerInfo->pCallbacks->pfnCompare)
		{
			nResult = pPickerInfo->pCallbacks->pfnCompare(pcpp->hwndPicker, index1, index2, pcpp->nSortColumn, false);
		}
		else
		{
			// no default sort proc, just get the string and compare them
			std::wstring s1 = Picker_CallGetItemString(pcpp->hwndPicker, index1, pcpp->nSortColumn);
			std::wstring s2 = Picker_CallGetItemString(pcpp->hwndPicker, index2, pcpp->nSortColumn);
			nResult = mui_wcsicmp(s1, s2);
		}
		if (pcpp->bReverse)
			nResult = -nResult;
	}

	return nResult;
}



void Picker_Sort(HWND hwndPicker)
{
	//PickerInfo *pPickerInfo;
	CompareProcParams params;

	//pPickerInfo = GetPickerInfo(hwndPicker);

	// populate the CompareProcParams structure
	Picker_PopulateCompareProcParams(hwndPicker, &params);

	list_view::sort_item(hwndPicker, Picker_CompareProc, (LPARAM) &params);

	Picker_ResetHeaderSortIcon(hwndPicker);

	LVFINDINFOW lvfi;
	lvfi = {};
	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = Picker_GetSelectedItem(hwndPicker);
	 int item_index = list_view::find_item(hwndPicker, -1, &lvfi);

	list_view::ensure_visible(hwndPicker, item_index, false);
}



int Picker_InsertItemSorted(HWND hwndPicker, int item_index)
{
	//PickerInfo *pPickerInfo;
	int nLow = 0, nMid = 0;
	CompareProcParams params;
	int nCompareResult = 0;
	LVITEMW lvi;
	//pPickerInfo = GetPickerInfo(hwndPicker);

	int nHigh = list_view::get_item_count(hwndPicker);

	// populate the CompareProcParams structure
	Picker_PopulateCompareProcParams(hwndPicker, &params);

	while(nLow < nHigh)
	{
		nMid = (nHigh + nLow) / 2;

		lvi = {};
		lvi.mask = LVIF_PARAM;
		lvi.iItem = nMid;
		list_view::get_item(hwndPicker, &lvi);
		nCompareResult = Picker_CompareProc(static_cast<LPARAM>(item_index), lvi.lParam, (LPARAM) &params);

		if (nCompareResult > 0)
			nLow  = nMid + 1;
		else if (nCompareResult < 0)
			nHigh = nMid;
		else
		{
			nLow = nMid;
			break;
		}
	}

	lvi = {};
	lvi.mask     = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;
	lvi.iItem    = nLow;
	lvi.iSubItem = 0;
	lvi.lParam   = static_cast<LPARAM>(item_index);
	lvi.pszText  = LPSTR_TEXTCALLBACK;
	lvi.iImage   = I_IMAGECALLBACK;

	return list_view::insert_item(hwndPicker, &lvi);
}



int Picker_GetRealColumnFromViewColumn(HWND hWnd, int nViewColumn)
{
	PickerInfo *pPickerInfo;
	int nRealColumn = 0;

	pPickerInfo = GetPickerInfo(hWnd);
	if (nViewColumn >= 0 && nViewColumn < pPickerInfo->nColumnCount)
		nRealColumn = pPickerInfo->pnColumnsOrder[nViewColumn];
	return nRealColumn;
}



int Picker_GetViewColumnFromRealColumn(HWND hWnd, int nRealColumn)
{
	PickerInfo *pPickerInfo;

	pPickerInfo = GetPickerInfo(hWnd);
	for (size_t i = 0; i < pPickerInfo->nColumnCount; i++)
		if (pPickerInfo->pnColumnsOrder[i] == nRealColumn)
			return i;

	// major error, shouldn't be possible, but no good way to warn
	return 0;
}



bool Picker_HandleNotify(LPNMHDR lpNmHdr)
{
	bool bResult = false;
	HWND hWnd = lpNmHdr->hwndFrom;
	NM_LISTVIEW *pnmv = (NM_LISTVIEW *) lpNmHdr;
	PickerInfo *pPickerInfo = GetPickerInfo(hWnd);

	switch (lpNmHdr->code)
	{
	case NM_RCLICK:
	case NM_CLICK:
	case NM_DBLCLK:
	{
		// don't allow selection of blank spaces in the listview
		if (!PickerHitTest(hWnd))
		{
			// we have no current item selected
			if (pPickerInfo->nLastItem != -1)
			{
				Picker_SetSelectedItem(hWnd, pPickerInfo->nLastItem);
			}
			bResult = true;
		}
		else if ((lpNmHdr->code == NM_DBLCLK) && (pPickerInfo->pCallbacks->pfnDoubleClick))
		{
			// double click!
			pPickerInfo->pCallbacks->pfnDoubleClick();
			bResult = true;
		}
	}
	break;

	case LVN_GETDISPINFO:
	{
		LV_DISPINFOW* pDispInfo = (LV_DISPINFOW*)lpNmHdr;
		 int item_index = (int)pDispInfo->item.lParam;

		if (pDispInfo->item.mask & LVIF_IMAGE)
		{
			// retrieve item image
			if (pPickerInfo->pCallbacks->pfnGetItemImage)
				pDispInfo->item.iImage = pPickerInfo->pCallbacks->pfnGetItemImage(hWnd, item_index);
			else
				pDispInfo->item.iImage = 0;
			bResult = true;
		}

		if (pDispInfo->item.mask & LVIF_STATE)
		{
			pDispInfo->item.state = 0;
			bResult = true;
		}

		if (pDispInfo->item.mask & LVIF_TEXT)
		{
			// retrieve item text
			int nColumn = Picker_GetRealColumnFromViewColumn(hWnd, pDispInfo->item.iSubItem);
			std::wstring item_string = Picker_CallGetItemString(hWnd, item_index, nColumn);

			mui_wcsncpy(pDispInfo->item.pszText, item_string.c_str(), item_string.length());

			bResult = true;
		}
	}
	break;

	case LVN_ITEMCHANGED:
		if ((pnmv->uOldState & LVIS_SELECTED)
			&& !(pnmv->uNewState & LVIS_SELECTED))
		{
			if (pnmv->lParam != -1)
				pPickerInfo->nLastItem = pnmv->lParam;
			if (pPickerInfo->pCallbacks->pfnLeavingItem)
				pPickerInfo->pCallbacks->pfnLeavingItem(hWnd, pnmv->lParam);
		}
		if (!(pnmv->uOldState & LVIS_SELECTED)
			&& (pnmv->uNewState & LVIS_SELECTED))
		{
			if (pPickerInfo->pCallbacks->pfnEnteringItem)
				pPickerInfo->pCallbacks->pfnEnteringItem(hWnd, pnmv->lParam);
		}
		bResult = true;
		break;

	case LVN_COLUMNCLICK:
	{
		// if clicked on the same column we're sorting by, reverse it
		bool bReverse = false;
		if (pPickerInfo->pCallbacks->pfnGetSortColumn() == Picker_GetRealColumnFromViewColumn(hWnd, pnmv->iSubItem))
			bReverse = !pPickerInfo->pCallbacks->pfnGetSortReverse();

		pPickerInfo->pCallbacks->pfnSetSortReverse(bReverse);
		pPickerInfo->pCallbacks->pfnSetSortColumn(Picker_GetRealColumnFromViewColumn(hWnd, pnmv->iSubItem));
		Picker_Sort(hWnd);
		bResult = true;
	}
	break;

	case LVN_BEGINDRAG:
		if (pPickerInfo->pCallbacks->pfnBeginListViewDrag)
			pPickerInfo->pCallbacks->pfnBeginListViewDrag(pnmv);
		break;
	}
	return bResult;
}



int Picker_GetNumColumns(HWND hWnd)
{
	int  nColumnCount = 0;
	int  *shown;
	PickerInfo *pPickerInfo;
	pPickerInfo = GetPickerInfo(hWnd);

	shown = new int[pPickerInfo->nColumnCount];
	if (!shown)
		return -1;

	pPickerInfo->pCallbacks->pfnGetColumnShown(shown);
	HWND hwndHeader = list_view::get_header(hWnd);

	if ((nColumnCount = header_control::get_item_count(hwndHeader)) < 1)
	{
		nColumnCount = 0;
		for (size_t i = 0; i < pPickerInfo->nColumnCount ; i++ )
			if (shown[i])
				nColumnCount++;
	}

	delete[] shown;
	return nColumnCount;
}



// Add ... to Items in ListView if needed
static LPCWSTR MakeShortString(HDC hDC, LPCWSTR lpszLong, int nColumnLen, int nOffset)
{
	static const wchar_t szThreeDots[] = L"...";
	static wchar_t szShort[MAX_PATH];
	int lpszLong_len = mui_wcslen(lpszLong);
	SIZE size;

	(void)gdi::get_text_extent_point_32(hDC, lpszLong, lpszLong_len, &size);
	if (lpszLong_len == 0 || size.cx + nOffset <= nColumnLen)
		return lpszLong;

	(void)mui_wcscpy(szShort, lpszLong);
	(void)gdi::get_text_extent_point_32(hDC, szThreeDots, std::size(szThreeDots), &size);
	int nAddLen = size.cx;

	for (size_t i = lpszLong_len; i > 0; i--)
	{
		szShort[i] = 0;
		(void)gdi::get_text_extent_point_32(hDC, szShort, i, &size);
		if (size.cx + nOffset + nAddLen <= nColumnLen)
			break;
	}

	mui_wcscpy((szShort + lpszLong_len), szThreeDots);

	return szShort;
}

#define ListView_GetItemRect_Modified(hwnd,i,prc,code) (BOOL)SNDMSG((hwnd),LVM_GETITEMRECT,(WPARAM)(int)(i),(((RECT *)(prc))->left = (code),(LPARAM)(RECT *)(prc)))

void Picker_HandleDrawItem(HWND hWnd, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	PickerInfo *pPickerInfo;
	pPickerInfo = GetPickerInfo(hWnd);
	int          *order;
	order = new int[pPickerInfo->nColumnCount];
	if (!order)
		return;

	HDC          hDC = lpDrawItemStruct->hDC;
	RECT         rcItem = lpDrawItemStruct->rcItem;
	UINT         uiFlags = ILD_TRANSPARENT;
	HIMAGELIST   hImageList;
	int          item_index = lpDrawItemStruct->itemID;
	COLORREF     clrTextSave = 0;
	COLORREF     clrBkSave = 0;
	COLORREF     clrImage = windows::get_sys_color(COLOR_WINDOW);
	static wchar_t szBuff[MAX_PATH];
	bool         bFocus = (input::get_focus() == hWnd);
	LPCWSTR      pszText;
	UINT         nStateImageMask = 0;
	bool         bSelected = 0;
	LV_COLUMNW    lvc;
	LVITEMW      lvi;
	RECT         rcAllLabels;
	RECT         rcLabel;
	RECT         rcIcon;
	int          offset = 0;
	SIZE         size;
	int          i = 0, j = 0;
	int          nColumn = 0;
	bool         bDrawAsChild = 0;
	int          indent_space = 0;
	bool         bColorChild = false;
	bool         bParentFound = false;
	int          nParent = 0;
	HBITMAP      hBackground = GetBackgroundBitmap();
	MYBITMAPINFO *pbmDesc = GetBackgroundInfo();

	int nColumnMax = Picker_GetNumColumns(hWnd);

	// Get the Column Order and save it
	(void)list_view::get_column_order_array(hWnd, nColumnMax, order);

	// Disallow moving column 0
	if (order[0] != 0)
	{
		for (i = 0; i < nColumnMax; i++)
		{
			if (order[i] == 0)
			{
				order[i] = order[0];
				order[0] = 0;
			}
		}
		list_view::get_column_order_array(hWnd, nColumnMax, order);
	}

	// Labels are offset by a certain amount
	// This offset is related to the width of a space character
	gdi::get_text_extent_point_32(hDC, L" ", 1, &size);
	offset = size.cx;

	lvi.mask       = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
	lvi.iItem      = item_index;
	lvi.iSubItem   = order[0];
	lvi.pszText    = szBuff;
	lvi.cchTextMax = MAX_PATH;
	lvi.stateMask  = 0xFFFF;   // get all state flags
	(void)list_view::get_item(hWnd, &lvi);

	bSelected = ((lvi.state & LVIS_DROPHILITED) || ( (lvi.state & LVIS_SELECTED) && ((bFocus) || (windows::get_window_long_ptr(hWnd, GWL_STYLE) & LVS_SHOWSELALWAYS))));

	// figure out if we indent and draw grayed
	if (pPickerInfo->pCallbacks->pfnFindItemParent)
		nParent = pPickerInfo->pCallbacks->pfnFindItemParent(hWnd, lvi.lParam);
	else
		nParent = -1;
	bDrawAsChild = (pPickerInfo->pCallbacks->pfnGetViewMode() == VIEW_GROUPED && (nParent >= 0));

	// only indent if parent is also in this view
#if 1   // minimal listview flickering.
	if ((nParent >= 0) && bDrawAsChild)
	{
		if (GetParentFound(lvi.lParam))
			bParentFound = true;
	}
#else
	if ((nParent >= 0) && bDrawAsChild)
	{
		for (i = 0; i < list_view::get_item_count(hWnd); i++)
		{
			lvi.mask = LVIF_PARAM;
			lvi.iItem = i;
			(void) list_view::get_item(hWnd, &lvi);

			if (lvi.lParam == nParent)
			{
				bParentFound = true;
				break;
			}
		}
	}
#endif

	if (pPickerInfo->pCallbacks->pfnGetOffsetChildren && pPickerInfo->pCallbacks->pfnGetOffsetChildren())
	{
		if (!bParentFound && bDrawAsChild)
		{
			/*Reset it, as no Parent is there*/
			bDrawAsChild = false;
			bColorChild = true;
		}
		else
		{
			nParent = -1;
			bParentFound = false;
		}
	}

	(void)ListView_GetItemRect_Modified(hWnd, item_index, &rcAllLabels, LVIR_BOUNDS);
	(void)ListView_GetItemRect_Modified(hWnd, item_index, &rcLabel, LVIR_LABEL);

	rcAllLabels.left = rcLabel.left;

	if (hBackground)
	{
		RECT rcClient;
		HRGN rgnBitmap;
		RECT rcTmpBmp = rcItem;
		RECT rcFirstItem;
		HPALETTE hPAL;

		HDC htempDC = gdi::create_compatible_dc(hDC);

		HBITMAP oldBitmap = (HBITMAP)gdi::select_object(htempDC, hBackground);

		(void)windows::get_client_rect(hWnd, &rcClient);
		rcTmpBmp.right = rcClient.right;
		// We also need to check whether it is the last item
		// The update region has to be extended to the bottom if it is
		if (item_index == list_view::get_item_count(hWnd) - 1)
			rcTmpBmp.bottom = rcClient.bottom;

		rgnBitmap = gdi::create_rect_rgn_indirect(&rcTmpBmp);
		gdi::select_clip_rgn(hDC, rgnBitmap);
		(void)gdi::delete_bitmap((HBITMAP)rgnBitmap);

		hPAL = GetBackgroundPalette();
		if (hPAL == nullptr)
			hPAL = gdi::create_half_tone_palette(hDC);

		if (gdi::get_device_caps(htempDC, RASTERCAPS) & RC_PALETTE && hPAL != nullptr)
		{
			gdi::select_palette(htempDC, hPAL, false);
			gdi::realize_palette(htempDC);
		}

		(void) ListView_GetItemRect_Modified(hWnd, 0, &rcFirstItem, LVIR_BOUNDS);

		for (i = rcFirstItem.left; i < rcClient.right; i += pbmDesc->bmWidth)
			for (j = rcFirstItem.top; j < rcClient.bottom; j +=  pbmDesc->bmHeight)
				(void)gdi::bit_blt(hDC, i, j, pbmDesc->bmWidth, pbmDesc->bmHeight, htempDC, 0, 0, SRCCOPY);

		(void)gdi::select_object(htempDC, oldBitmap);
		(void)gdi::delete_dc(htempDC);

		if (GetBackgroundPalette() == nullptr)
		{
			(void)gdi::delete_palette(hPAL);
			hPAL = nullptr;
		}
	}

	indent_space = 0;

	if (bDrawAsChild)
	{
		RECT rect;

		(void) ListView_GetItemRect_Modified(hWnd, item_index, &rect, LVIR_ICON);

		// indent width of icon + the space between the icon and text
		//  so left of clone icon starts at text of parent
		indent_space = rect.right - rect.left + offset;
	}

	rcAllLabels.left += indent_space;

	if (bSelected)
	{
		HBRUSH hBrush;
		HBRUSH hOldBrush;

		if (bFocus)
		{
			clrTextSave = gdi::set_text_color(hDC, windows::get_sys_color(COLOR_HIGHLIGHTTEXT));
			clrBkSave = gdi::set_bk_color(hDC, windows::get_sys_color(COLOR_HIGHLIGHT));
			hBrush = gdi::create_solid_brush(windows::get_sys_color(COLOR_HIGHLIGHT));
		}
		else
		{
			clrTextSave = gdi::set_text_color(hDC, windows::get_sys_color(COLOR_BTNTEXT));
			clrBkSave = gdi::set_bk_color(hDC, windows::get_sys_color(COLOR_BTNFACE));
			hBrush = gdi::create_solid_brush(windows::get_sys_color(COLOR_BTNFACE));
		}

		hOldBrush = (HBRUSH)gdi::select_object(hDC, hBrush);
		(void)gdi::fill_rect(hDC, &rcAllLabels, hBrush);
		(void)gdi::select_object(hDC, hOldBrush);
		(void)gdi::delete_brush(hBrush);
	}
	else
	{
		if (hBackground == nullptr)
		{
			HBRUSH hBrush = gdi::create_solid_brush(windows::get_sys_color(COLOR_WINDOW));
			(void)gdi::fill_rect(hDC, &rcAllLabels, hBrush);
			(void)gdi::delete_brush(hBrush);
		}

		if (pPickerInfo->pCallbacks->pfnGetOffsetChildren && pPickerInfo->pCallbacks->pfnGetOffsetChildren())
		{
			if (bDrawAsChild || bColorChild)
				clrTextSave = gdi::set_text_color(hDC, GetListCloneColor());
			else
				clrTextSave = gdi::set_text_color(hDC, GetListFontColor());
		}
		else
		{
			if (bDrawAsChild)
				clrTextSave = gdi::set_text_color(hDC, GetListCloneColor());
			else
				clrTextSave = gdi::set_text_color(hDC, GetListFontColor());
		}

		clrBkSave = gdi::set_bk_color(hDC, windows::get_sys_color(COLOR_WINDOW));
	}


	if (lvi.state & LVIS_CUT)
	{
		clrImage = windows::get_sys_color(COLOR_WINDOW);
		uiFlags |= ILD_BLEND50;
	}
	else
	if (bSelected)
	{
		if (bFocus)
			clrImage = windows::get_sys_color(COLOR_HIGHLIGHT);
		else
			clrImage = windows::get_sys_color(COLOR_BTNFACE);

		uiFlags |= ILD_BLEND50;
	}

	nStateImageMask = lvi.state & LVIS_STATEIMAGEMASK;

	if (nStateImageMask)
	{
		int nImage = (nStateImageMask >> 12) - 1;
		hImageList = list_view::get_image_list(hWnd, LVSIL_STATE);
		if (hImageList)
			image_list::draw(hImageList, nImage, hDC, rcItem.left, rcItem.top, ILD_TRANSPARENT);
	}

	(void)ListView_GetItemRect_Modified(hWnd, item_index, &rcIcon, LVIR_ICON);

	rcIcon.left += indent_space;

	(void)ListView_GetItemRect_Modified(hWnd, item_index, &rcItem, LVIR_LABEL);

	hImageList = list_view::get_image_list(hWnd, LVSIL_SMALL);
	if (hImageList)
	{
		UINT nOvlImageMask = lvi.state & LVIS_OVERLAYMASK;
		if (rcIcon.left + 16 + indent_space < rcItem.right)
			image_list::draw_ex(hImageList, lvi.iImage, hDC, rcIcon.left, rcIcon.top, 16, 16, windows::get_sys_color(COLOR_WINDOW), clrImage, uiFlags | nOvlImageMask);
	}

	(void)ListView_GetItemRect_Modified(hWnd, item_index, &rcItem, LVIR_LABEL);

	pszText = MakeShortString(hDC, szBuff, rcItem.right - rcItem.left, 2*offset + indent_space);

	rcLabel = rcItem;
	rcLabel.left  += offset + indent_space;
	rcLabel.right -= offset;

	gdi::draw_text(hDC, pszText, -1, &rcLabel, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER);

	for (nColumn = 1; nColumn < nColumnMax; nColumn++)
	{
		UINT nJustify;
		LVITEMW lvItem;

		lvc.mask = LVCF_FMT | LVCF_WIDTH;
		(void)list_view::get_column(hWnd, order[nColumn], &lvc);

		lvItem.mask       = LVIF_TEXT;
		lvItem.iItem      = item_index;
		lvItem.iSubItem   = order[nColumn];
		lvItem.pszText    = szBuff;
		lvItem.cchTextMax = MAX_PATH;

		if (list_view::get_item(hWnd, &lvItem) == false)
			continue;

		rcItem.left   = rcItem.right;
		rcItem.right += lvc.cx;

		if (*szBuff == L'\0')
			continue;

		pszText = MakeShortString(hDC, szBuff, rcItem.right - rcItem.left, 2 * offset);

		nJustify = DT_LEFT;

		if (pszText == szBuff)
		{
			switch (lvc.fmt & LVCFMT_JUSTIFYMASK)
			{
			case LVCFMT_RIGHT:
				nJustify = DT_RIGHT;
				break;

			case LVCFMT_CENTER:
				nJustify = DT_CENTER;
				break;

			default:
				break;
			}
		}

		rcLabel = rcItem;
		rcLabel.left  += offset;
		rcLabel.right -= offset;
		gdi::draw_text(hDC, pszText, -1, &rcLabel, nJustify | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER);
	}

	if (lvi.state & LVIS_FOCUSED && bFocus)
		gdi::draw_focus_rect(hDC, &rcAllLabels);

	(void)gdi::set_text_color(hDC, clrTextSave);
	(void)gdi::set_bk_color(hDC, clrBkSave);
	delete[] order;
}




const PickerCallbacks *Picker_GetCallbacks(HWND hwndPicker)
{
	const PickerInfo *pPickerInfo = GetPickerInfo(hwndPicker);
	return (pPickerInfo) ? pPickerInfo->pCallbacks : nullptr;
}



int Picker_GetColumnCount(HWND hwndPicker)
{
	const PickerInfo *pPickerInfo = GetPickerInfo(hwndPicker);
	return (pPickerInfo) ? pPickerInfo->nColumnCount : 0;
}



const std::wstring *Picker_GetColumnNames(HWND hwndPicker)
{
	const PickerInfo *pPickerInfo = GetPickerInfo(hwndPicker);
	return (pPickerInfo) ? pPickerInfo->column_names : nullptr;
}



void Picker_SetHeaderImageList(HWND hwndPicker, HIMAGELIST hHeaderImages)
{
	HWND hwndHeader = list_view::get_header(hwndPicker);
	(void)windows::send_message(hwndHeader, HDM_SETIMAGELIST, 0, (LPARAM) (void *) hHeaderImages);
}



bool Picker_SaveColumnWidths(HWND hwndPicker)
{
	PickerInfo* pPickerInfo;
	int nColumnMax = 0, i = 0;
	bool bSuccess = false;

	pPickerInfo = GetPickerInfo(hwndPicker);

	// allocate space for the column info
	int* widths, * order, * tmpOrder;
	widths = new int[pPickerInfo->nColumnCount];
	order = new int[pPickerInfo->nColumnCount];
	tmpOrder = new int[pPickerInfo->nColumnCount];
	if (!widths || !order || !tmpOrder)
		goto done;

	// retrieve the values
	pPickerInfo->pCallbacks->pfnGetColumnWidths(widths);
	pPickerInfo->pCallbacks->pfnGetColumnOrder(order);

	// switch the list view to LVS_REPORT style so column widths reported correctly
	(void)windows::set_window_long_ptr(hwndPicker, GWL_STYLE, (windows::get_window_long_ptr(hwndPicker, GWL_STYLE) & ~LVS_TYPEMASK) | LVS_REPORT);

	nColumnMax = Picker_GetNumColumns(hwndPicker);

	// Get the Column Order and save it
	(void)list_view::get_column_order_array(hwndPicker, nColumnMax, tmpOrder);

	for (i = 0; i < nColumnMax; i++)
	{
		widths[Picker_GetRealColumnFromViewColumn(hwndPicker, i)] =list_view::get_column_width(hwndPicker, i);
		order[i] = Picker_GetRealColumnFromViewColumn(hwndPicker, tmpOrder[i]);
	}

	pPickerInfo->pCallbacks->pfnSetColumnWidths(widths);
	pPickerInfo->pCallbacks->pfnSetColumnOrder(order);
	bSuccess = true;

done:
	if (widths)
		delete[] widths;
	if (order)
		delete[] order;
	if (tmpOrder)
		delete[] tmpOrder;
	return bSuccess;
}
