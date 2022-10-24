// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

// standard C++ headers
#include <filesystem>
#include <iostream>

// standard windows headers
#include "winapi_common.h"

// MAME headers

// shlwapi.h pulls in objbase.h which defines interface as a struct which conflict with a template declaration in device.h
#ifdef interface
#undef interface // undef interface which is for COM interfaces
#endif
#include "emu.h"
#define interface struct

// MAMEUI headers
#include "mui_wcstrconv.h"

#include "windows_messages.h"

#include "screenshot.h"
#include "winui.h"

#include "tabview.h"

using namespace mameui::winapi;

using TabViewInfo = struct tabview_info
{
	const TabViewCallbacks *pCallbacks;
	int nTabCount;
	WNDPROC pfnParentWndProc;
};



static TabViewInfo *GetTabViewInfo(HWND hWnd)
{
	LONG_PTR l = windows::get_window_long_ptr(hWnd, GWLP_USERDATA);
	return (TabViewInfo *) l;
}



static LRESULT CallParentWndProc(WNDPROC pfnParentWndProc, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (!pfnParentWndProc)
		pfnParentWndProc = GetTabViewInfo(hWnd)->pfnParentWndProc;

	return windows::call_window_proc(pfnParentWndProc, hWnd, message, wParam, lParam);
}



static LRESULT CALLBACK TabViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TabViewInfo *pTabViewInfo;
	pTabViewInfo = GetTabViewInfo(hWnd);

	WNDPROC pfnParentWndProc = pTabViewInfo->pfnParentWndProc;

	switch(message)
	{
		case WM_DESTROY:
			delete pTabViewInfo;
			(void)windows::set_window_long_ptr(hWnd, GWLP_WNDPROC, (LONG_PTR) pfnParentWndProc);
			(void)windows::set_window_long_ptr(hWnd, GWLP_USERDATA, (LONG_PTR) nullptr);
			break;
	}

	LRESULT rc = 0;
	// this is weird...
	bool bHandled = false;
	if (!bHandled)
		rc = CallParentWndProc(pfnParentWndProc, hWnd, message, wParam, lParam);

	switch(message)
	{
		case WM_MOVE:
		case WM_SIZE:
			if (pTabViewInfo->pCallbacks->pfnOnMoveSize)
				pTabViewInfo->pCallbacks->pfnOnMoveSize();
			break;
	}

	return rc;
}



static int TabView_GetTabFromTabIndex(HWND hwndTabView, int tab_index)
{
	int shown_tabs = -1;
	TabViewInfo *pTabViewInfo;

	pTabViewInfo = GetTabViewInfo(hwndTabView);

	for (size_t i = 0; i < pTabViewInfo->nTabCount; i++)
	{
		if (!pTabViewInfo->pCallbacks->pfnGetShowTab || pTabViewInfo->pCallbacks->pfnGetShowTab(i))
		{
			shown_tabs++;
			if (shown_tabs == tab_index)
				return i;
		}
	}
	std::cout << "invalid tab index " << tab_index << "\n";
	return 0;
}



int TabView_GetCurrentTab(HWND hwndTabView)
{
	TabViewInfo *pTabViewInfo;
	pTabViewInfo = GetTabViewInfo(hwndTabView);
	return pTabViewInfo->pCallbacks->pfnGetCurrentTab();
}



void TabView_SetCurrentTab(HWND hwndTabView, int nTab)
{
	TabViewInfo *pTabViewInfo;
	pTabViewInfo = GetTabViewInfo(hwndTabView);
	pTabViewInfo->pCallbacks->pfnSetCurrentTab(nTab);
}



static int TabView_GetCurrentTabIndex(HWND hwndTabView)
{
	int shown_tabs = 0;
	TabViewInfo *pTabViewInfo;

	pTabViewInfo = GetTabViewInfo(hwndTabView);
	int nCurrentTab = TabView_GetCurrentTab(hwndTabView);

	for (size_t i = 0; i < pTabViewInfo->nTabCount; i++)
	{
		if (i == nCurrentTab)
			break;

		if (!pTabViewInfo->pCallbacks->pfnGetShowTab || pTabViewInfo->pCallbacks->pfnGetShowTab(i))
			shown_tabs++;
	}
	return shown_tabs;
}



void TabView_UpdateSelection(HWND hwndTabView)
{
	(void)TabCtrl_SetCurSel(hwndTabView, TabView_GetCurrentTabIndex(hwndTabView));
}



bool TabView_HandleNotify(LPNMHDR lpNmHdr)
{
	TabViewInfo *pTabViewInfo;
	bool bResult = false;

	HWND hwndTabView = lpNmHdr->hwndFrom;
	pTabViewInfo = GetTabViewInfo(hwndTabView);

	switch (lpNmHdr->code)
	{
		case TCN_SELCHANGE:
			int nTabIndex = TabCtrl_GetCurSel(hwndTabView);
			int nTab = TabView_GetTabFromTabIndex(hwndTabView, nTabIndex);
			TabView_SetCurrentTab(hwndTabView, nTab);
			if (pTabViewInfo->pCallbacks->pfnOnSelectionChanged)
				pTabViewInfo->pCallbacks->pfnOnSelectionChanged();
			bResult = true;
			break;
	}
	return bResult;
}



void TabView_CalculateNextTab(HWND hwndTabView)
{
	TabViewInfo *pTabViewInfo;
	int nCurrentTab;

	pTabViewInfo = GetTabViewInfo(hwndTabView);

	// at most loop once through all options
	for (size_t i = 0; i < pTabViewInfo->nTabCount; i++)
	{
		nCurrentTab = TabView_GetCurrentTab(hwndTabView);
		TabView_SetCurrentTab(hwndTabView, (nCurrentTab + 1) % pTabViewInfo->nTabCount);
		nCurrentTab = TabView_GetCurrentTab(hwndTabView);

		if (!pTabViewInfo->pCallbacks->pfnGetShowTab || pTabViewInfo->pCallbacks->pfnGetShowTab(nCurrentTab))
		{
			// this tab is being shown, so we're all set
			return;
		}
	}
}


void TabView_Reset(HWND hwndTabView)
{
	std::cout << "TabView_Reset: A" << "\n";
	TabViewInfo *pTabViewInfo;
	pTabViewInfo = GetTabViewInfo(hwndTabView);

	std::cout << "TabView_Reset: B" << "\n";
	(void)TabCtrl_DeleteAllItems(hwndTabView);

	TC_ITEM tci{ TCIF_TEXT };
	tci.cchTextMax = 20;

	std::cout << "TabView_Reset: C" << "\n";
	for (int i = 0; i < pTabViewInfo->nTabCount; i++)
	{
		if (!pTabViewInfo->pCallbacks->pfnGetShowTab || pTabViewInfo->pCallbacks->pfnGetShowTab(i))
		{
			std::wstring tab_name(pTabViewInfo->pCallbacks->pfnGetTabLongName(i));
			tci.pszText = const_cast<wchar_t*>(tab_name.c_str());
			(void)TabCtrl_InsertItem(hwndTabView, i, &tci);
		}
	}
	std::cout << "TabView_Reset: E" << "\n";
	TabView_UpdateSelection(hwndTabView);
	std::cout << "TabView_Reset: Finished" << "\n";
}


bool SetupTabView(HWND hwndTabView, const TabViewOptions *pOptions)
{
	//assert(hwndTabView);
	std::cout << "SetupTabView: A" << "\n";
	TabViewInfo *pTabViewInfo;

	// Allocate the list view struct
	pTabViewInfo = new TabViewInfo{};
	if (!pTabViewInfo)
		return false;

	// And fill it out
	std::cout << "SetupTabView: B" << "\n";
	pTabViewInfo->pCallbacks = pOptions->pCallbacks;
	pTabViewInfo->nTabCount = pOptions->nTabCount;

	// Hook in our wndproc and userdata pointer
	std::cout << "SetupTabView: C" << "\n";
	LONG_PTR l = windows::get_window_long_ptr(hwndTabView, GWLP_WNDPROC);
	pTabViewInfo->pfnParentWndProc = (WNDPROC) l;
	(void)windows::set_window_long_ptr(hwndTabView, GWLP_USERDATA, (LONG_PTR) pTabViewInfo);
	(void)windows::set_window_long_ptr(hwndTabView, GWLP_WNDPROC, (LONG_PTR) TabViewWndProc);

	std::cout << "SetupTabView: D" << "\n";
	bool bShowTabView = pTabViewInfo->pCallbacks->pfnGetShowTabCtrl ? pTabViewInfo->pCallbacks->pfnGetShowTabCtrl() : true;
	std::cout << "SetupTabView: E" << "\n";
	windows::show_window(hwndTabView, bShowTabView ? SW_SHOW : SW_HIDE);

	std::cout << "SetupTabView: F" << "\n";
	TabView_Reset(hwndTabView);
	if (pTabViewInfo->pCallbacks->pfnOnSelectionChanged)
		pTabViewInfo->pCallbacks->pfnOnSelectionChanged();
	std::cout << "SetupTabView: Finished" << "\n";
	return true;
}


