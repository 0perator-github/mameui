// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

 /***************************************************************************

  winui.cpp

  Win32 GUI code.

  Created 8/12/97 by Christopher Kirmse (ckirmse@ricochet.net)
  Additional code November 1997 by Jeff Miller (miller@aa.net)
  More July 1998 by Mike Haaland (mhaaland@hypertech.com)
  Added Spitters/Property Sheets/Removed Tabs/Added Tree Control in
  Nov/Dec 1998 - Mike Haaland

***************************************************************************/

// standard C++ headers
#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

// standard C++ headers

// standard windows headers
#include "winapi_common.h"
#include <dlgs.h>

// MAME headers

// shlwapi.h pulls in objbase.h which defines interface as a struct which conflict with a template declaration in device.h
#ifdef interface
#undef interface // undef interface which is for COM interfaces
#endif
#include "emu.h"
#define interface struct

#include "drivenum.h"
#include "language.h"
#include "mame.h"
#include "mameopts.h"
#include "unzip.h"
#include "winmain.h"
#include "window.h"
#include "zippath.h"

#include "ui/moptions.h"
#include "winopts.h"

// MAMEUI headers
#include "mui_cstr.h"
#include "mui_padstr.h"
#include "mui_stringtokenizer.h"
//#include "mui_trimstr.h"
#include "mui_wcstr.h"
#include "mui_wcstrconv.h"

#include "dialog_boxes.h"
#include "menus_other_res.h"
#include "processes_threads.h"
#include "system_services.h"
#include "windows_controls.h"
#include "windows_gdi.h"
#include "windows_input.h"
#include "windows_messages.h"
#include "windows_registry.h"
#include "windows_shell.h"

#include "bitmask.h"
#include "columnedit.h"
#include "dialogs.h"
#include "dijoystick.h"     // For DIJoystick availability.
#include "directinput.h"
#include "dirwatch.h"
#include "emu_opts.h"
#include "gamepicker.h"
//#include "game_opts.h"
#include "help.h"
#include "history.h"
#include "mui_audit.h"
#include "mui_opts.h"
#include "directories.h"
#include "mui_util.h"
#include "properties.h"
#include "resource.h"
#include "resource.hm"
#include "screenshot.h"
#include "splitters.h"
#include "swconfig.h"
#include "tabview.h"
#include "treeview.h"

//#ifdef MESS
#include "messui.h"
#include "softwarelist.h"
//#endif

#include "winui.h"

using namespace mameui::util::string_util;
using namespace mameui::winapi::controls;
using namespace mameui::winapi;
using namespace std::string_literals;

#ifndef LVS_EX_LABELTIP
#define LVS_EX_LABELTIP         0x00004000 // listview unfolds partly hidden labels if it does not have infotip text
#endif // LVS_EX_LABELTIP

// fix warning: cast does not match function type
#if defined(__GNUC__) && defined(ListView_CreateDragImage)
#undef ListView_CreateDragImage
#endif

#ifndef ListView_CreateDragImage
#define ListView_CreateDragImage(hwnd, i, lpptUpLeft) \
	(HIMAGELIST)(LRESULT)(int)windows::send_message((hwnd), LVM_CREATEDRAGIMAGE, (WPARAM)(int)(i), (LPARAM)(LPPOINT)(lpptUpLeft))
#endif // ListView_CreateDragImage

#ifndef TreeView_EditLabel
#define TreeView_EditLabel(w, i) \
	SNDMSG(w,TVM_EDITLABEL,0,(LPARAM)(i))
#endif // TreeView_EditLabel

#ifndef HDF_SORTUP
#define HDF_SORTUP 0x400
#endif // HDF_SORTUP

#ifndef HDF_SORTDOWN
#define HDF_SORTDOWN 0x200
#endif // HDF_SORTDOWN

#ifndef LVM_SETBKIMAGEA
#define LVM_SETBKIMAGEA         (LVM_FIRST + 68)
#endif // LVM_SETBKIMAGEA

#ifndef LVM_SETBKIMAGEW
#define LVM_SETBKIMAGEW         (LVM_FIRST + 138)
#endif // LVM_SETBKIMAGEW

#ifndef LVM_GETBKIMAGEA
#define LVM_GETBKIMAGEA         (LVM_FIRST + 69)
#endif // LVM_GETBKIMAGEA

#ifndef LVM_GETBKIMAGEW
#define LVM_GETBKIMAGEW         (LVM_FIRST + 139)
#endif // LVM_GETBKIMAGEW

#ifndef LVBKIMAGE

using LVBKIMAGEA = struct tagLVBKIMAGEA
{
	ULONG ulFlags;
	HBITMAP hbm;
	LPSTR pszImage;
	UINT cchImageMax;
	int xOffsetPercent;
	int yOffsetPercent;
};
using LPLVBKIMAGEA = LVBKIMAGEA*;

using LVBKIMAGEW = struct tagLVBKIMAGEW
{
	ULONG ulFlags;
	HBITMAP hbm;
	LPWSTR pszImage;
	UINT cchImageMax;
	int xOffsetPercent;
	int yOffsetPercent;
};
using LPLVBKIMAGEW = LVBKIMAGEW*;

#ifdef UNICODE
#define LVBKIMAGE               LVBKIMAGEW
#define LPLVBKIMAGE             LPLVBKIMAGEW
#define LVM_SETBKIMAGE          LVM_SETBKIMAGEW
#define LVM_GETBKIMAGE          LVM_GETBKIMAGEW
#else
#define LVBKIMAGE               LVBKIMAGEA
#define LPLVBKIMAGE             LPLVBKIMAGEA
#define LVM_SETBKIMAGE          LVM_SETBKIMAGEA
#define LVM_GETBKIMAGE          LVM_GETBKIMAGEA
#endif
#endif

#ifndef LVBKIF_SOURCE_NONE
#define LVBKIF_SOURCE_NONE      0x00000000
#endif // LVBKIF_SOURCE_NONE

#ifndef LVBKIF_SOURCE_HBITMAP
#define LVBKIF_SOURCE_HBITMAP   0x00000001
#endif

#ifndef LVBKIF_SOURCE_URL
#define LVBKIF_SOURCE_URL       0x00000002
#endif // LVBKIF_SOURCE_URL

#ifndef LVBKIF_SOURCE_MASK
#define LVBKIF_SOURCE_MASK      0x00000003
#endif // LVBKIF_SOURCE_MASK

#ifndef LVBKIF_STYLE_NORMAL
#define LVBKIF_STYLE_NORMAL     0x00000000
#endif // LVBKIF_STYLE_NORMAL

#ifndef LVBKIF_STYLE_TILE
#define LVBKIF_STYLE_TILE       0x00000010
#endif // LVBKIF_STYLE_TILE

#ifndef LVBKIF_STYLE_MASK
#define LVBKIF_STYLE_MASK       0x00000010
#endif // LVBKIF_STYLE_MASK

#ifndef ListView_SetBkImage
#define ListView_SetBkImage(hwnd, plvbki) \
	(BOOL)SNDMSG((hwnd), LVM_SETBKIMAGE, 0, (LPARAM)(plvbki))
#endif // ListView_SetBkImage

#ifndef ListView_GetBkImage
#define ListView_GetBkImage(hwnd, plvbki) \
	(BOOL)SNDMSG((hwnd), LVM_GETBKIMAGE, 0, (LPARAM)(plvbki))
#endif // ListView_GetBkImage

#ifndef LVS_EX_LABELTIP
#define LVS_EX_LABELTIP         0x00004000 // listview unfolds partly hidden labels if it does not have infotip text
#endif

/***************************************************************************
 externally defined global variables
 ***************************************************************************/

extern const ICONDATA g_iconData[];
UINT8 playopts_apply = 0;
static bool m_resized = false;

struct play_options
{
	std::string record;      // OPTION_RECORD
	std::string playback;    // OPTION_PLAYBACK
	std::string state;       // OPTION_STATE
	std::string wavwrite;    // OPTION_WAVWRITE
	std::string mngwrite;    // OPTION_MNGWRITE
	std::string aviwrite;    // OPTION_AVIWRITE
};

/***************************************************************************
    External variables
 ***************************************************************************/

/***************************************************************************
    Internal structures
 ***************************************************************************/

// These next two structs represent how the icon information
// is stored in an ICO file.
using ICONDIRENTRY = struct icon_directory_entry
{
	BYTE    bWidth;               // Width of the image
	BYTE    bHeight;              // Height of the image (times 2)
	BYTE    bColorCount;          // Number of colors in image (0 if >=8bpp)
	BYTE    bReserved;            // Reserved
	WORD    wPlanes;              // Color Planes
	WORD    wBitCount;            // Bits per pixel
	DWORD   dwBytesInRes;         // how many bytes in this resource?
	DWORD   dwImageOffset;        // where in the file is this image
};
using LPICONDIRENTRY = ICONDIRENTRY*;

using ICONIMAGE = struct icon_image
{
	UINT            Width, Height, Colors; // Width, Height and bpp
	LPBYTE          lpBits;                // ptr to DIB bits
	DWORD           dwNumBytes;            // how many bytes?
	LPBITMAPINFO    lpbi;                  // ptr to header
	LPBYTE          lpXOR;                 // ptr to XOR image bits
	LPBYTE          lpAND;                 // ptr to AND image bits
};
using LPICONIMAGE = ICONIMAGE*;

using ResizeItem = struct resize_item
{
	int         type;       // Either RA_ID or RA_HWND, to indicate which member of u is used; or RA_END
	// to signify last entry
	union                   // Can identify a child window by control id or by handle
	{
		int     id;         // Window control id
		HWND    hwnd;       // Window handle
	} u;
	bool        setfont;    // Do we set this item's font?
	int         action;     // What to do when control is resized
	void* subwindow; // Points to a Resize structure for this subwindow; nullptr if none
};
using LPResizeItem = ResizeItem*;

using GUISequence = struct gui_sequence
{
	char        name[40];    // functionality name (optional)
	input_seq   is;      // the input sequence (the keys pressed)
	UINT        func_id;        // the identifier
	input_seq* (* const getiniptr)(void);  // pointer to function to get the value from .ini file
};
using LPGUISequence = GUISequence*;

using Resize = struct resize_window
{
	RECT        rect;       // Client rect of window; must be initialized before first resize
	const ResizeItem* items;      // Array of subitems to be resized
};
using LPResize = Resize*;

struct popup_string
{
	HMENU hMenu;
	UINT uiString;
};
using POPUPSTRING = struct popup_string;

// Struct needed for Game Window Communication

struct find_window_handle
{
	LPPROCESS_INFORMATION ProcessInfo;
	HWND hwndFound;
};
using FINDWINDOWHANDLE = struct find_window_handle;
using LPFINDWINDOWHANDLE = struct find_window_handle*;

/***************************************************************************
    Internal variables
 ***************************************************************************/

// This ifdef block was transplanted from layout.cpp. I'm not to sure it
// ever really needed to be there, but it'll be safe here for now.
#ifdef MESS
constexpr wchar_t g_szPlayGameString[] = L"&Run ";
constexpr char g_szGameCountString[] = "%d machines";
#else
constexpr wchar_t g_szPlayGameString[] = L"&Play ";
constexpr char g_szGameCountString[] = "%d games";
#endif

// use a joystick subsystem in the gui?
std::unique_ptr<DIJoystickCallbacks> g_pJoyGUI;

// store current keyboard state (in bools) here
static bool keyboard_state[4096]; // __code_max #defines the number of internal key_codes

// search
std::string g_SearchText;

// master keyboard translation table
static const int win_key_trans_table[][4] =
{
	// MAME key             dinput key          virtual key     ascii
	{ ITEM_ID_ESC,          DIK_ESCAPE,         VK_ESCAPE,      27 },
	{ ITEM_ID_1,            DIK_1,              '1',            '1' },
	{ ITEM_ID_2,            DIK_2,              '2',            '2' },
	{ ITEM_ID_3,            DIK_3,              '3',            '3' },
	{ ITEM_ID_4,            DIK_4,              '4',            '4' },
	{ ITEM_ID_5,            DIK_5,              '5',            '5' },
	{ ITEM_ID_6,            DIK_6,              '6',            '6' },
	{ ITEM_ID_7,            DIK_7,              '7',            '7' },
	{ ITEM_ID_8,            DIK_8,              '8',            '8' },
	{ ITEM_ID_9,            DIK_9,              '9',            '9' },
	{ ITEM_ID_0,            DIK_0,              '0',            '0' },
	{ ITEM_ID_BACKSPACE,    DIK_BACK,           VK_BACK,        8 },
	{ ITEM_ID_TAB,          DIK_TAB,            VK_TAB,         9 },
	{ ITEM_ID_Q,            DIK_Q,              'Q',            'Q' },
	{ ITEM_ID_W,            DIK_W,              'W',            'W' },
	{ ITEM_ID_E,            DIK_E,              'E',            'E' },
	{ ITEM_ID_R,            DIK_R,              'R',            'R' },
	{ ITEM_ID_T,            DIK_T,              'T',            'T' },
	{ ITEM_ID_Y,            DIK_Y,              'Y',            'Y' },
	{ ITEM_ID_U,            DIK_U,              'U',            'U' },
	{ ITEM_ID_I,            DIK_I,              'I',            'I' },
	{ ITEM_ID_O,            DIK_O,              'O',            'O' },
	{ ITEM_ID_P,            DIK_P,              'P',            'P' },
	{ ITEM_ID_OPENBRACE,    DIK_LBRACKET,       VK_OEM_4,       '[' },
	{ ITEM_ID_CLOSEBRACE,   DIK_RBRACKET,       VK_OEM_6,       ']' },
	{ ITEM_ID_ENTER,        DIK_RETURN,         VK_RETURN,      13 },
	{ ITEM_ID_LCONTROL,     DIK_LCONTROL,       VK_LCONTROL,    0 },
	{ ITEM_ID_A,            DIK_A,              'A',            'A' },
	{ ITEM_ID_S,            DIK_S,              'S',            'S' },
	{ ITEM_ID_D,            DIK_D,              'D',            'D' },
	{ ITEM_ID_F,            DIK_F,              'F',            'F' },
	{ ITEM_ID_G,            DIK_G,              'G',            'G' },
	{ ITEM_ID_H,            DIK_H,              'H',            'H' },
	{ ITEM_ID_J,            DIK_J,              'J',            'J' },
	{ ITEM_ID_K,            DIK_K,              'K',            'K' },
	{ ITEM_ID_L,            DIK_L,              'L',            'L' },
	{ ITEM_ID_COLON,        DIK_SEMICOLON,      VK_OEM_1,       ';' },
	{ ITEM_ID_QUOTE,        DIK_APOSTROPHE,     VK_OEM_7,       '\'' },
	{ ITEM_ID_TILDE,        DIK_GRAVE,          VK_OEM_3,       '`' },
	{ ITEM_ID_LSHIFT,       DIK_LSHIFT,         VK_LSHIFT,      0 },
	{ ITEM_ID_BACKSLASH,    DIK_BACKSLASH,      VK_OEM_5,       '\\' },
	{ ITEM_ID_Z,            DIK_Z,              'Z',            'Z' },
	{ ITEM_ID_X,            DIK_X,              'X',            'X' },
	{ ITEM_ID_C,            DIK_C,              'C',            'C' },
	{ ITEM_ID_V,            DIK_V,              'V',            'V' },
	{ ITEM_ID_B,            DIK_B,              'B',            'B' },
	{ ITEM_ID_N,            DIK_N,              'N',            'N' },
	{ ITEM_ID_M,            DIK_M,              'M',            'M' },
	{ ITEM_ID_SLASH,        DIK_SLASH,          VK_OEM_2,       '/' },
	{ ITEM_ID_RSHIFT,       DIK_RSHIFT,         VK_RSHIFT,      0 },
	{ ITEM_ID_ASTERISK,     DIK_MULTIPLY,       VK_MULTIPLY,    '*' },
	{ ITEM_ID_LALT,         DIK_LMENU,          VK_LMENU,       0 },
	{ ITEM_ID_SPACE,        DIK_SPACE,          VK_SPACE,       ' ' },
	{ ITEM_ID_CAPSLOCK,     DIK_CAPITAL,        VK_CAPITAL,     0 },
	{ ITEM_ID_F1,           DIK_F1,             VK_F1,          0 },
	{ ITEM_ID_F2,           DIK_F2,             VK_F2,          0 },
	{ ITEM_ID_F3,           DIK_F3,             VK_F3,          0 },
	{ ITEM_ID_F4,           DIK_F4,             VK_F4,          0 },
	{ ITEM_ID_F5,           DIK_F5,             VK_F5,          0 },
	{ ITEM_ID_F6,           DIK_F6,             VK_F6,          0 },
	{ ITEM_ID_F7,           DIK_F7,             VK_F7,          0 },
	{ ITEM_ID_F8,           DIK_F8,             VK_F8,          0 },
	{ ITEM_ID_F9,           DIK_F9,             VK_F9,          0 },
	{ ITEM_ID_F10,          DIK_F10,            VK_F10,         0 },
	{ ITEM_ID_NUMLOCK,      DIK_NUMLOCK,        VK_NUMLOCK,     0 },
	{ ITEM_ID_SCRLOCK,      DIK_SCROLL,         VK_SCROLL,      0 },
	{ ITEM_ID_7_PAD,        DIK_NUMPAD7,        VK_NUMPAD7,     0 },
	{ ITEM_ID_8_PAD,        DIK_NUMPAD8,        VK_NUMPAD8,     0 },
	{ ITEM_ID_9_PAD,        DIK_NUMPAD9,        VK_NUMPAD9,     0 },
	{ ITEM_ID_MINUS_PAD,    DIK_SUBTRACT,       VK_SUBTRACT,    0 },
	{ ITEM_ID_4_PAD,        DIK_NUMPAD4,        VK_NUMPAD4,     0 },
	{ ITEM_ID_5_PAD,        DIK_NUMPAD5,        VK_NUMPAD5,     0 },
	{ ITEM_ID_6_PAD,        DIK_NUMPAD6,        VK_NUMPAD6,     0 },
	{ ITEM_ID_PLUS_PAD,     DIK_ADD,            VK_ADD,         0 },
	{ ITEM_ID_1_PAD,        DIK_NUMPAD1,        VK_NUMPAD1,     0 },
	{ ITEM_ID_2_PAD,        DIK_NUMPAD2,        VK_NUMPAD2,     0 },
	{ ITEM_ID_3_PAD,        DIK_NUMPAD3,        VK_NUMPAD3,     0 },
	{ ITEM_ID_0_PAD,        DIK_NUMPAD0,        VK_NUMPAD0,     0 },
	{ ITEM_ID_DEL_PAD,      DIK_DECIMAL,        VK_DECIMAL,     0 },
	{ ITEM_ID_F11,          DIK_F11,            VK_F11,         0 },
	{ ITEM_ID_F12,          DIK_F12,            VK_F12,         0 },
	{ ITEM_ID_F13,          DIK_F13,            VK_F13,         0 },
	{ ITEM_ID_F14,          DIK_F14,            VK_F14,         0 },
	{ ITEM_ID_F15,          DIK_F15,            VK_F15,         0 },
	{ ITEM_ID_ENTER_PAD,    DIK_NUMPADENTER,    VK_RETURN,      0 },
	{ ITEM_ID_RCONTROL,     DIK_RCONTROL,       VK_RCONTROL,    0 },
	{ ITEM_ID_SLASH_PAD,    DIK_DIVIDE,         VK_DIVIDE,      0 },
	{ ITEM_ID_PRTSCR,       DIK_SYSRQ,          0,              0 },
	{ ITEM_ID_RALT,         DIK_RMENU,          VK_RMENU,       0 },
	{ ITEM_ID_HOME,         DIK_HOME,           VK_HOME,        0 },
	{ ITEM_ID_UP,           DIK_UP,             VK_UP,          0 },
	{ ITEM_ID_PGUP,         DIK_PRIOR,          VK_PRIOR,       0 },
	{ ITEM_ID_LEFT,         DIK_LEFT,           VK_LEFT,        0 },
	{ ITEM_ID_RIGHT,        DIK_RIGHT,          VK_RIGHT,       0 },
	{ ITEM_ID_END,          DIK_END,            VK_END,         0 },
	{ ITEM_ID_DOWN,         DIK_DOWN,           VK_DOWN,        0 },
	{ ITEM_ID_PGDN,         DIK_NEXT,           VK_NEXT,        0 },
	{ ITEM_ID_INSERT,       DIK_INSERT,         VK_INSERT,      0 },
	{ ITEM_ID_DEL,          DIK_DELETE,         VK_DELETE,      0 },
	{ ITEM_ID_LWIN,         DIK_LWIN,           VK_LWIN,        0 },
	{ ITEM_ID_RWIN,         DIK_RWIN,           VK_RWIN,        0 },
	{ ITEM_ID_MENU,         DIK_APPS,           VK_APPS,        0 },
	{ ITEM_ID_PAUSE,        DIK_PAUSE,          VK_PAUSE,       0 },
	{ ITEM_ID_CANCEL,       0,                  VK_CANCEL,      0 },
};

static const GUISequence GUISequenceControl[] =
{
	{"gui_key_up",                   input_seq(),  ID_UI_UP,                  Get_ui_key_up },
	{"gui_key_down",                 input_seq(),  ID_UI_DOWN,                Get_ui_key_down },
	{"gui_key_left",                 input_seq(),  ID_UI_LEFT,                Get_ui_key_left },
	{"gui_key_right",                input_seq(),  ID_UI_RIGHT,               Get_ui_key_right },
	{"gui_key_start",                input_seq(),  ID_UI_START,               Get_ui_key_start },
	{"gui_key_pgup",                 input_seq(),  ID_UI_PGUP,                Get_ui_key_pgup },
	{"gui_key_pgdwn",                input_seq(),  ID_UI_PGDOWN,              Get_ui_key_pgdwn },
	{"gui_key_home",                 input_seq(),  ID_UI_HOME,                Get_ui_key_home },
	{"gui_key_end",                  input_seq(),  ID_UI_END,                 Get_ui_key_end },
	{"gui_key_ss_change",            input_seq(),  IDC_SSFRAME,               Get_ui_key_ss_change },
	{"gui_key_history_up",           input_seq(),  ID_UI_HISTORY_UP,          Get_ui_key_history_up },
	{"gui_key_history_down",         input_seq(),  ID_UI_HISTORY_DOWN,        Get_ui_key_history_down },

	{"gui_key_context_filters",      input_seq(),  ID_CONTEXT_FILTERS,        Get_ui_key_context_filters },
	{"gui_key_select_random",        input_seq(),  ID_CONTEXT_SELECT_RANDOM,  Get_ui_key_select_random },
	{"gui_key_game_audit",           input_seq(),  ID_GAME_AUDIT,             Get_ui_key_game_audit },
	{"gui_key_game_properties",      input_seq(),  ID_GAME_PROPERTIES,        Get_ui_key_game_properties },
	{"gui_key_help_contents",        input_seq(),  ID_HELP_CONTENTS,          Get_ui_key_help_contents },
	{"gui_key_update_gamelist",      input_seq(),  ID_UPDATE_GAMELIST,        Get_ui_key_update_gamelist },
	{"gui_key_view_folders",         input_seq(),  ID_VIEW_FOLDERS,           Get_ui_key_view_folders },
	{"gui_key_view_fullscreen",      input_seq(),  ID_VIEW_FULLSCREEN,        Get_ui_key_view_fullscreen },
	{"gui_key_view_pagetab",         input_seq(),  ID_VIEW_PAGETAB,           Get_ui_key_view_pagetab },
	{"gui_key_view_picture_area",    input_seq(),  ID_VIEW_PICTURE_AREA,      Get_ui_key_view_picture_area },
	{"gui_key_view_software_area",   input_seq(),  ID_VIEW_SOFTWARE_AREA,     Get_ui_key_view_software_area },
	{"gui_key_view_status",          input_seq(),  ID_VIEW_STATUS,            Get_ui_key_view_status },
	{"gui_key_view_toolbars",        input_seq(),  ID_VIEW_TOOLBARS,          Get_ui_key_view_toolbars },

	{"gui_key_view_tab_cabinet",     input_seq(),  ID_VIEW_TAB_CABINET,       Get_ui_key_view_tab_cabinet },
	{"gui_key_view_tab_cpanel",      input_seq(),  ID_VIEW_TAB_CONTROL_PANEL, Get_ui_key_view_tab_cpanel },
	{"gui_key_view_tab_flyer",       input_seq(),  ID_VIEW_TAB_FLYER,         Get_ui_key_view_tab_flyer },
	{"gui_key_view_tab_history",     input_seq(),  ID_VIEW_TAB_HISTORY,       Get_ui_key_view_tab_history },
	{"gui_key_view_tab_marquee",     input_seq(),  ID_VIEW_TAB_MARQUEE,       Get_ui_key_view_tab_marquee },
	{"gui_key_view_tab_screenshot",  input_seq(),  ID_VIEW_TAB_SCREENSHOT,    Get_ui_key_view_tab_screenshot },
	{"gui_key_view_tab_title",       input_seq(),  ID_VIEW_TAB_TITLE,         Get_ui_key_view_tab_title },
	{"gui_key_view_tab_pcb",         input_seq(),  ID_VIEW_TAB_PCB,           Get_ui_key_view_tab_pcb },
	{"gui_key_quit",                 input_seq(),  ID_FILE_EXIT,              Get_ui_key_quit },
};

static const TBBUTTON tbb[] =
{
	{0, ID_VIEW_FOLDERS,       TBSTATE_ENABLED, TBSTYLE_CHECK,      {0, 0}, 0, 0},
	{1, ID_VIEW_SOFTWARE_AREA, TBSTATE_ENABLED, TBSTYLE_CHECK,      {0, 0}, 0, 1},
	{1, ID_VIEW_PICTURE_AREA,  TBSTATE_ENABLED, TBSTYLE_CHECK,      {0, 0}, 0, 1},
	{0, 0,                     TBSTATE_ENABLED, TBSTYLE_SEP,        {0, 0}, 0, 0},
	{2, ID_VIEW_LARGE_ICON,    TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 2},
	{3, ID_VIEW_SMALL_ICON,    TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 3},
	{4, ID_VIEW_LIST_MENU,     TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 4},
	{5, ID_VIEW_DETAIL,        TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 5},
	{6, ID_VIEW_GROUPED,       TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 6},
	{0, 0,                     TBSTATE_ENABLED, TBSTYLE_SEP,        {0, 0}, 0, 0},
	{7, ID_HELP_ABOUT,         TBSTATE_ENABLED, TBSTYLE_BUTTON,     {0, 0}, 0, 7},
	//  {8, ID_HELP_CONTENTS,      TBSTATE_ENABLED, TBSTYLE_BUTTON,     {0, 0}, 0, 8}
};

constexpr auto NUM_TOOLTIPS = 9;

static const wchar_t szTbStrings[NUM_TOOLTIPS + 1][30] =
{
	L"Toggle Folder List",
	L"Toggle Software List",
	L"Toggle Screen Shot",
	L"Large Icons",
	L"Small Icons",
	L"List",
	L"Details",
	L"Grouped",
	L"About",
	L"Help"
};

static const int CommandToString[] =
{
	ID_VIEW_FOLDERS,
	ID_VIEW_SOFTWARE_AREA,
	ID_VIEW_PICTURE_AREA,
	ID_VIEW_LARGE_ICON,
	ID_VIEW_SMALL_ICON,
	ID_VIEW_LIST_MENU,
	ID_VIEW_DETAIL,
	ID_VIEW_GROUPED,
	ID_HELP_ABOUT,
	ID_HELP_CONTENTS,
	-1
};

static const int s_nPickers[] =
{
	IDC_LIST,
	//#ifdef MESS
		IDC_SWLIST,
		IDC_SOFTLIST
		//#endif
};

// Which edges of a control are anchored to the corresponding side of the parent window
constexpr auto RA_LEFT = 1;
constexpr auto RA_RIGHT = 2;
constexpr auto RA_TOP = 4;
constexpr auto RA_BOTTOM = 8;
constexpr auto RA_ALL = 15;

constexpr auto RA_END = 0;
constexpr auto RA_ID = 1;
constexpr auto RA_HWND = 2;

// How to resize toolbar sub window
static ResizeItem toolbar_resize_items[] =
{
	{ RA_ID,   { ID_TOOLBAR_EDIT }, true,  RA_RIGHT | RA_TOP, nullptr },
	{ RA_END,  { 0 },               false, 0,                 nullptr }
};

static Resize toolbar_resize = { {0, 0, 0, 0}, toolbar_resize_items };

// How to resize main window
static ResizeItem main_resize_items[] =
{
	{ RA_HWND, { 0 },            false, RA_LEFT | RA_RIGHT | RA_TOP,     &toolbar_resize },
	{ RA_HWND, { 0 },            false, RA_LEFT | RA_RIGHT | RA_BOTTOM,  nullptr },
	{ RA_ID,   { IDC_DIVIDER },  false, RA_LEFT | RA_RIGHT | RA_TOP,     nullptr },
	{ RA_ID,   { IDC_TREE },     true,  RA_LEFT | RA_BOTTOM | RA_TOP,     nullptr },
	{ RA_ID,   { IDC_LIST },     true,  RA_ALL,                            nullptr },
	{ RA_ID,   { IDC_SPLITTER }, false, RA_LEFT | RA_BOTTOM | RA_TOP,     nullptr },
	{ RA_ID,   { IDC_SPLITTER2 },FALSE, RA_LEFT | RA_BOTTOM | RA_TOP,     nullptr },
	{ RA_ID,   { IDC_SSFRAME },  false, RA_LEFT | RA_BOTTOM | RA_TOP,     nullptr },
	{ RA_ID,   { IDC_SSPICTURE },FALSE, RA_LEFT | RA_BOTTOM | RA_TOP,     nullptr },
	{ RA_ID,   { IDC_HISTORY },  true,  RA_LEFT | RA_BOTTOM | RA_TOP,     nullptr },
	{ RA_ID,   { IDC_SSTAB },    false, RA_LEFT | RA_TOP,                 nullptr },
	//#ifdef MESS
		{ RA_ID,   { IDC_SWLIST },    true, RA_LEFT | RA_BOTTOM | RA_TOP,     nullptr },
		{ RA_ID,   { IDC_SOFTLIST },  true, RA_LEFT | RA_BOTTOM | RA_TOP,     nullptr },
		//#endif
			{ RA_ID,   { IDC_SPLITTER3 },FALSE, RA_LEFT | RA_BOTTOM | RA_TOP,     nullptr },
			{ RA_END,  { 0 },            false, 0,                                 nullptr }
};

static Resize main_resize = { {0, 0, 0, 0}, main_resize_items };

constexpr auto JOYGUI_MS = 100;

constexpr auto JOYGUI_TIMER = 1;
constexpr auto SCREENSHOT_TIMER = 2;
constexpr auto GAMEWND_TIMER = 3;

// Max size of a sub-menu
constexpr auto DBU_MIN_WIDTH = 292;
constexpr auto DBU_MIN_HEIGHT = 190;

constexpr auto NO_FOLDER = -1;
constexpr auto STATESAVE_VERSION = 1;
//I could not find a predefined value for this event and docs just say it has 1 for the parameter
constexpr auto TOOLBAR_EDIT_ACCELERATOR_PRESSED = 1;

constexpr auto WM_MAMEUI_FILECHANGED = WM_USER + 808;
constexpr auto WM_MAMEUI_AUDITGAME = WM_USER + 657;
constexpr auto WM_MAMEUI_PLAYGAME = WM_APP + 1542;

constexpr auto MAX_MENUS = 3;

// table copied from windows/inputs.c
// table entry indices
constexpr auto MAME_KEY = 0;
constexpr auto DI_KEY = 1;
constexpr auto VIRTUAL_KEY = 2;
constexpr auto ASCII_KEY = 3;

constexpr auto NUM_GUI_SEQUENCES = std::size(GUISequenceControl);

constexpr auto NUM_TOOLBUTTONS = std::size(tbb);

// List view Icon defines
const auto LG_ICONMAP_WIDTH = windows::get_system_metrics(SM_CXICON);
const auto LG_ICONMAP_HEIGHT = windows::get_system_metrics(SM_CYICON);
const auto ICONMAP_WIDTH = windows::get_system_metrics(SM_CXSMICON);
const auto ICONMAP_HEIGHT = windows::get_system_metrics(SM_CYSMICON);

static int MIN_WIDTH = DBU_MIN_WIDTH;
static int MIN_HEIGHT = DBU_MIN_HEIGHT;

static HBRUSH hBrush = nullptr;
//static HBRUSH hBrushDlg = nullptr;
static HDC hDC = nullptr;
static HWND hSplash = nullptr;
static HWND hProgress = nullptr;
static intptr_t CALLBACK StartupProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static bool m_lock = false;    // prevent MAME from being launched twice by accident, and crashing the entire app.

static HWND   hMain  = nullptr;
static HMENU  hMainMenu = nullptr;
static HACCEL hAccel = nullptr;

static HWND hwndList  = nullptr;
static HWND hTreeView = nullptr;
static HWND hProgWnd  = nullptr;
static HWND hTabCtrl  = nullptr;

static HINSTANCE hInst = nullptr;

static HFONT hFont = nullptr;     // Font for list view

static int optionfolder_count = 0;

// global data--know where to send messages
static bool in_emulation = 0;

// idle work at startup
static bool idle_work = false;

static int  game_index = 0;
static int  progBarStep = 0;

static bool bDoGameCheck = false;

// Tree control variables
static bool bShowTree       = true;
static bool bShowToolBar    = true;
static bool bShowStatusBar  = true;
static bool bShowTabCtrl    = true;
static bool bProgressShown  = false;
static bool bListReady      = false;

static PDirWatcher s_pWatcher;

static UINT    lastColumnClick = 0;
static WNDPROC g_lpHistoryWndProc = nullptr;
static WNDPROC g_lpPictureFrameWndProc = nullptr;
static WNDPROC g_lpPictureWndProc = nullptr;

static POPUPSTRING popstr[MAX_MENUS + 1];

// Tool and Status bar variables
static HWND hStatusBar = 0;
static HWND s_hToolBar = 0;

// Used to recalculate the main window layout
static int  bottomMargin = 0;
static int  topMargin = 0;
static bool  have_history = false;

static bool have_selection = false;

static HBITMAP hMissing_bitmap = nullptr;

// Icon variables
static HIMAGELIST   hLarge = nullptr;
static HIMAGELIST   hSmall = nullptr;
static HIMAGELIST   hHeaderImages = nullptr;
static std::unique_ptr<int[]> icon_index; // for custom per-game icons

static bool g_listview_dragging = false;
static HIMAGELIST himl_drag;
static int game_dragged; // which game started the drag
static HTREEITEM prev_drag_drop_target = nullptr; // which tree view item we're currently highlighting

static bool g_in_treeview_edit = false;

constexpr int MAX_SCREENSHOT_HEIGHT = 264; // Max height of screenshot area

/***************************************************************************
    Global variables
 ***************************************************************************/

// Background Image handles also accessed from TreeView.cpp
static HPALETTE         hpBackground   = nullptr;
static HBITMAP          hbBackground  = nullptr;
static MYBITMAPINFO     bmDesc;

// List view Column text
const std::wstring column_names[COLUMN_COUNT] =
{
	L"Machine",
	L"Source",
	L"Directory",
	L"Type",
	L"Screen",
	L"Manufacturer",
	L"Year",
	L"Played",
	L"Play Time",
	L"Clone Of",
	L"Trackball",
	L"Samples",
	L"ROMs"
};

/***************************************************************************
    function prototypes
 ***************************************************************************/

static bool MameUI_init(HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow);
static void MameUI_exit(void);

static bool PumpMessage(void);
static bool OnIdle(HWND hWnd);
static void OnSize(HWND hwnd, UINT state, int width, int height);
static LRESULT CALLBACK MameUIWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void SetView(int menu_id);
static void ResetListView(void);
static void UpdateGameList(bool bUpdateRomAudit, bool bUpdateSampleAudit);
static void DestroyIcons(void);
static void ReloadIcons(void);
static void PollGUIJoystick(void);
//static void PressKey(HWND hwnd,UINT vk);
static bool MameUICommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static void KeyboardKeyDown(int syskey, int vk_code, int special);
static void KeyboardKeyUp(int syskey, int vk_code, int special);
static void KeyboardStateClear(void);

static void UpdateStatusBar(void);
//static bool PickerHitTest(HWND hWnd);
static bool TreeViewNotify(NMHDR *nm);

//static void ResetBackground(char *szFile);
std::optional<HICON> LoadIconFromArchive(const std::filesystem::path &parent_path, const std::string &icon_name);
static void LoadBackgroundBitmap(void);
static void PaintBackgroundImage(HWND hWnd, HRGN hRgn, int x, int y);

static void DisableSelection(void);
static void EnableSelection(int nGame);

static HICON GetSelectedPickItemIcon(void);
static void SetRandomPickItem(void);
static void PickCloneColor(void);
static void PickColor(COLORREF *cDefault);

static LPTREEFOLDER GetSelectedFolder(void);
static HICON GetSelectedFolderIcon(void);
static void RemoveCurrentGameCustomFolder(void);
static void RemoveGameCustomFolder(int driver_index);

static UINT_PTR CALLBACK CFHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK HistoryWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK PictureFrameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK PictureWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static void MameUI_RecordInput(void);
static void MameUI_PlayBackInput(void);
static void MamePlayRecordWave(void);
static void MamePlayRecordMNG(void);
static void MamePlayRecordAVI(void);
static void MameUI_LoadState(void);
static bool GameCheck(void);
static bool FolderCheck(void);

static void ToggleScreenShot(void);
static void ToggleSoftware(void);
static void AdjustMetrics(void);

// Icon routines
static DWORD GetShellLargeIconSize(void);
static DWORD GetShellSmallIconSize(void);
static void CreateIcons(void);
static int GetIconForDriver(int item_index);
static void AddDriverIcon(int item_index, int default_icon_index);

// Context Menu handlers
static void UpdateMenu(HMENU hMenu);
static void InitTreeContextMenu(HMENU hTreeMenu);
static void InitBodyContextMenu(HMENU hBodyContextMenu);
static void ToggleShowFolder(int folder);
static bool HandleTreeContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam);
static bool HandleScreenShotContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam);
static int compareAuditResults(int result1, int result2);
static int GamePicker_Compare(HWND hwndPicker, int index1, int index2, int sort_subitem, bool ascending = false);
static void GamePicker_OnHeaderContextMenu(POINT pt, int nColumn);
static void GamePicker_OnBodyContextMenu(POINT pt);

static void InitListView(void);

// Re/initialize the ListView header columns
static void ResetColumnDisplay(bool first_time);

static void BeginListViewDrag(NM_LISTVIEW *pnmv);
static void MouseMoveListViewDrag(POINTS pt);
static void ButtonUpListViewDrag(POINTS p);
static void CopyToolTipText(LPTOOLTIPTEXT lpttt);

static HWND InitProgressBar(HWND hParent);
static void ProgressBarShow(void);
static void ProgressBarHide(void);
static void ResizeProgressBar(void);
static void ProgressBarStep(void);
static void ProgressBarStepParam(int iGameIndex, int nGameCount);

static HWND InitToolbar(HWND hParent);

static HWND InitStatusBar(HWND hParent);
static LRESULT Statusbar_MenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam);

static void UpdateHistory(std::string software);

static void CalculateBestScreenShotRect(HWND hWnd, RECT *pRect, bool restrict_height);

bool MouseHasBeenMoved(void);
static void SwitchFullScreenMode(void);

static void ResizeWindow(HWND hParent, Resize *r);
static void SetAllWindowsFont(HWND hParent, const Resize *r, HFONT hFont, bool bRedraw);
static void CLIB_DECL MameMessageBox(const char *fmt, ...) ATTR_PRINTF(1, 2);
//static void CLIB_DECL MameMessageBoxW(const wchar_t *fmt, ...);

void delete_in_dirs(const stringtokenizer &tokenizer, const std::string &name, const std::string &extension = {});

/***************************************************************************
    Message Macros
 ***************************************************************************/

#ifndef StatusBar_GetItemRect
#define StatusBar_GetItemRect(hWnd, iPart, lpRect) \
   windows::send_message(hWnd, SB_GETRECT, (WPARAM) iPart, (LPARAM) (LPRECT) lpRect)
#endif

#ifndef ToolBar_CheckButton
#define ToolBar_CheckButton(hWnd, idButton, fCheck) \
   windows::send_message(hWnd, TB_CHECKBUTTON, (WPARAM)idButton, (LPARAM)MAKELONG(fCheck, 0))
#endif

//============================================================
//  winui_output_error
//============================================================
//  List of output types:
//      case OSD_OUTPUT_CHANNEL_ERROR:
//      case OSD_OUTPUT_CHANNEL_WARNING:
//          vfprintf(stderr, msg, args);     // send errors and warnings to standard error (=console)
//          break;
//      case OSD_OUTPUT_CHANNEL_INFO:
//      case OSD_OUTPUT_CHANNEL_LOG:
//          vfprintf(stdout, msg, args);     // send info and logging to standard output (=console)
//          break;
//      case OSD_OUTPUT_CHANNEL_VERBOSE:
//          if (verbose()) vfprintf(stdout, msg, args);      // send verbose (2nd half) to console if enabled (first half lost)
//          break;
//      case OSD_OUTPUT_CHANNEL_DEBUG:   // only for debug builds
//          vfprintf(stdout, msg, args);

class mameui_output_error : public osd_output
{
private:
	struct ui_state
	{
		~ui_state()
		{
			if (thread && thread->joinable())
				thread->join();
		}

		std::ostringstream buffer;
		std::optional<std::thread> thread;
		std::mutex mutex;
		bool active;
	};

	static ui_state& get_state()
	{
		static ui_state state;
		return state;
	}

public:
	virtual void output_callback(osd_output_channel channel, const util::format_argument_pack<char>& args) override
	{
		if (channel == OSD_OUTPUT_CHANNEL_ERROR)
		{
			// if we are in fullscreen mode, go to windowed mode
			if ((video_config.windowed == 0) && !osd_common_t::window_list().empty())
				winwindow_toggle_full_screen();

			auto& state(get_state());
			std::lock_guard<std::mutex> guard(state.mutex);
			util::stream_format(state.buffer, args);
			if (!state.active)
			{
				if (state.thread && state.thread->joinable())
				{
					state.thread->join();
					state.thread.reset();
				}
				state.thread.emplace(
					[]()
					{
						auto& state(get_state());
						std::string message;
						std::ofstream outfile("winui.log", std::ios::out | std::ios::app);

						while (true)
						{
							{
								std::lock_guard<std::mutex> guard(state.mutex);
								message = std::move(state.buffer).str();
								if (message.empty())
								{
									state.active = false;
									return;
								}
								state.buffer.str(std::string());
							}
							// Don't hold any locks lock while calling MessageBox.
							// Parent window isn't set because MAME could destroy
							// the window out from under us on a fatal error.
							dialog_boxes::message_box_utf8(nullptr, message.c_str(), emulator_info::get_appname(), MB_OK);
							outfile.write(message.c_str(), message.size());
							outfile.flush();
						}
					});
				state.active = true;
			}
		}
	}
};

/***************************************************************************
    External functions
 ***************************************************************************/
static DWORD RunMAME(int nGameIndex, std::shared_ptr<play_options> playopts)
{
	std::string name;
	double elapsedtime;
	mameui_output_error winerror;
	windows_options global_opts;
	windows_osd_interface osd(global_opts);
	std::string start_finish_message;
	time_t start = 0, end = 0;
	std::unique_ptr<mame_machine_manager> manager;
	std::ostringstream option_errors;

	m_lock = true;

	// Tell mame where to get the INIs
	emu_opts.load_options(emu_opts.GetGlobalOpts(), SOFTWARETYPE_GAME, nGameIndex, false);
	name = driver_list::driver(nGameIndex).name;

	// set some startup options
	global_opts.set_value(OPTION_LANGUAGE, emu_opts.GetLanguageUI(), OPTION_PRIORITY_HIGH);
	global_opts.set_value(OPTION_PLUGINDATAPATH, emu_opts.GetPluginDataPath(), OPTION_PRIORITY_HIGH);
	global_opts.set_value(OPTION_PLUGINS, emu_opts.GetEnablePlugins(), OPTION_PRIORITY_HIGH);
	global_opts.set_value(OPTION_PLUGIN, emu_opts.GetPlugins(), OPTION_PRIORITY_HIGH);
	global_opts.set_value(OPTION_SYSTEMNAME, name, OPTION_PRIORITY_HIGH);

	// set any specified play options
	if (playopts_apply == 0x57)
	{
		if (!playopts->record.empty())
			global_opts.set_value(OPTION_RECORD, playopts->record, OPTION_PRIORITY_HIGH);
		if (!playopts->playback.empty())
			global_opts.set_value(OPTION_PLAYBACK, playopts->playback, OPTION_PRIORITY_HIGH);
		if (!playopts->state.empty())
			global_opts.set_value(OPTION_STATE, playopts->state, OPTION_PRIORITY_HIGH);
		if (!playopts->wavwrite.empty())
			global_opts.set_value(OPTION_WAVWRITE, playopts->wavwrite, OPTION_PRIORITY_HIGH);
		if (!playopts->mngwrite.empty())
			global_opts.set_value(OPTION_MNGWRITE, playopts->mngwrite, OPTION_PRIORITY_HIGH);
		if (!playopts->aviwrite.empty())
			global_opts.set_value(OPTION_AVIWRITE, playopts->aviwrite, OPTION_PRIORITY_HIGH);
	}

	// redirect messages to our handler
	start_finish_message = pad_to_center("STARTING " + name, true);
	std::cout << start_finish_message << "\n";
	osd_output::push(&winerror);
	osd_printf_verbose(start_finish_message);
	osd_printf_info(start_finish_message);

	for (size_t i = 0; i < std::size(s_nPickers); i++)
		Picker_ClearIdle(dialog_boxes::get_dlg_item(hMain, s_nPickers[i]));

	// hide mameui
	(void)windows::show_window(hMain, SW_HIDE);

	// run the emulation

	// pass down any command-line arguments
	osd.register_options();
	manager.reset(mame_machine_manager::instance(global_opts, osd));

	// parse all the needed .ini files.
	mame_options::parse_standard_inis(global_opts, option_errors);
	load_translation(global_opts);

	// start processes
	manager->start_http_server();
	manager->start_luaengine();
	time(&start);

	// run the game
	manager->execute();

	// save game time played
	time(&end);

	start_finish_message = pad_to_center("FINISHED " + name,true);
	osd_printf_verbose(start_finish_message);
	osd_printf_info(start_finish_message);

	// turn off message redirect
	osd_output::pop(&winerror);
	std::cout << start_finish_message << "\n";

	// clear any specified play options
	// do it this way to preserve slots and software entries
	if (playopts_apply == 0x57)
	{
		windows_options o;

		emu_opts.load_options(o, SOFTWARETYPE_GAME, nGameIndex, 0);
		if (!playopts->record.empty())
			o.set_value(OPTION_RECORD, "", OPTION_PRIORITY_HIGH);
		if (!playopts->playback.empty())
			o.set_value(OPTION_PLAYBACK, "", OPTION_PRIORITY_HIGH);
		if (!playopts->state.empty())
			o.set_value(OPTION_STATE, "", OPTION_PRIORITY_HIGH);
		if (!playopts->wavwrite.empty())
			o.set_value(OPTION_WAVWRITE, "", OPTION_PRIORITY_HIGH);
		if (!playopts->mngwrite.empty())
			o.set_value(OPTION_MNGWRITE, "", OPTION_PRIORITY_HIGH);
		if (!playopts->aviwrite.empty())
			o.set_value(OPTION_AVIWRITE, "", OPTION_PRIORITY_HIGH);
		// apply the above to the ini file
		emu_opts.save_options(o, SOFTWARETYPE_GAME, nGameIndex);
	}
	playopts_apply = 0;

	playopts.reset();

	elapsedtime = end - start;
	IncrementPlayTime(nGameIndex, elapsedtime);

	// the emulation is complete; continue
	for (size_t i = 0; i < std::size(s_nPickers); i++)
		Picker_ResetIdle(dialog_boxes::get_dlg_item(hMain, s_nPickers[i]));

	(void)windows::show_window(hMain, SW_SHOW);
	(void)windows::set_active_window(hMain);

//#ifdef MESS
	// update display in case software was changed by the machine or by newui
	MessReadMountedSoftware(nGameIndex); // messui.cpp
//#endif

	m_lock = false;

	return static_cast<DWORD>(0);
}

int MameUIMain(HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	std::setlocale(LC_ALL, "");
	// delete old log file, ignore any error
	unlink("winui.log");

	// console output not allowed before here, else they get into mame queries
	std::cout << "MAMEUI starting" << "\n";

	hSplash = dialog_boxes::create_dialog(hInstance, menus::make_int_resource(IDD_STARTUP), hMain, StartupProc, 0L);
	(void)windows::set_active_window(hSplash);
	(void)windows::set_active_window(hSplash);

	bool bResult = MameUI_init(hInstance, lpCmdLine, nCmdShow);
	windows::destroy_window(hSplash);
	if (!bResult)
		return 1;

	// pump message, but quit on WM_QUIT
	while (PumpMessage());

	MameUI_exit();
	return 0;
}


HWND GetMainWindow(void)
{
	return hMain;
}


HWND GetTreeView(void)
{
	return hTreeView;
}


// used by messui.cpp
HIMAGELIST GetLargeImageList(void)
{
	return hLarge;
}


// used by messui.cpp
HIMAGELIST GetSmallImageList(void)
{
	return hSmall;
}


void GetRealColumnOrder(int order[])
{
	int tmpOrder[COLUMN_COUNT] = {0};
	int nColumnMax = Picker_GetNumColumns(hwndList);

	// Get the Column Order and save it
	list_view::get_column_order_array(hwndList, nColumnMax, tmpOrder);

	for (size_t i = 0; i < nColumnMax; i++)
		order[i] = Picker_GetRealColumnFromViewColumn(hwndList, tmpOrder[i]);
}


//
// PURPOSE: Format raw data read from an ICO file to an HICON
// PARAMS:  PBYTE ptrBuffer  - Raw data from an ICO file
//          UINT nBufferSize - Size of buffer ptrBuffer
// RETURNS: HICON - handle to the icon, nullptr for failure
// History: July '95 - Created
//          March '00- Seriously butchered from MSDN for mine own
//          purposes, sayeth H0ek.
//----------------------------------------------------------------

static HICON FormatICOInMemoryToHICON(PBYTE ptrBuffer, UINT nBufferSize)
{
	HICON hIcon = nullptr;
	ICONIMAGE IconImage;
	LPICONDIRENTRY lpIDE;
	UINT nSizeofWord = sizeof(WORD),
		nBufferIndex = 0,
		nNumImages,
		nIdeBufferSize;

	// Is there a WORD?
	if (nBufferSize < nSizeofWord)
		return nullptr;

	// Was it 'reserved' ?   (ie 0)
	if ((WORD)(ptrBuffer[nBufferIndex]) != 0)
		return nullptr;

	nBufferIndex += nSizeofWord;

	// Is there a WORD?
	if (nBufferSize - nBufferIndex < nSizeofWord)
		return nullptr;

	// Was it type 1?
	if ((WORD)(ptrBuffer[nBufferIndex]) != 1)
		return nullptr;

	nBufferIndex += nSizeofWord;

	// Is there a WORD?
	if (nBufferSize - nBufferIndex < nSizeofWord)
		return nullptr;

	// Then that's the number of images in the ICO file
	nNumImages = (WORD)(ptrBuffer[nBufferIndex]);

	// Is there at least one icon in the file?
	if ( nNumImages < 1 )
		return nullptr;

	nBufferIndex += nSizeofWord;
	nIdeBufferSize = nNumImages * sizeof(ICONDIRENTRY);
	// Is there enough space for the icon directory entries?
	if ((nBufferIndex + nIdeBufferSize) > nBufferSize)
		return nullptr;

	// Assign icon directory entries from buffer
	lpIDE = (LPICONDIRENTRY)(&ptrBuffer[nBufferIndex]);
	nBufferIndex += nIdeBufferSize;

	IconImage.dwNumBytes = lpIDE->dwBytesInRes;

	// Seek to beginning of this image
	if ( lpIDE->dwImageOffset > nBufferSize )
		return nullptr;

	nBufferIndex = lpIDE->dwImageOffset;

	// Read it in
	if ((nBufferIndex + lpIDE->dwBytesInRes) > nBufferSize)
		return nullptr;

	IconImage.lpBits = &ptrBuffer[nBufferIndex];
	nBufferIndex += lpIDE->dwBytesInRes;

	// We would break on NT if we try with a 16bpp image
	if (((LPBITMAPINFO)IconImage.lpBits)->bmiHeader.biBitCount != 16)
		hIcon = menus::create_icon_from_resource_ex(IconImage.lpBits, IconImage.dwNumBytes, true, 0x00030000,0,0,LR_DEFAULTSIZE);

	return hIcon;
}

std::optional<HICON> LoadIconFromArchive( const std::filesystem::path &parent_path, const std::string &icon_name)
{
	const std::array<std::filesystem::path, 4> candidates =
	{
		parent_path / (icon_name + ".zip"),
		parent_path / (icon_name + ".7z"),
		parent_path / "icons.zip",
		parent_path / "icons.7z"
	};

	for (const auto& archive_path : candidates)
	{
		if (!std::filesystem::exists(archive_path))
			continue;

		util::archive_file::ptr compressed_archive;
		std::error_condition ec;

		if (archive_path.extension() == ".zip")
			ec = util::archive_file::open_zip(archive_path.string().c_str(), compressed_archive);
		else if (archive_path.extension() == ".7z")
			ec = util::archive_file::open_7z(archive_path.string().c_str(), compressed_archive);

		if (ec || !compressed_archive)
			continue;

		const std::string archived_file = icon_name + ".ico";
		if (compressed_archive->search(archived_file.c_str(), false) < 0)
			continue;

		uint64_t size = compressed_archive->current_uncompressed_length();
		std::unique_ptr<BYTE[]> buffer(new BYTE[size]);

		if (!compressed_archive->decompress(buffer.get(), size))
		{
			HICON hIcon = FormatICOInMemoryToHICON(buffer.get(), size);
			if (hIcon)
				return hIcon;
		}
	}

	return std::nullopt;
}

HICON LoadIconFromFile(const std::string &icon_name)
{
	if (icon_name.empty())
		return nullptr;

	stringtokenizer tokenizer(emu_opts.dir_get_value(DIRPATH_ICONS_PATH), ";");

	for (std::filesystem::path parent_path : tokenizer)
	{
		if (!std::filesystem::exists(parent_path))
			continue;

		// Try .ico file directly
		std::filesystem::path icon_path = parent_path / (icon_name + ".ico");
		if (std::filesystem::exists(icon_path))
		{
			std::string icon_path_str = icon_path.string();
			HICON hIcon = shell::extract_icon_utf8(hInst, icon_path_str.c_str(), 0);
			if (hIcon) return hIcon;
		}

		// Try archive fallback
		auto icon_from_archive = LoadIconFromArchive(parent_path, icon_name);
		if (icon_from_archive)
			return *icon_from_archive;
	}

	return nullptr;
}


// Return the number of folders with options
void SetNumOptionFolders(int count)
{
	optionfolder_count = count;
}


// search
std::string &GetSearchText(void)
{
	return g_SearchText;
}


// Sets the treeview and listviews sizes in accordance with their visibility and the splitters
static void ResizeTreeAndListViews(bool bResizeHidden)
{
	AREA area;
	bool bShowScreenShot = is_flag_set(GetWindowPanes(), window_pane::SCREENSHOT_PANE);
	bool bShowSoftware = is_flag_set(GetWindowPanes(), window_pane::SOFTWARE_PANE);
	bool bShowTree = is_flag_set(GetWindowPanes(), window_pane::TREEVIEW_PANE);
	int nFullWidth, nLastWidth = 0;

	GetWindowArea(&area);
	nFullWidth = area.width;

	// Size the List Control in the Picker
	RECT rect;
	(void)windows::get_client_rect(hMain, &rect);

	// first time, we use the saved values rather than current ones
	if (!m_resized)
	{
	}
	else
	{
	}

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;
	if (bShowToolBar)
		rect.top += topMargin;

	// Tree control
	(void)windows::show_window(dialog_boxes::get_dlg_item(hMain, IDC_TREE), bShowTree ? SW_SHOW : SW_HIDE);

	for (size_t i = 0; g_splitterInfo[i].nSplitterWindow; i++)
	{
		HWND leftwindow_handle = dialog_boxes::get_dlg_item(hMain, g_splitterInfo[i].nLeftWindow);
		LONG_PTR window_style = windows::get_window_long_ptr(leftwindow_handle, GWL_STYLE);
		bool bVisible = (window_style & WS_VISIBLE) != 0 ? true : false;

		if (bResizeHidden || bVisible)
		{
			const int leftwindow_width = nSplitterOffset[i] - SPLITTER_WIDTH / 2 - nLastWidth;
			int x_pos = nLastWidth;
			int y_pos = rect.top + 2;
			int width = leftwindow_width;
			int height = rect.bottom - rect.top - 4;

			// special case for the rightmost pane when the screenshot is gone
			if (!bShowScreenShot && !bShowSoftware && !g_splitterInfo[i + 1].nSplitterWindow)
				width = nFullWidth - nLastWidth;

			//std::cout << "Sizes: nLastWidth " << nLastWidth << ", nFullWidth " << nFullWidth << ", nLastWidth + width " << (nLastWidth + width) << "\n";
			if (nLastWidth > nFullWidth)
				nLastWidth = nFullWidth - MIN_VIEW_WIDTH;

			if ((nLastWidth + width) > nFullWidth)
				width = MIN_VIEW_WIDTH;

			//std::cout << "ResizeTreeAndListViews: Window " << i << ", Left " << nLastWidth << ", Right " << width + nLastWidth << "\n";
			(void)windows::move_window(leftwindow_handle, x_pos, y_pos, width, height, true); // window

			x_pos = nSplitterOffset[i];
			width = SPLITTER_WIDTH;
			(void)windows::move_window(dialog_boxes::get_dlg_item(hMain, g_splitterInfo[i].nSplitterWindow), x_pos, y_pos, width, height, true); // splitter

			if (bVisible)
				nLastWidth += leftwindow_width + SPLITTER_WIDTH;
		}

	}
}

void UpdateSoftware()
{
	// first time through can't do this stuff
	if (hwndList == nullptr)
		return;

	bool bShowSoftware = is_flag_set(GetWindowPanes(), window_pane::SOFTWARE_PANE);
	bool bShowScreenShot = is_flag_set(GetWindowPanes(), window_pane::SCREENSHOT_PANE);
	//int  nWidth;

	// Size the List Control in the Picker
	RECT rect;
	(void)windows::get_client_rect(hMain, &rect);

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;
	if (bShowToolBar)
		rect.top += topMargin;

	(void)menus::check_menu_item(hMainMenu, ID_VIEW_SOFTWARE_AREA, bShowSoftware ? MF_CHECKED : MF_UNCHECKED);
	(void)tool_bar::check_button(s_hToolBar, ID_VIEW_SOFTWARE_AREA, bShowSoftware ? MF_CHECKED : MF_UNCHECKED);

	//#ifdef MESS
		//int nGame = Picker_GetSelectedItem(hwndList);
	if (bShowSoftware) // && DriverHasSoftware(nGame))   // not working correctly, look at it later
	{
		(void)windows::show_window(dialog_boxes::get_dlg_item(hMain, IDC_SWTAB), SW_SHOW);
		SoftwareTabView_OnSelectionChanged();
	}
	else
		//#endif
	{
		(void)windows::show_window(dialog_boxes::get_dlg_item(hMain, IDC_SWLIST), SW_HIDE);
		(void)windows::show_window(dialog_boxes::get_dlg_item(hMain, IDC_SWDEVVIEW), SW_HIDE);
		(void)windows::show_window(dialog_boxes::get_dlg_item(hMain, IDC_SOFTLIST), SW_HIDE);
		(void)windows::show_window(dialog_boxes::get_dlg_item(hMain, IDC_SWTAB), SW_HIDE);
	}
	ResizePickerControls(hMain);

	if (bShowScreenShot)
		UpdateScreenShot();
}

// Adjust the list view and screenshot button based on GetShowScreenShot()
void UpdateScreenShot()
{
	// first time through can't do this stuff
	//std::cout << "Update Screenshot : A" << "\n";
	if (hwndList == nullptr)
		return;

	// Size the List Control in the Picker
	RECT rect;
	(void)windows::get_client_rect(hMain, &rect);

	//std::cout << "Update Screenshot: B" << "\n";
	if (bShowStatusBar)
		rect.bottom -= bottomMargin;
	if (bShowToolBar)
		rect.top += topMargin;

	//std::cout << "Update Screenshot: C" << "\n";
	bool bShowScreenShot = is_flag_set(GetWindowPanes(), window_pane::SCREENSHOT_PANE);
	(void)menus::check_menu_item(hMainMenu, ID_VIEW_PICTURE_AREA, bShowScreenShot ? MF_CHECKED : MF_UNCHECKED);
	(void)tool_bar::check_button(s_hToolBar, ID_VIEW_PICTURE_AREA, bShowScreenShot ? MF_CHECKED : MF_UNCHECKED);

	//std:cout << "Update Screenshot: F" << "\n";
//  ResizePickerControls(hMain);
	ResizeTreeAndListViews(FALSE);

	//std::cout << "Update Screenshot: G" << "\n";
	FreeScreenShot();

	//std::cout << "Update Screenshot: H" << "\n";
	if (have_selection)
	{
		int item_index = Picker_GetSelectedItem(hwndList);
		if (item_index >= 0)
		{
			//#ifdef MESS
			if (!g_szSelectedItem.empty())
				LoadScreenShot(item_index, g_szSelectedItem, TabView_GetCurrentTab(hTabCtrl));
			else
			//#endif
				LoadScreenShot(item_index, std::string{}, TabView_GetCurrentTab(hTabCtrl));
		}
	}

	// figure out if we have a history or not, to place our other windows properly
	//std::cout << "Update Screenshot: I" << "\n";
	UpdateHistory(g_szSelectedItem);

	// setup the picture area

	//std::cout << "Update Screenshot: J" << "\n";
	if (bShowScreenShot)
	{
		DWORD dwStyle;
		DWORD dwStyleEx;
		bool showing_history;

		POINT p = {0, 0};
		(void)gdi::client_to_screen(hMain, &p);
		RECT fRect;
		(void)windows::get_window_rect(dialog_boxes::get_dlg_item(hMain, IDC_SSFRAME), &fRect);
		gdi::offset_rect(&fRect, -p.x, -p.y);

		// show history on this tab IF
		// - we have history for the game
		// - we're on the first tab
		// - we DON'T have a separate history tab
		showing_history = (have_history && (TabView_GetCurrentTab(hTabCtrl) == GetHistoryTab()
			|| GetHistoryTab() == TAB_ALL ) && GetShowTab(TAB_HISTORY) == false);
		CalculateBestScreenShotRect(dialog_boxes::get_dlg_item(hMain, IDC_SSFRAME), &rect,showing_history);

		dwStyle   = windows::get_window_long_ptr(dialog_boxes::get_dlg_item(hMain, IDC_SSPICTURE), GWL_STYLE);
		dwStyleEx = windows::get_window_long_ptr(dialog_boxes::get_dlg_item(hMain, IDC_SSPICTURE), GWL_EXSTYLE);

		windows::adjust_window_rect_ex(&rect, dwStyle, false, dwStyleEx);
		(void)windows::move_window(dialog_boxes::get_dlg_item(hMain, IDC_SSPICTURE), fRect.left + rect.left, fRect.top + rect.top, rect.right - rect.left, rect.bottom - rect.top, true);

		(void)windows::show_window(dialog_boxes::get_dlg_item(hMain,IDC_SSPICTURE), (TabView_GetCurrentTab(hTabCtrl) != TAB_HISTORY) ? SW_SHOW : SW_HIDE);
		(void)windows::show_window(dialog_boxes::get_dlg_item(hMain,IDC_SSFRAME),SW_SHOW);
		(void)windows::show_window(dialog_boxes::get_dlg_item(hMain,IDC_SSTAB),bShowTabCtrl ? SW_SHOW : SW_HIDE);

		gdi::invalidate_rect(dialog_boxes::get_dlg_item(hMain,IDC_SSPICTURE),nullptr,FALSE);
	}
	else
	{
		(void)windows::show_window(dialog_boxes::get_dlg_item(hMain,IDC_SSPICTURE),SW_HIDE);
		(void)windows::show_window(dialog_boxes::get_dlg_item(hMain,IDC_SSFRAME),SW_HIDE);
		(void)windows::show_window(dialog_boxes::get_dlg_item(hMain,IDC_SSTAB),SW_HIDE);
	}
	std::cout << "Update Screenshot: Finished" << "\n";
}


void ResizePickerControls(HWND hWnd)
{
	RECT rect, sRect;
	static bool afirstTime = true;
	bool doSSControls = true;
	int nSplitterCount = GetSplitterCount();

	// Size the List Control in the Picker
	(void)windows::get_client_rect(hWnd, &rect);

	// Calc the display sizes based on g_splitterInfo
	if (afirstTime)
	{
		for (size_t i = 0; i < nSplitterCount; i++)
//          nSplitterOffset[i] = rect.right * g_splitterInfo[i].dPosition;
			nSplitterOffset[i] = GetSplitterPos(i);

		RECT rWindow;
		(void)windows::get_window_rect(hStatusBar, &rWindow);
		bottomMargin = rWindow.bottom - rWindow.top;
		(void)windows::get_window_rect(s_hToolBar, &rWindow);
		topMargin = rWindow.bottom - rWindow.top;
		//buttonMargin = (sRect.bottom + 4);

		afirstTime = false;
	}
	else
	{
		doSSControls = is_flag_set(GetWindowPanes(), window_pane::SCREENSHOT_PANE);
	}

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;

	if (bShowToolBar)
		rect.top += topMargin;

	(void)windows::move_window(dialog_boxes::get_dlg_item(hWnd, IDC_DIVIDER), rect.left, rect.top - 4, rect.right, 2, true);

	ResizeTreeAndListViews(TRUE);
	int nListWidth = 0;
	if (is_flag_set(GetWindowPanes(), window_pane::SOFTWARE_PANE))
		nListWidth = nSplitterOffset[2];
	else
		nListWidth = nSplitterOffset[1];

	int nScreenShotWidth = rect.right - nListWidth - SPLITTER_WIDTH;

	// Screen shot Page tab control
	if (bShowTabCtrl)
	{
		(void)windows::move_window(dialog_boxes::get_dlg_item(hWnd, IDC_SSTAB), nListWidth + 4, rect.top + 2, nScreenShotWidth - 2, rect.top + 20, doSSControls);
		rect.top += 20;
	}

	// resize the Screen shot frame
	(void)windows::move_window(dialog_boxes::get_dlg_item(hWnd, IDC_SSFRAME), nListWidth + 4, rect.top + 2, nScreenShotWidth - 2, rect.bottom - rect.top - 4, doSSControls);

	// The screen shot controls
	RECT frameRect;
	(void)windows::get_client_rect(dialog_boxes::get_dlg_item(hWnd, IDC_SSFRAME), &frameRect);

	// Text control - game history
	sRect.left = nListWidth + 14;
	sRect.right = sRect.left + nScreenShotWidth - 22;

	if (GetShowTab(TAB_HISTORY))
	{
		// We're using the new mode, with the history filling the entire tab (almost)
		sRect.top = rect.top + 14;
		sRect.bottom = rect.bottom - rect.top - 30;
	}
	else
	{
		// We're using the original mode, with the history beneath the SS picture
		sRect.top = rect.top + MAX_SCREENSHOT_HEIGHT;
		sRect.bottom = rect.bottom - rect.top - 278;
	}

	(void)windows::move_window(dialog_boxes::get_dlg_item(hWnd, IDC_HISTORY), sRect.left, sRect.top, sRect.right - sRect.left, sRect.bottom, doSSControls);

	// the other screen shot controls will be properly placed in UpdateScreenshot()
}


HBITMAP GetBackgroundBitmap(void)
{
	return hbBackground;
}


HPALETTE GetBackgroundPalette(void)
{
	return hpBackground;
}


MYBITMAPINFO * GetBackgroundInfo(void)
{
	return &bmDesc;
}

int GetMinimumScreenShotWindowWidth(void)
{
	BITMAP bmp;
	(void)gdi::get_object(hMissing_bitmap,sizeof(BITMAP),&bmp);

	return bmp.bmWidth + 6; // 6 is for a little breathing room
}


int GetParentIndex(const game_driver *driver)
{
	assert(driver);
	const char* parent_name = driver->parent;
	return GetGameNameIndex(parent_name);
}


int GetCompatIndex(const game_driver *driver)
{
	const char *t = driver->compatible_with;
	if (t)
	{
		return GetGameNameIndex(t);
	}
	else
	{
		return -1;
	}
}


int GetParentRomSetIndex(const game_driver *driver)
{
	int nParentIndex = GetGameNameIndex(driver->parent);

	if( nParentIndex >= 0)
	{
		if ((driver_list::driver(nParentIndex).flags & MACHINE_IS_BIOS_ROOT) == 0)
			return nParentIndex;
	}

	return -1;
}


int GetGameNameIndex(const char *name)
{
	return driver_list::find(name);
}


/***************************************************************************
    Internal functions
 ***************************************************************************/
#if 0
static wchar_t *GetMainTitle(void)
{
  int iTextLength = windows::get_window_text_length(hMain);
  wchar_t *strWinTitle = new wchar_t[iTextLength] {'\0'};
  windows::get_window_text(hMain, strWinTitle, iTextLength);
  return strWinTitle;
}
#endif
static void SetMainTitle(void)
{
	std::wstring main_title(MAMEUINAME);

	main_title.append(L" "s + mui_utf16_from_utf8string(GetVersionString()));
	windows::set_window_text(hMain, main_title.c_str());
}


//static void memory_error(const char *message)
//{
//  dialog_boxes::message_box_utf8(hMain, message, emulator_info::get_appname(), MB_OK);
//  exit(-1);
//}


static intptr_t CALLBACK StartupProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			// Need a correctly-sized bitmap
			HBITMAP hBmp = (HBITMAP)menus::load_image(system_services::get_module_handle(nullptr), menus::make_int_resource(IDB_SPLASH), IMAGE_BITMAP, 0, 0, LR_SHARED);
			(void)windows::send_message(dialog_boxes::get_dlg_item(hDlg, IDC_SPLASH), STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmp);
			hBrush = gdi::get_sys_color_brush(COLOR_3DFACE);
			hProgress = windows::create_window_ex(0, PROGRESS_CLASS, nullptr, WS_CHILD | WS_VISIBLE, 0, 136, 526, 18, hDlg, nullptr, hInst, nullptr);
			window::set_window_theme(hProgress, L" ", L" ");
			(void)windows::send_message(hProgress, PBM_SETBKCOLOR, 0, windows::get_sys_color(COLOR_3DFACE));
			//(void)windows::send_message(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
			(void)windows::send_message(hProgress, PBM_SETPOS, 0, 0);
			return true;
		}

		case WM_CTLCOLORDLG:
			return (LRESULT) hBrush;

		case WM_CTLCOLORSTATIC:
			hDC = (HDC)wParam;
			(void)gdi::set_bk_mode(hDC, TRANSPARENT);
			(void)gdi::set_text_color(hDC, windows::get_sys_color(COLOR_HIGHLIGHT));
			return (LRESULT) hBrush;
	}

	return false;
}


static bool MameUI_init(HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hSplash, IDC_PROGBAR), "Please wait...");
	(void)windows::send_message(hProgress, PBM_SETPOS, 10, 0);

	extern const FOLDERDATA g_folderData[];
	extern const FILTER_ITEM g_filterList[];
	m_resized = false;

	std::cout << "MameUI_init: About to init options" << "\n";
	OptionsInit();
	(void)windows::send_message(hProgress, PBM_SETPOS, 25, 0);
	emu_opts.emu_opts_init(false);
	std::cout << "MameUI_init: Options loaded" << "\n";
	(void)windows::send_message(hProgress, PBM_SETPOS, 40, 0);
	//dialog_boxes::message_box_utf8(hMain, "test", emulator_info::get_appname(), MB_OK);

	// custom per-game icons
	icon_index = make_unique_clear<int[]>(driver_list::total());

	// set up window class
	WNDCLASS wndclass;
	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = MameUIWindowProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = DLGWINDOWEXTRA;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = menus::load_icon(hInstance, menus::make_int_resource(IDI_MAMEUI));
	wndclass.hCursor       = nullptr;
	wndclass.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wndclass.lpszMenuName  = menus::make_int_resource(IDR_UI_MENU);
	wndclass.lpszClassName = L"MainClass";

	windows::register_class(&wndclass);

//#ifdef MESS
	MView_RegisterClass(); // messui.cpp
//#endif

	InitCommonControls();
	(void)windows::send_message(hProgress, PBM_SETPOS, 55, 0);
	HelpInit();

	hMain = dialog_boxes::create_dialog(hInstance, menus::make_int_resource(IDD_MAIN), 0, nullptr, 0L);
	if (hMain == nullptr)
	{
		std::cout << "MameUI_init: Error creating main dialog, aborting" << "\n";
		return false;
	}

	hMainMenu = menus::get_menu(hMain);

	s_pWatcher = DirWatcher_Init(hMain, WM_MAMEUI_FILECHANGED);
	if (s_pWatcher)
	{
		DirWatcher_Watch(s_pWatcher, 0, emu_opts.dir_get_value(DIRPATH_MEDIAPATH), true);  // roms
		DirWatcher_Watch(s_pWatcher, 1, emu_opts.dir_get_value(DIRPATH_SAMPLEPATH), true);  // samples
	}

	SetMainTitle();
	hTabCtrl = dialog_boxes::get_dlg_item(hMain, IDC_SSTAB);
	(void)windows::send_message(hProgress, PBM_SETPOS, 70, 0);

	{
		TabViewOptions opts;

		static const TabViewCallbacks s_tabviewCallbacks =
		{
			GetShowTabCtrl,         // pfnGetShowTabCtrl
			SetCurrentTab,          // pfnSetCurrentTab
			GetCurrentTab,          // pfnGetCurrentTab
			SetShowTab,             // pfnSetShowTab
			GetShowTab,             // pfnGetShowTab

			GetImageTabShortName,   // pfnGetTabShortName
			GetImageTabLongName,    // pfnGetTabLongName
			UpdateScreenShot        // pfnOnSelectionChanged
		};

		opts = {};
		opts.pCallbacks = &s_tabviewCallbacks;
		opts.nTabCount = MAX_TAB_TYPES;

		if (!SetupTabView(hTabCtrl, &opts))
			return false;
	}

	// subclass history window
	LONG_PTR l = windows::get_window_long_ptr(dialog_boxes::get_dlg_item(hMain, IDC_HISTORY), GWLP_WNDPROC);
	g_lpHistoryWndProc = (WNDPROC)l;
	(void)windows::set_window_long_ptr(dialog_boxes::get_dlg_item(hMain, IDC_HISTORY), GWLP_WNDPROC, (LONG_PTR)HistoryWndProc);

	// subclass picture frame area
	l = windows::get_window_long_ptr(dialog_boxes::get_dlg_item(hMain, IDC_SSFRAME), GWLP_WNDPROC);
	g_lpPictureFrameWndProc = (WNDPROC)l;
	(void)windows::set_window_long_ptr(dialog_boxes::get_dlg_item(hMain, IDC_SSFRAME), GWLP_WNDPROC, (LONG_PTR)PictureFrameWndProc);

	// subclass picture area
	l = windows::get_window_long_ptr(dialog_boxes::get_dlg_item(hMain, IDC_SSPICTURE), GWLP_WNDPROC);
	g_lpPictureWndProc = (WNDPROC)l;
	(void)windows::set_window_long_ptr(dialog_boxes::get_dlg_item(hMain, IDC_SSPICTURE), GWLP_WNDPROC, (LONG_PTR)PictureWndProc);

	// Load the pic for the default screenshot.
	hMissing_bitmap = gdi::load_bitmap(system_services::get_module_handle(nullptr),menus::make_int_resource(IDB_ABOUT));

	// Stash hInstance for later use
	hInst = hInstance;

	s_hToolBar   = InitToolbar(hMain);
	hStatusBar = InitStatusBar(hMain);
	hProgWnd   = InitProgressBar(hStatusBar);

	main_resize_items[0].u.hwnd = s_hToolBar;
	main_resize_items[1].u.hwnd = hStatusBar;

	// In order to handle 'Large Fonts' as the Windows
	// default setting, we need to make the dialogs small
	// enough to fit in our smallest window size with
	// large fonts, then resize the picker, tab and button
	// controls to fill the window, no matter which font
	// is currently set.  This will still look like bad
	// if the user uses a bigger default font than 125%
	// (Large Fonts) on the Windows display setting tab.
	//
	// NOTE: This has to do with Windows default font size
	// settings, NOT our picker font size.

	RECT rect;
	(void)windows::get_client_rect(hMain, &rect);

	hTreeView = dialog_boxes::get_dlg_item(hMain, IDC_TREE);
	hwndList  = dialog_boxes::get_dlg_item(hMain, IDC_LIST);

	if (!InitSplitters())
		return false;

	int nSplitterCount = GetSplitterCount();
	for (size_t i = 0; i < nSplitterCount; i++)
	{
		HWND hWnd = dialog_boxes::get_dlg_item(hMain, g_splitterInfo[i].nSplitterWindow);
		HWND hWndLeft = dialog_boxes::get_dlg_item(hMain, g_splitterInfo[i].nLeftWindow);
		HWND hWndRight = dialog_boxes::get_dlg_item(hMain, g_splitterInfo[i].nRightWindow);

		AddSplitter(hWnd, hWndLeft, hWndRight, g_splitterInfo[i].pfnAdjust);
	}

	// Initial adjustment of controls on the Picker window
	ResizePickerControls(hMain);

	TabView_UpdateSelection(hTabCtrl);

	bDoGameCheck = GetGameCheck();
	idle_work    = true;
	game_index   = 0;

	bShowTree = is_flag_set(GetWindowPanes(), window_pane::TREEVIEW_PANE);
	bShowToolBar   = GetShowToolBar();
	bShowStatusBar = GetShowStatusBar();
	bShowTabCtrl   = GetShowTabCtrl();

	(void)menus::check_menu_item(hMainMenu, ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
	(void)tool_bar::check_button(s_hToolBar, ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
	(void)menus::check_menu_item(hMainMenu, ID_VIEW_TOOLBARS, (bShowToolBar) ? MF_CHECKED : MF_UNCHECKED);
	(void)windows::show_window(s_hToolBar, (bShowToolBar) ? SW_SHOW : SW_HIDE);
	(void)menus::check_menu_item(hMainMenu, ID_VIEW_STATUS, (bShowStatusBar) ? MF_CHECKED : MF_UNCHECKED);
	(void)windows::show_window(hStatusBar, (bShowStatusBar) ? SW_SHOW : SW_HIDE);
	(void)menus::check_menu_item(hMainMenu, ID_VIEW_PAGETAB, (bShowTabCtrl) ? MF_CHECKED : MF_UNCHECKED);

	LoadBackgroundBitmap();

	(void)windows::send_message(hProgress, PBM_SETPOS, 85, 0);
	std::cout << "MameUI_init: About to InitTreeView" << "\n";
	InitTreeView(g_folderData, g_filterList);
	std::cout << "MameUI_init: Did InitTreeView" << "\n";
	(void)windows::send_message(hProgress, PBM_SETPOS, 100, 0);

	// Initialize listview columns
//#ifdef MESS
	InitMessPicker(); // messui.cpp
//#endif

	std::cout << "MameUI_init: About to InitListView" << "\n";
	InitListView();
	(void)input::set_focus(hwndList);
	std::cout << "MameUI_init: Did InitListView" << "\n";
	// Reset the font
	std::cout << "MameUI_init: Reset the font" << "\n";
	{
		LOGFONT logfont;

		GetListFont(&logfont);
		if (hFont)
		{
			//Cleanup old Font, otherwise we have a GDI handle leak
			(void)gdi::delete_font(hFont);
		}
		hFont = gdi::create_font_indirect(&logfont);
		if (hFont)
			SetAllWindowsFont(hMain, &main_resize, hFont, false);
	}

	// Init DirectInput
	std::cout << "MameUI_init: Init directinput" << "\n";
	if (!DirectInputInitialize())
	{
		(void)dialog_boxes::dialog_box(system_services::get_module_handle(nullptr), menus::make_int_resource(IDD_DIRECTX), nullptr, DirectXDialogProc, 0L);
		return false;
	}

	std::cout << "MameUI_init: Adjusting window metrics" << "\n";
	AdjustMetrics();
	UpdateSoftware();
	UpdateScreenShot();

	hAccel = menus::load_accelerators(hInstance, menus::make_int_resource(IDA_TAB_KEYS));

	// clear keyboard state
	std::cout << "MameUI_init: Clearing keyboard state" << "\n";
	KeyboardStateClear();

	std::cout << "MameUI_init: Init joystick input" << "\n";
	if (GetJoyGUI() == true)
	{
		g_pJoyGUI = std::make_unique<DIJoystickCallbacks>(DIJoystick);
		if (g_pJoyGUI->init() != 0)
			g_pJoyGUI.reset();
		else
			SetTimer(hMain, JOYGUI_TIMER, JOYGUI_MS, nullptr);
	}
	else
		g_pJoyGUI.reset();

	std::cout << "MameUI_init: Centering mouse cursor position" << "\n";
	if (GetHideMouseOnStartup())
	{
		//  For some reason the mouse is centered when a game is exited, which of
		//  course causes a WM_MOUSEMOVE event that shows the mouse. So we center
		//  it now, before the startup coords are initialised, and that way the mouse
		//  will still be hidden when exiting from a game (i hope) :)
		menus::set_cursor_pos(windows::get_system_metrics(SM_CXSCREEN)/2,windows::get_system_metrics(SM_CYSCREEN)/2);

		// Then hide it
		menus::show_cursor(FALSE);
	}

	std::cout << "MameUI_init: About to show window" << "\n";

	nCmdShow = GetWindowState();
	if (nCmdShow == SW_HIDE || nCmdShow == SW_MINIMIZE || nCmdShow == SW_SHOWMINIMIZED)
		nCmdShow = SW_RESTORE;

	if (GetRunFullScreen())
	{
		LONG lMainStyle;

		// Remove menu
		menus::set_menu(hMain,nullptr);

		// Frameless dialog (fake fullscreen)
		lMainStyle = windows::get_window_long_ptr(hMain, GWL_STYLE);
		lMainStyle = lMainStyle & (WS_BORDER ^ 0xffffffff);
		(void)windows::set_window_long_ptr(hMain, GWL_STYLE, lMainStyle);

		nCmdShow = SW_MAXIMIZE;
	}

	(void)windows::show_window(hMain, nCmdShow);


	switch (GetViewMode())
	{
	case VIEW_LARGE_ICONS :
		SetView(ID_VIEW_LARGE_ICON);
		break;
	case VIEW_SMALL_ICONS :
		SetView(ID_VIEW_SMALL_ICON);
		break;
	case VIEW_INLIST :
		SetView(ID_VIEW_LIST_MENU);
		break;
	case VIEW_REPORT :
		SetView(ID_VIEW_DETAIL);
		break;
	case VIEW_GROUPED :
	default :
		SetView(ID_VIEW_GROUPED);
		break;
	}

	if (GetCycleScreenshot() > 0)
	{
		SetTimer(hMain, SCREENSHOT_TIMER, GetCycleScreenshot()*1000, nullptr); //scale to Seconds
	}

//#ifndef MESS
//  (void)windows::send_message(s_hToolBar, TB_ENABLEBUTTON, ID_VIEW_SOFTWARE_AREA, false);
//  (void)menus::enable_menu_item(hMainMenu, ID_VIEW_SOFTWARE_AREA, MF_DISABLED | MF_GRAYED);
//  (void)menus::draw_menu_bar(hMain);
//  (void)windows::show_window(dialog_boxes::get_dlg_item(hMain, IDC_SWLIST), SW_HIDE);
//  (void)windows::show_window(dialog_boxes::get_dlg_item(hMain, IDC_SWDEVVIEW), SW_HIDE);
//  (void)windows::show_window(dialog_boxes::get_dlg_item(hMain, IDC_SOFTLIST), SW_HIDE);
//  (void)windows::show_window(dialog_boxes::get_dlg_item(hMain, IDC_SWTAB), SW_HIDE);
//#endif

	return true;
}


static void MameUI_exit()
{
//#ifdef MESS
	MySoftwareListClose(); // messui.cpp
//#endif

	if (g_pJoyGUI)
		g_pJoyGUI->exit();

	// Free GDI resources
	if (hMain) {
		(void)gdi::delete_object(hMain);
		hMain = nullptr;
	}

	if (hMissing_bitmap)
	{
		(void)gdi::delete_bitmap(hMissing_bitmap);
		hMissing_bitmap = nullptr;
	}

	if (hbBackground)
	{
		(void)gdi::delete_bitmap(hbBackground);
		hbBackground = nullptr;
	}

	if (hpBackground)
	{
		(void)gdi::delete_palette(hpBackground);
		hpBackground = nullptr;
	}

	if (hFont)
	{
		(void)gdi::delete_font(hFont);
		hFont = nullptr;
	}

	DestroyIcons();

	DestroyAcceleratorTable(hAccel);

	DirectInputClose();

	SetSavedFolderID(GetCurrentFolderID());
	SaveGameListOptions();
	mui_save_ini();
	emu_opts.ui_save_ini();

	FreeFolders();

	// DestroyTree(hTreeView);

	FreeScreenShot();

	HelpExit();
}


static LRESULT CALLBACK MameUIWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CTLCOLORSTATIC:
	{
		if (hbBackground && (HWND)lParam == dialog_boxes::get_dlg_item(hMain, IDC_HISTORY))
		{
			HDC hDC = (HDC)wParam;
			(void)gdi::set_bk_mode(hDC, TRANSPARENT);
			(void)gdi::set_text_color(hDC, GetListFontColor());
		}

		return (INT_PTR)hBrush;
	}

	case WM_INITDIALOG:
	{
		// Initialize info for resizing subitems
		if (windows::get_client_rect(hWnd, &main_resize.rect))
			return true;
	}
	break;

	case WM_SETFOCUS:
	{
		(void)input::set_focus(hwndList);
	}
	break;

	case WM_SETTINGCHANGE:
	{
		AdjustMetrics();
	}
	break;

	case WM_SIZE:
	{
		OnSize(hWnd, wParam, LOWORD(lParam), HIWORD(lParam));
	}
	break;

	case WM_MENUSELECT:
	{
		return Statusbar_MenuSelect(hWnd, wParam, lParam);
	}
	break;
	case WM_MAMEUI_PLAYGAME:
	{
		MamePlayGame();
	}
	return true;
	case WM_INITMENUPOPUP:
	{
		UpdateMenu(menus::get_menu(hWnd));
	}
	break;

	case WM_CONTEXTMENU:
	{
		if (HandleTreeContextMenu(hWnd, wParam, lParam) || HandleScreenShotContextMenu(hWnd, wParam, lParam))
			break;

		return windows::def_window_proc(hWnd, message, wParam, lParam);
	}
	case WM_COMMAND:
	{
		return MameUICommand(hWnd, (int)(LOWORD(wParam)), (HWND)(lParam), (UINT)HIWORD(wParam));
	}
	break;
	case WM_GETMINMAXINFO:
	{
		MINMAXINFO* mminfo;

		// Don't let the window get too small; it can break resizing
		mminfo = (MINMAXINFO*)lParam;
		mminfo->ptMinTrackSize.x = MIN_WIDTH;
		mminfo->ptMinTrackSize.y = MIN_HEIGHT;
	}
	break;
	case WM_TIMER:
	{
		switch (wParam)
		{
		case JOYGUI_TIMER:
			PollGUIJoystick();
			break;
		case SCREENSHOT_TIMER:
			TabView_CalculateNextTab(hTabCtrl);
			UpdateScreenShot();
			TabView_UpdateSelection(hTabCtrl);
			break;
		}
	}
	break;
	//return true;

	case WM_CLOSE:
	{
		// save current item
		WINDOWPLACEMENT wndpl;

		wndpl.length = sizeof(WINDOWPLACEMENT);
		(void)windows::get_window_placement(hMain, &wndpl);
		UINT state = wndpl.showCmd;

		// Restore the window before we attempt to save parameters,
		// This fixed the lost window on startup problem, among other problems
		if (state == SW_MINIMIZE || state == SW_SHOWMINIMIZED || state == SW_MAXIMIZE)
		{
			if (wndpl.flags & WPF_RESTORETOMAXIMIZED || state == SW_MAXIMIZE)
				state = SW_MAXIMIZE;
			else
			{
				state = SW_RESTORE;
				(void)windows::show_window(hWnd, SW_RESTORE);
			}
		}
		for (size_t i = 0; i < GetSplitterCount(); i++)
			SetSplitterPos(i, nSplitterOffset[i]);
		SetWindowState(state);

		for (size_t i = 0; i < std::size(s_nPickers); i++)
			(void)Picker_SaveColumnWidths(dialog_boxes::get_dlg_item(hMain, s_nPickers[i]));

		// Save the current gui screen dimensions
		RECT rect;
		AREA area;
		(void)windows::get_window_rect(hWnd, &rect);
		area.x = rect.left;
		area.y = rect.top;
		area.width = rect.right - rect.left;
		area.height = rect.bottom - rect.top;
		SetWindowArea(&area);

		// Save the users current game options and default game
		int item_index = Picker_GetSelectedItem(hwndList);
		SetDefaultGame(item_index);

		// hide window to prevent orphan empty rectangles on the taskbar
		// (void)windows::show_window(hWnd,SW_HIDE);
		(void)windows::destroy_window(hWnd);
	}
	break;

	case WM_DESTROY:
		windows::post_quiet_message(0);
		break;

	case WM_LBUTTONDOWN:
		OnLButtonDown(hWnd, (UINT)wParam, MAKEPOINTS(lParam));
		break;

		//        Check to see if the mouse has been moved by the user since
		//        startup. I'd like this checking to be done only in the
		//        main WinProc (here), but somehow the WM_MOUSEDOWN messages
		//        are eaten up before they reach MameWindowProc. That's why
		//        there is one check for each of the subclassed windows too.
		//
		//        POSSIBLE BUGS:
		//        I've included this check in the subclassed windows, but a
		//        mouse move in either the title bar, the menu, or the
		//        toolbar will not generate a WM_MOUSEOVER message. At least
		//        not one that I know how to pick up. A solution could maybe
		//        be to subclass those too, but that's too much work :)

	case WM_MOUSEMOVE:
	{
		if (MouseHasBeenMoved())
			(void)menus::show_cursor(TRUE);

		if (g_listview_dragging)
			MouseMoveListViewDrag(MAKEPOINTS(lParam));
		else
			// for splitters
			OnMouseMove(hWnd, (UINT)wParam, MAKEPOINTS(lParam));
	}
	break;

	case WM_LBUTTONUP:
	{
		if (g_listview_dragging)
			ButtonUpListViewDrag(MAKEPOINTS(lParam));
		else
			// for splitters
			OnLButtonUp(hWnd, (UINT)wParam, MAKEPOINTS(lParam));
	}
	break;

	case WM_NOTIFY:
		// Where is this message intended to go
	{
		LPNMHDR lpNmHdr = (LPNMHDR)lParam;
		wchar_t szClass[256];

		// Fetch tooltip text
		if (lpNmHdr->code == TTN_NEEDTEXT)
		{
			LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)lParam;
			CopyToolTipText(lpttt);
			return true;
		}

		if (lpNmHdr->hwndFrom == hTreeView)
			return TreeViewNotify(lpNmHdr);

		windows::get_classname(lpNmHdr->hwndFrom, szClass, std::size(szClass));
		if (!mui_wcscmp(szClass, L"SysListView32"))
			return Picker_HandleNotify(lpNmHdr);
		if (!mui_wcscmp(szClass, L"SysTabControl32"))
			return TabView_HandleNotify(lpNmHdr);
	}
	break;

	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT lpDis = (LPDRAWITEMSTRUCT)lParam;
		wchar_t szClass[256];

		windows::get_classname(lpDis->hwndItem, szClass, std::size(szClass));
		if (!mui_wcscmp(szClass, L"SysListView32"))
			Picker_HandleDrawItem(dialog_boxes::get_dlg_item(hMain, lpDis->CtlID), lpDis);
	}
	return true;

	case WM_MEASUREITEM:
	{
		LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;

		// tell the list view that each row (item) should be just taller than our font

		//windows::def_window_proc(hWnd, message, wParam, lParam);
		//std::cout << "Default row height calculation gives " << lpmis->itemHeight << "\n";

		TEXTMETRIC tm;
		HDC hDC = gdi::get_dc(nullptr);
		HFONT hFontOld = (HFONT)gdi::select_object(hDC, hFont);

		(void)gdi::get_text_metrics(hDC, &tm);

		lpmis->itemHeight = tm.tmHeight + tm.tmExternalLeading + 1;
		if (lpmis->itemHeight < 17)
			lpmis->itemHeight = 17;
		//std::cout << "We would do " << (tm.tmHeight + tm.tmExternalLeading + 1) << "\n";
		(void)gdi::select_object(hDC, hFontOld);
		(void)gdi::release_dc(nullptr, hDC);
	}
	return true;

	case WM_MAMEUI_AUDITGAME:
	{
		LVFINDINFOW lvfi;
		int nItemIndex,
			nGameIndex;

		nGameIndex = lParam;

		switch (HIWORD(wParam))
		{
		case 0:
			(void)MameUIVerifyRomSet(nGameIndex, false);
			break;
		case 1:
			(void)MameUIVerifySampleSet(nGameIndex);
			break;
		}

		lvfi = {};
		lvfi.flags = LVFI_PARAM;
		lvfi.lParam = nGameIndex;

		nItemIndex = list_view::find_item(hwndList, -1, &lvfi);
		if (nItemIndex != -1)
			(void)list_view::redraw_items(hwndList, nItemIndex, nItemIndex);
	}
	break;

	case WM_MAMEUI_FILECHANGED:
	{
		int (*pfnGetAuditResults)(int driver_index) = nullptr;
		void (*pfnSetAuditResults)(int driver_index, int audit_results) = nullptr;

		switch (HIWORD(wParam))
		{
		case 0:
			pfnGetAuditResults = GetRomAuditResults;
			pfnSetAuditResults = SetRomAuditResults;
			break;
		case 1:
			pfnGetAuditResults = GetSampleAuditResults;
			pfnSetAuditResults = SetSampleAuditResults;
			break;
		}

		if (pfnGetAuditResults && pfnSetAuditResults)
		{
			const char *pFileName = reinterpret_cast<const char*>(lParam);
			if (!pFileName || !*pFileName)
				break;

			std::string_view svFilename = pFileName;

			size_t last_slash = svFilename.find_last_of("/\\");
			if (last_slash != std::string_view::npos)
				svFilename.remove_prefix(last_slash + 1);

			size_t last_dot = svFilename.find_last_of('.');
			if (last_dot != std::string_view::npos)
				svFilename.remove_suffix(svFilename.size() - last_dot);

			for (int nGameIndex = 0; nGameIndex < driver_list::total(); nGameIndex++)
			{
				int nParentIndex = nGameIndex;
				while (nParentIndex != -1)
				{
					const game_driver& driver = driver_list::driver(nParentIndex);
					if (mui_stricmp(driver.name, svFilename) == 0)
					{
						// If game matches the filename, handle audit results.
						if (pfnGetAuditResults(nGameIndex) != UNKNOWN)
						{
							pfnSetAuditResults(nGameIndex, UNKNOWN);
							(void)windows::post_message(hMain, WM_MAMEUI_AUDITGAME, wParam, nGameIndex);
						}
						break;  // Break after processing the match.
					}
					nParentIndex = GetParentIndex(&driver);  // Get the parent index for the next iteration.
				}
			}
		}
	}
	break;

	default:
		return windows::def_window_proc(hWnd, message, wParam, lParam);
	}

	return false;
}


static int HandleKeyboardGUIMessage(HWND hWnd, UINT message, UINT wParam, LONG lParam)
{
	switch (message)
	{
		case WM_CHAR: // List-View controls use this message for searching the items "as user types"
			//dialog_boxes::message_box(nullptr,L"wm_char message arrived",L"TitleBox",MB_OK);
			return true;

		case WM_KEYDOWN:
			KeyboardKeyDown(0, wParam, lParam);
			return true;

		case WM_KEYUP:
			KeyboardKeyUp(0, wParam, lParam);
			return true;

		case WM_SYSKEYDOWN:
			KeyboardKeyDown(1, wParam, lParam);
			return true;

		case WM_SYSKEYUP:
			KeyboardKeyUp(1, wParam, lParam);
			return true;
	}

	return false; // message not processed
}


static bool PumpMessage()
{
	MSG msg;

	if (!GetMessage(&msg, nullptr, 0, 0))
		return false;

	if (windows::is_window(hMain))
	{
		bool absorbed_key = false;
		if (GetKeyGUI())
			absorbed_key = HandleKeyboardGUIMessage(msg.hwnd, msg.message, msg.wParam, msg.lParam);
		else
			absorbed_key = TranslateAccelerator(hMain, hAccel, &msg);

		if (!absorbed_key)
		{
			if (!IsDialogMessage(hMain, &msg))
			{
				(void)TranslateMessage(&msg);
				(void)DispatchMessageW(&msg);
			}
		}
	}

	return true;
}


static bool FolderCheck(void)
{
	int nGameIndex;
	LVFINDINFOW lvfi;
	int nCount = list_view::get_item_count(hwndList);
	MSG msg;

	for (size_t i = 0; i < nCount; i++)
	{
		LVITEMW lvi{};
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_PARAM;
		(void)list_view::get_item(hwndList, &lvi);
		nGameIndex = lvi.lParam;
		SetRomAuditResults(nGameIndex, UNKNOWN);
	}
	if (nCount > 0)
		ProgressBarShow();
	else
		return false;

	int iStep = 0;
	if (nCount < 100)
		iStep = 100 / nCount;
	else
		iStep = nCount / 100;

	UpdateListView();
	(void)UpdateWindow(hMain);

	bool changed = false;
	for (size_t i = 0; i < nCount; i++)
	{
		int nItemIndex;
		LVITEMW lvi{};

		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_PARAM;
		(void)list_view::get_item(hwndList, &lvi);
		nGameIndex = lvi.lParam;
		if (GetRomAuditResults(nGameIndex) == UNKNOWN)
		{
			MameUIVerifyRomSet(nGameIndex, 0);
			changed = true;
		}

		if (GetSampleAuditResults(nGameIndex) == UNKNOWN)
		{
			MameUIVerifySampleSet(nGameIndex);
			changed = true;
		}

		lvfi.flags = LVFI_PARAM;
		lvfi.lParam = nGameIndex;

		nItemIndex = list_view::find_item(hwndList, -1, &lvfi);
		if (changed && i != -1)
		{
			(void)list_view::redraw_items(hwndList, i, i);
			while (PeekMessageW(&msg, hwndList, 0, 0, PM_REMOVE) != 0)
			{
				(void)TranslateMessage(&msg);
				(void)DispatchMessageW(&msg);
			}
		}
		changed = false;
		if ((nItemIndex % iStep) == 0)
			ProgressBarStepParam(i, nCount);
	}
	ProgressBarHide();
	UpdateStatusBar();

	return true;
}


static bool GameCheck(void)
{
	bool changed = false;
	int nItemIndex = -1;

	if (game_index == 0)
		ProgressBarShow();

	if (game_index >= driver_list::total())
	{
		bDoGameCheck = false;
		ProgressBarHide();
		ResetWhichGamesInFolders();
		ResetListView(); // reset the list after F5
		return changed;
	}

	if (GetRomAuditResults(game_index) == UNKNOWN)
	{
		(void)MameUIVerifyRomSet(game_index, 0);
		changed = true;
	}

	if (GetSampleAuditResults(game_index) == UNKNOWN)
	{
		(void)MameUIVerifySampleSet(game_index);
		changed = true;
	}

	LVFINDINFOW lvfi;
	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = game_index;

	nItemIndex = list_view::find_item(hwndList, -1, &lvfi);
	if (changed && (nItemIndex != -1))
		(void)list_view::redraw_items(hwndList, nItemIndex, nItemIndex);

	if ((game_index % progBarStep) == 0)
		ProgressBarStep();

	game_index++;

	return changed;
}


static bool OnIdle(HWND hWnd)
{
	static bool bFirstTime = true;

	if (bFirstTime)
	{
		bFirstTime = false;
	}
	if (bDoGameCheck)
	{
		GameCheck();
		return idle_work;
	}
	// NPW 17-Jun-2003 - Commenting this out because it is redundant
	// and it causes the game to reset back to the original game after an F5
	// refresh
	//driver_index = GetGameNameIndex(GetDefaultGame());
	//SetSelectedPickItem(driver_index);

	// in case it's not found, get it back
	idle_work = false;
	UpdateStatusBar();
	bFirstTime = true;

// don't need this any more 2014-01-26
//  if (!idle_work)
//      (void)windows::post_message(GetMainWindow(),WM_COMMAND, MAKEWPARAM(ID_VIEW_LINEUPICONS, true),(LPARAM)nullptr);
	return idle_work;
}


static void OnSize(HWND hWnd, UINT nState, int nWidth, int nHeight)
{
	static bool firstTime = true;

	if (nState != SIZE_MAXIMIZED && nState != SIZE_RESTORED)
		return;

	ResizeWindow(hWnd, &main_resize);
	ResizeProgressBar();
	if (firstTime == false)
	{
		OnSizeSplitter(hMain);
		m_resized = true;
	}
	//firstTime = false;
	// Update the splitters structures as appropriate
	RecalcSplitters();
	if (firstTime == false)
		ResizePickerControls(hMain);
	firstTime = false;
	UpdateScreenShot();
	std::cout << "OnSize: Finished" << "\n";
}


static HWND GetResizeItemWindow(HWND hParent, const ResizeItem *ri)
{
	HWND hControl;
	if (ri->type == RA_ID)
		hControl = dialog_boxes::get_dlg_item(hParent, ri->u.id);
	else
		hControl = ri->u.hwnd;
	return hControl;
}


static void SetAllWindowsFont(HWND hParent, const Resize *r, HFONT hTheFont, bool bRedraw)
{
	for (size_t i = 0; r->items[i].type != RA_END; i++)
	{
		HWND hControl = GetResizeItemWindow(hParent, &r->items[i]);
		if (r->items[i].setfont)
			windows::set_window_font(hControl, hTheFont, bRedraw);

		// Take care of subcontrols, if appropriate
		if (r->items[i].subwindow != nullptr)
			SetAllWindowsFont(hControl, (const Resize*)r->items[i].subwindow, hTheFont, bRedraw);
	}
}


static void ResizeWindow(HWND hParent, Resize *r)
{
	if (hParent == nullptr)
		return;

	RECT parent_rect, rect;

	// Calculate change in width and height of parent window
	(void)windows::get_client_rect(hParent, &parent_rect);
	int dy = parent_rect.bottom - r->rect.bottom;
	int dx = parent_rect.right - r->rect.right;
	POINT p = {0, 0};
	(void)gdi::client_to_screen(hParent, &p);

	HWND hControl = 0;
	const ResizeItem *ri;
	int cmkindex = 0;
	while (r->items[cmkindex].type != RA_END)
	{
		ri = &r->items[cmkindex];
		if (ri->type == RA_ID)
			hControl = dialog_boxes::get_dlg_item(hParent, ri->u.id);
		else
			hControl = ri->u.hwnd;

		if (hControl == nullptr)
		{
			cmkindex++;
			continue;
		}

		// Get control's rectangle relative to parent
		(void)windows::get_window_rect(hControl, &rect);
		gdi::offset_rect(&rect, -p.x, -p.y);
		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;

		if (!(ri->action & RA_LEFT))
			rect.left += dx;

		if (!(ri->action & RA_TOP))
			rect.top += dy;

		if (ri->action & RA_RIGHT)
			rect.right += dx;

		if (ri->action & RA_BOTTOM)
			rect.bottom += dy;

		//Sanity Check the child rect
		if (parent_rect.top > rect.top)
			rect.top = parent_rect.top;

		if (parent_rect.left > rect.left)
			rect.left = parent_rect.left;

		if (parent_rect.bottom < rect.bottom)
		{
			rect.bottom = parent_rect.bottom;
			//ensure we have at least a minimal height
			rect.top = rect.bottom - height;
			if (rect.top < parent_rect.top)
				rect.top = parent_rect.top;
		}

		if (parent_rect.right < rect.right)
		{
			rect.right = parent_rect.right;
			//ensure we have at least a minimal width
			rect.left = rect.right - width;
			if (rect.left < parent_rect.left)
				rect.left = parent_rect.left;
		}

		(void)windows::move_window(hControl, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, true);

		// Take care of subcontrols, if appropriate
		if (ri->subwindow)
			ResizeWindow(hControl, (Resize*)ri->subwindow);

		cmkindex++;
	}

	// Record parent window's new location
	r->rect = parent_rect;
}


static void ProgressBarShow()
{
	int widths[2] = {150, -1};

	if (driver_list::total() < 100)
		progBarStep = 100 / driver_list::total();
	else
		progBarStep = (driver_list::total() / 100);

	(void)windows::send_message(hStatusBar, SB_SETPARTS, (WPARAM)2, (LPARAM)(LPINT)widths);
	(void)windows::send_message(hProgWnd, PBM_SETRANGE, 0, (LPARAM)MAKELONG(0, driver_list::total()));
	(void)windows::send_message(hProgWnd, PBM_SETSTEP, (WPARAM)progBarStep, 0);
	(void)windows::send_message(hProgWnd, PBM_SETPOS, 0, 0);

	RECT rect;
	(void)StatusBar_GetItemRect(hStatusBar, 1, &rect);

	(void)windows::move_window(hProgWnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, true);

	bProgressShown = true;
}


static void ProgressBarHide()
{
	if (hProgWnd == nullptr)
		return;

	(void)windows::show_window(hProgWnd, SW_HIDE);

	UpdateStatusBar();

	bProgressShown = false;
}


static void ResizeProgressBar()
{
	if (bProgressShown)
	{
		RECT rect;
		int  widths[2] = {150, -1};

		(void)windows::send_message(hStatusBar, SB_SETPARTS, (WPARAM)2, (LPARAM)(LPINT)widths);
		(void)StatusBar_GetItemRect(hStatusBar, 1, &rect);
		(void)windows::move_window(hProgWnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, true);
	}
	else
	{
		ProgressBarHide();
	}
}


static void ProgressBarStepParam(int iGameIndex, int nGameCount)
{
	SetStatusBarTextF(0, "Game search %d%% complete",((iGameIndex + 1) * 100) / nGameCount);
	if (iGameIndex == 0)
		(void)windows::show_window(hProgWnd, SW_SHOW);

	(void)windows::send_message(hProgWnd, PBM_STEPIT, 0, 0);
}


static void ProgressBarStep()
{
	ProgressBarStepParam(game_index, driver_list::total());
}


static HWND InitProgressBar(HWND hParent)
{
	RECT rect;

	(void)StatusBar_GetItemRect(hStatusBar, 0, &rect);

	rect.left += 150;

	return windows::create_window_ex(WS_EX_STATICEDGE,
			PROGRESS_CLASS,
			L"Progress Bar",
			WS_CHILD | PBS_SMOOTH,
			rect.left,
			rect.top,
			rect.right - rect.left,
			rect.bottom - rect.top,
			hParent,
			nullptr,
			hInst,
			nullptr);
}


static void CopyToolTipText(LPTOOLTIPTEXT lpttt)
{
	int iButton = lpttt->hdr.idFrom;
	bool bConverted = false;
	static const wchar_t* wcs_tooltip_text;

	// Map command ID to string index
	for (size_t i = 0; CommandToString[i] != -1; i++)
	{
		if (CommandToString[i] == iButton)
		{
			iButton = i;
			bConverted = true;
			break;
		}
	}

	if (bConverted)
	{
		// Check for valid parameter
		if (iButton > NUM_TOOLTIPS)
			wcs_tooltip_text = L"Invalid Button Index";
		else
			wcs_tooltip_text = szTbStrings[iButton];
	}
	else if (iButton <= 2)
	{
		//Statusbar
		(void)windows::send_message(lpttt->hdr.hwndFrom, TTM_SETMAXTIPWIDTH, 0, 200);
		if (iButton != 1)
		{
			int sb_textlength = LOWORD(windows::send_message(hStatusBar, SB_GETTEXTLENGTHW, (WPARAM)iButton, (LPARAM)nullptr));
			wcs_tooltip_text = new(std::nothrow) wchar_t[sb_textlength + 1];
			(void)windows::send_message(hStatusBar, SB_GETTEXTW, (WPARAM)iButton, (LPARAM)wcs_tooltip_text);
		}
		else
		{
			//for first pane we get the Status directly, to get the line breaks
			std::string status_info = GameInfoStatus(Picker_GetSelectedItem(hwndList), false);
			const wchar_t *wcs_gameInfo_status = mui_utf16_from_utf8cstring(status_info.c_str());
			if (!wcs_gameInfo_status)
				return;
			if (wcs_tooltip_text)
				delete[] wcs_tooltip_text;
			wcs_tooltip_text = wcs_gameInfo_status;
		}
	}
	else
		wcs_tooltip_text = L"Invalid Button Index";

	lpttt->lpszText = const_cast<wchar_t*>(wcs_tooltip_text);
}


static HWND InitToolbar(HWND hParent)
{
	HWND hToolBar = CreateToolbarEx(hParent,
						WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
						CCS_TOP | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
						1,
						8,
						hInst,
						IDB_TOOLBAR,
						tbb,
						NUM_TOOLBUTTONS,
						16,
						16,
						0,
						0,
						sizeof(TBBUTTON));
	RECT rect;

	// get Edit Control position
	int idx = windows::send_message(hToolBar, TB_BUTTONCOUNT, (WPARAM)0, (LPARAM)0) - 1;
	(void)windows::send_message(hToolBar, TB_GETITEMRECT, (WPARAM)idx, (LPARAM)&rect);
	int iPosX = rect.right + 10;
	int iPosY = rect.top + 1;
	int iHeight = rect.bottom - rect.top - 2;

	// create Edit Control
	windows::create_window_ex_utf8( 0L, "Edit", &SEARCH_PROMPT[0], WS_CHILD | WS_BORDER | WS_VISIBLE | ES_LEFT,
					iPosX, iPosY, 200, iHeight, hToolBar, (HMENU)ID_TOOLBAR_EDIT, hInst, nullptr );

	return hToolBar;
}


static HWND InitStatusBar(HWND hParent)
{
	HMENU hMenu = menus::get_menu(hParent);

	popstr[0].hMenu    = 0;
	popstr[0].uiString = 0;
	popstr[1].hMenu    = hMenu;
	popstr[1].uiString = IDS_UI_FILE;
	popstr[2].hMenu    = GetSubMenu(hMenu, 1);
	popstr[2].uiString = IDS_VIEW_TOOLBAR;
	popstr[3].hMenu    = 0;
	popstr[3].uiString = 0;

	return CreateStatusWindowW(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | CCS_BOTTOM | SBARS_SIZEGRIP | SBT_TOOLTIPS, L"Ready", hParent, 2);
}


static LRESULT Statusbar_MenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	UINT  fuFlags = (UINT)HIWORD(wParam);
	HMENU hMainMenu = nullptr;
	int iMenu = 1;

	// Handle non-system popup menu descriptions.
	if (  (fuFlags & MF_POPUP) && (!(fuFlags & MF_SYSMENU)))
	{
		for (iMenu = 1; iMenu < MAX_MENUS; iMenu++)
		{
			if ((HMENU)lParam == popstr[iMenu].hMenu)
			{
				hMainMenu = (HMENU)lParam;
				break ;
			}
		}
	}

	if (hMainMenu)
	{
		// Display helpful text in status bar
		MenuHelp(WM_MENUSELECT, wParam, lParam, hMainMenu, hInst, hStatusBar, (UINT *)&popstr[iMenu]);
	}
	else
	{
		UINT nZero[4]{ 0 };
		MenuHelp(WM_MENUSELECT, wParam, lParam, nullptr, hInst, hStatusBar, nZero);
	}

	return 0;
}


static void UpdateStatusBar()
{
	if (!hProgWnd || !hStatusBar)
		return;

	LPTREEFOLDER lpFolder = GetCurrentFolder();
	if (!lpFolder)
		return;

//#ifdef MESS
	std::string item_count;
	int items = SoftwareList_GetNumberOfItems();
	if (items > 0)
		item_count = std::to_string(items) + " Titles";

	// Show number of software titles in the status bar
	SetStatusBarText(3, item_count.c_str());
//#endif

	int games_shown = 0;

	int nItemIndex = FindGame(lpFolder, 0);
	while (nItemIndex > -1)
	{
		if (!GameFiltered(nItemIndex, lpFolder->m_dwFlags))
			games_shown++;

		nItemIndex = FindGame(lpFolder, nItemIndex + 1);
	}

	if (games_shown <= 0)
	{
		DisableSelection();
		return;
	}

	// Show the number of systems in the status bar
	SetStatusBarTextF(2, g_szGameCountString, games_shown);

	nItemIndex = Picker_GetSelectedItem(hwndList);
	if (nItemIndex < 0 || nItemIndex > driver_list::total())
	{
		DisableSelection();
		return;
	}

	std::string status_info = GameInfoStatus(nItemIndex, false);
	std::replace(status_info.begin(), status_info.end(), '\n', ' ');
	// Show this game's status to the status bar
	SetStatusBarText(1, status_info.c_str());

	const char *pText = driver_list::driver(nItemIndex).type.fullname();
	if (!pText)
	{
		DisableSelection();
		return;
	}

	// Show this game's full name in the status bar
	SetStatusBarText(0, pText);

	int  widths[4]{};
	SIZE size;

	// ensure that the status bar parts are wide enough to show their text
	HDC hDC = gdi::get_dc(hStatusBar);

	int sb_textlength = LOWORD(windows::send_message(hStatusBar, SB_GETTEXTLENGTHW, (WPARAM)3, (LPARAM)nullptr));
	std::vector<wchar_t> statusbar_text(sb_textlength + 1);

	(void)windows::send_message(hStatusBar, SB_GETTEXTW, (WPARAM)3, (LPARAM)statusbar_text.data());
	(void)gdi::get_text_extent_point_32(hDC, statusbar_text.data(), sb_textlength, &size);
	widths[3] = size.cx;

	sb_textlength = LOWORD(windows::send_message(hStatusBar, SB_GETTEXTLENGTHW, (WPARAM)2, (LPARAM)nullptr));
	statusbar_text.resize(sb_textlength + 1);

	(void)windows::send_message(hStatusBar, SB_GETTEXTW, (WPARAM)2, (LPARAM)statusbar_text.data());
	(void)gdi::get_text_extent_point_32(hDC, statusbar_text.data(), sb_textlength, &size);
	widths[2] = size.cx;

	sb_textlength = LOWORD(windows::send_message(hStatusBar, SB_GETTEXTLENGTHW, (WPARAM)1, (LPARAM)nullptr));
	statusbar_text.resize(sb_textlength + 1);

	(void)windows::send_message(hStatusBar, SB_GETTEXTW, (WPARAM)1, (LPARAM)statusbar_text.data());
	(void)gdi::get_text_extent_point_32(hDC, statusbar_text.data(), sb_textlength, &size);
	widths[1] = size.cx;

	(void)gdi::release_dc(hStatusBar, hDC);

	// reset part count to one inorder to get the entire width of the status bar
	widths[0] = -1;
	(void)windows::send_message(hStatusBar, SB_SETPARTS, (WPARAM)1, (LPARAM)(LPINT)widths);

	RECT rect;
	(void)StatusBar_GetItemRect(hStatusBar, 0, &rect);
	int last_part_width = (widths[1] + widths[2] + widths[3]);
	int total_width = (rect.right - rect.left);
	widths[0] = (total_width - last_part_width);
	widths[1] += widths[0];
	widths[2] += widths[1];
	widths[3] = -1;

	int numParts = 4;
	(void)windows::send_message(hStatusBar, SB_SETPARTS, (WPARAM)numParts, (LPARAM)(LPINT)widths);

	SetStatusBarText(3, item_count.c_str());
	SetStatusBarTextF(2, g_szGameCountString, games_shown);
	SetStatusBarText(1, status_info.c_str());
	SetStatusBarText(0, pText);
}


static void UpdateHistory(std::string software)
{
	//DWORD dwStyle = windows::get_window_long_ptr(dialog_boxes::get_dlg_item(hMain, IDC_HISTORY), GWL_STYLE);
	have_history = false;

	if (GetSelectedPick() >= 0)
	{
		std::string history_text = GetGameHistory(Picker_GetSelectedItem(hwndList), software);

		have_history = (history_text.empty()) ? false : true;
		windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hMain, IDC_HISTORY), history_text.c_str());
	}

	if (have_history && is_flag_set(GetWindowPanes(), window_pane::SCREENSHOT_PANE)
		&& ((TabView_GetCurrentTab(hTabCtrl) == TAB_HISTORY) ||
			(TabView_GetCurrentTab(hTabCtrl) == GetHistoryTab() && GetShowTab(TAB_HISTORY) == false) ||
			(TAB_ALL == GetHistoryTab() && GetShowTab(TAB_HISTORY) == false) ))
	{
		RECT rect;
		(void)Edit_GetRect(dialog_boxes::get_dlg_item(hMain, IDC_HISTORY),&rect);
		int nLines = Edit_GetLineCount(dialog_boxes::get_dlg_item(hMain, IDC_HISTORY) );
		HDC hDC = gdi::get_dc(dialog_boxes::get_dlg_item(hMain, IDC_HISTORY));
		TEXTMETRIC tm;
		(void)gdi::get_text_metrics (hDC, &tm);
		int nLineHeight = tm.tmHeight - tm.tmInternalLeading;
		if( ( (rect.bottom - rect.top) / nLineHeight) < (nLines) ) //more than one Page, so show Scrollbar
			(void)SetScrollRange(dialog_boxes::get_dlg_item(hMain, IDC_HISTORY), SB_VERT, 0, nLines, true);
		else //hide Scrollbar
			(void)SetScrollRange(dialog_boxes::get_dlg_item(hMain, IDC_HISTORY),SB_VERT, 0, 0, true);

		(void)windows::show_window(dialog_boxes::get_dlg_item(hMain, IDC_HISTORY), SW_SHOW);
	}
	else
		(void)windows::show_window(dialog_boxes::get_dlg_item(hMain, IDC_HISTORY), SW_HIDE);
}


static void DisableSelection()
{
	MENUITEMINFOW mmi;
	bool prev_have_selection = have_selection;

	mmi.cbSize = sizeof(mmi);
	mmi.fMask = MIIM_TYPE;
	mmi.fType = MFT_STRING;
	mmi.dwTypeData = (wchar_t *) L"&Play";
	mmi.cch = mui_wcslen(mmi.dwTypeData);
	(void)menus::set_menu_item_info(hMainMenu, ID_FILE_PLAY, false, &mmi);

	(void)menus::enable_menu_item(hMainMenu, ID_FILE_PLAY, MF_GRAYED);
	(void)menus::enable_menu_item(hMainMenu, ID_FILE_PLAY_RECORD, MF_GRAYED);
	(void)menus::enable_menu_item(hMainMenu, ID_GAME_PROPERTIES, MF_GRAYED);
	(void)menus::enable_menu_item(hMainMenu, ID_MESS_OPEN_SOFTWARE, MF_GRAYED);

	SetStatusBarText(0, "No Selection");
	SetStatusBarText(1, "");
	SetStatusBarText(3, "");

	have_selection = false;

	if (prev_have_selection != have_selection)
		UpdateScreenShot();
}


static void EnableSelection(int nGame)
{
	const char* enable_selection = "EnableSelection:";

//#ifdef MESS
	std::cout << enable_selection << " A" << "\n";
	bool has_software = MyFillSoftwareList(nGame, false); // messui.cpp
//#endif

	std::cout << enable_selection << " B" << "\n";
	std::string description = ConvertAmpersandString(driver_list::driver(nGame).type.fullname());
	std::unique_ptr<wchar_t[]> wcs_description(mui_utf16_from_utf8cstring(description.c_str()));
	if( !wcs_description )
		return;

	std::cout << enable_selection << " C" << "\n";
	std::wstring play_game_string = std::wstring(g_szPlayGameString) + wcs_description.get();
	MENUITEMINFOW mmi;
	mmi.cbSize = sizeof(mmi);
	mmi.fMask = MIIM_TYPE;
	mmi.fType = MFT_STRING;
	mmi.dwTypeData = &play_game_string[0];
	mmi.cch = mui_wcslen(mmi.dwTypeData);
	(void)menus::set_menu_item_info(hMainMenu, ID_FILE_PLAY, false, &mmi);
	std::cout << enable_selection << " D" << "\n";

	UpdateStatusBar();

	// If doing updating game status

	std::cout << enable_selection << " F" << "\n";
	(void)menus::enable_menu_item(hMainMenu, ID_FILE_PLAY, MF_ENABLED);
	(void)menus::enable_menu_item(hMainMenu, ID_FILE_PLAY_RECORD, MF_ENABLED);

//#ifdef MESS
	if (has_software)
		(void)menus::enable_menu_item(hMainMenu, ID_MESS_OPEN_SOFTWARE, MF_ENABLED);
	else
//#endif
		(void)menus::enable_menu_item(hMainMenu, ID_MESS_OPEN_SOFTWARE, MF_GRAYED);

	(void)menus::enable_menu_item(hMainMenu, ID_GAME_PROPERTIES, MF_ENABLED);

	std::cout << enable_selection << " G" << "\n";
	if (bProgressShown && bListReady == true)
		SetDefaultGame(nGame);

	have_selection = true;

	std::cout << enable_selection << " H" << "\n";
	UpdateScreenShot();
	//UpdateSoftware();   // to fix later

	std::cout << enable_selection << " Finished" << "\n";
}


static void PaintBackgroundImage(HWND hWnd, HRGN hRgn, int x, int y)
{
	RECT rcClient;
	HDC hDC = gdi::get_dc(hWnd);

	// x and y are offsets within the background image that should be at 0,0 in hWnd

	// So we don't paint over the control's border
	(void)windows::get_client_rect(hWnd, &rcClient);

	HDC htempDC = gdi::create_compatible_dc(hDC);
	HBITMAP oldBitmap = (HBITMAP)gdi::select_object(htempDC, hbBackground);

	if (hRgn == nullptr)
	{
		// create a region of our client area
		HRGN rgnBitmap = gdi::create_rect_rgn_indirect(&rcClient);
		(void)gdi::select_clip_rgn(hDC, rgnBitmap);
		(void)gdi::delete_bitmap((HBITMAP)rgnBitmap);
	}
	else
	{
		// use the passed in region
		(void)gdi::select_clip_rgn(hDC, hRgn);
	}

	HPALETTE hPAL = GetBackgroundPalette();
	if (hPAL == nullptr)
		hPAL = gdi::create_half_tone_palette(hDC);

	if (gdi::get_device_caps(htempDC, RASTERCAPS) & RC_PALETTE && hPAL != nullptr)
	{
		(void)gdi::select_palette(htempDC, hPAL, false);
		(void)gdi::realize_palette(htempDC);
	}

	for (size_t i = rcClient.left-x; i < rcClient.right; i += bmDesc.bmWidth)
		for (size_t j = rcClient.top-y; j < rcClient.bottom; j += bmDesc.bmHeight)
			(void)gdi::bit_blt(hDC, i, j, bmDesc.bmWidth, bmDesc.bmHeight, htempDC, 0, 0, SRCCOPY);

	(void)gdi::select_object(htempDC, oldBitmap);
	(void)gdi::delete_dc(htempDC);

	if (GetBackgroundPalette() == nullptr)
	{
		(void)gdi::delete_palette(hPAL);
		hPAL = nullptr;
	}

	(void)gdi::release_dc(hWnd, hDC);
}


static const char* GetCloneParentName(int item_index)
{
	const char* fullname = "";
	int nParentIndex = -1;

	if (DriverIsClone(item_index) == true)
	{
		nParentIndex = GetParentIndex(&driver_list::driver(item_index));
		if (nParentIndex >= 0)
		{
			fullname = driver_list::driver(nParentIndex).type.fullname();
		}
	}

	return fullname;
}


static bool TreeViewNotify(LPNMHDR nm)
{
	switch (nm->code)
	{
		case TVN_SELCHANGED :
		{
			HTREEITEM hti = tree_view::get_selection(hTreeView);
			TVITEM tvi;

			tvi.mask  = TVIF_PARAM | TVIF_HANDLE;
			tvi.hItem = hti;

			if (tree_view::get_item(hTreeView, &tvi))
			{
				SetCurrentFolder((LPTREEFOLDER)tvi.lParam);
				if (bListReady)
				{
					ResetListView();

//#ifdef MESS
					MessUpdateSoftwareList(); // messui.cpp
//#endif
					UpdateScreenShot();
				}
			}
			return true;
		}
		case TVN_BEGINLABELEDIT :
		{
			TV_DISPINFO *ptvdi = (TV_DISPINFO *)nm;
			LPTREEFOLDER folder = (LPTREEFOLDER)ptvdi->item.lParam;

			if (folder->m_dwFlags & F_CUSTOM)
			{
				// user can edit custom folder names
				g_in_treeview_edit = true;
				return false;
			}
			// user can't edit built in folder names
			return true;
		}
		case TVN_ENDLABELEDIT :
		{
			TV_DISPINFO* ptvdi = (TV_DISPINFO*)nm;
			LPTREEFOLDER folder = (LPTREEFOLDER)ptvdi->item.lParam;
			bool result = 0;
			g_in_treeview_edit = false;

			if (!ptvdi->item.pszText)
				return false;

			std::string utf8_szText = mui_utf8_from_utf16string(ptvdi->item.pszText);
			if (utf8_szText.empty())
				return false;

			result = TryRenameCustomFolder(folder, utf8_szText.c_str());
			return result;
		}
	}
	return false;
}


static void GamePicker_OnHeaderContextMenu(POINT pt, int nColumn)
{
	// Right button was clicked on header

	HMENU hMenuLoad = menus::load_menu(hInst, menus::make_int_resource(IDR_CONTEXT_HEADER));
	HMENU hMenu = GetSubMenu(hMenuLoad, 0);
	lastColumnClick = nColumn;
	(void)menus::track_popup_menu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hMain,nullptr);

	(void)DestroyMenu(hMenuLoad);
}


static bool GUI_seq_pressed(const input_seq *seq)
{
	int codenum = 0;
	bool res = true;
	bool invert = false;
	int count = 0;

	for (codenum = 0; (*seq)[codenum] != input_seq::end_code; codenum++)
	{
		input_code code = (*seq)[codenum];

		if (code == input_seq::not_code)
			invert = !invert;

		else if (code == input_seq::or_code)
		{
			if (res && count)
				return 1;
			res = true;
			count = 0;
		}
		else
		{
			if (res)
			{
				if ((keyboard_state[(int)(code.item_id())] != 0) == invert)
					res = false;
			}
			invert = false;
			++count;
		}
	}
	return res && count;
}


static void check_for_GUI_action(void)
{
	for (size_t i = 0; i < NUM_GUI_SEQUENCES; i++)
	{
		const input_seq *is = &(GUISequenceControl[i].is);

		if (GUI_seq_pressed(is))
		{
			std::cout << "seq =" << GUISequenceControl[i].name << "pressed" << "\n";
			switch (GUISequenceControl[i].func_id)
			{
			case ID_GAME_AUDIT:
			case ID_GAME_PROPERTIES:
			case ID_CONTEXT_FILTERS:
			case ID_UI_START:
				KeyboardStateClear(); // because we won't receive KeyUp mesage when we lose focus
				break;
			default:
				break;
			}
			(void)windows::send_message(hMain, WM_COMMAND, GUISequenceControl[i].func_id, 0);
		}
	}
}


static void KeyboardStateClear(void)
{
	std::fill_n(keyboard_state, std::size(keyboard_state), 0);
	std::cout << "keyboard gui state cleared." << "\n";
}


static void KeyboardKeyDown(int syskey, int vk_code, int special)
{
	bool found = false;
	int icode = 0;
	int special_code = (special >> 24) & 1;
	int scancode = (special>>16) & 0xff;

	if ((vk_code==VK_MENU) || (vk_code==VK_CONTROL) || (vk_code==VK_SHIFT))
	{
		found = true;

		// a hack for right shift - it's better to use Direct X for keyboard input it seems......
		if (vk_code==VK_SHIFT)
			if (scancode>0x30) // on my keyboard left shift scancode is 0x2a, right shift is 0x36
				special_code = 1;

		if (special_code) // right hand keys
		{
			switch(vk_code)
			{
			case VK_MENU:
				icode = (int)(KEYCODE_RALT.item_id());
				break;
			case VK_CONTROL:
				icode = (int)(KEYCODE_RCONTROL.item_id());
				break;
			case VK_SHIFT:
				icode = (int)(KEYCODE_RSHIFT.item_id());
				break;
			}
		}
		else // left hand keys
		{
			switch(vk_code)
			{
			case VK_MENU:
				icode = (int)(KEYCODE_LALT.item_id());
				break;
			case VK_CONTROL:
				icode = (int)(KEYCODE_LCONTROL.item_id());
				break;
			case VK_SHIFT:
				icode = (int)(KEYCODE_LSHIFT.item_id());
				break;
			}
		}
	}
	else
	{
		for (size_t i = 0; i < std::size(win_key_trans_table); i++)
		{
			if ( vk_code == win_key_trans_table[i][VIRTUAL_KEY])
			{
				icode = win_key_trans_table[i][MAME_KEY];
				found = true;
				break;
			}
		}
	}
	if (!found)
	{
		std::cout << "vk_code pressed not found = " << vk_code << "\n";
		//dialog_boxes::message_box(nullptr,L"keydown message arrived not processed",L"TitleBox",MB_OK);
		return;
	}

	// save cout format flags
	std::ios_base::fmtflags saved_flags(std::cout.flags());

	std::cout << "vk_code pressed found = " << vk_code
		<< ", syskey =" << syskey << ", mame_keycode =" << icode
		<< ", special = " << std::hex << std::setw(8) << std::setfill('0') << special << "\n";

	std::cout.flags(saved_flags); // restore cout format flags

	keyboard_state[icode] = true;
	check_for_GUI_action();
}


static void KeyboardKeyUp(int syskey, int vk_code, int special)
{
	bool found = false;
	int icode = 0;
	int special_code = (special >> 24) & 1;
	int scancode = (special>>16) & 0xff;

	if ((vk_code==VK_MENU) || (vk_code==VK_CONTROL) || (vk_code==VK_SHIFT))
	{
		found = true;

		// a hack for right shift - it's better to use Direct X for keyboard input it seems......
		if (vk_code==VK_SHIFT)
			if (scancode>0x30) // on my keyboard left shift scancode is 0x2a, right shift is 0x36
				special_code = 1;

		if (special_code) // right hand keys
		{
			switch(vk_code)
			{
			case VK_MENU:
				icode = (int)(KEYCODE_RALT.item_id());
				break;
			case VK_CONTROL:
				icode = (int)(KEYCODE_RCONTROL.item_id());
				break;
			case VK_SHIFT:
				icode = (int)(KEYCODE_RSHIFT.item_id());
				break;
			}
		}
		else // left hand keys
		{
			switch(vk_code)
			{
			case VK_MENU:
				icode = (int)(KEYCODE_LALT.item_id());
				break;
			case VK_CONTROL:
				icode = (int)(KEYCODE_LCONTROL.item_id());
				break;
			case VK_SHIFT:
				icode = (int)(KEYCODE_LSHIFT.item_id());
				break;
			}
		}
	}
	else
	{
		for (size_t i = 0; i < std::size(win_key_trans_table); i++)
		{
			if (vk_code == win_key_trans_table[i][VIRTUAL_KEY])
			{
				icode = win_key_trans_table[i][MAME_KEY];
				found = true;
				break;
			}
		}
	}
	if (!found)
	{
		std::cout << "vk_code released not found = " << vk_code << "\n";
		//dialog_boxes::message_box(nullptr,L"keyup message arrived not processed",L"TitleBox",MB_OK);
		return;
	}
	keyboard_state[icode] = false;

	std::ios_base::fmtflags saved_flags(std::cout.flags());

	std::cout << "vk_code pressed found = " << vk_code
		<< ", syskey =" << syskey << ", mame_keycode =" << icode
		<< ", special = " << std::hex << std::setw(8) << std::setfill('0') << special << "\n";

	std::cout.flags(saved_flags); // restore cout format flags

	check_for_GUI_action();
}


static void PollGUIJoystick()
{
	if (in_emulation)
		return;

	if (g_pJoyGUI == nullptr)
		return;

	g_pJoyGUI->poll_joysticks();

	// User pressed UP
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyUp(0), GetUIJoyUp(1), GetUIJoyUp(2), GetUIJoyUp(3))))
		(void)windows::send_message(hMain, WM_COMMAND, ID_UI_UP, 0);

	// User pressed DOWN
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyDown(0), GetUIJoyDown(1), GetUIJoyDown(2), GetUIJoyDown(3))))
		(void)windows::send_message(hMain, WM_COMMAND, ID_UI_DOWN, 0);

	// User pressed LEFT
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyLeft(0), GetUIJoyLeft(1), GetUIJoyLeft(2), GetUIJoyLeft(3))))
		(void)windows::send_message(hMain, WM_COMMAND, ID_UI_LEFT, 0);

	// User pressed RIGHT
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyRight(0), GetUIJoyRight(1), GetUIJoyRight(2), GetUIJoyRight(3))))
		(void)windows::send_message(hMain, WM_COMMAND, ID_UI_RIGHT, 0);

	// User pressed START GAME
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyStart(0), GetUIJoyStart(1), GetUIJoyStart(2), GetUIJoyStart(3))))
		(void)windows::send_message(hMain, WM_COMMAND, ID_UI_START, 0);

	// User pressed PAGE UP
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyPageUp(0), GetUIJoyPageUp(1), GetUIJoyPageUp(2), GetUIJoyPageUp(3))))
		(void)windows::send_message(hMain, WM_COMMAND, ID_UI_PGUP, 0);

	// User pressed PAGE DOWN
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyPageDown(0), GetUIJoyPageDown(1), GetUIJoyPageDown(2), GetUIJoyPageDown(3))))
		(void)windows::send_message(hMain, WM_COMMAND, ID_UI_PGDOWN, 0);

	// User pressed HOME
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyHome(0), GetUIJoyHome(1), GetUIJoyHome(2), GetUIJoyHome(3))))
		(void)windows::send_message(hMain, WM_COMMAND, ID_UI_HOME, 0);

	// User pressed END
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyEnd(0), GetUIJoyEnd(1), GetUIJoyEnd(2), GetUIJoyEnd(3))))
		(void)windows::send_message(hMain, WM_COMMAND, ID_UI_END, 0);

	// User pressed CHANGE SCREENSHOT
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoySSChange(0), GetUIJoySSChange(1), GetUIJoySSChange(2), GetUIJoySSChange(3))))
		(void)windows::send_message(hMain, WM_COMMAND, IDC_SSFRAME, 0);

	// User pressed SCROLL HISTORY UP
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyHistoryUp(0), GetUIJoyHistoryUp(1), GetUIJoyHistoryUp(2), GetUIJoyHistoryUp(3))))
		(void)windows::send_message(hMain, WM_COMMAND, ID_UI_HISTORY_UP, 0);

	// User pressed SCROLL HISTORY DOWN
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyHistoryDown(0), GetUIJoyHistoryDown(1), GetUIJoyHistoryDown(2), GetUIJoyHistoryDown(3))))
		(void)windows::send_message(hMain, WM_COMMAND, ID_UI_HISTORY_DOWN, 0);

	// For the exec timer, will keep track of how long the button has been pressed
	static int exec_counter = 0;

	// User pressed EXECUTE COMMANDLINE
	// Note: this option is not documented, nor supported in the GUI.
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyExec(0), GetUIJoyExec(1), GetUIJoyExec(2), GetUIJoyExec(3))))
	{
		// validate
		int execwait = GetExecWait();
		if (execwait < 1)
			return;
		if (++exec_counter >= execwait) // Button has been pressed > exec timeout
		{
			// validate
			std::string exec_command = GetExecCommand();
			if (exec_command.empty())
				return;

			// Reset counter
			exec_counter = 0;

			STARTUPINFO si;
			si = {};
			si.dwFlags = STARTF_FORCEONFEEDBACK;
			si.cb = sizeof(si);

			PROCESS_INFORMATION pi;
			pi = {};
			std::unique_ptr<wchar_t[]> wcs_exec_command(mui_utf16_from_utf8cstring(&exec_command[0]));
			if (!wcs_exec_command)
				return;

			(void)processes_threads::create_process(nullptr, const_cast<wchar_t*>(wcs_exec_command.get()), nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi);
			// We will not wait for the process to finish cause it might be a background task
			// The process won't get closed when MAME32 closes either.

			// But close the handles cause we won't need them anymore. Will not close process.
			(void)system_services::close_handle(pi.hProcess);
			(void)system_services::close_handle(pi.hThread);
		}
	}
	else
	{
		// Button has been released within the timeout period, reset the counter
		exec_counter = 0;
	}
}


static void SetView(int menu_id)
{
	bool force_reset = false;

	// first uncheck previous menu item, check new one
	(void)menus::check_menu_radio_item(hMainMenu, ID_VIEW_LARGE_ICON, ID_VIEW_GROUPED, menu_id, MF_CHECKED);
	(void)tool_bar::check_button(s_hToolBar, menu_id, MF_CHECKED);

	if (Picker_GetViewID(hwndList) == VIEW_GROUPED || menu_id == ID_VIEW_GROUPED)
	{
		// this changes the sort order, so redo everything
		force_reset = true;
	}

	for (size_t i = 0; i < std::size(s_nPickers); i++)
		Picker_SetViewID(dialog_boxes::get_dlg_item(hMain, s_nPickers[i]), menu_id - ID_VIEW_LARGE_ICON);

	if (force_reset)
	{
		for (size_t i = 0; i < std::size(s_nPickers); i++)
			Picker_Sort(dialog_boxes::get_dlg_item(hMain, s_nPickers[i]));
	}
}


static void ResetListView()
{
	LPTREEFOLDER lpFolder = GetCurrentFolder();
	if (!lpFolder)
		return;

	int nItemIndex = -1;
	LVITEMW lvi{};
	bool no_selection = false;

	// If the last folder was empty, no_selection is true
	if (have_selection == false)
		no_selection = true;

	int nCurrentGame = Picker_GetSelectedItem(hwndList);
	if (nCurrentGame < 0)
		nCurrentGame = 0;

	SetWindowRedraw(hwndList,FALSE);
	(void)list_view::delete_all_items(hwndList);

	// hint to have it allocate it all at once
	(void)list_view::set_item_count(hwndList,driver_list::total());

	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.stateMask = 0;

	do
	{
		// Add the games that are in this folder
		nItemIndex = FindGame(lpFolder, nItemIndex + 1);
			if (nItemIndex != -1)
			{
				if (GameFiltered(nItemIndex, lpFolder->m_dwFlags))
					continue;

				lvi.iItem = nItemIndex;
				lvi.iSubItem = 0;
				lvi.lParam = nItemIndex;
				lvi.pszText = LPSTR_TEXTCALLBACK;
				lvi.iImage = I_IMAGECALLBACK;
				(void)list_view::insert_item(hwndList, &lvi);
			}
	} while (nItemIndex != -1);

	Picker_Sort(hwndList);

	if (bListReady)
	{
		// If last folder was empty, select the first item in this folder
		if (no_selection)
			nCurrentGame = 0;

		Picker_SetSelectedItem(hwndList, nCurrentGame);
	}

	/*RS Instead of the Arrange Call that was here previously on all Views
	     We now need to set the ViewMode for SmallIcon again,
	     for an explanation why, see SetView*/
	if (GetViewMode() == VIEW_SMALL_ICONS)
		SetView(ID_VIEW_SMALL_ICON);

	SetWindowRedraw(hwndList, true);

	UpdateStatusBar();
}


static void UpdateGameList(bool bUpdateRomAudit, bool bUpdateSampleAudit)
{
	for (size_t i = 0; i < driver_list::total(); i++)
	{
		if (bUpdateRomAudit && DriverUsesRoms(i))
			SetRomAuditResults(i, UNKNOWN);
		if (bUpdateSampleAudit && DriverUsesSamples(i))
			SetSampleAuditResults(i, UNKNOWN);
	}

	idle_work = true;
	bDoGameCheck = true;
	game_index = 0;

	ReloadIcons();

	// Let REFRESH also load new background if found
	LoadBackgroundBitmap();
	gdi::invalidate_rect(hMain,nullptr,TRUE);
	Picker_ResetIdle(hwndList);
}

static void UpdateCache()
{
	int current_id = GetCurrentFolderID(); // remember selected folder
	SetWindowRedraw(hwndList, false);   // stop screen updating
	ForceRebuild();          // tell system that cache needs redoing
	OptionsInit();      // reload options and fix game cache
	emu_opts.emu_opts_init(true);
	//extern const FOLDERDATA g_folderData[];
	//extern const FILTER_ITEM g_filterList[];
	//InitTree(g_folderData, g_filterList);         // redo folders... This crashes, leave out for now
	ResetTreeViewFolders();                      // something with folders
	SelectTreeViewFolder(current_id);            // select previous folder
	SetWindowRedraw(hwndList, true);             // refresh screen
}

static UINT_PTR CALLBACK CFHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
// handle to dialog box, message identifier, message parameter, message parameter
{
	int iIndex = 0;
	COLORREF cCombo=0, cList=0;
	switch (uiMsg)
	{
		case WM_INITDIALOG:
			(void)dialog_boxes::send_dlg_item_message(hdlg, cmb4, CB_ADDSTRING, 0, (LPARAM)L"Custom");
			iIndex = dialog_boxes::send_dlg_item_message(hdlg, cmb4, CB_GETCOUNT, 0, 0);
			cList = GetListFontColor();
			(void)dialog_boxes::send_dlg_item_message(hdlg, cmb4, CB_SETITEMDATA,(WPARAM)iIndex-1,(LPARAM)cList );
			for(size_t i = 0; i< iIndex; i++)
			{
				cCombo = dialog_boxes::send_dlg_item_message(hdlg, cmb4, CB_GETITEMDATA,(WPARAM)i,0 );
				if( cList == cCombo)
				{
					(void)dialog_boxes::send_dlg_item_message(hdlg, cmb4, CB_SETCURSEL,(WPARAM)i,0 );
					break;
				}
			}
			break;
		case WM_COMMAND:
			if( LOWORD(wParam) == cmb4)
			{
				switch (HIWORD(wParam))
				{
					case CBN_SELCHANGE:  // The color ComboBox changed selection
						iIndex = (int)dialog_boxes::send_dlg_item_message(hdlg, cmb4, CB_GETCURSEL, 0, 0L);
						if( iIndex == dialog_boxes::send_dlg_item_message(hdlg, cmb4, CB_GETCOUNT, 0, 0)-1)
						{
							//Custom color selected
							cList = GetListFontColor();
							PickColor(&cList);
							(void)dialog_boxes::send_dlg_item_message(hdlg, cmb4, CB_DELETESTRING, iIndex, 0);
							(void)dialog_boxes::send_dlg_item_message(hdlg, cmb4, CB_ADDSTRING, 0, (LPARAM)L"Custom");
							(void)dialog_boxes::send_dlg_item_message(hdlg, cmb4, CB_SETITEMDATA,(WPARAM)iIndex,(LPARAM)cList);
							(void)dialog_boxes::send_dlg_item_message(hdlg, cmb4, CB_SETCURSEL,(WPARAM)iIndex,0 );
							return true;
						}
				}
			}
			break;
	}
	return false;
}


static void PickFont(void)
{
	LOGFONT font;
	CHOOSEFONT cf;

	GetListFont(&font);
	font.lfQuality = DEFAULT_QUALITY;

	cf.lStructSize = sizeof(CHOOSEFONT);
	cf.hwndOwner   = hMain;
	cf.lpLogFont   = &font;
	cf.lpfnHook = &CFHookProc;
	cf.rgbColors   = GetListFontColor();
	cf.Flags  = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_EFFECTS | CF_ENABLEHOOK;
	if (!dialog_boxes::choose_font(&cf))
		return;

	SetListFont(&font);
	if (hFont != nullptr)
		(void)gdi::delete_font(hFont);

	hFont = gdi::create_font_indirect(&font);
	if (hFont != nullptr)
	{
		COLORREF textColor = cf.rgbColors;
		if (textColor == RGB(255,255,255))
			textColor = RGB(240, 240, 240);

		SetAllWindowsFont(hMain, &main_resize, hFont, true);

		HWND hWnd = windows::get_window(hMain, GW_CHILD);
		while(hWnd)
		{
			wchar_t szClass[265];

			if (windows::get_classname(hWnd, szClass, std::size(szClass)))
			{
				if (!mui_wcscmp(szClass, L"SysListView32"))
					(void)list_view::set_text_color(hWnd, textColor);
				else if (!mui_wcscmp(szClass, L"SysTreeView32"))
					(void)tree_view::set_text_color(hTreeView, textColor);
			}
			hWnd = windows::get_window(hWnd, GW_HWNDNEXT);
		}
		SetListFontColor(cf.rgbColors);
		ResetListView();
	}
}


static void PickColor(COLORREF *cDefault)
{
	CHOOSECOLORW cc;
	COLORREF choice_colors[16];

	for (size_t i=0;i<16;i++)
		choice_colors[i] = GetCustomColor(i);

	cc.lStructSize = sizeof(CHOOSECOLORW);
	cc.hwndOwner   = hMain;
	cc.rgbResult   = *cDefault;
	cc.lpCustColors = choice_colors;
	cc.Flags       = CC_ANYCOLOR | CC_RGBINIT | CC_SOLIDCOLOR;
	if (!dialog_boxes::choose_color(&cc))
		return;

	for (size_t i=0;i<16;i++)
		SetCustomColor(i,choice_colors[i]);

	*cDefault = cc.rgbResult;
}


static void PickCloneColor(void)
{
	COLORREF cClonecolor;
	cClonecolor = GetListCloneColor();
	PickColor( &cClonecolor);
	SetListCloneColor(cClonecolor);
	gdi::invalidate_rect(hwndList,nullptr,FALSE);
}


static bool MameUICommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify)
{

	bool bResult;
	LPTREEFOLDER folder;
	//char* utf8_szFile;
	int nCurrentGame = Picker_GetSelectedItem(hwndList);

	switch (id)
	{
	case ID_FILE_PLAY:
		MamePlayGame();
		return true;

	case ID_FILE_PLAY_RECORD:
		MameUI_RecordInput();
		return true;

	case ID_FILE_PLAY_BACK:
		MameUI_PlayBackInput();
		return true;

	case ID_FILE_PLAY_RECORD_WAVE:
		MamePlayRecordWave();
		return true;

	case ID_FILE_PLAY_RECORD_MNG:
		MamePlayRecordMNG();
		return true;

	case ID_FILE_PLAY_RECORD_AVI:
		MamePlayRecordAVI();
		return true;

	case ID_FILE_LOADSTATE :
		MameUI_LoadState();
		return true;

	case ID_FILE_AUDIT:
		AuditDialog(hMain, 1);
		ResetWhichGamesInFolders();
		ResetListView();
		(void)input::set_focus(hwndList);
		return true;

	case ID_FILE_AUDIT_X:
		AuditDialog(hMain, 2);
		ResetWhichGamesInFolders();
		ResetListView();
		(void)input::set_focus(hwndList);
		return true;

	case ID_FILE_EXIT:
		(void)windows::post_message(hMain, WM_CLOSE, 0, 0);
		return true;

	case ID_VIEW_LARGE_ICON:
		SetView(ID_VIEW_LARGE_ICON);
		return true;

	case ID_VIEW_SMALL_ICON:
		SetView(ID_VIEW_SMALL_ICON);
		ResetListView();
		return true;

	case ID_VIEW_LIST_MENU:
		SetView(ID_VIEW_LIST_MENU);
		return true;

	case ID_VIEW_DETAIL:
		SetView(ID_VIEW_DETAIL);
		return true;

	case ID_VIEW_GROUPED:
		SetView(ID_VIEW_GROUPED);
		return true;

	// Arrange Icons submenu
	case ID_VIEW_BYGAME:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_GAMES);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_BYDIRECTORY:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_DIRECTORY);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_BYMANUFACTURER:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_MANUFACTURER);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_BYTIMESPLAYED:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_PLAYED);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_BYTYPE:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_TYPE);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_BYYEAR:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_YEAR);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_FOLDERS:
	{
		int val = GetWindowPanes() ^ 1;
		bool bShowTree = is_flag_set(val, 0);
		SetWindowPanes(val);
		(void)menus::check_menu_item(hMainMenu, ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
		(void)tool_bar::check_button(s_hToolBar, ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
		UpdateScreenShot();
		break;
	}

	case ID_VIEW_TOOLBARS:
		bShowToolBar = !bShowToolBar;
		SetShowToolBar(bShowToolBar);
		(void)menus::check_menu_item(hMainMenu, ID_VIEW_TOOLBARS, (bShowToolBar) ? MF_CHECKED : MF_UNCHECKED);
		(void)tool_bar::check_button(s_hToolBar, ID_VIEW_TOOLBARS, (bShowToolBar) ? MF_CHECKED : MF_UNCHECKED);
		(void)windows::show_window(s_hToolBar, (bShowToolBar) ? SW_SHOW : SW_HIDE);
		ResizePickerControls(hMain);
		UpdateScreenShot();
		break;

	case ID_VIEW_STATUS:
		bShowStatusBar = !bShowStatusBar;
		SetShowStatusBar(bShowStatusBar);
		(void)menus::check_menu_item(hMainMenu, ID_VIEW_STATUS, (bShowStatusBar) ? MF_CHECKED : MF_UNCHECKED);
		(void)tool_bar::check_button(s_hToolBar, ID_VIEW_STATUS, (bShowStatusBar) ? MF_CHECKED : MF_UNCHECKED);
		(void)windows::show_window(hStatusBar, (bShowStatusBar) ? SW_SHOW : SW_HIDE);
		ResizePickerControls(hMain);
		UpdateScreenShot();
		break;

	case ID_VIEW_PAGETAB:
		bShowTabCtrl = !bShowTabCtrl;
		SetShowTabCtrl(bShowTabCtrl);
		(void)windows::show_window(hTabCtrl, (bShowTabCtrl) ? SW_SHOW : SW_HIDE);
		ResizePickerControls(hMain);
		UpdateScreenShot();
		gdi::invalidate_rect(hMain,nullptr,TRUE);
		break;

	   // Switches to fullscreen mode. No check mark handling
	   // for this item cause in fullscreen mode the menu won't
	   // be visible anyways.

	case ID_VIEW_FULLSCREEN:
		SwitchFullScreenMode();
		break;

	case ID_TOOLBAR_EDIT:
		{
			char *buf;
			HWND hToolbarEdit;

			buf = windows::get_window_text_utf8(hwndCtl);
			switch (codeNotify)
			{
			case TOOLBAR_EDIT_ACCELERATOR_PRESSED:
				hToolbarEdit = dialog_boxes::get_dlg_item( s_hToolBar, ID_TOOLBAR_EDIT);
				(void)input::set_focus(hToolbarEdit);
				break;
			case EN_CHANGE:
				//put search routine here first, add a 200ms timer later.
				if (buf && *buf != '\0')
				{
					if (!mui_stricmp(buf, SEARCH_PROMPT) || !mui_stricmp(g_SearchText, SEARCH_PROMPT))
						g_SearchText = buf;
					else
					{
						g_SearchText = buf;
						ResetListView();
					}
				}
				break;
			case EN_SETFOCUS:
				if (!mui_stricmp(buf, SEARCH_PROMPT))
					windows::set_window_text_utf8(hwndCtl, "");
				break;
			case EN_KILLFOCUS:
				if (!buf || *buf == '\0')
				{
					std::string search_prompt(SEARCH_PROMPT);
					windows::set_window_text_utf8(hwndCtl, search_prompt.c_str());
				}
				break;
			}
			delete[] buf;
		}
		break;

	case ID_GAME_AUDIT:
		InitGameAudit(0);
		if (nCurrentGame >= 0)
		{
			InitPropertyPageToPage(hInst, hwnd, GetSelectedPickItemIcon(), SOFTWARETYPE_GAME, -1, nCurrentGame, AUDIT_PAGE);
		}
		// Just in case the toggle MMX on/off
		UpdateStatusBar();
		break;

	// ListView Context Menu
	case ID_CONTEXT_ADD_CUSTOM:
	{
		if (nCurrentGame >= 0)
			dialog_boxes::dialog_box(system_services::get_module_handle(nullptr),menus::make_int_resource(IDD_CUSTOM_FILE), hMain, AddCustomFileDialogProc, nCurrentGame);
		(void)input::set_focus(hwndList);
		break;
	}

	case ID_CONTEXT_REMOVE_CUSTOM:
	{
		RemoveCurrentGameCustomFolder();
		break;
	}

	// Tree Context Menu
	case ID_CONTEXT_FILTERS:
		if (dialog_boxes::dialog_box(system_services::get_module_handle(nullptr), menus::make_int_resource(IDD_FILTERS), hMain, FilterDialogProc, 0L) == true)
			ResetListView();
		(void)input::set_focus(hwndList);
		return true;

		// ScreenShot Context Menu
		// select current tab
	case ID_VIEW_TAB_SCREENSHOT:
	case ID_VIEW_TAB_TITLE:
	case ID_VIEW_TAB_SCORES:
	case ID_VIEW_TAB_HOWTO:
	case ID_VIEW_TAB_SELECT:
	case ID_VIEW_TAB_VERSUS:
	case ID_VIEW_TAB_BOSSES:
	case ID_VIEW_TAB_COVER:
	case ID_VIEW_TAB_ENDS:
	case ID_VIEW_TAB_GAMEOVER:
	case ID_VIEW_TAB_LOGO:
	case ID_VIEW_TAB_ARTWORK:
	case ID_VIEW_TAB_FLYER:
	case ID_VIEW_TAB_CABINET:
	case ID_VIEW_TAB_MARQUEE:
	case ID_VIEW_TAB_CONTROL_PANEL:
	case ID_VIEW_TAB_PCB:
	case ID_VIEW_TAB_HISTORY:
		if (id == ID_VIEW_TAB_HISTORY && GetShowTab(TAB_HISTORY) == false)
			break;

		TabView_SetCurrentTab(hTabCtrl, id - ID_VIEW_TAB_ARTWORK);
		UpdateScreenShot();
		TabView_UpdateSelection(hTabCtrl);
		break;

		// toggle tab's existence
	case ID_TOGGLE_TAB_SCREENSHOT:
	case ID_TOGGLE_TAB_TITLE:
	case ID_TOGGLE_TAB_SCORES:
	case ID_TOGGLE_TAB_HOWTO:
	case ID_TOGGLE_TAB_SELECT:
	case ID_TOGGLE_TAB_VERSUS:
	case ID_TOGGLE_TAB_BOSSES:
	case ID_TOGGLE_TAB_COVER:
	case ID_TOGGLE_TAB_ENDS:
	case ID_TOGGLE_TAB_GAMEOVER:
	case ID_TOGGLE_TAB_LOGO:
	case ID_TOGGLE_TAB_ARTWORK:
	case ID_TOGGLE_TAB_FLYER:
	case ID_TOGGLE_TAB_CABINET:
	case ID_TOGGLE_TAB_MARQUEE:
	case ID_TOGGLE_TAB_CONTROL_PANEL:
	case ID_TOGGLE_TAB_PCB:
	case ID_TOGGLE_TAB_HISTORY:
	{
		int toggle_flag = id - ID_TOGGLE_TAB_ARTWORK;

		if (AllowedToSetShowTab(toggle_flag,!GetShowTab(toggle_flag)) == false)
		{
			// attempt to hide the last tab
			// should show error dialog? hide picture area? or ignore?
			break;
		}

		SetShowTab(toggle_flag,!GetShowTab(toggle_flag));

		TabView_Reset(hTabCtrl);

		if (TabView_GetCurrentTab(hTabCtrl) == toggle_flag && GetShowTab(toggle_flag) == false)
		{
			// we're deleting the tab we're on, so go to the next one
			TabView_CalculateNextTab(hTabCtrl);
		}

		// Resize the controls in case we toggled to another history
		// mode (and the history control needs resizing).

		ResizePickerControls(hMain);
		UpdateScreenShot();
		TabView_UpdateSelection(hTabCtrl);
		break;
	}

	// Header Context Menu
	case ID_SORT_ASCENDING:
		SetSortReverse(FALSE);
		SetSortColumn(Picker_GetRealColumnFromViewColumn(hwndList, lastColumnClick));
		Picker_Sort(hwndList);
		break;

	case ID_SORT_DESCENDING:
		SetSortReverse(TRUE);
		SetSortColumn(Picker_GetRealColumnFromViewColumn(hwndList, lastColumnClick));
		Picker_Sort(hwndList);
		break;

	case ID_CUSTOMIZE_FIELDS:
		if (dialog_boxes::dialog_box(system_services::get_module_handle(nullptr), menus::make_int_resource(IDD_COLUMNS), hMain, ColumnDialogProc, 0L) == true)
			ResetColumnDisplay(FALSE);
		(void)input::set_focus(hwndList);
		return true;

	// View Menu -  MESSUI: not used any more 2014-01-26
	case ID_VIEW_LINEUPICONS:
		if( codeNotify == false)
			ResetListView();
		else
		{
			// it was sent after a refresh (F5) was done, we only reset the View if "available" is the selected folder
			// as it doesn't affect the others.
			folder = GetSelectedFolder();
			if( folder )
			{
				if (folder->m_nFolderId == FOLDER_AVAILABLE )
					ResetListView();
			}
		}
		break;

	case ID_GAME_PROPERTIES:
		if (nCurrentGame >= 0)
		{
			InitPropertyPageToPage(hInst, hwnd, GetSelectedPickItemIcon(), SOFTWARETYPE_GAME, -1, nCurrentGame, PROPERTIES_PAGE);
			{
				if (g_bModifiedSoftwarePaths)
				{
					g_bModifiedSoftwarePaths = false;
//#ifdef MESS
					MessUpdateSoftwareList(); // messui.cpp
//#endif
				}
			}
		}
		UpdateStatusBar();
		break;

	case ID_FOLDER_PROPERTIES:
		{
			folder = GetSelectedFolder();
			if (folder)
				if (folder->m_dwFlags & F_INIEDIT)
				{
					LPCFOLDERDATA data = FindFilter(folder->m_nFolderId);
					if (data)
						if (data->m_soft_type_opt < TOTAL_SOFTWARETYPE_OPTIONS)
							InitPropertyPage(hInst, hwnd, GetSelectedFolderIcon(), data->m_soft_type_opt, folder->m_nFolderId, 0);
				}
		}
		UpdateStatusBar();
		break;

	case ID_FOLDER_SOURCEPROPERTIES:
	{
		if (nCurrentGame < 0)
			return true;
		InitPropertyPage(hInst, hwnd, GetSelectedFolderIcon(), SOFTWARETYPE_SOURCE, -1, nCurrentGame);
		UpdateStatusBar();
		(void)input::set_focus(hwndList);
		return true;
		//folder = GetFolderByName(FOLDER_SOURCE, GetDriverFilename(Picker_GetSelectedItem(hwndList)) );
		//InitPropertyPage(hInst, hwnd, GetSelectedFolderIcon(), (folder->m_nFolderId == FOLDER_VECTOR) ? SOFTWARETYPE_VECTOR : SOFTWARETYPE_SOURCE , folder->m_nFolderId, Picker_GetSelectedItem(hwndList));
		//UpdateStatusBar();
		//break;
	}

	case ID_FOLDER_VECTORPROPERTIES:
		if (nCurrentGame >= 0)
		{
			folder = GetFolderByID( FOLDER_VECTOR );
			InitPropertyPage(hInst, hwnd, GetSelectedFolderIcon(), SOFTWARETYPE_VECTOR, folder->m_nFolderId, nCurrentGame);
		}
		// Just in case the toggle MMX on/off
		UpdateStatusBar();
		break;

	case ID_FOLDER_AUDIT:
		FolderCheck();
		// Just in case the toggle MMX on/off
		UpdateStatusBar();
		break;

	case ID_VIEW_PICTURE_AREA :
		ToggleScreenShot();
		break;

//#ifdef MESS // check if really need to disable the line below
	case ID_VIEW_SOFTWARE_AREA :
		ToggleSoftware();
		break;
//#endif

	case ID_UPDATE_GAMELIST:
		UpdateGameList(TRUE, true);
		break;

	case ID_UPDATE_CACHE:
		UpdateCache();
		break;

	case ID_OPTIONS_FONT:
		PickFont();
		return true;

	case ID_OPTIONS_CLONE_COLOR:
		PickCloneColor();
		return true;

	case ID_OPTIONS_DEFAULTS:
		// Check the return value to see if changes were applied
		InitDefaultPropertyPage(hInst, hwnd);
		(void)input::set_focus(hwndList);
		return true;

	case ID_OPTIONS_DIR:
		{
			int nResult = dialog_boxes::dialog_box(system_services::get_module_handle(nullptr), menus::make_int_resource(IDD_DIRECTORIES), hMain, DirectoriesDialogProc, 0L);

			emu_opts.global_save_ini();
			mui_save_ini();
			emu_opts.ui_save_ini();

			bool bUpdateRoms    = ((nResult & DIRDLG_ROMS) == DIRDLG_ROMS) ? true : false;
			bool bUpdateSamples = ((nResult & DIRDLG_SAMPLES) == DIRDLG_SAMPLES) ? true : false;
//#ifdef MESS
			bool bUpdateSoftware = ((nResult & DIRDLG_SW) == DIRDLG_SW) ? true : false;

			if (bUpdateSoftware)
				MessUpdateSoftwareList(); // messui.cpp
//#endif

			if (s_pWatcher)
			{
				if (bUpdateRoms)
					DirWatcher_Watch(s_pWatcher, 0, emu_opts.dir_get_value(DIRPATH_MEDIAPATH), true);
				if (bUpdateSamples)
					DirWatcher_Watch(s_pWatcher, 1, emu_opts.dir_get_value(DIRPATH_SAMPLEPATH), true);
			}

			// update game list
			if (bUpdateRoms == true || bUpdateSamples == true)
				UpdateGameList(bUpdateRoms, bUpdateSamples);

			(void)input::set_focus(hwndList);
		}
		return true;

	case ID_OPTIONS_RESET_DEFAULTS:
		if (dialog_boxes::dialog_box(system_services::get_module_handle(nullptr), menus::make_int_resource(IDD_RESET), hMain, ResetDialogProc, 0L) == true)
		{
			// these may have been changed
			emu_opts.global_save_ini();
			mui_save_ini();
			emu_opts.ui_save_ini();
			windows::destroy_window(hwnd);
			windows::post_quiet_message(0);
		}
		else
		{
			ResetListView();
			(void)input::set_focus(hwndList);
		}
		return true;

	case ID_OPTIONS_INTERFACE:
		(void)dialog_boxes::dialog_box(system_services::get_module_handle(nullptr), menus::make_int_resource(IDD_INTERFACE_OPTIONS), hMain, InterfaceDialogProc, 0L);
		emu_opts.global_save_ini();
		mui_save_ini();
		emu_opts.ui_save_ini();

		KillTimer(hMain, SCREENSHOT_TIMER);
		if( GetCycleScreenshot() > 0)
			SetTimer(hMain, SCREENSHOT_TIMER, GetCycleScreenshot()*1000, nullptr ); // Scale to seconds

		return true;

	case ID_VIDEO_SNAP:
		{
			int nGame = Picker_GetSelectedItem(hwndList);
			if (nGame >= 0)
			{
				std::string videofile_name = driver_list::driver(nGame).name + ".mp4"s;
				std::filesystem::path videofile_path = std::filesystem::path(GetVideoDir()) / videofile_name;
				ShellExecuteCommon(hMain, videofile_path.wstring());
			}
			(void)input::set_focus(hwndList);
		}
		break;

	case ID_MANUAL:
		{
			int nGame = Picker_GetSelectedItem(hwndList);
			if (nGame >= 0)
			{
				std::string manualfile_name = driver_list::driver(nGame).name + ".pdf"s;
				std::filesystem::path manualfile_path = std::filesystem::path(GetManualsDir()) / manualfile_name;
				ShellExecuteCommon(hMain, manualfile_path.wstring());
			}
			(void)input::set_focus(hwndList);
		}
		break;

	case ID_RC_CLEAN:
	{
		int nGame = Picker_GetSelectedItem(hwndList);
		if (nGame >= 0)
		{
			std::string drvname = driver_list::driver(nGame).name;
			stringtokenizer tokenizer;

			// INI
			tokenizer.set_input(emu_opts.dir_get_value(DIRPATH_INIPATH), ";");
			delete_in_dirs(tokenizer, drvname, ".ini");

			// CFG
			tokenizer.set_input(emu_opts.dir_get_value(DIRPATH_CFG_DIRECTORY), ";");
			delete_in_dirs(tokenizer, drvname, ".cfg");

			// NVRAM (no extension)
			tokenizer.set_input(emu_opts.dir_get_value(DIRPATH_NVRAM_DIRECTORY), ";");
			delete_in_dirs(tokenizer, drvname);

			// Save states (no extension)
			tokenizer.set_input(emu_opts.dir_get_value(DIRPATH_STATE_DIRECTORY), ";");
			delete_in_dirs(tokenizer, drvname);
		}
		(void)input::set_focus(hwndList);
	}
	break;

	case ID_NOTEPAD:
		{
			int nGame = Picker_GetSelectedItem(hwndList);
			if (nGame >= 0)
			{
				std::filesystem::path history_path(".");
				std::filesystem::path history_filename("history.wtx");
				history_path /= history_filename;
				std::ofstream outfile (history_path, std::ios::out | std::ios::trunc);
				std::string file_contents = GetGameHistory(nGame);
				outfile.write(file_contents.c_str(), file_contents.size());
				outfile.close();
				ShellExecuteCommon(hMain, history_path.wstring());
			}
			(void)input::set_focus(hwndList);
		}
		break;

	case ID_OPTIONS_BG:
		{
			// Get the path from the existing filename; if no filename go to root
			OPENFILENAMEW OFN;
			wchar_t path_buffer[MAX_PATH]{ L"\0" };
			std::wstring initial_dir_path;

			std::filesystem::path background_path(GetBgDir());
			if (!std::filesystem::exists(background_path))
				initial_dir_path = L".";
			else
			{
				std::wstring filename = background_path.filename().wstring();
				initial_dir_path = background_path.parent_path().wstring();
				mui_wcscpy(path_buffer, filename.c_str());
			}

			OFN.lStructSize       = sizeof(OPENFILENAMEW);
			OFN.hwndOwner         = hMain;
			OFN.hInstance         = 0;
			OFN.lpstrFilter       = L"Image Files (*.png)\0*.PNG\0";
			OFN.lpstrCustomFilter = nullptr;
			OFN.nMaxCustFilter    = 0;
			OFN.nFilterIndex      = 1;
			OFN.lpstrFile         = path_buffer;
			OFN.nMaxFile          = MAX_PATH;
			OFN.lpstrFileTitle    = nullptr;
			OFN.nMaxFileTitle     = 0;
			OFN.lpstrInitialDir   = initial_dir_path.c_str();
			OFN.lpstrTitle        = L"Select a Background Image";
			OFN.nFileOffset       = 0;
			OFN.nFileExtension    = 0;
			OFN.lpstrDefExt       = nullptr;
			OFN.lCustData         = 0;
			OFN.lpfnHook          = nullptr;
			OFN.lpTemplateName    = nullptr;
			OFN.Flags             = OFN_NOCHANGEDIR | OFN_SHOWHELP | OFN_EXPLORER;

			bResult = dialog_boxes::get_open_filename(&OFN);
			if (bResult)
			{
				std::filesystem::path file_path(path_buffer);
				if (!std::filesystem::exists(file_path))
					return false;

				SetBgDir(file_path.string());

				// Display new background
				LoadBackgroundBitmap();
				(void)gdi::invalidate_rect(hMain, nullptr, true);

				return true;
			}
		}
		break;

	case ID_HELP_ABOUT:
		(void)dialog_boxes::dialog_box(system_services::get_module_handle(nullptr), menus::make_int_resource(IDD_ABOUT), hMain, AboutDialogProc, 0L);
		(void)input::set_focus(hwndList);
		return true;

	case IDOK :
		if (codeNotify != EN_CHANGE && codeNotify != EN_UPDATE)
		{
			// enter key
			if (g_in_treeview_edit)
			{
				bResult = TreeView_EndEditLabelNow(hTreeView, false);
				return true;
			}
			else
				if (have_selection)
					MamePlayGame();
		}
		break;

	case IDCANCEL : // esc key
		if (g_in_treeview_edit)
			bResult = TreeView_EndEditLabelNow(hTreeView, true);
		break;

	case IDC_PLAY_GAME :
		if (have_selection)
			MamePlayGame();
		break;

	case ID_UI_START:
		(void)input::set_focus(hwndList);
		MamePlayGame();
		break;

	case ID_UI_UP:
		Picker_SetSelectedPick(hwndList, GetSelectedPick() - 1);
		break;

	case ID_UI_DOWN:
		Picker_SetSelectedPick(hwndList, GetSelectedPick() + 1);
		break;

	case ID_UI_PGUP:
		Picker_SetSelectedPick(hwndList, GetSelectedPick() - list_view::get_count_per_page(hwndList));
		break;

	case ID_UI_PGDOWN:
		if ((GetSelectedPick() + list_view::get_count_per_page(hwndList)) < list_view::get_item_count(hwndList))
			Picker_SetSelectedPick(hwndList, GetSelectedPick() + list_view::get_count_per_page(hwndList));
		else
			Picker_SetSelectedPick(hwndList, list_view::get_item_count(hwndList) - 1);
		break;

	case ID_UI_HOME:
		Picker_SetSelectedPick(hwndList, 0);
		break;

	case ID_UI_END:
		Picker_SetSelectedPick(hwndList, list_view::get_item_count(hwndList) - 1);
		break;

	case ID_UI_LEFT:
		(void)windows::send_message(hwndList,WM_HSCROLL, SB_LINELEFT, 0);
		break;

	case ID_UI_RIGHT:
		(void)windows::send_message(hwndList,WM_HSCROLL, SB_LINERIGHT, 0);
		break;

	case ID_UI_HISTORY_UP:
		{
			HWND hHistory = dialog_boxes::get_dlg_item(hMain, IDC_HISTORY);
			(void)windows::send_message(hHistory, EM_SCROLL, SB_PAGEUP, 0);
		}
		break;

	case ID_UI_HISTORY_DOWN:
		{
			HWND hHistory = dialog_boxes::get_dlg_item(hMain, IDC_HISTORY);
			(void)windows::send_message(hHistory, EM_SCROLL, SB_PAGEDOWN, 0);
		}
		break;

	case IDC_SSFRAME:
		TabView_CalculateNextTab(hTabCtrl);
		UpdateScreenShot();
		TabView_UpdateSelection(hTabCtrl);
		break;

	case ID_CONTEXT_SELECT_RANDOM:
		SetRandomPickItem();
		break;

	case ID_CONTEXT_RESET_PLAYSTATS:
		if (nCurrentGame < 0)
			break;
		ResetPlayTime(nCurrentGame);
		ResetPlayCount(nCurrentGame);
		bResult = list_view::redraw_items(hwndList, GetSelectedPick(), GetSelectedPick());
		break;

	case ID_CONTEXT_RENAME_CUSTOM :
		tree_view::edit_label(hTreeView,tree_view::get_selection(hTreeView));
		break;

	default:
		if (id >= ID_CONTEXT_SHOW_FOLDER_START && id < ID_CONTEXT_SHOW_FOLDER_END)
		{
			ToggleShowFolder(id - ID_CONTEXT_SHOW_FOLDER_START);
			break;
		}
		for (size_t i = 0; g_helpInfo[i].nMenuItem > 0; i++)
		{
			const wchar_t *shellExecVerb = L"open",
						*shellExecParam = L"";

			if (g_helpInfo[i].nMenuItem == id)
			{
				//std::wcout << std::hex << std::uppercase << g_helpInfo[i].bIsHtmlHelp << ": " << g_helpInfo[i].lpFile << "\n";

				if (i == 1) // get current whatsnew.txt from mamedev.org
				{
					std::string version = std::string(GetVersionString()); // turn version string into std
					version.erase(1,1); // take out the decimal point
					version.erase(4, std::string::npos); // take out the date
					std::string url = "https://mamedev.org/releases/whatsnew_" + version + ".txt"; // construct url
					std::unique_ptr<wchar_t[]> utf8_to_wcs(mui_utf16_from_utf8cstring(url.c_str())); // then convert to const wchar_t*
					(void)shell::shell_execute(hMain, shellExecVerb, utf8_to_wcs.get(), shellExecParam, nullptr, SW_SHOWNORMAL); // show web page
				}
				else
				if (g_helpInfo[i].bIsHtmlHelp)
//                  HelpFunction(hMain, g_helpInfo[i].lpFile, HH_DISPLAY_TOPIC, 0);
					(void)shell::shell_execute(hMain, shellExecVerb, g_helpInfo[i].lpFile.c_str(), shellExecParam, nullptr, SW_SHOWNORMAL);
//              else
//                  DisplayTextFile(hMain, g_helpInfo[i].lpFile);
				return false;
			}
		}
//#ifdef MESS
		return MessCommand(hwnd, id, hwndCtl, codeNotify); // messui.cpp: Open Other Software menu choice
//#endif
	}
	return false;
}


static void LoadBackgroundBitmap()
{
	HGLOBAL hDIBbg;

	if (hbBackground)
	{
		(void)gdi::delete_bitmap(hbBackground);
		hbBackground = nullptr;
	}

	if (hpBackground)
	{
		(void)gdi::delete_palette(hpBackground);
		hpBackground = nullptr;
	}

	if (LoadDIBBG(&hDIBbg, &hpBackground)) // screenshot.cpp
	{
		HDC hDC = gdi::get_dc(hwndList);
		hbBackground = DIBToDDB(hDC, hDIBbg, &bmDesc);
		(void)GlobalFree(hDIBbg);
		(void)gdi::release_dc(hwndList, hDC);
	}
}


static void ResetColumnDisplay(bool first_time)
{
	if (!first_time)
		Picker_ResetColumnDisplay(hwndList);

	ResetListView();

	Picker_SetSelectedItem(hwndList, GetDefaultGame());
}


static int GamePicker_GetItemImage(HWND hwndPicker, int item_index)
{
	return GetIconForDriver(item_index);
}


static std::wstring GamePicker_GetItemString(HWND hwndPicker, int item_index, int nColumn)
{
	game_driver driver = driver_list::driver(item_index);

	switch (nColumn)
	{
	case COLUMN_GAMES:
		// Driver description
		return mui_utf16_from_utf8string(driver.type.fullname());
	case COLUMN_ORIENTATION:
		// Screen orientation
		return DriverIsVertical(item_index) ? L"Vertical" : L"Horizontal";
	case COLUMN_ROMS:
		// ROMs
		return GetAuditString(GetRomAuditResults(item_index));
	case COLUMN_SAMPLES:
		// Samples
		return DriverUsesSamples(item_index) ? GetAuditString(GetSampleAuditResults(item_index)) : L"-";
	case COLUMN_DIRECTORY:
		// Driver name (directory)
		return mui_utf16_from_utf8string(driver.name);
	case COLUMN_SRCDRIVERS:
		// Source drivers
		return GetDriverFilename(item_index);
	case COLUMN_PLAYTIME:
		// Play time
		return GetTextPlayTime(item_index);
	case COLUMN_TYPE:
	{
		// Vector/Raster
		machine_config config(driver, emu_opts.GetGlobalOpts());
		return isDriverVector(&config) ? L"Vector" : L"Raster";
	}
	case COLUMN_TRACKBALL:
		// Trackball
		return DriverUsesTrackball(item_index) ? L"Yes" : L"No";
	case COLUMN_PLAYED:
		// Times played
		return std::to_wstring(GetPlayCount(item_index));
	case COLUMN_MANUFACTURER:
		// Manufacturer
		return mui_utf16_from_utf8string(driver.manufacturer);

	case COLUMN_YEAR:
		// Year
		return mui_utf16_from_utf8string(driver.year);
	case COLUMN_CLONE:
		// Clones
		return mui_utf16_from_utf8string(GetCloneParentName(item_index));
	default:
		return std::wstring{};
	}
}


static void GamePicker_LeavingItem(HWND hwndPicker, int item_index)
{
//#ifdef MESS
	// leaving item
	g_szSelectedItem.clear();
//#endif
}


static void GamePicker_EnteringItem(HWND hwndPicker, int item_index)
{
	//std::cout << "entering " << driver_list::driver(item_index).name << "\n";
	EnableSelection(item_index);

//#ifdef MESS
	MessReadMountedSoftware(item_index); // messui.cpp
//#endif

	// decide if it is valid to load a savestate
	(void)menus::enable_menu_item(hMainMenu, ID_FILE_LOADSTATE, DriverSupportsSaveState(item_index) ? MFS_ENABLED : MFS_GRAYED);
}


static int GamePicker_FindItemParent(HWND hwndPicker, int item_index)
{
	return GetParentRomSetIndex(&driver_list::driver(item_index));
}


// Initialize the Picker and List controls
static void InitListView()
{
	LVBKIMAGEW bki;
	std::unique_ptr<wchar_t[]> wcs_BgDir;

	static const PickerCallbacks s_gameListCallbacks =
	{
		SetSortColumn,                  // pfnSetSortColumn
		GetSortColumn,                  // pfnGetSortColumn
		SetSortReverse,                 // pfnSetSortReverse
		GetSortReverse,                 // pfnGetSortReverse
		SetViewMode,                    // pfnSetViewMode
		GetViewMode,                    // pfnGetViewMode
		SetColumnWidths,                // pfnSetColumnWidths
		GetColumnWidths,                // pfnGetColumnWidths
		SetColumnOrder,                 // pfnSetColumnOrder
		GetColumnOrder,                 // pfnGetColumnOrder
		SetColumnShown,                 // pfnSetColumnShown
		GetColumnShown,                 // pfnGetColumnShown
		GetOffsetClones,                // pfnGetOffsetChildren

		GamePicker_Compare,             // pfnCompare
		MamePlayGame,                   // pfnDoubleClick
		GamePicker_GetItemString,       // pfnGetItemString
		GamePicker_GetItemImage,        // pfnGetItemImage
		GamePicker_LeavingItem,         // pfnLeavingItem
		GamePicker_EnteringItem,        // pfnEnteringItem
		BeginListViewDrag,              // pfnBeginListViewDrag
		GamePicker_FindItemParent,      // pfnFindItemParent
		OnIdle,                         // pfnIdle
		GamePicker_OnHeaderContextMenu, // pfnOnHeaderContextMenu
		GamePicker_OnBodyContextMenu    // pfnOnBodyContextMenu
	};

	PickerOptions opts;

	// subclass the list view
	opts = {};
	opts.pCallbacks = &s_gameListCallbacks;
	opts.nColumnCount = COLUMN_COUNT;
	opts.column_names = column_names;
	SetupPicker(hwndList, &opts);

	(void)list_view::set_text_bk_color(hwndList, CLR_NONE);
	(void)list_view::set_bk_color(hwndList, CLR_NONE);
	std::wstring utf16_background_path = mui_utf16_from_utf8string(GetBgDir());
	if (utf16_background_path.empty())
		return;

	bki.ulFlags = LVBKIF_SOURCE_URL | LVBKIF_STYLE_TILE;
	bki.pszImage = const_cast<wchar_t*>(utf16_background_path.c_str());
	if( hbBackground )
		(void)ListView_SetBkImage(hwndList, &bki);

	CreateIcons();
	ResetColumnDisplay(TRUE);

	// Allow selection to change the default saved game
	bListReady = true;
}


static void AddDriverIcon(int item_index,int default_icon_index)
{
	HICON hIcon = 0;
	int nParentIndex = -1;

	// if already set to rom or clone icon, we've been here before
	if (icon_index[item_index] == 1 || icon_index[item_index] == 3)
		return;

	hIcon = LoadIconFromFile((char *)driver_list::driver(item_index).name);
	if (hIcon == nullptr)
	{
		nParentIndex = GetParentIndex(&driver_list::driver(item_index));
		if( nParentIndex >= 0)
		{
			hIcon = LoadIconFromFile((char *)driver_list::driver(nParentIndex).name);
			if (hIcon == nullptr)
			{
				nParentIndex = GetParentIndex(&driver_list::driver(nParentIndex));
				if (!(nParentIndex < 0))
					hIcon = LoadIconFromFile((char*)driver_list::driver(nParentIndex).name);
			}
		}
	}

	if (hIcon)
	{
		int nIconPos = -1;
		image_list::add_icon(hLarge, hIcon);
		nIconPos = image_list::add_icon(hSmall, hIcon);
		if (nIconPos != -1)
			icon_index[item_index] = nIconPos;
		(void)DestroyIcon(hIcon);
	}
	if (icon_index[item_index] == 0)
		icon_index[item_index] = default_icon_index;
}


static void DestroyIcons(void)
{
	if (hSmall)
	{
		(void)ImageList_Destroy(hSmall);
		hSmall = nullptr;
	}

	if (icon_index)
	{
		for (size_t i=0;i<driver_list::total();i++)
			icon_index[i] = 0; // these are indices into hSmall
	}

	if (hLarge)
	{
		(void)ImageList_Destroy(hLarge);
		hLarge = nullptr;
	}

	if (hHeaderImages)
	{
		(void)ImageList_Destroy(hHeaderImages);
		hHeaderImages = nullptr;
	}

}


static void ReloadIcons(void)
{
	HICON hIcon;

	// clear out all the images
	ImageList_RemoveAll(hSmall);
	ImageList_RemoveAll(hLarge);

	if (icon_index)
		for (size_t i=0;i<driver_list::total();i++)
			icon_index[i] = 0; // these are indices into hSmall

	for (size_t i = 0; !g_iconData[i].icon_name.empty(); i++)
	{
		hIcon = LoadIconFromFile(g_iconData[i].icon_name.c_str());
		if (hIcon == nullptr)
			hIcon = menus::load_icon(hInst, menus::make_int_resource(g_iconData[i].resource));

		image_list::add_icon(hSmall, hIcon);
		image_list::add_icon(hLarge, hIcon);
		(void)DestroyIcon(hIcon);
	}
}


static DWORD GetShellLargeIconSize(void)
{
	DWORD  dwSize = 32, dwLength = 512, dwType = REG_SZ;
	HKEY   hKey;
	LPWSTR wErrorMessage = nullptr;

	// Get the Key
	LONG lRes = registry::reg_open_key_ex(HKEY_CURRENT_USER, L"Control Panel\\Desktop\\WindowMetrics", REG_OPTION_OPEN_LINK, KEY_READ, &hKey);
	if( lRes != ERROR_SUCCESS )
	{
		GetSystemErrorMessage(lRes, &wErrorMessage);
		(void)dialog_boxes::message_box(GetMainWindow(), wErrorMessage, L"Large shell icon size registry access", MB_OK | MB_ICONERROR);
		(void)system_services::local_free(wErrorMessage);
		return dwSize;
	}

	// Save the last size
	wchar_t  szBuffer[512];
	lRes = registry::reg_query_value_ex(hKey, L"Shell Icon Size", nullptr, &dwType, (LPBYTE)szBuffer, &dwLength);
	if( lRes != ERROR_SUCCESS )
	{
		GetSystemErrorMessage(lRes, &wErrorMessage);
		(void)dialog_boxes::message_box(GetMainWindow(), wErrorMessage, L"Large shell icon size registry query", MB_OK | MB_ICONERROR);
		(void)system_services::local_free(wErrorMessage);
	}
	else
	{
		dwSize = _wtol(szBuffer);
		if (dwSize < 32)
			dwSize = 32;

		if (dwSize > 48)
			dwSize = 48;
	}
	// Clean up
	(void)registry::reg_close_key(hKey);
	return dwSize;
}


static DWORD GetShellSmallIconSize(void)
{
	DWORD dwSize = ICONMAP_WIDTH;

	if (dwSize > 48)
		dwSize = 48;
	else if (dwSize < 32)
		dwSize = 16;
	else
		dwSize = 32;

	return dwSize;
}


// create iconlist for Listview control
static void CreateIcons(void)
{
	DWORD dwSmallIconSize = GetShellSmallIconSize();
	DWORD dwLargeIconSize = GetShellLargeIconSize();
	HICON hIcon;
	int icon_count = 0;
	int grow = 5000;

	while(!g_iconData[icon_count].icon_name.empty())
		icon_count++;

	// the current window style affects the sizing of the rows when changing
	// between list views, so put it in small icon mode temporarily while we associate
	// our image list

	// using large icon mode instead kills the horizontal scrollbar when doing
	// full refresh, which seems odd (it should recreate the scrollbar when
	// set back to report mode, for example, but it doesn't).

	LONG_PTR lpListViewStyle = windows::get_window_long_ptr(hwndList,GWL_STYLE);
	(void)windows::set_window_long_ptr(hwndList,GWL_STYLE,(lpListViewStyle & ~LVS_TYPEMASK) | LVS_ICON);

	hSmall = image_list::create(dwSmallIconSize, dwSmallIconSize, ILC_COLORDDB | ILC_MASK, icon_count, icon_count + grow);

	if (nullptr == hSmall)
	{
		(void)dialog_boxes::message_box(GetMainWindow(), L"Cannot allocate small icon image list", L"Allocation error - Exiting", MB_OK | MB_ICONERROR);
		windows::post_quiet_message(0);
	}

	hLarge = image_list::create(dwLargeIconSize, dwLargeIconSize, ILC_COLORDDB | ILC_MASK, icon_count, icon_count + grow);

	if (nullptr == hLarge)
	{
		(void)dialog_boxes::message_box(GetMainWindow(), L"Cannot allocate large icon image list", L"Allocation error - Exiting", MB_OK | MB_ICONERROR);
		windows::post_quiet_message(0);
	}

	ReloadIcons();

	// Associate the image lists with the list view control.
	(void)list_view::set_image_list(hwndList, hSmall, LVSIL_SMALL);
	(void)list_view::set_image_list(hwndList, hLarge, LVSIL_NORMAL);

	// restore our view
	(void)windows::set_window_long_ptr(hwndList,GWL_STYLE, lpListViewStyle);

//#ifdef MESS
	CreateMessIcons(); // messui.cpp
//#endif

	// Now set up header specific stuff
	hHeaderImages = image_list::create(8,8,ILC_COLORDDB | ILC_MASK,2,2);
	hIcon = menus::load_icon(hInst,menus::make_int_resource(IDI_HEADER_UP));
	image_list::add_icon(hHeaderImages,hIcon);
	hIcon = menus::load_icon(hInst,menus::make_int_resource(IDI_HEADER_DOWN));
	image_list::add_icon(hHeaderImages,hIcon);

	for (size_t i = 0; i < std::size(s_nPickers); i++)
		Picker_SetHeaderImageList(dialog_boxes::get_dlg_item(hMain, s_nPickers[i]), hHeaderImages);
}

static int compareAuditResults(int result1, int result2)
{
	if (result1 < 0 && result2 < 0)
		return 0;
	else if (result1 < 0)
		return -1;
	else if (result2 < 0)
		return 1;
	else
		return result1 - result2;
}

static int GamePicker_Compare(HWND hwndPicker, int index1, int index2, int sort_subitem, bool ascending)
{
	game_driver game1 = driver_list::driver(index1);
	game_driver game2 = driver_list::driver(index2);
	int value = 0;  // Default to 0, for unknown case

	switch (sort_subitem)
	{
	case COLUMN_CLONE: value = mui_stricmp(GetCloneParentName(index1), GetCloneParentName(index2)); break;
	case COLUMN_DIRECTORY: value = mui_stricmp(game1.name, game2.name); break;
	case COLUMN_GAMES: value = mui_stricmp(game1.type.fullname(), game2.type.fullname()); break;
	case COLUMN_MANUFACTURER: value = mui_stricmp(game1.manufacturer, game2.manufacturer); break;
	case COLUMN_ORIENTATION: value = DriverIsVertical(index1) - DriverIsVertical(index2); break;
	case COLUMN_PLAYED: value = GetPlayCount(index1) - GetPlayCount(index2); break;
	case COLUMN_PLAYTIME: value = GetPlayTime(index1) - GetPlayTime(index2); break;
	case COLUMN_ROMS:
	{
		int rom1 = GetRomAuditResults(index1);
		int rom2 = GetRomAuditResults(index2);
		value = compareAuditResults(rom1, rom2);
	}
	break;
	case COLUMN_SAMPLES:
	{
		int sample1 = GetSampleAuditResults(index1);
		int sample2 = GetSampleAuditResults(index2);
		value = compareAuditResults(sample1, sample2);
	}
	break;
	case COLUMN_SRCDRIVERS: value = mui_wcsicmp(GetDriverFilename(index1), GetDriverFilename(index2)); break;
	case COLUMN_TRACKBALL: value = DriverUsesTrackball(index1) - DriverUsesTrackball(index2); break;
	case COLUMN_TYPE:
	{
		machine_config config1(game1, emu_opts.GetGlobalOpts());
		machine_config config2(game2, emu_opts.GetGlobalOpts());
		value = isDriverVector(&config1) - isDriverVector(&config2);
	}
	break;
	case COLUMN_YEAR: value = mui_stricmp(game1.year, game2.year); break;
	}

	// Handle same comparisons here
	if (0 == value && COLUMN_GAMES != sort_subitem)
		value = GamePicker_Compare(hwndPicker, index1, index2, COLUMN_GAMES);

	return ascending ? -value : value;
}


int GetSelectedPick()
{
	// returns index of listview selected item
	// This will return -1 if not found
	return list_view::get_next_item(hwndList, -1, LVIS_SELECTED | LVIS_FOCUSED);
}


static HICON GetSelectedPickItemIcon()
{
	LVITEMW lvi;
	lvi.iItem = GetSelectedPick();
	lvi.iSubItem = 0;
	lvi.mask = LVIF_IMAGE;
	list_view::get_item(hwndList, &lvi);
	return ImageList_GetIcon(hLarge, lvi.iImage, ILD_TRANSPARENT);
}


static void SetRandomPickItem()
{
	int nListCount = list_view::get_item_count(hwndList);

	if (nListCount > 0)
	{
		static thread_local std::mt19937 gen(std::random_device{}());
		std::uniform_int_distribution<> dis(0, nListCount - 1);
		Picker_SetSelectedPick(hwndList, dis(gen));
	}
}


bool CommonFileDialog(common_file_dialog_proc cfd, wchar_t *filename, int filetype)
{
	OPENFILENAMEW ofn;
	std::string dir_option_value;

	switch (filetype)
	{
	case FILETYPE_INPUT_FILES :
		ofn.lpstrFilter   = L"input files (*.inp,*.zip,*.7z)\0*.inp;*.zip;*.7z\0All files (*.*)\0*.*\0";
		ofn.lpstrDefExt   = L"inp";
		dir_option_value = emu_opts.dir_get_value(DIRPATH_INPUT_DIRECTORY);
		break;
	case FILETYPE_SAVESTATE_FILES :
		ofn.lpstrFilter   = L"savestate files (*.sta)\0*.sta;\0All files (*.*)\0*.*\0";
		ofn.lpstrDefExt   = L"sta";
		dir_option_value = emu_opts.dir_get_value(DIRPATH_STATE_DIRECTORY);
		break;
	case FILETYPE_WAVE_FILES :
		ofn.lpstrFilter   = L"sounds (*.wav)\0*.wav;\0All files (*.*)\0*.*\0";
		ofn.lpstrDefExt   = L"wav";
		dir_option_value = emu_opts.dir_get_value(DIRPATH_SNAPSHOT_DIRECTORY);
		break;
	case FILETYPE_MNG_FILES :
		ofn.lpstrFilter   = L"videos (*.mng)\0*.mng;\0All files (*.*)\0*.*\0";
		ofn.lpstrDefExt   = L"mng";
		dir_option_value = emu_opts.dir_get_value(DIRPATH_SNAPSHOT_DIRECTORY);
		break;
	case FILETYPE_AVI_FILES :
		ofn.lpstrFilter   = L"videos (*.avi)\0*.avi;\0All files (*.*)\0*.*\0";
		ofn.lpstrDefExt   = L"avi";
		dir_option_value = emu_opts.dir_get_value(DIRPATH_SNAPSHOT_DIRECTORY);
		break;
	case FILETYPE_EFFECT_FILES :
		ofn.lpstrFilter   = L"effects (*.png)\0*.png;\0All files (*.*)\0*.*\0";
		ofn.lpstrDefExt   = L"png";
		dir_option_value = emu_opts.dir_get_value(DIRPATH_ARTPATH);
		break;
	case FILETYPE_SHADER_FILES :
		ofn.lpstrFilter   = L"shaders (*.vsh)\0*.vsh;\0";
		ofn.lpstrDefExt   = L"vsh";
		dir_option_value = emu_opts.dir_get_value(DIRPATH_HLSLPATH); // + PATH_SEPARATOR + "hlsl";
//      ofn.lpstrTitle  = _T("Select a HLSL shader file");
		break;
	case FILETYPE_BGFX_FILES :
		ofn.lpstrFilter   = L"bgfx (*.json)\0*.json;\0All files (*.*)\0*.*\0";
		ofn.lpstrDefExt   = L"json";
		dir_option_value = (std::filesystem::path(emu_opts.dir_get_value(DIRPATH_BGFX_PATH)) / "chains").string();
		break;
	case FILETYPE_LUASCRIPT_FILES :
		ofn.lpstrFilter   = L"scripts (*.lua)\0*.lua;\0All files (*.*)\0*.*\0";
		ofn.lpstrDefExt   = L"lua";
		dir_option_value = ".";
		break;
	default:
		return false;
	}

	// Only want first directory
	size_t i = dir_option_value.find(";");
	if (i != std::string::npos)
		dir_option_value.resize(i);

	if (dir_option_value.empty())
		dir_option_value = ".";

	std::filesystem::path dir_path = dir_option_value;
	std::wstring initial_dir = dir_path.wstring();

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hMain;
	ofn.hInstance = nullptr;
	ofn.lpstrCustomFilter = nullptr;
	ofn.nMaxCustFilter    = 0;
	ofn.nFilterIndex      = 1;
	ofn.lpstrFile         = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle    = nullptr;
	ofn.nMaxFileTitle     = 0;
	ofn.lpstrInitialDir   = initial_dir.c_str();
	ofn.lpstrTitle        = nullptr;
	ofn.Flags             = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	ofn.nFileOffset       = 0;
	ofn.nFileExtension    = 0;
	ofn.lCustData         = 0;
	ofn.lpfnHook          = nullptr;
	ofn.lpTemplateName    = nullptr;

	bool success = cfd(&ofn);
//  if (success)
//  {
		//std::wcout << "Got filename " << filename << " nFileExtension " << ofn.nFileExtension << "\n";
		//GetDirectory(filename,last_directory,(last_directory));
//  }

	return success;
}


void SetStatusBarText(int part_index, const char *message)
{
	if (!message)
		message = "";

	std::unique_ptr<wchar_t[]> w_message(mui_utf16_from_utf8cstring(message));

	(void)windows::send_message(hStatusBar, SB_SETTEXTW, MAKEWPARAM(part_index,nullptr), (LPARAM)w_message.get());
}


void SetStatusBarTextF(int part_index, const char *fmt, ...)
{
	va_list args;
	va_list args_copy;

	va_start(args, fmt);
	va_copy(args_copy, args);

	const int len = vsnprintf(nullptr, 0, fmt, args_copy);

	va_end(args_copy);

	if (len < 0)
	{
		va_end(args);
		return;
	}

	std::vector<char> buffer(len + 1);
	vsnprintf(buffer.data(), buffer.size(), fmt, args);

	va_end(args);

	SetStatusBarText(part_index, buffer.data());
}


static void MameMessageBox(const char *fmt, ...)
{
	va_list args;
	va_list args_copy;

	va_start(args, fmt);
	va_copy(args_copy, args);

	const int len = vsnprintf(nullptr, 0, fmt, args_copy);

	va_end(args_copy);

	if (len < 0)
	{
		va_end(args);
		return;
	}

	std::vector<char> buffer(len + 1);
	vsnprintf(buffer.data(), buffer.size(), fmt, args);

	va_end(args);

	std::string utf8_mameuiname = mui_utf8_from_utf16string(MAMEUINAME);
	dialog_boxes::message_box_utf8(GetMainWindow(), buffer.data(), utf8_mameuiname.c_str(), MB_OK | MB_ICONERROR);
}

#if 0
static void MameMessageBoxW(const wchar_t *fmt, ...)
{
	va_list args;
	va_list args_copy;

	va_start(args, fmt);
	va_copy(args_copy, args);

#if defined(_WIN32)
	const int len = _vscwprintf(fmt, args_copy);
#else
	std::cout << "Using vswprintf" << "\n";
	const int len = vswprintf(nullptr, 0, fmt, args_copy);
#endif

	va_end(args_copy);

	if (len < 0)
	{
		va_end(args);
		return;
	}

	std::vector<wchar_t> buffer(static_cast<size_t>(len) + 1);

#if defined(_WIN32)
	vswprintf_s(buffer.data(), buffer.size(), fmt, args);
#else
	vswprintf(buffer.data(), buffer.size(), fmt, args);
#endif

	va_end(args);

	dialog_boxes::message_box(GetMainWindow(), buffer.data(), &MAMEUINAME[0], MB_OK | MB_ICONERROR);
}
#endif // disabled

static void MamePlayGameWithOptions(int nGame, std::shared_ptr<play_options> playopts)
{
	m_lock = true;
	if (g_pJoyGUI)
		KillTimer(hMain, JOYGUI_TIMER);

	if (GetCycleScreenshot() > 0)
		KillTimer(hMain, SCREENSHOT_TIMER);

	in_emulation = true;

	DWORD dwExitCode = RunMAME(nGame, std::move(playopts));
	if (dwExitCode == 0)
	{
		IncrementPlayCount(nGame);
		(void)list_view::redraw_items(hwndList, GetSelectedPick(), GetSelectedPick());

		// re-sort if sorting on # of times played
		if (GetSortColumn() == COLUMN_PLAYED)
			Picker_Sort(hwndList);

		UpdateStatusBar();

	}

	(void)windows::show_window(hMain, SW_SHOW);
	(void)input::set_focus(hwndList);

	in_emulation = false;

	if (g_pJoyGUI)
		SetTimer(hMain, JOYGUI_TIMER, JOYGUI_MS, nullptr);

	if (GetCycleScreenshot() > 0)
		SetTimer(hMain, SCREENSHOT_TIMER, GetCycleScreenshot()*1000, nullptr); //scale to seconds
}


static void MameUI_LoadState()
{
	int driver_index = Picker_GetSelectedItem(hwndList);
	if (driver_index < 0)
		return;

	std::string utf8_filename = std::string(driver_list::driver(driver_index).name) + ".sta";
	std::filesystem::path file_path = emu_opts.GetGlobalOpts().state_directory();
	file_path /= utf8_filename;

	// Convert to UTF-16 for the file dialog
	std::wstring utf16_file_path = file_path.wstring();
	std::vector<wchar_t> buffer(MAX_PATH);

	(void)mui_wcscpy(buffer.data(), utf16_file_path.c_str());

	if (!CommonFileDialog(dialog_boxes::get_open_filename, buffer.data(), FILETYPE_SAVESTATE_FILES))
		return;

	file_path = buffer.data();
	utf8_filename = file_path.filename().string();

	emu_file pSaveState(emu_opts.GetGlobalOpts().state_directory(), OPEN_FLAG_READ);
	std::error_condition file_error = pSaveState.open(utf8_filename);
	if (!file_error)
	{
		MameMessageBox("Could not open '%s' as a valid savestate file.", utf8_filename.c_str());
		return;
	}

	// call the MAME core function to check the save state file
	//int rc = state_manager::check_file(nullptr, pSaveState, file_path.wstring(), MameMessageBox);
	//if (rc)

	std::string filename_stem = file_path.stem().string();
	if (filename_stem.size() > 6)
		filename_stem.resize(filename_stem.size() - 6); // remove the " State" appened at the end of the filename

	for (driver_index = 0; driver_index < driver_list::total(); driver_index++)
	{
		std::string driver_fullname = driver_list::driver(driver_index).type.fullname();

		//std::cout << "Checking driver " << driver_fullname << " against " << filename_stem << "\n";
		if (mui_strcmp(driver_fullname, filename_stem) == 0)
		{
			std::shared_ptr<play_options> playopts = std::make_shared<play_options>();
			playopts->state = std::move(filename_stem);
			playopts_apply = 0x57;
			MamePlayGameWithOptions(driver_index, std::move(playopts));
			return;
		}
	}

	MameMessageBox("Game \"%s\" cannot be found", filename_stem.c_str());

}



static void MameUI_PlayBackInput()
{
	int driver_index = Picker_GetSelectedItem(hwndList);
	if (driver_index < 0)
		return;

	std::string utf8_filename = std::string(driver_list::driver(driver_index).name) + ".inp";

//  std::vector<wchar_t> buffer(MAX_PATH);
//  utf16_filename = mui_utf16_fromutf8string(utf8_filename);
//  mui_wcscpy(buffer.data(), utf16_filename.c_str());
//  if (!CommonFileDialog(dialog_boxes::get_open_filename, buffer.data(), FILETYPE_INPUT_FILES))
//      return;

//  std::filesystem::path file_path = buffer.data();
//  utf8_filename = file_path.filename().string();

	emu_file pPlayBack(emu_opts.GetGlobalOpts().input_directory(), OPEN_FLAG_READ);
	std::error_condition file_error = pPlayBack.open(utf8_filename);
	if (file_error)
	{
		MameMessageBox("Could not open '%s' as a valid input file.", utf8_filename.c_str());
		return;
	}

	// check for game name embedded in .inp header
	inp_header header;

	// read the header and verify that it is a modern version; if not, print an error
	if (!header.read(pPlayBack))
	{
		MameMessageBox("Input file is corrupt or invalid (missing header)");
		return;
	}
	if ((!header.check_magic()) || (header.get_majversion() != inp_header::MAJVERSION))
	{
		MameMessageBox("Input file invalid or in an older, unsupported format");
		return;
	}

	std::string const sysname = header.get_sysname();
	for (driver_index = 0; driver_index < driver_list::total(); driver_index++) // find game and play it
	{
		const char* driver_name = driver_list::driver(driver_index).name;
		if (driver_name == sysname)
		{
			std::shared_ptr<play_options> playopts = std::make_shared<play_options>();
			playopts->playback = std::move(utf8_filename);
			playopts_apply = 0x57;
			MamePlayGameWithOptions(driver_index, std::move(playopts));
			return;
		}
	}

	MameMessageBox("Game \"%s\" cannot be found", sysname.c_str());
}

static void MameUI_RecordInput()
{
	int driver_index = Picker_GetSelectedItem(hwndList);
	if (driver_index < 0)
		return;

	std::string utf8_filename = std::string(driver_list::driver(driver_index).name) + ".inp";

	std::shared_ptr<play_options> playopts = std::make_shared<play_options>();
	playopts->record = std::move(utf8_filename);
	playopts_apply = 0x57;
	MamePlayGameWithOptions(driver_index, std::move(playopts));
}


void MamePlayGame(void)
{
	int nGame = Picker_GetSelectedItem(hwndList);

	if (m_lock)
		return;

	if (nGame != -1)
	{
		std::shared_ptr<play_options> playopts;

		playopts = std::make_shared<play_options>();
		MamePlayGameWithOptions(nGame, std::move(playopts));
	}
}


static void MamePlayRecordWave()
{
	int driver_index = Picker_GetSelectedItem(hwndList);
	if (driver_index < 0)
		return;

	std::string driver_name = driver_list::driver(driver_index).name;
	std::filesystem::path file_path = std::filesystem::path(emu_opts.GetGlobalOpts().media_path()) / (driver_name + ".wav");

	// convert to UTF-16 for file dialog
	std::wstring utf16_path = file_path.wstring();
	std::vector<wchar_t> filename_buffer(MAX_PATH);
	mui_wcscpy(filename_buffer.data(), utf16_path.c_str());

	if (CommonFileDialog(dialog_boxes::get_save_filename, filename_buffer.data(), FILETYPE_WAVE_FILES))
	{
		// Instead of creating a new path, just overwrite
		file_path = filename_buffer.data();
		file_path.replace_extension(".wav");

		auto playopts = std::make_shared<play_options>();
		playopts->wavwrite = file_path.u8string(); // reuse file_path
		playopts_apply = 0x57;
		MamePlayGameWithOptions(driver_index, std::move(playopts));
	}
}


static void MamePlayRecordMNG()
{
	int driver_index = Picker_GetSelectedItem(hwndList);
	if (driver_index < 0)
		return;

	std::string driver_name = driver_list::driver(driver_index).name;
	std::filesystem::path file_path = std::filesystem::path(emu_opts.GetGlobalOpts().media_path()) / (driver_name + ".mng");

	// convert to UTF-16 for file dialog
	std::wstring utf16_path = file_path.wstring();
	std::vector<wchar_t> filename_buffer(MAX_PATH);
	mui_wcscpy(filename_buffer.data(), utf16_path.c_str());

	if (CommonFileDialog(dialog_boxes::get_save_filename, filename_buffer.data(), FILETYPE_MNG_FILES))
	{
		// Instead of creating a new path, just overwrite
		file_path = filename_buffer.data();
		file_path.replace_extension(".mng");

		auto playopts = std::make_shared<play_options>();
		playopts->mngwrite = file_path.u8string(); // reuse file_path
		playopts_apply = 0x57;
		MamePlayGameWithOptions(driver_index, std::move(playopts));
	}
}


static void MamePlayRecordAVI()
{
	int driver_index = Picker_GetSelectedItem(hwndList);
	if (driver_index < 0)
		return;

	std::string driver_name = driver_list::driver(driver_index).name;
	std::filesystem::path file_path = std::filesystem::path(emu_opts.GetGlobalOpts().media_path()) / (driver_name + ".avi");

	// convert to UTF-16 for file dialog
	std::wstring utf16_path = file_path.wstring();
	std::vector<wchar_t> filename_buffer(MAX_PATH);
	mui_wcscpy(filename_buffer.data(), utf16_path.c_str());

	if (CommonFileDialog(dialog_boxes::get_save_filename, filename_buffer.data(), FILETYPE_AVI_FILES))
	{
		// Instead of creating a new path, just overwrite
		file_path = filename_buffer.data();
		file_path.replace_extension(".avi");

		auto playopts = std::make_shared<play_options>();
		playopts->aviwrite = file_path.u8string(); // reuse file_path
		playopts_apply = 0x57;
		MamePlayGameWithOptions(driver_index, std::move(playopts));
	}
}


// Toggle ScreenShot ON/OFF
static void ToggleScreenShot(void)
{
	UINT val = GetWindowPanes() ^ 8;
	bool show = is_flag_set(val, 3);
	SetWindowPanes(val);
	UpdateScreenShot();

	// Redraw list view
	if (hbBackground && show)
		gdi::invalidate_rect(hwndList, nullptr, false);
}

static void ToggleSoftware(void)
{
	UINT val = GetWindowPanes() ^ 4;
	bool show = is_flag_set(val, 2);
	SetWindowPanes(val);
	UpdateSoftware();

	// Redraw list view
	if (hbBackground && show)
		gdi::invalidate_rect(hwndList, nullptr, false);
}

static void AdjustMetrics(void)
{
	std::cout << "Adjust Metrics" << "\n";
	// WM_SETTINGCHANGE also
	int xtraX  = windows::get_system_metrics(SM_CXFIXEDFRAME); // Dialog frame width
	int xtraY  = windows::get_system_metrics(SM_CYFIXEDFRAME); // Dialog frame height
	xtraY += windows::get_system_metrics(SM_CYMENUSIZE);       // Menu height
	xtraY += windows::get_system_metrics(SM_CYCAPTION);        // Caption Height
	int maxX   = windows::get_system_metrics(SM_CXSCREEN);     // Screen Width
	int maxY   = windows::get_system_metrics(SM_CYSCREEN);     // Screen Height

	TEXTMETRIC tm;
	HDC hDC = gdi::get_dc(hMain);
	(void)gdi::get_text_metrics(hDC, &tm);

	// Convert MIN Width/Height from Dialog Box Units to pixels.
	MIN_WIDTH  = (int)((tm.tmAveCharWidth / 4.0) * DBU_MIN_WIDTH)  + xtraX;
	MIN_HEIGHT = (int)((tm.tmHeight / 8.0) * DBU_MIN_HEIGHT) + xtraY;
	(void)gdi::release_dc(hMain, hDC);

	COLORREF textColor;
	if ((textColor = GetListFontColor()) == RGB(255, 255, 255))
		textColor = RGB(240, 240, 240);

	HWND hWnd = windows::get_window(hMain, GW_CHILD);
	while(hWnd)
	{
		wchar_t szClass[256];

		if (windows::get_classname(hWnd, szClass, std::size(szClass)))
		{
			if (!mui_wcscmp(szClass, L"SysListView32"))
			{
				(void)list_view::set_bk_color(hWnd, windows::get_sys_color(COLOR_WINDOW));
				(void)list_view::set_text_color(hWnd, textColor);
			}
			else if (!mui_wcscmp(szClass, L"SysTreeView32"))
			{
				(void)tree_view::set_bk_color(hTreeView, windows::get_sys_color(COLOR_WINDOW));
				(void)tree_view::set_text_color(hTreeView, textColor);
			}
		}
		hWnd = windows::get_window(hWnd, GW_HWNDNEXT);
	}

	AREA area;
	GetWindowArea(&area); // read window size from ini

	// Reposition the window so that the top or left side is in view.
	// The width and height never change, even if they stretch off the screen.
	if (area.x < 0)
		area.x = 0;
	if (area.y < 0)
		area.y = 0;

	// If the width or height is too small, or bigger than the screen, default them to the max screen size.
	if ((area.width < 200) || (area.width > maxX))
		area.width = maxX;
	if ((area.height < 100) || (area.height > maxY))
		area.height = maxY;

	SetWindowArea(&area);
	windows::set_window_pos(hMain, 0, area.x, area.y, area.width, area.height, SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOACTIVATE);
	std::cout << "Adjust Metrics: Finished" << "\n";
}


int FindIconIndex(int nIconResource)
{
	for(size_t i = 0; !g_iconData[i].icon_name.empty(); i++)
	{
		if (g_iconData[i].resource == nIconResource)
			return i;
	}
	return -1;
}

// not used
int FindIconIndexByName(const char *icon_name)
{
	for (size_t i = 0; !g_iconData[i].icon_name.empty(); i++)
	{
		if (!mui_strcmp(g_iconData[i].icon_name, icon_name))
			return i;
	}
	return -1;
}


static int GetIconForDriver(int item_index)
{
	int iconRoms = 1;

	if (DriverUsesRoms(item_index))
	{
		int audit_result = GetRomAuditResults(item_index);
		if (audit_result == -1)
			return 2;
		else
		if (IsAuditResultYes(audit_result))
			iconRoms = 1;
		else
			iconRoms = 0;
	}

	// iconRoms is now either 0 (no roms), 1 (roms), or 2 (unknown)

	// these are indices into icon_names, which maps into our image list
	// also must match IDI_WIN_NOROMS + iconRoms

	if (iconRoms == 1)
	{
		// Show Red-X if the ROMs are present and flagged as NOT WORKING
		if (DriverIsBroken(item_index))
			iconRoms = FindIconIndex(IDI_WIN_REDX);  // iconRoms now = 4
		else
		// Show imperfect if the ROMs are present and flagged as imperfect
		if (DriverIsImperfect(item_index))
			iconRoms = FindIconIndex(IDI_WIN_IMPERFECT); // iconRoms now = 5
		else
		// show clone icon if we have roms and game is working
		if (DriverIsClone(item_index))
			iconRoms = FindIconIndex(IDI_WIN_CLONE); // iconRoms now = 3
	}

	// if we have the roms, then look for a custom per-game icon to override
	// not 2, because this indicates F5 must be done; not 0, because this indicates roms are missing; only use 4 if user chooses it
	bool redx = GetOverrideRedX() & (iconRoms == 4);
	if (iconRoms == 1 || iconRoms == 3 || iconRoms == 5 || redx)
	{
		if (icon_index[item_index] == 0)
			AddDriverIcon(item_index,iconRoms);
		iconRoms = icon_index[item_index];
	}

	return iconRoms;
}


static bool HandleTreeContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	TVHITTESTINFO hti;
	POINT pt;

	if ((HWND)wParam != dialog_boxes::get_dlg_item(hWnd, IDC_TREE))
		return false;

	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);
	if (pt.x < 0 && pt.y < 0)
		menus::get_cursor_pos(&pt);

	// select the item that was right clicked or shift-F10'ed
	hti.pt = pt;
	(void)gdi::screen_to_client(hTreeView,&hti.pt);
	(void)tree_view::hit_test(hTreeView,&hti);
	if ((hti.flags & TVHT_ONITEM) != 0)
		(void)tree_view::select_item(hTreeView, hti.hItem);

	HMENU hTreeMenu = menus::load_menu(hInst,menus::make_int_resource(IDR_CONTEXT_TREE));

	InitTreeContextMenu(hTreeMenu);

	HMENU hMenu = GetSubMenu(hTreeMenu, 0);

	UpdateMenu(hMenu);

	menus::track_popup_menu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hWnd,nullptr);

	(void)DestroyMenu(hTreeMenu);

	return true;
}


static void GamePicker_OnBodyContextMenu(POINT pt)
{
	HMENU hMenuLoad = menus::load_menu(hInst, menus::make_int_resource(IDR_CONTEXT_MENU));
	HMENU hMenu = GetSubMenu(hMenuLoad, 0);
	InitBodyContextMenu(hMenu);

	UpdateMenu(hMenu);

	menus::track_popup_menu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hMain,nullptr);

	(void)DestroyMenu(hMenuLoad);
}


static bool HandleScreenShotContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	POINT pt;

	if ((HWND)wParam != dialog_boxes::get_dlg_item(hWnd, IDC_SSPICTURE) && (HWND)wParam != dialog_boxes::get_dlg_item(hWnd, IDC_SSFRAME))
		return false;

	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);
	if (pt.x < 0 && pt.y < 0)
		menus::get_cursor_pos(&pt);

	HMENU hMenuLoad = menus::load_menu(hInst, menus::make_int_resource(IDR_CONTEXT_SCREENSHOT));
	HMENU hMenu = GetSubMenu(hMenuLoad, 0);

	UpdateMenu(hMenu);

	menus::track_popup_menu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hWnd,nullptr);

	(void)DestroyMenu(hMenuLoad);

	return true;
}


static void UpdateMenu(HMENU hMenu)
{
	MENUITEMINFOW mItem;
	int nGame = Picker_GetSelectedItem(hwndList);
	if (nGame < 0)
		have_selection = false;

	LPTREEFOLDER lpFolder = GetCurrentFolder();

	if (have_selection)
	{
		std::string description = ConvertAmpersandString(driver_list::driver(nGame).type.fullname());
		std::unique_ptr<wchar_t[]> wcs_description(mui_utf16_from_utf8cstring(description.c_str()));
		if( !wcs_description)
			return;

		std::wstring play_game_string = std::wstring(g_szPlayGameString) + wcs_description.get();

		mItem.cbSize = sizeof(mItem);
		mItem.fMask = MIIM_TYPE;
		mItem.fType = MFT_STRING;
		mItem.dwTypeData = &play_game_string[0];
		mItem.cch = mui_wcslen(mItem.dwTypeData);

		(void)menus::set_menu_item_info(hMenu, ID_FILE_PLAY, false, &mItem);

		(void)menus::enable_menu_item(hMenu, ID_CONTEXT_SELECT_RANDOM, MF_ENABLED);
	}
	else
	{
		(void)menus::enable_menu_item(hMenu, ID_FILE_PLAY, MF_GRAYED);
		(void)menus::enable_menu_item(hMenu, ID_FILE_PLAY_RECORD, MF_GRAYED);
		(void)menus::enable_menu_item(hMenu, ID_GAME_PROPERTIES, MF_GRAYED);
		(void)menus::enable_menu_item(hMenu, ID_CONTEXT_SELECT_RANDOM, MF_GRAYED);
	}

	if (lpFolder->m_dwFlags & F_CUSTOM)
	{
		(void)menus::enable_menu_item(hMenu,ID_CONTEXT_REMOVE_CUSTOM,MF_ENABLED);
		(void)menus::enable_menu_item(hMenu,ID_CONTEXT_RENAME_CUSTOM,MF_ENABLED);
	}
	else
	{
		(void)menus::enable_menu_item(hMenu,ID_CONTEXT_REMOVE_CUSTOM,MF_GRAYED);
		(void)menus::enable_menu_item(hMenu,ID_CONTEXT_RENAME_CUSTOM,MF_GRAYED);
	}
	//const char* pParent = GetFolderNameByID(lpFolder->m_nParent+1);

	if (lpFolder->m_dwFlags & F_INIEDIT)
		(void)menus::enable_menu_item(hMenu,ID_FOLDER_PROPERTIES,MF_ENABLED);
	else
		(void)menus::enable_menu_item(hMenu,ID_FOLDER_PROPERTIES,MF_GRAYED);

	(void)menus::check_menu_radio_item(hMenu, ID_VIEW_TAB_ARTWORK, ID_VIEW_TAB_HISTORY,
		ID_VIEW_TAB_ARTWORK + TabView_GetCurrentTab(hTabCtrl), MF_BYCOMMAND);

	// set whether we're showing the tab control or not
	if (bShowTabCtrl)
		(void)menus::check_menu_item(hMenu,ID_VIEW_PAGETAB,MF_BYCOMMAND | MF_CHECKED);
	else
		(void)menus::check_menu_item(hMenu,ID_VIEW_PAGETAB,MF_BYCOMMAND | MF_UNCHECKED);

	for (size_t i=0;i<MAX_TAB_TYPES;i++)
	{
		// disable menu items for tabs we're not currently showing
		if (GetShowTab(i))
			(void)menus::enable_menu_item(hMenu,ID_VIEW_TAB_ARTWORK + i,MF_BYCOMMAND | MF_ENABLED);
		else
			(void)menus::enable_menu_item(hMenu,ID_VIEW_TAB_ARTWORK + i,MF_BYCOMMAND | MF_GRAYED);

		// check toggle menu items
		if (GetShowTab(i))
			(void)menus::check_menu_item(hMenu, ID_TOGGLE_TAB_ARTWORK + i,MF_BYCOMMAND | MF_CHECKED);
		else
			(void)menus::check_menu_item(hMenu, ID_TOGGLE_TAB_ARTWORK + i,MF_BYCOMMAND | MF_UNCHECKED);
	}

	for (size_t i=0;i<MAX_FOLDERS;i++)
	{
		if (GetShowFolder(i))
			(void)menus::check_menu_item(hMenu,ID_CONTEXT_SHOW_FOLDER_START + i,MF_BYCOMMAND | MF_CHECKED);
		else
			(void)menus::check_menu_item(hMenu,ID_CONTEXT_SHOW_FOLDER_START + i,MF_BYCOMMAND | MF_UNCHECKED);
	}
}


void InitTreeContextMenu(HMENU hTreeMenu)
{
	extern const FOLDERDATA g_folderData[];

	MENUITEMINFOW mii;
	mii = {};
	mii.cbSize = sizeof(mii);
	mii.wID = -1;
	mii.fMask = MIIM_SUBMENU | MIIM_ID;

	HMENU hMenu = GetSubMenu(hTreeMenu, 0);

	if (menus::get_menu_item_info(hMenu,3,TRUE,&mii) == false)
	{
		std::cout << "can't find show folders context menu" << "\n";
		return;
	}

	if (mii.hSubMenu == nullptr)
	{
		std::cout << "can't find submenu for show folders context menu" << "\n";
		return;
	}

	hMenu = mii.hSubMenu;

	for (size_t i=0; !g_folderData[i].m_lpTitle.empty(); i++)
	{
		if (!g_folderData[i].m_process)
		{
			std::wstring utf16_title = mui_utf16_from_utf8string(g_folderData[i].m_lpTitle);
			if( utf16_title.empty())
				return;

			mii.fMask = MIIM_TYPE | MIIM_ID;
			mii.fType = MFT_STRING;
			mii.dwTypeData = const_cast<wchar_t*>(utf16_title.c_str());
			mii.cch = mui_wcslen(mii.dwTypeData);
			mii.wID = ID_CONTEXT_SHOW_FOLDER_START + g_folderData[i].m_nFolderId;

			// menu in resources has one empty item (needed for the submenu to setup properly)
			// so overwrite this one, append after
			if (i == 0)
				(void)menus::set_menu_item_info(hMenu,ID_CONTEXT_SHOW_FOLDER_START,FALSE,&mii);
			else
				(void)menus::insert_menu_item(hMenu,i,FALSE,&mii);
		}
	}
}


void InitBodyContextMenu(HMENU hBodyContextMenu)
{
	int nCurrentGame = Picker_GetSelectedItem(hwndList);
	if (nCurrentGame < 0)
		return;

	MENUITEMINFOW mii;
	mii = {};
	mii.cbSize = sizeof(mii);

	if (menus::get_menu_item_info(hBodyContextMenu,ID_FOLDER_SOURCEPROPERTIES,FALSE,&mii) == false)
	{
		std::cout << "can't find show folders context menu" << "\n";
		return;
	}

	std::wostringstream oss;
	oss << L"Properties for " << GetDriverFilename(nCurrentGame);
	const std::wstring &menuitem_text = oss.str();
	mii.fMask = MIIM_TYPE | MIIM_ID;
	mii.fType = MFT_STRING;
	mii.dwTypeData = const_cast<wchar_t*>(menuitem_text.c_str());
	mii.cch = mui_wcslen(mii.dwTypeData);
	mii.wID = ID_FOLDER_SOURCEPROPERTIES;

	// menu in resources has one default item
	// so overwrite this one
	(void)menus::set_menu_item_info(hBodyContextMenu,ID_FOLDER_SOURCEPROPERTIES,FALSE,&mii);
	(void)menus::enable_menu_item(hBodyContextMenu, ID_FOLDER_VECTORPROPERTIES, DriverIsVector(nCurrentGame) ? MF_ENABLED : MF_GRAYED);
}


void ToggleShowFolder(int folder)
{
	int current_id = GetCurrentFolderID();
	SetWindowRedraw(hwndList, false);
	SetShowFolder(folder,!GetShowFolder(folder));
	ResetTreeViewFolders();
	SelectTreeViewFolder(current_id);
	SetWindowRedraw(hwndList, true);
}


static LRESULT CALLBACK HistoryWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (hbBackground)
	{
		switch (uMsg)
		{
		case WM_MOUSEMOVE:
			{
				if (MouseHasBeenMoved())
					menus::show_cursor(TRUE);
				break;
			}

			case WM_ERASEBKGND:
				return true;
			case WM_PAINT:
			{
				POINT p = { 0, 0 };

				// get base point of background bitmap
				(void)gdi::map_window_points(hWnd,hTreeView,&p,1);
				PaintBackgroundImage(hWnd, nullptr, p.x, p.y);
				// to ensure our parent procedure repaints the whole client area
				gdi::invalidate_rect(hWnd, nullptr, false);
				break;
			}
		}
	}
	return windows::call_window_proc(g_lpHistoryWndProc, hWnd, uMsg, wParam, lParam);
}


static LRESULT CALLBACK PictureFrameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_MOUSEMOVE:
		{
			if (MouseHasBeenMoved())
				menus::show_cursor(TRUE);
			break;
		}

	case WM_NCHITTEST :
		{
			POINT pt;
			RECT  rect;
			HWND hHistory = dialog_boxes::get_dlg_item(hMain, IDC_HISTORY);

			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			(void)windows::get_window_rect(hHistory, &rect);
			// check if they clicked on the picture area (leave 6 pixel no man's land
			// by the history window to reduce mistaken clicks)
			// no more no man's land, the Cursor changes when Edit control is left, should be enough feedback
			if (have_history &&
				( ( (TabView_GetCurrentTab(hTabCtrl) == TAB_HISTORY) ||
					(TabView_GetCurrentTab(hTabCtrl) == GetHistoryTab() && GetShowTab(TAB_HISTORY) == false) ||
					(TAB_ALL == GetHistoryTab() && GetShowTab(TAB_HISTORY) == false) ) &&
//                  (rect.top - 6) < pt.y && pt.y < (rect.bottom + 6) ) )
					gdi::pt_in_rect( &rect, pt ) ) )

			{
				return HTTRANSPARENT;
			}
			else
			{
				return HTCLIENT;
			}
		}
		break;
	case WM_CONTEXTMENU:
		if ( HandleScreenShotContextMenu(hWnd, wParam, lParam))
			return false;
		break;
	}

	if (hbBackground)
	{
		switch (uMsg)
		{
		case WM_ERASEBKGND :
			return true;
		case WM_PAINT :
			{
				RECT rect,nodraw_rect;
				HRGN region,nodraw_region;
				POINT p = { 0, 0 };

				// get base point of background bitmap
				gdi::map_window_points(hWnd,hTreeView,&p,1);

				// get big region
				(void)windows::get_client_rect(hWnd,&rect);
				region = gdi::create_rect_rgn_indirect(&rect);

				if (windows::is_window_visible(dialog_boxes::get_dlg_item(hMain,IDC_HISTORY)))
				{
					// don't draw over this window
					(void)windows::get_window_rect(dialog_boxes::get_dlg_item(hMain,IDC_HISTORY),&nodraw_rect);
					gdi::map_window_points(HWND_DESKTOP,hWnd,(LPPOINT)&nodraw_rect,2);
					nodraw_region = gdi::create_rect_rgn_indirect(&nodraw_rect);
					gdi::combine_rgn(region,region,nodraw_region,RGN_DIFF);
					(void)gdi::delete_object(nodraw_region);
				}

				if (windows::is_window_visible(dialog_boxes::get_dlg_item(hMain,IDC_SSPICTURE)))
				{
					// don't draw over this window
					(void)windows::get_window_rect(dialog_boxes::get_dlg_item(hMain,IDC_SSPICTURE),&nodraw_rect);
					gdi::map_window_points(HWND_DESKTOP,hWnd,(LPPOINT)&nodraw_rect,2);
					nodraw_region = gdi::create_rect_rgn_indirect(&nodraw_rect);
					gdi::combine_rgn(region,region,nodraw_region,RGN_DIFF);
					(void)gdi::delete_object(nodraw_region);
				}

				PaintBackgroundImage(hWnd,region,p.x,p.y);

				(void)gdi::delete_object(region);

				// to ensure our parent procedure repaints the whole client area
				gdi::invalidate_rect(hWnd, nullptr, false);

				break;
			}
		}
	}
	return windows::call_window_proc(g_lpPictureFrameWndProc, hWnd, uMsg, wParam, lParam);
}


static LRESULT CALLBACK PictureWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ERASEBKGND :
		return true;
	case WM_PAINT :
		{
			int nBordersize = GetScreenshotBorderSize();
			HBRUSH hBrush = gdi::create_solid_brush(GetScreenshotBorderColor());
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd,&ps);
			HDC hdc_temp = gdi::create_compatible_dc(hdc);

			HBITMAP old_bitmap;
			int width,height;
			if (ScreenShotLoaded())
			{
				width = GetScreenShotWidth();
				height = GetScreenShotHeight();
				old_bitmap = (HBITMAP)gdi::select_object(hdc_temp,GetScreenShotHandle());
			}
			else
			{
				BITMAP bmp;
				(void)gdi::get_object(hMissing_bitmap,sizeof(BITMAP),&bmp);
				width = bmp.bmWidth;
				height = bmp.bmHeight;
				old_bitmap = (HBITMAP)gdi::select_object(hdc_temp,hMissing_bitmap);
			}

			RECT rect;
			(void)windows::get_client_rect(hWnd,&rect);
			RECT rect2 = rect;
			//Configurable Borders around images
			rect.bottom -= nBordersize;
			if( rect.bottom < 0)
				rect.bottom = rect2.bottom;
			rect.right -= nBordersize;
			if( rect.right < 0)
				rect.right = rect2.right;
			rect.top += nBordersize;
			if( rect.top > rect.bottom )
				rect.top = rect2.top;
			rect.left += nBordersize;
			if( rect.left > rect.right )
				rect.left = rect2.left;
			HRGN region1 = gdi::create_rect_rgn_indirect(&rect);
			HRGN region2 = gdi::create_rect_rgn_indirect(&rect2);
			gdi::combine_rgn(region2,region2,region1,RGN_DIFF);
			HBRUSH holdBrush = (HBRUSH)gdi::select_object(hdc, hBrush);
			gdi::fill_rgn(hdc,region2, hBrush );
			(void)gdi::select_object(hdc, holdBrush);
			(void)gdi::delete_brush(hBrush);
			SetStretchBltMode(hdc,STRETCH_HALFTONE);
			gdi::stretch_blt(hdc,nBordersize,nBordersize,rect.right-rect.left,rect.bottom-rect.top,hdc_temp,0,0,width,height,SRCCOPY);
			(void)gdi::select_object(hdc_temp,old_bitmap);
			(void)gdi::delete_dc(hdc_temp);
			(void)gdi::delete_object(region1);
			(void)gdi::delete_object(region2);
			(void)EndPaint(hWnd,&ps);
			return true;
		}
	}

	return windows::call_window_proc(g_lpPictureWndProc, hWnd, uMsg, wParam, lParam);
}


static void RemoveCurrentGameCustomFolder(void)
{
	int nCurrentGame = Picker_GetSelectedItem(hwndList);
	if (nCurrentGame >= 0)
		RemoveGameCustomFolder(nCurrentGame);
}


static void RemoveGameCustomFolder(int driver_index)
{
	TREEFOLDER **folders;
	int num_folders = 0;

	GetFolders(&folders,&num_folders);

	for (size_t i=0;i<num_folders;i++)
	{
		if (folders[i]->m_dwFlags & F_CUSTOM && folders[i]->m_nFolderId == GetCurrentFolderID())
		{
			int current_pick_index;

			RemoveFromCustomFolder(folders[i],driver_index);

			if (driver_index == Picker_GetSelectedItem(hwndList))
			{
			// if we just removed the current game,
			// move the current selection so that when we rebuild the listview it
			// leaves the cursor on next or previous one
				current_pick_index = GetSelectedPick();
				Picker_SetSelectedPick(hwndList, GetSelectedPick() + 1);
				if (current_pick_index == GetSelectedPick()) // we must have deleted the last item
					Picker_SetSelectedPick(hwndList, GetSelectedPick() - 1);
			}

			ResetListView();
			return;
		}
	}
	dialog_boxes::message_box(GetMainWindow(), L"Error searching for custom folder", &MAMEUINAME[0], MB_OK | MB_ICONERROR);
}


static void BeginListViewDrag(NM_LISTVIEW *pnmv)
{
	LVITEMW lvi;
	POINT pt;

	lvi.iItem = pnmv->iItem;
	lvi.mask = LVIF_PARAM;
	(void)list_view::get_item(hwndList, &lvi);

	game_dragged = lvi.lParam;

	pt.x = 0;
	pt.y = 0;

	// Tell the list view control to create an image to use for dragging.
	himl_drag = list_view::create_drag_image(hwndList,pnmv->iItem,&pt);

	// Start the drag operation.
	ImageList_BeginDrag(himl_drag, 0, 0, 0);

	pt = pnmv->ptAction;
	(void)gdi::client_to_screen(hwndList,&pt);
	ImageList_DragEnter(windows::get_desktop_window(),pt.x,pt.y);

	// Hide the mouse cursor, and direct mouse input to the parent window.
	input::set_capture(hMain);

	prev_drag_drop_target = nullptr;

	g_listview_dragging = true;
}


static void MouseMoveListViewDrag(POINTS p)
{
	HTREEITEM htiTarget;
	TV_HITTESTINFO tvht;
//  bool res = false;

	POINT pt;
	pt.x = p.x;
	pt.y = p.y;

	(void)gdi::client_to_screen(hMain,&pt);

	ImageList_DragMove(pt.x,pt.y);

	gdi::map_window_points(windows::get_desktop_window(),hTreeView,&pt,1);

	tvht.pt = pt;
	htiTarget = tree_view::hit_test(hTreeView,&tvht);

	if (htiTarget != prev_drag_drop_target)
	{
		ImageList_DragShowNolock(FALSE);
		if (htiTarget != nullptr)
			(void)tree_view::select_drop_target(hTreeView, htiTarget);
		else
			(void)tree_view::select_drop_target(hTreeView, nullptr);
		ImageList_DragShowNolock(TRUE);

		prev_drag_drop_target = htiTarget;
	}
}


static void ButtonUpListViewDrag(POINTS p)
{
	POINT pt;
	HTREEITEM htiTarget;
	TV_HITTESTINFO tvht;
	TVITEM tvi;

	input::release_capture();

	ImageList_DragLeave(hwndList);
	ImageList_EndDrag();
	ImageList_Destroy(himl_drag);

	(void)tree_view::select_drop_target(hTreeView, nullptr);

	g_listview_dragging = false;

	// see where the game was dragged

	pt.x = p.x;
	pt.y = p.y;

	gdi::map_window_points(hMain,hTreeView,&pt,1);

	tvht.pt = pt;
	htiTarget = tree_view::hit_test(hTreeView,&tvht);
	if (htiTarget == nullptr)
	{
		LVHITTESTINFO lvhtti;
		LPTREEFOLDER folder;
		RECT rcList;

		// the user dragged a game onto something other than the treeview
		// try to remove if we're in a custom folder

		// see if it was dragged within the list view; if so, ignore

		gdi::map_window_points(hTreeView,hwndList,&pt,1);
		lvhtti.pt = pt;
		(void)windows::get_window_rect(hwndList, &rcList);
		(void)gdi::client_to_screen(hwndList, &pt);
		if( gdi::pt_in_rect(&rcList, pt) != 0 )
			return;

		folder = GetCurrentFolder();
		if (folder->m_dwFlags & F_CUSTOM)
		{
			// dragged out of a custom folder, so let's remove it
			RemoveCurrentGameCustomFolder();
		}
		return;
	}

	tvi.lParam = 0;
	tvi.mask  = TVIF_PARAM | TVIF_HANDLE;
	tvi.hItem = htiTarget;

	if (tree_view::get_item(hTreeView, &tvi))
	{
		LPTREEFOLDER folder = (LPTREEFOLDER)tvi.lParam;
		AddToCustomFolder(folder,game_dragged);
	}
}


static LPTREEFOLDER GetSelectedFolder(void)
{
	LPTREEFOLDER lpfolder = nullptr;
	HTREEITEM htree = tree_view::get_selection(hTreeView);
	if(htree)
	{
		TVITEM tvi;
		tvi.hItem = htree;
		tvi.mask = TVIF_PARAM;

		if(tree_view::get_item(hTreeView, &tvi))
			lpfolder = reinterpret_cast<LPTREEFOLDER>(tvi.lParam);
	}
	return lpfolder;
}


#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
static HICON GetSelectedFolderIcon(void)
{
	LPTREEFOLDER folder;
	HTREEITEM htree = tree_view::get_selection(hTreeView);
	if (htree)
	{
		TVITEM tvi;
		tvi.hItem = htree;
		tvi.mask = TVIF_PARAM;
		(void)tree_view::get_item(hTreeView,&tvi);
		folder = (LPTREEFOLDER)tvi.lParam;
		HIMAGELIST hSmall_icon;
		//hSmall_icon = tree_view::get_image_list(hTreeView,(int)tvi.iImage);
		hSmall_icon = nullptr;
		return ImageList_GetIcon(hSmall_icon, tvi.iImage, ILD_TRANSPARENT);
	}
	return nullptr;
}
#pragma GCC diagnostic error "-Wunused-but-set-variable"


// Updates all currently displayed Items in the List with the latest Data*/
void UpdateListView(void)
{
	if ((GetViewMode() == VIEW_GROUPED) || (GetViewMode() == VIEW_DETAILS))
	{
		int nFirstItemIndex = list_view::get_top_index(hwndList),
			nLastItemIndex = nFirstItemIndex + list_view::get_count_per_page(hwndList);

		(void)list_view::redraw_items(hwndList, nFirstItemIndex, nLastItemIndex);
	}
}


static void CalculateBestScreenShotRect(HWND hWnd, RECT* pRect, bool restrict_height)
{
	RECT rect;
	(void)windows::get_client_rect(hWnd, &rect);

	// Get the width and height of the available area
	long rWidth = (rect.right - rect.left),
		rHeight = (rect.bottom - rect.top);

	// Limit the screen shot to max height of 264
	if (restrict_height == true && rHeight > MAX_SCREENSHOT_HEIGHT)
	{
		rect.bottom = rect.top + MAX_SCREENSHOT_HEIGHT;
		rHeight = MAX_SCREENSHOT_HEIGHT;
	}

	// Scale the bitmap to the frame specified by the passed in hwnd
	long destW = 0, destH = 0;
	if (ScreenShotLoaded())
	{
		destW = GetScreenShotWidth();
		destH = GetScreenShotHeight();
	}
	else // Use the bitmap's size if it fits
	{
		BITMAP bmp;
		(void)gdi::get_object(hMissing_bitmap, sizeof(BITMAP), &bmp);
		destW = bmp.bmWidth;
		destH = bmp.bmHeight;
	}

	bool bReduce = false;

	// If the bitmap does NOT fit in the screenshot area
	if ((destW > rWidth - 10 || destH > rHeight - 10) || GetStretchScreenShotLarger())
	{
		double scale = 1.0; // Default scale
		rect.right -= 10;
		rect.bottom -= 10;
		rWidth -= 10;
		rHeight -= 10;
		bReduce = true;

		// Calculate scale based on width, then height, ensuring we don't divide by zero
		if (destW > 0)
			scale = (double)rWidth / destW;

		if (destH > 0)
			scale = std::min<double>(scale, (double)rHeight / destH); // Use the smaller scale

		destW = (long)(destW * scale);
		destH = (long)(destH * scale);

		// If it's still too big, scale again
		if (destW > rWidth || destH > rHeight)
		{
			if (destW > rWidth)
				scale = (double)rWidth / destW;
			else
				scale = (double)rHeight / destH;

			destW = (long)(destW * scale);
			destH = (long)(destH * scale);
		}
	}

	long destX = (rWidth - destW) / 2;
	long destY = (rHeight - destH) / 2;

	// Adjust for borders if needed
	if (bReduce)
	{
		destX += 5;
		destY += 5;
	}

	// Now set the rect
	int nBorder = GetScreenshotBorderSize();
	pRect->left = std::max<long>(destX - nBorder, 2);  // Ensure left border isn't too small
	pRect->right = std::min<long>(rWidth - pRect->left, destX + destW + nBorder); // don't exceed right edge
	pRect->top = std::max<long>(destY - nBorder, 2);   // Ensure top border isn't too small
	pRect->bottom = std::min<long>(rHeight - pRect->top, destY + destH + nBorder); // don't exceed bottom edge
}


/*
  Switches to either fullscreen or normal mode, based on the
  current mode.

  POSSIBLE BUGS:
  Removing the menu might cause problems later if some
  function tries to poll info stored in the menu. Don't
  know if you've done that, but this was the only way I
  knew to remove the menu dynamically.
*/
static void SwitchFullScreenMode(void)
{
	LONG_PTR lpMainStyle;

	if (GetRunFullScreen())
	{
		// Return to normal

		// Restore the menu
		menus::set_menu(hMain, menus::load_menu(hInst,menus::make_int_resource(IDR_UI_MENU)));

		// Refresh the checkmarks
		(void)menus::check_menu_item(hMainMenu, ID_VIEW_FOLDERS, is_flag_set(GetWindowPanes(), window_pane::TREEVIEW_PANE) ? MF_CHECKED : MF_UNCHECKED);
		(void)menus::check_menu_item(hMainMenu, ID_VIEW_TOOLBARS, GetShowToolBar() ? MF_CHECKED : MF_UNCHECKED);
		(void)menus::check_menu_item(hMainMenu, ID_VIEW_STATUS, GetShowStatusBar() ? MF_CHECKED : MF_UNCHECKED);
		(void)menus::check_menu_item(hMainMenu, ID_VIEW_PAGETAB, GetShowTabCtrl() ? MF_CHECKED : MF_UNCHECKED);

		// Add frame to dialog again
		lpMainStyle = windows::get_window_long_ptr(hMain, GWL_STYLE);
		lpMainStyle = lpMainStyle | WS_BORDER;
		(void)windows::set_window_long_ptr(hMain, GWL_STYLE, lpMainStyle);

		// Show the window maximized
		if( GetWindowState() == SW_MAXIMIZE )
		{
			(void)windows::show_window(hMain, SW_NORMAL);
			(void)windows::show_window(hMain, SW_MAXIMIZE);
		}
		else
			(void)windows::show_window(hMain, SW_RESTORE);

		SetRunFullScreen(FALSE);
	}
	else
	{
		// Set to fullscreen

		// Remove menu
		menus::set_menu(hMain,nullptr);

		// Frameless dialog (fake fullscreen)
		lpMainStyle = windows::get_window_long_ptr(hMain, GWL_STYLE);
		lpMainStyle = lpMainStyle & (WS_BORDER ^ 0xffffffff);
		(void)windows::set_window_long_ptr(hMain, GWL_STYLE, lpMainStyle);
		if( IsMaximized(hMain) )
		{
			(void)windows::show_window(hMain, SW_NORMAL);
			SetWindowState( SW_MAXIMIZE );
		}
		(void)windows::show_window(hMain, SW_MAXIMIZE);

		SetRunFullScreen(TRUE);
	}
}


/*
  Checks to see if the mouse has been moved since this func
  was first called (which is at startup). The reason for
  storing the startup coordinates of the mouse is that when
  a window is created it generates WM_MOUSEOVER events, even
  though the user didn't actually move the mouse. So we need
  to know when the WM_MOUSEOVER event is user-triggered.

  POSSIBLE BUGS:
  Gets polled at every WM_MOUSEMOVE so it might cause lag,
  but there's probably another way to code this that's
  way better?

*/
bool MouseHasBeenMoved(void)
{
	static int mouse_x = -1;
	static int mouse_y = -1;
	POINT p;

	menus::get_cursor_pos(&p);

	if (mouse_x == -1) // First time
	{
		mouse_x = p.x;
		mouse_y = p.y;
	}

	return (p.x != mouse_x || p.y != mouse_y);
}


void delete_in_dirs(const stringtokenizer &tokenizer, const std::string &name, const std::string &extension)
{
	for (const auto& token : tokenizer)
	{
		std::filesystem::path path(token);
		path /= name + extension;

		std::cout << "Deleting " << path.string() << "\n";
		// If you actually want to remove:
		// std::filesystem::remove(path);            // for files
		// std::filesystem::remove_all(path);        // for files/directories
	}
}

// End of source file
