// license:BSD-3-Clause
// copyright-holders:0perator

//===========================================================================
//
// system_services.cpp - Wrapper functions for utilizing Win32 system services
//
//===========================================================================

// standard C++ headers
#include <memory>
#include <string>

// standard windows headers
#include "winapi_common.h"
#include <debugapi.h>
#include <handleapi.h>
#include <libloaderapi.h>
#include <synchapi.h>
#include <Versionhelpers.h>

// MAMEUI headers
#include "mui_wcstrconv.h"

#include "system_services.h"

using namespace mameui::util::string_util;

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

HANDLE mameui::winapi::system_services::create_event(LPSECURITY_ATTRIBUTES security_attributes, BOOL manual_reset, BOOL initial_state, const wchar_t *event_name)
{
	return CreateEventW(security_attributes, manual_reset, initial_state, event_name);
}

//============================================================
//  create_event_utf8
//============================================================

HANDLE mameui::winapi::system_services::create_event_utf8(LPSECURITY_ATTRIBUTES security_attributes, BOOL manual_reset, BOOL initial_state, const char *event_name)
{
	return CreateEventA(security_attributes, manual_reset, initial_state, event_name);
}

//============================================================
//  create_thread
//============================================================

HANDLE mameui::winapi::system_services::create_thread(LPSECURITY_ATTRIBUTES security_attributes, SIZE_T stack_size, LPTHREAD_START_ROUTINE start_address, LPVOID parameter, DWORD creation_flag, LPDWORD thread_id)
{
	return CreateThread(security_attributes, stack_size, start_address, parameter, creation_flag, thread_id);
}

//============================================================
//  delete_critical_section
//============================================================

void mameui::winapi::system_services::delete_critical_section(LPCRITICAL_SECTION lpCriticalSection)
{
	DeleteCriticalSection(lpCriticalSection);
}

//============================================================
//  enter_critical_section
//============================================================

void mameui::winapi::system_services::enter_critical_section(LPCRITICAL_SECTION lpCriticalSection)
{
	EnterCriticalSection(lpCriticalSection);
}

//============================================================
//  free_library
//============================================================

BOOL mameui::winapi::system_services::free_library(HMODULE module_handle)
{
	return FreeLibrary(module_handle);
}

//============================================================
//  get_commandline
//============================================================

wchar_t *mameui::winapi::system_services::get_commandline(void)
{
	return GetCommandLineW();
}

//=============================================================
// get_commandline_utf8
//=============================================================

char *mameui::winapi::system_services::get_commandline_utf8(void)
{
	return GetCommandLineA();
}

//============================================================
//  get_last_error
//============================================================

DWORD mameui::winapi::system_services::get_last_error(void)
{
	return GetLastError();
}

//============================================================
//  get_module_filename
//============================================================

DWORD mameui::winapi::system_services::get_module_filename(HMODULE module_handle, wchar_t *buffer, DWORD buffer_size)
{
	return GetModuleFileNameW(module_handle, buffer, buffer_size);
}

//============================================================
//  get_module_filename_utf8
//============================================================

DWORD mameui::winapi::system_services::get_module_filename_utf8(HMODULE module_handle, char *buffer, DWORD buffer_size)
{
	return GetModuleFileNameA(module_handle, buffer, buffer_size);
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
	return GetModuleHandleA(module_name);
}

//============================================================
//  get_proc_address
//============================================================

FARPROC mameui::winapi::system_services::get_proc_address(HMODULE module_handle, const wchar_t *procedure_name)
{
	if (!module_handle || !procedure_name)
		return nullptr;

	std::unique_ptr<char[]> utf8_procedure_name(mui_utf8_from_utf16cstring(procedure_name));

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
//  get_tick_count
//============================================================

DWORD mameui::winapi::system_services::get_tick_count(void)
{
	return GetTickCount();
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
//  leave_critical_section
//============================================================

void mameui::winapi::system_services::leave_critical_section(LPCRITICAL_SECTION lpCriticalSection)
{
	LeaveCriticalSection(lpCriticalSection);
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

HMODULE mameui::winapi::system_services::load_library_utf8(const char *file_name)
{
	return LoadLibraryA(file_name);
}

//============================================================
//  local_free
//============================================================

HLOCAL mameui::winapi::system_services::local_free(HLOCAL memory_object)
{
	return LocalFree(memory_object);
}

//============================================================
//  output_debug_string
//============================================================

void mameui::winapi::system_services::output_debug_string(const wchar_t *output_string)
{
	// coverity[DC.DEBUGAPI] false positive: unused WinAPI function defined for completeness and possible future use.
	OutputDebugStringW(output_string);
}

//============================================================
//  output_debug_string_utf8
//============================================================

void mameui::winapi::system_services::output_debug_string_utf8(const char *output_string)
{
	// coverity[DC.DEBUGAPI] false positive: unused WinAPI function defined for completeness and possible future use.
	OutputDebugStringA(output_string);
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

//============================================================
//  virtual_query
//============================================================

SIZE_T mameui::winapi::system_services::virtual_query(LPCVOID region_address, PMEMORY_BASIC_INFORMATION page_range_info, SIZE_T region_length)
{
	return VirtualQuery(region_address, page_range_info, region_length);
}
