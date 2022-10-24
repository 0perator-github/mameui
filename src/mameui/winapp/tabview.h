// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_TABVIEW_H
#define MAMEUI_WINAPP_TABVIEW_H

#pragma once

using TabViewCallbacks = struct tabview_callbacks
{
	// Options retrieval
	bool (*pfnGetShowTabCtrl)(void);
	void (*pfnSetCurrentTab)(int val);
	int (*pfnGetCurrentTab)(void);
	void (*pfnSetShowTab)(int tab_index, bool show);
	int (*pfnGetShowTab)(int tab_index);

	// Accessors
	std::wstring_view (*pfnGetTabShortName)(int tab_index);
	std::wstring_view (*pfnGetTabLongName)(int tab_index);

	// Callbacks
	void (*pfnOnSelectionChanged)(void);
	void (*pfnOnMoveSize)(void);
};

using TabViewOptions = struct tabview_options
{
	const TabViewCallbacks *pCallbacks;
	int nTabCount;
};

bool SetupTabView(HWND hwndTabView, const TabViewOptions *pOptions);

void TabView_Reset(HWND hwndTabView);
void TabView_CalculateNextTab(HWND hwndTabView);
int TabView_GetCurrentTab(HWND hwndTabView);
void TabView_SetCurrentTab(HWND hwndTabView, int tab_index);
void TabView_UpdateSelection(HWND hwndTabView);

// These are used to handle events received by the parent regarding
// tabview controls
bool TabView_HandleNotify(LPNMHDR lpNmHdr);

#endif // MAMEUI_WINAPP_TABVIEW_H
