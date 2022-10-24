// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

//============================================================
//
//  datamap.cpp - Win32 dialog and options bridge code
//
//============================================================

// standard C++ headers
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view> //emu_opts.h

// standard windows headers
#include "winapi_common.h"
#include <uxtheme.h>

// MAME headers

// shlwapi.h pulls in objbase.h which defines interface as a struct which conflict with a template declaration in device.h
#ifdef interface 
#undef interface // undef interface which is for COM interfaces
#endif
#include "emu.h"
#define interface struct

#include "ui/moptions.h" // emu_opts.h
#include "winopts.h"	 // emu_opts.h

// MAMEUI headers
#include "mui_cstr.h"
#include "mui_wcstr.h"

#include "windows_controls.h"
#include "dialog_boxes.h"
#include "windows_input.h"
#include "windows_messages.h"

#include "emu_opts.h"
#include "mui_opts.h"

#include "datamap.h"

using namespace mameui::util::string_util;
using namespace mameui::winapi::controls;
using namespace mameui::winapi;

//============================================================
//  TYPE DEFINITIONS
//============================================================

enum class control_type :uint32_t
{
	UNKNOWN,
	BUTTON,
	STATIC,
	EDIT,
	COMBOBOX,
	TRACKBAR,
	LISTVIEW
};

struct datamap_entry
{
	// the basics about the entry
	int dlgitem;
	datamap_entry_type type;
	const char* option_name;

	// callbacks
	datamap_callback callbacks[datamap_callback_type::DCT_COUNT];
	get_option_name_callback get_option_name;

	// formats
	const char* int_format;
	const char* float_format;

	// trackbar options
	bool use_trackbar_options;
	float trackbar_min;
	float trackbar_max;
	float trackbar_increments;
};

struct datamap
{
	int entry_count;
	datamap_entry entries[256]; // 256 options entries seems enough for now...
};

using datamap_default_callback = void (*)(datamap* map, HWND hwnd, windows_options* o, datamap_entry* entry, const char* option_name);

//============================================================
//  PROTOTYPES
//============================================================

static datamap_entry *find_entry(datamap *map, int dlgitem);
static control_type get_control_type(HWND hwnd);
static int control_operation(datamap *map, HWND dialog, windows_options *o, datamap_entry *entry, datamap_callback_type callback_type);
static void read_control(datamap *map, HWND hwnd, windows_options *o, datamap_entry *entry, const char *option_name);
static void populate_control(datamap *map, HWND hwnd, windows_options *o, datamap_entry *entry, const char *option_name);
//static char *tztrim(float float_value);


//============================================================
//  datamap_create
//============================================================

datamap *datamap_create(void)
{
	datamap *map = new datamap();
	if (!map)
		return nullptr;

	map->entry_count = 0;
	return map;
}



//============================================================
//  datamap_free
//============================================================

void datamap_free(datamap *map)
{
	delete map;
}



//============================================================
//  datamap_add
//============================================================

void datamap_add(datamap *map, int dlgitem, datamap_entry_type type, const char *option_name)
{
	// sanity check for too many entries
	if (!(map->entry_count < std::size(map->entries)))
	{
		std::cout << "Datamap.cpp Line __LINE__ too many entries" << "\n";
		return;
	}

	// add entry to the datamap
	map->entries[map->entry_count] = datamap_entry{};
	map->entries[map->entry_count].dlgitem = dlgitem;
	map->entries[map->entry_count].type = type;
	map->entries[map->entry_count].option_name = option_name;
	map->entry_count++;
}



//============================================================
//  datamap_set_callback
//============================================================

void datamap_set_callback(datamap *map, int dlgitem, datamap_callback_type callback_type, datamap_callback callback)
{
	datamap_entry *entry;

	assert(callback_type >= 0);
	assert(callback_type < DCT_COUNT);

	entry = find_entry(map, dlgitem);
	entry->callbacks[callback_type] = callback;
}



//============================================================
//  datamap_set_option_name_callback
//============================================================

void datamap_set_option_name_callback(datamap *map, int dlgitem, get_option_name_callback get_option_name)
{
	datamap_entry *entry;
	entry = find_entry(map, dlgitem);
	entry->get_option_name = get_option_name;
}



//============================================================
//  datamap_set_trackbar_range
//============================================================

void datamap_set_trackbar_range(datamap *map, int dlgitem, float min, float max, float increments)
{
	datamap_entry *entry = find_entry(map, dlgitem);
	entry->use_trackbar_options = true;
	entry->trackbar_min = min;
	entry->trackbar_max = max;
	entry->trackbar_increments = increments;
}



//============================================================
//  datamap_set_int_format
//============================================================

void datamap_set_int_format(datamap *map, int dlgitem, const char *format)
{
	datamap_entry *entry = find_entry(map, dlgitem);
	entry->int_format = format;
}



//============================================================
//  datamap_set_float_format
//============================================================

void datamap_set_float_format(datamap *map, int dlgitem, const char *format)
{
	datamap_entry *entry = find_entry(map, dlgitem);
	entry->float_format = format;
}



//============================================================
//  datamap_read_control
//============================================================

bool datamap_read_control(datamap *map, HWND dialog, windows_options &o, int dlgitem)
{
	datamap_entry *entry = find_entry(map, dlgitem);
	return control_operation(map, dialog, &o, entry, DCT_READ_CONTROL);
}



//============================================================
//  datamap_read_all_controls
//============================================================

void datamap_read_all_controls(datamap *map, HWND dialog, windows_options &o)
{
	for (size_t i = 0; i < map->entry_count; i++)
		control_operation(map, dialog, &o, &map->entries[i], DCT_READ_CONTROL);
}



//============================================================
//  datamap_populate_control
//============================================================

void datamap_populate_control(datamap *map, HWND dialog, windows_options &o, int dlgitem)
{
	datamap_entry *entry = find_entry(map, dlgitem);
	control_operation(map, dialog, &o, entry, DCT_POPULATE_CONTROL);
}



//============================================================
//  datamap_populate_all_controls
//============================================================

void datamap_populate_all_controls(datamap *map, HWND dialog, windows_options &o)
{
	for (size_t i = 0; i < map->entry_count; i++)
		control_operation(map, dialog, &o, &map->entries[i], DCT_POPULATE_CONTROL);
}



//============================================================
//  datamap_update_control
//============================================================

void datamap_update_control(datamap *map, HWND dialog, windows_options &o, int dlgitem)
{
	datamap_entry *entry = find_entry(map, dlgitem);
	control_operation(map, dialog, &o, entry, DCT_UPDATE_STATUS);
}



//============================================================
//  datamap_update_all_controls
//============================================================

void datamap_update_all_controls(datamap *map, HWND dialog, windows_options *o)
{
	for (size_t i = 0; i < map->entry_count; i++)
		control_operation(map, dialog, o, &map->entries[i], DCT_UPDATE_STATUS);
}



//============================================================
//  find_entry
//============================================================

static datamap_entry *find_entry(datamap *map, int dlgitem)
{
	for (size_t i = 0; i < map->entry_count; i++)
		if (map->entries[i].dlgitem == dlgitem)
			return &map->entries[i];

	// should not reach here
	std::cout << "Datamap.cpp line __LINE__ couldn't find an entry" << "\n";
	return nullptr;
}



//============================================================
//  get_control_type
//============================================================

static control_type get_control_type(HWND hwnd)
{
	control_type type;
	wchar_t class_name[256];

	windows::get_classname(hwnd, class_name, std::size(class_name));
	if (!mui_wcscmp(class_name, WC_BUTTONW))
		type = control_type::BUTTON;
	else if (!mui_wcscmp(class_name, WC_STATICW))
		type = control_type::STATIC;
	else if (!mui_wcscmp(class_name, WC_EDITW))
		type = control_type::EDIT;
	else if (!mui_wcscmp(class_name, WC_COMBOBOXW))
		type = control_type::COMBOBOX;
	else if (!mui_wcscmp(class_name, TRACKBAR_CLASSW))
		type = control_type::TRACKBAR;
	else if (!mui_wcscmp(class_name, WC_LISTVIEWW))
		type = control_type::LISTVIEW;
	else
		type = control_type::UNKNOWN;

	return type;
}



//============================================================
//  is_control_displayonly
//============================================================

static bool is_control_displayonly(HWND hwnd)
{
	bool displayonly = false;
	switch (get_control_type(hwnd))
	{
	case control_type::STATIC:
	{
		displayonly = true;
		break;
	}
	case control_type::EDIT:
	{
		LONG_PTR edit_control_style = windows::get_window_long_ptr(hwnd, GWL_STYLE);
		displayonly = (edit_control_style & ES_READONLY) ? true : false;
		break;
	}

	default:
		displayonly = false;
		break;
	}

	if (!input::is_window_enabled(hwnd))
		displayonly = true;
	return displayonly;
}



//============================================================
//  broadcast_changes
//============================================================

static void broadcast_changes(datamap *map, HWND dialog, windows_options *o, datamap_entry *entry, const char *option_name)
{
	HWND other_control;
	const char *that_option_name;

	for (size_t i = 0; i < map->entry_count; i++)
	{
		// search for an entry with the same option_name, but is not the exact
		// same entry
		that_option_name = map->entries[i].option_name;
		if (map->entries[i].option_name && (&map->entries[i] != entry) && !mui_strcmp(that_option_name, option_name))
		{
			// we've found a control sharing the same option; populate it
			other_control = dialog_boxes::get_dlg_item(dialog, map->entries[i].dlgitem);
			if (other_control)
				populate_control(map, other_control, o, &map->entries[i], that_option_name);
		}
	}
}



//============================================================
//  control_operation
//============================================================

static int control_operation(datamap *map, HWND dialog, windows_options *o, datamap_entry *entry, datamap_callback_type callback_type)
{
	static const datamap_default_callback default_callbacks[DCT_COUNT] =
	{
		read_control,
		populate_control,
		nullptr
	};
	int result = 0;
	const char *option_name;
	char option_name_buffer[64] = {};
	char option_value[1024] = {};
	HWND hwnd = 0;

	hwnd = dialog_boxes::get_dlg_item(dialog, entry->dlgitem);
	if (hwnd)
	{
		// don't do anything if we're reading from a display-only control
		if ((callback_type != DCT_READ_CONTROL) || !is_control_displayonly(hwnd))
		{
			// figure out the option_name
			if (entry->get_option_name)
			{
				entry->get_option_name(map, dialog, hwnd, option_name_buffer, std::size(option_name_buffer));
				option_name = option_name_buffer;
			}
			else
				option_name = entry->option_name;

			// if reading, get the option value, solely for the purposes of comparison
			if ((callback_type == DCT_READ_CONTROL) && option_name)
				snprintf(option_value, std::size(option_value), "%s", o->value(option_name));

			if (entry->callbacks[callback_type])
			{
				// use custom callback
				result = entry->callbacks[callback_type](map, dialog, hwnd, o, option_name);
			}
			else
			if (default_callbacks[callback_type] && option_name)
			{
				// use default callback
				default_callbacks[callback_type](map, hwnd, o, entry, option_name);
			}

			// the result is dependent on the type of control
			switch(callback_type)
			{
				case DCT_READ_CONTROL:
					// For callbacks that returned true, do not broadcast_changes.
					if (!result)
					{
						// do a check to see if the control changed
						result = option_name && !mui_strcmp(option_value, o->value(option_name));
						if (result)
						{
							// the value has changed; we may need to broadcast the change
							broadcast_changes(map, dialog, o, entry, option_name);
						}
					}
					break;

				default:
					// do nothing
					break;
			}
		}
	}

	return result;
}



//============================================================
//  trackbar_value_from_position
//============================================================

static float trackbar_value_from_position(datamap_entry *entry, int position)
{
	float position_f = position;

	if (entry->use_trackbar_options)
		position_f = (position_f * entry->trackbar_increments) + entry->trackbar_min;

	return position_f;
}



//============================================================
//  trackbar_position_from_value
//============================================================

static int trackbar_position_from_value(datamap_entry *entry, float value)
{
	if (entry->use_trackbar_options)
		value = floor((value - entry->trackbar_min) / entry->trackbar_increments + 0.5);

	return (int) value;
}



//============================================================
//  read_control
//============================================================

static void read_control(datamap *map, HWND hwnd, windows_options *o, datamap_entry *entry, const char *option_name)
{
	bool bool_value = 0;
	int int_value = 0;
	float float_value = 0;
	const char *string_value;
	int selected_index = 0;
	int trackbar_pos = 0;
	// use default read value behavior
	switch(get_control_type(hwnd))
	{
		case control_type::BUTTON:
			//assert(entry->type == DM_BOOL);
			bool_value = button_control::get_check(hwnd);
			emu_opts.emu_set_value(o, option_name, bool_value);
			break;

		case control_type::COMBOBOX:
			selected_index = combo_box::get_cur_sel(hwnd);
			if (selected_index >= 0)
			{
				switch(entry->type)
				{
				case datamap_entry_type::DM_INT:
				{
					int_value = (int)combo_box::get_item_data(hwnd, selected_index);
					emu_opts.emu_set_value(o, option_name, int_value);
					break;
				}
				case datamap_entry_type::DM_STRING:
					{
						string_value = (const char *) combo_box::get_item_data(hwnd, selected_index);
						std::string svalue = string_value ? std::string(string_value) : "";
						emu_opts.emu_set_value(o, option_name, svalue);
						break;
					}

					default:
						break;
				}
			}
			break;

		case control_type::TRACKBAR:
			trackbar_pos = track_bar::get_pos(hwnd);
			float_value = trackbar_value_from_position(entry, trackbar_pos);
			switch(entry->type)
			{
			case datamap_entry_type::DM_INT:
			{
				int_value = (int)float_value;
				emu_opts.emu_set_value(o, option_name, int_value);
				break;
			}
			case datamap_entry_type::DM_FLOAT:
			{
				emu_opts.emu_set_value(o, option_name, float_value);
				break;
			}
				default:
					break;
			}
			break;

		case control_type::EDIT:
			// NYI
			break;

		case control_type::STATIC:
		case control_type::LISTVIEW:
		case control_type::UNKNOWN:
			// non applicable
			break;
	}
}



//============================================================
//  populate_control
//============================================================

static void populate_control(datamap *map, HWND hwnd, windows_options *o, datamap_entry *entry, const char *option_name)
{
	bool bool_value = 0;
	int int_value = 0;
	float float_value = 0;
	int selected_index = 0;
	int trackbar_range = 0;
	int trackbar_pos = 0;
	double trackbar_range_d = 0;
	std::string c = emu_opts.emu_get_value(o, option_name);

	// use default populate control value
	switch(get_control_type(hwnd))
	{
	case control_type::BUTTON:
	{
		bool_value = (c == "0") ? 0 : 1;
		button_control::set_check(hwnd, bool_value);
		break;
	}
		case control_type::EDIT:
		case control_type::STATIC:
		{
			switch (entry->type)
			{
			case datamap_entry_type::DM_STRING:
				break;

			case datamap_entry_type::DM_INT:
				if (entry->int_format)
				{
					int_value = std::stoi(c);
					c = util::string_format(entry->int_format, int_value);
				}
				break;

			case datamap_entry_type::DM_FLOAT:
				if (entry->float_format)
				{
					float_value = std::stof(c);
					c = util::string_format(entry->float_format, float_value);
				}
				break;

			default:
				c.clear();
				break;
			}
			if (c.empty())
				c = "";
			windows::set_window_text_utf8(hwnd, c.c_str());
			break;
		}
		case control_type::COMBOBOX:
		{
			selected_index = 0;
			switch (entry->type)
			{
			case datamap_entry_type::DM_INT:
			{
				int_value = std::stoi(c);
				for (size_t i = 0; i < combo_box::get_count(hwnd); i++)
				{
					if (int_value == (int)combo_box::get_item_data(hwnd, i))
					{
						selected_index = i;
						break;
					}
				}
				break;
			}
			case datamap_entry_type::DM_STRING:
			{
				for (size_t i = 0; i < combo_box::get_count(hwnd); i++)
				{
					std::string name = (const char*)combo_box::get_item_data(hwnd, i);
					if (c == name)
					{
						selected_index = i;
						break;
					}
				}
				break;
			}
			default:
				break;
			}
			combo_box::set_cur_sel(hwnd, selected_index);
			break;
		}
		case control_type::TRACKBAR:
		{
			// do we need to set the trackbar options?
/*          if (!entry->use_trackbar_options)
			{
				switch(options_get_range_type(o, option_name))
				{
					case OPTION_RANGE_NONE:
						// do nothing
						break;

					case OPTION_RANGE_INT:
						options_get_range_int(o, option_name, &minval_int, &maxval_int);
						entry->use_trackbar_options = true;
						entry->trackbar_min = minval_int;
						entry->trackbar_max = maxval_int;
						entry->trackbar_increments = 1;
						break;

					case OPTION_RANGE_FLOAT:
						options_get_range_float(o, option_name, &minval_float, &maxval_float);
						entry->use_trackbar_options = true;
						entry->trackbar_min = minval_float;
						entry->trackbar_max = maxval_float;
						entry->trackbar_increments = (float)0.05;
						break;
				}
			}
		*/

		// do we specify default options for this control?  if so, we need to specify
		// the range
			if (entry->use_trackbar_options)
			{
				trackbar_range_d = floor(((entry->trackbar_max - entry->trackbar_min) / entry->trackbar_increments) + 0.5);
				trackbar_range = (int)trackbar_range_d;
				track_bar::set_range_min(hwnd, (BOOL)0, 0);
				track_bar::set_range_max(hwnd, (BOOL)0, trackbar_range);
			}

			switch (entry->type)
			{
			case datamap_entry_type::DM_INT:
			{
				int_value = std::stoi(c);
				trackbar_pos = trackbar_position_from_value(entry, int_value);
				break;
			}
			case datamap_entry_type::DM_FLOAT:
			{
				float_value = std::stof(c);
				trackbar_pos = trackbar_position_from_value(entry, float_value);
				break;
			}
			default:
			{
				trackbar_pos = 0;
				break;
			}
			}

			track_bar::set_pos(hwnd, (BOOL)1, trackbar_pos);
			break;
		}
		case control_type::LISTVIEW:
		case control_type::UNKNOWN:
			// non applicable
			break;
	}
}

#if 0
// Return a string from a float value with trailing zeros removed.
std::string tztrim(float float_value)
{
	std::string float_string = std::to_string(float_value);w

	float_string.resize(float_string.find_last_not_of('0'));

	return float_string;
}
#endif
