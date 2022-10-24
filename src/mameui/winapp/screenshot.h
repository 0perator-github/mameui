// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

#ifndef MAMEUI_WINAPP_SCREENSHOT_H
#define MAMEUI_WINAPP_SCREENSHOT_H

#pragma once

using MYBITMAPINFO = struct my_bitmap_info
{
	int bmWidth;
	int bmHeight;
	int bmColors;
};
using LPMYBITMAPINFO = MYBITMAPINFO*;

#if 0
using SSINFO = struct screenshot_info
{
	TEXT tabtext;
	const char *zipname;
	int tabenum;
	std::string(*pfnGetTheseDirs)(void);
};

const SSINFO m_ssInfo[] =
{
	{ "Snapshot",       "snap",      TAB_SCREENSHOT,     GetImgDir          },
	{ "Flyer",          "flyers",    TAB_FLYER,          GetFlyerDir        },
	{ "Cabinet",        "cabinets",  TAB_CABINET,        GetCabinetDir      },
	{ "Marquee",        "marquees",  TAB_MARQUEE,        GetMarqueeDir      },
	{ "Title",          "titles",    TAB_TITLE,          GetTitleDir        },
	{ "Control Panel",  "cpanel",    TAB_CONTROL_PANEL,  GetControlPanelDir },
	{ "PCB",            "pcb",       TAB_PCB,            GetPcbDir          },
};

/* if adding a new tab, need to also update:
- dialogs.cpp (~50)                (history on tab or not)
- winui.cpp   MameCommand  (~4173) (action the mouse click in the menu)
- mameui.rc                        (show in menu)
- ui_opts.h                        (tab enabled or not)
- resource.h
- mui_opts.cpp/h                   (directory get function)
- mui_opts.h                       enum of names
- mui_opts.cpp                     image_tabs_long_name
- screenshot.cpp (~316)            (choose image to display)
*/
#endif

extern bool LoadScreenShot(int driver_index, const std::string& lpSoftwareName, int nType);
extern HANDLE GetScreenShotHandle(void);
extern int GetScreenShotWidth(void);
extern int GetScreenShotHeight(void);

extern void FreeScreenShot(void);
extern bool ScreenShotLoaded(void);

extern bool LoadDIBBG(HGLOBAL *phDIB, HPALETTE *pPal);
extern HBITMAP DIBToDDB(HDC hDC, HANDLE hDIB, LPMYBITMAPINFO desc);

#endif // MAMEUI_WINAPP_SCREENSHOT_H
