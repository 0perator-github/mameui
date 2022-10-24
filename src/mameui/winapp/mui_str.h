// license:BSD-3-Clause
// copyright-holders:0perator
#ifndef MAMEUI_WINAPP_MUI_STR_H
#define MAMEUI_WINAPP_MUI_STR_H

#pragma once

// standard C++ headers
#include <memory>
#include <string>
#include <string_view>

namespace util
{
	constexpr size_t MAX_STRING_SIZE = 2048;
}

// mui_strcpy
size_t mui_strcpy(char *dst, const char *src);
char *mui_strcpy(const char *src);

// mui_strncpy
size_t mui_strncpy(char *dst, const char *src, const size_t count);
char *mui_strncpy(const char *src, const size_t count);

// mui_strchr
char* mui_strchr(std::string_view str, std::string_view delim);

// mui_strtok
std::string mui_strtok(std::string_view str, std::string_view delim);

// mui_strcmp
int mui_strcmp(std::string_view s1, std::string_view s2);

// mui_stricmp
int mui_stricmp(std::string_view s1, std::string_view s2);

// mui_strncmp
int mui_strncmp(std::string_view s1, std::string_view s2, int count);

// mui_strnicmp
int mui_strnicmp(std::string_view s1, std::string_view s2, int count);

// mui_strnlen
size_t mui_strnlen(const char *src, const size_t max_count);

// mui_strlen
size_t mui_strlen(const char *src);

#endif // MAMEUI_WINAPP_MUI_STR_H
