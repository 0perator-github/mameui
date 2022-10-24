// license:BSD-3-Clause
// copyright-holders:0perator

#ifndef MAMEUI_LIB_WINAPI_SYSTEM_SERVICES_H
#define MAMEUI_LIB_WINAPI_SYSTEM_SERVICES_H

namespace mameui::winapi::system_services
{
	BOOL close_handle(HANDLE object_handle);

	HANDLE create_event(LPSECURITY_ATTRIBUTES security_attributes, BOOL manual_reset, BOOL initial_state, const wchar_t *event_name);
	HANDLE create_event_utf8(LPSECURITY_ATTRIBUTES security_attributes, BOOL manual_reset, BOOL initial_state, const char *event_name);

	HANDLE create_thread(LPSECURITY_ATTRIBUTES security_attributes, SIZE_T stack_size, LPTHREAD_START_ROUTINE start_address, LPVOID parameter, DWORD creation_flag, LPDWORD thread_id);


	void delete_critical_section(LPCRITICAL_SECTION lpCriticalSection);

	void enter_critical_section(LPCRITICAL_SECTION lpCriticalSection);

	BOOL free_library(HMODULE module_handle);

	wchar_t *get_commandline(void);
	char *get_commandline_utf8(void);

	DWORD get_last_error(void);

	DWORD get_module_filename(HMODULE module_handle, wchar_t *buffer, DWORD buffer_size);
	DWORD get_module_filename_utf8(HMODULE module_handle, char *buffer, DWORD buffer_size);

	HMODULE get_module_handle(const wchar_t *module_name);
	HMODULE get_module_handle_utf8(const char *module_name);

	FARPROC get_proc_address(HMODULE module_handle, const wchar_t *procedure_name);
	FARPROC get_proc_address_utf8(HMODULE module_handle, const char *procedure_name);

	DWORD get_tick_count(void);

	void initialize_critical_section(LPCRITICAL_SECTION critical_section);

	bool is_windows7_or_greater();

	void leave_critical_section(LPCRITICAL_SECTION lpCriticalSection);

	HMODULE load_library(const wchar_t *file_name);
	HMODULE load_library_utf8(const char *file_name);

	HLOCAL local_free(HLOCAL memory_object);

	void output_debug_string(const wchar_t *output_string);
	void output_debug_string_utf8(const char *output_string);

	BOOL set_event(HANDLE event_handle);

	DWORD wait_for_single_object(HANDLE object_handle, DWORD time_out);

	SIZE_T virtual_query(LPCVOID region_address, PMEMORY_BASIC_INFORMATION page_range_info, SIZE_T region_length);
}

#endif // MAMEUI_LIB_WINAPI_SYSTEM_SERVICES_H
