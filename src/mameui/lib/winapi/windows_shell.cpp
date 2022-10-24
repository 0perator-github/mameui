// license:BSD-3-Clause
// copyright-holders:0perator

//===========================================================================
//
// windows_shell.cpp - Wrapper functions for utilizing the Windows Shell API
//
//===========================================================================

// standard C++ headers
#include <memory>
#include <new>
#include <string_view>
#include <string>


// standard windows headers
#include "winapi_common.h"


// MAMEUI headers
#include "mui_wcstrconv.h"

#include "windows_shell.h"

using namespace mameui::util::string_util;

//=============================================================
// commandline_to_argv
//=============================================================

wchar_t **mameui::winapi::shell::commandline_to_argv(const wchar_t *commandline, int *number_of_args)
{
	return CommandLineToArgvW(commandline, number_of_args);
}

//=============================================================
// commandline_to_argv_utf8
//=============================================================

char **mameui::winapi::shell::commandline_to_argv_utf8(const char *commandline, int *number_of_args)
{
	if (!commandline || !number_of_args)
		return nullptr;

	std::unique_ptr<wchar_t[]> utf16_commandline(mui_utf16_from_utf8cstring(commandline));
	if (!utf16_commandline)
		return nullptr;


	std::unique_ptr<wchar_t*[]> utf16_argv(commandline_to_argv(utf16_commandline.get(), number_of_args));
	if (!utf16_argv || *number_of_args <= 0)
		return nullptr;

	auto utf8_argv = std::make_unique<char* []>(*number_of_args);

	for (int index = 0; index < *number_of_args; index++)
	{
		utf8_argv[index] = mui_utf8_from_utf16cstring(utf16_argv[index]);
		if (!utf8_argv[index])
			return nullptr;
	}

	return utf8_argv.release();
}


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
	return ExtractIconA(instance_handle, exe_filename, icon_index);
}

//=============================================================
// path_find_filename
//=============================================================

wchar_t *mameui::winapi::shell::path_find_filename(const wchar_t *path)
{
	return PathFindFileNameW(path);
}

//=============================================================
// path_find_filename_utf8
//=============================================================

char *mameui::winapi::shell::path_find_filename_utf8(const char *path)
{
	return PathFindFileNameA(path);
}

//=============================================================
// path_remove_extension
//=============================================================

void mameui::winapi::shell::path_remove_extension(wchar_t *path)
{
	return PathRemoveExtensionW(path);
}

//=============================================================
// path_remove_extension_utf8
//=============================================================

void mameui::winapi::shell::path_remove_extension_utf8(char *path)
{
	return PathRemoveExtensionA(path);
}

//=============================================================
// shell_browse_for_folder
//=============================================================

LPITEMIDLIST mameui::winapi::shell::shell_browse_for_folder(LPBROWSEINFOW browse_info)
{
	return SHBrowseForFolderW(browse_info);
}

//=============================================================
// shell_browse_for_folder_utf8
//=============================================================

LPITEMIDLIST mameui::winapi::shell::shell_browse_for_folder_utf8(LPBROWSEINFOA browse_info)
{
	return SHBrowseForFolderA(browse_info);
}

//=============================================================
// shell_execute
//=============================================================

HINSTANCE mameui::winapi::shell::shell_execute(HWND window_handle, const wchar_t *operation, const wchar_t *file, const wchar_t *parameters, const wchar_t *directory, int show_command)
{
	return ShellExecuteW(window_handle, operation, file, parameters, directory, show_command);
}

//=============================================================
// shell_execute_utf8
//=============================================================

HINSTANCE mameui::winapi::shell::shell_execute_utf8(HWND window_handle, const char* operation, const char* file, const char* parameters, const char* directory, int show_command)
{
	return ShellExecuteA(window_handle, operation, file, parameters, directory, show_command);
}

//=============================================================
// shell_get_path_from_id_list
//=============================================================

BOOL mameui::winapi::shell::shell_get_path_from_id_list(PCIDLIST_ABSOLUTE id_list, wchar_t *path)
{
	return SHGetPathFromIDListW(id_list, path);
}

//=============================================================
// shell_get_path_from_id_list_utf8
//=============================================================

BOOL mameui::winapi::shell::shell_get_path_from_id_list_utf8(PCIDLIST_ABSOLUTE id_list, char *path)
{
	return SHGetPathFromIDListA(id_list, path);
}
