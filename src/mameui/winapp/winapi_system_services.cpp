// license:BSD-3-Clause
// copyright-holders:0perator

/***************************************************************************

  winapi_system_services.cpp

 ***************************************************************************/
#include "winapi_system_services.h"

// standard C++ headers
#include <memory>

// standard windows headers

// MAMEUI headers
#include "mui_wcsconv.h"


//============================================================
//  close_handle
//============================================================

BOOL mameui::winapi::system_services::close_handle(HANDLE object_handle)
{
	return CloseHandle(object_handle);
}


//============================================================
//  create_event
//============================================================

HANDLE mameui::winapi::system_services::create_event(LPSECURITY_ATTRIBUTES security_attributes, BOOL manual_reset,BOOL initial_state,const wchar_t *event_name)
{
	return CreateEventW(security_attributes, manual_reset, initial_state,event_name);
}


//============================================================
//  create_thread
//============================================================

HANDLE mameui::winapi::system_services::create_thread(LPSECURITY_ATTRIBUTES security_attributes, SIZE_T stack_size, LPTHREAD_START_ROUTINE start_address, LPVOID parameter, DWORD creation_flag, LPDWORD thread_id)
{
	return CreateThread(security_attributes, stack_size, start_address, parameter, creation_flag, thread_id);
}


//============================================================
//  free_library
//============================================================

BOOL mameui::winapi::system_services::free_library(HMODULE module_handle)
{
	return FreeLibrary(module_handle);
}


//============================================================
//  get_module_filename
//============================================================

DWORD mameui::winapi::system_services::get_module_filename(HMODULE module_handle,wchar_t *buffer,DWORD buffer_size)
{
	return GetModuleFileNameW(module_handle, buffer, buffer_size);
}


//============================================================
//  get_module_filename_utf8
//============================================================

DWORD mameui::winapi::system_services::get_module_filename_utf8(HMODULE module_handle, char *buffer, DWORD buffer_size)
{
	std::unique_ptr<wchar_t[]> wcs_buffer(mui_wcstring_from_utf8(buffer));
	if (!wcs_buffer)
		return 0;

	return GetModuleFileNameW(module_handle, wcs_buffer.get(), buffer_size);
}


//============================================================
//  get_module_handle
//============================================================

HMODULE mameui::winapi::system_services::get_module_handle(const wchar_t *module_name)
{
	return GetModuleHandleW(module_name);
}


//============================================================
//  get_module_handle_utf8
//============================================================

HMODULE mameui::winapi::system_services::get_module_handle_utf8(const char *module_name)
{
	std::unique_ptr<wchar_t[]>  wcs_module_name(mui_wcstring_from_utf8(module_name));

	if (!wcs_module_name)
		return 0;

	return get_module_handle(wcs_module_name.get());
}


//============================================================
//  get_proc_address
//============================================================

FARPROC mameui::winapi::system_services::get_proc_address(HMODULE module_handle, const wchar_t *procedure_name)
{
	std::unique_ptr<char[]>  utf8_procedure_name(mui_utf8_from_wcstring(procedure_name));

	if (!utf8_procedure_name)
		return 0;

	return get_proc_address_utf8(module_handle, utf8_procedure_name.get());
}


//============================================================
//  get_proc_address_utf8
//============================================================

FARPROC mameui::winapi::system_services::get_proc_address_utf8(HMODULE module_handle, const char *procedure_name)
{
	return GetProcAddress(module_handle, procedure_name);
}


//============================================================
//  initialize_critical_section
//============================================================

void mameui::winapi::system_services::initialize_critical_section(LPCRITICAL_SECTION critical_section)
{
	InitializeCriticalSection(critical_section);
}


//============================================================
//  is_windows7_or_greater
//============================================================

bool mameui::winapi::system_services::is_windows7_or_greater()
{
	return IsWindows7OrGreater();
}


//============================================================
//  load_library
//============================================================

HMODULE mameui::winapi::system_services::load_library(const wchar_t *file_name)
{
	return LoadLibraryW(file_name);
}


//============================================================
//  load_library_utf8
//============================================================

HMODULE mameui::winapi::system_services::load_library_utf8(const char* file_name)
{
	std::unique_ptr<wchar_t[]>  wcs_file_name(mui_wcstring_from_utf8(file_name));

	if (!wcs_file_name)
		return 0;

	return load_library(wcs_file_name.get());
}


//============================================================
//  output_debug_string
//============================================================

void mameui::winapi::system_services::output_debug_string(const wchar_t *output_string)
{
	OutputDebugStringW(output_string);
}


//============================================================
//  output_debug_string_utf8
//============================================================

void mameui::winapi::system_services::output_debug_string_utf8(const char* output_string)
{
	std::unique_ptr<wchar_t[]>  wcs_output_string(mui_wcstring_from_utf8(output_string));

	if (!wcs_output_string)
		return;

	output_debug_string(wcs_output_string.get());
}


//============================================================
//  set_event
//============================================================

BOOL mameui::winapi::system_services::set_event(HANDLE event_handle)
{
	return SetEvent(event_handle);
}


//============================================================
//  wait_for_single_object
//============================================================

DWORD mameui::winapi::system_services::wait_for_single_object(HANDLE object_handle, DWORD time_out)
{
	return WaitForSingleObject(object_handle, time_out);
}
