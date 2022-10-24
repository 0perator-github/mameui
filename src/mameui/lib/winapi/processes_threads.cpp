// license:BSD-3-Clause
// copyright-holders:0perator

//===========================================================================
//
// processes_threads.cpp - Wrapper functions for managing Win32 processes and threads
//
//===========================================================================

// standard C++ headers

// standard windows headers
#include "winapi_common.h"
#include <handleapi.h>
#include <processthreadsapi.h>

// MAME headers

// MAMEUI headers

#include "processes_threads.h"

//============================================================
//  create_process
//============================================================

BOOL mameui::winapi::processes_threads::create_process(const wchar_t *application_name, wchar_t *commande_line, LPSECURITY_ATTRIBUTES process_security_attributes, LPSECURITY_ATTRIBUTES thread_security_attributes, BOOL inherit_handles, DWORD creation_flags, LPVOID environment_block, const wchar_t *current_directory, LPSTARTUPINFOW startup_info, LPPROCESS_INFORMATION process_info)
{
	return CreateProcessW(application_name, commande_line, process_security_attributes, thread_security_attributes, inherit_handles, creation_flags, environment_block, current_directory, startup_info, process_info);
}

//============================================================
//  create_process_utf8
//============================================================

BOOL mameui::winapi::processes_threads::create_process_utf8(const char *application_name, char *commande_line, LPSECURITY_ATTRIBUTES process_security_attributes, LPSECURITY_ATTRIBUTES thread_security_attributes, BOOL inherit_handles, DWORD creation_flags, LPVOID environment_block, const char *current_directory, LPSTARTUPINFOA startup_info, LPPROCESS_INFORMATION process_info)
{
	return CreateProcessA(application_name, commande_line, process_security_attributes, thread_security_attributes, inherit_handles, creation_flags, environment_block, current_directory, startup_info, process_info);
}

//============================================================
//  create_thread
//============================================================

HANDLE mameui::winapi::processes_threads::create_thread(LPSECURITY_ATTRIBUTES security_attributes, SIZE_T stack_size, LPTHREAD_START_ROUTINE start_address, LPVOID parameter, DWORD creation_flag, LPDWORD thread_id)
{
	return CreateThread(security_attributes, stack_size, start_address, parameter, creation_flag, thread_id);
}

//============================================================
//  get_current_thread
//============================================================

HANDLE mameui::winapi::processes_threads::get_current_thread(void)
{
	return GetCurrentThread();
}

//============================================================
//  get_current_thread_id
//============================================================

DWORD mameui::winapi::processes_threads::get_current_thread_id(void)
{
	return GetCurrentThreadId();
}
