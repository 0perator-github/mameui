// license:BSD-3-Clause
// copyright-holders:0perator

#ifndef MAMEUI_LIB_WINAPI_DATA_ACCESS_STORAGE_H
#define MAMEUI_LIB_WINAPI_DATA_ACCESS_STORAGE_H

namespace mameui::winapi::storage
{
	HANDLE create_file(const wchar_t *filename, DWORD desired_access, DWORD share_mode, LPSECURITY_ATTRIBUTES security_attributes, DWORD creation_disposition, DWORD flags_and_attributes, HANDLE template_handle);
	HANDLE create_file_utf8(const char *filename, DWORD desired_access, DWORD share_mode, LPSECURITY_ATTRIBUTES security_attributes, DWORD creation_disposition, DWORD flags_and_attributes, HANDLE template_handle);

	HANDLE find_first_file(const wchar_t *filename, LPWIN32_FIND_DATAW find_data);
	HANDLE find_first_file_utf8(const char *filename, LPWIN32_FIND_DATAA find_data);

	BOOL find_next_file(HANDLE find_file, LPWIN32_FIND_DATAW find_data);
	BOOL find_next_file_utf8(HANDLE find_file, LPWIN32_FIND_DATAA find_data);

	DWORD get_current_directory(DWORD buffer_length, wchar_t *buffer);
	DWORD get_current_directory_utf8(DWORD buffer_length, char *buffer);

	DWORD get_file_attributes(const wchar_t *filename);
	DWORD get_file_attributes_utf8(const char *filename);

	BOOL move_file(const wchar_t *existing_filename, const wchar_t *new_filename);
	BOOL move_file_utf8(const char *existing_filename, const char *new_filename);
}

#endif // MAMEUI_LIB_WINAPI_DATA_ACCESS_STORAGE_H
