// license:BSD-3-Clause
// copyright-holders:0perator
#ifndef MAMEUI_LIB_UTIL_MUI_CSTR_H
#define MAMEUI_LIB_UTIL_MUI_CSTR_H

#pragma once

#if 0 // these headers are needed when including this file
// standard C++ headers
#include <cctype>
#include <cstdint>
#include <cwctype>
#include <new>
#include <string>
#include <string_view>

// mameui headers
#include "mui_strutil.h"
#endif


// mui_strcpy
size_t mui_strcpy(char *dst, const char *src);
char *mui_strcpy(const char *src);

// mui_strncpy
size_t mui_strncpy(char *dst, const char *src, const size_t count);
char *mui_strncpy(const char *src, const size_t count);

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

#endif // MAMEUI_LIB_UTIL_MUI_CSTR_H
