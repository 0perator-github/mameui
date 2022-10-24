// license:BSD-3-Clause
// copyright-holders:0perator
// ============================================================================

#ifndef MAMEUI_LIB_WINAPI_DATA_ACCESS_STORAGE_H
#define MAMEUI_LIB_WINAPI_DATA_ACCESS_STORAGE_H

#pragma once

namespace mameui::winapi::storage
{
	using read_directory_changes_proc = BOOL(WINAPI*)(HANDLE directory, LPVOID buffer, DWORD buffer_length, BOOL watch_subtree, DWORD notify_filter, LPDWORD bytes_returned, LPOVERLAPPED overlapped, LPOVERLAPPED_COMPLETION_ROUTINE completion_routine);

	BOOL read_directory_changes(read_directory_changes_proc procedure, HANDLE directory, LPVOID buffer, DWORD buffer_length, BOOL watch_subtree, DWORD notify_filter, LPDWORD bytes_returned, LPOVERLAPPED overlapped, LPOVERLAPPED_COMPLETION_ROUTINE completion_routine);

	HANDLE create_file(const wchar_t *file_name, DWORD desired_access, DWORD share_mode, LPSECURITY_ATTRIBUTES security_attributes, DWORD creation_disposition, DWORD flags_and_attributes, HANDLE template_handle);
	HANDLE create_file_utf8(const char *file_name, DWORD desired_access, DWORD share_mode, LPSECURITY_ATTRIBUTES security_attributes, DWORD creation_disposition, DWORD flags_and_attributes, HANDLE template_handle);

	HANDLE find_first_file(const wchar_t *file_name, LPWIN32_FIND_DATAW find_data);
	HANDLE find_first_file_utf8(const char *file_name, LPWIN32_FIND_DATAA find_data);

	BOOL find_next_file(HANDLE find_file, LPWIN32_FIND_DATAW find_data);
	BOOL find_next_file_utf8(HANDLE find_file, LPWIN32_FIND_DATAA find_data);

	BOOL find_close(HANDLE find_file);

	DWORD get_file_size(HANDLE file_handle, LPDWORD file_size_high);

	HANDLE create_file_mapping(HANDLE file_handle, LPSECURITY_ATTRIBUTES security_attributes, DWORD protection, DWORD maximum_size_high, DWORD maximum_size_low, const wchar_t *name);

	LPVOID map_view_of_file(HANDLE file_mapping, DWORD desired_access, DWORD file_offset_high, DWORD file_offset_low, SIZE_T number_of_bytes_to_map);

	BOOL unmap_view_of_file(LPCVOID base_address);

	DWORD get_current_directory(DWORD buffer_length, wchar_t *buffer);
	DWORD get_current_directory_utf8(DWORD buffer_length, char *buffer);

	DWORD get_file_attributes(const wchar_t *file_name);
	DWORD get_file_attributes_utf8(const char *file_name);

	DWORD get_full_path_name(const wchar_t *file_name, DWORD buffer_length, wchar_t *buffer, wchar_t **file_part);
	DWORD get_full_path_name_utf8(const char *file_name, DWORD buffer_length, char *buffer, char **file_part);

	BOOL move_file(const wchar_t *existing_file_name, const wchar_t *new_file_name);
	BOOL move_file_utf8(const char *existing_file_name, const char *new_file_name);
}

#endif // MAMEUI_LIB_WINAPI_DATA_ACCESS_STORAGE_H
