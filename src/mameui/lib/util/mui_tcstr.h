// license:BSD-3-Clause
// copyright-holders:0perator
#ifndef MAMEUI_LIB_UTIL_MUI_TCSTR_H
#define MAMEUI_LIB_UTIL_MUI_TCSTR_H

#pragma once

#include "mui_strutil.h"

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

#endif // MAMEUI_LIB_UTIL_MUI_TCSTR_H
