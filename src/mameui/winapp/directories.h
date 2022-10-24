// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

#ifndef MAMEUI_WINAPP_DIRECTORIES_H
#define MAMEUI_WINAPP_DIRECTORIES_H

#pragma once

/* Dialog return codes - do these do anything??? */
#define DIRDLG_ROMS         0x0010  // this one does
#define DIRDLG_SAMPLES      0x0020  // this one does
//#define DIRDLG_INI          0x0040
//#define DIRDLG_CFG          0x0100
//#define DIRDLG_IMG          0x0400
//#define DIRDLG_INP          0x0800
//#define DIRDLG_CTRLR        0x1000
#define DIRDLG_SW           0x4000  // this one does
//#define DIRDLG_CHEAT        0x8000

constexpr std::wstring_view DIRLIST_NEWENTRYTEXT{ L"<               >" };

INT_PTR CALLBACK DirectoriesDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

#endif // MAMEUI_WINAPP_DIRECTORIES_H
