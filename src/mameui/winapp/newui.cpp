// For licensing and usage information, read docs/winui_license.txt
//************************************************************************************************
// MASTER
//
//  newui.cpp - This is the NEWUI Windows dropdown menu system
//
//  known bugs:
//  -  Unable to modify keyboard or joystick. Last known to be working in 0.158 .
//     Need to use the default UI.
//
//
//************************************************************************************************

// standard C++ headers
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

// standard windows headers
#include "winapi_common.h"

// MAME headers

// shlwapi.h pulls in objbase.h which defines interface as a struct which conflict with a template declaration in device.h
#ifdef interface 
#undef interface // undef interface which is for COM interfaces
#endif
#include "emu.h"
#include "imagedev/cassette.h"
#include "softlist_dev.h"
#define interface struct

#include "path.h"
#include "strconv.h"

#include "emuopts.h"
#include "uiinput.h"
#include "iptseqpoll.h"
#include "natkeyboard.h"
#include "mame.h"
#include "ui/ui.h"
#include "window.h"
#include "winmain.h"

// MAMEUI headers
#include "mui_cstr.h"
#include "mui_wcstr.h"
#include "mui_wcstrconv.h"
#include "mui_stringtokenizer.h"

#include "data_access_storage.h"
#include "dialog_boxes.h"
#include "windows_gdi.h"
#include "menus_other_res.h"
#include "windows_controls.h"
#include "windows_input.h"
#include "windows_shell.h"
#include "system_services.h"
#include "windows_messages.h"

#include "newuires.h"

#include "newui.h"

using namespace mameui::winapi;
using namespace mameui::winapi::controls;
using namespace mameui::util;

#define SEQWM_SETFOCUS  (WM_APP + 0)
#define SEQWM_KILLFOCUS (WM_APP + 1)

using dialog_box = struct dialog_box;
using dialog_info_trigger = struct dialog_info_trigger;
using dialog_layout = struct dialog_layout;
using dialog_object_pool = struct dialog_object_pool;
using seqselect_info = struct seqselect_info;
using win_open_file_name = struct win_open_file_name;

using dialog_itemchangedproc = void (*)(dialog_box* dialog, HWND dlgitem, void* changed_param);
using dialog_itemstoreval = void (*)(void* param, int val);
using dialog_notification = void (*)(dialog_box* dialog, HWND dlgwnd, NMHDR* notification, void* param);
using trigger_function = LRESULT(*)(dialog_box* dialog, HWND dlgwnd, UINT message, WPARAM wparam, LPARAM lparam);

using seqselect_state = enum seqselect_state
{
	SEQSELECT_STATE_NOT_POLLING,
	SEQSELECT_STATE_POLLING,
	SEQSELECT_STATE_POLLING_COMPLETE
};

using dialog_trigger_flag = enum dialog_trigger_flag
{
	TRIGGER_INITDIALOG = 1,
	TRIGGER_APPLY = 2,
	TRIGGER_CHANGED = 4
};

using win_file_dialog_type = enum win_file_dialog_type
{
	WIN_FILE_DIALOG_OPEN = 1,
	WIN_FILE_DIALOG_SAVE
};

struct win_open_file_name
{
	win_file_dialog_type type;     // type of file dialog
	HWND owner;                    // owner of the dialog
	HINSTANCE instance;            // instance
	const char *filter;            // pipe char ("|") delimited strings
	DWORD filter_index;            // index into filter
	char filename[MAX_PATH];       // filename buffer
	const char *initial_directory; // initial directory for dialog
	DWORD flags;                   // standard flags
	LPARAM custom_data;            // custom data for dialog hook
	LPOFNHOOKPROC hook;            // custom dialog hook
	LPCTSTR template_name;         // custom dialog template
};

struct dialog_layout
{
	short label_width;
	short combo_width;
};


struct dialog_info_trigger
{
	dialog_info_trigger *next;
	WORD dialog_item;
	WORD trigger_flags;
	UINT message;
	WPARAM wparam;
	LPARAM lparam;
	void (*storeval)(void *param, int val);
	void *storeval_param;
	trigger_function trigger_proc;
};

struct dialog_object_pool
{
	HGDIOBJ objects[16];
};

struct dialog_box
{
	std::vector<uint8_t> buffer;
	size_t handle_size;
	dialog_info_trigger *trigger_first;
	dialog_info_trigger *trigger_last;
	WORD item_count;
	WORD size_x, size_y;
	WORD pos_x, pos_y;
	WORD cursize_x, cursize_y;
	WORD home_y;
	DWORD style;
	int combo_string_count;
	int combo_default_value;
	//object_pool *mempool;
	dialog_object_pool *objpool;
	const dialog_layout *layout;

	// singular notification callback; hack
	UINT notify_code;
	dialog_notification notify_callback;
	void *notify_param;
};

// this is the structure that gets associated with each input_seq edit box

struct seqselect_info
{
	WNDPROC oldwndproc;
	ioport_field *field; // pointer to the field
	ioport_field::user_settings settings; // the new settings
	input_seq *code; // the input_seq within settings
	WORD pos;
	bool is_analog;
	seqselect_state poll_state;
};



//============================================================
//  PARAMETERS
//============================================================

#define DIM_VERTICAL_SPACING    3
#define DIM_HORIZONTAL_SPACING  5
#define DIM_NORMAL_ROW_HEIGHT   10
#define DIM_COMBO_ROW_HEIGHT    12
#define DIM_SLIDER_ROW_HEIGHT   18
#define DIM_BUTTON_ROW_HEIGHT   12
#define DIM_EDIT_WIDTH          120
#define DIM_BUTTON_WIDTH        50
#define DIM_ADJUSTER_SCR_WIDTH  12
#define DIM_ADJUSTER_HEIGHT     12
#define DIM_SCROLLBAR_WIDTH     10
#define DIM_BOX_VERTSKEW        -3

#define DLGITEM_BUTTON          ((const WCHAR *) dlgitem_button)
#define DLGITEM_EDIT            ((const WCHAR *) dlgitem_edit)
#define DLGITEM_STATIC          ((const WCHAR *) dlgitem_static)
#define DLGITEM_LISTBOX         ((const WCHAR *) dlgitem_listbox)
#define DLGITEM_SCROLLBAR       ((const WCHAR *) dlgitem_scrollbar)
#define DLGITEM_COMBOBOX        ((const WCHAR *) dlgitem_combobox)

#define DLGTEXT_OK              "OK"
#define DLGTEXT_APPLY           "Apply"
#define DLGTEXT_CANCEL          "Cancel"

#define FONT_SIZE               8
#define FONT_FACE               L"Arial"

#define SCROLL_DELTA_LINE       10
#define SCROLL_DELTA_PAGE       100

#define LOG_WINMSGS             0
#define DIALOG_STYLE            WS_POPUP | WS_BORDER | WS_SYSMENU | DS_MODALFRAME | WS_CAPTION | DS_SETFONT
#define MAX_DIALOG_HEIGHT       200



//============================================================
//  LOCAL VARIABLES
//============================================================

static running_machine *Machine;         // HACK - please fix

static double pixels_to_xdlgunits;
static double pixels_to_ydlgunits;

static const dialog_layout default_layout = { 80, 140 };
static const WORD dlgitem_button[] = { 0xFFFF, 0x0080 };
static const WORD dlgitem_edit[] = { 0xFFFF, 0x0081 };
static const WORD dlgitem_static[] = { 0xFFFF, 0x0082 };
static const WORD dlgitem_listbox[] = { 0xFFFF, 0x0083 };
static const WORD dlgitem_scrollbar[] = { 0xFFFF, 0x0084 };
static const WORD dlgitem_combobox[] = { 0xFFFF, 0x0085 };
static int joystick_menu_setup = 0;
static std::string state_filename;
static std::map<std::string,std::string> slmap;
struct slot_data { std::string slotname; std::string optname; };
static std::map<int, slot_data> slot_map;
struct part_data { std::string part_name; device_image_interface *img; };
static std::map<int, part_data> part_map;


//============================================================
//  PROTOTYPES
//============================================================

dialog_box *win_dialog_init(const char *title, const dialog_layout *layout);
//static bool check_for_miscinput(running_machine &machine);
static bool get_softlist_info(HWND wnd, device_image_interface *img);
static bool win_file_dialog(running_machine &machine, HWND parent, win_file_dialog_type dlgtype, const char *filter, const char *initial_dir, char *filename);
static bool win_get_file_name_dialog(win_open_file_name *ofn);
static std::string win_dirname(const std::string_view filename);
static int dialog_add_scrollbar(dialog_box *dialog);
//static int dialog_add_single_seqselect(dialog_box *di, short x, short y, short cx, short cy, ioport_field *field, int is_analog, int seqtype);
static int dialog_add_trigger(dialog_box *di, WORD dialog_item, WORD trigger_flags, UINT message, trigger_function trigger_proc, WPARAM wparam, LPARAM lparam, void (*storeval)(void *param, int val), void *storeval_param);
static int dialog_write(dialog_box *di, const void *ptr, size_t sz, int align);
static int dialog_write_item(dialog_box *di, DWORD style, short x, short y, short width, short height, const char *str, const WCHAR *class_name, WORD *id);
static int dialog_write_item(dialog_box *di, DWORD style, short x, short y, short width, short height, const char *str, const wchar_t *class_name, WORD *id);
static int dialog_write_string(dialog_box *di, const wchar_t *str);
static int frameskip_level_count(running_machine &machine);
static int is_windowed(void);
static int port_type_is_analog(int type);
static int win_dialog_add_active_combobox(dialog_box *dialog, const char *item_label, int default_value, dialog_itemstoreval storeval, void *storeval_param, dialog_itemchangedproc changed, void *changed_param);
static int win_dialog_add_adjuster(dialog_box *dialog, const char *item_label, int default_value, int min_value, int max_value, bool is_percentage, dialog_itemstoreval storeval, void *storeval_param);
static int win_dialog_add_combobox(dialog_box *dialog, const char *item_label, int default_value, void (*storeval)(void *param, int val), void *storeval_param);
static int win_dialog_add_combobox_item(dialog_box *dialog, const char *item_label, int item_data);
//static int win_dialog_add_portselect(dialog_box *dialog, ioport_field *field);
static int win_dialog_add_standard_buttons(dialog_box *dialog);
static LRESULT adjuster_sb_setup(dialog_box *dialog, HWND sbwnd, UINT message, WPARAM wparam, LPARAM lparam);

static LRESULT dialog_combo_changed(dialog_box *dialog, HWND dlgitem, UINT message, WPARAM wparam, LPARAM lparam);
static LRESULT dialog_get_adjuster_value(dialog_box *dialog, HWND dialog_item, UINT message, WPARAM wparam, LPARAM lparam);
static LRESULT dialog_get_combo_value(dialog_box *dialog, HWND dialog_item, UINT message, WPARAM wparam, LPARAM lparam);
static LRESULT dialog_scrollbar_init(dialog_box *dialog, HWND dlgwnd, UINT message, WPARAM wparam, LPARAM lparam);
//static seqselect_info *get_seqselect_info(HWND editwnd);
static std::string newui_longdots(std::string incoming, uint16_t howmany);
static void add_filter_entry(std::string& dest, const std::string_view description, const std::string_view extensions);
static void after_display_dialog(running_machine &machine);
static void before_display_dialog(running_machine &machine);
static void build_generic_filter(device_image_interface *img, bool is_save, std::string &filter);
static void calc_dlgunits_multiple(void);
static void change_device(HWND wnd, device_image_interface *image, bool is_save);
static void copy_extension_list(std::string &dest, const std::string_view extensions);
static void customise_analogcontrols(running_machine &machine, HWND wnd);
static void customise_configuration(running_machine &machine, HWND wnd);
static void customise_dipswitches(running_machine &machine, HWND wnd);
//static void customise_input(running_machine &machine, HWND wnd, const char *title, u8 player_number, ioport_type_class input_type_class);
static void customise_joystick(running_machine &machine, HWND wnd, int joystick_num);
static void customise_keyboard(running_machine &machine, HWND wnd);
//static void customise_miscinput(running_machine &machine, HWND wnd);
static void customise_switches(running_machine &machine, HWND hwnd_parent, const char* title_string, ioport_type target_input_type);
static device_image_interface* decode_deviceoption(running_machine& machine, int command, int* devoption);
static void device_command(HWND wnd, device_image_interface *img, int devoption);
static void dialog_finish_control(dialog_box *di, short x, short y);
static void dialog_new_control(dialog_box *di, short *x, short *y);
static void dialog_prime(dialog_box *di);
static void dialog_prime(dialog_box *di);
static void dialog_trigger(HWND dlgwnd, WORD trigger_flags);
static void help_about_mess(HWND wnd);
static void help_about_thissystem(running_machine &machine, HWND wnd);
static void help_display(HWND wnd, const char *chapter);
static void load_item(HWND wnd, device_image_interface *img, bool is_save);

//static LRESULT seqselect_setup(dialog_box* dialog, HWND editwnd, UINT message, WPARAM wparam, LPARAM lparam);
//static void seqselect_settext(HWND editwnd);
//static void seqselect_start_read_from_main_thread(void *param);
//static void seqselect_stop_read_from_main_thread(void *param);

static HMENU find_sub_menu(HMENU menu, const char* menutext, bool create_sub_menu);
static void prepare_menus(HWND wnd);
static void setup_joystick_menu(running_machine &machine, HMENU menu_bar);
static void win_toggle_menubar(void);
static int win_setup_menus(running_machine& machine, HMODULE module, HMENU menu_bar);
static void set_menu_text(HMENU menu_bar, int command, const char* text);
static void set_command_state(HMENU menu_bar, UINT command, UINT state);

static LRESULT CALLBACK call_windowproc(WNDPROC wndproc, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
static INT_PTR CALLBACK adjuster_sb_wndproc(HWND sbwnd, UINT msg, WPARAM wparam, LPARAM lparam);
static INT_PTR CALLBACK dialog_proc(HWND dlgwnd, UINT msg, WPARAM wparam, LPARAM lparam);
//static INT_PTR CALLBACK seqselect_wndproc(HWND editwnd, UINT msg, WPARAM wparam, LPARAM lparam);

static void pause(running_machine &machine);

static void set_speed(running_machine &machine, int speed);
static void state_dialog(HWND wnd, win_file_dialog_type dlgtype, DWORD fileproc_flags, bool is_load, running_machine &machine);
static void state_load(HWND wnd, running_machine &machine);
static void state_save(running_machine &machine);
static void state_save_as(HWND wnd, running_machine &machine);
static void store_analogitem(void *param, int val, int selected_item);
static void store_centerdelta(void *param, int val);
static void store_delta(void *param, int val);
static void store_reverse(void *param, int val);
static void store_sensitivity(void *param, int val);
static void update_keyval(void *param, int val);
static void win_dialog_exit(dialog_box *dialog);
static void win_dialog_runmodal(running_machine &machine, HWND wnd, dialog_box *dialog);
static void win_scroll_window(HWND window, WPARAM wparam, int scroll_bar, int scroll_delta_line);
static void win_resource_module(HMODULE &output_resource_module);

//============================================================
//  PARAMETERS
//============================================================

#define ID_FRAMESKIP_0   10000
#define ID_DEVICE_0      11000
#define ID_JOYSTICK_0    12000
#define ID_VIDEO_VIEW_0  14000
#define ID_SWPART        15000
#define MAX_JOYSTICKS    (8)

enum
{
	DEVOPTION_OPEN,
	DEVOPTION_CREATE,
	DEVOPTION_CLOSE,
	DEVOPTION_ITEM,
	DEVOPTION_CASSETTE_PLAYRECORD,
	DEVOPTION_CASSETTE_STOPPAUSE,
	DEVOPTION_CASSETTE_PLAY,
	DEVOPTION_CASSETTE_RECORD,
	DEVOPTION_CASSETTE_REWIND,
	DEVOPTION_CASSETTE_FASTFORWARD,
	DEVOPTION_CASSETTE_MOTOR,
	DEVOPTION_CASSETTE_SOUND,
	DEVOPTION_MAX
};


//========================================================================
//  LOCAL STRING FUNCTIONS (these require free after being called)
//========================================================================

// This function truncates a long string, replacing the end with ...
static std::string newui_longdots(std::string incoming, uint16_t howmany)
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


//============================================================
//  win_get_file_name_dialog - sanitize all of the ugliness
//  in invoking GetOpenFileNameW() and dialog_boxes::get_save_filename()
//     called from win_file_dialog, state_dialog
//============================================================

static bool win_get_file_name_dialog(win_open_file_name *ofn)
{
	bool result = 0;
	bool dialog_result;
	OPENFILENAMEW os_ofn{};
	std::unique_ptr<wchar_t[]>  wcs_filter;
	std::unique_ptr<wchar_t[]>  wcs_file;
	std::unique_ptr<wchar_t[]>  wcs_initial_directory;

	// do we have to translate the filter?
	if (ofn->filter)
	{
		size_t i;

		std::unique_ptr<wchar_t[]> utf8_to_wcs(mui_utf16_from_utf8cstring(ofn->filter));
		if (!utf8_to_wcs)
			return result;

		// convert a pipe-char delimited string into a NUL delimited string
		wcs_filter = std::make_unique<wchar_t[]>(mui_wcslen(utf8_to_wcs.get()) + 2);
		for (i = 0; utf8_to_wcs[i] != L'\0'; i++)
			wcs_filter[i] = (utf8_to_wcs[i] != L'|') ? utf8_to_wcs[i] : L'\0';
		wcs_filter[i++] = L'\0';
		wcs_filter[i++] = L'\0';
	}

	// do we need to translate the file parameter?
	if (*(ofn->filename) != '\0')
	{

		std::unique_ptr<wchar_t[]> utf8_to_wcs(mui_utf16_from_utf8cstring(ofn->filename));
		if (!utf8_to_wcs)
			return result;

		wcs_file = std::make_unique<wchar_t[]>(MAX_PATH);
		(void)mui_wcscpy(wcs_file.get(), utf8_to_wcs.get());
	}

	// do we need to translate the initial directory?
	if (*(ofn->initial_directory) != '\0')
	{
		wcs_initial_directory = std::unique_ptr<wchar_t[]>(mui_utf16_from_utf8cstring(ofn->initial_directory));
		if (!wcs_initial_directory)
			return result;
	}

	// translate our custom structure to a native Win32 structure
	os_ofn.lStructSize = sizeof(OPENFILENAMEW);
	os_ofn.hwndOwner = ofn->owner;
	os_ofn.hInstance = ofn->instance;
	os_ofn.lpstrFilter = wcs_filter.get();
	os_ofn.nFilterIndex = ofn->filter_index;
	os_ofn.lpstrFile = wcs_file.get();
	os_ofn.nMaxFile = MAX_PATH;
	os_ofn.lpstrInitialDir = wcs_initial_directory.get();
	os_ofn.Flags = ofn->flags;
	os_ofn.lCustData = ofn->custom_data;
	os_ofn.lpfnHook = ofn->hook;
	os_ofn.lpTemplateName = ofn->template_name;

	// invoke the correct Win32 call
	switch(ofn->type)
	{
		case WIN_FILE_DIALOG_OPEN:
			dialog_result = dialog_boxes::get_open_filename(&os_ofn);
			break;

		case WIN_FILE_DIALOG_SAVE:
			dialog_result = dialog_boxes::get_save_filename(&os_ofn);
			break;

		default:
			// should not reach here
			dialog_result = false;
			break;
	}

	// copy data out
	ofn->filter_index = os_ofn.nFilterIndex;
	ofn->flags = os_ofn.Flags;

	// copy file back out into passed structure
	if (wcs_file)
	{
		std::unique_ptr<char[]> wcs_to_utf8(mui_utf8_from_utf16cstring(wcs_file.get()));
		if (!wcs_to_utf8)
			return result;

		mui_strcpy(ofn->filename, const_cast<const char*>(wcs_to_utf8.get()));
	}
	// we've completed the process
	result = dialog_result;

	return result;
}



//============================================================
//  win_scroll_window
//    called from dialog_proc
//============================================================

static void win_scroll_window(HWND window, WPARAM wparam, int scroll_bar, int scroll_delta_line)
{
	SCROLLINFO si{ sizeof(si) };
	int scroll_pos;

	// retrieve vital info about the scroll bar
	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
	scroll_bar::get_scroll_info(window, scroll_bar, &si);

	scroll_pos = si.nPos;

	// change scroll_pos in accordance with this message
	switch(LOWORD(wparam))
	{
		case SB_BOTTOM:
			scroll_pos = si.nMax;
			break;
		case SB_LINEDOWN:
			scroll_pos += scroll_delta_line;
			break;
		case SB_LINEUP:
			scroll_pos -= scroll_delta_line;
			break;
		case SB_PAGEDOWN:
			scroll_pos += scroll_delta_line;
			break;
		case SB_PAGEUP:
			scroll_pos -= scroll_delta_line;
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			scroll_pos = HIWORD(wparam);
			break;
		case SB_TOP:
			scroll_pos = si.nMin;
			break;
	}

	// max out the scroll bar value
	if (scroll_pos < si.nMin)
		scroll_pos = si.nMin;
	else if (scroll_pos > (si.nMax - si.nPage))
		scroll_pos = (si.nMax - si.nPage);

	// if the value changed, set the scroll position
	if (scroll_pos != si.nPos)
	{
		scroll_bar::set_scroll_pos(window, scroll_bar, scroll_pos, true);
		scroll_bar::scroll_window_ex(window, 0, si.nPos - scroll_pos, nullptr, nullptr, nullptr, nullptr, SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE);
	}
}


//============================================================
//  call_windowproc
//    called from adjuster_sb_wndproc, seqselect_wndproc
//============================================================

static LRESULT CALLBACK call_windowproc(WNDPROC wndproc, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (!hwnd)
		return 0;

	return windows::call_window_proc(wndproc, hwnd, msg, wparam, lparam);
}



//==========================================================================
//  dialog_write
//    called from dialog_write_string, win_dialog_init, dialog_write_item
//==========================================================================

static int dialog_write(dialog_box *di, const void *ptr, size_t sz, int align)
{
	if (!di || !ptr || align <= 0) return 1;

	// Calculate aligned base
	size_t base = di->handle_size;
	size_t aligned_base = (base + align - 1) & ~(align - 1);
	size_t new_size = aligned_base + sz;

	// Resize buffer if needed
	if (di->buffer.size() < new_size)
		di->buffer.resize(new_size, 0); // zero-init new memory

	// Copy new data into the aligned position
	std::memcpy(&di->buffer[aligned_base], ptr, sz);

	// Update size tracker
	di->handle_size = new_size;

	return 0;
}



//============================================================
//  dialog_write_string
//    called from win_dialog_init, dialog_write_item
//============================================================

static int dialog_write_string(dialog_box *di, const wchar_t *str)
{
	if (!str)
		str = L"";
	return dialog_write(di, str, (mui_wcslen(str) + 1) * sizeof(WCHAR), sizeof(WCHAR));
}




//============================================================
//  win_dialog_exit
//    called from win_dialog_init, calc_dlgunits_multiple, change_device, and all customise_input functions
//============================================================

static void win_dialog_exit(dialog_box *dialog)
{
	int i;
	dialog_object_pool *objpool;

	if (!dialog)
		std::cout << "No dialog passed to win_dialog_exit" << std::endl;

	assert(dialog);

	objpool = dialog->objpool;
	if (objpool)
	{
		for (i = 0; i < std::size(objpool->objects); i++)
			(void)gdi::delete_object(objpool->objects[i]);
	}

	delete dialog;
}



//===========================================================================
//  win_dialog_init
//    called from calc_dlgunits_multiple, and all customise_input functions
//===========================================================================

dialog_box *win_dialog_init(const char *title, const dialog_layout *layout)
{
	dialog_box *di;
	DLGTEMPLATE dlg_template;
	std::unique_ptr<const wchar_t[]> wcs_title;
	WORD w[2];
	int rc;

	// use default layout if not specified
	if (!layout)
		layout = &default_layout;

	// create the dialog structure
	di = new dialog_box();
	if (!di)
		goto error;

	di->layout = layout;
	di->style = DIALOG_STYLE;

	dlg_template = { di->style };
	dlg_template.x = 10;
	dlg_template.y = 10;
	if (dialog_write(di, &dlg_template, sizeof(dlg_template), 4))
		goto error;

	w[0] = 0;
	w[1] = 0;
	if (dialog_write(di, w, sizeof(w), 2))
		goto error;

	wcs_title = std::unique_ptr<const wchar_t[]>(mui_utf16_from_utf8cstring(title));
	rc = dialog_write_string(di, wcs_title.get());
	if (rc)
		goto error;

	// set the font, if necessary
	if (di->style & DS_SETFONT)
	{
		w[0] = FONT_SIZE;
		if (dialog_write(di, w, sizeof(w[0]), 2))
			goto error;
		if (dialog_write_string(di, FONT_FACE))
			goto error;
	}

	return di;

error:
	if (di)
		win_dialog_exit(di);
	return nullptr;
}


//============================================================
//  compute_dlgunits_multiple
//    called from dialog_scrollbar_init
//============================================================

static void calc_dlgunits_multiple(void)
{
	dialog_box *dialog = nullptr;
	short offset_x = 2048;
	short offset_y = 2048;
	const char *wnd_title = "Foo";
	WORD id;
	HWND dlg_window = nullptr;
	HWND child_window;
	RECT r;

	if ((pixels_to_xdlgunits == 0) || (pixels_to_ydlgunits == 0))
	{
		// create a bogus dialog
		dialog = win_dialog_init(nullptr, nullptr);
		if (!dialog)
			goto done;

		if (dialog_write_item(dialog, WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 0, offset_x, offset_y, wnd_title, DLGITEM_STATIC, &id))
			goto done;

		dialog_prime(dialog);
		dlg_window = dialog_boxes::create_dialog_indirect(nullptr, reinterpret_cast<LPCDLGTEMPLATEW>(dialog->buffer.data()), nullptr, nullptr, 0);
		child_window = dialog_boxes::get_dlg_item(dlg_window, id);

		(void)windows::get_window_rect(child_window, &r);
		pixels_to_xdlgunits = (double)(r.right - r.left) / offset_x;
		pixels_to_ydlgunits = (double)(r.bottom - r.top) / offset_y;

done:
		if (dialog)
			win_dialog_exit(dialog);
		if (dlg_window)
			windows::destroy_window(dlg_window);
	}
}



//============================================================
//  dialog_trigger
//    called from dialog_proc, file_dialog_hook
//============================================================

static void dialog_trigger(HWND dlgwnd, WORD trigger_flags)
{
	LRESULT result;
	HWND dialog_item;
	dialog_box *di;
	dialog_info_trigger *trigger;
	LONG_PTR l;

	l = windows::get_window_long_ptr(dlgwnd, GWLP_USERDATA);
	di = (dialog_box *) l;
	if (!di)
		std::cout << "Unexpected result of di in dialog_trigger" << std::endl;
	assert(di);
	for (trigger = di->trigger_first; trigger; trigger = trigger->next)
	{
		if (trigger->trigger_flags & trigger_flags)
		{
			if (trigger->dialog_item)
				dialog_item = dialog_boxes::get_dlg_item(dlgwnd, trigger->dialog_item);
			else
				dialog_item = dlgwnd;
			if (!dialog_item)
				std::cout << "Unexpected result of dialog_item in dialog_trigger" << std::endl;
			assert(dialog_item);
			result = 0;

			if (trigger->message)
				result =  windows::send_message(dialog_item, trigger->message, trigger->wparam, trigger->lparam);
			if (trigger->trigger_proc)
				result = trigger->trigger_proc(di, dialog_item, trigger->message, trigger->wparam, trigger->lparam);

			if (trigger->storeval)
				trigger->storeval(trigger->storeval_param, result);
		}
	}
}


//============================================================
//  dialog_proc
//    called from win_dialog_runmodal
//============================================================

static INT_PTR CALLBACK dialog_proc(HWND dlgwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	bool handled = true;

	switch (msg)
	{
	case WM_INITDIALOG:
	{
		windows::set_window_long_ptr(dlgwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(lparam));
		dialog_trigger(dlgwnd, TRIGGER_INITDIALOG);
		return 1; // TRUE
	}

	case WM_COMMAND:
	{
		WORD command = LOWORD(wparam);
		const HWND ctlwnd = reinterpret_cast<HWND>(lparam);
		auto ctl_text = std::unique_ptr<char[]>(windows::get_window_text_utf8(ctlwnd));
		if (!ctl_text)
		{
			handled = false;
			break;
		}

		if (!mui_strcmp(ctl_text.get(), DLGTEXT_OK))
			command = IDOK;
		else if (!mui_strcmp(ctl_text.get(), DLGTEXT_CANCEL))
			command = IDCANCEL;

		switch (command)
		{
		case IDOK:
			dialog_trigger(dlgwnd, TRIGGER_APPLY);
			[[fallthrough]];

		case IDCANCEL:
			dialog_boxes::end_dialog(dlgwnd, 0);
			break;

		default:
			handled = false;
			break;
		}
		break;
	}

	case WM_SYSCOMMAND:
		if (wparam == SC_CLOSE)
		{
			dialog_boxes::end_dialog(dlgwnd, 0);
		}
		else
		{
			handled = false;
		}
		break;

	case WM_VSCROLL:
		if (lparam)
		{
			// Message came from a scroll bar control; pass it along.
			windows::send_message(reinterpret_cast<HWND>(lparam), msg, wparam, lparam);
		}
		else
		{
			// Scroll the dialog.
			win_scroll_window(dlgwnd, wparam, SB_VERT, SCROLL_DELTA_LINE);
		}
		break;

	default:
		handled = false;
		break;
	}

	return static_cast<INT_PTR>(handled);
}


//=========================================================================================================================================================================================
//  dialog_write_item
//    called from calc_dlgunits_multiple, win_dialog_add_active_combobox, win_dialog_add_adjuster, dialog_add_single_seqselect, win_dialog_add_portselect, win_dialog_add_standard_buttons
//=========================================================================================================================================================================================

static int dialog_write_item(dialog_box *di, DWORD style, short x, short y, short width, short height, const char *str, const wchar_t *class_name, WORD *id)
{
	DLGITEMTEMPLATE item_template;
	UINT class_name_length;
	WORD w;
	int rc;

	item_template = { style };
	item_template.x = x;
	item_template.y = y;
	item_template.cx = width;
	item_template.cy = height;
	item_template.id = di->item_count + 1;

	if (dialog_write(di, &item_template, sizeof(item_template), 4))
		return 1;

	if (*class_name == 0xffff)
		class_name_length = 4;
	else
		class_name_length = (mui_wcslen(class_name) + 1) * sizeof(wchar_t);
	if (dialog_write(di, class_name, class_name_length, 2))
		return 1;

	std::unique_ptr<const wchar_t[]> wcs_str(mui_utf16_from_utf8cstring(str));
	rc = dialog_write_string(di, wcs_str.get());

	if (rc)
		return 1;

	w = 0;
	if (dialog_write(di, &w, sizeof(w), 2))
		return 1;

	di->item_count++;

	if (id)
		*id = di->item_count;
	return 0;
}



//==========================================================================================================================================================
//  dialog_add_trigger
//    called from dialog_add_scrollbar, win_dialog_add_active_combobox, win_dialog_add_combobox_item, win_dialog_add_adjuster, dialog_add_single_seqselect
//==========================================================================================================================================================

static int dialog_add_trigger(dialog_box *di, WORD dialog_item, WORD trigger_flags, UINT message, trigger_function trigger_proc,
	WPARAM wparam, LPARAM lparam, void (*storeval)(void *param, int val), void *storeval_param)
{
	if (!di)
		std::cout << "Unexpected result of di in dialog_add_trigger" << std::endl;
	assert(di);
	if (!trigger_flags)
		std::cout << "Unexpected result of trigger_flags in dialog_add_trigger" << std::endl;
	assert(trigger_flags);

	dialog_info_trigger *trigger = new(dialog_info_trigger);

	trigger->next = nullptr;
	trigger->trigger_flags = trigger_flags;
	trigger->dialog_item = dialog_item;
	trigger->message = message;
	trigger->trigger_proc = trigger_proc;
	trigger->wparam = wparam;
	trigger->lparam = lparam;
	trigger->storeval = storeval;
	trigger->storeval_param = storeval_param;

	if (di->trigger_last)
		di->trigger_last->next = trigger;
	else
		di->trigger_first = trigger;
	di->trigger_last = trigger;
	return 0;
}



//============================================================
//  dialog_scrollbar_init
//    called from dialog_add_scrollbar
//============================================================

static LRESULT dialog_scrollbar_init(dialog_box *dialog, HWND dlgwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	SCROLLINFO si;

	calc_dlgunits_multiple();

	si = { sizeof(si) };
	si.nMin  = pixels_to_ydlgunits * 0;
	si.nMax  = pixels_to_ydlgunits * dialog->size_y;
	si.nPage = pixels_to_ydlgunits * MAX_DIALOG_HEIGHT;
	si.fMask = SIF_PAGE | SIF_RANGE;

	(void)scroll_bar::set_scroll_info(dlgwnd, SB_VERT, &si, true);
	return 0;
}



//============================================================
//  dialog_add_scrollbar
//    called from dialog_prime
//============================================================

static int dialog_add_scrollbar(dialog_box *dialog)
{
	if (dialog_add_trigger(dialog, 0, TRIGGER_INITDIALOG, 0, dialog_scrollbar_init, 0, 0, nullptr, nullptr))
		return 1;

	dialog->style |= WS_VSCROLL;
	return 0;
}



//==============================================================================
//  dialog_prime
//    called from calc_dlgunits_multiple, win_dialog_runmodal, win_file_dialog
//==============================================================================

static void dialog_prime(dialog_box *di)
{
	DLGTEMPLATE *dlg_template;

	if (di->size_y > MAX_DIALOG_HEIGHT)
	{
		di->size_x += DIM_SCROLLBAR_WIDTH;
		dialog_add_scrollbar(di);
	}

	dlg_template = reinterpret_cast<DLGTEMPLATE*>(di->buffer.data());
	dlg_template->cdit = di->item_count;
	dlg_template->cx = di->size_x;
	dlg_template->cy = (di->size_y > MAX_DIALOG_HEIGHT) ? MAX_DIALOG_HEIGHT : di->size_y;
	dlg_template->style = di->style;
}



//============================================================
//  dialog_get_combo_value
//    called from win_dialog_add_active_combobox
//============================================================

static LRESULT dialog_get_combo_value(dialog_box *dialog, HWND dialog_item, UINT message, WPARAM wparam, LPARAM lparam)
{
	int idx;
	idx =  windows::send_message(dialog_item, CB_GETCURSEL, 0, 0);
	if (idx == CB_ERR)
		return 0;
	return  windows::send_message(dialog_item, CB_GETITEMDATA, idx, 0);
}



//============================================================
//  dialog_get_adjuster_value
//    called from win_dialog_add_adjuster
//============================================================

static LRESULT dialog_get_adjuster_value(dialog_box *dialog, HWND dialog_item, UINT message, WPARAM wparam, LPARAM lparam)
{
	constexpr size_t buf_size = 32;
	wchar_t buf[buf_size];
	wchar_t* buf_end = buf + buf_size - 1;

	windows::get_window_text(dialog_item, buf, buf_size);

	return std::wcstol(buf, &buf_end, 10);
}



//====================================================================================================
//  dialog_new_control
//    called from win_dialog_add_active_combobox, win_dialog_add_adjuster, win_dialog_add_portselect
//====================================================================================================

static void dialog_new_control(dialog_box *di, short *x, short *y)
{
	*x = di->pos_x + DIM_HORIZONTAL_SPACING;
	*y = di->pos_y + DIM_VERTICAL_SPACING;
}



//====================================================================================================
//  dialog_finish_control
//    called from win_dialog_add_active_combobox, win_dialog_add_adjuster, win_dialog_add_portselect
//====================================================================================================

static void dialog_finish_control(dialog_box *di, short x, short y)
{
	di->pos_y = y;

	// update dialog size
	if (x > di->size_x)
		di->size_x = x;
	if (y > di->size_y)
		di->size_y = y;
	if (x > di->cursize_x)
		di->cursize_x = x;
	if (y > di->cursize_y)
		di->cursize_y = y;
}



//============================================================
//  dialog_combo_changed
//    called from win_dialog_add_active_combobox
//============================================================

static LRESULT dialog_combo_changed(dialog_box *dialog, HWND dlgitem, UINT message, WPARAM wparam, LPARAM lparam)
{
	dialog_itemchangedproc changed = (dialog_itemchangedproc) wparam;
	changed(dialog, dlgitem, (void *) lparam);
	return 0;
}



//============================================================
//  win_dialog_add_active_combobox
//    called from win_dialog_add_combobox
//       dialog = handle of dialog box?
//       item_label = name of key
//       default_value = current value of key
//       storeval = function to handle changed key
//       storeval_param = ?
//       changed = ?
//       changed_param = ?
//       rc = return code (1 = failure)
//============================================================

static int win_dialog_add_active_combobox(dialog_box *dialog, const char *item_label, int default_value,
	dialog_itemstoreval storeval, void *storeval_param, dialog_itemchangedproc changed, void *changed_param)
{
	int rc = 1;
	short x;
	short y;

	dialog_new_control(dialog, &x, &y);

	// put name of key on the left
	if (dialog_write_item(dialog, WS_CHILD | WS_VISIBLE | SS_LEFT, x, y, dialog->layout->label_width, DIM_COMBO_ROW_HEIGHT, item_label, DLGITEM_STATIC, nullptr))
		goto done;

	y += DIM_BOX_VERTSKEW;

	x += dialog->layout->label_width + DIM_HORIZONTAL_SPACING;
	if (dialog_write_item(dialog, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST,
			x, y, dialog->layout->combo_width, DIM_COMBO_ROW_HEIGHT * 8, nullptr, DLGITEM_COMBOBOX, nullptr))
		goto done;
	dialog->combo_string_count = 0;
	dialog->combo_default_value = default_value; // show current value

	// add the trigger invoked when the apply button is pressed
	if (dialog_add_trigger(dialog, dialog->item_count, TRIGGER_APPLY, 0, dialog_get_combo_value, 0, 0, storeval, storeval_param))
		goto done;

	// if appropriate, add the optional changed trigger
	if (changed)
		if (dialog_add_trigger(dialog, dialog->item_count, TRIGGER_INITDIALOG | TRIGGER_CHANGED, 0, dialog_combo_changed, (WPARAM) changed, (LPARAM) changed_param, nullptr, nullptr))
			goto done;

	x += dialog->layout->combo_width + DIM_HORIZONTAL_SPACING;
	y += DIM_COMBO_ROW_HEIGHT + DIM_VERTICAL_SPACING * 2;

	dialog_finish_control(dialog, x, y);
	rc = 0;

done:
	return rc;
}



//============================================================
//  win_dialog_add_combobox
//    called from customise_switches, customise_analogcontrols
//============================================================

static int win_dialog_add_combobox(dialog_box *dialog, const char *item_label, int default_value, void (*storeval)(void *param, int val), void *storeval_param)
{
	return win_dialog_add_active_combobox(dialog, item_label, default_value, storeval, storeval_param, nullptr, nullptr);
}



//============================================================
//  win_dialog_add_combobox_item
//    called from customise_switches, customise_analogcontrols
//============================================================

static int win_dialog_add_combobox_item(dialog_box *dialog, const char *item_label, int item_data)
{
	// create our own copy of the string
	size_t newsize = mui_strlen(item_label) + 1;
	wchar_t * t_item_label = new wchar_t[newsize];
	//size_t convertedChars = 0;
	//mbstowcs_s(&convertedChars, t_item_label, newsize, item_label, _TRUNCATE);
	mbstowcs(t_item_label, item_label, newsize);

	if (dialog_add_trigger(dialog, dialog->item_count, TRIGGER_INITDIALOG, CB_ADDSTRING, nullptr, 0, (LPARAM) t_item_label, nullptr, nullptr))
		return 1;
	dialog->combo_string_count++;
	if (dialog_add_trigger(dialog, dialog->item_count, TRIGGER_INITDIALOG, CB_SETITEMDATA, nullptr, dialog->combo_string_count-1, (LPARAM) item_data, nullptr, nullptr))
		return 1;
	if (item_data == dialog->combo_default_value)
	{
		if (dialog_add_trigger(dialog, dialog->item_count, TRIGGER_INITDIALOG, CB_SETCURSEL, nullptr, dialog->combo_string_count-1, 0, nullptr, nullptr))
			return 1;
	}
	return 0;
}



//============================================================
//  adjuster_sb_wndproc
//    called from adjuster_sb_setup
//============================================================

struct adjuster_sb_stuff
{
	WNDPROC oldwndproc;
	int min_value;
	int max_value;
};

static INT_PTR CALLBACK adjuster_sb_wndproc(HWND sbwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	INT_PTR result;
	struct adjuster_sb_stuff *stuff;
	std::string buf;
	HWND dlgwnd, editwnd;
	int value, id;
	LONG_PTR l;

	l = windows::get_window_long_ptr(sbwnd, GWLP_USERDATA);
	stuff = (struct adjuster_sb_stuff *) l;

	if (msg == WM_VSCROLL)
	{
		id = windows::get_window_long_ptr(sbwnd, GWL_ID);
		dlgwnd = windows::get_parent(sbwnd);
		editwnd = dialog_boxes::get_dlg_item(dlgwnd, id - 1);
		buf = windows::get_window_text_utf8(editwnd);
		value = atoi(buf.c_str());

		switch(wparam)
		{
			case SB_LINEDOWN:
			case SB_PAGEDOWN:
				value--;
				break;

			case SB_LINEUP:
			case SB_PAGEUP:
				value++;
				break;
		}

		if (value < stuff->min_value)
			value = stuff->min_value;
		else if (value > stuff->max_value)
			value = stuff->max_value;
		buf = std::to_string(value);
		windows::set_window_text_utf8(editwnd, buf.c_str());
		result = 0;
	}
	else
		result = call_windowproc(stuff->oldwndproc, sbwnd, msg, wparam, lparam);

	return result;
}



//============================================================
//  adjuster_sb_setup
//    called from win_dialog_add_adjuster
//============================================================

static LRESULT adjuster_sb_setup(dialog_box *dialog, HWND sbwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	struct adjuster_sb_stuff *stuff;
	LONG_PTR l;

	stuff = new adjuster_sb_stuff;
	if (!stuff)
		return 1;
	stuff->min_value = (WORD) (lparam >> 0);
	stuff->max_value = (WORD) (lparam >> 16);

	l = (LONG_PTR) stuff;
	(void)windows::set_window_long_ptr(sbwnd, GWLP_USERDATA, l);
	l = (LONG_PTR) adjuster_sb_wndproc;
	l = windows::set_window_long_ptr(sbwnd, GWLP_WNDPROC, l);
	stuff->oldwndproc = (WNDPROC) l;
	return 0;
}



//============================================================
//  win_dialog_add_adjuster
//    called from customise_analogcontrols
//============================================================

static int win_dialog_add_adjuster(dialog_box *dialog, const char *item_label, int default_value,
	int min_value, int max_value, bool is_percentage, dialog_itemstoreval storeval, void *storeval_param)
{
	short x;
	short y;
	wchar_t buf[32];
	std::wostringstream wss_percent(buf);
	std::unique_ptr<wchar_t[]> wcs_percent_string(new wchar_t[33]);

	dialog_new_control(dialog, &x, &y);

	if (dialog_write_item(dialog, WS_CHILD | WS_VISIBLE | SS_LEFT, x, y, dialog->layout->label_width, DIM_ADJUSTER_HEIGHT, item_label, DLGITEM_STATIC, nullptr))
		goto error;
	x += dialog->layout->label_width + DIM_HORIZONTAL_SPACING;

	y += DIM_BOX_VERTSKEW;

	if (dialog_write_item(dialog, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_NUMBER,
			x, y, dialog->layout->combo_width - DIM_ADJUSTER_SCR_WIDTH, DIM_ADJUSTER_HEIGHT, nullptr, DLGITEM_EDIT, nullptr))
		goto error;
	x += dialog->layout->combo_width - DIM_ADJUSTER_SCR_WIDTH;
	wss_percent << (is_percentage ? L"%d%%" : L"%d") << std::to_wstring(default_value) << std::flush;
	(void)mui_wcscpy(wcs_percent_string.get(), wss_percent.str().c_str());

	if (!wcs_percent_string)
		return 1;
	if (dialog_add_trigger(dialog, dialog->item_count, TRIGGER_INITDIALOG, WM_SETTEXT, nullptr, 0, (LPARAM)wcs_percent_string.get(), nullptr, nullptr))
		goto error;

	// add the trigger invoked when the apply button is pressed
	if (dialog_add_trigger(dialog, dialog->item_count, TRIGGER_APPLY, 0, dialog_get_adjuster_value, 0, 0, storeval, storeval_param))
		goto error;

	if (dialog_write_item(dialog, WS_CHILD | WS_VISIBLE | WS_TABSTOP | SBS_VERT, x, y, DIM_ADJUSTER_SCR_WIDTH, DIM_ADJUSTER_HEIGHT, nullptr, DLGITEM_SCROLLBAR, nullptr))
		goto error;
	x += DIM_ADJUSTER_SCR_WIDTH + DIM_HORIZONTAL_SPACING;

	if (dialog_add_trigger(dialog, dialog->item_count, TRIGGER_INITDIALOG, 0, adjuster_sb_setup, 0, MAKELONG(min_value, max_value), nullptr, nullptr))
		return 1;

	y += DIM_COMBO_ROW_HEIGHT + DIM_VERTICAL_SPACING * 2;

	dialog_finish_control(dialog, x, y);
	return 0;

error:
	return 1;
}


#if 0  // FIX ME: keybinding isn't working.
//============================================================
//  get_seqselect_info
//============================================================

static seqselect_info *get_seqselect_info(HWND editwnd)
{
	if (!editwnd)
		return nullptr;

	LONG_PTR lp = 0UL;
	lp = windows::get_window_long_ptr(editwnd, GWLP_USERDATA);
	if (!lp)
		return nullptr;

	return reinterpret_cast<seqselect_info*>(lp);
}



//=============================================================================
//  seqselect_settext
//    called from seqselect_start_read_from_main_thread, seqselect_setup
//=============================================================================

//#pragma GCC diagnostic push
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-value"
#endif
static void seqselect_settext(HWND editwnd)
{
	seqselect_info* stuff = get_seqselect_info(editwnd);
	if (!stuff)
		return; // Should not happen — maybe assert here?

	// Get current input sequence name
	std::string seqstring = Machine->input().seq_name(*stuff->code);

	// Get current edit control text
	auto edit_text = std::unique_ptr<char[]>(windows::get_window_text_utf8(editwnd));
	if (!edit_text)
	{
		stuff->poll_state = SEQSELECT_STATE_NOT_POLLING;
		return; // Defensive: could not read text
	}

	// Only update if it differs
	if (mui_strcmp(seqstring, edit_text.get()) != 0)
		windows::set_window_text_utf8(editwnd, seqstring.c_str());

	// Restore full selection if focused
	if (input::get_focus() == editwnd)
	{
		DWORD start = 0, end = 0;
		windows::send_message(editwnd, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);

		// Check if not already selecting the whole string
		size_t current_len = mui_strlen(edit_text.get());
		if (start != 0 || end != current_len)
			windows::send_message(editwnd, EM_SETSEL, 0, -1);
	}
}
#ifdef __GNUC__
#pragma GCC diagnostic error "-Wunused-value"
#endif



//============================================================
//  seqselect_start_read_from_main_thread
//    called from seqselect_wndproc
// MAMEUI: Tried working on this. Actually
// hard to believe it ever worked. I Might
// fully remove this feature since it's been
// disabled for so long and doing the key
// bindings through mame's ui is already a
// very simple task.
//============================================================

static void seqselect_start_read_from_main_thread(void *param)
{
	int pause_count;
	HWND editwnd;
	seqselect_info *stuff;

	auto seq_poll = std::make_unique<switch_sequence_poller>(Machine->input());

	// get the basics
	editwnd = (HWND) param;
	stuff = get_seqselect_info(editwnd);

	// are we currently polling?  if so bail out

	if (stuff->poll_state != SEQSELECT_STATE_NOT_POLLING)
		return;

	// change the state
	stuff->poll_state = SEQSELECT_STATE_POLLING;

	// the Win32 OSD code thinks that we are paused, we need to temporarily
	// unpause ourselves or else we will block
	pause_count = 0;
	while(Machine->paused() && !winwindow_ui_is_paused(*Machine))
	{
		winwindow_ui_pause(*Machine, false);
		pause_count++;
	}

	// butt ugly hack so that we accept focus
	osd_window* osd_window_ptr = osd_common_t::window_list().front().get();
	win_window_info *old_window_list = dynamic_cast<win_window_info*>(osd_window_ptr);
	//fake_window_info = (win_window_info*)malloc(sizeof(fake_window_info));
	if (old_window_list)
		old_window_list->focus();
	//win_window_list = fake_window_info;

	// start the polling
	if (stuff->code)
		seq_poll->start(*stuff->code);
	else
	seq_poll->start();

	while (stuff->poll_state == SEQSELECT_STATE_POLLING)
	{
		if (Machine->ui_input().pressed(IPT_UI_CANCEL))
			break;
		// poll
		if (seq_poll->poll())
		{
			stuff->code = const_cast<input_seq*>(&seq_poll->sequence());
			seqselect_settext(editwnd);
		}

	}

	// clean up after hack
	//win_window_list = old_window_list;

	// we are no longer polling
	stuff->poll_state = SEQSELECT_STATE_NOT_POLLING;

	// repause the OSD code
	while(pause_count--)
		winwindow_ui_pause(*Machine, true);

	seq_poll.reset();
}



//============================================================
//  seqselect_stop_read_from_main_thread
//    called from seqselect_wndproc
//============================================================

static void seqselect_stop_read_from_main_thread(void *param)
{
	if (!param)
		return;

	HWND editwnd = reinterpret_cast<HWND>(param);


	seqselect_info *stuff = get_seqselect_info(editwnd);
	if (!stuff)
		return;

	// stop the read
	if (stuff->poll_state == SEQSELECT_STATE_POLLING)
		stuff->poll_state = SEQSELECT_STATE_POLLING_COMPLETE;
}



//============================================================
//  seqselect_wndproc
//    called from seqselect_setup
//============================================================

static INT_PTR CALLBACK seqselect_wndproc(HWND editwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	seqselect_info *stuff = get_seqselect_info(editwnd);
	if (!stuff)
		return 0;

	switch(msg)
	{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_CHAR:
		case WM_KEYUP:
		case WM_SYSKEYUP:
			return 1;

		case WM_SETFOCUS:
			windows::post_message(editwnd, SEQWM_SETFOCUS, 0, 0);
			break;

		case WM_KILLFOCUS:
			windows::post_message(editwnd, SEQWM_KILLFOCUS, 0, 0);
			break;

		case SEQWM_SETFOCUS:
			// if we receive the focus, we should start a polling loop
			seqselect_start_read_from_main_thread(reinterpret_cast<void*>(editwnd));
			break;

		case SEQWM_KILLFOCUS:
			// when we abort the focus, end any current polling loop
			seqselect_stop_read_from_main_thread(reinterpret_cast<void*>(editwnd));
			break;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			(void)input::set_focus(editwnd);
			(void) windows::send_message(editwnd, EM_SETSEL, 0, -1);
			return 0;
	}

	return call_windowproc(stuff->oldwndproc, editwnd, msg, wparam, lparam);
}



//============================================================
//  seqselect_setup
//    called from dialog_add_single_seqselect
//============================================================

static LRESULT seqselect_setup(dialog_box *dialog, HWND editwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	if (!editwnd || !lparam)
		return 0;

	seqselect_info* stuff = reinterpret_cast<seqselect_info*>(lparam);

	const LONG_PTR old_proc = windows::set_window_long_ptr(editwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(seqselect_wndproc));
	if (old_proc)
		stuff->oldwndproc = reinterpret_cast<WNDPROC>(old_proc);

	(void)windows::set_window_long_ptr(editwnd, GWLP_USERDATA, lparam);
	seqselect_settext(editwnd);

	return 0;
}



//============================================================
//  seqselect_apply
//    called from dialog_add_single_seqselect
//============================================================

static LRESULT seqselect_apply(dialog_box* dialog, HWND editwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	// store the settings
	seqselect_info* stuff = get_seqselect_info(editwnd);
	if (stuff)
		stuff->field->set_user_settings(stuff->settings);

	return 0;
}



//============================================================
//  dialog_add_single_seqselect
//    called from win_dialog_add_portselect
//============================================================

static int dialog_add_single_seqselect(dialog_box *di, short x, short y, short cx, short cy, ioport_field *field, int is_analog, int seqtype)
{
	
	// write the dialog item
	DWORD style_flags = WS_CHILD | WS_VISIBLE | SS_ENDELLIPSIS | ES_CENTER | SS_SUNKEN;
	if (dialog_write_item(di, style_flags, x, y, cx, cy, nullptr, DLGITEM_EDIT, nullptr))
		return 1;

	// allocate a seqselect_info
	seqselect_info *stuff = new seqselect_info();
	if (!stuff)
		return 0;

	// initialize the structure
	field->get_user_settings(stuff->settings);
	stuff->field = field;
	stuff->pos = di->item_count;
	stuff->is_analog = is_analog;

	// This next line is completely unsafe, but I do not know what to use *****************
	stuff->code = const_cast<input_seq*>(&field->seq( SEQ_TYPE_STANDARD ));

	if (dialog_add_trigger(di, di->item_count, TRIGGER_INITDIALOG, 0, seqselect_setup, di->item_count, (LPARAM)stuff, nullptr, nullptr))
		return 1;

	return dialog_add_trigger(di, di->item_count, TRIGGER_APPLY, 0, seqselect_apply, 0, 0, nullptr, nullptr);
}



//============================================================
//  win_dialog_add_seqselect
//    called from customise_input, customise_miscinput
//============================================================
static int win_dialog_add_portselect(dialog_box* dialog, ioport_field* field)
{
	int result = 0;
	constexpr int seq_count = 3;
	int seq_types[seq_count]{ SEQ_TYPE_STANDARD, SEQ_TYPE_DECREMENT, SEQ_TYPE_INCREMENT };
	bool is_analog[seq_count]{ true, false, false };
	std::string port_name = field->name();
	std::string_view port_suffix[seq_count]{ " Analog", " Dec", " Inc" };

	if (port_name.empty())
	{
		std::cout << "Blank port_name encountered in win_dialog_add_portselect" << std::endl;
		return 1;
	}

	short x, y;

	if (field->type() > IPT_ANALOG_FIRST && field->type() < IPT_ANALOG_LAST)
	{
		for (int seq = 0; seq < seq_count; seq++)
		{
			dialog_new_control(dialog, &x, &y);

			result = dialog_write_item(dialog,
				WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
				x, y,
				dialog->layout->label_width,
				DIM_NORMAL_ROW_HEIGHT,
				std::string(port_name + std::string(port_suffix[seq])).c_str(),
				DLGITEM_STATIC,
				nullptr);
			if (result)
				break;

			x += dialog->layout->label_width + DIM_HORIZONTAL_SPACING;

			result = dialog_add_single_seqselect(dialog, x, y, DIM_EDIT_WIDTH, DIM_NORMAL_ROW_HEIGHT, field, is_analog[seq], seq_types[seq]);
			if (result)
				break;

			y += DIM_VERTICAL_SPACING + DIM_NORMAL_ROW_HEIGHT;
			x += DIM_EDIT_WIDTH + DIM_HORIZONTAL_SPACING;

			dialog_finish_control(dialog, x, y);
		}
	}
	else
	{
		dialog_new_control(dialog, &x, &y);

		result = dialog_write_item(dialog,
			WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
			x, y,
			dialog->layout->label_width,
			DIM_NORMAL_ROW_HEIGHT,
			port_name.c_str(),
			DLGITEM_STATIC,
			nullptr);
		if (result)
			return result;

		x += dialog->layout->label_width + DIM_HORIZONTAL_SPACING;

		result = dialog_add_single_seqselect(dialog, x, y, DIM_EDIT_WIDTH, DIM_NORMAL_ROW_HEIGHT, field, false, SEQ_TYPE_STANDARD);
		if (result)
			return result;

		y += DIM_VERTICAL_SPACING + DIM_NORMAL_ROW_HEIGHT;
		x += DIM_EDIT_WIDTH + DIM_HORIZONTAL_SPACING;

		dialog_finish_control(dialog, x, y);
	}

	return result;
}



//============================================================
//  customise_input
//============================================================

void customise_input(running_machine& machine, HWND wnd, const char* title, u8 player_number, ioport_type_class input_type_class)
{
	dialog_box* dlg;

	/* create the dialog */
	dlg = win_dialog_init(title, nullptr);
	if (!dlg)
		return;

	for (auto& port : machine.ioport().ports())
	{
		ioport_type_class input_field_type_class;
		u8 input_field_player;
		std::string input_field_name;
		//      ioport_type input_field_type;
		//      ioport_group input_field_group_type;

		for (ioport_field& field : port.second->fields())
		{
			input_field_type_class = field.type_class();
			input_field_name = field.name();
			input_field_player = field.player();
			//          input_field_type = field.type();
			//          input_field_group_type = machine.ioport().type_group(input_field_type, input_field_player);

						/* add if we match the group and we have a valid name */
			if (!input_field_name.empty() && field.enabled()
				// check me     && (input_field_type == IPT_OTHER || input_field_group_type != IPG_INVALID)
				&& input_field_player == player_number && input_field_type_class == input_type_class)
			{
				if (win_dialog_add_portselect(dlg, &field))
					goto done;
			}
		}
	}

	/* ...and now add OK/Cancel */
	if (win_dialog_add_standard_buttons(dlg))
		goto done;

	/* ...and finally display the dialog */
	win_dialog_runmodal(machine, wnd, dlg);

done:
	if (dlg)
		win_dialog_exit(dlg);
}



//===============================================================================================
//  check_for_miscinput
//  (to decide if "Miscellaneous Inputs" menu item should show or not).
//  We do this here because the core check has been broken for years (always returns false).
//===============================================================================================

static bool check_for_miscinput(running_machine& machine)
{

	for (auto& port : machine.ioport().ports())
	{
		ioport_type_class input_field_type_class;
		u8 input_field_player;
		std::string input_field_name;
		ioport_type input_field_type;
		ioport_group input_field_group_type;

		for (ioport_field& field : port.second->fields())
		{
			input_field_type_class = field.type_class();
			input_field_name = field.name();
			input_field_player = field.player();
			input_field_type = field.type();
			input_field_group_type = machine.ioport().type_group(input_field_type, input_field_player);

			/* add if we match the group and we have a valid name */
			if (!input_field_name.empty() && field.enabled() && (input_field_type == IPT_OTHER || input_field_group_type != IPG_INVALID)
				&& input_field_type_class != INPUT_CLASS_CONTROLLER && input_field_type_class != INPUT_CLASS_KEYBOARD)
			{
				return true;
			}
		}
	}
	return false;
}



//============================================================
//  customise_miscinput
//============================================================

static void customise_miscinput(running_machine& machine, HWND wnd)
{
	const char* title = "Miscellaneous Inputs";
	dialog_box* dlg = win_dialog_init(title, nullptr);
	if (!dlg)
		return;

	for (auto& port : machine.ioport().ports())
	{
		for (ioport_field& field : port.second->fields())
		{
			if (!field.name().empty() && field.enabled() &&
				(field.type() == IPT_OTHER || machine.ioport().type_group(field.type(), field.player()) != IPG_INVALID) &&
				field.type_class() != INPUT_CLASS_CONTROLLER &&
				field.type_class() != INPUT_CLASS_KEYBOARD)
			{
				if (win_dialog_add_portselect(dlg, &field))
					goto done;
			}
		}
	}

	if (win_dialog_add_standard_buttons(dlg))
		goto done;

	win_dialog_runmodal(machine, wnd, dlg);

done:
	win_dialog_exit(dlg);
}
#endif // FIX ME: keybinding isn't working.


//============================================================
//  win_dialog_add_standard_buttons
//============================================================

static int win_dialog_add_standard_buttons(dialog_box* dialog)
{
	if (!dialog)
		return 1;

	short x, y;

	// Ensure dialog has a base size
	if (!dialog->size_x)
		dialog->size_x = 3 * DIM_HORIZONTAL_SPACING + 2 * DIM_BUTTON_WIDTH;
	if (!dialog->size_y)
		dialog->size_y = DIM_VERTICAL_SPACING;

	// Position Cancel button
	x = dialog->size_x - DIM_HORIZONTAL_SPACING - DIM_BUTTON_WIDTH;
	y = dialog->size_y + DIM_VERTICAL_SPACING;

	if (dialog_write_item(dialog, WS_CHILD | WS_VISIBLE | SS_LEFT, x, y,
		DIM_BUTTON_WIDTH, DIM_BUTTON_ROW_HEIGHT, DLGTEXT_CANCEL, DLGITEM_BUTTON, nullptr))
		return 1;

	// Position OK button
	x -= DIM_HORIZONTAL_SPACING + DIM_BUTTON_WIDTH;

	if (dialog_write_item(dialog, WS_CHILD | WS_VISIBLE | SS_LEFT, x, y,
		DIM_BUTTON_WIDTH, DIM_BUTTON_ROW_HEIGHT, DLGTEXT_OK, DLGITEM_BUTTON, nullptr))
		return 1;

	// Update dialog height to accommodate buttons
	dialog->size_y += DIM_BUTTON_ROW_HEIGHT + DIM_VERTICAL_SPACING * 2;

	return 0;
}



//============================================================
//  before_display_dialog
//============================================================

static void before_display_dialog(running_machine &machine)
{
	Machine = &machine;
	winwindow_ui_pause(machine, true);
}



//============================================================
//  after_display_dialog
//============================================================

static void after_display_dialog(running_machine &machine)
{
	winwindow_ui_pause(machine, false);
}



//============================================================
//  win_dialog_runmodal
//    called from the various customise_inputs functions
//============================================================

static void win_dialog_runmodal(running_machine &machine, HWND wnd, dialog_box *dialog)
{
	if (!dialog)
		std::cout << "Unexpected result in win_dialog_runmodal" << std::endl;
	assert(dialog);

	// finishing touches on the dialog
	dialog_prime(dialog);

	// show the dialog
	before_display_dialog(machine);
	dialog_boxes::dialog_box_indirect(nullptr, reinterpret_cast<LPCDLGTEMPLATEW>(dialog->buffer.data()), wnd, (DLGPROC)dialog_proc, (LPARAM)dialog);
	after_display_dialog(machine);
}



//============================================================
//  win_file_dialog
//    called from change_device
//============================================================

static bool win_file_dialog(running_machine &machine, HWND parent, win_file_dialog_type dlgtype, const char *filter, const char *initial_dir, char *filename)
{
	win_open_file_name ofn{};
	bool result = false;

	// set up the OPENFILENAMEW data structure
	ofn.type = dlgtype;
	ofn.owner = parent;
	ofn.flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	ofn.filter = filter;
	ofn.initial_directory = initial_dir;

	if (dlgtype == WIN_FILE_DIALOG_OPEN)
		ofn.flags |= OFN_FILEMUSTEXIST;

	mui_strcpy(ofn.filename, filename);

	before_display_dialog(machine);
	result = win_get_file_name_dialog(&ofn);
	after_display_dialog(machine);

	mui_strcpy(filename, ofn.filename);

	return result;
}



//============================================================
//  customise_joystick
//============================================================

static void customise_joystick(running_machine &machine, HWND wnd, int joystick_num)
{
//  customise_input(machine, wnd, "Joysticks/Controllers", joystick_num, INPUT_CLASS_CONTROLLER);
}



//============================================================
//  customise_keyboard
//============================================================

static void customise_keyboard(running_machine &machine, HWND wnd)
{
//  customise_input(machine, wnd, "Emulated Keyboard", 0, INPUT_CLASS_KEYBOARD);
}



//============================================================
//  update_keyval
//    called from customise_switches
//============================================================

static void update_keyval(void *param, int val)
{
	ioport_field *field = (ioport_field *) param;
	ioport_field::user_settings settings;

	field->get_user_settings(settings);
	settings.value = (ioport_value) val;
	field->set_user_settings(settings);
}



//============================================================
//  customise_switches
//============================================================

static void customise_switches(running_machine &machine, HWND hwnd_parent, const char* title_string, ioport_type target_input_type)
{
	dialog_box *dlg;

	dlg = win_dialog_init(title_string, nullptr);
	if (!dlg)
		return;

	for (auto &port : machine.ioport().ports())
	{
		const char* item_label;
		ioport_field* input_field;
		ioport_field::user_settings input_field_user_settings;
		ioport_type input_field_type;
		std::string input_field_name;

		for (ioport_field &field : port.second->fields())
		{
			input_field_type = field.type();

			if (input_field_type == target_input_type)
			{
				input_field_name = field.name();
				item_label = input_field_name.c_str();
				field.get_user_settings(input_field_user_settings);
				input_field = &field;
				if (win_dialog_add_combobox(dlg, item_label, input_field_user_settings.value, update_keyval, (void*)input_field))
					goto done;

				for (auto setting : field.settings())
				{
					if (win_dialog_add_combobox_item(dlg, setting.name(), setting.value()))
						goto done;
				}
			}
		}
	}

	if (win_dialog_add_standard_buttons(dlg))
		goto done;

	win_dialog_runmodal(machine, hwnd_parent, dlg);

done:
	if (dlg)
		win_dialog_exit(dlg);
}



//============================================================
//  customise_dipswitches
//============================================================

static void customise_dipswitches(running_machine &machine, HWND wnd)
{
	customise_switches(machine, wnd, "Dip Switches", IPT_DIPSWITCH);
}



//============================================================
//  customise_configuration
//============================================================

static void customise_configuration(running_machine &machine, HWND wnd)
{
	customise_switches(machine, wnd, "Driver Configuration", IPT_CONFIG);
}



//============================================================
//  customise_analogcontrols
//============================================================

enum
{
	ANALOG_ITEM_KEYSPEED,
	ANALOG_ITEM_CENTERSPEED,
	ANALOG_ITEM_REVERSE,
	ANALOG_ITEM_SENSITIVITY
};



static void store_analogitem(void *param, int val, int selected_item)
{
	ioport_field *field = (ioport_field *) param;
	ioport_field::user_settings settings;

	field->get_user_settings(settings);

	switch(selected_item)
	{
		case ANALOG_ITEM_KEYSPEED:
			settings.delta = val;
			break;
		case ANALOG_ITEM_CENTERSPEED:
			settings.centerdelta = val;
			break;
		case ANALOG_ITEM_REVERSE:
			settings.reverse = val;
			break;
		case ANALOG_ITEM_SENSITIVITY:
			settings.sensitivity = val;
			break;
	}
	field->set_user_settings(settings);
}



static void store_delta(void *param, int val)
{
	store_analogitem(param, val, ANALOG_ITEM_KEYSPEED);
}



static void store_centerdelta(void *param, int val)
{
	store_analogitem(param, val, ANALOG_ITEM_CENTERSPEED);
}



static void store_reverse(void *param, int val)
{
	store_analogitem(param, val, ANALOG_ITEM_REVERSE);
}



static void store_sensitivity(void *param, int val)
{
	store_analogitem(param, val, ANALOG_ITEM_SENSITIVITY);
}



static int port_type_is_analog(int type)
{
	return (type > IPT_ANALOG_FIRST && type < IPT_ANALOG_LAST);
}



static void customise_analogcontrols(running_machine &machine, HWND wnd)
{
	dialog_box *dlg;
	ioport_field::user_settings settings;
	ioport_field *afield;
	;
	static const dialog_layout layout = { 120, 52 };

	dlg = win_dialog_init("Analog Controls", &layout);
	if (!dlg)
		return;

	for (auto &port : machine.ioport().ports())
	{
		for (ioport_field &field : port.second->fields())
		{
			if (port_type_is_analog(field.type()))
			{
				std::string item_label;
				field.get_user_settings(settings);
				afield = &field;

				item_label = util::string_format("%s %s", field.name(), "Digital Speed");
				if (win_dialog_add_adjuster(dlg, item_label.c_str(), settings.delta, 1, 255, false, store_delta, (void*)afield))
					goto done;

				item_label = util::string_format("%s %s", field.name(), "Autocenter Speed");
				if (win_dialog_add_adjuster(dlg, item_label.c_str(), settings.centerdelta, 0, 255, false, store_centerdelta, (void *) afield))
					goto done;

				item_label = util::string_format("%s %s", field.name(), "Reverse");
				if (win_dialog_add_combobox(dlg, item_label.c_str(), settings.reverse ? 1 : 0, store_reverse, (void *) afield))
					goto done;

				if (win_dialog_add_combobox_item(dlg, "Off", 0))
					goto done;

				if (win_dialog_add_combobox_item(dlg, "On", 1))
					goto done;

				item_label = util::string_format("%s %s", field.name(), "Sensitivity");
				if (win_dialog_add_adjuster(dlg, item_label.c_str(), settings.sensitivity, 1, 255, true, store_sensitivity, (void *) afield))
					goto done;
			}
		}
	}

	if (win_dialog_add_standard_buttons(dlg))
		goto done;

	win_dialog_runmodal(machine, wnd, dlg);

done:
	if (dlg)
		win_dialog_exit(dlg);
}


//============================================================
//  win_dirname
//    called from state_dialog
//============================================================

static std::string win_dirname(const std::string_view filename)
{
	// nullptr begets nullptr
	if (filename.empty())
		return {};

	std::filesystem::path path(filename);
	path.make_preferred();  // replaces / with \ on Windows

	return path.parent_path().string();
}


//============================================================
//  state_dialog
//    called when loading or saving a state
//============================================================

static void state_dialog(HWND wnd, win_file_dialog_type dlgtype, DWORD fileproc_flags, bool is_load, running_machine &machine)
{
	std::string dir;

	if (state_filename.empty())
		osd_get_full_path(state_filename, "sta");

	dir = win_dirname(state_filename);

	win_open_file_name ofn{};
	ofn.type = dlgtype;
	ofn.owner = wnd;
	ofn.flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | fileproc_flags;
	ofn.filter = "State Files (*.sta)|*.sta|All Files (*.*)|*.*";
	ofn.initial_directory = dir.c_str();

	bool ends_with_sta = core_filename_ends_with(ofn.filename, "sta");
	if (ends_with_sta)
		mui_strcpy(ofn.filename, state_filename.c_str());
	else
		mui_strcpy(ofn.filename,util::string_format("%s.sta", state_filename).c_str());

	bool result = win_get_file_name_dialog(&ofn);
	if (result)
	{
		// the core doesn't add the extension if it's an absolute path
		if (osd_is_absolute_path(ofn.filename))
		{
			if (ends_with_sta)
				state_filename = util::string_format("%s.sta", ofn.filename);
			else
				state_filename = ofn.filename;
		}

		if (is_load)
			machine.schedule_load(state_filename.c_str());
		else
			machine.schedule_save(state_filename.c_str());
	}
}



static void state_load(HWND wnd, running_machine &machine)
{
	state_dialog(wnd, WIN_FILE_DIALOG_OPEN, OFN_FILEMUSTEXIST, true, machine);
}

static void state_save_as(HWND wnd, running_machine &machine)
{
	state_dialog(wnd, WIN_FILE_DIALOG_SAVE, OFN_OVERWRITEPROMPT, false, machine);
}

static void state_save(running_machine &machine)
{
	machine.schedule_save(state_filename.c_str());
}



//============================================================
//  copy_extension_list
//============================================================

static void copy_extension_list(std::string &dest, const std::string_view extensions)
{
	// our extension lists are comma delimited; Win32 expects to see lists
	// delimited by semicolons
	std::string_view::size_type start = 0;
	while (start < extensions.size())
	{
		// Find the next comma
		auto end = extensions.find(',', start);
		if (end == extensions.npos)
			end = extensions.size();

		// Add semicolon if not the first extension
		if (!dest.empty())
			dest.push_back(';');

		// Append "*." + extension
		dest.append("*.");
		dest.append(extensions.substr(start, end - start));

		// Move to the next extension
		start = end + 1;
	}
}



//============================================================
//  add_filter_entry
//============================================================

static void add_filter_entry(std::string& dest, const std::string_view description, const std::string_view extensions)
{
	// add the description
	dest.append(description);
	dest.append(" (");

	// add the extensions to the description
	copy_extension_list(dest, extensions);

	// add the trailing rparen and '|' character
	dest.append(")|");

	// now add the extension list itself
	copy_extension_list(dest, extensions);

	// append a '|'
	dest.append("|");
}



//============================================================
//  build_generic_filter
//============================================================

static void build_generic_filter(device_image_interface *img, bool is_save, std::string &filter)
{
	std::string file_extension;

	if (img)
		file_extension = img->file_extensions();

	if (!is_save)
		file_extension.append(",zip,7z");

	add_filter_entry(filter, "Common image types", file_extension.c_str());

	filter.append("All files (*.*)|*.*|");

	if (!is_save)
		filter.append("Compressed Images (*.zip;*.7z)|*.zip;*.7z|");
}



//============================================================
//  get_softlist_info
//============================================================
static bool get_softlist_info(HWND wnd, device_image_interface* img)
{
    bool has_software = false;
    bool passes_tests = false;
    std::string sl_dir;
    const std::string opt_name = img->instance_name();

    // Get window info from user data
    LONG_PTR ptr = windows::get_window_long_ptr(wnd, GWLP_USERDATA);
    auto* window = reinterpret_cast<win_window_info*>(ptr);

    // Get media_path from machine options
    std::string rompath = window->machine().options().emu_options::media_path();

    // Get the path to suitable software
    for (software_list_device& swlist : software_list_device_enumerator(window->machine().root_device()))
    {
        for (const software_info& swinfo : swlist.get_info())
        {
            const software_part& part = swinfo.parts().front();

            if (swlist.is_compatible(part) == SOFTWARE_IS_COMPATIBLE)
            {
                for (device_image_interface& image : image_interface_enumerator(window->machine().root_device()))
                {
                    if (!image.user_loadable())
                        continue;

                    if (!has_software && (opt_name == image.instance_name()))
                    {
                        const char* image_interface = image.image_interface();
                        if (image_interface && part.matches_interface(image_interface))
                        {
                            sl_dir = "\\" + swlist.list_name();
                            has_software = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (has_software)
    {
        // Now, scan through the media_path looking for the required folder
        stringtokenizer tokenizer(rompath, ";");
		for (const auto &token : tokenizer)
		{
			std::string test_path = token + sl_dir;
			DWORD dwAttrib = storage::get_file_attributes_utf8(test_path.c_str());
			if ((dwAttrib != INVALID_FILE_ATTRIBUTES) && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
			{
				slmap[opt_name] = test_path;
				passes_tests = true;
				break;
			}
		}
    }

    return passes_tests;
}



//============================================================
//  change_device
//    open a dialog box to open or create a software file
//============================================================

static void change_device(HWND wnd, device_image_interface *image, bool is_save)
{
	std::size_t buf_size = mui_strlen(image->device().machine().options().emu_options::sw_path()) + 2;
	// Get the path for loose software from <gamename>.ini
	// if this is invalid, then windows chooses whatever directory it used last.
	std::unique_ptr<char[]> buf(new char [buf_size]);
	std::fill_n(buf.get(), buf_size-1, '\0');
	(void)mui_strcpy(buf.get(), image->device().machine().options().emu_options::sw_path());
	// This pulls out the first path from a multipath field
	const char* t1 = strtok(buf.get(), ";");
	std::string initial_dir = t1 ? std::string(t1) : "";
	// must be specified, must exist
	if (initial_dir.empty() || (!osd::directory::open(initial_dir.c_str())))
	{
		// NOTE: the working directory can come from the .cfg file. If it's wrong delete the cfg.
		initial_dir = image->working_directory().c_str();
		// make sure this exists too
		if (!osd::directory::open(initial_dir.c_str()))
			// last fallback is to mame root
			osd_get_full_path(initial_dir, ".");
	}

	// remove any trailing backslash
	if (initial_dir.length() == initial_dir.find_last_of("\\"))
		initial_dir.erase(initial_dir.length()-1);

	// file name
	uint16_t filesz = 0;
	if (image->exists())
		filesz = mui_strlen(image->basename());

	char filename[MAX_PATH];
	std::fill_n(filename, sizeof(filename) - 1, '\0');
	if (filesz)
		mui_strcpy(filename, image->basename());

	// build a normal filter
	std::string filter;
	build_generic_filter(image, is_save, filter);

	// display the dialog
	util::option_resolution *create_args = nullptr;
	bool result = win_file_dialog(image->device().machine(), wnd, is_save ? WIN_FILE_DIALOG_SAVE : WIN_FILE_DIALOG_OPEN, filter.c_str(), initial_dir.c_str(), filename);
	if (result)
	{
		// mount the image
		if (is_save)
			image->create(filename, image->device_get_indexed_creatable_format(0), create_args);
		else
			image->load(filename);
	}
}


//============================================================
//  load_item
//    open a dialog box to choose a software-list-item to load
//============================================================
static void load_item(HWND wnd, device_image_interface *img, bool is_save)
{
	std::string opt_name = img->instance_name();
	std::string as = slmap.find(opt_name)->second;

	/* Make sure a folder was specified in the tab, and that it exists */
	if ((!osd::directory::open(as.c_str())) || (as.find(':') == std::string::npos))
	{
		/* Default to emu directory */
		osd_get_full_path(as, ".");
	}

	// build a normal filter
	std::string filter;
	build_generic_filter(nullptr, is_save, filter);

	// display the dialog
	char filename[16384] = "";
	bool result = win_file_dialog(img->device().machine(), wnd, WIN_FILE_DIALOG_OPEN, filter.c_str(), as.c_str(), filename);

	if (result)
	{
		// Get the Item name out of the full path
		std::string buf = filename; // convert to a c++ string so we can manipulate it
		size_t t1 = buf.find(".zip"); // get rid of zip name and anything after
		if (t1 != std::string::npos)
			buf.erase(t1);
		else
		{
			t1 = buf.find(".7z"); // get rid of 7zip name and anything after
			if (t1 != std::string::npos)
				buf.erase(t1);
		}
		t1 = buf.find_last_of("\\");   // put the swlist name in
		buf[t1] = ':';
		t1 = buf.find_last_of("\\"); // get rid of path; we only want the item name
		buf.erase(0, t1+1);

		// load software
		img->load_software(buf);
	}
}



//============================================================
//  pause
//============================================================

static void pause(running_machine &machine)
{
	if (!winwindow_ui_is_paused(machine))
		machine.pause();
	else
		machine.resume();
}



//============================================================
//  get_menu_item_string
//============================================================

static bool get_menu_item_string(HMENU menu, UINT item, bool by_position, HMENU *sub_menu, LPWSTR buffer, size_t buffer_len)
{
	MENUITEMINFOW mii;

	// clear out results
	std::fill_n(buffer, buffer_len, L'\0');
	if (sub_menu)
		*sub_menu = nullptr;

	// prepare MENUITEMINFO structure
	mii = { sizeof(mii) };
	mii.fMask = MIIM_TYPE | (sub_menu ? MIIM_SUBMENU : 0);
	mii.dwTypeData = buffer;
	mii.cch = buffer_len;

	// call menus::get_menu_item_info()
	if (!menus::get_menu_item_info(menu, item, by_position, &mii))
		return false;

	// return results
	if (sub_menu)
		*sub_menu = mii.hSubMenu;
	if (mii.fType == MFT_SEPARATOR)
		mui_wcsncpy(buffer, L"-", buffer_len);
	return true;
}



//============================================================
//  find_sub_menu
//============================================================

static HMENU find_sub_menu(HMENU menu, const char *menutext, bool create_sub_menu)
{
	wchar_t buf[128];
	HMENU sub_menu;

	while(*menutext)
	{
		std::unique_ptr<wchar_t[]> wcs_menutext(mui_utf16_from_utf8cstring(menutext));

		int i = -1;
		do
		{
			if (!get_menu_item_string(menu, ++i, true, &sub_menu, buf, std::size(buf)))
			{
				return nullptr;
			}
		}
		while(mui_wcscmp(wcs_menutext.get(), buf));

		if (!sub_menu && create_sub_menu)
		{
			MENUITEMINFOW mii;
			mii = { sizeof(mii) };
			mii.fMask = MIIM_SUBMENU;
			mii.hSubMenu = menus::create_menu();
			if (!menus::set_menu_item_info(menu, i, true, &mii))
			{
				i = system_services::get_last_error();
				return nullptr;
			}

			sub_menu = mii.hSubMenu;
		}
		menu = sub_menu;
		if (!menu)
			return nullptr;

		menutext += mui_strlen(menutext) + 1;
	}

	return menu;
}



//============================================================
//  set_command_state
//============================================================

static void set_command_state(HMENU menu_bar, UINT command, UINT state)
{
	MENUITEMINFOW mii;
	mii = { sizeof(mii) };
	mii.fMask = MIIM_STATE;
	mii.fState = state;
	(void)menus::set_menu_item_info(menu_bar, command, false, &mii);
}




//============================================================
//  remove_menu_items
//============================================================

static void remove_menu_items(HMENU menu)
{
	while(menus::remove_menu(menu, 0, MF_BYPOSITION));
}



//============================================================
//  setup_joystick_menu
//============================================================

static void setup_joystick_menu(running_machine &machine, HMENU menu_bar)
{
	HMENU joystick_menu = find_sub_menu(menu_bar, "&Options\0&Joysticks\0", true);
	if (!joystick_menu)
		return;

	// set up joystick menu
	int child_count = 0;
	int joystick_count = machine.ioport().count_players();
	if (joystick_count > 0)
	{
		for (int i = 0; i < joystick_count; i++)
		{
			std::string joystick_id = util::string_format("Joystick %i", i + 1);
			menus::append_menu_utf8(joystick_menu, MF_STRING, ID_JOYSTICK_0 + i, joystick_id.c_str());
			child_count++;
		}
	}

	// last but not least, enable the joystick menu (or not)
	set_command_state(menu_bar, ID_OPTIONS_JOYSTICKS, child_count ? MFS_ENABLED : MFS_GRAYED);
}



//============================================================
//  is_windowed
//============================================================

static int is_windowed(void)
{
	return video_config.windowed;
}



//============================================================
//  frameskip_level_count
//============================================================

static int frameskip_level_count(running_machine &machine)
{
	static int count = -1;

	if (count < 0)
	{
		int frameskip_max = 10;
		count = frameskip_max + 1;
	}
	return count;
}



//============================================================
//  prepare_menus
//============================================================

static void prepare_menus(HWND wnd)
{
	bool has_config, has_dipswitch, has_keyboard,
		has_misc, has_analog, usage_shown;
	const char* view_name;
	int cnt, frameskip, orientation, speed;
	LONG_PTR ptr = windows::get_window_long_ptr(wnd, GWLP_USERDATA);
	win_window_info *window = (win_window_info *)ptr;
	mame_ui_manager mame_ui(window->machine());
	HMENU menu_bar = menus::get_menu(wnd);
	HMENU slot_menu, sub_menu, device_menu, video_menu;
	UINT_PTR new_item, new_switem;
	UINT flags_for_exists, flags_for_writing, menu_flags;
	wchar_t wcs_buf[MAX_PATH];

	if (!menu_bar)
		return;

	if (!joystick_menu_setup)
	{
		setup_joystick_menu(window->machine(), menu_bar);
		joystick_menu_setup = 1;
	}

	frameskip = window->machine().video().frameskip();
	orientation = window->target()->orientation();
	speed = window->machine().video().throttled() ? window->machine().video().speed_factor() : 0;

	has_config = window->machine().ioport().type_class_present(INPUT_CLASS_CONFIG);
	has_dipswitch = window->machine().ioport().type_class_present(INPUT_CLASS_DIPSWITCH);
	has_keyboard = window->machine().ioport().type_class_present(INPUT_CLASS_KEYBOARD);
	// FIX ME: keybinding isn't working. I restored some
	// of it, but i'm not sure how to get the polling to
	// work with the focus on the dialog instead of the
	// OSD window.
#if 0
	has_misc = check_for_miscinput(window->machine());

	has_analog = false;
	for (auto &port : window->machine().ioport().ports())
	{
		for (ioport_field &field : port.second->fields())
		{
			if (port_type_is_analog(field.type()))
			{
				has_analog = true;
				break;
			}
		}
	}
#else
has_misc = false;
has_analog = false;
#endif // FIX ME: keybinding isn't working.
	if (window->machine().system().flags & MACHINE_SUPPORTS_SAVE)
	{
		set_command_state(menu_bar, ID_FILE_LOADSTATE_NEWUI, MFS_ENABLED);
		set_command_state(menu_bar, ID_FILE_SAVESTATE_AS, MFS_ENABLED);
		set_command_state(menu_bar, ID_FILE_SAVESTATE, state_filename[0] != '\0' ? MFS_ENABLED : MFS_GRAYED);
	}
	else
	{
		set_command_state(menu_bar, ID_FILE_LOADSTATE_NEWUI, MFS_GRAYED);
		set_command_state(menu_bar, ID_FILE_SAVESTATE_AS, MFS_GRAYED);
		set_command_state(menu_bar, ID_FILE_SAVESTATE, MFS_GRAYED);
	}

	set_command_state(menu_bar, ID_EDIT_PASTE, window->machine().natkeyboard().can_post() ? MFS_ENABLED : MFS_GRAYED);

	set_command_state(menu_bar, ID_OPTIONS_PAUSE, winwindow_ui_is_paused(window->machine()) ? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_OPTIONS_CONFIGURATION, has_config ? MFS_ENABLED : MFS_GRAYED);
	set_command_state(menu_bar, ID_OPTIONS_DIPSWITCHES, has_dipswitch ? MFS_ENABLED : MFS_GRAYED);
	set_command_state(menu_bar, ID_OPTIONS_MISCINPUT, has_misc ? MFS_ENABLED : MFS_GRAYED);
	set_command_state(menu_bar, ID_OPTIONS_ANALOGCONTROLS, has_analog ? MFS_ENABLED : MFS_GRAYED);
	set_command_state(menu_bar, ID_FILE_FULLSCREEN, !is_windowed() ? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_OPTIONS_TOGGLEFPS, mame_machine_manager::instance()->ui().show_fps() ? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_FILE_UIACTIVE, has_keyboard ? (mame_ui.ui_active() ? MFS_CHECKED : MFS_ENABLED): MFS_CHECKED | MFS_GRAYED);

	set_command_state(menu_bar, ID_KEYBOARD_EMULATED, has_keyboard ? (!window->machine().natkeyboard().in_use() ? MFS_CHECKED : MFS_ENABLED): MFS_GRAYED);
	set_command_state(menu_bar, ID_KEYBOARD_NATURAL, (has_keyboard && window->machine().natkeyboard().can_post()) ? (window->machine().natkeyboard().in_use() ? MFS_CHECKED : MFS_ENABLED): MFS_GRAYED);
	set_command_state(menu_bar, ID_KEYBOARD_CUSTOMIZE, has_keyboard ? MFS_ENABLED : MFS_GRAYED);

	set_command_state(menu_bar, ID_VIDEO_ROTATE_0, (orientation == ROT0) ? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_VIDEO_ROTATE_90, (orientation == ROT90) ? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_VIDEO_ROTATE_180, (orientation == ROT180) ? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_VIDEO_ROTATE_270, (orientation == ROT270) ? MFS_CHECKED : MFS_ENABLED);

	set_command_state(menu_bar, ID_THROTTLE_50, (speed == 500) ? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_THROTTLE_100, (speed == 1000) ? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_THROTTLE_200, (speed == 2000) ? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_THROTTLE_500, (speed == 5000) ? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_THROTTLE_1000, (speed == 10000) ? MFS_CHECKED : MFS_ENABLED);
	set_command_state(menu_bar, ID_THROTTLE_UNTHROTTLED, (speed == 0) ? MFS_CHECKED : MFS_ENABLED);

	set_command_state(menu_bar, ID_FRAMESKIP_AUTO, (frameskip < 0) ? MFS_CHECKED : MFS_ENABLED);

	for (size_t i = 0; i < frameskip_level_count(window->machine()); i++)
		set_command_state(menu_bar, ID_FRAMESKIP_0 + i, (frameskip == i) ? MFS_CHECKED : MFS_ENABLED);

	// set up screens in video menu
	video_menu = find_sub_menu(menu_bar, "&Options\0&Video\0", false);
	do
	{
		get_menu_item_string(video_menu, 0, true, nullptr, wcs_buf, std::size(wcs_buf));
		if (mui_wcscmp(wcs_buf, L"-"))
			RemoveMenu(video_menu, 0, MF_BYPOSITION);
	}
	while(mui_wcscmp(wcs_buf, L"-"));

	for (size_t i = 0, view_index = window->target()->view(); (view_name = window->target()->view_name(i)); i++)
	{
		std::unique_ptr<const wchar_t[]> wcs_view_name(mui_utf16_from_utf8cstring(view_name));
		InsertMenu(video_menu, i, MF_BYPOSITION | (i == view_index ? MF_CHECKED : 0), ID_VIDEO_VIEW_0 + i, wcs_view_name.get());
	}

	// set up device menu; first remove all existing menu items
	device_menu = find_sub_menu(menu_bar, "&Media\0", false);
	remove_menu_items(device_menu);

	new_switem = 0;
	usage_shown = false;
	cnt = 0;
	// then set up the actual devices
	for (device_image_interface &img : image_interface_enumerator(window->machine().root_device()))
	{
		if (!img.user_loadable())
			continue;

		if (!img.device().machine().options().has_image_option(img.instance_name()))
			continue;

		new_item = ID_DEVICE_0 + (cnt * DEVOPTION_MAX);
		flags_for_exists = MF_STRING;

		if (!img.exists())
			flags_for_exists |= MF_GRAYED;

		flags_for_writing = flags_for_exists;
		if (img.is_readonly())
			flags_for_writing |= MF_GRAYED;

		sub_menu = menus::create_menu();

		// Software-list processing
		if (get_softlist_info(wnd, &img))
		{
			bool anything = false;
			// If there's a swlist item mounted, see if has multiple parts and if so, display them as choices
			if (img.loaded_through_softlist())
			{
				const software_part *tmp = img.part_entry();
				const char *image_interface = img.image_interface();
				const software_info *swinfo = img.software_entry();
				for (const software_part &swpart : swinfo->parts())
				{
					if (swpart.matches_interface(image_interface))
					{
						// Extract the Usage data from the "info" fields.
						for (const software_info_item &flist : swinfo->info())
						{
							if (flist.name() == "usage" && !usage_shown)
							{
								std::string usage = "Usage: " + newui_longdots(flist.value(),200);
								menus::append_menu_utf8(sub_menu, MF_STRING, 0, usage.c_str());
								menus::append_menu_utf8(sub_menu, MF_SEPARATOR, 0, nullptr);
								usage_shown = true;
							}
						}

						// Get any multiple parts
						if (!tmp->name().empty())
						{
							// Don't show the part that is already loaded
							if (tmp->name() != swpart.name())
							{
								// check if the available parts have specific part_id to be displayed (e.g. "Map Disc", "Bonus Disc", etc.)
								// if not, we simply display "part_name"; if yes we display "part_name (part_id)"
								std::string menu_part_name(swpart.name());
								if (swpart.feature("part_id"))
									menu_part_name.append(": ").append(newui_longdots(swpart.feature("part_id"),50));
								menus::append_menu_utf8(sub_menu, MF_STRING, new_switem + ID_SWPART, menu_part_name.c_str());
								part_map[new_switem] = part_data{ swpart.name(), &img };
								new_switem++;
								anything = true;
							}
						}
					}
				}
			}

			if (anything)
				menus::append_menu_utf8(sub_menu, MF_SEPARATOR, 0, nullptr);

			// Since a software list exists, add the Mount Item menu
			menus::append_menu_utf8(sub_menu, MF_STRING, new_item + DEVOPTION_ITEM, "Mount Item...");
		}

		menus::append_menu_utf8(sub_menu, MF_STRING, new_item + DEVOPTION_OPEN, "Mount File...");

		if (img.is_creatable())
			menus::append_menu_utf8(sub_menu, MF_STRING, new_item + DEVOPTION_CREATE, "Create...");

		if (img.exists())
		{
			menus::append_menu_utf8(sub_menu, flags_for_exists, new_item + DEVOPTION_CLOSE, "Unmount");

			if (img.device().type() == CASSETTE)
			{
				cassette_state state,t_state;
				state = cassette_state(img.exists() ? (dynamic_cast<cassette_image_device*>(&img.device())->get_state()) : 0);
				t_state = cassette_state(state & CASSETTE_MASK_UISTATE);
				menus::append_menu_utf8(sub_menu, MF_SEPARATOR, 0, nullptr);
				menus::append_menu_utf8(sub_menu, flags_for_exists | ((t_state == CASSETTE_STOPPED) ? MF_CHECKED : 0), new_item + DEVOPTION_CASSETTE_STOPPAUSE, "Pause/Stop");
				menus::append_menu_utf8(sub_menu, flags_for_exists | ((t_state == CASSETTE_PLAY) ? MF_CHECKED : 0), new_item + DEVOPTION_CASSETTE_PLAY, "Play");
				menus::append_menu_utf8(sub_menu, flags_for_writing | ((t_state == CASSETTE_RECORD) ? MF_CHECKED : 0), new_item + DEVOPTION_CASSETTE_RECORD, "Record");
				menus::append_menu_utf8(sub_menu, flags_for_exists, new_item + DEVOPTION_CASSETTE_REWIND, "Rewind");
				menus::append_menu_utf8(sub_menu, flags_for_exists, new_item + DEVOPTION_CASSETTE_FASTFORWARD, "Fast Forward");
				menus::append_menu_utf8(sub_menu, MF_SEPARATOR, 0, nullptr);
				// Motor state can be overriden by the driver
				t_state = cassette_state(state & CASSETTE_MASK_MOTOR);
				menus::append_menu_utf8(sub_menu, flags_for_exists | ((t_state == CASSETTE_MOTOR_ENABLED) ? MF_CHECKED : 0), new_item + DEVOPTION_CASSETTE_MOTOR, "Motor");
				// Speaker requires that cassette-wave device be included in the machine config
				t_state = cassette_state(state & CASSETTE_MASK_SPEAKER);
				menus::append_menu_utf8(sub_menu, flags_for_exists | ((t_state == CASSETTE_SPEAKER_ENABLED) ? MF_CHECKED : 0), new_item + DEVOPTION_CASSETTE_SOUND, "Audio while Loading");
			}
		}

		std::string filename;
		if (img.basename())
		{
			filename.assign(img.basename());

			// if the image has been loaded through softlist, also show the loaded part
			if (img.loaded_through_softlist())
			{
				const software_part *tmp = img.part_entry();
				if (!tmp->name().empty())
				{
					filename.append(" (").append(tmp->name());
					// also check if this part has a specific part_id (e.g. "Map Disc", "Bonus Disc", etc.), and in case display it
					if (img.get_feature("part_id"))
						filename.append(": ").append(newui_longdots(img.get_feature("part_id"),50));
					filename.append(")");
				}
			}
		}
		else
			filename.assign("---");

		// Get instance names instead, like Media View, and mame's File Manager
		std::string instance = img.instance_name() + std::string(" (") + img.brief_instance_name() + std::string("): ") + newui_longdots(filename,127);
		std::transform(instance.begin(), instance.begin()+1, instance.begin(), ::toupper); // turn first char to uppercase
		menus::append_menu_utf8(device_menu, MF_POPUP, (UINT_PTR)sub_menu, instance.c_str());

		cnt++;
	}

	// Print a warning if the media devices overrun the allocated range
	if ((ID_DEVICE_0 + (cnt * DEVOPTION_MAX)) >= ID_JOYSTICK_0)
		std::cout << "Maximum number of media items exceeded !!!" << std::endl;

	// set up slot menu; first remove all existing menu items
	slot_menu = find_sub_menu(menu_bar, "&Slots\0", false);
	remove_menu_items(slot_menu);
	cnt = 3400;
	// cycle through all slots for this system
	for (device_slot_interface &slot : slot_interface_enumerator(window->machine().root_device()))
	{


		if (slot.fixed())
			continue;
		// does this slot have any selectable options?
		if (!slot.has_selectable_options())
			continue;

		std::string opt_name="0", current="0";

		// name this option
		const char *slot_option_name = slot.slot_name();
		current = window->machine().options().slot_option(slot_option_name).value();

		const device_slot_interface::slot_option *option = slot.option(current.c_str());
		if (option)
			opt_name = option->name();

		sub_menu = menus::create_menu();
		// add the slot
		menus::append_menu_utf8(slot_menu, MF_POPUP, (UINT_PTR)sub_menu, slot.slot_name());
		// build a list of user-selectable options
		std::vector<device_slot_interface::slot_option *> option_list;
		for (auto &option : slot.option_list())
			if (option.second->selectable())
				option_list.push_back(option.second.get());

		// add the empty option
		slot_map[cnt] = slot_data { slot.slot_name(), "" };
		menu_flags = MF_STRING;
		if (opt_name == "0")
			menu_flags |= MF_CHECKED;
		menus::append_menu_utf8(sub_menu, menu_flags, cnt++, "[Empty]");

		// sort them by name
		std::sort(option_list.begin(), option_list.end(), [](device_slot_interface::slot_option *opt1, device_slot_interface::slot_option *opt2) {return mui_strcmp(opt1->name(), opt2->name()) < 0;});

		// add each option in sorted order
		for (device_slot_interface::slot_option *opt : option_list)
		{
			std::string temp = opt->name() + std::string(" (") + opt->devtype().fullname() + std::string(")");
			slot_map[cnt] = slot_data { slot.slot_name(), opt->name() };
			menu_flags = MF_STRING;
			if (opt->name()==opt_name)
				menu_flags |= MF_CHECKED;
			menus::append_menu_utf8(sub_menu, menu_flags, cnt++, temp.c_str());
		}
	}
}



//============================================================
//  set_speed
//============================================================

static void set_speed(running_machine &machine, int speed)
{
	bool throttled = speed ? true : false;
	if (throttled)
	{
		machine.video().set_speed_factor(speed);
		float dspeed = float(speed) / 1000;
		machine.options().emu_options::set_value(OPTION_SPEED, dspeed, OPTION_PRIORITY_CMDLINE);
	}

	machine.video().set_throttled(throttled);
	machine.options().emu_options::set_value(OPTION_THROTTLE, throttled, OPTION_PRIORITY_CMDLINE);
}



//============================================================
//  win_toggle_menubar
//============================================================

static void win_toggle_menubar(void)
{
	LONG width_diff = 0;
	LONG height_diff = 0;
	DWORD style = 0, exstyle = 0;
	HWND hwnd = 0;
	HMENU menu = 0;

	for (const auto &window : osd_common_t::window_list())
	{
		RECT before_rect = { 100, 100, 200, 200 };
		RECT after_rect = { 100, 100, 200, 200 };

		hwnd = dynamic_cast<win_window_info&>(*window).platform_window();

		// get current menu
		menu = menus::get_menu(hwnd);

		// get before rect
		style = windows::get_window_long_ptr(hwnd, GWL_STYLE);
		exstyle = windows::get_window_long_ptr(hwnd, GWL_EXSTYLE);
		windows::adjust_window_rect_ex(&before_rect, style, menu ? true : false, exstyle);

		// toggle the menu
		if (menu)
		{
			windows::set_prop(hwnd, L"menu", static_cast<HANDLE>(menu));
			menu = nullptr;
		}
		else
			menu = static_cast<HMENU>(windows::get_prop(hwnd, L"menu"));

		(void)menus::set_menu(hwnd, menu);

		// get after rect, and width/height diff
		windows::adjust_window_rect_ex(&after_rect, style, menu ? true : false, exstyle);
		width_diff = (after_rect.right - after_rect.left) - (before_rect.right - before_rect.left);
		height_diff = (after_rect.bottom - after_rect.top) - (before_rect.bottom - before_rect.top);

		if (is_windowed())
		{
			RECT window_rect;
			(void)windows::get_window_rect(hwnd, &window_rect);
			(void)windows::set_window_pos(hwnd, HWND_TOP, 0, 0, window_rect.right - window_rect.left + width_diff, window_rect.bottom - window_rect.top + height_diff, SWP_NOMOVE | SWP_NOZORDER);
		}

		(void)gdi::redraw_window(hwnd, nullptr, nullptr, 0);
	}
}



//============================================================
//  device_command
//    This handles all options under the "Media" dropdown
//============================================================

static void device_command(HWND wnd, device_image_interface *img, int devoption)
{
	switch(devoption)
	{
		case DEVOPTION_OPEN:
			change_device(wnd, img, false);
			break;

		case DEVOPTION_ITEM:
			load_item(wnd, img, false);
			break;

		case DEVOPTION_CREATE:
			change_device(wnd, img, true);
			break;

		case DEVOPTION_CLOSE:
		{
			std::string t = img->instance_name();
			img->unload();
			img->device().machine().options().image_option(img->instance_name()).specify("");
			// Some cartridges have their own extra slot. When the cart is removed we need to restart to remove the slot too.
			// This could fail if the system normally has 2 slots.
			if (!img->device().machine().options().has_image_option(t))
				img->device().machine().schedule_hard_reset();
		}

			//img->device().machine().options().emu_options::set_value(img->instance_name().c_str(), "", OPTION_PRIORITY_CMDLINE);
			break;

		default:
			if (img->device().type() == CASSETTE)
			{
				cassette_image_device* cassette = dynamic_cast<cassette_image_device*>(&img->device());
				bool s;
				switch(devoption)
				{
					case DEVOPTION_CASSETTE_STOPPAUSE:
						cassette->change_state(CASSETTE_STOPPED, CASSETTE_MASK_UISTATE);
						break;

					case DEVOPTION_CASSETTE_PLAY:
						cassette->change_state(CASSETTE_PLAY, CASSETTE_MASK_UISTATE);
						break;

					case DEVOPTION_CASSETTE_RECORD:
						cassette->change_state(CASSETTE_RECORD, CASSETTE_MASK_UISTATE);
						break;

					case DEVOPTION_CASSETTE_REWIND:
						cassette->seek(0, SEEK_SET); // start
						break;

					case DEVOPTION_CASSETTE_FASTFORWARD:
						cassette->seek(+300.0, SEEK_CUR); // 5 minutes forward or end, whichever comes first
						break;

					case DEVOPTION_CASSETTE_MOTOR:
						s =((cassette->get_state() & CASSETTE_MASK_MOTOR) == CASSETTE_MOTOR_DISABLED);
						cassette->change_state(s ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
						break;

					case DEVOPTION_CASSETTE_SOUND:
						s =((cassette->get_state() & CASSETTE_MASK_SPEAKER) == CASSETTE_SPEAKER_MUTED);
						cassette->change_state(s ? CASSETTE_SPEAKER_ENABLED : CASSETTE_SPEAKER_MUTED, CASSETTE_MASK_SPEAKER);
						break;
				}
			}
			break;
	}
}



//============================================================
//  help_display
//============================================================

static void help_display(HWND wnd, const char *chapter)
{
	using htmlhelpproc = HWND(WINAPI*)(HWND hwndCaller, LPCTSTR pszFile, UINT uCommand, DWORD_PTR dwData);
	static htmlhelpproc htmlhelp;
	static DWORD htmlhelp_cookie;

	if (htmlhelp == nullptr)
	{
//#ifdef UNICODE
//		htmlhelp_funcname = "HtmlHelpW";
//#else
//		htmlhelp_funcname = "HtmlHelpA";
//#endif
		htmlhelp = (htmlhelpproc)system_services::get_proc_address(system_services::load_library(L"hhctrl.ocx"), L"HtmlHelpW");
		if (!htmlhelp)
			return;
		htmlhelp(nullptr, nullptr, 28 /*HH_INITIALIZE*/, (DWORD_PTR) &htmlhelp_cookie);
	}

	// if full screen, turn it off
	if (!is_windowed())
		winwindow_toggle_full_screen();

	std::unique_ptr<const wchar_t[]> wcs_chapter(mui_utf16_from_utf8cstring(chapter));
//  htmlhelp(wnd, t_chapter, 0 /*HH_DISPLAY_TOPIC*/, 0);
//  std::wstring wcs_Site = L"http://messui.polygonal-moogle.com/onlinehelp/";
//  wcs_Site.append(wcs_chapter.get());
//  wcs_Site.append(L".html");
//  shell::shell_execute(wnd, L"open", L"http://www.microsoft.com/directx", L"", nullptr, SW_SHOWNORMAL);
	shell::shell_execute(wnd, L"open", wcs_chapter.get(), L"", nullptr, SW_SHOWNORMAL);
}



//============================================================
//  help_about_mess
//============================================================

static void help_about_mess(HWND wnd)
{
	//help_display(wnd, "mess.chm::/windows/main.htm"); //doesnt do anything
	//help_display(wnd, "mess.chm");
	help_display(wnd, "https://mamedev.org/");
}



//============================================================
//  help_about_thissystem
//============================================================

static void help_about_thissystem(running_machine &machine, HWND wnd)
{
	std::string help_url;
//  help_url = util::string_format("mess.chm::/sysinfo/%s.htm", machine.system().name);
//  help_url = util::string_format("http://messui.polygonal-moogle.com/onlinehelp/%s.html", machine.system().name);
//  help_url = util::string_format("http://www.progettoemma.net/mess/system.php?machine=%s", machine.system().name);
	help_url = util::string_format("http://adb.arcadeitalia.net/dettaglio_mame.php?game_name=%s", machine.system().name);
	help_display(wnd, help_url.c_str());
}



//============================================================
//  decode_deviceoption
//============================================================

static device_image_interface *decode_deviceoption(running_machine &machine, int command, int *devoption)
{
	command -= ID_DEVICE_0;
	int absolute_index = command / DEVOPTION_MAX;

	if (devoption)
		*devoption = command % DEVOPTION_MAX;

	image_interface_enumerator iter(machine.root_device());
	return iter.byindex(absolute_index);
}



//============================================================
//  set_window_orientation
//============================================================

static void set_window_orientation(win_window_info *window, int orientation)
{
	window->target()->set_orientation(orientation);
	if (window->target()->is_ui_target())
	{
		render_container::user_settings settings = window->machine().render().ui_container().get_user_settings();
		settings.m_orientation = orientation;
		window->machine().render().ui_container().set_user_settings(settings);
	}
	window->update();
}



//============================================================
//  pause_for_command
//============================================================

static int pause_for_command(UINT command)
{
	// we really should be more conservative and only pause for commands
	// that do dialog stuff
	return (command != ID_OPTIONS_PAUSE);
}



//============================================================
//  invoke_command
//============================================================

static bool invoke_command(HWND wnd, UINT command)
{
	bool handled = true;
	int dev_command = 0;
	device_image_interface *img;
	LONG_PTR ptr = windows::get_window_long_ptr(wnd, GWLP_USERDATA);
	win_window_info *window = (win_window_info *)ptr;
	mame_ui_manager &mame_ui = mame_machine_manager::instance()->ui();
	ioport_field::user_settings settings;

	// pause while invoking certain commands
	if (pause_for_command(command))
		winwindow_ui_pause(window->machine(), true);

	switch(command)
	{
		case ID_FILE_LOADSTATE_NEWUI:
			state_load(wnd, window->machine());
			break;

		case ID_FILE_SAVESTATE:
			state_save(window->machine());
			break;

		case ID_FILE_SAVESTATE_AS:
			state_save_as(wnd, window->machine());
			break;

		case ID_FILE_SAVESCREENSHOT:
			window->machine().video().save_active_screen_snapshots();
			break;

		case ID_FILE_UIACTIVE:
			mame_ui.set_ui_active(!mame_ui.ui_active());
			break;
		case ID_FILE_EXIT_NEWUI:
			window->machine().schedule_exit();
			break;

		case ID_EDIT_PASTE:
			windows::post_message(wnd, WM_PASTE, 0, 0);
			break;

		case ID_KEYBOARD_NATURAL:
			window->machine().natkeyboard().set_in_use(true);
			break;

		case ID_KEYBOARD_EMULATED:
			window->machine().natkeyboard().set_in_use(false);
			break;

		case ID_KEYBOARD_CUSTOMIZE:
			customise_keyboard(window->machine(), wnd);
			break;

		case ID_VIDEO_ROTATE_0:
			set_window_orientation(window, ROT0);
			break;

		case ID_VIDEO_ROTATE_90:
			set_window_orientation(window, ROT90);
			break;

		case ID_VIDEO_ROTATE_180:
			set_window_orientation(window, ROT180);
			break;

		case ID_VIDEO_ROTATE_270:
			set_window_orientation(window, ROT270);
			break;

		case ID_OPTIONS_PAUSE:
			pause(window->machine());
			break;

		case ID_OPTIONS_HARDRESET:
			window->machine().schedule_hard_reset();
			break;

		case ID_OPTIONS_SOFTRESET:
			window->machine().schedule_soft_reset();
			break;

		case ID_OPTIONS_CONFIGURATION:
			customise_configuration(window->machine(), wnd);
			break;

		case ID_OPTIONS_DIPSWITCHES:
			customise_dipswitches(window->machine(), wnd);
			break;

		case ID_OPTIONS_MISCINPUT:
//			customise_miscinput(window->machine(), wnd); // FIX ME: keybinding isn't working.
			break;

		case ID_OPTIONS_ANALOGCONTROLS:
			customise_analogcontrols(window->machine(), wnd);
			break;

		case ID_FILE_OLDUI:
			mame_ui.show_menu();
			break;

		case ID_FILE_FULLSCREEN:
			winwindow_toggle_full_screen();
			break;

		case ID_OPTIONS_TOGGLEFPS:
			mame_ui.set_show_fps(!mame_ui.show_fps());
			break;

		case ID_OPTIONS_USEMOUSE:
			{
				// FIXME
//              extern int win_use_mouse;
//              win_use_mouse = !win_use_mouse;
			}
			break;

		case ID_FILE_TOGGLEMENUBAR:
			win_toggle_menubar();
			break;

		case ID_FRAMESKIP_AUTO:
			window->machine().video().set_frameskip(-1);
			window->machine().options().emu_options::set_value(OPTION_AUTOFRAMESKIP, 1, OPTION_PRIORITY_CMDLINE);
			break;

		case ID_HELP_ABOUT_NEWUI:
			help_about_mess(wnd);
			break;

		case ID_HELP_ABOUTSYSTEM:
			help_about_thissystem(window->machine(), wnd);
			break;

		case ID_THROTTLE_50:
			set_speed(window->machine(), 500);
			break;

		case ID_THROTTLE_100:
			set_speed(window->machine(), 1000);
			break;

		case ID_THROTTLE_200:
			set_speed(window->machine(), 2000);
			break;

		case ID_THROTTLE_500:
			set_speed(window->machine(), 5000);
			break;

		case ID_THROTTLE_1000:
			set_speed(window->machine(), 10000);
			break;

		case ID_THROTTLE_UNTHROTTLED:
			set_speed(window->machine(), 0);
			break;

		default:
			if ((command >= ID_FRAMESKIP_0) && (command < ID_FRAMESKIP_0 + frameskip_level_count(window->machine())))
			{
				// change frameskip
				window->machine().video().set_frameskip(command - ID_FRAMESKIP_0);
				window->machine().options().emu_options::set_value(OPTION_AUTOFRAMESKIP, 0, OPTION_PRIORITY_CMDLINE);
				window->machine().options().emu_options::set_value(OPTION_FRAMESKIP, (int)command - ID_FRAMESKIP_0, OPTION_PRIORITY_CMDLINE);
			}
			else
			if ((command >= ID_DEVICE_0) && (command < ID_JOYSTICK_0))
			{
				// change device
				img = decode_deviceoption(window->machine(), command, &dev_command);
				device_command(wnd, img, dev_command);
			}
			else
			if ((command >= ID_JOYSTICK_0) && (command < ID_JOYSTICK_0 + MAX_JOYSTICKS))
				// customise joystick
				customise_joystick(window->machine(), wnd, command - ID_JOYSTICK_0);
			else
			if ((command >= ID_VIDEO_VIEW_0) && (command < ID_VIDEO_VIEW_0 + 1000))
			{
				// render views
				window->target()->set_view(command - ID_VIDEO_VIEW_0);
				window->update(); // actually change window size
			}
			else
			if ((command >= 3400) && (command < 4000))
			{
				slot_option &opt(window->machine().options().slot_option(slot_map[command].slotname.c_str()));
				opt.specify(slot_map[command].optname.c_str());
				window->machine().schedule_hard_reset();
			}
			else
			if ((command >= ID_SWPART) && (command < ID_SWPART + 100))
			{
				int mapindex = command - ID_SWPART;
				img = part_map[mapindex].img;
				std::string instance = std::string(img->software_list_name()) + ":" + std::string(img->basename()) + ":" + part_map[mapindex].part_name;
//				std::cout << "Loading index " << mapindex << " : " << instance << " *******" << std::endl;
				img->load_software(instance);
			}
			else
				// bogus command
				handled = false;

			break;
	}

	// resume emulation
	if (pause_for_command(command))
		winwindow_ui_pause(window->machine(), false);

	return handled;
}



//============================================================
//  set_menu_text
//============================================================

static void set_menu_text(HMENU menu_bar, int command, const char *text)
{
	// convert to WCHAR
	std::unique_ptr<wchar_t[]> t_text(const_cast<wchar_t*>(mui_utf16_from_utf8cstring(text)));

	// invoke SetMenuItemInfo()
	MENUITEMINFOW mii = { sizeof(mii) };
	mii.fMask = MIIM_TYPE;
	mii.dwTypeData = t_text.get();
	menus::set_menu_item_info(menu_bar, command, false, &mii);
}



//============================================================
//  win_setup_menus
//============================================================

static int win_setup_menus(running_machine &machine, HMODULE module, HMENU menu_bar)
{
	HMENU frameskip_menu;
	std::string buf;
	int i = 0;

	// initialize critical values
	joystick_menu_setup = 0;

	// set up frameskip menu
	frameskip_menu = find_sub_menu(menu_bar, "&Options\0&Frameskip\0", false);

	if (!frameskip_menu)
		return 1;

	for(i = 0; i < frameskip_level_count(machine); i++)
	{
		buf = util::string_format(" % i", i);
		menus::append_menu_utf8(frameskip_menu, MF_STRING, ID_FRAMESKIP_0 + i, buf.c_str());
	}

	// set the help menu to refer to this machine
	buf = util::string_format("About %s (%s)...", machine.system().type.fullname(), machine.system().name);
	set_menu_text(menu_bar, ID_HELP_ABOUTSYSTEM, buf.c_str());

	// initialize state_filename for each driver, so we don't carry names in-between them
	{
		char *src;
		char *dst;

		state_filename = util::string_format("%s State", machine.system().type.fullname());

		src = &state_filename[0];
		dst = &state_filename[0];
		do
		{
			if (isalnum(*src) || mui_strchr("(),. \0", *src))
				*(dst++) = *src;
		}
		while(*(src++));
	}

	return 0;
}



//============================================================
//  win_resource_module
//============================================================

static void win_resource_module(HMODULE &output_resource_module)
{
	if (output_resource_module == nullptr)
	{
		MEMORY_BASIC_INFORMATION info;
		if ((system_services::virtual_query((const void*)win_resource_module, &info, sizeof(info))) == sizeof(info))
			output_resource_module = (HMODULE)info.AllocationBase;
	}
}

///// Interface to the core /////

//============================================================
//  winwindow_create_menu
//============================================================

int winwindow_create_menu(running_machine &machine, HMENU *menus)
{
	// do not show in the mewui ui.
	if (mui_strcmp(machine.system().name, "___empty") == 0)
		return 0;

	HMODULE res_module = nullptr;
	win_resource_module(res_module);
	HMENU menu_bar = menus::load_menu(res_module, menus::make_int_resource(IDR_RUNTIME_MENU));

	if (!menu_bar)
	{
		std::cout << "No memory for the menu, running without it." << std::endl;
		return 0;
	}

	if (win_setup_menus(machine, res_module, menu_bar))
	{
		std::cout << "Unable to setup the menu, running without it." << std::endl;
		if (menu_bar)
			(void)menus::destroy_menu(menu_bar);
		return 0; // return 1 causes a crash
	}

	*menus = menu_bar;
	return 0;
}



//============================================================
//  winwindow_video_window_proc_ui
//============================================================

LRESULT CALLBACK winwindow_video_window_proc_ui(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch(message)
	{
		case WM_INITMENU:
			prepare_menus(wnd);
			break;

		case WM_PASTE:
			{
				mame_machine_manager::instance()->ui().machine().natkeyboard().paste();
			}
			break;

		case WM_COMMAND:
			if (invoke_command(wnd, wparam))
				break;
			/* fall through */

		default:
			return win_window_info::video_window_proc(wnd, message, wparam, lparam);
	}
	return 0;
}
