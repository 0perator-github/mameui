// license:BSD-3-Clause
// copyright-holders:0perator
//============================================================

#ifndef MAMEUI_LIB_UTIL_MUI_WCSCONV_H
#define MAMEUI_LIB_UTIL_MUI_WCSCONV_H

#pragma once

#if 0 // these headers are needed when including this file
// standard C++ headers
#include <cstddef> // optional for size_t
#include <memory>
#include <new>
#include <string_view>
#include <string>

// standard windows headers
#include <stringapiset.h>

// MAMEUI headers
#include "mui_cstr.h"
#include "mui_wcstr.h"
#endif

namespace mameui::util::string_util
{
	// UTF-8 to UTF-16 conversions
	int mui_utf8_from_utf16string(std::string& utf8string, std::wstring_view utf16string, int wchar_count = -1);
	[[nodiscard]] std::string mui_utf8_from_utf16string(std::wstring_view utf16string, int wchar_count = -1);

	int mui_utf8_from_utf16cstring(char* utf8cstring, int byte_count, const wchar_t* utf16cstring, int wchar_count = -1);
	int mui_utf8_from_utf16cstring(char* utf8cstring, const wchar_t* utf16cstring, int wchar_count = -1);
	char* mui_utf8_from_utf16cstring(const wchar_t* utf16cstring, int wchar_count = -1);

	// UTF-16 to UTF-8 conversions
	int mui_utf16_from_utf8string(std::wstring& utf16string, std::string_view utf8string, int byte_count = -1);
	[[nodiscard]] std::wstring mui_utf16_from_utf8string(std::string_view utf8string, int byte_count = -1);

	int mui_utf16_from_utf8cstring(wchar_t* utf16cstring, int wchar_count, const char* utf8cstring, int byte_count = -1);
	int mui_utf16_from_utf8cstring(wchar_t* utf16cstring, const char* utf8cstring, int byte_count = -1);
	wchar_t* mui_utf16_from_utf8cstring(const char* utf8cstring, int byte_count = -1);
}

#endif // MAMEUI_LIB_UTIL_MUI_WCSCONV_H
