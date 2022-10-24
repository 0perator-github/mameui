//	For licensing and usage information, read docs/winui_license.txt
//	MASTER
//	===========================================================================

//	===========================================================================
//	mui_wcs.cpp - C++ functions for wide character C string manipulation
//	===========================================================================

// standard C++ headers
#include <cctype>
#include <cstdint>
#include <cwctype>
#include <new>
#include <string>
#include <string_view>

// MAMEUI headers
#include "mui_cstr.h" // for MUI_MAX_CHAR_COUNT

#include "mui_wcstr.h"

using namespace mameui::util::string_util;

//	-------------------------------------------------------
//	mui_wcscpy - copies a wide character string from src to dst
//	parameters:
//	wchar_t *dst - destination string
//	const wchar_t *src - source string
//	-------------------------------------------------------

size_t mameui::util::string_util::mui_wcscpy(wchar_t *dst, const wchar_t *src)
{
	if (!src || *src == L'\0' || !dst)
		return 0;

	size_t result = 0;
	while (src[result] != L'\0')
	{
		if (wchar_diff_cs(dst[result], src[result]))
			dst[result] = src[result];

		result++;
	}

	dst[result] = L'\0';

	return result;
}

//	-------------------------------------------------------
//	mui_wcscpy - copies a wide character string from src to a new allocated
//	string
//	parameters:
//	const wchar_t *src - source string
//	-------------------------------------------------------

wchar_t *mameui::util::string_util::mui_wcscpy(const wchar_t *src)
{
	if (!src || src[0] == L'\0')
		return nullptr;

	const size_t src_len = mui_wcslen(src);
	wchar_t* result = new(std::nothrow) wchar_t[src_len + 1];

	if (!result)
		return result;

	if(!mui_wcscpy(result, src))
	{
		delete[] result;
		result = nullptr;
	}

	return result;
}

//	-------------------------------------------------------
//	mui_wcsncpy - copies a wide character string from src to dst, up to
//	a number of characters
//	parameters:
//	wchar_t *dst - destination string
//	const wchar_t *src - source string
//	const size_t count - maximum number of characters to copy
//	-------------------------------------------------------

size_t mameui::util::string_util::mui_wcsncpy(wchar_t *dst, const wchar_t *src, const size_t count)
{
	if (!src || *src == L'\0' || !dst)
		return 0;

	size_t result = 0;
	while (result < count && src[result] != L'\0')
	{
		if (wchar_diff_cs(dst[result], src[result]))
			dst[result] = src[result];

		result++;
	}

	dst[result] = L'\0';

	return result;
}

//	-------------------------------------------------------
//	mui_wcsncpy - copies a wide character string from src to a new allocated
//	string, up to count characters
//	parameters:
//	const wchar_t *src - source string
//	const size_t count - maximum number of characters to copy
//	-------------------------------------------------------

wchar_t *mameui::util::string_util::mui_wcsncpy(const wchar_t* src, const size_t count)
{
	if (!src || src[0] == L'\0')
		return nullptr;

	const size_t src_len = mui_wcslen(src);
	wchar_t* result = (!src_len) ? 0 : new(std::nothrow) wchar_t[src_len + 1];

	if (!result)
		return result;

	if(!mui_wcsncpy(result, src, count))
	{
		delete[] result;
		result = nullptr;
	}

	return result;
}

//	-------------------------------------------------------
//	mui_wcschr - finds the first occurrence of a character in a
//	wide character string
//	parameters:
//	std::wstring & str - the string to search
//	wchar_t delim - the character to find
//	-------------------------------------------------------

wchar_t *mameui::util::string_util::mui_wcschr(std::wstring & str, wchar_t delim)
{
	auto pos = str.find(delim);
	return (pos != std::wstring::npos && !str.empty()) ? &str[0] + pos : nullptr;
}

//	-------------------------------------------------------
//	mui_wcschr - finds the first occurrence of a character in a
//	wide character string
//	parameters:
//	std::wstring_view str - the string to search
//	wchar_t delim - the character to find
//	-------------------------------------------------------

wchar_t *mameui::util::string_util::mui_wcschr(std::wstring &str, std::wstring_view delim)
{
	auto pos = str.find(delim);
	return (pos != std::wstring::npos && !str.empty()) ? &str[0] + pos : nullptr;
}

//	-------------------------------------------------------
//	mui_wcschr - finds the first occurrence of a character in a
//	wide character string
//	parameters:
//	std::wstring_view str - the string to search
//	wchar_t delim - the character to find
//	-------------------------------------------------------

const wchar_t *mameui::util::string_util::mui_wcschr(std::wstring_view str, wchar_t delim)
{
	auto pos = str.find(delim);
	return (pos != std::wstring_view::npos && !str.empty()) ? str.data() + pos : nullptr;
}

//	-------------------------------------------------------
//	mui_wcschr - finds the first occurrence of a character in a
//	wide character string
//	parameters:
//	std::wstring_view str - the string to search
//	wchar_t delim - the character to find
//	-------------------------------------------------------

const wchar_t *mameui::util::string_util::mui_wcschr(std::wstring_view str, std::wstring_view delim)
{
	auto pos = str.find(delim);
	return (pos != std::wstring_view::npos && !str.empty()) ? str.data() + pos : nullptr;
}

//	-------------------------------------------------------
//	mui_wcscmp - compares two wide character strings
//	parameters:
//	std::wstring_view s1 - first string
//	std::wstring_view s2 - second string
//	-------------------------------------------------------

int mameui::util::string_util::mui_wcscmp(std::wstring_view s1, std::wstring_view s2)
{
	const size_t s1_size = s1.size(), s2_size = s2.size();

	size_t index = 0;
	while (index < s1_size && index < s2_size)
	{
		int diff = wchar_diff_cs(s1[index], s2[index]);
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
//	mui_wcsncmp - compares two wide character strings up to a specified count
//	parameters:
//	std::wstring_view s1 - first string
//	std::wstring_view s2 - second string
//	const size_t count - maximum number of characters to compare
//	-------------------------------------------------------

int mameui::util::string_util::mui_wcsncmp(std::wstring_view s1, std::wstring_view s2, const size_t count)
{
	const size_t s1_size = s1.size(), s2_size = s2.size();

	size_t index = 0;
	while (index < count && index < s1_size && index < s2_size)
	{
		int diff = wchar_diff_cs(s1[index], s2[index]);
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
//	mui_wcsicmp - compares two wide character strings case-insensitively
//	parameters:
//	std::wstring_view s1 - first string
//	std::wstring_view s2 - second string
//	-------------------------------------------------------

int mameui::util::string_util::mui_wcsicmp(std::wstring_view s1, std::wstring_view s2)
{
	const size_t s1_size = s1.size(), s2_size = s2.size();

	size_t index = 0;
	while (index < s1_size && index < s2_size)
	{
		int diff = wchar_diff_ci(s1[index], s2[index]);
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
//	mui_wcsnicmp - compares two wide character strings case-insensitively up to a specified count
//	parameters:
//	std::wstring_view s1 - first string
//	std::wstring_view s2 - second string
//	const size_t count - maximum number of characters to compare
//	-------------------------------------------------------

int mameui::util::string_util::mui_wcsnicmp(std::wstring_view s1, std::wstring_view s2, const size_t count)
{
	const size_t s1_size = s1.size(), s2_size = s2.size();

	size_t index = 0;
	while (index < count && index < s1_size && index < s2_size)
	{
		int diff = wchar_diff_ci(s1[index], s2[index]);
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
//	mui_wcsnlen - gets the length of a wide character string up to a specified count
//	parameters:
//	const wchar_t* src - the string to measure
//	const size_t max_count - maximum number of characters to consider
//	-------------------------------------------------------

size_t mameui::util::string_util::mui_wcsnlen(const wchar_t* src, const size_t max_count)
{
	size_t result = 0;

	if (src)
	{
		while (result < max_count && src[result] != L'\0')
		{
			result++;
		}
	}

	return result;
}

//	-------------------------------------------------------
//	mui_wcslen - gets the length of a wide character string
//	parameters:
//	const wchar_t* src - the string to measure
//	-------------------------------------------------------

size_t mameui::util::string_util::mui_wcslen(const wchar_t *src)
{
	return mui_wcsnlen(src, MUI_MAX_CHAR_COUNT);
}
