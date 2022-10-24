// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

//============================================================
//
//  datamap.c - Win32 dialog and options bridge code
//
//============================================================

#ifndef MAMEUI_WINAPP_DATAMAP_H
#define MAMEUI_WINAPP_DATAMAP_H

#pragma once

//============================================================
//  TYPE DEFINITIONS
//============================================================

using DatamapEntryType = enum datamap_entry_type :uint32_t
{
	DM_NONE = 0,
	DM_BOOL,
	DM_INT,
	DM_FLOAT,
	DM_STRING
};

using DatamapCallbackType = enum datamap_callback_type :uint32_t
{
	DCT_READ_CONTROL,
	DCT_POPULATE_CONTROL,
	DCT_UPDATE_STATUS,
	DCT_COUNT
};

using Datamap = struct datamap;

// MSH - Callback can now return TRUE, signifying that changes have been made, but should NOT be broadcast.'
using datamap_callback = bool (*)(datamap *map, HWND dialog, HWND hwnd, windows_options *opts, std::string option_name);
using get_option_name_callback = std::string (*)(datamap *map, HWND dialog, HWND hwnd);

//============================================================
//  PROTOTYPES
//============================================================

// datamap creation and disposal
datamap *datamap_create(void);
void datamap_free(datamap *map);

// datamap setup
void datamap_add(datamap *map, int dlgitem, datamap_entry_type type, std::string_view option_name);
void datamap_set_callback(datamap *map, int dlgitem, datamap_callback_type callback_type, datamap_callback callback);
void datamap_set_option_name_callback(datamap *map, int dlgitem, get_option_name_callback get_option_name);
void datamap_set_trackbar_range(datamap *map, int dlgitem, float min, float max, float increments);
void datamap_set_int_format(datamap *map, int dlgitem, std::string_view format);
void datamap_set_float_format(datamap *map, int dlgitem, std::string_view format);

// datamap operations
bool datamap_read_control(datamap *map, HWND dialog, windows_options &opts, int dlgitem);
void datamap_read_all_controls(datamap *map, HWND dialog, windows_options &opts);
void datamap_populate_control(datamap *map, HWND dialog, windows_options &opts, int dlgitem);
void datamap_populate_all_controls(datamap *map, HWND dialog, windows_options &opts);
void datamap_update_control(datamap *map, HWND dialog, windows_options &opts, int dlgitem);
void datamap_update_all_controls(datamap *map, HWND dialog, windows_options *opts);

#endif // MAMEUI_WINAPP_DATAMAP_H

