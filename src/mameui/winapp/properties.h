// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_PROPERTIES_H
#define MAMEUI_WINAPP_PROPERTIES_H

#pragma once

/* Get title string to display in the top of the property page,
 * Called also in ui_audit.cpp
 */
std::string GameInfoTitle(SOFTWARETYPE_OPTIONS software_type, int driver_index);

/* Called in winui.cpp to create the property page */
void InitPropertyPage(HINSTANCE hInst, HWND hWnd, HICON hIcon, SOFTWARETYPE_OPTIONS software_type, int folder_id, int game_num);

constexpr auto PROPERTIES_PAGE = 0;
constexpr auto AUDIT_PAGE = 1;

void InitPropertyPageToPage(HINSTANCE hInst, HWND hWnd, HICON hIcon, SOFTWARETYPE_OPTIONS software_type, int folder_id, int game_num, int start_page);
void InitDefaultPropertyPage(HINSTANCE hInst, HWND hWnd);

/* Get Help ID array for WM_HELP and WM_CONTEXTMENU */
DWORD_PTR GetHelpIDs(void);

/* Get Game status text string */
std::string GameInfoStatus(int driver_index, bool bRomStatus);

/* Property sheet info for layout.c */
using PROPERTYSHEETINFO = struct property_sheet_info
{
	bool bOnDefaultPage = false;
//  bool (*pfnFilterProc)(const machine_config *drv, const game_driver *gamedrv);
	bool (*pfnFilterProc)(int driver_index) = nullptr;
	DWORD dwDlgID = 0UL;
	DLGPROC pfnDlgProc = nullptr;
};

extern const PROPERTYSHEETINFO g_propSheets[];

bool PropSheetFilter_Vector(const machine_config *drv, const game_driver *gamedrv);

INT_PTR CALLBACK GamePropertiesDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK GameOptionsProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

int PropertiesCurrentGame(HWND hDlg);

// from propertiesms.h (MESSUI)

bool MessPropertiesCommand(HWND hWnd, WORD wNotifyCode, WORD wID, bool *changed);
extern bool g_bModifiedSoftwarePaths;
INT_PTR CALLBACK GameMessOptionsProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
//bool PropSheetFilter_Config(const machine_config *drv, const game_driver *gamedrv);  // not used

#endif // MAMEUI_WINAPP_PROPERTIES_H

