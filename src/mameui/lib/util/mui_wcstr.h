// license:BSD-3-Clause
// copyright-holders:0perator
#ifndef MAMEUI_LIB_UTIL_MUI_WCSTR_H
#define MAMEUI_LIB_UTIL_MUI_WCSTR_H

#pragma once

// standard C++ headers
#include <cctype>
#include <cstdint>
#include <cwctype>
#include <new>
#include <string>
#include <string_view>

#include "mui_strutil.h"

// mui_wcscpy
size_t mui_wcscpy(wchar_t *dst, const wchar_t *src);
wchar_t *mui_wcscpy(const wchar_t *src);

// mui_wcsncpy
size_t mui_wcsncpy(wchar_t *dst, const wchar_t *src, const size_t count);
wchar_t *mui_wcsncpy(const wchar_t *src, const size_t count);

// mui_wcschr
wchar_t *mui_wcschr(std::wstring &str, wchar_t delim);
wchar_t *mui_wcschr(std::wstring &str, std::wstring_view delim);
const wchar_t *mui_wcschr(std::wstring_view str, wchar_t delim);
const wchar_t *mui_wcschr(std::wstring_view str, std::wstring_view delim);

// mui_wcscmp
int mui_wcscmp(std::wstring_view s1, std::wstring_view s2);

// mui_wcsicmp
int mui_wcsicmp(std::wstring_view s1, std::wstring_view s2);

// mui_wcsncmp
int mui_wcsncmp(std::wstring_view s1, std::wstring_view s2, const size_t count);

// mui_wcsnicmp
int mui_wcsnicmp(std::wstring_view s1, std::wstring_view s2, const size_t count);

// mui_wsclen
size_t mui_wcsnlen(const wchar_t *src, const size_t max_count);

// mui_wsclen
size_t mui_wcslen(const wchar_t *src);

#endif // MAMEUI_LIB_UTIL_MUI_WCSTR_H
