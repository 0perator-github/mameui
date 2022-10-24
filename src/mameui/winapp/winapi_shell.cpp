// license:BSD-3-Clause
// copyright-holders:0perator

/***************************************************************************

  winapi_shell.cpp

 ***************************************************************************/

// standard windows headers

// standard C++ headers
#include <memory>

// MAME/MAMEUI headers
#include "mui_wcsconv.h"
#include "winapi_shell.h"


//=============================================================
// extract_icon
//=============================================================

HICON mameui::winapi::shell::extract_icon(HINSTANCE instance_handle, const wchar_t *exe_filename, UINT icon_index)
{
	return ExtractIconW(instance_handle, exe_filename, icon_index);
}


//=============================================================
// extract_icon_utf8
//=============================================================

HICON mameui::winapi::shell::extract_icon_utf8(HINSTANCE instance_handle, const char *exe_filename, UINT icon_index)
{
	std::unique_ptr<wchar_t[]> wcs_exe_filename(mui_wcstring_from_utf8(exe_filename));

	if (!wcs_exe_filename)
		return 0;

	return extract_icon(instance_handle, wcs_exe_filename.get(), icon_index);
}


//=============================================================
// path_find_filename
//=============================================================

wchar_t *mameui::winapi::shell::path_find_filename(const wchar_t *path)
{
	return PathFindFileNameW(path);
}


//=============================================================
// shell_browse_for_folder
//=============================================================

LPITEMIDLIST mameui::winapi::shell::shell_browse_for_folder(LPBROWSEINFOW browse_info)
{
	return SHBrowseForFolderW(browse_info);
}


//=============================================================
// shell_execute
//=============================================================

HINSTANCE mameui::winapi::shell::shell_execute(HWND window_handle, const wchar_t *operation, const wchar_t *file, const wchar_t *parameters, const wchar_t *directory, int show_command)
{
	return ShellExecuteW(window_handle, operation, file, parameters, directory, show_command);
}


//=============================================================
// shell_get_path_from_id_list
//=============================================================

BOOL mameui::winapi::shell::shell_get_path_from_id_list(PCIDLIST_ABSOLUTE id_list, wchar_t *path)
{
	return SHGetPathFromIDListW(id_list, path);
}
