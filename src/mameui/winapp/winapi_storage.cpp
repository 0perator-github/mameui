// license:BSD-3-Clause
// copyright-holders:0perator

/***************************************************************************

  winapi_storage.cpp

 ***************************************************************************/
#include "winapi_storage.h"

// standard C++ headers
#include <memory>

// standard windows headers
#include <fileapi.h>

// MAMEUI headers
#include "mui_wcsconv.h"


//============================================================
// create_file
//============================================================

HANDLE mameui::winapi::storage::create_file(const wchar_t* filename, DWORD desired_access, DWORD share_mode,
	LPSECURITY_ATTRIBUTES security_attributes, DWORD creation_disposition, DWORD flags_and_attributes, HANDLE template_handle)
{
	return CreateFileW(filename, desired_access, share_mode, security_attributes, creation_disposition, flags_and_attributes, template_handle);;
}


//============================================================
// create_file_utf8
//============================================================

HANDLE mameui::winapi::storage::create_file_utf8(const char* filename, DWORD desired_access, DWORD share_mode,
	LPSECURITY_ATTRIBUTES security_attributes, DWORD creation_disposition, DWORD flags_and_attributes, HANDLE template_handle)
{
	std::unique_ptr<wchar_t[]>  wcs_filename(mui_wcstring_from_utf8(filename));

	if (!wcs_filename)
		return 0;

	return create_file(wcs_filename.get(), desired_access, share_mode, security_attributes, creation_disposition, flags_and_attributes, template_handle);
}


//============================================================
//  find_first_file
//============================================================

HANDLE mameui::winapi::storage::find_first_file(const wchar_t *filename, LPWIN32_FIND_DATAW find_data)
{
	return FindFirstFileW(filename, find_data);
}


//============================================================
//  find_first_file_utf8
//============================================================

HANDLE mameui::winapi::storage::find_first_file_utf8(const char *filename, LPWIN32_FIND_DATAA find_data)
{
	return FindFirstFileA(filename, find_data);
}


//============================================================
//  find_next_file
//============================================================

BOOL mameui::winapi::storage::find_next_file(HANDLE find_file, LPWIN32_FIND_DATAW find_data)
{
	return FindNextFileW(find_file, find_data);
}


//============================================================
//  find_next_file_utf8
//============================================================

BOOL mameui::winapi::storage::find_next_file_utf8(HANDLE find_file, LPWIN32_FIND_DATAA find_data)
{
	return FindNextFileA(find_file, find_data);
}


//============================================================
//  get_current_directory
//============================================================

DWORD mameui::winapi::storage::get_current_directory(DWORD buffer_length, wchar_t *buffer)
{
	return GetCurrentDirectoryW(buffer_length, buffer);
}


//============================================================
//  get_current_directory_utf8
//============================================================

DWORD mameui::winapi::storage::get_current_directory_utf8(DWORD buffer_length, char *buffer)
{
	DWORD result = 0;
	std::unique_ptr<wchar_t[]> wcs_buffer(new wchar_t[buffer_length]);

	if (!wcs_buffer)
		return result;

	result = get_current_directory(buffer_length, wcs_buffer.get());
	if (!result)
		return result;

	result = mui_utf8_from_wcstring(buffer, wcs_buffer.get());

	return result;
}


//============================================================
//  move_file
//============================================================

BOOL mameui::winapi::storage::move_file(const wchar_t *existing_filename, const wchar_t *new_filename)
{
	return MoveFileW(existing_filename, new_filename);
}


//============================================================
//  move_file_utf8
//============================================================

BOOL mameui::winapi::storage::move_file_utf8(const char *existing_filename, const char *new_filename)
{
	BOOL result = false;
	std::unique_ptr<wchar_t[]> wcs_existing_filename, wcs_new_filename;


	wcs_existing_filename = std::unique_ptr<wchar_t[]>(mui_wcstring_from_utf8(existing_filename));
	if (!wcs_existing_filename)
		return result;

	wcs_new_filename = std::unique_ptr<wchar_t[]>(mui_wcstring_from_utf8(new_filename));
	if (!wcs_new_filename)
		return result;

	return move_file(wcs_existing_filename.get(), wcs_new_filename.get());
}
