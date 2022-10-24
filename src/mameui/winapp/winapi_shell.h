// license:BSD-3-Clause
// copyright-holders:0perator

#ifndef MAMEUI_WINAPP_WINAPI_SHELL_H
#define MAMEUI_WINAPP_WINAPI_SHELL_H

#pragma once

// standard windows headers
#include <windows.h>
#include <windef.h>
#include <shellapi.h>
#include <objidl.h>
#include <shlobj.h>
#include <shlwapi.h>

#undef interface

namespace mameui::winapi
{
	namespace shell
	{
		HICON extract_icon(HINSTANCE instance_handle, const wchar_t *exe_filename, UINT icon_index);
		HICON extract_icon_utf8(HINSTANCE instance_handle, const char *exe_filename, UINT icon_index);

		wchar_t *path_find_filename(const wchar_t *path);

		LPITEMIDLIST shell_browse_for_folder(LPBROWSEINFOW browse_info);

		HINSTANCE shell_execute(HWND window_handle, const wchar_t *operation, const wchar_t *file, const wchar_t *parameters, const wchar_t *directory, int show_command);

		BOOL shell_get_path_from_id_list(PCIDLIST_ABSOLUTE id_list, wchar_t *path);
	}
}

#endif // MAMEUI_WINAPP_WINAPI_SHELL_H
