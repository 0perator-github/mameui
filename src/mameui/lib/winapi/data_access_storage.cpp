// license:BSD-3-Clause
// copyright-holders:0perator

// ============================================================================
// data_access_storage.cpp - Wrapper functions for Windows file I/O
// ============================================================================

// standard C++ headers

// standard windows headers
#include "winapi_common.h"
#include <fileapi.h>

// MAME headers

// MAMEUI headers

#include "data_access_storage.h"

// --------------------------------------------------------
// read_directory_changes
// --------------------------------------------------------

BOOL mameui::winapi::storage::read_directory_changes(read_directory_changes_proc procedure, HANDLE directory, LPVOID buffer, DWORD buffer_length, BOOL watch_subtree, DWORD notify_filter, LPDWORD bytes_returned, LPOVERLAPPED overlapped, LPOVERLAPPED_COMPLETION_ROUTINE completion_routine)
{
	return procedure(directory, buffer, buffer_length, watch_subtree, notify_filter, bytes_returned, overlapped, completion_routine);
}

// --------------------------------------------------------
// create_file
// --------------------------------------------------------

HANDLE mameui::winapi::storage::create_file(const wchar_t *file_name, DWORD desired_access, DWORD share_mode,
	LPSECURITY_ATTRIBUTES security_attributes, DWORD creation_disposition, DWORD flags_and_attributes, HANDLE template_handle)
{
	return CreateFileW(file_name, desired_access, share_mode, security_attributes, creation_disposition, flags_and_attributes, template_handle);
}

// --------------------------------------------------------
// create_file_utf8
// --------------------------------------------------------

HANDLE mameui::winapi::storage::create_file_utf8(const char *file_name, DWORD desired_access, DWORD share_mode,
	LPSECURITY_ATTRIBUTES security_attributes, DWORD creation_disposition, DWORD flags_and_attributes, HANDLE template_handle)
{
	return CreateFileA(file_name, desired_access, share_mode, security_attributes, creation_disposition, flags_and_attributes, template_handle);
}

// --------------------------------------------------------
// find_first_file
// --------------------------------------------------------

HANDLE mameui::winapi::storage::find_first_file(const wchar_t *file_name, LPWIN32_FIND_DATAW find_data)
{
	return FindFirstFileW(file_name, find_data);
}

// --------------------------------------------------------
// find_first_file_utf8
// --------------------------------------------------------

HANDLE mameui::winapi::storage::find_first_file_utf8(const char *file_name, LPWIN32_FIND_DATAA find_data)
{
	return FindFirstFileA(file_name, find_data);
}

// --------------------------------------------------------
// find_next_file
// --------------------------------------------------------

BOOL mameui::winapi::storage::find_next_file(HANDLE find_file, LPWIN32_FIND_DATAW find_data)
{
	return FindNextFileW(find_file, find_data);
}

// --------------------------------------------------------
// find_close
// --------------------------------------------------------

BOOL mameui::winapi::storage::find_close(HANDLE find_file)
{
	return FindClose(find_file);
}

// --------------------------------------------------------
// get_file_size
// --------------------------------------------------------

DWORD mameui::winapi::storage::get_file_size(HANDLE file_handle, LPDWORD file_size_high)
{
	return GetFileSize(file_handle, file_size_high);
}

// --------------------------------------------------------
// create_file_mapping
// --------------------------------------------------------

HANDLE mameui::winapi::storage::create_file_mapping(HANDLE file_handle, LPSECURITY_ATTRIBUTES security_attributes, DWORD protection, DWORD maximum_size_high, DWORD maximum_size_low, const wchar_t *name)
{
	return CreateFileMappingW(file_handle, security_attributes, protection, maximum_size_high, maximum_size_low, name);
}

// --------------------------------------------------------
// map_view_of_file
// --------------------------------------------------------

LPVOID mameui::winapi::storage::map_view_of_file(HANDLE file_mapping, DWORD desired_access, DWORD file_offset_high, DWORD file_offset_low, SIZE_T number_of_bytes_to_map)
{
	return MapViewOfFile(file_mapping, desired_access, file_offset_high, file_offset_low, number_of_bytes_to_map);
}

// --------------------------------------------------------
// unmap_view_of_file
// --------------------------------------------------------

BOOL mameui::winapi::storage::unmap_view_of_file(LPCVOID base_address)
{
	return UnmapViewOfFile(base_address);
}

// --------------------------------------------------------
// find_next_file_utf8
// --------------------------------------------------------

BOOL mameui::winapi::storage::find_next_file_utf8(HANDLE find_file, LPWIN32_FIND_DATAA find_data)
{
	return FindNextFileA(find_file, find_data);
}

// --------------------------------------------------------
// get_current_directory
// --------------------------------------------------------

DWORD mameui::winapi::storage::get_current_directory(DWORD buffer_length, wchar_t *buffer)
{
	return GetCurrentDirectoryW(buffer_length, buffer);
}

// --------------------------------------------------------
// get_current_directory_utf8
// --------------------------------------------------------

DWORD mameui::winapi::storage::get_current_directory_utf8(DWORD buffer_length, char *buffer)
{
	return GetCurrentDirectoryA(buffer_length, buffer);
}

// --------------------------------------------------------
//  get_file_attributes
// --------------------------------------------------------

DWORD mameui::winapi::storage::get_file_attributes(const wchar_t *file_name)
{
	return GetFileAttributesW(file_name);
}


// --------------------------------------------------------
//  get_file_attributes_utf8
// --------------------------------------------------------

DWORD mameui::winapi::storage::get_file_attributes_utf8(const char *file_name)
{
	return GetFileAttributesA(file_name);
}

// --------------------------------------------------------
// get_full_path_name
// --------------------------------------------------------

DWORD mameui::winapi::storage::get_full_path_name(const wchar_t *file_name, DWORD buffer_length, wchar_t *buffer, wchar_t **file_part)
{
	return GetFullPathNameW(file_name, buffer_length, buffer, file_part);
}

// --------------------------------------------------------
// get_full_path_name_utf8
// --------------------------------------------------------

DWORD mameui::winapi::storage::get_full_path_name_utf8(const char *file_name, DWORD buffer_length, char *buffer, char **file_part)
{
	return GetFullPathNameA(file_name, buffer_length, buffer, file_part);
}


// --------------------------------------------------------
// move_file
// --------------------------------------------------------

BOOL mameui::winapi::storage::move_file(const wchar_t *existing_file_name, const wchar_t *new_file_name)
{
	return MoveFileW(existing_file_name, new_file_name);
}

// --------------------------------------------------------
// move_file_utf8
// --------------------------------------------------------

BOOL mameui::winapi::storage::move_file_utf8(const char *existing_file_name, const char *new_file_name)
{
	return MoveFileA(existing_file_name, new_file_name);
}
