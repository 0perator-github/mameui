// license:BSD-3-Clause
// copyright-holders:0perator

#ifndef MAMEUI_LIB_WINAPI_WINDOWS_REGISTRY_H
#define MAMEUI_LIB_WINAPI_WINDOWS_REGISTRY_H

namespace mameui::winapi
{
	namespace registry
	{
		LSTATUS reg_close_key(HKEY opened_key_handle);

		LSTATUS reg_open_key_ex(HKEY key_handle, const wchar_t *sub_key, DWORD options, REGSAM desired_access_rights, PHKEY opened_key_handle);

		LSTATUS reg_open_key_ex_utf8(HKEY key_handle, const char *sub_key, DWORD options, REGSAM desired_access_rights, PHKEY opened_key_handle);

		LSTATUS reg_query_value_ex(HKEY opened_key_handle, const wchar_t *registry_value, LPDWORD reserved, LPDWORD value_type, LPBYTE value_data, LPDWORD data_size);

		LSTATUS reg_query_value_ex_utf8(HKEY opened_key_handle, const char* registry_value, LPDWORD reserved, LPDWORD value_type, LPBYTE value_data, LPDWORD data_size);
	}
}

#endif // MAMEUI_LIB_WINAPI_WINDOWS_REGISTRY_H
