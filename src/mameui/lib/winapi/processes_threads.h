// license:BSD-3-Clause
// copyright-holders:0perator

#ifndef MAMEUI_LIB_WINAPI_PROCESSES_THREADS_H
#define MAMEUI_LIB_WINAPI_PROCESSES_THREADS_H

namespace mameui::winapi::processes_threads
{
	BOOL create_process(const wchar_t *application_name, wchar_t *commande_line, LPSECURITY_ATTRIBUTES process_security_attributes, LPSECURITY_ATTRIBUTES thread_security_attributes, BOOL inherit_handles, DWORD creation_flags, LPVOID environment_block, const wchar_t *current_directory, LPSTARTUPINFOW startup_info, LPPROCESS_INFORMATION process_info);
	BOOL create_process_utf8(const char *application_name, char *commande_line, LPSECURITY_ATTRIBUTES process_security_attributes, LPSECURITY_ATTRIBUTES thread_security_attributes, BOOL inherit_handles, DWORD creation_flags, LPVOID environment_block, const char *current_directory, LPSTARTUPINFOA startup_info, LPPROCESS_INFORMATION process_info);

	HANDLE create_thread(LPSECURITY_ATTRIBUTES security_attributes, SIZE_T stack_size, LPTHREAD_START_ROUTINE start_address, LPVOID parameter, DWORD creation_flag, LPDWORD thread_id);

	HANDLE get_current_thread(void);

	DWORD get_current_thread_id(void);
}

#endif // MAMEUI_LIB_WINAPI_PROCESSES_THREADS_H
