// license:BSD-3-Clause
// copyright-holders:0perator
#ifndef MAMEUI_LIB_UTIL_MUI_TCSTR_H
#define MAMEUI_LIB_UTIL_MUI_TCSTR_H

#pragma once

#if 0 // these headers are needed when including this file
// standard C++ headers
#include <cctype>
#include <cstddef> // optional for size_t
#include <cstdint>
#include <cwctype>
#include <new>
#include <string>
#include <string_view>

// standard windows headers
#include <tchar.h>
#endif

namespace mameui::util::string_util
{
	// tchar_diff
#if defined(UNICODE) || defined(_UNICODE)
	inline int tchar_diff_cs(wchar_t c1, wchar_t c2) { return  c1 - c2; }
	inline int tchar_diff_ci(wchar_t c1, wchar_t c2) { return  towupper(c1) - towupper(c2); }
#else
	inline int tchar_diff_cs(char c1, char c2) { return  static_cast<uint8_t>(c1) - static_cast<uint8_t>(c2); }
	inline int tchar_diff_ci(char c1, char c2) { return  toupper(static_cast<uint8_t>(c1)) - toupper(static_cast<uint8_t>(c2)); }
#endif // UNICODE

	using tstring = std::basic_string<TCHAR>;
	using tstring_view = std::basic_string_view<TCHAR>;

	// mui_tcscpy
	size_t mui_tcscpy(TCHAR *dst, const TCHAR *src);
	TCHAR *mui_tcscpy(const TCHAR *src);

	// mui_tcsncpy
	size_t mui_tcsncpy(TCHAR *dst, const TCHAR *src, const size_t count);
	TCHAR *mui_tcsncpy(const TCHAR *src, const size_t count);

	// mui_tcschr
	TCHAR *mui_tcschr(tstring &str, TCHAR delim);
	TCHAR *mui_tcschr(tstring &str, tstring_view delim);
	const TCHAR *mui_tcschr(tstring_view str, TCHAR delim);
	const TCHAR *mui_tcschr(tstring_view str, tstring_view delim);

	// mui_tcscmp
	int mui_tcscmp(tstring_view s1, tstring_view s2);

	// mui_tcsicmp
	int mui_tcsicmp(tstring_view s1, tstring_view s2);

	// mui_tcsncmp
	int mui_tcsncmp(tstring_view s1, tstring_view s2, const size_t count);

	// mui_tcsnicmp
	int mui_tcsnicmp(tstring_view s1, tstring_view s2, const size_t count);

	// mui_tscnlen
	size_t mui_tcsnlen(const TCHAR *src, const size_t max_count);

	// mui_tsclen
	size_t mui_tcslen(const TCHAR *src);
}

#endif // MAMEUI_LIB_UTIL_MUI_TCSTR_H
