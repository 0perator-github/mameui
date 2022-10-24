// For licensing and usage information, read docs/release/winui_license.txt
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
#include <string_view>
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
//#define interface struct

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

using namespace mameui::util::string_util;
using namespace mameui::winapi::controls;
using namespace mameui::winapi;

//============================================================
//  TYPE DEFINITIONS
//============================================================

using dialog_box = struct dialog_box;
using dialog_info_trigger = struct dialog_info_trigger;
using dialog_layout = struct dialog_layout;
using dialog_object_pool = struct dialog_object_pool;
using seqselect_info = struct seqselect_info;
using win_open_file_name = struct win_open_file_name;

using dialog_itemchangedproc = void (*)(dialog_box *dialog, HWND dlgitem, void *changed_param);
using dialog_itemstoreval = void (*)(void *param, int val);
using dialog_notification = void (*)(dialog_box *dialog, HWND dlgwnd, NMHDR *notification, void *param);
using trigger_function = LRESULT(*)(dialog_box *dialog, HWND dlgwnd, UINT message, WPARAM wparam, LPARAM lparam);

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
	win_file_dialog_type dialog_type;               // type of file dialog
	HWND owner = nullptr;                           // owner of the dialog
	HINSTANCE instance = nullptr;                   // instance
	const wchar_t *filter = nullptr;                // pipe char ("|") delimited strings
	DWORD filter_index = 0;                         // index into filter
	wchar_t filename[MAX_PATH]{ L'\0' };            // filename buffer
	const wchar_t *initial_directory = nullptr;     // initial directory for dialog
	DWORD flags = 0;                                // standard flags
	LPARAM custom_data = 0;                         // custom data for dialog hook
	LPOFNHOOKPROC hook = nullptr;                   // custom dialog hook
	const wchar_t *template_name = nullptr;         // custom dialog template
};

struct dialog_layout
{
	short label_width = 0;
	short combo_width = 0;
};


struct dialog_info_trigger
{
	dialog_info_trigger *next = nullptr;
	WORD dialog_item = 0;
	WORD trigger_flags = 0;
	uint32_t message = 0;
	WPARAM wparam;
	LPARAM lparam;
	dialog_itemstoreval store_val;
	void *storeval_param = nullptr;
	trigger_function trigger_proc;
};

struct dialog_object_pool
{
	HGDIOBJ objects[16] = { nullptr };
};

struct dialog_box
{
	std::vector<uint8_t> buffer;
	dialog_info_trigger *trigger_first = nullptr;
	dialog_info_trigger *trigger_last = nullptr;
	WORD item_count = 0;
	WORD size_x = 0, size_y = 0;
	WORD pos_x = 0, pos_y = 0;
	WORD cursize_x = 0, cursize_y = 0;
	WORD home_y = 0;
	DWORD style = 0;
	int32_t combo_string_count = 0;
	int32_t combo_default_value = 0;
	//object_pool *mempool;
	dialog_object_pool *objpool = nullptr;
	const dialog_layout *layout = nullptr;

	// singular notification callback; hack
	uint32_t notify_code = 0;
	dialog_notification notify_callback = nullptr;
	void *notify_param = nullptr;
};

// this is the structure that gets associated with each input_seq edit box

struct seqselect_info
{
	bool is_analog = false;
	input_seq *code = nullptr;              // the input_seq within settings
	ioport_field *field = nullptr;          // pointer to the field
	ioport_field::user_settings settings;   // the new settings
	seqselect_state poll_state;
	std::unique_ptr<switch_sequence_poller> seq_poll;
	WNDPROC oldwndproc = nullptr;
	WORD pos = 0;
};

struct slot_data
{
	std::string slotname;
	std::string optname;
};

struct part_data
{
	std::string part_name;
	device_image_interface *img = nullptr;
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

#define DLGITEM_BUTTON          ((const wchar_t *) dlgitem_button)
#define DLGITEM_EDIT            ((const wchar_t *) dlgitem_edit)
#define DLGITEM_STATIC          ((const wchar_t *) dlgitem_static)
#define DLGITEM_LISTBOX         ((const wchar_t *) dlgitem_listbox)
#define DLGITEM_SCROLLBAR       ((const wchar_t *) dlgitem_scrollbar)
#define DLGITEM_COMBOBOX        ((const wchar_t *) dlgitem_combobox)

#define DLGTEXT_OK              L"OK"
#define DLGTEXT_APPLY           L"Apply"
#define DLGTEXT_CANCEL          L"Cancel"

#define FONT_SIZE               8
#define FONT_FACE               L"Arial"

#define ID_FRAMESKIP_0          10000
#define ID_DEVICE_0             11000
#define ID_JOYSTICK_0           12000
#define ID_VIDEO_VIEW_0         14000
#define ID_SWPART               15000

#define MAX_JOYSTICKS    (8)

#define SCROLL_DELTA_LINE       10
#define SCROLL_DELTA_PAGE       100

#define SEQSELECT_POLL_TIMER_ID 0xFACE

#define SEQWM_SETFOCUS          (WM_APP + 549)
#define SEQWM_KILLFOCUS         (WM_APP + 550)

#define LOG_WINMSGS             0
#define DIALOG_STYLE            WS_POPUP | WS_BORDER | WS_SYSMENU | DS_MODALFRAME | WS_CAPTION | DS_SETFONT
#define MAX_DIALOG_HEIGHT       200

//============================================================
//  LOCAL VARIABLES
//============================================================

static running_machine *Machine;         // HACK - please fix

// conversion factors for pixels to dialog units
static double pixels_to_xdlgunits;
static double pixels_to_ydlgunits;

static dialog_layout default_layout = { 80, 140 };

// dialog item class names
static const WORD dlgitem_button[] = { 0xFFFF, 0x0080 };
static const WORD dlgitem_edit[] = { 0xFFFF, 0x0081 };
static const WORD dlgitem_static[] = { 0xFFFF, 0x0082 };
static const WORD dlgitem_listbox[] = { 0xFFFF, 0x0083 };
static const WORD dlgitem_scrollbar[] = { 0xFFFF, 0x0084 };
static const WORD dlgitem_combobox[] = { 0xFFFF, 0x0085 };

static bool joystick_menu_setup = false;

static std::wstring state_directoryname;
static std::wstring state_filename;

static std::map<std::string, std::string> slmap; // software list map
static std::map<int, slot_data> slot_map; // slot map
static std::map<int, part_data> part_map; // part map

// static menu paths
constexpr std::wstring_view menu_path_frameskip(L"&Options\0&Frameskip\0", 21);
constexpr std::wstring_view menu_path_joysticks(L"&Options\0&Joysticks\0", 21);
constexpr std::wstring_view menu_path_media(L"&Media\0", 8);
constexpr std::wstring_view menu_path_slots(L"&Slots\0", 8);
constexpr std::wstring_view menu_path_video(L"&Options\0&Video\0", 17);

// static dialog titles
constexpr std::wstring_view dialog_title_miscinput(L"Miscellaneous Inputs");
constexpr std::wstring_view dialog_title_analog(L"Analog Controls");
constexpr std::wstring_view dialog_title_drivercfg(L"Driver Configuration");
constexpr std::wstring_view dialog_title_dipswitch(L"DIP Switches");


//============================================================
//  PROTOTYPES
//============================================================

//static bool win_append_menu(HMENU menu, UINT flags, UINT_PTR id, const wchar_t *item);
//static std::vector<wchar_t> convert_null_to_pipe_delimited_string(const wchar_t *input);
static bool get_menu_item_string(HMENU menu, UINT item, bool by_position, HMENU *sub_menu, std::vector<wchar_t> &buffer);
static bool get_softlist_info(HWND wnd, device_image_interface *img);
static bool invoke_command(HWND wnd, UINT command);
static bool state_generate_filename(const std::wstring_view full_name);
static bool win_append_menu_utf8(HMENU menu, UINT flags, UINT_PTR id, const char *item);
static bool win_file_dialog(running_machine &machine, HWND parent, win_file_dialog_type dlgtype, const wchar_t *filter, const wchar_t *initial_dir, wchar_t *filename);
static bool win_get_file_name_dialog(win_open_file_name *ofn);
static device_image_interface *decode_deviceoption(running_machine &machine, int command, int *devoption);
static dialog_box *win_dialog_init(const std::wstring_view title, const struct dialog_layout *layout);
static HMENU find_sub_menu(HMENU menu, const std::wstring_view menu_path, bool create_sub_menu);
static int add_portselect_entry(dialog_box *di, short &x, short &y, std::wstring_view port_name, std::wstring_view port_suffix, ioport_field *field, bool is_analog, int seq_type);
static int dialog_add_scrollbar(dialog_box *dialog);
static int dialog_add_single_seqselect(dialog_box *di, short x, short y, short cx, short cy, ioport_field *field, int is_analog, int seqtype);
static int dialog_add_trigger(dialog_box *di, WORD dialog_item, WORD trigger_flags, UINT message, trigger_function trigger_proc, WPARAM wparam, LPARAM lparam, void (*storeval)(void *param, int val), void *storeval_param);
static int dialog_write(dialog_box *di, const void *ptr, size_t sz, int align);
static int dialog_write_item(dialog_box *di, DWORD style, short x, short y, short width, short height, const std::wstring_view str, const wchar_t *class_name, WORD *id);
static int dialog_write_string(dialog_box *di, const std::wstring_view str);
static int frameskip_level_count(running_machine &machine);
static int is_windowed(void);
static int pause_for_command(UINT command);
static int port_type_is_analog(int type);
static int win_dialog_add_active_combobox(dialog_box *dialog, const wchar_t *item_label, int default_value, dialog_itemstoreval storeval, void *storeval_param, dialog_itemchangedproc changed, void *changed_param);
static int win_dialog_add_adjuster(dialog_box *dialog, const wchar_t *item_label, int default_value, int min_value, int max_value, bool is_percentage, dialog_itemstoreval storeval, void *storeval_param);
static int win_dialog_add_combobox(dialog_box *dialog, const wchar_t *item_label, int default_value, void (*storeval)(void *param, int val), void *storeval_param);
static int win_dialog_add_combobox_item(dialog_box *dialog, const wchar_t *item_label, int item_data);
static int win_dialog_add_portselect(dialog_box *dialog, ioport_field *field);
static int win_dialog_add_standard_buttons(dialog_box *dialog);
static int win_setup_menus(running_machine &machine, HMENU menu_bar);
static INT_PTR CALLBACK adjuster_sb_wndproc(HWND sbwnd, UINT msg, WPARAM wparam, LPARAM lparam);
static INT_PTR CALLBACK dialog_proc(HWND dlgwnd, UINT msg, WPARAM wparam, LPARAM lparam);
static INT_PTR CALLBACK seqselect_wndproc(HWND editwnd, UINT msg, WPARAM wparam, LPARAM lparam);
static LRESULT adjuster_sb_setup(dialog_box *dialog, HWND sbwnd, UINT message, WPARAM wparam, LPARAM lparam);
static LRESULT call_windowproc(WNDPROC wndproc, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
static LRESULT dialog_combo_changed(dialog_box *dialog, HWND dlgitem, UINT message, WPARAM wparam, LPARAM lparam);
static LRESULT dialog_get_adjuster_value(dialog_box *dialog, HWND dialog_item, UINT message, WPARAM wparam, LPARAM lparam);
static LRESULT dialog_get_combo_value(dialog_box *dialog, HWND dialog_item, UINT message, WPARAM wparam, LPARAM lparam);
static LRESULT dialog_scrollbar_init(dialog_box *dialog, HWND dlgwnd, UINT message, WPARAM wparam, LPARAM lparam);
static LRESULT seqselect_apply(dialog_box *dialog, HWND editwnd, UINT message, WPARAM wparam, LPARAM lparam);
static LRESULT seqselect_setup(dialog_box *dialog, HWND editwnd, UINT message, WPARAM wparam, LPARAM lparam);
static seqselect_info *get_seqselect_info(HWND editwnd);
static std::string truncate_long_text_utf8(std::string_view incoming, size_t character_count);
static std::vector<wchar_t> convert_pipe_to_null_delimited_string(const std::wstring_view input);
static std::wstring win_dirname(std::wstring_view filename);
static void add_filter_entry(std::wstring &filter, const wchar_t *description, const wchar_t *extensions);
static void after_display_dialog(running_machine &machine);
static void before_display_dialog(running_machine &machine);
static void build_generic_filter(device_image_interface *img, bool is_save, std::wstring &filter);
static void calc_dlgunits_multiple(void);
static void change_device(HWND wnd, device_image_interface *image, bool is_save);
static void copy_extension_list(std::wstring &filter, const wchar_t *extensions);
static void customise_analogcontrols(running_machine &machine, HWND wnd);
static void customise_configuration(running_machine &machine, HWND wnd);
static void customise_dipswitches(running_machine &machine, HWND wnd);
static void customise_input(running_machine &machine, HWND wnd, const wchar_t *title, int player, int inputclass);
static void customise_joystick(running_machine &machine, HWND wnd, int joystick_num);
static void customise_keyboard(running_machine &machine, HWND wnd);
static void customise_miscinput(running_machine &machine, HWND wnd);
static void customise_switches(running_machine &machine, HWND wnd, std::wstring_view title, UINT32 ipt_name);
static void device_command(HWND wnd, device_image_interface *img, int devoption);
static void dialog_finish_control(dialog_box *di, short x, short y);
static void dialog_new_control(dialog_box *di, short *x, short *y);
static void dialog_prime(dialog_box *di);
static void dialog_trigger(HWND dlgwnd, WORD trigger_flags);
static void help_about_mess(HWND wnd);
static void help_about_thissystem(running_machine &machine, HWND wnd);
static void help_display(HWND wnd, const wchar_t *chapter);
static void load_item(HWND wnd, device_image_interface *img, bool is_save);
static void pause(running_machine &machine);
static void prepare_menus(HWND wnd);
static void remove_menu_items(HMENU menu);
static void seqselect_settext(HWND editwnd);
static void seqselect_start_read_from_main_thread(void *param);
static void seqselect_stop_read_from_main_thread(void *param);
static void set_command_state(HMENU menu_bar, UINT command, UINT state);
static void set_menu_text(HMENU menu_bar, int command, const wchar_t *menu_text);
static void set_speed(running_machine &machine, int speed);
static void set_window_orientation(win_window_info *window, int orientation);
static void setup_joystick_menu(running_machine &machine, HMENU menu_bar);
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
static void win_resource_module(HMODULE &output_resource_module);
static void win_scroll_window(HWND window, WPARAM wparam, int scroll_bar, int scroll_delta_line);
static void win_toggle_menubar(void);


//============================================================
//  PARAMETERS
//============================================================



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



//=============================================================
//  truncate_long_text_utf8
//    This function truncates a long string, replacing the end
//  with ... for displaying long text in the UI, such as in
//  tooltips or labels.
//  called from win_get_file_name_dialog_utf8
//==============================================================

static std::string truncate_long_text_utf8(std::string_view incoming, size_t character_count)
{
	if (character_count < 4 || incoming.length() < character_count)
		return std::string{};

	std::string outgoing = std::string(incoming);

	for (char &ch : outgoing)
		if (ch == '\n')
			ch = ' ';

	if (incoming.length() <= character_count)
		return outgoing;

	return outgoing.substr(0, character_count - 3) + "...";
}

#if 0 // This function is not used in the current codebase, but is kept for potential future use.
//=============================================================
//  truncate_long_text
//    This function truncates a long wide string, replacing the
//  end with ... for displaying long text in the UI, such as in
//  tooltips or labels.
// called from win_get_file_name_dialog
//==============================================================

static std::wstring truncate_long_text(std::wstring_view incoming, size_t character_count)
{
	if (character_count < 4 || incoming.length() < character_count)
		return std::wstring{};

	std::wstring outgoing = std::wstring(incoming);

	for (wchar_t &ch : outgoing)
		if (ch == L'\n')
			ch = L' ';

	if (incoming.length() <= character_count)
		return outgoing;

	return outgoing.substr(0, character_count - 3) + L"...";
}
#endif // This function is not used in the current codebase, but is kept for potential future use.

//=============================================================
//  convert_pipe_to_null_delimited_string
//    convert a pipe-delimited string into a null-delimited
//  string so that it can be used with the OPENFILENAME
//  structure
//  called from win_get_file_name_dialog
//=============================================================

static std::vector<wchar_t> convert_pipe_to_null_delimited_string(const std::wstring_view input)
{
	if (input.empty())
	{
		return { L'\0' }; // Return a vector with a single null terminator
	}

	std::vector<wchar_t> output;

	// copy characters from input to output, replacing '|' with '\0'
	for (wchar_t ch : input)
		output.push_back(ch == L'|' ? L'\0' : ch);

	output.push_back(L'\0'); // Ensure the output is null-terminated

	return output;
}
#if 0 // This function is not used in the current codebase, but is kept for potential future use.
//=============================================================
//  convert_null_to_pipe_delimited_string
//    convert a null-delimited string into a pipe-delimited
//  string so that it can be used with the win_open_file_name
//  structure
//  called from ...
//=============================================================

static std::vector<wchar_t> convert_null_to_pipe_delimited_string(const wchar_t *input)
{
	std::vector<wchar_t> output;

	// If input is null or empty, return a vector with a single null terminator
	if (!input || *input == L'\0')
	{
		output.push_back(L'\0');
		return output;
	}

	const wchar_t *char_pos = input;

	while (*char_pos != L'\0')
	{
		// Copy characters until we hit a null terminator
		while (*char_pos != L'\0')
		{
			output.push_back(*char_pos++);
		}

		// If the next character is not the final null terminator, insert pipe
		if (*(char_pos + 1) != L'\0')
		{
			output.push_back(L'|');
		}

		++char_pos; // Skip the null terminator between strings
	}

	output.push_back(L'\0'); // Ensure null-terminated result

	return output;
}
#endif // This function is not used in the current codebase, but is kept for potential future use.

//============================================================
//  win_get_file_name_dialog - sanitize all of the ugliness
//  in invoking GetOpenFileName() and GetSaveFileName()
//     called from win_file_dialog, state_dialog
//============================================================

static bool win_get_file_name_dialog(win_open_file_name *ofn)
{
	bool dialog_result;
	bool result = 0;
	OPENFILENAMEW os_ofn{};

	// translate our custom structure to a native Win32 structure
	std::vector<wchar_t> os_ofn_filter = convert_pipe_to_null_delimited_string(ofn->filter);

	os_ofn.lStructSize = sizeof(OPENFILENAMEW);
	os_ofn.hwndOwner = ofn->owner;
	os_ofn.hInstance = ofn->instance;
	os_ofn.lpstrFilter = os_ofn_filter.data();
	os_ofn.nFilterIndex = ofn->filter_index;
	os_ofn.lpstrFile = ofn->filename;
	os_ofn.nMaxFile = MAX_PATH;
	os_ofn.lpstrInitialDir = ofn->initial_directory;
	os_ofn.Flags = ofn->flags;
	os_ofn.lCustData = ofn->custom_data;
	os_ofn.lpfnHook = ofn->hook;
	os_ofn.lpTemplateName = ofn->template_name;

	// invoke the correct Win32 call
	switch (ofn->dialog_type)
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
	SCROLLINFO si;
	int scroll_pos;

	// retrieve vital info about the scroll bar
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
	GetScrollInfo(window, scroll_bar, &si);

	scroll_pos = si.nPos;

	// change scroll_pos in accordance with this message
	switch (LOWORD(wparam))
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
		SetScrollPos(window, scroll_bar, scroll_pos, true);
		ScrollWindowEx(window, 0, si.nPos - scroll_pos, nullptr, nullptr, nullptr, nullptr, SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE);
	}
}

#if 0 // This function is not used in the current codebase, but is kept for potential future use.
//============================================================
//  win_append_menu
//    create a menu item
//============================================================

static bool win_append_menu(HMENU menu, UINT flags, UINT_PTR id, const wchar_t *item)
{
	if (!menu)
	{
		std::cerr << "Error: win_append_menu_utf8 called with a null menu handle." << "\n";
		return false;
	}

	if (!item || !*item)
	{
		std::cerr << "Error: win_append_menu_utf8 called with an empty menu item string." << "\n";
		return false;
	}

	return menus::append_menu(menu, flags, id, item);
}
#endif // This function is not used in the current codebase, but is kept for potential future use.

//============================================================
//  win_append_menu_utf8
//    create a menu item
//============================================================

static bool win_append_menu_utf8(HMENU menu, UINT flags, UINT_PTR id, const char *item)
{
	if (!menu)
	{
		std::cerr << "Error: win_append_menu_utf8 called with a null menu handle." << "\n";
		return false;
	}

	if (!item || !*item)
	{
		std::cerr << "Error: win_append_menu_utf8 called with an empty menu item string." << "\n";
		return false;
	}

	return menus::append_menu_utf8(menu, flags, id, item);
}



//============================================================
//  call_windowproc
//    called from adjuster_sb_wndproc, seqselect_wndproc
//============================================================

static LRESULT call_windowproc(WNDPROC wndproc, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	LRESULT rc;
	if (IsWindowUnicode(hwnd))
		rc = CallWindowProcW(wndproc, hwnd, msg, wparam, lparam);
	else
		rc = CallWindowProcA(wndproc, hwnd, msg, wparam, lparam);
	return rc;
}



//==========================================================================
//  dialog_write
//    called from dialog_write_string, win_dialog_init, dialog_write_item
//==========================================================================

static int dialog_write(dialog_box *di, const void *ptr, size_t sz, int align)
{
	if (!di)
	{
		std::cerr << "Error: dialog_write: Called with a null dialogbox pointer" << "\n";
		return 1;
	}
	if (!ptr || sz == 0)
	{
		std::cerr << "Error: dialog_write: Called with a empty buffer to write" << "\n";
		return 1;
	}

	size_t base;

	if (di->buffer.empty())
	{
		base = 0;
		di->buffer.resize(sz, 0);  // Zero-initialize
	}
	else
	{
		base = (di->buffer.size() + align - 1) & ~(align - 1);
		if (base + sz > di->buffer.size())
			di->buffer.resize(base + sz, 0);  // Resize + zero-fill
	}

	std::memcpy(&di->buffer[base], ptr, sz);
	return 0;
}


//============================================================
//  dialog_write_string
//    called from win_dialog_init, dialog_write_item
//============================================================

static int dialog_write_string(dialog_box *di, const std::wstring_view str)
{
	if (!di)
	{
		std::cerr << "Error: dialog_write_string: Called with a null dialogbox pointer" << "\n";
		return 1;
	}

	std::wstring buffer{ str };

	return dialog_write(di, buffer.c_str(), (buffer.size() + 1) * sizeof(wchar_t), sizeof(wchar_t));
}




//============================================================
//  win_dialog_exit
//    called from win_dialog_init, calc_dlgunits_multiple, change_device, and all customise_input functions
//============================================================

static void win_dialog_exit(dialog_box *dialog)
{

	if (!dialog)
	{
		std::cerr << "Error: win_dialog_exit: Called with a null dialogbox pointer" << "\n";
		return;
	}
	assert(dialog);

	dialog_object_pool *objpool = dialog->objpool;
	if (objpool)
	{
		for (int i = 0; i < std::size(objpool->objects); i++)
			DeleteObject(objpool->objects[i]);
	}

	if (!dialog->buffer.empty())
		dialog->buffer.clear();

	delete dialog;
}



//===========================================================================
//  win_dialog_init
//    called from calc_dlgunits_multiple, and all customise_input functions
//===========================================================================

static dialog_box *win_dialog_init(const std::wstring_view title, const struct dialog_layout *layout)
{
	// check for a valid title
	if (title.empty())
	{
		std::cerr << "Error: win_dialog_init: Called with an empty title string" << "\n";
		return nullptr;
	}

	// create the dialog structure
	dialog_box *di = new(std::nothrow) dialog_box();
	if (!di)
		return nullptr;

	// use default layout if not specified
	di->layout = (!layout) ? &default_layout : layout;

	DLGTEMPLATE dlg_template{};
	dlg_template.style = di->style = DIALOG_STYLE;
	dlg_template.x = 10;
	dlg_template.y = 10;

	int rc = dialog_write(di, &dlg_template, sizeof(dlg_template), sizeof(DWORD));
	if (rc)
	{
		win_dialog_exit(di);
		return nullptr;
	}

	WORD w[2]{};

	rc = dialog_write(di, w, sizeof(w), sizeof(WORD));
	if (rc)
	{
		win_dialog_exit(di);
		return nullptr;
	}

	rc = dialog_write_string(di, title);
	if (rc)
	{
		win_dialog_exit(di);
		return nullptr;
	}

	// set the font, if necessary
	if (di->style & DS_SETFONT)
	{
		WORD font_size = static_cast<WORD>(FONT_SIZE);

		rc = dialog_write(di, &font_size, sizeof(font_size), sizeof(WORD));
		if (rc)
		{
			win_dialog_exit(di);
			return nullptr;
		}

		rc = dialog_write_string(di, FONT_FACE);
		if (rc)
		{
			win_dialog_exit(di);
			return nullptr;
		}
	}

	return di;
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
	const wchar_t *wnd_title = L"Foo";
	WORD id;
	HWND dlg_window = nullptr;
	HWND child_window;
	RECT r;

	if ((pixels_to_xdlgunits == 0) || (pixels_to_ydlgunits == 0))
	{
		// create a bogus dialog
		dialog = win_dialog_init(L"", nullptr);
		if (!dialog)
			goto done;

		if (dialog_write_item(dialog, WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 0, offset_x, offset_y, wnd_title, DLGITEM_STATIC, &id))
			goto done;

		dialog_prime(dialog);
		dlg_window = CreateDialogIndirectParam(nullptr, (const DLGTEMPLATE*)dialog->buffer.data(), nullptr, nullptr, 0);
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
	LONG_PTR l = windows::get_window_long_ptr(dlgwnd, GWLP_USERDATA);

	dialog_box *di = (dialog_box *) l;
	if (!di)
	{
		std::cerr << "Error: dialog_trigger: Called with a null dialogbox pointer" << "\n";
		return;
	}
	assert(di);

	for (dialog_info_trigger *trigger = di->trigger_first; trigger; trigger = trigger->next)
	{
		if (trigger->trigger_flags & trigger_flags)
		{
			HWND dialog_item = nullptr;
			if (trigger->dialog_item)
				dialog_item = dialog_boxes::get_dlg_item(dlgwnd, trigger->dialog_item);
			else
				dialog_item = dlgwnd;

			if (!dialog_item)
			{
				std::cerr << "Error: dialog_trigger: Could not find dialog item with ID " << trigger->dialog_item << "\n";
				continue;
			}
			assert(dialog_item);

			LRESULT result = 0;
			if (trigger->message)
				result = windows::send_message(dialog_item, trigger->message, trigger->wparam, trigger->lparam);

			if (trigger->trigger_proc)
				result = trigger->trigger_proc(di, dialog_item, trigger->message, trigger->wparam, trigger->lparam);

			if (trigger->store_val)
				trigger->store_val(trigger->storeval_param, result);
		}
	}
}

//============================================================
//  dialog_proc
//    called from win_dialog_runmodal
//============================================================

static INT_PTR CALLBACK dialog_proc(HWND dlgwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	INT_PTR handled = true;
	WORD command;

	switch(msg)
	{
		case WM_INITDIALOG:
			windows::set_window_long_ptr(dlgwnd, GWLP_USERDATA, (LONG_PTR) lparam);
			dialog_trigger(dlgwnd, TRIGGER_INITDIALOG);
			break;

		case WM_COMMAND:
		{
			command = LOWORD(wparam);
			const HWND ctlwnd = reinterpret_cast<HWND>(lparam);
			auto ctl_text = std::unique_ptr<wchar_t[]>(windows::get_window_text(ctlwnd));
			if (!ctl_text)
				ctl_text.reset(new wchar_t[] { L'\0' });

			if (!mui_wcscmp(ctl_text.get(), DLGTEXT_OK))
				command = IDOK;
			else if (!mui_wcscmp(ctl_text.get(), DLGTEXT_CANCEL))
				command = IDCANCEL;
			else
				command = 0;

			switch (command)
			{
			case IDOK:
				dialog_trigger(dlgwnd, TRIGGER_APPLY);
				// fall through

			case IDCANCEL:
				EndDialog(dlgwnd, 0);
				break;

			default:
				handled = false;
				break;
			}
			break;
		}
		case WM_SYSCOMMAND:
			if (wparam == SC_CLOSE)
				EndDialog(dlgwnd, 0);
			else
				handled = false;

			break;

		case WM_VSCROLL:
			if (lparam)
				// this scroll message came from an actual scroll bar window;
				// pass it on
				windows::send_message((HWND) lparam, msg, wparam, lparam);
			else
				// scroll the dialog
				win_scroll_window(dlgwnd, wparam, SB_VERT, SCROLL_DELTA_LINE);

			break;

		default:
			handled = false;
			break;
	}
	return handled;
}



//=========================================================================================================================================================================================
//  dialog_write_item
//    called from calc_dlgunits_multiple, win_dialog_add_active_combobox, win_dialog_add_adjuster, dialog_add_single_seqselect, win_dialog_add_portselect, win_dialog_add_standard_buttons
//=========================================================================================================================================================================================

static int dialog_write_item(dialog_box *di, DWORD style, short x, short y, short width, short height, const std::wstring_view str, const wchar_t *class_name, WORD *id)
{
	if (!di)
	{
		std::cerr << "Error: dialog_write_item: Called with a null dialogbox pointer" << "\n";
		return 1;
	}

	if (!class_name || !*class_name)
	{
		std::cerr << "Error: dialog_write_item: Called with an empty class name" << "\n";
		return 1;
	}

	DLGITEMTEMPLATE item_template{};
	int rc;

	item_template.style = style;
	item_template.x = x;
	item_template.y = y;
	item_template.cx = width;
	item_template.cy = height;
	item_template.id = ++di->item_count;

	rc = dialog_write(di, &item_template, sizeof(item_template), sizeof(DWORD));
	if (rc)
		return 1;

	bool is_atom = reinterpret_cast<const WORD*>(class_name)[0] == 0xffff;
	size_t class_name_length = (is_atom) ? 4 : (wcslen(class_name) + 1) * sizeof(wchar_t);

	rc = dialog_write(di, class_name, class_name_length, sizeof(wchar_t));
	if (rc)
		return 1;

	rc = dialog_write_string(di, str);
	if (rc)
		return 1;

	WORD w = 0;
	rc = dialog_write(di, &w, sizeof(w), sizeof(WORD));
	if (rc)
		return 1;

	if (id)
		*id = item_template.id;

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
	{
		std::cerr << "Error: dialog_add_trigger: Called with a null dialogbox pointer" << "\n";
		return 1;
	}

	if (!trigger_flags)
	{
		std::cerr << "Error: dialog_add_trigger: Called with a zero trigger_flags" << "\n";
		return 1;
	}

	dialog_info_trigger *trigger = new(dialog_info_trigger);

	trigger->next = nullptr;
	trigger->trigger_flags = trigger_flags;
	trigger->dialog_item = dialog_item;
	trigger->message = message;
	trigger->trigger_proc = trigger_proc;
	trigger->wparam = wparam;
	trigger->lparam = lparam;
	trigger->store_val = storeval;
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

	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	si.nMin  = pixels_to_ydlgunits * 0;
	si.nMax  = pixels_to_ydlgunits * dialog->size_y;
	si.nPage = pixels_to_ydlgunits * MAX_DIALOG_HEIGHT;
	si.fMask = SIF_PAGE | SIF_RANGE;

	SetScrollInfo(dlgwnd, SB_VERT, &si, true);
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
	if (!di)
	{
		std::cerr << "Error: dialog_prime: Called with a null dialogbox pointer" << "\n";
		return;
	}

	if (di->buffer.empty())
	{
		std::cerr << "Error: dialog_prime: Called with an empty dialogbox buffer" << "\n";
		return;
	}

	if (di->size_y > MAX_DIALOG_HEIGHT)
	{
		di->size_x += DIM_SCROLLBAR_WIDTH;
		dialog_add_scrollbar(di);  // assumes this modifies the dialog correctly
	}

	// Cast beginning of vector to DLGTEMPLATE pointer
	DLGTEMPLATE *dlg_template = reinterpret_cast<DLGTEMPLATE*>(di->buffer.data());

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
	idx = windows::send_message(dialog_item, CB_GETCURSEL, 0, 0);
	if (idx == CB_ERR)
		return 0;
	return windows::send_message(dialog_item, CB_GETITEMDATA, idx, 0);
}



//============================================================
//  dialog_get_adjuster_value
//    called from win_dialog_add_adjuster
//============================================================

static LRESULT dialog_get_adjuster_value(dialog_box *dialog, HWND dialog_item, UINT message, WPARAM wparam, LPARAM lparam)
{
	auto window_text = std::unique_ptr<wchar_t>(windows::get_window_text(dialog_item));
	return std::stoi(window_text.get()); // consider using wcstol() for error checking
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

static int win_dialog_add_active_combobox(dialog_box *dialog, const wchar_t *item_label, int default_value,
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
	if (dialog_write_item(dialog, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST, x, y, dialog->layout->combo_width, DIM_COMBO_ROW_HEIGHT * 8, L"", DLGITEM_COMBOBOX, nullptr))
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

static int win_dialog_add_combobox(dialog_box *dialog, const wchar_t *item_label, int default_value, void (*storeval)(void *param, int val), void *storeval_param)
{
	return win_dialog_add_active_combobox(dialog, item_label, default_value, storeval, storeval_param, nullptr, nullptr);
}



//============================================================
//  win_dialog_add_combobox_item
//    called from customise_switches, customise_analogcontrols
//============================================================

static int win_dialog_add_combobox_item(dialog_box *dialog, const wchar_t *item_label, int item_data)
{
	// create our own copy of the string
	size_t newsize = wcslen(item_label) + 1;
	wchar_t  *item_label_cpy = new(std::nothrow) wchar_t[newsize];
	if (!item_label_cpy)
		return 1; // out of memory

	mui_wcscpy(item_label_cpy, item_label);

	if (dialog_add_trigger(dialog, dialog->item_count, TRIGGER_INITDIALOG, CB_ADDSTRING, nullptr, 0, (LPARAM)item_label_cpy, nullptr, nullptr))
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
	HWND dlgwnd, editwnd;
	int value, id;
	LONG_PTR l;

	l = windows::get_window_long_ptr(sbwnd, GWLP_USERDATA);
	stuff = (struct adjuster_sb_stuff *) l;

	if (msg == WM_VSCROLL)
	{
		id =  windows::get_window_long_ptr(sbwnd, GWL_ID);
		dlgwnd = GetParent(sbwnd);
		editwnd = dialog_boxes::get_dlg_item(dlgwnd, id - 1);
		auto win_text = std::unique_ptr<char[]>(windows::get_window_text_utf8(editwnd));
		value = std::stoi(win_text.get());

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
		std::string text = std::to_string(value);
		windows::set_window_text_utf8(editwnd, text.c_str());
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
	windows::set_window_long_ptr(sbwnd, GWLP_USERDATA, l);
	l = (LONG_PTR) adjuster_sb_wndproc;
	l = windows::set_window_long_ptr(sbwnd, GWLP_WNDPROC, l);
	stuff->oldwndproc = (WNDPROC) l;
	return 0;
}



//============================================================
//  win_dialog_add_adjuster
//    called from customise_analogcontrols
//============================================================

static int win_dialog_add_adjuster(dialog_box *dialog, const wchar_t *item_label, int default_value,
	int min_value, int max_value, bool is_percentage, dialog_itemstoreval storeval, void *storeval_param)
{
	short x;
	short y;

	dialog_new_control(dialog, &x, &y);

	if (dialog_write_item(dialog, WS_CHILD | WS_VISIBLE | SS_LEFT, x, y, dialog->layout->label_width, DIM_ADJUSTER_HEIGHT, item_label, DLGITEM_STATIC, nullptr))
		return 1;

	x += dialog->layout->label_width + DIM_HORIZONTAL_SPACING;
	y += DIM_BOX_VERTSKEW;

	if (dialog_write_item(dialog, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_NUMBER,
		x, y, dialog->layout->combo_width - DIM_ADJUSTER_SCR_WIDTH, DIM_ADJUSTER_HEIGHT, L"", DLGITEM_EDIT, nullptr))
		return 1;

	x += dialog->layout->combo_width - DIM_ADJUSTER_SCR_WIDTH;

	std::wstring value_str = std::to_wstring(default_value) + (is_percentage ? L"%" : L"");
	if (value_str.empty())
		return 1;
	wchar_t *value_wstr = new(std::nothrow) wchar_t[value_str.size() + 1];
	if (!value_wstr)
		return 1;

	if (!mui_wcscpy(value_wstr, value_str.c_str()))
	{
		delete[] value_wstr;
		return 1;
	}

	if (dialog_add_trigger(dialog, dialog->item_count, TRIGGER_INITDIALOG, WM_SETTEXT, nullptr, 0, (LPARAM)value_wstr, nullptr, nullptr))
		return 1;

	// add the trigger invoked when the apply button is pressed
	if (dialog_add_trigger(dialog, dialog->item_count, TRIGGER_APPLY, 0, dialog_get_adjuster_value, 0, 0, storeval, storeval_param))
		return 1;

	if (dialog_write_item(dialog, WS_CHILD | WS_VISIBLE | WS_TABSTOP | SBS_VERT, x, y, DIM_ADJUSTER_SCR_WIDTH, DIM_ADJUSTER_HEIGHT, L"", DLGITEM_SCROLLBAR, nullptr))
		return 1;

	x += DIM_ADJUSTER_SCR_WIDTH + DIM_HORIZONTAL_SPACING;

	if (dialog_add_trigger(dialog, dialog->item_count, TRIGGER_INITDIALOG, 0, adjuster_sb_setup, 0, MAKELONG(min_value, max_value), nullptr, nullptr))
		return 1;

	y += DIM_COMBO_ROW_HEIGHT + DIM_VERTICAL_SPACING * 2;

	dialog_finish_control(dialog, x, y);
	return 0;
}




//============================================================
//  get_seqselect_info
//============================================================

static seqselect_info *get_seqselect_info(HWND editwnd)
{
	LONG_PTR lp;
	lp = windows::get_window_long_ptr(editwnd, GWLP_USERDATA);
	return (seqselect_info*)lp;
}



//=============================================================================
//  seqselect_settext
//    called from seqselect_start_read_from_main_thread, seqselect_setup
//=============================================================================

static void seqselect_settext(HWND editwnd)
{
	seqselect_info *stuff;

	// the basics
	stuff = get_seqselect_info(editwnd);
	if (stuff == nullptr)
	{
		std::cerr << "Error: seqselect_settext: Called with a null seqselect_info pointer" << "\n";
		return; // this should not happen - need to fix this
	}
	// retrieve the seq name
	std::string new_seq = Machine->input().seq_name(*stuff->code);

	// change the text - avoid calls to SetWindowText() if we can
	std::unique_ptr<char[]> old_seq(windows::get_window_text_utf8(editwnd));
	if (!old_seq)
		old_seq.reset(new char[] { '\0' });

	if (new_seq != old_seq.get())
		windows::set_window_text_utf8(editwnd, new_seq.c_str());
	std::cout << "seqselect_settext: " << new_seq << "\n";
	// reset the selection
	if (input::get_focus() == editwnd)
	{
		DWORD start = 0, end = 0;
		windows::send_message(editwnd, EM_GETSEL, (WPARAM) (LPDWORD) &start, (LPARAM) (LPDWORD) &end);
		if (start != 0 || end != mui_strlen(old_seq.get()))
			windows::send_message(editwnd, EM_SETSEL, 0, -1);
	}
}



//============================================================
//  seqselect_start_read_from_main_thread
//    called from seqselect_wndproc
//============================================================

static void seqselect_start_read_from_main_thread(void *param)
{
	HWND editwnd = (HWND)param;
	seqselect_info *stuff = get_seqselect_info(editwnd);

	if (!stuff)
		return;

	if (stuff->poll_state != SEQSELECT_STATE_NOT_POLLING)
		return;

	// Change the state to polling
	stuff->poll_state = SEQSELECT_STATE_POLLING;

	// Create and start the poller
	stuff->seq_poll = std::make_unique<switch_sequence_poller>(Machine->input());
	if (stuff->code)
		stuff->seq_poll->start(*stuff->code);
	else
		stuff->seq_poll->start();

	// Set a timer for polling; WM_TIMER will handle the polling
	SetTimer(editwnd, SEQSELECT_POLL_TIMER_ID, 10, nullptr);
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
	seqselect_info *stuff;
	INT_PTR result = 0;
	bool call_baseclass = true;

	stuff = get_seqselect_info(editwnd);

	switch (msg)
	{
	case WM_TIMER:
		if (wparam == SEQSELECT_POLL_TIMER_ID && stuff->poll_state == SEQSELECT_STATE_POLLING)
		{
			winwindow_ui_pause(*Machine, false);
			auto code = stuff->seq_poll->poll();
			if (Machine->ui_input().pressed(IPT_UI_CANCEL))
			{
				KillTimer(editwnd, SEQSELECT_POLL_TIMER_ID);
				stuff->poll_state = SEQSELECT_STATE_NOT_POLLING;
				stuff->seq_poll.reset();
			}
			else if (code)
			{
				stuff->code = const_cast<input_seq*>(&stuff->seq_poll->sequence());
				seqselect_settext(editwnd);
				KillTimer(editwnd, SEQSELECT_POLL_TIMER_ID);
				stuff->poll_state = SEQSELECT_STATE_NOT_POLLING;
				stuff->seq_poll.reset();
			}
		}
		break;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_CHAR:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		result = 1;
		call_baseclass = false;
		break;

	case WM_SETFOCUS:
		windows::post_message(editwnd, SEQWM_SETFOCUS, 0, 0);
		break;

	case WM_KILLFOCUS:
		windows::post_message(editwnd, SEQWM_KILLFOCUS, 0, 0);
		break;

	case SEQWM_SETFOCUS:
		// if we receive the focus, we should start a polling loop
		seqselect_start_read_from_main_thread(static_cast<void*>(editwnd));
		break;

	case SEQWM_KILLFOCUS:
		// when we abort the focus, end any current polling loop
		seqselect_stop_read_from_main_thread(static_cast<void*>(editwnd));
			break;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			SetFocus(editwnd);
			windows::send_message(editwnd, EM_SETSEL, 0, -1);
			call_baseclass = false;
			result = 0;
			break;
	}

	if (call_baseclass)
		result = call_windowproc(stuff->oldwndproc, editwnd, msg, wparam, lparam);
	return result;
}



//============================================================
//  seqselect_setup
//    called from dialog_add_single_seqselect
//============================================================

static LRESULT seqselect_setup(dialog_box *dialog, HWND editwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	seqselect_info *stuff = (seqselect_info *) lparam;
	LONG_PTR lp;

	lp = windows::set_window_long_ptr(editwnd, GWLP_WNDPROC, (LONG_PTR) seqselect_wndproc);
	stuff->oldwndproc = (WNDPROC) lp;
	windows::set_window_long_ptr(editwnd, GWLP_USERDATA, lparam);
	seqselect_settext(editwnd);
	return 0;
}



//============================================================
//  seqselect_apply
//    called from dialog_add_single_seqselect
//============================================================

static LRESULT seqselect_apply(dialog_box *dialog, HWND editwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	seqselect_info *stuff;
	stuff = get_seqselect_info(editwnd);

	// store the settings
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
	if (dialog_write_item(di, WS_CHILD | WS_VISIBLE | SS_ENDELLIPSIS | ES_CENTER | SS_SUNKEN, x, y, cx, cy, L"", DLGITEM_EDIT, nullptr))
		return 1;

	// allocate a seqselect_info
	seqselect_info *stuff = new(std::nothrow) seqselect_info{};
	//seqselect_info *stuff;
	//stuff = global_alloc(seqselect_info);
	if (!stuff)
		return 1;

	// initialize the structure
	field->get_user_settings(stuff->settings);
	stuff->field = field;
	stuff->pos = di->item_count;
	stuff->is_analog = is_analog;

	// This next line is completely unsafe, but I do not know what to use *****************
	stuff->code = const_cast <input_seq*> (&field->seq( SEQ_TYPE_STANDARD ));

	if (dialog_add_trigger(di, di->item_count, TRIGGER_INITDIALOG, 0, seqselect_setup, di->item_count, (LPARAM) stuff, nullptr, nullptr))
		return 1;
	if (dialog_add_trigger(di, di->item_count, TRIGGER_APPLY, 0, seqselect_apply, 0, 0, nullptr, nullptr))
		return 1;
	return 0;
}


//============================================================
//  add_portselect_entry
//    called from win_dialog_add_seqselect
//============================================================

static int add_portselect_entry(dialog_box *di, short &x, short &y, std::wstring_view port_name, std::wstring_view port_suffix, ioport_field *field, bool is_analog, int seq_type)
{
	if (!di)
	{
		std::cerr << "Error: add_portselect_entry: Called with a null dialogbox pointer" << "\n";
		return 1;
	}

	if (port_name.empty())
	{
		std::cerr << "Error: add_portselect_entry: Called with an empty port_name" << "\n";
		return 1;
	}

	if (field == nullptr)
	{
		std::cerr << "Error: add_portselect_entry: Called with a null field pointer" << "\n";
		return 1;
	}

	// Combine base name with suffix, if any
	std::wstring full_name = std::wstring(port_name);
	if (!port_suffix.empty())
		full_name += port_suffix;

	dialog_new_control(di, &x, &y);

	if (dialog_write_item(di, WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX, x, y, di->layout->label_width, DIM_NORMAL_ROW_HEIGHT, full_name, DLGITEM_STATIC, nullptr))
	{
		return 1;
	}

	x += di->layout->label_width + DIM_HORIZONTAL_SPACING;

	if (dialog_add_single_seqselect(di, x, y, DIM_EDIT_WIDTH, DIM_NORMAL_ROW_HEIGHT, field, is_analog, seq_type))
	{
		return 1;
	}

	y += DIM_VERTICAL_SPACING + DIM_NORMAL_ROW_HEIGHT;
	x += DIM_EDIT_WIDTH + DIM_HORIZONTAL_SPACING;

	dialog_finish_control(di, x, y);

	return 0;
}

//============================================================
//  win_dialog_add_seqselect
//    called from customise_input, customise_miscinput
//============================================================

static int win_dialog_add_portselect(dialog_box *dialog, ioport_field *field)
{
	dialog_box *di = dialog;
	short x;
	short y;

	constexpr const wchar_t *port_suffix[3]{ L" Analog" ,L" Dec",L" Inc" };
	int seq_types[3]{ SEQ_TYPE_STANDARD ,SEQ_TYPE_DECREMENT ,SEQ_TYPE_INCREMENT };
	bool is_analog[3]{ true,false,false };

	std::wstring port_name = mui_utf16_from_utf8string(field->name());
	if (port_name.empty())
	{
		std::cout << "Error: win_dialog_add_portselect: Blank port_name encountered" << "\n";
		return 1; // error
	}
	assert(!port_name.empty());

	if (field->type() > IPT_ANALOG_FIRST && field->type() < IPT_ANALOG_LAST)
	{

		for (int seq = 0; seq < std::size(seq_types); seq++)
		{
			// create our local name for this entry; also convert from
			// MAME strings to wide strings
			if (add_portselect_entry(di, x, y, port_name, port_suffix[seq], field, is_analog[seq], seq_types[seq]))
				return 1;
		}
	}
	else
	{
		if (add_portselect_entry(di, x, y, port_name, L"", field, false, SEQ_TYPE_STANDARD))
			return 1;
	}

	return 0;
}



//============================================================
//  win_dialog_add_standard_buttons
//============================================================

static int win_dialog_add_standard_buttons(dialog_box *dialog)
{
	dialog_box *di = dialog;
	short x;
	short y;

	// Handle the case of an empty dialog box (size_x & size_y will be 0)
	if (!di->size_x)
		di->size_x = 3 * DIM_HORIZONTAL_SPACING + 2 * DIM_BUTTON_WIDTH;
	if (!di->size_y)
		di->size_y = DIM_VERTICAL_SPACING;

	// work out where cancel button goes
	x = di->size_x - DIM_HORIZONTAL_SPACING - DIM_BUTTON_WIDTH;
	y = di->size_y + DIM_VERTICAL_SPACING;

	// display cancel button
	if (dialog_write_item(di, WS_CHILD | WS_VISIBLE | SS_LEFT, x, y, DIM_BUTTON_WIDTH, DIM_BUTTON_ROW_HEIGHT, DLGTEXT_CANCEL, DLGITEM_BUTTON, nullptr))
		return 1;

	// work out where OK button goes
	x -= DIM_HORIZONTAL_SPACING + DIM_BUTTON_WIDTH;

	// display OK button
	if (dialog_write_item(di, WS_CHILD | WS_VISIBLE | SS_LEFT, x, y, DIM_BUTTON_WIDTH, DIM_BUTTON_ROW_HEIGHT, DLGTEXT_OK, DLGITEM_BUTTON, nullptr))
		return 1;

	di->size_y += DIM_BUTTON_ROW_HEIGHT + DIM_VERTICAL_SPACING * 2;

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
	{
		std::cerr << "Error: win_dialog_runmodal: Called with a null dialog box pointer" << "\n";
		return;
	}

	// finishing touches on the dialog
	dialog_prime(dialog);

	// show the dialog
	before_display_dialog(machine);
	DialogBoxIndirectParamW(nullptr, (const DLGTEMPLATE*)dialog->buffer.data(), wnd, (DLGPROC)dialog_proc, (LPARAM)dialog);
	after_display_dialog(machine);
}



//============================================================
//  win_file_dialog
//    called from change_device
//============================================================

static bool win_file_dialog(running_machine &machine, HWND parent, win_file_dialog_type dlgtype, const wchar_t *filter, const wchar_t *initial_dir, wchar_t *filename)
{
	bool result = false;

	// set up the OPENFILENAME data structure
	win_open_file_name ofn{};
	ofn.dialog_type = dlgtype;
	ofn.owner = parent;
	ofn.flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	ofn.filter = filter;
	ofn.initial_directory = initial_dir;

	if (dlgtype == WIN_FILE_DIALOG_OPEN)
		ofn.flags |= OFN_FILEMUSTEXIST;

	mui_wcscpy(ofn.filename, filename);

	before_display_dialog(machine);
	result = win_get_file_name_dialog(&ofn);
	after_display_dialog(machine);

	mui_wcscpy(filename, ofn.filename);

	return result;
}



//============================================================
//  customise_input
//============================================================

static void customise_input(running_machine &machine, HWND wnd, const wchar_t *title, int player, int inputclass)
{
	// create the dialog
	dialog_box *dlg = win_dialog_init(title, nullptr);
	if (!dlg)
	{
		std::cerr << "Error: customise_input: Failed to create dialog box" << "\n";
		return;
	}

	for (auto &port : machine.ioport().ports())
	{
		for (ioport_field &field : port.second->fields())
		{
			/* add if we match the group and we have a valid name */
			std::string this_name = field.name();
			int this_inputclass = field.type_class();
			int this_player = field.player();

			if (field.enabled()
				&& !this_name.empty()
				&& (field.type() == IPT_OTHER || machine.ioport().type_group(field.type(), this_player) != IPG_INVALID)
				&& this_inputclass != INPUT_CLASS_CONTROLLER
				&& this_inputclass != INPUT_CLASS_KEYBOARD)
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



//============================================================
//  customise_joystick
//============================================================

static void customise_joystick(running_machine &machine, HWND wnd, int joystick_num)
{
	customise_input(machine, wnd, L"Joysticks/Controllers", joystick_num, INPUT_CLASS_CONTROLLER);
}



//============================================================
//  customise_keyboard
//============================================================

static void customise_keyboard(running_machine &machine, HWND wnd)
{
	customise_input(machine, wnd, L"Emulated Keyboard", 0, INPUT_CLASS_KEYBOARD);
}



//============================================================
//  customise_miscinput
//============================================================

static void customise_miscinput(running_machine &machine, HWND wnd)
{


	/* create the dialog */
	dialog_box *dlg = win_dialog_init(dialog_title_miscinput, nullptr);
	if (!dlg)
		return;

	for (auto &port : machine.ioport().ports())
	{
		for (ioport_field &field : port.second->fields())
		{
			std::string this_name = field.name();
			int this_inputclass = field.type_class();

			/* add if we match the group and we have a valid name */
			if (field.enabled()
				&& !this_name.empty()
				&& (field.type() == IPT_OTHER || machine.ioport().type_group(field.type(), field.player()) != IPG_INVALID)
				&& this_inputclass != INPUT_CLASS_CONTROLLER
				&& this_inputclass != INPUT_CLASS_KEYBOARD)
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

static void customise_switches(running_machine &machine, HWND wnd, std::wstring_view title, UINT32 ipt_name)
{
	dialog_box *dlg;
	ioport_field *afield;
	ioport_field::user_settings settings;

	UINT32 type = 0;

	dlg = win_dialog_init(title, nullptr);
	if (!dlg)
		return;

	for (auto &port : machine.ioport().ports())
	{
		for (ioport_field &field : port.second->fields())
		{
			type = field.type();

			if (type == ipt_name)
			{
				const std::wstring switch_name = mui_utf16_from_utf8string(field.name());

				field.get_user_settings(settings);
				afield = &field;
				if (win_dialog_add_combobox(dlg, switch_name.c_str(), settings.value, update_keyval, (void*)afield))
					goto done;

				for (auto setting : field.settings())
				{
					const std::wstring setting_name = mui_utf16_from_utf8string(setting.name());
					if (win_dialog_add_combobox_item(dlg, setting_name.c_str(), setting.value()))
						goto done;
				}
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
//  customise_dipswitches
//============================================================

static void customise_dipswitches(running_machine &machine, HWND wnd)
{
	customise_switches(machine, wnd, dialog_title_dipswitch, IPT_DIPSWITCH);
}



//============================================================
//  customise_configuration
//============================================================

static void customise_configuration(running_machine &machine, HWND wnd)
{
	customise_switches(machine, wnd, dialog_title_drivercfg, IPT_CONFIG);
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
	static const struct dialog_layout layout = { 120, 52 };

	dlg = win_dialog_init(dialog_title_analog, &layout);
	if (!dlg)
		return;

	for (auto &port : machine.ioport().ports())
	{
		for (ioport_field &field : port.second->fields())
		{
			if (port_type_is_analog(field.type()))
			{
				field.get_user_settings(settings);
				const std::wstring name = mui_utf16_from_utf8string(field.name());
				afield = &field;


				std::wstring label = name + L" Digital Speed";
				if (win_dialog_add_adjuster(dlg, label.c_str(), settings.delta, 1, 255, false, store_delta, (void*)afield))
					goto done;

				label = name + L" Autocenter Speed";
				if (win_dialog_add_adjuster(dlg, label.c_str(), settings.centerdelta, 0, 255, false, store_centerdelta, (void*)afield))
					goto done;

				label = name + L" Reverse";
				if (win_dialog_add_combobox(dlg, label.c_str(), settings.reverse ? 1 : 0, store_reverse, (void*)afield))
					goto done;

				if (win_dialog_add_combobox_item(dlg, L"Off", 0))
					goto done;

				if (win_dialog_add_combobox_item(dlg, L"On", 1))
					goto done;

				label = name + L" Sensitivity";
				if (win_dialog_add_adjuster(dlg, label.c_str(), settings.sensitivity, 1, 255, true, store_sensitivity, (void*)afield))
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

static std::wstring win_dirname(std::wstring_view filename)
{
	// nullptr begets nullptr
	if (filename.empty())
		return std::wstring{};
	std::filesystem::path path(filename);

	if (path.has_parent_path())
	{
		path.make_preferred();  // replaces / with \ on Windows
		return path.parent_path().wstring();
	}

	path = std::filesystem::current_path() / "sta" / state_directoryname;

	if (!std::filesystem::exists(path))
		std::filesystem::create_directories(path);

	return path.wstring();
}


//============================================================
//  state_dialog
//    called when loading or saving a state
//============================================================

static void state_dialog(HWND wnd, win_file_dialog_type dlgtype, DWORD fileproc_flags, bool is_load, running_machine& machine)
{
	std::wstring dir;
	if (!state_filename.empty())
		dir = win_dirname(state_filename);

	win_open_file_name ofn{};
	ofn.dialog_type = dlgtype;
	ofn.owner = wnd;
	ofn.flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | fileproc_flags;
	ofn.filter = L"State Files (*.sta)|*.sta|All Files (*.*)|*.*";
	ofn.initial_directory = dir.c_str();

	std::filesystem::path state_filepath(state_filename);
	if (!state_filepath.has_extension())
		state_filepath.replace_extension(L".sta");

	mui_wcscpy(ofn.filename, state_filepath.c_str());

	bool result = win_get_file_name_dialog(&ofn);
	if (result)
	{
		state_filepath = ofn.filename;
		// the core doesn't add the extension if it's an absolute path

		if (is_load)
			machine.schedule_load(state_filepath.string());
		else
			machine.schedule_save(state_filepath.string());
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
	std::string utf8_state_filename = mui_utf8_from_utf16string(state_filename);
	machine.schedule_save(std::move(utf8_state_filename));
}



//============================================================
//  copy_extension_list
//============================================================

static void copy_extension_list(std::wstring &filter, const wchar_t *extensions)
{
	// our extension lists are comma delimited; Win32 expects to see lists
	// delimited by semicolons

	// Pointer-based iteration to avoid repeated calls to `find`
	const auto extensions_begin = extensions;
	const auto extensions_end = extensions + mui_wcslen(extensions);
	auto start = extensions_begin;
	auto end = start;

	// Reserve enough space in filter to avoid reallocations
	size_t extension_count = std::count(extensions_begin, extensions_end, L',') + (!extensions || *extensions == L'\0' ? 0 : 1);
	size_t required_size = extension_count * 5; // "*." + ext, e.g. "*.bin"
	filter.reserve(filter.size() + required_size);

	while (start != extensions_end)
	{
		// Find the next comma or end of string
		end = std::find(start, extensions_end, L',');

		// If it's not the first extension, add a semicolon separator
		if (start != extensions_begin)
			filter.push_back(L';');

		// Append "*." + the current extension (without commas)
		filter.append(L"*.");
		filter.append(start, end); // Copy the substring from start to end

		// Move to the next extension after the comma
		start = (end != extensions_end) ? end + 1 : end;
	}
}



//============================================================
//  get_softlist_info
//============================================================

static bool get_softlist_info(HWND wnd, device_image_interface *img)
{
	bool has_software = false;
	bool passes_tests = false;
	std::string sl_dir;
	const std::string opt_name = img->instance_name();

	// Get window info from user data
	LONG_PTR ptr = windows::get_window_long_ptr(wnd, GWLP_USERDATA);
	auto *window = reinterpret_cast<win_window_info*>(ptr);

	// Get media_path from machine options
	std::string rompath = window->machine().options().emu_options::media_path();

	// Get the path to suitable software
	for (software_list_device &swlist : software_list_device_enumerator(window->machine().root_device()))
	{
		for (const software_info &swinfo : swlist.get_info())
		{
			const software_part &part = swinfo.parts().front();

			if (swlist.is_compatible(part) == SOFTWARE_IS_COMPATIBLE)
			{
				for (device_image_interface &image : image_interface_enumerator(window->machine().root_device()))
				{
					if (!image.user_loadable())
						continue;

					if (!has_software && (opt_name == image.instance_name()))
					{
						const char *image_interface = image.image_interface();
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
				slmap[opt_name] = std::move(test_path);
				passes_tests = true;
				break;
			}
		}
	}

	return passes_tests;
}



//============================================================
//  add_filter_entry
//============================================================

static void add_filter_entry(std::wstring &filter, const wchar_t *description, const wchar_t *extensions)
{

	// add the description
	filter.append(description);
	filter.append(L" (");

	// add the extensions to the description
	copy_extension_list(filter, extensions);

	// add the trailing rparen and '|' character
	filter.append(L")|");

	// now add the extension list itself
	copy_extension_list(filter, extensions);

	// append a '|'
	filter.append(L"|");
}


//============================================================
//  build_generic_filter
//============================================================

static void build_generic_filter(device_image_interface *img, bool is_save, std::wstring &filter)
{
	if (!img || !img->file_extensions() || *(img->file_extensions()) == L'\0')
		return; // No extensions, nothing to do

	std::wstring file_extension = mui_utf16_from_utf8string(img->file_extensions());

	if (!is_save)
		file_extension.append(L",zip,7z");

	add_filter_entry(filter, L"Common image types", file_extension.c_str());

	filter.append(L"All files (*.*)|*.*|");

	if (!is_save)
		filter.append(L"Compressed Images (*.zip;*.7z)|*.zip;*.7z|");
}



//============================================================
//  change_device
//    open a dialog box to open or create a software file
//============================================================

static void change_device(HWND wnd, device_image_interface *image, bool is_save)
{
	// Get the path for loose software from <gamename>.ini
	std::wstring initial_dir;
	stringtokenizer tokenizer(image->device().machine().options().emu_options::sw_path(), ";");
	for (std::filesystem::path path : tokenizer)
	{
		// skip empty paths
		if (path.empty()) continue;

		// assign the first path that exists in the software path list to initial_dir
		if (std::filesystem::exists(path))
		{
			initial_dir = path.parent_path().wstring();
			break;
		}
	}
	// must be specified, must exist
	if (initial_dir.empty())
	{
		initial_dir = std::filesystem::absolute(".");
	}

	// file name
	std::wstring image_basename;
	if (image->exists())
		image_basename = mui_utf16_from_utf8string(image->basename());

	std::vector<wchar_t> filename(MAX_PATH, L'\0');
	if (!image_basename.empty())
		mui_wcscpy(filename.data(), image_basename.c_str());

	// build a normal filter
	std::wstring filter;
	build_generic_filter(image, is_save, filter);

	// display the dialog
	util::option_resolution *create_args = nullptr;
	bool result = win_file_dialog(image->device().machine(), wnd, is_save ? WIN_FILE_DIALOG_SAVE : WIN_FILE_DIALOG_OPEN, filter.c_str(), initial_dir.c_str(), filename.data());
	if (result)
	{
		std::string utf8_filename = mui_utf8_from_utf16string(filename.data());
		// mount the image
		if (is_save)
			image->create(utf8_filename, image->device_get_indexed_creatable_format(0), create_args);
		else
			image->load(utf8_filename);
	}
}


//============================================================
//  load_item
//    open a dialog box to choose a software-list-item to load
//============================================================
static void load_item(HWND wnd, device_image_interface *img, bool is_save)
{
	std::string opt_name = img->instance_name();
	std::wstring as;

	auto find_option = slmap.find(opt_name);
	if (find_option != slmap.end())
		as = mui_utf16_from_utf8string(find_option->second);

	// Make sure a folder was specified in the tab, and that it exists
	if (as.empty() || !std::filesystem::exists(as) || as.find(':') == std::string::npos)
		as = std::filesystem::absolute("."); // Default to emu directory

	// build a normal filter
	std::wstring filter;
	build_generic_filter(nullptr, is_save, filter);

	std::vector<wchar_t> filename(MAX_PATH, L'\0');
	if (!as.empty())
		mui_wcscpy(filename.data(), as.c_str());

	bool result = win_file_dialog(img->device().machine(), wnd, WIN_FILE_DIALOG_OPEN, filter.c_str(), as.c_str(), filename.data());

	if (result)
	{
		// Get the Item name out of the full path
		std::string buf = mui_utf8_from_utf16string(filename.data()); // convert to a c++ string so we can manipulate it
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
		if (t1 != std::string::npos)
			buf[t1] = ':';
		t1 = buf.find_last_of("\\"); // get rid of path; we only want the item name
		if (t1 != std::string::npos)
			buf.erase(0, t1 + 1);

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

static bool get_menu_item_string(HMENU menu, UINT item, bool by_position, HMENU *sub_menu, std::vector<wchar_t> &buffer)
{
	// because vector is a dynamic array, we need to ensure it has enough space
	if (buffer.size() < 2)
		buffer.resize(2);

	// clear out results
	std::fill(buffer.begin(), buffer.end(), L'\0');
	if (sub_menu)
		*sub_menu = nullptr;

	// prepare MENUITEMINFO structure
	MENUITEMINFOW mii{ sizeof(mii) };
	mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mii.dwTypeData = buffer.data();
	mii.cch = static_cast<UINT>(buffer.size() - 1); // -1 to leave space for null terminator

	// call GetMenuItemInfo()
	if (!menus::get_menu_item_info(menu, item, by_position, &mii))
		return false;

	// return results
	if (sub_menu)
		*sub_menu = mii.hSubMenu;

	// write a dash if this is a separator
	if (mii.fType == MFT_SEPARATOR)
		mui_wcscpy(buffer.data(), L"-");

	return true;
}



//============================================================
//  find_sub_menu
//============================================================

static HMENU find_sub_menu(HMENU menu, const std::wstring_view menu_path, bool create_sub_menu)
{
	if (!menu || menu_path.empty())
		return nullptr;

	std::vector<wchar_t> item_string(MAX_PATH, L'\0');
	HMENU sub_menu = nullptr;

	// Tokenize the null-delimited menu path
	auto tokenizer = wstringtokenizer::from_multisz(menu_path.data());
	for (const auto &menu_text : tokenizer)
	{
//      const wchar_t *token = menu_token.c_str(); // guaranteed null-terminated

		int i = -1;
		do
		{
			if (!get_menu_item_string(menu, ++i, true, &sub_menu, item_string))
				return nullptr;
		} while (mui_wcscmp(item_string.data(), menu_text) != 0);

		if (!sub_menu && create_sub_menu)
		{
			MENUITEMINFOW mii{ sizeof(mii) };
			mii.fMask = MIIM_SUBMENU;
			mii.hSubMenu = menus::create_menu();
			if (!menus::set_menu_item_info(menu, i, true, &mii))
				return nullptr;

			sub_menu = mii.hSubMenu;
		}

		menu = sub_menu;
		if (!menu)
			return nullptr;
	}

	return menu;
}



//============================================================
//  set_command_state
//============================================================

static void set_command_state(HMENU menu_bar, UINT command, UINT state)
{
	if (!menu_bar)
	{
		std::cerr << "error: set_command_state: Menu bar is null" << "\n";
		return;
	}

	MENUITEMINFO mii{ sizeof(mii) };
	mii.fMask = MIIM_STATE;
	mii.fState = state;
	bool result = menus::set_menu_item_info(menu_bar, command, false, &mii);
	if (!result)
	{
		int error_code = system_services::get_last_error();
		std::cerr << "error: " << error_code <<": set_command_state: SetMenuItemInfo failed for command " << command << "\n";
	}
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
	HMENU joystick_menu = find_sub_menu(menu_bar, menu_path_joysticks, true);
	if (!joystick_menu)
		return;

	// set up joystick menu
	int child_count = 0;
	int joystick_count = machine.ioport().count_players();
	if (joystick_count > 0)
	{
		for (int i = 0; i < joystick_count; i++)
		{
			std::string menuitem_text = std::string("Joystick ") + std::to_string(i + 1);
			win_append_menu_utf8(joystick_menu, MF_STRING, ID_JOYSTICK_0 + i, menuitem_text.c_str());
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
	const char *view_name;
	int cnt, frameskip, orientation, speed;
	LONG_PTR ptr = windows::get_window_long_ptr(wnd, GWLP_USERDATA);
	win_window_info *window = (win_window_info*)ptr;
	mame_ui_manager mame_ui(window->machine());
	HMENU menu_bar = menus::get_menu(wnd);
	HMENU slot_menu, sub_menu, device_menu, video_menu;
	UINT_PTR new_item, new_switem;
	UINT flags_for_exists, flags_for_writing, menu_flags;

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
	if (window->machine().save().supported())
	{
		set_command_state(menu_bar, ID_FILE_LOADSTATE_NEWUI, MFS_ENABLED);
		set_command_state(menu_bar, ID_FILE_SAVESTATE_AS, MFS_ENABLED);
		set_command_state(menu_bar, ID_FILE_SAVESTATE, !state_filename.empty() ? MFS_ENABLED : MFS_GRAYED);
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
	set_command_state(menu_bar, ID_FILE_UIACTIVE, has_keyboard ? (mame_ui.ui_active() ? MFS_CHECKED : MFS_ENABLED) : MFS_CHECKED | MFS_GRAYED);

	set_command_state(menu_bar, ID_KEYBOARD_EMULATED, has_keyboard ? (!window->machine().natkeyboard().in_use() ? MFS_CHECKED : MFS_ENABLED) : MFS_GRAYED);
	set_command_state(menu_bar, ID_KEYBOARD_NATURAL, (has_keyboard && window->machine().natkeyboard().can_post()) ? (window->machine().natkeyboard().in_use() ? MFS_CHECKED : MFS_ENABLED) : MFS_GRAYED);
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
	video_menu = find_sub_menu(menu_bar, menu_path_video, false);
	if (!video_menu)
		return;

	std::vector<wchar_t> buffer(MAX_PATH, L'\0');
	do
	{
		get_menu_item_string(video_menu, 0, true, nullptr, buffer);
		if (mui_wcscmp(buffer.data(), L"-"))
			menus::remove_menu(video_menu, 0, MF_BYPOSITION);
	} while (mui_wcscmp(buffer.data(), L"-"));

	for (size_t i = 0, view_index = window->target()->view(); (view_name = window->target()->view_name(i)); i++)
	{
		std::unique_ptr<const wchar_t[]> wcs_view_name(mui_utf16_from_utf8cstring(view_name));
		menus::insert_menu(video_menu, i, MF_BYPOSITION | (i == view_index ? MF_CHECKED : 0), ID_VIDEO_VIEW_0 + i, wcs_view_name.get());
	}

	// set up device menu; first remove all existing menu items
	device_menu = find_sub_menu(menu_bar, menu_path_media, false);
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
								std::string usage = "Usage: " + truncate_long_text_utf8(flist.value(), 200);
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
									menu_part_name.append(": ").append(truncate_long_text_utf8(swpart.feature("part_id"), 50));
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
				cassette_state state, t_state;
				cassette_image_device *device = dynamic_cast<cassette_image_device*>(&img.device());
				state = cassette_state(!device ? 0 : device->get_state());
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
						filename.append(": ").append(truncate_long_text_utf8(img.get_feature("part_id"), 50));
					filename.append(")");
				}
			}
		}
		else
			filename.assign("---");

		// Get instance names instead, like Media View, and mame's File Manager
		std::string instance = img.instance_name() + std::string(" (") + img.brief_instance_name() + std::string("): ") + truncate_long_text_utf8(filename, 127);
		std::transform(instance.begin(), instance.begin() + 1, instance.begin(), ::toupper); // turn first char to uppercase
		menus::append_menu_utf8(device_menu, MF_POPUP, (UINT_PTR)sub_menu, instance.c_str());

		cnt++;
	}

	// Print a warning if the media devices overrun the allocated range
	if ((ID_DEVICE_0 + (cnt * DEVOPTION_MAX)) >= ID_JOYSTICK_0)
		std::cout << "Maximum number of media items exceeded !!!" << "\n";

	// set up slot menu; first remove all existing menu items
	slot_menu = find_sub_menu(menu_bar, menu_path_slots, false);
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

		std::string opt_name = "0", current = "0";

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
		std::vector<device_slot_interface::slot_option*> option_list;
		for (auto &option : slot.option_list())
			if (option.second->selectable())
				option_list.push_back(option.second.get());

		// add the empty option
		slot_map[cnt] = slot_data{ slot.slot_name(), "" };
		menu_flags = MF_STRING;
		if (opt_name == "0")
			menu_flags |= MF_CHECKED;
		menus::append_menu_utf8(sub_menu, menu_flags, cnt++, "[Empty]");

		// sort them by name
		std::sort(option_list.begin(), option_list.end(), [](device_slot_interface::slot_option *opt1, device_slot_interface::slot_option *opt2) {return mui_strcmp(opt1->name(), opt2->name()) < 0; });

		// add each option in sorted order
		for (device_slot_interface::slot_option *opt : option_list)
		{
			std::string temp = opt->name() + std::string(" (") + opt->devtype().fullname() + std::string(")");
			slot_map[cnt] = slot_data{ slot.slot_name(), opt->name() };
			menu_flags = MF_STRING;
			if (opt->name() == opt_name)
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

	for (auto const &window : osd_common_t::window_list())
	{
		RECT before_rect = { 100, 100, 200, 200 };
		RECT after_rect = { 100, 100, 200, 200 };

		hwnd = dynamic_cast<win_window_info &>(*window).platform_window();

		// get current menu
		menu = menus::get_menu(hwnd);

		// get before rect
		style =  windows::get_window_long_ptr(hwnd, GWL_STYLE);
		exstyle =  windows::get_window_long_ptr(hwnd, GWL_EXSTYLE);
		windows::adjust_window_rect_ex(&before_rect, style, menu ? true : false, exstyle);

		// toggle the menu
		if (menu)
		{
			SetProp(hwnd, TEXT("menu"), (HANDLE) menu);
			menu = nullptr;
		}
		else
			menu = (HMENU) GetProp(hwnd, TEXT("menu"));

		SetMenu(hwnd, menu);

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

		RedrawWindow(hwnd, nullptr, nullptr, 0);
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
				cassette_image_device *cassette = dynamic_cast<cassette_image_device*>(&img->device());
				if (!cassette)
					break;

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

static void help_display(HWND wnd, const wchar_t *chapter)
{
	using htmlhelpproc = HWND(WINAPI*)(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData);
	static htmlhelpproc htmlhelp;
	static DWORD htmlhelp_cookie;

	if (!htmlhelp)
	{
		HMODULE hhctrl_ocx = system_services::load_library(L"hhctrl.ocx");
		if (!hhctrl_ocx)
		{
			std::cerr << "error: help_display: unable to load hhctrl.ocx" << "\n";
			return;
		}

		htmlhelp = (htmlhelpproc)system_services::get_proc_address(hhctrl_ocx, L"HtmlHelpW");
		if (htmlhelp)
		{
			htmlhelp(nullptr, nullptr, 28 /*HH_INITIALIZE*/, (DWORD_PTR)&htmlhelp_cookie);

			// if full screen, turn it off
			if (!is_windowed())
				winwindow_toggle_full_screen();

			shell::shell_execute(wnd, L"open", chapter, L"", nullptr, SW_SHOWNORMAL);
		}
		else
			std::cerr << "error: help_display: HtmlHelpW() not found" << "\n";

		system_services::free_library(hhctrl_ocx);
	}
}



//============================================================
//  help_about_mess
//============================================================

static void help_about_mess(HWND wnd)
{
	help_display(wnd, L"https://mamedev.org/");
}



//============================================================
//  help_about_thissystem
//============================================================

static void help_about_thissystem(running_machine &machine, HWND wnd)
{
	std::wostringstream help_url_buffer;

	const std::wstring system_name = mui_utf16_from_utf8string(machine.system().name);
	help_url_buffer << L"http://adb.arcadeitalia.net/dettaglio_mame.php?game_name=" << system_name << std::flush;
	const std::wstring help_url = help_url_buffer.str();
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
			mame_machine_manager::instance()->ui().set_ui_active(!mame_machine_manager::instance()->ui().ui_active());
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
			customise_miscinput(window->machine(), wnd);
			break;

		case ID_OPTIONS_ANALOGCONTROLS:
			customise_analogcontrols(window->machine(), wnd);
			break;

		case ID_FILE_OLDUI:
			mame_machine_manager::instance()->ui().show_menu();
			break;

		case ID_FILE_FULLSCREEN:
			winwindow_toggle_full_screen();
			break;

		case ID_OPTIONS_TOGGLEFPS:
			mame_machine_manager::instance()->ui().set_show_fps(!mame_machine_manager::instance()->ui().show_fps());
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
	//          std::cout << "Loading software part: " << instance << ", index: " << std::hex << mapindex << std::dec << "\n";
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
//  state_generate_filename
//============================================================

static bool state_generate_filename(const std::wstring_view full_name)
{
	if (full_name.empty())
		return false;

	state_filename.clear();
	state_filename.reserve(full_name.size());

	for (wchar_t ch : full_name)
	{
		if (std::iswalnum(ch) || mui_wcschr(L"()-_,. \0", ch))
			state_filename.push_back(ch);
	}

	if (state_filename.empty())
		return false;

	state_filename.append(L" State"); // append " State" to the end of the string

	return true;
}

//============================================================
//  set_menu_text
//============================================================

static void set_menu_text(HMENU menu_bar, int command, const wchar_t *menu_text)
{
	if (!menu_bar || command < 0)
		return;

	// invoke SetMenuItemInfo()
	MENUITEMINFOW mii = { sizeof(mii) };
	mii.fMask = MIIM_TYPE;
	mii.dwTypeData = const_cast<wchar_t*>(menu_text);
	menus::set_menu_item_info(menu_bar, command, false, &mii);
}


//============================================================
//  win_setup_menus
//============================================================

static int win_setup_menus(running_machine &machine, HMENU menu_bar)
{
	HMENU frameskip_menu;
	std::wostringstream str_buffer;
	int i = 0;

	// initialize critical values
	joystick_menu_setup = false;

	// set up frameskip menu
	frameskip_menu = find_sub_menu(menu_bar, menu_path_frameskip, false);

	if (!frameskip_menu)
		return 1;

	for (i = 0; i < frameskip_level_count(machine); i++)
	{
		str_buffer << i;
		std::wstring label = str_buffer.str();
		str_buffer.str(L"");
		str_buffer.clear();
		menus::append_menu(frameskip_menu, MF_STRING, ID_FRAMESKIP_0 + i, label.c_str());
	}

	// set the help menu to refer to this machine
	std::wstring full_name = mui_utf16_from_utf8string(machine.system().type.fullname());
	const std::wstring short_name = mui_utf16_from_utf8string(machine.system().name);

	str_buffer << L"About " << full_name << L" (" << short_name << L")...";
	std::wstring menu_text = str_buffer.str();
	str_buffer.str(L"");
	str_buffer.clear();

	set_menu_text(menu_bar, ID_HELP_ABOUTSYSTEM, menu_text.c_str());

	// initialize state_filename for each driver, so we don't carry names in-between them

	state_directoryname = std::move(short_name);

	if (!state_generate_filename(full_name))
	{
		std::cout << "Unable to create state filename for " << machine.system().name << "\n";
		return 1;
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
		std::cout << "No memory for the menu, running without it." << "\n";
		return 0;
	}

	if (win_setup_menus(machine, menu_bar))
	{
		std::cout << "Unable to setup the menu, running without it." << "\n";
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

