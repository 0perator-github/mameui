// license:BSD-3-Clause
// copyright-holders:0perator

#ifndef MAMEUI_WINAPP_MUI_TCSCONV_H
#define MAMEUI_WINAPP_MUI_TCSCONV_H

#pragma once

// mui_tcstring_from_utf8
size_t mui_tcstring_from_utf8(TCHAR *, size_t, const char *);
size_t mui_tcstring_from_utf8(TCHAR *, const char *);
TCHAR *mui_tcstring_from_utf8(const char *);

// mui_tcstring_from_wcstring
size_t mui_tcstring_from_wcstring(TCHAR *, size_t, const wchar_t *);
size_t mui_tcstring_from_wcstring(TCHAR *, const wchar_t *);
TCHAR *mui_tcstring_from_wcstring(const wchar_t *);

// mui_utf8_from_tcstring
size_t mui_utf8_from_tcstring(char *, size_t, const TCHAR *);
size_t mui_utf8_from_tcstring(char *, const TCHAR *);
char *mui_utf8_from_tcstring(const TCHAR *);

#endif // MAMEUI_WINAPP_MUI_TCSCONV_H
