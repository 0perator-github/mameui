// license:BSD-3-Clause
// copyright-holders:0perator
#ifndef MAMEUI_WINAPP_MUI_WCS_H
#define MAMEUI_WINAPP_MUI_WCS_H

#pragma once

// standard C++ headers
#include <string>
#include <string_view>

namespace util
{
	constexpr size_t MAX_WIDE_STRING_SIZE = 1024;
}

// mui_wcscpy
size_t mui_wcscpy(wchar_t *dst, const wchar_t *src);
wchar_t *mui_wcscpy(const wchar_t *src);

// mui_wcsncpy
size_t mui_wcsncpy(wchar_t *dst, const wchar_t *src, const size_t count);
wchar_t *mui_wcsncpy(const wchar_t *src, const size_t count);

// mui_wcschr
wchar_t *mui_wcschr(std::wstring_view str, std::wstring_view delim);

// mui_wcstok
std::wstring_view mui_wcstok(std::wstring_view str, std::wstring_view delim);
//wchar_t *mui_wcstok(wchar_t* str, const wchar_t* delim);

// mui_wcscmp
int mui_wcscmp(std::wstring_view s1, std::wstring_view s2);

// mui_wcsicmp
int mui_wcsicmp(std::wstring_view s1, std::wstring_view s2);

// mui_wcsncmp
int mui_wcsncmp(std::wstring_view s1, std::wstring_view s2, int count);

// mui_wcsnicmp
int mui_wcsnicmp(std::wstring_view s1, std::wstring_view s2, int count);

// mui_wsclen
size_t mui_wcsnlen(const wchar_t *src, const size_t max_count);

// mui_wsclen
size_t mui_wcslen(const wchar_t *src);

#endif // MAMEUI_WINAPP_MUI_WCS_H
