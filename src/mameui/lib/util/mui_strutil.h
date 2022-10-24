// license:BSD-3-Clause
// copyright-holders:0perator
#ifndef MAMEUI_LIB_UTIL_MUI_STRUTIL_H
#define MAMEUI_LIB_UTIL_MUI_STRUTIL_H

#pragma once

#if 0 // these headers are required when including this header file
// standard C++ headers
#include <cctype>
#include <cstdint>
include <cwctype>
#endif

namespace mameui::util
{
	// this is the max buffer size defined in strsafe.h for either ANSI or Unicode strings.
	// incidently, this is also the maximum charater count for an Oracle Database CLOB.
	constexpr size_t MUI_MAX_CHAR_COUNT = 2147483647;

	// char_diff
	int char_diff_cs(char c1, char c2) { return  static_cast<uint8_t>(c1) - static_cast<uint8_t>(c2); };
	int char_diff_ci(char c1, char c2) { return  toupper(static_cast<uint8_t>(c1)) - toupper(static_cast<uint8_t>(c2)); };

	// wchar_diff
	int wchar_diff_cs(wchar_t c1, wchar_t c2) { return  c1 - c2; };
	int wchar_diff_ci(wchar_t c1, wchar_t c2) { return  towupper(c1) - towupper(c2); };

	// tchar_diff
#if defined(UNICODE) || defined(_UNICODE)
	int tchar_diff_cs(wchar_t c1, wchar_t c2) { return  c1 - c2; };
	int tchar_diff_ci(wchar_t c1, wchar_t c2) { return  towupper(c1) - towupper(c2); };
#else
	int tchar_diff_cs(char c1, char c2) { return  static_cast<uint8_t>(c1) - static_cast<uint8_t>(c2); };
	int tchar_diff_ci(char c1, char c2) { return  toupper(static_cast<uint8_t>(c1)) - toupper(static_cast<uint8_t>(c2)); };
#endif // UNICODE
}

#endif // MAMEUI_LIB_UTIL_MUI_STRUTIL_H
