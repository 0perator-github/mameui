//	For licensing and usage information, read docs/winui_license.txt
//	MASTER
//	===========================================================================

//	===========================================================================
// mui_tcs.cpp - C++ functions for Win32 character C string manipulation
//	===========================================================================

// standard C++ headers
#include <cctype>
#include <cstdint>
#include <cwchar>
#include <new>
#include <string>
#include <string_view>

// standard windows headers
#include <tchar.h>

// MAMEUI headers
#include "mui_cstr.h" // for MUI_MAX_CHAR_COUNT

#include "mui_tcstr.h"

using namespace mameui::util::string_util;

//	-------------------------------------------------------
//	mui_tcscpy - copies a TCHAR string from src to dst
//	parameters:
//	TCHAR *dst - the destination string
//	const TCHAR *src - the source string
//	-------------------------------------------------------

size_t mameui::util::string_util::mui_tcscpy(TCHAR *dst, const TCHAR *src)
{
	if (!src || *src == _T('\0') || !dst)
		return 0;

	size_t result = 0;
	while (src[result] != _T('\0'))
	{
		if (tchar_diff_cs(dst[result], src[result]))
			dst[result] = src[result];

		result++;
	}

	dst[result] = _T('\0');

	return result;
}

//	-------------------------------------------------------
//	mui_tcscpy - copies a TCHAR string from src to a new allocated
//	string
//	parameters:
//	const TCHAR *src - the source string
//	-------------------------------------------------------

TCHAR *mameui::util::string_util::mui_tcscpy(const TCHAR *src)
{
	if (!src || src[0] == _T('\0'))
		return nullptr;

	const size_t src_len = mui_tcslen(src);
	char* result = new(std::nothrow) TCHAR[src_len + 1];

	if (!result)
		return result;

	if (!mui_tcscpy(result, src))
	{
		delete[] result;
		result = nullptr;
	}

	return result;
}

//	-------------------------------------------------------
//	mui_tcsncpy - copies a TCHAR string from src to dst, up to a specified count
//	parameters:
//	TCHAR *dst - the destination string
// 	const TCHAR *src - the source string
//	const size_t count - the maximum number of characters to copy
//============================================================

size_t mameui::util::string_util::mui_tcsncpy(TCHAR *dst, const TCHAR *src, const size_t count)
{
	if (!src || *src == _T('\0') || !dst)
		return 0;

	size_t result = 0;
	while (result < count && src[result] != _T('\0'))
	{
		if (tchar_diff_cs(dst[result], src[result]))
			dst[result] = src[result];

		result++;
	}

	dst[result] = _T('\0');

	return result;
}

//	-------------------------------------------------------
//	mui_tcsncpy - copies a TCHAR string from src to a new allocated
//	string, up to a specified count
//	parameters:
//	const TCHAR *src - the source string
//	const size_t count - the maximum number of characters to copy
//	-------------------------------------------------------

TCHAR *mameui::util::string_util::mui_tcsncpy(const TCHAR* src, const size_t count)
{
	if (!src || src[0] == _T('\0'))
		return nullptr;

	const size_t src_len = mui_tcslen(src);
	TCHAR *result = (!src_len) ? 0 : new(std::nothrow) TCHAR[src_len + 1];

	if (!result)
		return result;

	if (!mui_tcsncpy(result, src, count))
	{
		delete[] result;
		result = nullptr;
	}

	return result;
}

//	-------------------------------------------------------
//	mui_tcschr - finds the first occurrence of a character in a TCHAR string
//	parameters:
//	tstring &str - the string to search
//	TCHAR delim - the character to find
//	-------------------------------------------------------

TCHAR *mameui::util::string_util::mui_tcschr(tstring &str, TCHAR delim)
{
	auto pos = str.find(delim);
	return (pos != tstring::npos && !str.empty()) ? &str[0] + pos : nullptr;
}

//	-------------------------------------------------------
//	mui_tcschr - finds the first occurrence of a substring in a TCHAR string
//	parameters:
//	tstring &str - the string to search
//	tstring_view delim - the substring to find
//	-------------------------------------------------------

TCHAR *mameui::util::string_util::mui_tcschr(tstring &str, tstring_view delim)
{
	auto pos = str.find(delim);
	return (pos != tstring::npos && !str.empty()) ? &str[0] + pos : nullptr;
}

//	-------------------------------------------------------
//	mui_tcschr - finds the first occurrence of a character in a TCHAR string
//	parameters:
//	tstring_view str - the string to search
//	TCHAR delim - the character to find
//	-------------------------------------------------------

const TCHAR *mameui::util::string_util::mui_tcschr(tstring_view str, TCHAR delim)
{
	auto pos = str.find(delim);
	return (pos != tstring_view::npos && !str.empty()) ? str.data() + pos : nullptr;
}

//	-------------------------------------------------------
//	mui_tcschr - finds the first occurrence of a substring in a TCHAR string
//	parameters:
//	tstring_view str - the string to search
//	tstring_view delim - the substring to find
//	-------------------------------------------------------

const TCHAR *mameui::util::string_util::mui_tcschr(tstring_view str, tstring_view delim)
{
	auto pos = str.find(delim);
	return (pos != tstring_view::npos && !str.empty()) ? str.data() + pos : nullptr;
}

//	-------------------------------------------------------
//	mui_tcscmp - compares two TCHAR strings
//	parameters:
//	tstring_view s1 - the first string to compare
//	tstring_view s2 - the second string to compare
//	-------------------------------------------------------

int mameui::util::string_util::mui_tcscmp(tstring_view s1, tstring_view s2)
{
	const size_t s1_size = s1.size(), s2_size = s2.size();

	size_t index = 0;
	while (index < s1_size && index < s2_size)
	{
		int diff = tchar_diff_cs(s1[index], s2[index]);
		if (diff)
			return diff;

		index++;
	}

	if (index == s1_size && index == s2_size)
		return 0;
	else if (index == s1_size)
		return -1;
	else
		return 1;
}

//	-------------------------------------------------------
//	mui_tcsncmp - compares two TCHAR strings up to a specified number of characters
//	parameters:
//	tstring_view s1 - the first string to compare
//	tstring_view s2 - the second string to compare
//	const size_t count - the maximum number of characters to compare
//	-------------------------------------------------------

int mameui::util::string_util::mui_tcsncmp(tstring_view s1, tstring_view s2, const size_t count)
{
	const size_t s1_size = s1.size(), s2_size = s2.size();

	size_t index = 0;
	while (index < count && index < s1_size && index < s2_size)
	{
		int diff = tchar_diff_cs(s1[index], s2[index]);
		if (diff)
			return diff;

		index++;
	}

	if (index == count)
		return 0;

	if (index == s1_size && index == s2_size)
		return 0;
	else if (index == s1_size)
		return -1;
	else
		return 1;
}

//	-------------------------------------------------------
//	mui_tcsicmp - compares two TCHAR strings case-insensitively
//	parameters:
//	tstring_view s1 - the first string to compare
//	tstring_view s2 - the second string to compare
//	-------------------------------------------------------

int mameui::util::string_util::mui_tcsicmp(tstring_view s1, tstring_view s2)
{
	const size_t s1_size = s1.size(), s2_size = s2.size();

	size_t index = 0;
	while (index < s1_size && index < s2_size)
	{
		int diff = tchar_diff_ci(s1[index], s2[index]);
		if (diff)
			return diff;

		index++;
	}

	if (index == s1_size && index == s2_size)
		return 0;
	else if (index == s1_size)
		return -1;
	else
		return 1;
}

//	-------------------------------------------------------
//	mui_tcsnicmp - compares two TCHAR strings case-insensitively up to
// a specified number of characters
// parameters:
// tstring_view s1 - the first string to compare
// tstring_view s2 - the second string to compare
// const size_t count - the maximum number of characters to compare
//	-------------------------------------------------------

int mameui::util::string_util::mui_tcsnicmp(tstring_view s1, tstring_view s2, const size_t count)
{
	const size_t s1_size = s1.size(), s2_size = s2.size();

	size_t index = 0;
	while (index < count && index < s1_size && index < s2_size)
	{
		int diff = tchar_diff_ci(s1[index], s2[index]);
		if (diff)
			return diff;

		index++;
	}

	if (index == count)
		return 0;

	if (index == s1_size && index == s2_size)
		return 0;
	else if (index == s1_size)
		return -1;
	else
		return 1;
}

//	-------------------------------------------------------
//	mui_tcsnlen - returns the length of a TCHAR string up to
//	a specified maximum count
//	parameters:
//	const TCHAR *src - the string to measure
//	const size_t max_count - the maximum number of characters to measure
//	-------------------------------------------------------

size_t mameui::util::string_util::mui_tcsnlen(const TCHAR* src, const size_t max_count)
{
	size_t result = 0;

	if (src)
	{
		while (result < max_count && src[result] != _T('\0'))
		{
			result++;
		}
	}

	return result;
}

//	-------------------------------------------------------
//	mui_tcslen - returns the length of a TCHAR string
//	parameters:
//	const TCHAR *src - the string to measure
//	-------------------------------------------------------

size_t mameui::util::string_util::mui_tcslen(const TCHAR *src)
{
	return mui_tcsnlen(src, MUI_MAX_CHAR_COUNT);
}
