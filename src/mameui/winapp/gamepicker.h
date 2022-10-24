// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_PICKER_H
#define MAMEUI_WINAPP_PICKER_H

#pragma once

// --------------------------------------------------------------------------
// Type Declarations
// --------------------------------------------------------------------------

using PickerCallbacks = struct picker_callbacks
{
	// Options retrieval
	void (*pfnSetSortColumn)(int column);
	int (*pfnGetSortColumn)(void);
	void (*pfnSetSortReverse)(bool reverse);
	bool (*pfnGetSortReverse)(void);
	void (*pfnSetViewMode)(int val);
	int (*pfnGetViewMode)(void);
	void (*pfnSetColumnWidths)(int widths[]);
	void (*pfnGetColumnWidths)(int widths[]);
	void (*pfnSetColumnOrder)(int order[]);
	void (*pfnGetColumnOrder)(int order[]);
	void (*pfnSetColumnShown)(int shown[]);
	void (*pfnGetColumnShown)(int shown[]);
	bool (*pfnGetOffsetChildren)(void);

	int (*pfnCompare)(HWND hwndPicker, int nIndex1, int nIndex2, int nSortSubItem, bool ascending);
	void (*pfnDoubleClick)(void);
	std::wstring (*pfnGetItemString)(HWND hwndPicker,  int item_index, int nColumn);
	int (*pfnGetItemImage)(HWND hwndPicker, int item_index);
	void (*pfnLeavingItem)(HWND hwndPicker, int item_index);
	void (*pfnEnteringItem)(HWND hwndPicker, int item_index);
	void (*pfnBeginListViewDrag)(NM_LISTVIEW* pnlv);
	int (*pfnFindItemParent)(HWND hwndPicker,  int item_index);
	bool (*pfnOnIdle)(HWND hwndPicker);
	void (*pfnOnHeaderContextMenu)(POINT pt, int nColumn);
	void (*pfnOnBodyContextMenu)(POINT pt);
};

using PickerOptions = struct picker_options
{
	const PickerCallbacks* pCallbacks;
	int nColumnCount;
	const std::wstring *column_names;
};

enum
{
	VIEW_LARGE_ICONS = 0,
	VIEW_SMALL_ICONS,
	VIEW_INLIST,
	VIEW_REPORT,
	VIEW_GROUPED,
	VIEW_MAX
};


// --------------------------------------------------------------------------
// function prototypes
// --------------------------------------------------------------------------

bool SetupPicker(HWND hwndPicker, const PickerOptions *pOptions);

int Picker_GetViewID(HWND hwndPicker);
void Picker_SetViewID(HWND hwndPicker, int nViewID);
int Picker_GetRealColumnFromViewColumn(HWND hwndPicker, int nViewColumn);
int Picker_GetViewColumnFromRealColumn(HWND hwndPicker, int nRealColumn);
void Picker_Sort(HWND hwndPicker);
void Picker_ResetColumnDisplay(HWND hwndPicker);
int Picker_GetSelectedItem(HWND hwndPicker);
void Picker_SetSelectedItem(HWND hwndPicker, int item_index);
void Picker_SetSelectedPick(HWND hwndPicker, int item_index);
int Picker_GetNumColumns(HWND hWnd);
void Picker_ClearIdle(HWND hwndPicker);
void Picker_ResetIdle(HWND hwndPicker);
bool Picker_IsIdling(HWND hwndPicker);
void Picker_SetHeaderImageList(HWND hwndPicker, HIMAGELIST hHeaderImages);
int Picker_InsertItemSorted(HWND hwndPicker, int item_index);
bool Picker_SaveColumnWidths(HWND hwndPicker);

// These are used to handle events received by the parent regarding
// picker controls
bool Picker_HandleNotify(LPNMHDR lpNmHdr);
void Picker_HandleDrawItem(HWND hwndPicker, LPDRAWITEMSTRUCT lpDrawItemStruct);

// Accessors
const PickerCallbacks *Picker_GetCallbacks(HWND hwndPicker);
int Picker_GetColumnCount(HWND hwndPicker);
const std::wstring *Picker_GetColumnNames(HWND hwndPicker);

#endif // MAMEUI_WINAPP_PICKER_H
