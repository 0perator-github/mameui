// license:BSD-3-Clause  
// copyright-holders:0perator  

//===========================================================================
//
// data_access_storage.cpp - Wrapper functions for Windows file I/O
//
//===========================================================================  

// standard C++ headers  

// standard windows headers
#include "winapi_common.h"
#include <fileapi.h>

// MAME headers

// MAMEUI headers

#include "data_access_storage.h"

//============================================================  
// create_file  
//============================================================  

HANDLE mameui::winapi::storage::create_file(const wchar_t *filename, DWORD desired_access, DWORD share_mode,
	LPSECURITY_ATTRIBUTES security_attributes, DWORD creation_disposition, DWORD flags_and_attributes, HANDLE template_handle)
{
	return CreateFileW(filename, desired_access, share_mode, security_attributes, creation_disposition, flags_and_attributes, template_handle);
}

//============================================================  
// create_file_utf8  
//============================================================  

HANDLE mameui::winapi::storage::create_file_utf8(const char *filename, DWORD desired_access, DWORD share_mode,
	LPSECURITY_ATTRIBUTES security_attributes, DWORD creation_disposition, DWORD flags_and_attributes, HANDLE template_handle)
{
	return CreateFileA(filename, desired_access, share_mode, security_attributes, creation_disposition, flags_and_attributes, template_handle);
}

//============================================================  
// find_first_file  
//============================================================  

HANDLE mameui::winapi::storage::find_first_file(const wchar_t *filename, LPWIN32_FIND_DATAW find_data)
{
	return FindFirstFileW(filename, find_data);
}

//============================================================  
// find_first_file_utf8  
//============================================================  

HANDLE mameui::winapi::storage::find_first_file_utf8(const char *filename, LPWIN32_FIND_DATAA find_data)
{
	return FindFirstFileA(filename, find_data);
}

//============================================================  
// find_next_file  
//============================================================  

BOOL mameui::winapi::storage::find_next_file(HANDLE find_file, LPWIN32_FIND_DATAW find_data)
{
	return FindNextFileW(find_file, find_data);
}

//============================================================  
// find_next_file_utf8  
//============================================================  

BOOL mameui::winapi::storage::find_next_file_utf8(HANDLE find_file, LPWIN32_FIND_DATAA find_data)
{
	return FindNextFileA(find_file, find_data);
}

//============================================================  
// get_current_directory  
//============================================================  

DWORD mameui::winapi::storage::get_current_directory(DWORD buffer_length, wchar_t *buffer)
{
	return GetCurrentDirectoryW(buffer_length, buffer);
}

//============================================================  
// get_current_directory_utf8  
//============================================================  

DWORD mameui::winapi::storage::get_current_directory_utf8(DWORD buffer_length, char *buffer)
{
	return GetCurrentDirectoryA(buffer_length, buffer);
}


//============================================================
//  get_file_attributes
//============================================================

DWORD mameui::winapi::storage::get_file_attributes(const wchar_t *filename)
{
	return GetFileAttributesW(filename);
}


//============================================================
//  get_file_attributes_utf8
//============================================================

DWORD mameui::winapi::storage::get_file_attributes_utf8(const char *filename)
{
	return GetFileAttributesA(filename);
}


//============================================================  
// move_file  
//============================================================  

BOOL mameui::winapi::storage::move_file(const wchar_t *existing_filename, const wchar_t *new_filename)
{
	return MoveFileW(existing_filename, new_filename);
}

//============================================================  
// move_file_utf8  
//============================================================  

BOOL mameui::winapi::storage::move_file_utf8(const char *existing_filename, const char *new_filename)
{
	return MoveFileA(existing_filename, new_filename);
}
