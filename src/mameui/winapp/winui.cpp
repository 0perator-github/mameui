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
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>
#include <string>
#include <thread>

// standard windows headers
#include "windows.h"
#include "commdlg.h"
#include "dinput.h"

// MAME headers
#include "mameheaders.h"

// MAMEUI headers
#include "mui_str.h"
#include "mui_wcs.h"
#include "mui_wcsconv.h"

#include "winapi_controls.h"
#include "winapi_dialog_boxes.h"
#include "winapi_gdi.h"
#include "winapi_menus.h"
#include "winapi_shell.h"
#include "winapi_system_services.h"
#include "winapi_windows.h"

#include "bitmask.h"
#include "columnedit.h"
#include "dialogs.h"
#include "dijoystick.h"     // For DIJoystick availability.
#include "directinput.h"
#include "dirwatch.h"
#include "emu_opts.h"
#include "help.h"
#include "history.h"
#include "mui_audit.h"
#include "mui_opts.h"
#include "directories.h"
#include "mui_util.h"
#include "picker.h"
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

using namespace std::literals;

#include "winui.h"

using namespace mameui::winapi;
using namespace mameui::winapi::controls;

#if defined(__GNUC__)
#define ATTR_PRINTF(x,y)        __attribute__((format(printf, x, y)))
#else
#define ATTR_PRINTF(x,y)
#endif

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
#define tree_view::edit_label(w, i) \
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

typedef struct tagLVBKIMAGEA
{
	ULONG ulFlags;
	HBITMAP hbm;
	LPSTR pszImage;
	UINT cchImageMax;
	int xOffsetPercent;
	int yOffsetPercent;
} LVBKIMAGEA, *LPLVBKIMAGEA;

typedef struct tagLVBKIMAGEW
{
	ULONG ulFlags;
	HBITMAP hbm;
	LPWSTR pszImage;
	UINT cchImageMax;
	int xOffsetPercent;
	int yOffsetPercent;
} LVBKIMAGEW, *LPLVBKIMAGEW;

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

//#ifdef MESS // Set naming for MESSUI
//#ifndef PTR64
// std::wstring_view MAMEUINAME = L"MESSUI32"sv;
//#else
//std::wstring_view MAMEUINAME = L"MESSUI"sv;
//#endif
//std::string_view MUI_INI_FILENAME = "MESSUI.ini"sv;
//#else // or for MAMEUI
#ifndef PTR64
std::wstring_view MAMEUINAME(L"MAMEUI32"sv);
#else
std::wstring_view MAMEUINAME(L"MAMEUI"sv);
#endif
std::string_view MUI_INI_FILENAME("mameui.ini"sv);
//#endif

std::string_view SEARCH_PROMPT("<search here>"sv);

extern const ICONDATA g_iconData[];
UINT8 playopts_apply = 0;
static bool m_resized = false;

typedef struct play_options_t play_options;
struct play_options_t
{
	std::string_view record;      // OPTION_RECORD
	std::string_view playback;    // OPTION_PLAYBACK
	std::string_view state;       // OPTION_STATE
	std::string_view wavwrite;    // OPTION_WAVWRITE
	std::string_view mngwrite;    // OPTION_MNGWRITE
	std::string_view aviwrite;    // OPTION_AVIWRITE
};

/***************************************************************************
    External variables
 ***************************************************************************/

/***************************************************************************
    Internal structures
 ***************************************************************************/

// These next two structs represent how the icon information
// is stored in an ICO file.
typedef struct
{
	BYTE    bWidth;               // Width of the image
	BYTE    bHeight;              // Height of the image (times 2)
	BYTE    bColorCount;          // Number of colors in image (0 if >=8bpp)
	BYTE    bReserved;            // Reserved
	WORD    wPlanes;              // Color Planes
	WORD    wBitCount;            // Bits per pixel
	DWORD   dwBytesInRes;         // how many bytes in this resource?
	DWORD   dwImageOffset;        // where in the file is this image
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct
{
	UINT            Width, Height, Colors; // Width, Height and bpp
	LPBYTE          lpBits;                // ptr to DIB bits
	DWORD           dwNumBytes;            // how many bytes?
	LPBITMAPINFO    lpbi;                  // ptr to header
	LPBYTE          lpXOR;                 // ptr to XOR image bits
	LPBYTE          lpAND;                 // ptr to AND image bits
} ICONIMAGE, *LPICONIMAGE;

typedef struct
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
	void        *subwindow; // Points to a Resize structure for this subwindow; NULL if none
} ResizeItem;

typedef struct guisequence_t
{
	char        name[40];    // functionality name (optional)
	input_seq   is;      // the input sequence (the keys pressed)
	UINT        func_id;        // the identifier
	input_seq* (* const getiniptr)(void);  // pointer to function to get the value from .ini file
} GUISequence;

typedef struct
{
	RECT        rect;       // Client rect of window; must be initialized before first resize
	const ResizeItem* items;      // Array of subitems to be resized
} Resize;

typedef struct tagPOPUPSTRING
{
	HMENU hMenu;
	UINT uiString;
} POPUPSTRING;

// Struct needed for Game Window Communication

typedef struct
{
	LPPROCESS_INFORMATION ProcessInfo;
	HWND hwndFound;
} FINDWINDOWHANDLE;

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
std::unique_ptr<const struct osd_joystick_t> g_pJoyGUI;

// store current keyboard state (in bools) here
static bool keyboard_state[4096]; // __code_max #defines the number of internal key_codes

// search
static char g_SearchText[2048];

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
	{ RA_ID,   { ID_TOOLBAR_EDIT }, true,  RA_RIGHT | RA_TOP, NULL },
	{ RA_END,  { 0 },               false, 0,                 NULL }
};

static Resize toolbar_resize = { {0, 0, 0, 0}, toolbar_resize_items };

// How to resize main window
static ResizeItem main_resize_items[] =
{
	{ RA_HWND, { 0 },            false, RA_LEFT | RA_RIGHT | RA_TOP,     &toolbar_resize },
	{ RA_HWND, { 0 },            false, RA_LEFT | RA_RIGHT | RA_BOTTOM,  NULL },
	{ RA_ID,   { IDC_DIVIDER },  false, RA_LEFT | RA_RIGHT | RA_TOP,     NULL },
	{ RA_ID,   { IDC_TREE },     true,  RA_LEFT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_LIST },     true,  RA_ALL,                            NULL },
	{ RA_ID,   { IDC_SPLITTER }, false, RA_LEFT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SPLITTER2 },FALSE, RA_LEFT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SSFRAME },  false, RA_LEFT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SSPICTURE },FALSE, RA_LEFT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_HISTORY },  true,  RA_LEFT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SSTAB },    false, RA_LEFT | RA_TOP,                 NULL },
	//#ifdef MESS
		{ RA_ID,   { IDC_SWLIST },    true, RA_LEFT | RA_BOTTOM | RA_TOP,     NULL },
		{ RA_ID,   { IDC_SOFTLIST },  true, RA_LEFT | RA_BOTTOM | RA_TOP,     NULL },
		//#endif
			{ RA_ID,   { IDC_SPLITTER3 },FALSE, RA_LEFT | RA_BOTTOM | RA_TOP,     NULL },
			{ RA_END,  { 0 },            false, 0,                                 NULL }
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

constexpr auto WM_MAMEUI_FILECHANGED = WM_USER + 0;
constexpr auto WM_MAMEUI_AUDITGAME = WM_USER + 1;
constexpr auto WM_MAMEUI_PLAYGAME = WM_APP + 15000;

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

static HBRUSH hBrush = NULL;
//static HBRUSH hBrushDlg = NULL;
static HDC hDC = NULL;
static HWND hSplash = NULL;
static HWND hProgress = NULL;
static intptr_t CALLBACK StartupProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static bool m_lock = false;    // prevent MAME from being launched twice by accident, and crashing the entire app.

static HWND   hMain  = NULL;
static HMENU  hMainMenu = NULL;
static HACCEL hAccel = NULL;

static HWND hwndList  = NULL;
static HWND hTreeView = NULL;
static HWND hProgWnd  = NULL;
static HWND hTabCtrl  = NULL;

static HINSTANCE hInst = NULL;

static HFONT hFont = NULL;     // Font for list view

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
static WNDPROC g_lpHistoryWndProc = NULL;
static WNDPROC g_lpPictureFrameWndProc = NULL;
static WNDPROC g_lpPictureWndProc = NULL;

static POPUPSTRING popstr[MAX_MENUS + 1];

// Tool and Status bar variables
static HWND hStatusBar = 0;
static HWND s_hToolBar = 0;

// Used to recalculate the main window layout
static int  bottomMargin = 0;
static int  topMargin = 0;
static bool  have_history = false;

static bool have_selection = false;

static HBITMAP hMissing_bitmap = NULL;

// Icon variables
static HIMAGELIST   hLarge = NULL;
static HIMAGELIST   hSmall = NULL;
static HIMAGELIST   hHeaderImages = NULL;
static std::unique_ptr<int[]> icon_index; // for custom per-game icons

static bool g_listview_dragging = false;
static HIMAGELIST himl_drag;
static int game_dragged; // which game started the drag
static HTREEITEM prev_drag_drop_target = NULL; // which tree view item we're currently highlighting

static bool g_in_treeview_edit = false;

/***************************************************************************
    Global variables
 ***************************************************************************/

// Background Image handles also accessed from TreeView.cpp
static HPALETTE         hpBackground   = NULL;
static HBITMAP          hbBackground  = NULL;
static MYBITMAPINFO     bmDesc;

// List view Column text
extern const LPCWSTR column_names[COLUMN_MAX] =
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
	L"ROMs",
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
static bool TreeViewNotify(NMHDR* nm);

//static void ResetBackground(char *szFile);
static void LoadBackgroundBitmap(void);
static void PaintBackgroundImage(HWND hWnd, HRGN hRgn, int x, int y);

static void DisableSelection(void);
static void EnableSelection(int nGame);

static HICON GetSelectedPickItemIcon(void);
static void SetRandomPickItem(void);
static void PickColor(COLORREF* cDefault);

static LPTREEFOLDER GetSelectedFolder(void);
static HICON GetSelectedFolderIcon(void);
static void RemoveCurrentGameCustomFolder(void);
static void RemoveGameCustomFolder(int driver_index);

static LRESULT CALLBACK HistoryWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK PictureFrameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK PictureWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static void MamePlayRecordGame(void);
static void MamePlayBackGame(void);
static void MamePlayRecordWave(void);
static void MamePlayRecordMNG(void);
static void MamePlayRecordAVI(void);
static void MameLoadState(void);
static bool GameCheck(void);
static bool FolderCheck(void);

static void ToggleScreenShot(void);
static void ToggleSoftware(void);
static void AdjustMetrics(void);

// Icon routines
static DWORD GetShellLargeIconSize(void);
static DWORD GetShellSmallIconSize(void);
static void CreateIcons(void);
static int GetIconForDriver(int nItem);
static void AddDriverIcon(int nItem, int default_icon_index);

// Context Menu handlers
static void UpdateMenu(HMENU hMenu);
static void InitTreeContextMenu(HMENU hTreeMenu);
static void InitBodyContextMenu(HMENU hBodyContextMenu);
static void ToggleShowFolder(int folder);
static bool HandleTreeContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam);
static bool HandleScreenShotContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam);

static int GamePicker_Compare(HWND hwndPicker, int index1, int index2, int sort_subitem);
static void GamePicker_OnHeaderContextMenu(POINT pt, int nColumn);
static void GamePicker_OnBodyContextMenu(POINT pt);

static void InitListView(void);

// Re/initialize the ListView header columns
static void ResetColumnDisplay(bool first_time);

static void BeginListViewDrag(NM_LISTVIEW* pnmv);
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

static void CalculateBestScreenShotRect(HWND hWnd, RECT* pRect, bool restrict_height);

bool MouseHasBeenMoved(void);
static void SwitchFullScreenMode(void);

static void ResizeWindow(HWND hParent, Resize* r);
static void SetAllWindowsFont(HWND hParent, const Resize* r, HFONT hFont, bool bRedraw);

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
						outfile.close();
					});
				state.active = true;
			}
		}
//      else
//      {
//          chain_output(channel, args);
//      }
	}
};

/***************************************************************************
    External functions
 ***************************************************************************/
static DWORD RunMAME(int nGameIndex, std::shared_ptr<play_options> playopts)
{
	std::string_view name;
	double elapsedtime;
	mameui_output_error winerror;
	windows_options global_opts;
	windows_osd_interface osd(global_opts);
	std::string start_finish_message;
	time_t start = 0, end = 0;
	std::unique_ptr<mame_machine_manager> manager;
	std::unique_ptr<std::ostringstream> option_errors;

	m_lock = true;
	// Tell mame where to get the INIs
	SetDirectories(global_opts);

	SetSystemName(nGameIndex);
	name = driver_list::driver(nGameIndex).name;

	// set some startup options
	global_opts.set_value(OPTION_LANGUAGE, GetLanguageUI(), OPTION_PRIORITY_CMDLINE);
	global_opts.set_value(OPTION_PLUGINS, GetEnablePlugins(), OPTION_PRIORITY_CMDLINE);
	global_opts.set_value(OPTION_PLUGIN, GetPlugins(), OPTION_PRIORITY_CMDLINE);
	global_opts.set_value(OPTION_SYSTEMNAME, name, OPTION_PRIORITY_CMDLINE);

	// set any specified play options
	if (playopts_apply == 0x57)
	{
		if (!playopts->record.empty())
			global_opts.set_value(OPTION_RECORD, playopts->record, OPTION_PRIORITY_CMDLINE);
		if (!playopts->playback.empty())
			global_opts.set_value(OPTION_PLAYBACK, playopts->playback, OPTION_PRIORITY_CMDLINE);
		if (!playopts->state.empty())
			global_opts.set_value(OPTION_STATE, playopts->state, OPTION_PRIORITY_CMDLINE);
		if (!playopts->wavwrite.empty())
			global_opts.set_value(OPTION_WAVWRITE, playopts->wavwrite, OPTION_PRIORITY_CMDLINE);
		if (!playopts->mngwrite.empty())
			global_opts.set_value(OPTION_MNGWRITE, playopts->mngwrite, OPTION_PRIORITY_CMDLINE);
		if (!playopts->aviwrite.empty())
			global_opts.set_value(OPTION_AVIWRITE, playopts->aviwrite, OPTION_PRIORITY_CMDLINE);
	}

	// redirect messages to our handler
	start_finish_message = util::string_format("********** STARTING %s **********", name);
	std::cout << start_finish_message << std::endl;
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
	mame_options::parse_standard_inis(global_opts, *option_errors);
	load_translation(global_opts);

	// start processes
	manager->start_http_server();
	manager->start_luaengine();
	time(&start);

	// run the game
	manager->execute();

	// save game time played
	time(&end);

	start_finish_message = util::string_format("********** FINISHED %s **********", name);
	osd_printf_verbose(start_finish_message);
	osd_printf_info(start_finish_message);

	// turn off message redirect
	osd_output::pop(&winerror);
	std::cout << start_finish_message << std::endl;

	// clear any specified play options
	// do it this way to preserve slots and software entries
	if (playopts_apply == 0x57)
	{
		windows_options o;

		load_options(o, OPTIONS_GAME, nGameIndex, 0);
		if (!playopts->record.empty())
			o.set_value(OPTION_RECORD, "", OPTION_PRIORITY_CMDLINE);
		if (!playopts->playback.empty())
			o.set_value(OPTION_PLAYBACK, "", OPTION_PRIORITY_CMDLINE);
		if (!playopts->state.empty())
			o.set_value(OPTION_STATE, "", OPTION_PRIORITY_CMDLINE);
		if (!playopts->wavwrite.empty())
			o.set_value(OPTION_WAVWRITE, "", OPTION_PRIORITY_CMDLINE);
		if (!playopts->mngwrite.empty())
			o.set_value(OPTION_MNGWRITE, "", OPTION_PRIORITY_CMDLINE);
		if (!playopts->aviwrite.empty())
			o.set_value(OPTION_AVIWRITE, "", OPTION_PRIORITY_CMDLINE);
		// apply the above to the ini file
		save_options(o, OPTIONS_GAME, nGameIndex);
	}
	playopts_apply = 0;

	playopts.reset();

	elapsedtime = end - start;
	IncrementPlayTime(nGameIndex, elapsedtime);

	// the emulation is complete; continue
	for (size_t i = 0; i < std::size(s_nPickers); i++)
		Picker_ResetIdle(dialog_boxes::get_dlg_item(hMain, s_nPickers[i]));

	(void)windows::show_window(hMain, SW_SHOW);
	SetForegroundWindow(hMain);

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

	// printf's not allowed before here, else they get into mame queries

	std::cout << "MAMEUI starting" << std::endl;

	hSplash = dialog_boxes::create_dialog(hInstance, menus::make_int_resource(IDD_STARTUP), hMain, StartupProc, 0L);
	SetActiveWindow(hSplash);
	SetForegroundWindow(hSplash);

	bool bResult =MameUI_init(hInstance, lpCmdLine, nCmdShow);
	windows::destroy_window(hSplash);
	if (!bResult)
		return 1;

	// pump message, but quit on WM_QUIT
	while(PumpMessage());

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
	int tmpOrder[COLUMN_MAX] = {0};
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
// RETURNS: HICON - handle to the icon, NULL for failure
// History: July '95 - Created
//          March '00- Seriously butchered from MSDN for mine own
//          purposes, sayeth H0ek.
//----------------------------------------------------------------

static HICON FormatICOInMemoryToHICON(PBYTE ptrBuffer, UINT nBufferSize)
{
	HICON hIcon = NULL;
	ICONIMAGE IconImage;
	LPICONDIRENTRY lpIDE;
	UINT nSizeofWord = sizeof(WORD),
		nBufferIndex = 0,
		nNumImages,
		nIdeBufferSize;

	// Is there a WORD?
	if (nBufferSize < nSizeofWord)
		return NULL;

	// Was it 'reserved' ?   (ie 0)
	if ((WORD)(ptrBuffer[nBufferIndex]) != 0)
		return NULL;

	nBufferIndex += nSizeofWord;

	// Is there a WORD?
	if (nBufferSize - nBufferIndex < nSizeofWord)
		return NULL;

	// Was it type 1?
	if ((WORD)(ptrBuffer[nBufferIndex]) != 1)
		return NULL;

	nBufferIndex += nSizeofWord;

	// Is there a WORD?
	if (nBufferSize - nBufferIndex < nSizeofWord)
		return NULL;

	// Then that's the number of images in the ICO file
	nNumImages = (WORD)(ptrBuffer[nBufferIndex]);

	// Is there at least one icon in the file?
	if ( nNumImages < 1 )
		return NULL;

	nBufferIndex += nSizeofWord;
	nIdeBufferSize = nNumImages * sizeof(ICONDIRENTRY);
	// Is there enough space for the icon directory entries?
	if ((nBufferIndex + nIdeBufferSize) > nBufferSize)
		return NULL;

	// Assign icon directory entries from buffer
	lpIDE = (LPICONDIRENTRY)(&ptrBuffer[nBufferIndex]);
	nBufferIndex += nIdeBufferSize;

	IconImage.dwNumBytes = lpIDE->dwBytesInRes;

	// Seek to beginning of this image
	if ( lpIDE->dwImageOffset > nBufferSize )
		return NULL;

	nBufferIndex = lpIDE->dwImageOffset;

	// Read it in
	if ((nBufferIndex + lpIDE->dwBytesInRes) > nBufferSize)
		return NULL;

	IconImage.lpBits = &ptrBuffer[nBufferIndex];
	nBufferIndex += lpIDE->dwBytesInRes;

	// We would break on NT if we try with a 16bpp image
	if (((LPBITMAPINFO)IconImage.lpBits)->bmiHeader.biBitCount != 16)
		hIcon = menus::create_icon_from_resource_ex(IconImage.lpBits, IconImage.dwNumBytes, true, 0x00030000,0,0,LR_DEFAULTSIZE);

	return hIcon;
}


HICON LoadIconFromFile(const char *iconname)
{
	HICON hIcon = 0;
	std::filesystem::path ui_path(dir_get_value(DIRMAP_ICONS_PATH));
	std::string token;
	std::istringstream tokenStream(ui_path.string());

	while (std::getline(tokenStream, token, ';') && !hIcon)
	{
		std::string icon_filename;
		std::string icon_path;
		util::archive_file::ptr compressed_archive;

		icon_filename = std::string(iconname) + ".ico";
		icon_path = token + "\\" + icon_filename;
		hIcon = shell::extract_icon_utf8(hInst, &icon_path[0], 0);
		if (!hIcon)
		{
			bool result = false;

			icon_path = token + "\\icons.zip";
			result = !util::archive_file::open_zip(icon_path, compressed_archive);
			// no zip file? Then try to open a 7z file.
			if (!result)
			{
				//compressed_archive.reset();
				icon_path = token + "\\icons.7z";
				result = !util::archive_file::open_7z(icon_path, compressed_archive);
			}

			if (result)
			{
				result = compressed_archive->search(icon_filename, false) >= 0;
				if (result)
				{
					const uint64_t uncompressed_length = compressed_archive->current_uncompressed_length();
					std::unique_ptr<BYTE[]> buffer(new BYTE[uncompressed_length]);

					result = !compressed_archive->decompress(buffer.get(), uncompressed_length);
					if (result)
						hIcon = FormatICOInMemoryToHICON(buffer.get(), uncompressed_length);
				}
				compressed_archive.reset();
			}
		}
	}

	return hIcon;
}


// Return the number of folders with options
void SetNumOptionFolders(int count)
{
	optionfolder_count = count;
}


// search
const char* GetSearchText(void)
{
	return g_SearchText;
}


// Sets the treeview and listviews sizes in accordance with their visibility and the splitters
static void ResizeTreeAndListViews(bool bResizeHidden)
{
	AREA area;
	GetWindowArea(&area);
	bool bShowPicture = BIT(GetWindowPanes(), 3);
	bool bShowSoftware = BIT(GetWindowPanes(), 2);
	int nLastWidth = 0;
	int nLeftWindowWidth = 0;

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
	int fullwidth = area.width;

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;
	if (bShowToolBar)
		rect.top += topMargin;

	// Tree control
	(void)windows::show_window(dialog_boxes::get_dlg_item(hMain, IDC_TREE), BIT(GetWindowPanes(), 0) ? SW_SHOW : SW_HIDE);

	for (size_t i = 0; g_splitterInfo[i].nSplitterWindow; i++)
	{
		bool bVisible = windows::get_window_long_ptr(dialog_boxes::get_dlg_item(hMain, g_splitterInfo[i].nLeftWindow), GWL_STYLE) & WS_VISIBLE ? true : false;
		if (bResizeHidden || bVisible)
		{
			nLeftWindowWidth = nSplitterOffset[i] - SPLITTER_WIDTH/2 - nLastWidth;

			// special case for the rightmost pane when the screenshot is gone
			if (!bShowPicture && !bShowSoftware && !g_splitterInfo[i+1].nSplitterWindow)
				//nLeftWindowWidth = rect.right - nLastWidth;
				nLeftWindowWidth = fullwidth - nLastWidth;
			//printf("Sizes: nLastWidth %d, fullwidth %d, nLastWidth + nLeftWindowWidth %d\n",nLastWidth,fullwidth,nLastWidth + nLeftWindowWidth);
			if (nLastWidth > fullwidth)
				nLastWidth = fullwidth - MIN_VIEW_WIDTH;
			if ((nLastWidth + nLeftWindowWidth) > fullwidth)
				nLeftWindowWidth = MIN_VIEW_WIDTH;
			//printf("ResizeTreeAndListViews: Window %d, Left %d, Right %d\n",i,nLastWidth, nLeftWindowWidth + nLastWidth);
			(void)windows::move_window(dialog_boxes::get_dlg_item(hMain, g_splitterInfo[i].nLeftWindow), nLastWidth, rect.top + 2, nLeftWindowWidth, rect.bottom - rect.top - 4, true); // window
			(void)windows::move_window(dialog_boxes::get_dlg_item(hMain, g_splitterInfo[i].nSplitterWindow), nSplitterOffset[i], rect.top + 2, SPLITTER_WIDTH, rect.bottom - rect.top - 4, true); // splitter
		}

		if (bVisible)
		{
			nLastWidth += nLeftWindowWidth + SPLITTER_WIDTH;
		}
	}
}

void UpdateSoftware()
{
	// first time through can't do this stuff
	if (hwndList == NULL)
		return;

	bool bShowSoftware = BIT(GetWindowPanes(), 2);
	bool bShowImage = BIT(GetWindowPanes(), 3);
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

	if (bShowImage)
		UpdateScreenShot();
}

// Adjust the list view and screenshot button based on GetShowScreenShot()
void UpdateScreenShot()
{
	// first time through can't do this stuff
	//printf("Update Screenshot: A\n");fflush(stdout);
	if (hwndList == NULL)
		return;

	// Size the List Control in the Picker
	RECT rect;
	(void)windows::get_client_rect(hMain, &rect);

	//printf("Update Screenshot: B\n");fflush(stdout);
	if (bShowStatusBar)
		rect.bottom -= bottomMargin;
	if (bShowToolBar)
		rect.top += topMargin;

	//printf("Update Screenshot: C\n");fflush(stdout);
	bool bShowImage = BIT(GetWindowPanes(), 3); // ss
	(void)menus::check_menu_item(hMainMenu, ID_VIEW_PICTURE_AREA, bShowImage ? MF_CHECKED : MF_UNCHECKED);
	(void)tool_bar::check_button(s_hToolBar, ID_VIEW_PICTURE_AREA, bShowImage ? MF_CHECKED : MF_UNCHECKED);

	//printf("Update Screenshot: F\n");fflush(stdout);
//	ResizePickerControls(hMain);
	ResizeTreeAndListViews(FALSE);

	//printf("Update Screenshot: G\n");fflush(stdout);
	FreeScreenShot();

	std::string t_software;
	//printf("Update Screenshot: H\n");fflush(stdout);
	if (have_selection)
	{
//#ifdef MESS
		if (!g_szSelectedItem.empty())
		{
			LoadScreenShot(Picker_GetSelectedItem(hwndList), g_szSelectedItem, TabView_GetCurrentTab(hTabCtrl));
		}
		else
//#endif
			LoadScreenShot(Picker_GetSelectedItem(hwndList), std::string(), TabView_GetCurrentTab(hTabCtrl));
	}

	// figure out if we have a history or not, to place our other windows properly
	//printf("Update Screenshot: I\n");fflush(stdout);
	t_software = g_szSelectedItem;
	UpdateHistory(t_software);

	// setup the picture area

	//printf("Update Screenshot: J\n");fflush(stdout);
	if (bShowImage)
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

		gdi::invalidate_rect(dialog_boxes::get_dlg_item(hMain,IDC_SSPICTURE),NULL,FALSE);
	}
	else
	{
		(void)windows::show_window(dialog_boxes::get_dlg_item(hMain,IDC_SSPICTURE),SW_HIDE);
		(void)windows::show_window(dialog_boxes::get_dlg_item(hMain,IDC_SSFRAME),SW_HIDE);
		(void)windows::show_window(dialog_boxes::get_dlg_item(hMain,IDC_SSTAB),SW_HIDE);
	}
	std::cout << "Update Screenshot: Finished" << std::endl;
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
		doSSControls = BIT(GetWindowPanes(), 3);
	}

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;

	if (bShowToolBar)
		rect.top += topMargin;

	(void)windows::move_window(dialog_boxes::get_dlg_item(hWnd, IDC_DIVIDER), rect.left, rect.top - 4, rect.right, 2, true);

	ResizeTreeAndListViews(TRUE);
	int nListWidth = 0;
	if (BIT(GetWindowPanes(), 2)) // sw
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
		sRect.top = rect.top + 264;
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

//static wchar_t *GetMainTitle(void)
//{
//  int iTextLength = GetWindowTextLength(hMain);
//  wchar_t *strWinTitle = new wchar_t[iTextLength] { };
//  GetWindowText(hMain, strWinTitle, iTextLength);
//  return strWinTitle;
//}

static void SetMainTitle(void)
{
	std::wostringstream main_title;
	std::unique_ptr<wchar_t> wcs_version_string(mui_wcstring_from_utf8(GetVersionString()));

	main_title << MAMEUINAME << L" " << wcs_version_string.get();
	windows::set_window_text(hMain, &main_title.str()[0]);
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
			HBITMAP hBmp = (HBITMAP)menus::load_image(system_services::get_module_handle(NULL), menus::make_int_resource(IDB_SPLASH), IMAGE_BITMAP, 0, 0, LR_SHARED);
			(void)windows::send_message(dialog_boxes::get_dlg_item(hDlg, IDC_SPLASH), STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmp);
			hBrush = gdi::get_sys_color_brush(COLOR_3DFACE);
			hProgress = windows::create_window_ex(0, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE, 0, 136, 526, 18, hDlg, NULL, hInst, NULL);
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
	win_set_window_text_utf8(dialog_boxes::get_dlg_item(hSplash, IDC_PROGBAR), "Please wait...");
	(void)windows::send_message(hProgress, PBM_SETPOS, 10, 0);

	extern const FOLDERDATA g_folderData[];
	extern const FILTER_ITEM g_filterList[];
	m_resized = false;

	std::cout << "MameUI_init: About to init options" << std::endl;
	OptionsInit();
	(void)windows::send_message(hProgress, PBM_SETPOS, 25, 0);
	emu_opts_init(0);
	std::cout << "MameUI_init: Options loaded" << std::endl;
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
	wndclass.hCursor       = NULL;
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

	hMain = dialog_boxes::create_dialog(hInstance, menus::make_int_resource(IDD_MAIN), 0, NULL, 0L);
	if (hMain == NULL)
	{
		std::cout << "MameUI_init: Error creating main dialog, aborting" << std::endl;
		return false;
	}

	hMainMenu = GetMenu(hMain);

	s_pWatcher = DirWatcher_Init(hMain, WM_MAMEUI_FILECHANGED);
	if (s_pWatcher)
	{
		auto media_path_ptr = std::make_shared<std::string>(dir_get_value(DIRMAP_MEDIAPATH));
		auto sample_path_ptr = std::make_shared<std::string>(dir_get_value(DIRMAP_SAMPLEPATH));

		DirWatcher_Watch(s_pWatcher, 0, media_path_ptr, true);  // roms
		DirWatcher_Watch(s_pWatcher, 1, sample_path_ptr, true);  // samples
	}

	SetMainTitle();
	hTabCtrl = dialog_boxes::get_dlg_item(hMain, IDC_SSTAB);
	(void)windows::send_message(hProgress, PBM_SETPOS, 70, 0);

	{
		struct TabViewOptions opts;

		static const struct TabViewCallbacks s_tabviewCallbacks =
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
	hMissing_bitmap = LoadBitmap(system_services::get_module_handle(NULL),menus::make_int_resource(IDB_ABOUT));

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

	bShowTree = BIT(GetWindowPanes(), 0);
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
	std::cout << "MameUI_init: About to InitTreeView" << std::endl;
	InitTreeView(g_folderData, g_filterList);
	std::cout << "MameUI_init: Did InitTreeView" << std::endl;
	(void)windows::send_message(hProgress, PBM_SETPOS, 100, 0);

	// Initialize listview columns
//#ifdef MESS
	InitMessPicker(); // messui.cpp
//#endif

	std::cout << "MameUI_init: About to InitListView" << std::endl;
	InitListView();
	(void)SetFocus(hwndList);
	std::cout << "MameUI_init: Did InitListView" << std::endl;
	// Reset the font
	std::cout << "MameUI_init: Reset the font" << std::endl;
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
	std::cout << "MameUI_init: Init directinput" << std::endl;
	if (!DirectInputInitialize())
	{
		(void)dialog_boxes::dialog_box(system_services::get_module_handle(NULL), menus::make_int_resource(IDD_DIRECTX), NULL, DirectXDialogProc, 0L);
		return false;
	}

	std::cout << "MameUI_init: Adjusting window metrics" << std::endl;
	AdjustMetrics();
	UpdateSoftware();
	UpdateScreenShot();

	hAccel = menus::load_accelerators(hInstance, menus::make_int_resource(IDA_TAB_KEYS));

	// clear keyboard state
	std::cout << "MameUI_init: Clearing keyboard state" << std::endl;
	KeyboardStateClear();

	std::cout << "MameUI_init: Init joystick input" << std::endl;
	if (GetJoyGUI() == true)
	{
		g_pJoyGUI = std::make_unique<const struct osd_joystick_t>(DIJoystick);
		if (g_pJoyGUI->init() != 0)
			g_pJoyGUI.reset();
		else
			SetTimer(hMain, JOYGUI_TIMER, JOYGUI_MS, NULL);
	}
	else
		g_pJoyGUI.reset();

	std::cout << "MameUI_init: Centering mouse cursor position" << std::endl;
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

	std::cout << "MameUI_init: About to show window" << std::endl;

	nCmdShow = GetWindowState();
	if (nCmdShow == SW_HIDE || nCmdShow == SW_MINIMIZE || nCmdShow == SW_SHOWMINIMIZED)
		nCmdShow = SW_RESTORE;

	if (GetRunFullScreen())
	{
		LONG lMainStyle;

		// Remove menu
		SetMenu(hMain,NULL);

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
		SetTimer(hMain, SCREENSHOT_TIMER, GetCycleScreenshot()*1000, NULL); //scale to Seconds
	}

//#ifndef MESS
//	(void)windows::send_message(s_hToolBar, TB_ENABLEBUTTON, ID_VIEW_SOFTWARE_AREA, false);
//	(void)EnableMenuItem(hMainMenu, ID_VIEW_SOFTWARE_AREA, MF_DISABLED | MF_GRAYED);
//	DrawMenuBar(hMain);
//	(void)windows::show_window(dialog_boxes::get_dlg_item(hMain, IDC_SWLIST), SW_HIDE);
//	(void)windows::show_window(dialog_boxes::get_dlg_item(hMain, IDC_SWDEVVIEW), SW_HIDE);
//	(void)windows::show_window(dialog_boxes::get_dlg_item(hMain, IDC_SOFTLIST), SW_HIDE);
//	(void)windows::show_window(dialog_boxes::get_dlg_item(hMain, IDC_SWTAB), SW_HIDE);
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
		(void)DeleteObject(hMain);
		hMain = NULL;
	}

	if (hMissing_bitmap)
	{
		(void)gdi::delete_bitmap(hMissing_bitmap);
		hMissing_bitmap = NULL;
	}

	if (hbBackground)
	{
		(void)gdi::delete_bitmap(hbBackground);
		hbBackground = NULL;
	}

	if (hpBackground)
	{
		(void)gdi::delete_palette(hpBackground);
		hpBackground = NULL;
	}

	if (hFont)
	{
		(void)gdi::delete_font(hFont);
		hFont = NULL;
	}

	DestroyIcons();

	DestroyAcceleratorTable(hAccel);

	DirectInputClose();

	SetSavedFolderID(GetCurrentFolderID());
	SaveGameListOptions();
	mui_save_ini();
	ui_save_ini();

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
		(void)SetFocus(hwndList);
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
		UpdateMenu(GetMenu(hWnd));
	}
	break;

	case WM_CONTEXTMENU:
	{
		if (HandleTreeContextMenu(hWnd, wParam, lParam) || HandleScreenShotContextMenu(hWnd, wParam, lParam))
			break;

		return DefWindowProcW(hWnd, message, wParam, lParam);
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
		(void)GetWindowPlacement(hMain, &wndpl);
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
		int nItem = Picker_GetSelectedItem(hwndList);
		SetDefaultGame(nItem);

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

		//DefWindowProcW(hWnd, message, wParam, lParam);
		//printf("default row height calculation gives %u\n",lpmis->itemHeight);fflush(stdout);

		TEXTMETRIC tm;
		HDC hDC = GetDC(NULL);
		HFONT hFontOld = (HFONT)gdi::select_object(hDC, hFont);

		(void)GetTextMetricsW(hDC, &tm);

		lpmis->itemHeight = tm.tmHeight + tm.tmExternalLeading + 1;
		if (lpmis->itemHeight < 17)
			lpmis->itemHeight = 17;
		//printf("we would do %u\n",tm.tmHeight + tm.tmExternalLeading + 1);fflush(stdout);
		(void)gdi::select_object(hDC, hFontOld);
		(void)ReleaseDC(NULL, hDC);
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
			ListView_RedrawItems(hwndList, nItemIndex, nItemIndex);
	}
	break;

	case WM_MAMEUI_FILECHANGED:
	{
		int (*pfnGetAuditResults)(uint32_t driver_index) = NULL;
		void (*pfnSetAuditResults)(uint32_t driver_index, int audit_results) = NULL;

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
			int nGameIndex, nParentIndex = -1;
			char szFileName[MAX_PATH];
			snprintf(szFileName, std::size(szFileName), "%s", (LPCSTR)lParam);
			char* s = mui_strchr(szFileName, ".");
			if (s)
				*s = '\0';
			s = mui_strchr(szFileName, "\\");
			if (s)
				*s = '\0';

			for (nGameIndex = 0; nGameIndex < driver_list::total(); nGameIndex++)
			{
				for (nParentIndex = nGameIndex; nGameIndex == -1; nParentIndex = GetParentIndex(&driver_list::driver(nParentIndex)))
				{
					if (mui_stricmp(driver_list::driver(nParentIndex).name, szFileName) == 0)
					{
						if (pfnGetAuditResults(nGameIndex) != UNKNOWN)
						{
							pfnSetAuditResults(nGameIndex, UNKNOWN);
							(void)PostMessageW(hMain, WM_MAMEUI_AUDITGAME, wParam, nGameIndex);
						}
						break;
					}
				}
			}
		}
	}
	break;

	default:
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}

	return false;
}


static int HandleKeyboardGUIMessage(HWND hWnd, UINT message, UINT wParam, LONG lParam)
{
	switch (message)
	{
		case WM_CHAR: // List-View controls use this message for searching the items "as user types"
			//dialog_boxes::message_box(NULL,L"wm_char message arrived",L"TitleBox",MB_OK);
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

	if (!GetMessage(&msg, NULL, 0, 0))
		return false;

	if (IsWindow(hMain))
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
			(void)ListView_RedrawItems(hwndList, i, i);
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
	const char* pDescription;
	if (Picker_GetSelectedItem(hwndList) >= 0)
		pDescription = driver_list::driver(Picker_GetSelectedItem(hwndList)).type.fullname();
	else pDescription = "No Selection";
	SetStatusBarText(0, pDescription);
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
		(void)ListView_RedrawItems(hwndList, nItemIndex, nItemIndex);

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
	const char *pDescription;
	int drvindex = Picker_GetSelectedItem(hwndList);
	if (drvindex >= 0)
		pDescription = driver_list::driver(drvindex).type.fullname();
	else
		pDescription = "No Selection";
	SetStatusBarText(0, pDescription);
	idle_work = false;
	UpdateStatusBar();
	bFirstTime = true;

// don't need this any more 2014-01-26
//  if (!idle_work)
//      (void)PostMessageW(GetMainWindow(),WM_COMMAND, MAKEWPARAM(ID_VIEW_LINEUPICONS, true),(LPARAM)NULL);
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
	std::cout << "OnSize: Finished" << std::endl;
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
		if (r->items[i].subwindow != NULL)
			SetAllWindowsFont(hControl, (const Resize*)r->items[i].subwindow, hTheFont, bRedraw);
	}
}


static void ResizeWindow(HWND hParent, Resize *r)
{
	if (hParent == NULL)
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

		if (hControl == NULL)
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
	if (hProgWnd == NULL)
		return;

	int  widths[4];
	SIZE size;
	HDC hDC = GetDC(hProgWnd);

	(void)windows::show_window(hProgWnd, SW_HIDE);

	(void)GetTextExtentPoint32W(hDC, L"MMX", 3, &size);
	widths[3] = size.cx;
	(void)GetTextExtentPoint32W(hDC, L"MMMM games", 10, &size);
	widths[2] = size.cx;
	//Just specify 24 instead of 30, gives us sufficient space to display the message, and saves some space
	(void)GetTextExtentPoint32W(hDC, L"Screen flip support is missing", 24, &size);
	widths[1] = size.cx;

	(void)ReleaseDC(hProgWnd, hDC);

	widths[0] = -1;
	(void)windows::send_message(hStatusBar, SB_SETPARTS, (WPARAM)1, (LPARAM)(LPINT)widths);
	RECT rect;
	(void)StatusBar_GetItemRect(hStatusBar, 0, &rect);

	widths[0] = (rect.right - rect.left) - (widths[1] + widths[2] + widths[3]);
	widths[1] += widths[0];
	widths[2] += widths[1];
	widths[3] = -1;

	int numParts = 4;
	(void)windows::send_message(hStatusBar, SB_SETPARTS, (WPARAM)numParts, (LPARAM)(LPINT)widths);
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
			NULL,
			hInst,
			NULL);
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
			(void)windows::send_message(hStatusBar, SB_GETTEXT, (WPARAM)iButton, (LPARAM)&wcs_tooltip_text);
		else {
			//for first pane we get the Status directly, to get the line breaks
			std::string status_info = GameInfoStatus(Picker_GetSelectedItem(hwndList), false);
			const wchar_t *wcs_gameInfo_status = mui_wcstring_from_utf8(status_info.c_str());
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
	win_create_window_ex_utf8( 0L, "Edit", &SEARCH_PROMPT[0], WS_CHILD | WS_BORDER | WS_VISIBLE | ES_LEFT,
					iPosX, iPosY, 200, iHeight, hToolBar, (HMENU)ID_TOOLBAR_EDIT, hInst, NULL );

	return hToolBar;
}


static HWND InitStatusBar(HWND hParent)
{
	HMENU hMenu = GetMenu(hParent);

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
	HMENU hMainMenu = NULL;
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
		UINT nZero = 0;
		MenuHelp(WM_MENUSELECT, wParam, lParam, NULL, hInst, hStatusBar, &nZero);
	}

	return 0;
}


static void UpdateStatusBar()
{
	LPTREEFOLDER lpFolder = GetCurrentFolder();
	if (!lpFolder)
		return;

	int games_shown = 0;
	int nItemIndex = -1;

	while (1)
	{
		nItemIndex = FindGame(lpFolder, nItemIndex +1);
		if (nItemIndex == -1)
			break;

		if (!GameFiltered(nItemIndex, lpFolder->m_dwFlags))
			games_shown++;
	}

	// Show number of games in the current 'View' in the status bar
	SetStatusBarTextF(2, g_szGameCountString, games_shown);

	nItemIndex = Picker_GetSelectedItem(hwndList);

	if (games_shown == 0)
		DisableSelection();
	else
	{
		std::string status_info = GameInfoStatus(nItemIndex, false);
		SetStatusBarText(1, status_info.c_str());
	}
}


static void UpdateHistory(std::string software)
{
	//DWORD dwStyle = windows::get_window_long_ptr(dialog_boxes::get_dlg_item(hMain, IDC_HISTORY), GWL_STYLE);
	have_history = false;

	if (GetSelectedPick() >= 0)
	{
		char *histText = GetGameHistory(Picker_GetSelectedItem(hwndList), software);

		have_history = (histText && histText[0]) ? true : false;
		win_set_window_text_utf8(dialog_boxes::get_dlg_item(hMain, IDC_HISTORY), histText);
	}

	if (have_history && BIT(GetWindowPanes(), 3)
		&& ((TabView_GetCurrentTab(hTabCtrl) == TAB_HISTORY) ||
			(TabView_GetCurrentTab(hTabCtrl) == GetHistoryTab() && GetShowTab(TAB_HISTORY) == false) ||
			(TAB_ALL == GetHistoryTab() && GetShowTab(TAB_HISTORY) == false) ))
	{
		RECT rect;
		(void)Edit_GetRect(dialog_boxes::get_dlg_item(hMain, IDC_HISTORY),&rect);
		int nLines = Edit_GetLineCount(dialog_boxes::get_dlg_item(hMain, IDC_HISTORY) );
		HDC hDC = GetDC(dialog_boxes::get_dlg_item(hMain, IDC_HISTORY));
		TEXTMETRIC tm;
		(void)GetTextMetricsW (hDC, &tm);
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
	(void)SetMenuItemInfoW(hMainMenu, ID_FILE_PLAY, false, &mmi);

	(void)EnableMenuItem(hMainMenu, ID_FILE_PLAY, MF_GRAYED);
	(void)EnableMenuItem(hMainMenu, ID_FILE_PLAY_RECORD, MF_GRAYED);
	(void)EnableMenuItem(hMainMenu, ID_GAME_PROPERTIES, MF_GRAYED);
	(void)EnableMenuItem(hMainMenu, ID_MESS_OPEN_SOFTWARE, MF_GRAYED);

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
	std::cout << enable_selection << " A" << std::endl;
	bool has_software = MyFillSoftwareList(nGame, false); // messui.cpp
//#endif

	std::cout << enable_selection << " B" << std::endl;
	std::string description = ConvertAmpersandString(driver_list::driver(nGame).type.fullname());
	std::unique_ptr<wchar_t[]> wcs_description(mui_wcstring_from_utf8(description.c_str()));
	if( !wcs_description )
		return;

	std::cout << enable_selection << " C" << std::endl;
	std::wstring play_game_string = std::wstring(g_szPlayGameString) + wcs_description.get();
	MENUITEMINFOW mmi;
	mmi.cbSize = sizeof(mmi);
	mmi.fMask = MIIM_TYPE;
	mmi.fType = MFT_STRING;
	mmi.dwTypeData = &play_game_string[0];
	mmi.cch = mui_wcslen(mmi.dwTypeData);
	(void)SetMenuItemInfoW(hMainMenu, ID_FILE_PLAY, false, &mmi);
	std::cout << enable_selection << " D" << std::endl;
	const char * pText;
	pText = driver_list::driver(nGame).type.fullname();
	SetStatusBarText(0, pText);
	// Add this game's status to the status bar
	std::string status_info = GameInfoStatus(nGame, false);
	SetStatusBarText(1, status_info.c_str());

//#ifdef MESS
	// Show number of software_list items in box at bottom right.
	std::cout << enable_selection << " E" << std::endl;
	int items = SoftwareList_GetNumberOfItems();
	if (items)
		SetStatusBarText(3, &std::to_string(items)[0]);
	else
//#endif
		SetStatusBarText(3, "");

	// If doing updating game status

	std::cout << enable_selection << " F" << std::endl;
	(void)EnableMenuItem(hMainMenu, ID_FILE_PLAY, MF_ENABLED);
	(void)EnableMenuItem(hMainMenu, ID_FILE_PLAY_RECORD, MF_ENABLED);

//#ifdef MESS
	if (has_software)
		(void)EnableMenuItem(hMainMenu, ID_MESS_OPEN_SOFTWARE, MF_ENABLED);
	else
//#endif
		(void)EnableMenuItem(hMainMenu, ID_MESS_OPEN_SOFTWARE, MF_GRAYED);

	(void)EnableMenuItem(hMainMenu, ID_GAME_PROPERTIES, MF_ENABLED);

	std::cout << enable_selection << " G" << std::endl;
	if (bProgressShown && bListReady == true)
		SetDefaultGame(nGame);

	have_selection = true;

	std::cout << enable_selection << " H" << std::endl;
	UpdateScreenShot();
	//UpdateSoftware();   // to fix later

	std::cout << enable_selection << " Finished" << std::endl;
}


static void PaintBackgroundImage(HWND hWnd, HRGN hRgn, int x, int y)
{
	RECT rcClient;
	HDC hDC = GetDC(hWnd);

	// x and y are offsets within the background image that should be at 0,0 in hWnd

	// So we don't paint over the control's border
	(void)windows::get_client_rect(hWnd, &rcClient);

	HDC htempDC = gdi::create_compatible_dc(hDC);
	HBITMAP oldBitmap = (HBITMAP)gdi::select_object(htempDC, hbBackground);

	if (hRgn == NULL)
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
	if (hPAL == NULL)
		hPAL = gdi::create_half_tone_palette(hDC);

	if (gdi::get_device_caps(htempDC, RASTERCAPS) & RC_PALETTE && hPAL != NULL)
	{
		(void)gdi::select_palette(htempDC, hPAL, false);
		(void)gdi::realize_palette(htempDC);
	}

	for (size_t i = rcClient.left-x; i < rcClient.right; i += bmDesc.bmWidth)
		for (size_t j = rcClient.top-y; j < rcClient.bottom; j += bmDesc.bmHeight)
			(void)gdi::bit_blt(hDC, i, j, bmDesc.bmWidth, bmDesc.bmHeight, htempDC, 0, 0, SRCCOPY);

	(void)gdi::select_object(htempDC, oldBitmap);
	(void)gdi::delete_dc(htempDC);

	if (GetBackgroundPalette() == NULL)
	{
		(void)gdi::delete_palette(hPAL);
		hPAL = NULL;
	}

	(void)ReleaseDC(hWnd, hDC);
}


static const char* GetCloneParentName(int nItem)
{
	const char* fullname = "";
	int nParentIndex = -1;

	if (DriverIsClone(nItem) == true)
	{
		nParentIndex = GetParentIndex(&driver_list::driver(nItem));
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

			std::string utf8_szText = mui_utf8_from_wcstring(ptvdi->item.pszText);
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

	HMENU hMenuLoad = LoadMenu(hInst, menus::make_int_resource(IDR_CONTEXT_HEADER));
	HMENU hMenu = GetSubMenu(hMenuLoad, 0);
	lastColumnClick = nColumn;
	(void)TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hMain,NULL);

	(void)DestroyMenu(hMenuLoad);
}


std::string ConvertAmpersandString(std::string_view s) {
	std::string result;
	result.reserve(s.size() + std::count(s.begin(), s.end(), '&')); // Reserve space to avoid multiple allocations
	for (char ch : s) {
		if (ch == '&') {
			result += "&&";
		}
		else {
			result += ch;
		}
	}
	return result;
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
			std::cout << "seq =" << GUISequenceControl[i].name << "pressed" << std::endl;
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
	std::cout << "keyboard gui state cleared." << std::endl;
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
		std::cout << "vk_code pressed not found = " << vk_code << std::endl;
		//dialog_boxes::message_box(NULL,L"keydown message arrived not processed",L"TitleBox",MB_OK);
		return;
	}
	std::cout << "vk_code pressed found = " << vk_code
		<< ", syskey =" << syskey << ", mame_keycode =" << icode
		<< ", special = " << std::hex << std::setw(8) << std::setfill('0') << special << std::endl;

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
		std::cout << "vk_code released not found = " << vk_code << std::endl;
		//dialog_boxes::message_box(NULL,L"keyup message arrived not processed",L"TitleBox",MB_OK);
		return;
	}
	keyboard_state[icode] = false;
	std::cout << "vk_code pressed found = " << vk_code
		<< ", syskey =" << syskey << ", mame_keycode =" << icode
		<< ", special = " << std::hex << std::setw(8) << std::setfill('0') << special << std::endl;

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
			std::string_view exec_command = GetExecCommand();
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
			std::unique_ptr<wchar_t[]> wcs_exec_command(mui_wcstring_from_utf8(&exec_command[0]));
			if (!wcs_exec_command)
				return;

			(void)CreateProcessW(NULL, const_cast<wchar_t*>(wcs_exec_command.get()), NULL, NULL, false, 0, NULL, NULL, &si, &pi);
			// We will not wait for the process to finish cause it might be a background task
			// The process won't get closed when MAME32 closes either.

			// But close the handles cause we won't need them anymore. Will not close process.
			(void)CloseHandle(pi.hProcess);
			(void)CloseHandle(pi.hThread);
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
	(void)CheckMenuRadioItem(hMainMenu, ID_VIEW_LARGE_ICON, ID_VIEW_GROUPED, menu_id, MF_CHECKED);
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
				(void)ListView_InsertItem(hwndList, &lvi);
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
	gdi::invalidate_rect(hMain,NULL,TRUE);
	Picker_ResetIdle(hwndList);
}

static void UpdateCache()
{
	int current_id = GetCurrentFolderID(); // remember selected folder
	SetWindowRedraw(hwndList, false);   // stop screen updating
	ForceRebuild();          // tell system that cache needs redoing
	OptionsInit();      // reload options and fix game cache
	emu_opts_init(1);
	//extern const FOLDERDATA g_folderData[];
	//extern const FILTER_ITEM g_filterList[];
	//InitTree(g_folderData, g_filterList);         // redo folders... This crashes, leave out for now
	ResetTreeViewFolders();                      // something with folders
	SelectTreeViewFolder(current_id);            // select previous folder
	SetWindowRedraw(hwndList, true);             // refresh screen
}

UINT_PTR CALLBACK CFHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
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
	if (!ChooseFont(&cf))
		return;

	SetListFont(&font);
	if (hFont != NULL)
		(void)gdi::delete_font(hFont);

	hFont = gdi::create_font_indirect(&font);
	if (hFont != NULL)
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
	gdi::invalidate_rect(hwndList,NULL,FALSE);
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
		MamePlayRecordGame();
		return true;

	case ID_FILE_PLAY_BACK:
		MamePlayBackGame();
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
		MameLoadState();
		return true;

	case ID_FILE_AUDIT:
		AuditDialog(hMain, 1);
		ResetWhichGamesInFolders();
		ResetListView();
		(void)SetFocus(hwndList);
		return true;

	case ID_FILE_AUDIT_X:
		AuditDialog(hMain, 2);
		ResetWhichGamesInFolders();
		ResetListView();
		(void)SetFocus(hwndList);
		return true;

	case ID_FILE_EXIT:
		(void)PostMessageW(hMain, WM_CLOSE, 0, 0);
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
		bool bShowTree = BIT(val, 0);
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
		gdi::invalidate_rect(hMain,NULL,TRUE);
		break;

	   // Switches to fullscreen mode. No check mark handling
	   // for this item cause in fullscreen mode the menu won't
	   // be visible anyways.

	case ID_VIEW_FULLSCREEN:
		SwitchFullScreenMode();
		break;

	case ID_TOOLBAR_EDIT:
		{
			std::string buf;
			HWND hToolbarEdit;

			buf = win_get_window_text_utf8(hwndCtl);
			switch (codeNotify)
			{
			case TOOLBAR_EDIT_ACCELERATOR_PRESSED:
				hToolbarEdit = dialog_boxes::get_dlg_item( s_hToolBar, ID_TOOLBAR_EDIT);
				(void)SetFocus(hToolbarEdit);
				break;
			case EN_CHANGE:
				//put search routine here first, add a 200ms timer later.
				if ((!mui_stricmp(buf.c_str(), SEARCH_PROMPT) && !mui_stricmp(g_SearchText, "")) ||
					(!mui_stricmp(g_SearchText, SEARCH_PROMPT) && !mui_stricmp(buf.c_str(), "")))
				{
					(void)mui_strcpy(g_SearchText,buf.c_str());
				}
				else
				{
					(void)mui_strcpy(g_SearchText, buf.c_str());
					ResetListView();
				}
				break;
			case EN_SETFOCUS:
				if (!mui_stricmp(buf.c_str(), SEARCH_PROMPT))
					win_set_window_text_utf8(hwndCtl, "");
				break;
			case EN_KILLFOCUS:
				if (*buf.c_str() == 0)
					win_set_window_text_utf8(hwndCtl, &SEARCH_PROMPT[0]);
				break;
			}
		}
		break;

	case ID_GAME_AUDIT:
		InitGameAudit(0);
		if (nCurrentGame >= 0)
		{
			InitPropertyPageToPage(hInst, hwnd, GetSelectedPickItemIcon(), OPTIONS_GAME, -1, nCurrentGame, AUDIT_PAGE);
		}
		// Just in case the toggle MMX on/off
		UpdateStatusBar();
		break;

	// ListView Context Menu
	case ID_CONTEXT_ADD_CUSTOM:
	{
		if (nCurrentGame >= 0)
			DialogBoxParam(system_services::get_module_handle(NULL),menus::make_int_resource(IDD_CUSTOM_FILE), hMain, AddCustomFileDialogProc, nCurrentGame);
		(void)SetFocus(hwndList);
		break;
	}

	case ID_CONTEXT_REMOVE_CUSTOM:
	{
		RemoveCurrentGameCustomFolder();
		break;
	}

	// Tree Context Menu
	case ID_CONTEXT_FILTERS:
		if (dialog_boxes::dialog_box(system_services::get_module_handle(NULL), menus::make_int_resource(IDD_FILTERS), hMain, FilterDialogProc, 0L) == true)
			ResetListView();
		(void)SetFocus(hwndList);
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
		if (dialog_boxes::dialog_box(system_services::get_module_handle(NULL), menus::make_int_resource(IDD_COLUMNS), hMain, ColumnDialogProc, 0L) == true)
			ResetColumnDisplay(FALSE);
		(void)SetFocus(hwndList);
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
			InitPropertyPageToPage(hInst, hwnd, GetSelectedPickItemIcon(), OPTIONS_GAME, -1, nCurrentGame, PROPERTIES_PAGE);
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
						if (data->m_opttype < OPTIONS_MAX)
							InitPropertyPage(hInst, hwnd, GetSelectedFolderIcon(), data->m_opttype, folder->m_nFolderId, -1);
				}
		}
		UpdateStatusBar();
		break;

	case ID_FOLDER_SOURCEPROPERTIES:
	{
		if (nCurrentGame < 0)
			return true;
		InitPropertyPage(hInst, hwnd, GetSelectedFolderIcon(), OPTIONS_SOURCE, -1, nCurrentGame);
		UpdateStatusBar();
		(void)SetFocus(hwndList);
		return true;
		//folder = GetFolderByName(FOLDER_SOURCE, GetDriverFilename(Picker_GetSelectedItem(hwndList)) );
		//InitPropertyPage(hInst, hwnd, GetSelectedFolderIcon(), (folder->m_nFolderId == FOLDER_VECTOR) ? OPTIONS_VECTOR : OPTIONS_SOURCE , folder->m_nFolderId, Picker_GetSelectedItem(hwndList));
		//UpdateStatusBar();
		//break;
	}

	case ID_FOLDER_VECTORPROPERTIES:
		if (nCurrentGame >= 0)
		{
			folder = GetFolderByID( FOLDER_VECTOR );
			InitPropertyPage(hInst, hwnd, GetSelectedFolderIcon(), OPTIONS_VECTOR, folder->m_nFolderId, nCurrentGame);
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
		(void)SetFocus(hwndList);
		return true;

	case ID_OPTIONS_DIR:
		{
			int nResult = dialog_boxes::dialog_box(system_services::get_module_handle(NULL), menus::make_int_resource(IDD_DIRECTORIES), hMain, DirectoriesDialogProc, 0L);

			global_save_ini();
			mui_save_ini();
			ui_save_ini();

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
				{
					auto media_path_ptr = std::make_shared<std::string>(dir_get_value(DIRMAP_MEDIAPATH));
					DirWatcher_Watch(s_pWatcher, 0, media_path_ptr, true);
				}
				if (bUpdateSamples)
				{
					auto sample_path_ptr = std::make_shared<std::string>(dir_get_value(DIRMAP_SAMPLEPATH));
					DirWatcher_Watch(s_pWatcher, 1, sample_path_ptr, true);
				}
			}

			// update game list
			if (bUpdateRoms == true || bUpdateSamples == true)
				UpdateGameList(bUpdateRoms, bUpdateSamples);

			(void)SetFocus(hwndList);
		}
		return true;

	case ID_OPTIONS_RESET_DEFAULTS:
		if (dialog_boxes::dialog_box(system_services::get_module_handle(NULL), menus::make_int_resource(IDD_RESET), hMain, ResetDialogProc, 0L) == true)
		{
			// these may have been changed
			global_save_ini();
			mui_save_ini();
			ui_save_ini();
			windows::destroy_window(hwnd);
			windows::post_quiet_message(0);
		}
		else
		{
			ResetListView();
			(void)SetFocus(hwndList);
		}
		return true;

	case ID_OPTIONS_INTERFACE:
		(void)dialog_boxes::dialog_box(system_services::get_module_handle(NULL), menus::make_int_resource(IDD_INTERFACE_OPTIONS), hMain, InterfaceDialogProc, 0L);
		global_save_ini();
		mui_save_ini();
		ui_save_ini();

		KillTimer(hMain, SCREENSHOT_TIMER);
		if( GetCycleScreenshot() > 0)
			SetTimer(hMain, SCREENSHOT_TIMER, GetCycleScreenshot()*1000, NULL ); // Scale to seconds

		return true;

	case ID_VIDEO_SNAP:
		{
			int nGame = Picker_GetSelectedItem(hwndList);
			if (nGame >= 0)
			{
				std::string path = &GetVideoDir()[0] + "\\"s + driver_list::driver(nGame).name + ".mp4"s;
				ShellExecuteCommon(hMain, path.c_str());
			}
			(void)SetFocus(hwndList);
		}
		break;

	case ID_MANUAL:
		{
			int nGame = Picker_GetSelectedItem(hwndList);
			if (nGame >= 0)
			{
				std::string path = &GetManualsDir()[0] + "\\"s + driver_list::driver(nGame).name + ".pdf"s;
				ShellExecuteCommon(hMain, path.c_str());
			}
			(void)SetFocus(hwndList);
		}
		break;

	case ID_RC_CLEAN:
		{
			int nGame = Picker_GetSelectedItem(hwndList);
			if (nGame >= 0)
			{
				std::istringstream tokenStream;
				std::string dir_path,
					file_path,
					token;

				// INI
				dir_path = dir_get_value(DIRMAP_INIPATH);
				tokenStream.str(dir_path);
				while (std::getline(tokenStream, token, ';'))
				{
					file_path = token + PATH_SEPARATOR + driver_list::driver(nGame).name + ".ini";
					std::cout << "Deleting " << file_path << std::endl;
					remove(file_path.c_str());
				}
				// CFG
				dir_path = dir_get_value(DIRMAP_CFG_DIRECTORY);
				tokenStream.str(dir_path);
				while (std::getline(tokenStream, token, ';'))
				{
					file_path = token + PATH_SEPARATOR + driver_list::driver(nGame).name + ".cfg";
					std::cout << "Deleting " << file_path << std::endl;
					remove(file_path.c_str());
				}
				// NVRAM
				dir_path = dir_get_value(DIRMAP_NVRAM_DIRECTORY);
				tokenStream.str(dir_path);
				while (std::getline(tokenStream, token, ';'))
				{
					file_path = std::string("rd /s /q ") + token + PATH_SEPARATOR + driver_list::driver(nGame).name;
					std::cout << "Deleting " << file_path << std::endl;
					remove(file_path.c_str());
				}
				// Save states
				dir_path = dir_get_value(DIRMAP_STATE_DIRECTORY);
				tokenStream.str(dir_path);
				while (std::getline(tokenStream, token, ';'))
				{
					file_path = std::string("rd /s /q ") + token + PATH_SEPARATOR + driver_list::driver(nGame).name;
					std::cout << "Deleting " << file_path << std::endl;
					remove(file_path.c_str());
				}
			}
			(void)SetFocus(hwndList);
		}
		break;

	case ID_NOTEPAD:
		{
			int nGame = Picker_GetSelectedItem(hwndList);
			if (nGame >= 0)
			{
				const char* utf8_filename = "history.wtx";
				std::string t2 = GetGameHistory(nGame);
				std::ofstream outfile (utf8_filename, std::ios::out | std::ios::trunc);
				outfile.write(t2.c_str(), t2.size());
				outfile.close();
				std::string path = std::string(".\\") + utf8_filename;
				ShellExecuteCommon(hMain, path.c_str());
			}
			(void)SetFocus(hwndList);
		}
		break;

	case ID_OPTIONS_BG:
		{
			// Get the path from the existing filename; if no filename go to root
			OPENFILENAMEW OFN;
			wchar_t wcs_filepath[MAX_PATH] = L"\0";
			std::unique_ptr<wchar_t[]> szInitialDir;

			std::filesystem::path initial_dir_path(GetBgDir());
			if (std::filesystem::exists(initial_dir_path))
			{
				std::string parent_dir = initial_dir_path.parent_path().string();
				szInitialDir = std::unique_ptr<wchar_t[]>(mui_wcstring_from_utf8(parent_dir.c_str()));
			}
			else
				szInitialDir = std::unique_ptr<wchar_t[]>(const_cast<wchar_t*>(L"."));

			OFN.lStructSize       = sizeof(OPENFILENAMEW);
			OFN.hwndOwner         = hMain;
			OFN.hInstance         = 0;
			OFN.lpstrFilter       = L"Image Files (*.png)\0*.PNG\0";
			OFN.lpstrCustomFilter = NULL;
			OFN.nMaxCustFilter    = 0;
			OFN.nFilterIndex      = 1;
			OFN.lpstrFile         = wcs_filepath;
			OFN.nMaxFile          = std::size(wcs_filepath);
			OFN.lpstrFileTitle    = NULL;
			OFN.nMaxFileTitle     = 0;
			OFN.lpstrInitialDir   = szInitialDir.get();
			OFN.lpstrTitle        = L"Select a Background Image";
			OFN.nFileOffset       = 0;
			OFN.nFileExtension    = 0;
			OFN.lpstrDefExt       = NULL;
			OFN.lCustData         = 0;
			OFN.lpfnHook          = NULL;
			OFN.lpTemplateName    = NULL;
			OFN.Flags             = OFN_NOCHANGEDIR | OFN_SHOWHELP | OFN_EXPLORER;

			bResult = GetOpenFileNameW(&OFN);
			if (bResult)
			{
				std::filesystem::path file_path(wcs_filepath);
				if (!std::filesystem::exists(wcs_filepath))
					return false;

				std::unique_ptr<const char[]> utf8_filepath(mui_utf8_from_wcstring(wcs_filepath));
				SetBgDir(utf8_filepath.get());

				// Display new background
				LoadBackgroundBitmap();
				(void)gdi::invalidate_rect(hMain, NULL, true);

				return true;
			}
		}
		break;

	case ID_HELP_ABOUT:
		(void)dialog_boxes::dialog_box(system_services::get_module_handle(NULL), menus::make_int_resource(IDD_ABOUT), hMain, AboutDialogProc, 0L);
		(void)SetFocus(hwndList);
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
		(void)SetFocus(hwndList);
		MamePlayGame();
		break;

	case ID_UI_UP:
		Picker_SetSelectedPick(hwndList, GetSelectedPick() - 1);
		break;

	case ID_UI_DOWN:
		Picker_SetSelectedPick(hwndList, GetSelectedPick() + 1);
		break;

	case ID_UI_PGUP:
		Picker_SetSelectedPick(hwndList, GetSelectedPick() - ListView_GetCountPerPage(hwndList));
		break;

	case ID_UI_PGDOWN:
		if ( (GetSelectedPick() + ListView_GetCountPerPage(hwndList)) < list_view::get_item_count(hwndList) )
			Picker_SetSelectedPick(hwndList,  GetSelectedPick() + ListView_GetCountPerPage(hwndList) );
		else
			Picker_SetSelectedPick(hwndList,  list_view::get_item_count(hwndList)-1 );
		break;

	case ID_UI_HOME:
		Picker_SetSelectedPick(hwndList, 0);
		break;

	case ID_UI_END:
		Picker_SetSelectedPick(hwndList,  list_view::get_item_count(hwndList)-1 );
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
		bResult = ListView_RedrawItems(hwndList, GetSelectedPick(), GetSelectedPick());
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
				//printf("%X: %ls\n",g_helpInfo[i].bIsHtmlHelp, g_helpInfo[i].lpFile);fflush(stdout);
				if (i == 1) // get current whatsnew.txt from mamedev.org
				{
					std::string version = std::string(GetVersionString()); // turn version string into std
					version.erase(1,1); // take out the decimal point
					version.erase(4, std::string::npos); // take out the date
					std::string url = "https://mamedev.org/releases/whatsnew_" + version + ".txt"; // construct url
					std::unique_ptr<wchar_t[]> utf8_to_wcs(mui_wcstring_from_utf8(url.c_str())); // then convert to const wchar_t*
					(void)shell::shell_execute(hMain, shellExecVerb, utf8_to_wcs.get(), shellExecParam, NULL, SW_SHOWNORMAL); // show web page
				}
				else
				if (g_helpInfo[i].bIsHtmlHelp)
//                  HelpFunction(hMain, g_helpInfo[i].lpFile, HH_DISPLAY_TOPIC, 0);
					(void)shell::shell_execute(hMain, shellExecVerb, g_helpInfo[i].lpFile, shellExecParam, NULL, SW_SHOWNORMAL);
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
		hbBackground = NULL;
	}

	if (hpBackground)
	{
		(void)gdi::delete_palette(hpBackground);
		hpBackground = NULL;
	}

	if (LoadDIBBG(&hDIBbg, &hpBackground)) // screenshot.cpp
	{
		HDC hDC = GetDC(hwndList);
		hbBackground = DIBToDDB(hDC, hDIBbg, &bmDesc);
		(void)GlobalFree(hDIBbg);
		(void)ReleaseDC(hwndList, hDC);
	}
}


static void ResetColumnDisplay(bool first_time)
{
	if (!first_time)
		Picker_ResetColumnDisplay(hwndList);

	ResetListView();

	Picker_SetSelectedItem(hwndList, GetDefaultGame());
}


static int GamePicker_GetItemImage(HWND hwndPicker, int nItem)
{
	return GetIconForDriver(nItem);
}


static const wchar_t *GamePicker_GetItemString(HWND hwndPicker, int nItem, int nColumn, wchar_t *pszBuffer, UINT nBufferLength)
{
	const wchar_t *wcs_item_string = NULL;
	const char* utf8_s = NULL;
	game_driver driver = driver_list::driver(nItem);

	switch(nColumn)
	{
		case COLUMN_GAMES:
			// Driver description
			utf8_s = driver.type.fullname();
			break;

		case COLUMN_ORIENTATION:
		{
			// Screen orientation
			wcs_item_string = DriverIsVertical(nItem) ? L"Vertical" : L"Horizontal";
			break;
		}
		case COLUMN_ROMS:
		{
			// ROMs
			utf8_s = GetAuditString(GetRomAuditResults(nItem));
			break;
		}
		case COLUMN_SAMPLES:
		{
			// Samples
			utf8_s = DriverUsesSamples(nItem) ? GetAuditString(GetSampleAuditResults(nItem)) : "-";
			break;
		}
		case COLUMN_DIRECTORY:
		{
			// Driver name (directory)
			utf8_s = driver.name;
			break;
		}
		case COLUMN_SRCDRIVERS:
		{
			// Source drivers
			utf8_s = GetDriverFilename(nItem);
			break;
		}
		case COLUMN_PLAYTIME:
		{
			// Play time

			utf8_s = GetTextPlayTime(nItem).c_str();
			break;
		}
		case COLUMN_TYPE:
		{
			// Vector/Raster
			machine_config config(driver, MameUIGlobal());
			wcs_item_string = isDriverVector(&config) ? L"Vector" : L"Raster";
			break;
		}
		case COLUMN_TRACKBALL:
		{
			// Trackball
			wcs_item_string = DriverUsesTrackball(nItem) ? L"Yes" : L"No";
			break;
		}
		case COLUMN_PLAYED:
		{   // Times played
			utf8_s = util::string_format("%d", GetPlayCount(nItem)).c_str();
			break;
		}
		case COLUMN_MANUFACTURER:
		{
			// Manufacturer
			utf8_s = driver.manufacturer;
			break;
		}
		case COLUMN_YEAR:
		{
			// Year
			utf8_s = driver.year;
			break;
		}
		case COLUMN_CLONE:
		{
			utf8_s = GetCloneParentName(nItem);
			break;
		}
	}

	if(utf8_s && *utf8_s != '\0')
	{
		std::unique_ptr<wchar_t[]> wcsItemString(mui_wcstring_from_utf8(utf8_s));
		if( !wcsItemString)
			return wcs_item_string;

		mui_wcsncpy(pszBuffer, wcsItemString.get(),nBufferLength);
		wcs_item_string = pszBuffer;
	}

	return wcs_item_string;
}


static void GamePicker_LeavingItem(HWND hwndPicker, int nItem)
{
//#ifdef MESS
	// leaving item
	g_szSelectedItem.clear();
//#endif
}


static void GamePicker_EnteringItem(HWND hwndPicker, int nItem)
{
	// printf("entering %s\n",driver_list::driver(nItem).name);fflush(stdout);
	EnableSelection(nItem);

//#ifdef MESS
	MessReadMountedSoftware(nItem); // messui.cpp
//#endif

	// decide if it is valid to load a savestate
	(void)EnableMenuItem(hMainMenu, ID_FILE_LOADSTATE, (driver_list::driver(nItem).flags & MACHINE_SUPPORTS_SAVE) ? MFS_ENABLED : MFS_GRAYED);
}


static int GamePicker_FindItemParent(HWND hwndPicker, int nItem)
{
	return GetParentRomSetIndex(&driver_list::driver(nItem));
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
	opts.nColumnCount = COLUMN_MAX;
	opts.ppszColumnNames = column_names;
	SetupPicker(hwndList, &opts);

	(void)ListView_SetTextBkColor(hwndList, CLR_NONE);
	(void)ListView_SetBkColor(hwndList, CLR_NONE);
	wcs_BgDir = std::unique_ptr<wchar_t[]>(mui_wcstring_from_utf8(&GetBgDir()[0]));
	if( !wcs_BgDir)
		return;

	bki.ulFlags = LVBKIF_SOURCE_URL | LVBKIF_STYLE_TILE;
	bki.pszImage = const_cast<wchar_t*>(wcs_BgDir.get());
	if( hbBackground )
		(void)ListView_SetBkImage(hwndList, &bki);

	CreateIcons();
	ResetColumnDisplay(TRUE);

	// Allow selection to change the default saved game
	bListReady = true;
}


static void AddDriverIcon(int nItem,int default_icon_index)
{
	HICON hIcon = 0;
	int nParentIndex = -1;

	// if already set to rom or clone icon, we've been here before
	if (icon_index[nItem] == 1 || icon_index[nItem] == 3)
		return;

	hIcon = LoadIconFromFile((char *)driver_list::driver(nItem).name);
	if (hIcon == NULL)
	{
		nParentIndex = GetParentIndex(&driver_list::driver(nItem));
		if( nParentIndex >= 0)
		{
			hIcon = LoadIconFromFile((char *)driver_list::driver(nParentIndex).name);
			if (hIcon == NULL)
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
			icon_index[nItem] = nIconPos;
		(void)DestroyIcon(hIcon);
	}
	if (icon_index[nItem] == 0)
		icon_index[nItem] = default_icon_index;
}


static void DestroyIcons(void)
{
	if (hSmall)
	{
		(void)ImageList_Destroy(hSmall);
		hSmall = NULL;
	}

	if (icon_index)
	{
		for (size_t i=0;i<driver_list::total();i++)
			icon_index[i] = 0; // these are indices into hSmall
	}

	if (hLarge)
	{
		(void)ImageList_Destroy(hLarge);
		hLarge = NULL;
	}

	if (hHeaderImages)
	{
		(void)ImageList_Destroy(hHeaderImages);
		hHeaderImages = NULL;
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

	for (size_t i = 0; g_iconData[i].icon_name; i++)
	{
		hIcon = LoadIconFromFile((char *) g_iconData[i].icon_name);
		if (hIcon == NULL)
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
	LPWSTR wErrorMessage = NULL;

	// Get the Key
	LONG lRes = RegOpenKeyW(HKEY_CURRENT_USER, L"Control Panel\\Desktop\\WindowMetrics", &hKey);
	if( lRes != ERROR_SUCCESS )
	{
		GetSystemErrorMessage(lRes, &wErrorMessage);
		(void)dialog_boxes::message_box(GetMainWindow(), wErrorMessage, L"Large shell icon size registry access", MB_OK | MB_ICONERROR);
		(void)LocalFree(wErrorMessage);
		return dwSize;
	}

	// Save the last size
	wchar_t  szBuffer[512];
	lRes = RegQueryValueExW(hKey, L"Shell Icon Size", NULL, &dwType, (LPBYTE)szBuffer, &dwLength);
	if( lRes != ERROR_SUCCESS )
	{
		GetSystemErrorMessage(lRes, &wErrorMessage);
		(void)dialog_boxes::message_box(GetMainWindow(), wErrorMessage, L"Large shell icon size registry query", MB_OK | MB_ICONERROR);
		(void)LocalFree(wErrorMessage);
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
	(void)RegCloseKey(hKey);
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

	while(g_iconData[icon_count].icon_name)
		icon_count++;

	// the current window style affects the sizing of the rows when changing
	// between list views, so put it in small icon mode temporarily while we associate
	// our image list

	// using large icon mode instead kills the horizontal scrollbar when doing
	// full refresh, which seems odd (it should recreate the scrollbar when
	// set back to report mode, for example, but it doesn't).

	LONG_PTR lpListViewStyle = windows::get_window_long_ptr(hwndList,GWL_STYLE);
	(void)windows::set_window_long_ptr(hwndList,GWL_STYLE,(lpListViewStyle & ~LVS_TYPEMASK) | LVS_ICON);

	hSmall = ImageList_Create(dwSmallIconSize, dwSmallIconSize, ILC_COLORDDB | ILC_MASK, icon_count, icon_count + grow);

	if (NULL == hSmall)
	{
		(void)dialog_boxes::message_box(GetMainWindow(), L"Cannot allocate small icon image list", L"Allocation error - Exiting", MB_OK | MB_ICONERROR);
		windows::post_quiet_message(0);
	}

	hLarge = ImageList_Create(dwLargeIconSize, dwLargeIconSize, ILC_COLORDDB | ILC_MASK, icon_count, icon_count + grow);

	if (NULL == hLarge)
	{
		(void)dialog_boxes::message_box(GetMainWindow(), L"Cannot allocate large icon image list", L"Allocation error - Exiting", MB_OK | MB_ICONERROR);
		windows::post_quiet_message(0);
	}

	ReloadIcons();

	// Associate the image lists with the list view control.
	(void)ListView_SetImageList(hwndList, hSmall, LVSIL_SMALL);
	(void)ListView_SetImageList(hwndList, hLarge, LVSIL_NORMAL);

	// restore our view
	(void)windows::set_window_long_ptr(hwndList,GWL_STYLE, lpListViewStyle);

//#ifdef MESS
	CreateMessIcons(); // messui.cpp
//#endif

	// Now set up header specific stuff
	hHeaderImages = ImageList_Create(8,8,ILC_COLORDDB | ILC_MASK,2,2);
	hIcon = menus::load_icon(hInst,menus::make_int_resource(IDI_HEADER_UP));
	image_list::add_icon(hHeaderImages,hIcon);
	hIcon = menus::load_icon(hInst,menus::make_int_resource(IDI_HEADER_DOWN));
	image_list::add_icon(hHeaderImages,hIcon);

	for (size_t i = 0; i < std::size(s_nPickers); i++)
		Picker_SetHeaderImageList(dialog_boxes::get_dlg_item(hMain, s_nPickers[i]), hHeaderImages);
}


static int GamePicker_Compare(HWND hwndPicker, int index1, int index2, int sort_subitem)
{
	int value = 0;  // Default to 0, for unknown case
//  const char *name1 = NULL;
//  const char *name2 = NULL;
//  char file1[MAX_PATH];
//  char file2[MAX_PATH];
	const char *strCmpParam1 = NULL, *strCmpParam2 = NULL;
	int numCmpParam1=0, numCmpParam2=0;
	game_driver game1 = driver_list::driver(index1);
	game_driver game2 = driver_list::driver(index2);

	switch (sort_subitem)
	{
	case COLUMN_GAMES:
	{
		strCmpParam1 = game1.type.fullname();
		strCmpParam2 = game2.type.fullname();
		return mui_stricmp(strCmpParam1, strCmpParam2);
	}
	case COLUMN_ORIENTATION:
	{
		numCmpParam1 = DriverIsVertical(index1) ? 1 : 0;
		numCmpParam2 = DriverIsVertical(index2) ? 1 : 0;
		value = numCmpParam1 - numCmpParam2;
		break;
	}
	case COLUMN_DIRECTORY:
	{
		strCmpParam1 = game1.name;
		strCmpParam2 = game2.name;
		value = mui_stricmp(strCmpParam1, strCmpParam2);
		break;
	}
	case COLUMN_SRCDRIVERS:
	{
		//      (void)mui_strcpy(file1, GetDriverFilename(index1));
		//      (void)mui_strcpy(file2, GetDriverFilename(index2));
		strCmpParam1 = GetDriverFilename(index1);
		strCmpParam2 = GetDriverFilename(index2);
		value = mui_stricmp(strCmpParam1, strCmpParam2);
		break;
	}
	case COLUMN_PLAYTIME:
	{
		numCmpParam1 = GetPlayTime(index1);
		numCmpParam2 = GetPlayTime(index2);
		value = numCmpParam1 - numCmpParam2;
		break;
	}
	case COLUMN_ROMS:
	{
		numCmpParam1 = GetRomAuditResults(index1);
		numCmpParam2 = GetRomAuditResults(index2);
		value = numCmpParam1 - numCmpParam2;
		break;
	}
	case COLUMN_SAMPLES:
		numCmpParam1 = GetSampleAuditResults(index1);
		numCmpParam2 = GetSampleAuditResults(index2);
		value = numCmpParam1 - numCmpParam2;
		break;

	case COLUMN_TYPE:
	{
		machine_config config1(game1, MameUIGlobal());
		machine_config config2(game2, MameUIGlobal());
		value = isDriverVector(&config1) - isDriverVector(&config2);
		break;
	}

	case COLUMN_TRACKBALL:
	{
		numCmpParam1 = DriverUsesTrackball(index1) ? 1 : 0;
		numCmpParam2 = DriverUsesTrackball(index2) ? 1 : 0;
		value = numCmpParam1 - numCmpParam2;
		break;
	}
	case COLUMN_PLAYED:
	{
		numCmpParam1 = GetPlayCount(index1);
		numCmpParam2 = GetPlayCount(index2);
		value = numCmpParam1 - numCmpParam2;
		break;
	}
	case COLUMN_MANUFACTURER:
	{
		strCmpParam1 = game1.manufacturer;
		strCmpParam2 = game2.manufacturer;
		value = mui_stricmp(strCmpParam1, strCmpParam2);
		break;
	}
	case COLUMN_YEAR:
	{
		strCmpParam1 = game1.year;
		strCmpParam2 = game2.year;
		value = mui_stricmp(strCmpParam1, strCmpParam2);
		break;
	}
	case COLUMN_CLONE:
	{
		strCmpParam1 = GetCloneParentName(index1);
		strCmpParam2 = GetCloneParentName(index2);

		if (strCmpParam1 && !strCmpParam2)
			value = -1;
		else if (!strCmpParam1 && strCmpParam2)
			value = 1;
		else if (*strCmpParam1 != '\0' && *strCmpParam2 != '\0')
			value = mui_stricmp(strCmpParam1, strCmpParam2);
		break;
	}
	}

	// Handle same comparisons here
	if (0 == value && COLUMN_GAMES != sort_subitem)
		value = GamePicker_Compare(hwndPicker, index1, index2, COLUMN_GAMES);

	return value;
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
		Picker_SetSelectedPick(hwndList, rand() % nListCount);
}


bool CommonFileDialog(common_file_dialog_proc cfd, wchar_t *filename, int filetype)
{
	OPENFILENAMEW ofn;
	std::string dirname;

	ofn.lStructSize       = sizeof(ofn);
	ofn.hwndOwner         = hMain;
	ofn.hInstance         = NULL;
	switch (filetype)
	{
	case FILETYPE_INPUT_FILES :
		ofn.lpstrFilter   = L"input files (*.inp,*.zip,*.7z)\0*.inp;*.zip;*.7z\0All files (*.*)\0*.*\0";
		ofn.lpstrDefExt   = L"inp";
		dirname = dir_get_value(DIRMAP_INPUT_DIRECTORY);
		break;
	case FILETYPE_SAVESTATE_FILES :
		ofn.lpstrFilter   = L"savestate files (*.sta)\0*.sta;\0All files (*.*)\0*.*\0";
		ofn.lpstrDefExt   = L"sta";
		dirname = dir_get_value(DIRMAP_STATE_DIRECTORY);
		break;
	case FILETYPE_WAVE_FILES :
		ofn.lpstrFilter   = L"sounds (*.wav)\0*.wav;\0All files (*.*)\0*.*\0";
		ofn.lpstrDefExt   = L"wav";
		dirname = dir_get_value(DIRMAP_SNAPSHOT_DIRECTORY);
		break;
	case FILETYPE_MNG_FILES :
		ofn.lpstrFilter   = L"videos (*.mng)\0*.mng;\0All files (*.*)\0*.*\0";
		ofn.lpstrDefExt   = L"mng";
		dirname = dir_get_value(DIRMAP_SNAPSHOT_DIRECTORY);
		break;
	case FILETYPE_AVI_FILES :
		ofn.lpstrFilter   = L"videos (*.avi)\0*.avi;\0All files (*.*)\0*.*\0";
		ofn.lpstrDefExt   = L"avi";
		dirname = dir_get_value(DIRMAP_SNAPSHOT_DIRECTORY);
		break;
	case FILETYPE_EFFECT_FILES :
		ofn.lpstrFilter   = L"effects (*.png)\0*.png;\0All files (*.*)\0*.*\0";
		ofn.lpstrDefExt   = L"png";
		dirname = dir_get_value(DIRMAP_ARTPATH);
		break;
	case FILETYPE_SHADER_FILES :
		ofn.lpstrFilter   = L"shaders (*.vsh)\0*.vsh;\0";
		ofn.lpstrDefExt   = L"vsh";
		dirname = dir_get_value(DIRMAP_HLSLPATH); // + PATH_SEPARATOR + "hlsl";
//      ofn.lpstrTitle  = _T("Select a HLSL shader file");
		break;
	case FILETYPE_BGFX_FILES :
		ofn.lpstrFilter   = L"bgfx (*.json)\0*.json;\0All files (*.*)\0*.*\0";
		ofn.lpstrDefExt   = L"json";
		dirname = dir_get_value(DIRMAP_BGFX_PATH) + PATH_SEPARATOR + "chains";
		break;
	case FILETYPE_LUASCRIPT_FILES :
		ofn.lpstrFilter   = L"scripts (*.lua)\0*.lua;\0All files (*.*)\0*.*\0";
		ofn.lpstrDefExt   = L"lua";
		dirname = ".";
		break;
	default:
		return false;
	}

	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter    = 0;
	ofn.nFilterIndex      = 1;

	// convert the filename to UTF-8 and copy into buffer
	ofn.lpstrFile         = filename;
	ofn.nMaxFile          = sizeof(filename);

	ofn.lpstrFileTitle    = NULL;
	ofn.nMaxFileTitle     = 0;

	// Only want first directory
	size_t i = dirname.find(";");
	if (i != std::string::npos)
		dirname.resize(i);

	if (dirname.empty())
		dirname = ".";

	ofn.lpstrInitialDir   = mui_wcstring_from_utf8(dirname.c_str());

	ofn.lpstrTitle        = NULL;
	ofn.Flags             = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	ofn.nFileOffset       = 0;
	ofn.nFileExtension    = 0;
	ofn.lCustData         = 0;
	ofn.lpfnHook          = NULL;
	ofn.lpTemplateName    = NULL;

	bool success = cfd(&ofn);
	if (success)
	{
		//printf("got filename %s nFileExtension %u\n",filename,ofn.nFileExtension);fflush(stdout);
		/*GetDirectory(filename,last_directory,(last_directory));*/
	}

	(void)mui_wcscpy(filename, ofn.lpstrFile);

	return success;
}


void SetStatusBarText(int part_index, const char *message)
{
	std::unique_ptr<wchar_t[]> w_message(mui_wcstring_from_utf8(message));
	if( !w_message)
		return;
	(void)windows::send_message(hStatusBar, SB_SETTEXTW, MAKEWPARAM(part_index,NULL), (LPARAM)w_message.get());
}


void SetStatusBarTextF(int part_index, const char *fmt, ...)
{
	char buf[256];
	va_list va;

	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	va_end(va);

	SetStatusBarText(part_index, buf);
}


static void CLIB_DECL ATTR_PRINTF(1,2) MameMessageBox(const char *fmt, ...)
{
	char buf[2048];
	va_list va;

	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	va_end(va);

	std::unique_ptr<char[]> utf8_mameuiname(mui_utf8_from_wcstring(&MAMEUINAME[0]));
	MessageBoxA(GetMainWindow(), buf, utf8_mameuiname.get(), MB_OK | MB_ICONERROR);
}


static void CLIB_DECL MameMessageBoxW(const wchar_t *fmt, ...)
{
	wchar_t buf[1024];
	va_list va;

	va_start(va, fmt);
	vswprintf(buf, fmt, va);
	dialog_boxes::message_box(GetMainWindow(), buf, &MAMEUINAME[0], MB_OK | MB_ICONERROR);
	va_end(va);
}


static void MamePlayGameWithOptions(int nGame, std::shared_ptr<play_options> playopts)
{
	m_lock = true;
	if (g_pJoyGUI)
		KillTimer(hMain, JOYGUI_TIMER);

	if (GetCycleScreenshot() > 0)
		KillTimer(hMain, SCREENSHOT_TIMER);

	in_emulation = true;

	DWORD dwExitCode = RunMAME(nGame, playopts);
	if (dwExitCode == 0)
	{
		IncrementPlayCount(nGame);
		(void)ListView_RedrawItems(hwndList, GetSelectedPick(), GetSelectedPick());

		// re-sort if sorting on # of times played
		if (GetSortColumn() == COLUMN_PLAYED)
			Picker_Sort(hwndList);

		UpdateStatusBar();

	}
	
	(void)windows::show_window(hMain, SW_SHOW);
	(void)SetFocus(hwndList);

	in_emulation = false;

	if (g_pJoyGUI)
		SetTimer(hMain, JOYGUI_TIMER, JOYGUI_MS, NULL);

	if (GetCycleScreenshot() > 0)
		SetTimer(hMain, SCREENSHOT_TIMER, GetCycleScreenshot()*1000, NULL); //scale to seconds
}


static void MamePlayBackGame()
{
	wchar_t wcs_filepath[MAX_PATH]{};
	int nGame = Picker_GetSelectedItem(hwndList);

	if (nGame != -1)
	{
		const char* driver_name = driver_list::driver(nGame).name;
		std::unique_ptr<wchar_t[]> utf8_to_wcs(mui_wcstring_from_utf8(driver_name));
		(void)mui_wcscpy(wcs_filepath, utf8_to_wcs.get());
	}

	if (CommonFileDialog(GetOpenFileNameW, wcs_filepath, FILETYPE_INPUT_FILES))
	{
		std::shared_ptr<play_options> playopts;
		wchar_t wcs_drive[_MAX_DRIVE];
		wchar_t wcs_dir[_MAX_DIR];
		wchar_t wcs_bare_filename[_MAX_FNAME];
		wchar_t wcs_ext[_MAX_EXT];
		wchar_t wcs_path[MAX_PATH];
		wchar_t wcs_filename[MAX_PATH];

		_wsplitpath(wcs_filepath, wcs_drive, wcs_dir, wcs_bare_filename, wcs_ext);

		wsprintf(wcs_path,L"%s%s",wcs_drive,wcs_dir);
		wsprintf(wcs_filename,L"%s%s",wcs_bare_filename,wcs_ext);
		const size_t path_size = mui_wcslen(wcs_path) - 1;
		if (wcs_path[path_size] == L'\\')
			wcs_path[path_size] = L'\0'; // take off trailing back slash

		std::unique_ptr<char[]> utf8_filename(mui_utf8_from_wcstring(wcs_filename));
		emu_file pPlayBack(MameUIGlobal().input_directory(), OPEN_FLAG_READ);
		std::error_condition fileerr = pPlayBack.open(utf8_filename.get());
		if (fileerr)
		{
			MameMessageBoxW(L"Could not open '%s' as a valid input file.", wcs_filepath);
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
		nGame = -1;
		for (size_t i = 0; i < driver_list::total(); i++) // find game and play it
		{
			if (driver_list::driver(i).name == sysname)
			{
				nGame = i;
				break;
			}
		}
		if (nGame == -1)
		{
			MameMessageBox("Game \"%s\" cannot be found", sysname.c_str());
			return;
		}

		playopts = std::make_shared<play_options>();
		playopts->playback = utf8_filename.get();
		playopts_apply = 0x57;
		MamePlayGameWithOptions(nGame, playopts);
	}
}


static void MameLoadState()
{
	wchar_t wcs_filepath[MAX_PATH]{};
	wchar_t selected_filename[MAX_PATH];
	int nGame = Picker_GetSelectedItem(hwndList);

	if (nGame != -1)
	{
		std::unique_ptr<wchar_t[]> wcs_driver_name(mui_wcstring_from_utf8(driver_list::driver(nGame).name));
		(void)mui_wcscpy(wcs_filepath, wcs_driver_name.get());
		(void)mui_wcscpy(selected_filename, wcs_driver_name.get());
	}
	if (CommonFileDialog(GetOpenFileNameW, wcs_filepath, FILETYPE_SAVESTATE_FILES))
	{
		wchar_t wcs_drive[_MAX_DRIVE];
		wchar_t wcs_dir[_MAX_DIR];
		wchar_t wcs_ext[_MAX_EXT];
		wchar_t path[MAX_PATH];
		wchar_t wcs_filename[MAX_PATH];
		wchar_t wcs_bare_filename[_MAX_FNAME];

		_wsplitpath(wcs_filepath, wcs_drive, wcs_dir, wcs_bare_filename, wcs_ext);

		// parse path
		wsprintf(path, L"%s%s", wcs_drive, wcs_dir);
		wsprintf(wcs_filename, L"%s%s", wcs_bare_filename, wcs_ext);
		const size_t path_size = mui_wcslen(path) - 1;
		if (path[path_size] == L'\\')
			path[path_size] = L'\0'; // take off trailing back slash

		std::unique_ptr<char[]> utf8_filename(mui_utf8_from_wcstring(wcs_filename));

		emu_file pSaveState(MameUIGlobal().state_directory(), OPEN_FLAG_READ);
		if (pSaveState.open(utf8_filename.get()))
		{
			MameMessageBox("Could not open '%s' as a valid savestate file.", utf8_filename.get());
			return;
		}

		// call the MAME core function to check the save state file
		//int rc = state_manager::check_file(NULL, pSaveState, selected_filename, MameMessageBox);
		//if (rc)

		std::shared_ptr<play_options> playopts;

		playopts = std::make_shared<play_options>();
		playopts->state = utf8_filename.get();
		playopts_apply = 0x57;
		MamePlayGameWithOptions(nGame, playopts);
	}
}


static void MamePlayRecordGame()
{
	wchar_t wcs_filepath[MAX_PATH]{};
	int nGame = Picker_GetSelectedItem(hwndList);
	std::unique_ptr<wchar_t[]> wcs_driver_name(mui_wcstring_from_utf8(driver_list::driver(nGame).name));

	if (nGame != -1)
		(void)mui_wcscpy(wcs_filepath, wcs_driver_name.get());

	if (CommonFileDialog(GetSaveFileNameW, wcs_filepath, FILETYPE_INPUT_FILES))
	{
		wchar_t wcs_drive[_MAX_DRIVE];
		wchar_t wcs_dir[_MAX_DIR];
		wchar_t wcs_filename[_MAX_FNAME];
		wchar_t wcs_ext[_MAX_EXT];
		wchar_t path[MAX_PATH];

		_wsplitpath(wcs_filepath, wcs_drive, wcs_dir, wcs_filename, wcs_ext);

		wsprintf(path,L"%s%s",wcs_drive,wcs_dir);
		const size_t path_size = mui_wcslen(path) - 1;
		if (path[path_size] == L'\\')
			path[path_size] = L'\0'; // take off trailing back slash

		std::shared_ptr<play_options> playopts;

		playopts = std::make_shared<play_options>();
		wcscat(wcs_filename, L".inp");
		std::unique_ptr<char[]> utf8_filename(mui_utf8_from_wcstring(wcs_filename));
		playopts->record = utf8_filename.get();
		playopts_apply = 0x57;
		MamePlayGameWithOptions(nGame, playopts);
	}
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
		MamePlayGameWithOptions(nGame, playopts);
	}
}


static void MamePlayRecordWave()
{
	wchar_t wcs_filepath[MAX_PATH]{};
	int nGame = Picker_GetSelectedItem(hwndList);
	std::unique_ptr<wchar_t[]> wcs_driver_name(mui_wcstring_from_utf8(driver_list::driver(nGame).name));

	if (nGame != -1)
		(void)mui_wcscpy(wcs_filepath, wcs_driver_name.get());

	if (CommonFileDialog(GetSaveFileNameW, wcs_filepath, FILETYPE_WAVE_FILES))
	{
		wchar_t wcs_drive[_MAX_DRIVE];
		wchar_t wcs_dir[_MAX_DIR];
		wchar_t wcs_filename[_MAX_FNAME];
		wchar_t wcs_ext[_MAX_EXT];
		wchar_t path[MAX_PATH];

		_wsplitpath(wcs_filepath, wcs_drive, wcs_dir, wcs_filename, wcs_ext);

		wsprintf(path,L"%s%s",wcs_drive,wcs_dir);
		const size_t path_size = mui_wcslen(path) - 1;
		if (path[path_size] == L'\\')
			path[path_size] = L'\0'; // take off trailing back slash

		std::shared_ptr<play_options> playopts;
		playopts = std::make_shared<play_options>();
		wcscat(wcs_filename, L".wav");
		std::unique_ptr<char[]> utf8_filename(mui_utf8_from_wcstring(wcs_filename));

		playopts->wavwrite = utf8_filename.get();
		playopts_apply = 0x57;
		MamePlayGameWithOptions(nGame, playopts);
	}
}


static void MamePlayRecordMNG()
{
	wchar_t wcs_filepath[MAX_PATH]{};
	int nGame = Picker_GetSelectedItem(hwndList);
	std::unique_ptr<wchar_t[]> wcs_driver_name(mui_wcstring_from_utf8(driver_list::driver(nGame).name));

	if (nGame != -1)
		(void)mui_wcscpy(wcs_filepath, wcs_driver_name.get());

	if (CommonFileDialog(GetSaveFileNameW, wcs_filepath, FILETYPE_MNG_FILES))
	{
		wchar_t wcs_drive[_MAX_DRIVE];
		wchar_t wcs_dir[_MAX_DIR];
		wchar_t wcs_filename[_MAX_FNAME];
		wchar_t wcs_ext[_MAX_EXT];
		wchar_t path[MAX_PATH];

		_wsplitpath(wcs_filepath, wcs_drive, wcs_dir, wcs_filename, wcs_ext);

		wsprintf(path,L"%s%s",wcs_drive,wcs_dir);
		const size_t path_size = mui_wcslen(path) - 1;
		if (path[path_size] == L'\\')
			path[path_size] = L'\0'; // take off trailing back slash

		std::shared_ptr<play_options> playopts;
		playopts = std::make_shared<play_options>();
		wcscat(wcs_filename, L".mng");
		std::unique_ptr<char[]> utf8_filename(mui_utf8_from_wcstring(wcs_filename));

		playopts->mngwrite = utf8_filename.get();
		playopts_apply = 0x57;
		MamePlayGameWithOptions(nGame, playopts);
	}
}


static void MamePlayRecordAVI()
{
	wchar_t wcs_filepath[MAX_PATH]{};
	int nGame = Picker_GetSelectedItem(hwndList);
	std::unique_ptr<wchar_t[]> wcs_driver_name(mui_wcstring_from_utf8(driver_list::driver(nGame).name));

	if (nGame != -1)
		(void)mui_wcscpy(wcs_filepath, wcs_driver_name.get());

	if (CommonFileDialog(GetSaveFileNameW, wcs_filepath, FILETYPE_AVI_FILES))
	{
		wchar_t wcs_drive[_MAX_DRIVE];
		wchar_t wcs_dir[_MAX_DIR];
		wchar_t wcs_filename[_MAX_FNAME];
		wchar_t wcs_ext[_MAX_EXT];
		wchar_t path[MAX_PATH];

		_wsplitpath(wcs_filepath, wcs_drive, wcs_dir, wcs_filename, wcs_ext);

		wsprintf(path,L"%s%s",wcs_drive,wcs_dir);
		const size_t path_size = mui_wcslen(path) - 1;
		if (path[path_size] == '\\')
			path[path_size] = '\0'; // take off trailing back slash

		std::shared_ptr<play_options> playopts;
		playopts = std::make_shared<play_options>();
		wcscat(wcs_filename, L".avi");
		std::unique_ptr<char[]> utf8_filename(mui_utf8_from_wcstring(wcs_filename));
		playopts->aviwrite = utf8_filename.get();
		playopts_apply = 0x57;
		MamePlayGameWithOptions(nGame, playopts);
	}
}


// Toggle ScreenShot ON/OFF
static void ToggleScreenShot(void)
{
	UINT val = GetWindowPanes() ^ 8;
	bool show = BIT(val, 3);
	SetWindowPanes(val);
	UpdateScreenShot();

	// Redraw list view
	if (hbBackground && show)
		gdi::invalidate_rect(hwndList, NULL, false);
}

static void ToggleSoftware(void)
{
	UINT val = GetWindowPanes() ^ 4;
	bool show = BIT(val, 2);
	SetWindowPanes(val);
	UpdateSoftware();

	// Redraw list view
	if (hbBackground && show)
		gdi::invalidate_rect(hwndList, NULL, false);
}

static void AdjustMetrics(void)
{
	std::cout << "Adjust Metrics" << std::endl;
	// WM_SETTINGCHANGE also
	int xtraX  = windows::get_system_metrics(SM_CXFIXEDFRAME); // Dialog frame width
	int xtraY  = windows::get_system_metrics(SM_CYFIXEDFRAME); // Dialog frame height
	xtraY += windows::get_system_metrics(SM_CYMENUSIZE);       // Menu height
	xtraY += windows::get_system_metrics(SM_CYCAPTION);        // Caption Height
	int maxX   = windows::get_system_metrics(SM_CXSCREEN);     // Screen Width
	int maxY   = windows::get_system_metrics(SM_CYSCREEN);     // Screen Height

	TEXTMETRIC tm;
	HDC hDC = GetDC(hMain);
	GetTextMetricsW (hDC, &tm);

	// Convert MIN Width/Height from Dialog Box Units to pixels.
	MIN_WIDTH  = (int)((tm.tmAveCharWidth / 4.0) * DBU_MIN_WIDTH)  + xtraX;
	MIN_HEIGHT = (int)((tm.tmHeight / 8.0) * DBU_MIN_HEIGHT) + xtraY;
	(void)ReleaseDC(hMain, hDC);

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
				(void)ListView_SetBkColor(hWnd, windows::get_sys_color(COLOR_WINDOW));
				(void)list_view::set_text_color(hWnd, textColor);
			}
			else if (!mui_wcscmp(szClass, L"SysTreeView32"))
			{
				(void)TreeView_SetBkColor(hTreeView, windows::get_sys_color(COLOR_WINDOW));
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
	SetWindowPos(hMain, 0, area.x, area.y, area.width, area.height, SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOACTIVATE);
	std::cout << "Adjust Metrics: Finished" << std::endl;
}


int FindIconIndex(int nIconResource)
{
	for(size_t i = 0; g_iconData[i].icon_name; i++)
	{
		if (g_iconData[i].resource == nIconResource)
			return i;
	}
	return -1;
}

// not used
int FindIconIndexByName(const char *icon_name)
{
	for (size_t i = 0; g_iconData[i].icon_name; i++)
	{
		if (!mui_strcmp(g_iconData[i].icon_name, icon_name))
			return i;
	}
	return -1;
}


static int GetIconForDriver(int nItem)
{
	int iconRoms = 1;

	if (DriverUsesRoms(nItem))
	{
		int audit_result = GetRomAuditResults(nItem);
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
		if (DriverIsBroken(nItem))
			iconRoms = FindIconIndex(IDI_WIN_REDX);  // iconRoms now = 4
		else
		// Show imperfect if the ROMs are present and flagged as imperfect
		if (DriverIsImperfect(nItem))
			iconRoms = FindIconIndex(IDI_WIN_IMPERFECT); // iconRoms now = 5
		else
		// show clone icon if we have roms and game is working
		if (DriverIsClone(nItem))
			iconRoms = FindIconIndex(IDI_WIN_CLONE); // iconRoms now = 3
	}

	// if we have the roms, then look for a custom per-game icon to override
	// not 2, because this indicates F5 must be done; not 0, because this indicates roms are missing; only use 4 if user chooses it
	bool redx = GetOverrideRedX() & (iconRoms == 4);
	if (iconRoms == 1 || iconRoms == 3 || iconRoms == 5 || redx)
	{
		if (icon_index[nItem] == 0)
			AddDriverIcon(nItem,iconRoms);
		iconRoms = icon_index[nItem];
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
	(void)TreeView_HitTest(hTreeView,&hti);
	if ((hti.flags & TVHT_ONITEM) != 0)
		(void)tree_view::select_item(hTreeView, hti.hItem);

	HMENU hTreeMenu = LoadMenu(hInst,menus::make_int_resource(IDR_CONTEXT_TREE));

	InitTreeContextMenu(hTreeMenu);

	HMENU hMenu = GetSubMenu(hTreeMenu, 0);

	UpdateMenu(hMenu);

	TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hWnd,NULL);

	(void)DestroyMenu(hTreeMenu);

	return true;
}


static void GamePicker_OnBodyContextMenu(POINT pt)
{
	HMENU hMenuLoad = LoadMenu(hInst, menus::make_int_resource(IDR_CONTEXT_MENU));
	HMENU hMenu = GetSubMenu(hMenuLoad, 0);
	InitBodyContextMenu(hMenu);

	UpdateMenu(hMenu);

	TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hMain,NULL);

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

	HMENU hMenuLoad = LoadMenu(hInst, menus::make_int_resource(IDR_CONTEXT_SCREENSHOT));
	HMENU hMenu = GetSubMenu(hMenuLoad, 0);

	UpdateMenu(hMenu);

	TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hWnd,NULL);

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
		std::unique_ptr<wchar_t[]> wcs_description(mui_wcstring_from_utf8(description.c_str()));
		if( !wcs_description)
			return;

		std::wstring play_game_string = std::wstring(g_szPlayGameString) + wcs_description.get();

		mItem.cbSize = sizeof(mItem);
		mItem.fMask = MIIM_TYPE;
		mItem.fType = MFT_STRING;
		mItem.dwTypeData = &play_game_string[0];
		mItem.cch = mui_wcslen(mItem.dwTypeData);

		(void)SetMenuItemInfoW(hMenu, ID_FILE_PLAY, false, &mItem);

		(void)EnableMenuItem(hMenu, ID_CONTEXT_SELECT_RANDOM, MF_ENABLED);
	}
	else
	{
		(void)EnableMenuItem(hMenu, ID_FILE_PLAY, MF_GRAYED);
		(void)EnableMenuItem(hMenu, ID_FILE_PLAY_RECORD, MF_GRAYED);
		(void)EnableMenuItem(hMenu, ID_GAME_PROPERTIES, MF_GRAYED);
		(void)EnableMenuItem(hMenu, ID_CONTEXT_SELECT_RANDOM, MF_GRAYED);
	}

	if (lpFolder->m_dwFlags & F_CUSTOM)
	{
		(void)EnableMenuItem(hMenu,ID_CONTEXT_REMOVE_CUSTOM,MF_ENABLED);
		(void)EnableMenuItem(hMenu,ID_CONTEXT_RENAME_CUSTOM,MF_ENABLED);
	}
	else
	{
		(void)EnableMenuItem(hMenu,ID_CONTEXT_REMOVE_CUSTOM,MF_GRAYED);
		(void)EnableMenuItem(hMenu,ID_CONTEXT_RENAME_CUSTOM,MF_GRAYED);
	}
	//const char* pParent = GetFolderNameByID(lpFolder->m_nParent+1);

	if (lpFolder->m_dwFlags & F_INIEDIT)
		(void)EnableMenuItem(hMenu,ID_FOLDER_PROPERTIES,MF_ENABLED);
	else
		(void)EnableMenuItem(hMenu,ID_FOLDER_PROPERTIES,MF_GRAYED);

	(void)CheckMenuRadioItem(hMenu, ID_VIEW_TAB_ARTWORK, ID_VIEW_TAB_HISTORY,
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
			(void)EnableMenuItem(hMenu,ID_VIEW_TAB_ARTWORK + i,MF_BYCOMMAND | MF_ENABLED);
		else
			(void)EnableMenuItem(hMenu,ID_VIEW_TAB_ARTWORK + i,MF_BYCOMMAND | MF_GRAYED);

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

	if (GetMenuItemInfoW(hMenu,3,TRUE,&mii) == false)
	{
		std::cout << "can't find show folders context menu" << std::endl;
		return;
	}

	if (mii.hSubMenu == NULL)
	{
		std::cout << "can't find submenu for show folders context menu" << std::endl;
		return;
	}

	hMenu = mii.hSubMenu;

	for (size_t i=0; g_folderData[i].m_lpTitle; i++)
	{
		if (!g_folderData[i].m_process)
		{
			std::unique_ptr<wchar_t[]> wcs_title(mui_wcstring_from_utf8(g_folderData[i].m_lpTitle));
			if( !wcs_title)
				return;

			mii.fMask = MIIM_TYPE | MIIM_ID;
			mii.fType = MFT_STRING;
			mii.dwTypeData = const_cast<wchar_t*>(wcs_title.get());
			mii.cch = mui_wcslen(mii.dwTypeData);
			mii.wID = ID_CONTEXT_SHOW_FOLDER_START + g_folderData[i].m_nFolderId;

			// menu in resources has one empty item (needed for the submenu to setup properly)
			// so overwrite this one, append after
			if (i == 0)
				(void)SetMenuItemInfoW(hMenu,ID_CONTEXT_SHOW_FOLDER_START,FALSE,&mii);
			else
				(void)InsertMenuItemW(hMenu,i,FALSE,&mii);
		}
	}
}


void InitBodyContextMenu(HMENU hBodyContextMenu)
{
	int nCurrentGame = Picker_GetSelectedItem(hwndList);
	if (nCurrentGame < 0)
		return;

	wchar_t tmp[256];
	MENUITEMINFOW mii;
	mii = {};
	mii.cbSize = sizeof(mii);

	if (GetMenuItemInfoW(hBodyContextMenu,ID_FOLDER_SOURCEPROPERTIES,FALSE,&mii) == false)
	{
		std::cout << "can't find show folders context menu" << std::endl;
		return;
	}
	std::unique_ptr<wchar_t[]> utf8_to_wcs(mui_wcstring_from_utf8(GetDriverFilename(nCurrentGame)));
	_snwprintf(tmp,std::size(tmp),L"Properties for %s", utf8_to_wcs.get());
	mii.fMask = MIIM_TYPE | MIIM_ID;
	mii.fType = MFT_STRING;
	mii.dwTypeData = tmp;
	mii.cch = mui_wcslen(mii.dwTypeData);
	mii.wID = ID_FOLDER_SOURCEPROPERTIES;

	// menu in resources has one default item
	// so overwrite this one
	(void)SetMenuItemInfoW(hBodyContextMenu,ID_FOLDER_SOURCEPROPERTIES,FALSE,&mii);
	(void)EnableMenuItem(hBodyContextMenu, ID_FOLDER_VECTORPROPERTIES, DriverIsVector(nCurrentGame) ? MF_ENABLED : MF_GRAYED);
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
				PaintBackgroundImage(hWnd, NULL, p.x, p.y);
				// to ensure our parent procedure repaints the whole client area
				gdi::invalidate_rect(hWnd, NULL, false);
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

				if (IsWindowVisible(dialog_boxes::get_dlg_item(hMain,IDC_HISTORY)))
				{
					// don't draw over this window
					(void)windows::get_window_rect(dialog_boxes::get_dlg_item(hMain,IDC_HISTORY),&nodraw_rect);
					gdi::map_window_points(HWND_DESKTOP,hWnd,(LPPOINT)&nodraw_rect,2);
					nodraw_region = gdi::create_rect_rgn_indirect(&nodraw_rect);
					CombineRgn(region,region,nodraw_region,RGN_DIFF);
					(void)DeleteObject(nodraw_region);
				}

				if (IsWindowVisible(dialog_boxes::get_dlg_item(hMain,IDC_SSPICTURE)))
				{
					// don't draw over this window
					(void)windows::get_window_rect(dialog_boxes::get_dlg_item(hMain,IDC_SSPICTURE),&nodraw_rect);
					gdi::map_window_points(HWND_DESKTOP,hWnd,(LPPOINT)&nodraw_rect,2);
					nodraw_region = gdi::create_rect_rgn_indirect(&nodraw_rect);
					CombineRgn(region,region,nodraw_region,RGN_DIFF);
					(void)DeleteObject(nodraw_region);
				}

				PaintBackgroundImage(hWnd,region,p.x,p.y);

				(void)DeleteObject(region);

				// to ensure our parent procedure repaints the whole client area
				gdi::invalidate_rect(hWnd, NULL, false);

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
			CombineRgn(region2,region2,region1,RGN_DIFF);
			HBRUSH holdBrush = (HBRUSH)gdi::select_object(hdc, hBrush);
			FillRgn(hdc,region2, hBrush );
			(void)gdi::select_object(hdc, holdBrush);
			(void)gdi::delete_brush(hBrush);
			SetStretchBltMode(hdc,STRETCH_HALFTONE);
			StretchBlt(hdc,nBordersize,nBordersize,rect.right-rect.left,rect.bottom-rect.top,hdc_temp,0,0,width,height,SRCCOPY);
			(void)gdi::select_object(hdc_temp,old_bitmap);
			(void)gdi::delete_dc(hdc_temp);
			(void)DeleteObject(region1);
			(void)DeleteObject(region2);
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
	himl_drag = ListView_CreateDragImage(hwndList,pnmv->iItem,&pt);

	// Start the drag operation.
	ImageList_BeginDrag(himl_drag, 0, 0, 0);

	pt = pnmv->ptAction;
	(void)gdi::client_to_screen(hwndList,&pt);
	ImageList_DragEnter(GetDesktopWindow(),pt.x,pt.y);

	// Hide the mouse cursor, and direct mouse input to the parent window.
	SetCapture(hMain);

	prev_drag_drop_target = NULL;

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

	gdi::map_window_points(GetDesktopWindow(),hTreeView,&pt,1);

	tvht.pt = pt;
	htiTarget = TreeView_HitTest(hTreeView,&tvht);

	if (htiTarget != prev_drag_drop_target)
	{
		ImageList_DragShowNolock(FALSE);
		if (htiTarget != NULL)
			(void)TreeView_SelectDropTarget(hTreeView, htiTarget);
		else
			(void)TreeView_SelectDropTarget(hTreeView, NULL);
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

	ReleaseCapture();

	ImageList_DragLeave(hwndList);
	ImageList_EndDrag();
	ImageList_Destroy(himl_drag);

	(void)TreeView_SelectDropTarget(hTreeView, NULL);

	g_listview_dragging = false;

	// see where the game was dragged

	pt.x = p.x;
	pt.y = p.y;

	gdi::map_window_points(hMain,hTreeView,&pt,1);

	tvht.pt = pt;
	htiTarget = TreeView_HitTest(hTreeView,&tvht);
	if (htiTarget == NULL)
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
	HTREEITEM htree = tree_view::get_selection(hTreeView);
	if(htree)
	{
		TVITEM tvi;
		tvi.hItem = htree;
		tvi.mask = TVIF_PARAM;
		tree_view::get_item(hTreeView,&tvi);
		return (LPTREEFOLDER)tvi.lParam;
	}
	return NULL;
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
		//hSmall_icon = TreeView_GetImageList(hTreeView,(int)tvi.iImage);
		hSmall_icon = NULL;
		return ImageList_GetIcon(hSmall_icon, tvi.iImage, ILD_TRANSPARENT);
	}
	return NULL;
}
#pragma GCC diagnostic error "-Wunused-but-set-variable"


// Updates all currently displayed Items in the List with the latest Data*/
void UpdateListView(void)
{
	if ((GetViewMode() == VIEW_GROUPED) || (GetViewMode() == VIEW_DETAILS))
	{
		int nFirstItemIndex = ListView_GetTopIndex(hwndList),
			nLastItemIndex = nFirstItemIndex + ListView_GetCountPerPage(hwndList);

		(void)ListView_RedrawItems(hwndList, nFirstItemIndex, nLastItemIndex);
	}
}


static void CalculateBestScreenShotRect(HWND hWnd, RECT *pRect, bool restrict_height)
{
	int destX=0, destY=0;
	int destW=0, destH=0;
	RECT rect;
	// for scaling
	int x=0, y=0;
	double scale=0;
	bool bReduce = false;

	(void)windows::get_client_rect(hWnd, &rect);

	// Scale the bitmap to the frame specified by the passed in hwnd
	if (ScreenShotLoaded())
	{
		x = GetScreenShotWidth();
		y = GetScreenShotHeight();
	}
	else
	{
		BITMAP bmp;
		(void)gdi::get_object(hMissing_bitmap, sizeof(BITMAP), &bmp);
		x = bmp.bmWidth;
		y = bmp.bmHeight;
	}
	int rWidth  = (rect.right  - rect.left);
	int rHeight = (rect.bottom - rect.top);

	// Limit the screen shot to max height of 264
	if (restrict_height == true && rHeight > 264)
	{
		rect.bottom = rect.top + 264;
		rHeight = 264;
	}

	// If the bitmap does NOT fit in the screenshot area
	if ((x > rWidth - 10 || y > rHeight - 10) || GetStretchScreenShotLarger())
	{
		rect.right  -= 10;
		rect.bottom -= 10;
		rWidth  -= 10;
		rHeight -= 10;
		bReduce = true;
		// Try to scale it properly
		//  assumes square pixels, doesn't consider aspect ratio
		if (x > y)
			scale = (double)rWidth / x;
		else
			scale = (double)rHeight / y;

		destW = (int)(x * scale);
		destH = (int)(y * scale);

		// If it's still too big, scale again
		if (destW > rWidth || destH > rHeight)
		{
			if (destW > rWidth)
				scale = (double)rWidth / destW;
			else
				scale = (double)rHeight / destH;

			destW = (int)(destW * scale);
			destH = (int)(destH * scale);
		}
	}
	else
	{
		// Use the bitmaps size if it fits
		destW = x;
		destH = y;
	}

	destX = ((rWidth  - destW) / 2);
	destY = ((rHeight - destH) / 2);

	if (bReduce)
	{
		destX += 5;
		destY += 5;
	}

	int nBorder = GetScreenshotBorderSize();

	if( destX > nBorder+1)
		pRect->left = destX - nBorder;
	else
		pRect->left = 2;

	if( destY > nBorder+1)
		pRect->top = destY - nBorder;
	else
		pRect->top = 2;

	if( rWidth >= destX + destW + nBorder)
		pRect->right = destX + destW + nBorder;
	else
		pRect->right = rWidth - pRect->left;

	if( rHeight >= destY + destH + nBorder)
		pRect->bottom = destY + destH + nBorder;
	else
		pRect->bottom = rHeight - pRect->top;
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
		SetMenu(hMain, LoadMenu(hInst,menus::make_int_resource(IDR_UI_MENU)));

		// Refresh the checkmarks
		(void)menus::check_menu_item(hMainMenu, ID_VIEW_FOLDERS, BIT(GetWindowPanes(), 0) ? MF_CHECKED : MF_UNCHECKED);
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
		SetMenu(hMain,NULL);

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

// End of source file

std::string longdots(std::string incoming, uint16_t howmany)
{
	// change all newlines to spaces
	for (size_t i = 0; i < incoming.size(); i++)
		if (incoming[i] == '\n')
			incoming[i] = ' ';
	// Now assume all is ok
	std::string outgoing = incoming;
	// But if it's too long, replace the excess with dots
	if ((howmany > 5) && (incoming.length() > howmany))
		outgoing = incoming.substr(0, howmany) + "...";
	return outgoing;
}
