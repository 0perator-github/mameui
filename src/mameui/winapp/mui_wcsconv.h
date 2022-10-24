// license:BSD-3-Clause
// copyright-holders:0perator

#ifndef MAMEUI_WINAPP_MUI_WCSCONV_H
#define MAMEUI_WINAPP_MUI_WCSCONV_H

#pragma once

// mui_utf8_from_wcstring
int mui_utf8_from_wcstring(char *, int, const wchar_t *);
int mui_utf8_from_wcstring(char *, const wchar_t *);
char *mui_utf8_from_wcstring(const wchar_t *);

// mui_wcstring_from_utf8
int mui_wcstring_from_utf8(wchar_t *, int, const char *);
int mui_wcstring_from_utf8(wchar_t *, const char *);
wchar_t *mui_wcstring_from_utf8(const char *);

#endif // MAMEUI_WINAPP_MUI_WCSCONV_H
