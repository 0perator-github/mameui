// license:BSD-3-Clause
// copyright-holders:0perator
#ifndef MAMEUI_WINAPP_MUI_TCS_H
#define MAMEUI_WINAPP_MUI_TCS_H

#pragma once

// standard windows headers
#include <tchar.h>

// standard C++ headers
#include <string>
#include <string_view>

namespace util
{
#if _UNICODE
	constexpr size_t MAX_TYPE_STRING_SIZE = 1024;
#else
	constexpr size_t MAX_TYPE_STRING_SIZE = 2048;
#endif
}

// mui_tcscpy
size_t mui_tcscpy(TCHAR *dst, const TCHAR *src);
TCHAR *mui_tcscpy(const TCHAR *src);

// mui_tcsncpy
size_t mui_tcsncpy(TCHAR *dst, const TCHAR *src, size_t count);
TCHAR *mui_tcsncpy(const TCHAR *src, size_t count);

// mui_tfind_delimiters
std::basic_string_view<TCHAR>::size_type mui_tfind_delimiters(std::basic_string_view<TCHAR> str, std::basic_string_view<TCHAR> delim, const size_t offset = 0U);

// mui_tcstok
std::basic_string_view<TCHAR> mui_tcstok(std::basic_string_view<TCHAR> str, std::basic_string_view<TCHAR> delim);
TCHAR* mui_tcstok(const TCHAR* str, const TCHAR* delim);

// mui_tcscmp
int mui_tcscmp(std::basic_string_view<TCHAR> s1, std::basic_string_view<TCHAR> s2);

// mui_tcsicmp
int mui_tcsicmp(std::basic_string_view<TCHAR> s1, std::basic_string_view<TCHAR> s2);

// mui_tcsncmp
int mui_tcsncmp(std::basic_string_view<TCHAR> s1, std::basic_string_view<TCHAR> s2, int count);

// mui_tcsnicmp
int mui_tcsnicmp(std::basic_string_view<TCHAR> s1, std::basic_string_view<TCHAR> s2, int count);

// mui_tscnlen
size_t mui_strnlen(const TCHAR *src, const size_t max_count);

// mui_tsclen
size_t mui_tcslen(const TCHAR *src);

#endif // MAMEUI_WINAPP_MUI_TCS_H
