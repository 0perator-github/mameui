// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************
//============================================================
//
//  newui.h - NEWUI
//
//============================================================

#ifndef MAMEUI_WINAPP_NEWUI_H
#define MAMEUI_WINAPP_NEWUI_H

#pragma once

// standard windows headers

// standard C++ headers

// MAME/MAMEUI headers
#include "emu.h"
#include "mame.h"
#include "emuopts.h"
#include "ui/ui.h"
#include "newuires.h"
#include "natkeyboard.h"
#include "softlist_dev.h"
#include "imagedev/cassette.h"
#include "windows/window.h"
#include "winutf8.h"
#include "modules/lib/osdobj_common.h"



// These are called from src/osd/windows/windows.cpp and
//   provide the linkage between newui and the core.

LRESULT CALLBACK winwindow_video_window_proc_ui(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam);

int winwindow_create_menu(running_machine &machine, HMENU *menus);

#endif // MAMEUI_WINAPP_NEWUI_H
