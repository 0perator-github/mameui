// license:BSD-3-Clause
// copyright-holders:0perator

//===========================================================================
//
// windows_registry.cpp - Wrapper functions for the Windows Registry
//
//===========================================================================

// standard windows headers
#include "winapi_common.h"
#include "winreg.h"

// MAMEUI headers
#include "windows_messages.h"

#include "windows_registry.h"

using namespace mameui::winapi::windows;

//============================================================
// reg_close_key
//============================================================

LSTATUS mameui::winapi::registry::reg_close_key(HKEY opened_key_handle)
{
	return RegCloseKey(opened_key_handle);
}

//============================================================
// reg_open_key_ex
//============================================================

LSTATUS mameui::winapi::registry::reg_open_key_ex(HKEY key_handle, const wchar_t *sub_key, DWORD options, REGSAM desired_access_rights, PHKEY opened_key_handle)
{
	return RegOpenKeyExW(key_handle, sub_key, options, desired_access_rights, opened_key_handle);
}

//============================================================
// reg_open_key_ex_utf8
//============================================================

LSTATUS mameui::winapi::registry::reg_open_key_ex_utf8(HKEY key_handle, const char *sub_key, DWORD options, REGSAM desired_access_rights, PHKEY opened_key_handle)
{
	return RegOpenKeyExA(key_handle, sub_key, options, desired_access_rights, opened_key_handle);
}

//============================================================
// reg_query_value_ex
//============================================================

LSTATUS mameui::winapi::registry::reg_query_value_ex(HKEY opened_key_handle, const wchar_t *registry_value, LPDWORD reserved, LPDWORD value_type, LPBYTE value_data, LPDWORD data_size)
{
	return RegQueryValueExW(opened_key_handle, registry_value, reserved, value_type, value_data, data_size);
}

//============================================================
// reg_query_value_ex_utf8
//============================================================

LSTATUS mameui::winapi::registry::reg_query_value_ex_utf8(HKEY opened_key_handle, const char *registry_value, LPDWORD reserved, LPDWORD value_type, LPBYTE value_data, LPDWORD data_size)
{
	return RegQueryValueExA(opened_key_handle, registry_value, reserved, value_type, value_data, data_size);
}
