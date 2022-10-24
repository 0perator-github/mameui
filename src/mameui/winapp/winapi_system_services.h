// license:BSD-3-Clause
// copyright-holders:0perator

#ifndef MAMEUI_WINAPP_WINAPI_SYSTEM_SERVICES_H
#define MAMEUI_WINAPP_WINAPI_SYSTEM_SERVICES_H

#pragma once

// standard windows headers
#include <windows.h>
#include <winbase.h>
#include <handleapi.h>
#include <libloaderapi.h>
#include <synchapi.h>
#include <processthreadsapi.h>
#include <Versionhelpers.h>

namespace mameui::winapi
{
	namespace system_services
	{
		BOOL close_handle(HANDLE object_handle);

		HANDLE create_event(LPSECURITY_ATTRIBUTES security_attributes, BOOL manual_reset, BOOL initial_state, const wchar_t *event_name);

		HANDLE create_thread(LPSECURITY_ATTRIBUTES security_attributes, SIZE_T stack_size, LPTHREAD_START_ROUTINE start_address, LPVOID parameter, DWORD creation_flag, LPDWORD thread_id);

		BOOL free_library(HMODULE module_handle);

		DWORD get_module_filename(HMODULE module_handle, wchar_t *buffer, DWORD buffer_size);
		DWORD get_module_filename_utf8(HMODULE module_handle, char *buffer, DWORD buffer_size);

		HMODULE get_module_handle(const wchar_t *module_name);
		HMODULE get_module_handle_utf8(const char *module_name);

		FARPROC get_proc_address(HMODULE module_handle, const wchar_t *procedure_name);
		FARPROC get_proc_address_utf8(HMODULE module_handle, const char *procedure_name);

		void initialize_critical_section(LPCRITICAL_SECTION critical_section);

		bool is_windows7_or_greater();

		HMODULE load_library(const wchar_t *file_name);
		HMODULE load_library_utf8(const char *file_name);

		void output_debug_string(const wchar_t* output_string);
		void output_debug_string_utf8(const char* output_string);

		DWORD wait_for_single_object(HANDLE object_handle, DWORD time_out);

		BOOL set_event(HANDLE event_handle);
	}
}

#endif // MAMEUI_WINAPP_WINAPI_SYSTEM_SERVICES_H
