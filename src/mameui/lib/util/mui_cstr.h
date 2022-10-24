// license:BSD-3-Clause
// copyright-holders:0perator
#ifndef MAMEUI_LIB_UTIL_MUI_CSTR_H
#define MAMEUI_LIB_UTIL_MUI_CSTR_H

#pragma once

#if 0 // these headers are needed when including this file
// standard C++ headers
#include <cctype>
#include <cstddef> // optional for size_t
#include <cstdint>
#include <new>
#include <string>
#include <string_view>
#endif

namespace mameui::util::string_util
{
	// this is the max buffer size defined in strsafe.h for either ANSI or Unicode strings.
	// incidently, this is also the maximum charater count for an Oracle Database CLOB.
	inline constexpr size_t MUI_MAX_CHAR_COUNT = 2147483647; // Maximum character count for C string operations

	// char_diff
	inline int char_diff_cs(char c1, char c2) { return  static_cast<uint8_t>(c1) - static_cast<uint8_t>(c2); }
	inline int char_diff_ci(char c1, char c2) { return  toupper(static_cast<uint8_t>(c1)) - toupper(static_cast<uint8_t>(c2)); }

	// mui_strcpy
	size_t mui_strcpy(char* dst, const char* src);
	char *mui_strcpy(const char* src);

	// mui_strncpy
	size_t mui_strncpy(char* dst, const char* src, const size_t count);
	char *mui_strncpy(const char* src, const size_t count);

	// mui_strchr
	char *mui_strchr(std::string &str, char delim);
	char *mui_strchr(std::string &str, std::string_view delim);
	const char *mui_strchr(std::string_view str, char delim);
	const char *mui_strchr(std::string_view str, std::string_view delim);

	// mui_strcmp
	int mui_strcmp(std::string_view s1, std::string_view s2);

	// mui_stricmp
	int mui_stricmp(std::string_view s1, std::string_view s2);

	// mui_strncmp
	int mui_strncmp(std::string_view s1, std::string_view s2, const size_t count);

	// mui_strnicmp
	int mui_strnicmp(std::string_view s1, std::string_view s2, const size_t count);

	// mui_strnlen
	size_t mui_strnlen(const char *src, const size_t max_count);

	// mui_strlen
	size_t mui_strlen(const char *src);
}

#endif // MAMEUI_LIB_UTIL_MUI_CSTR_H
