// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_TABVIEW_H
#define MAMEUI_WINAPP_TABVIEW_H

#pragma once

struct TabViewCallbacks
{
	// Options retrieval
	bool (*pfnGetShowTabCtrl)(void);
	void (*pfnSetCurrentTab)(int val);
	int (*pfnGetCurrentTab)(void);
	void (*pfnSetShowTab)(int nTab, bool show);
	int (*pfnGetShowTab)(int nTab);

	// Accessors
	LPCSTR (*pfnGetTabShortName)(int nTab);
	LPCSTR (*pfnGetTabLongName)(int nTab);

	// Callbacks
	void (*pfnOnSelectionChanged)(void);
	void (*pfnOnMoveSize)(void);
};

struct TabViewOptions
{
	const struct TabViewCallbacks *pCallbacks;
	int nTabCount;
};


bool SetupTabView(HWND hwndTabView, const struct TabViewOptions *pOptions);

void TabView_Reset(HWND hwndTabView);
void TabView_CalculateNextTab(HWND hwndTabView);
int TabView_GetCurrentTab(HWND hwndTabView);
void TabView_SetCurrentTab(HWND hwndTabView, int nTab);
void TabView_UpdateSelection(HWND hwndTabView);

// These are used to handle events received by the parent regarding
// tabview controls
bool TabView_HandleNotify(LPNMHDR lpNmHdr);

#endif // MAMEUI_WINAPP_TABVIEW_H
