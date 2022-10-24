// license:BSD-3-Clause
// copyright-holders:0perator

#ifndef MAMEUI_LIB_WINAPI_WINDOWS_SHELL_H
#define MAMEUI_LIB_WINAPI_WINDOWS_SHELL_H

namespace mameui::winapi::shell
{
	wchar_t **commandline_to_argv(const wchar_t *commandline, int *number_of_args);
	char **commandline_to_argv_utf8(const char *commandline, int *number_of_args);

	HICON extract_icon(HINSTANCE instance_handle, const wchar_t *exe_filename, UINT icon_index);
	HICON extract_icon_utf8(HINSTANCE instance_handle, const char *exe_filename, UINT icon_index);

	wchar_t *path_find_filename(const wchar_t *path);
	char *path_find_filename_utf8(const char *path);

	void path_remove_extension(wchar_t *path);
	void path_remove_extension_utf8(char *path);

	LPITEMIDLIST shell_browse_for_folder(LPBROWSEINFOW browse_info);
	LPITEMIDLIST shell_browse_for_folder_utf8(LPBROWSEINFOA browse_info);

	HINSTANCE shell_execute(HWND window_handle, const wchar_t *operation, const wchar_t *file, const wchar_t *parameters, const wchar_t *directory, int show_command);
	HINSTANCE shell_execute_utf8(HWND window_handle, const char* operation, const char* file, const char* parameters, const char* directory, int show_command);

	BOOL shell_get_path_from_id_list(PCIDLIST_ABSOLUTE id_list, wchar_t *path);
	BOOL shell_get_path_from_id_list_utf8(PCIDLIST_ABSOLUTE id_list, char *path);
}

#endif // MAMEUI_LIB_WINAPI_WINDOWS_SHELL_H
