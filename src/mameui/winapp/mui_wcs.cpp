// For licensing and usage information, read docs/winui_license.txt
// MASTER
//============================================================================

//============================================================
//
// mui_wcs.cpp
//
// C++ functions for wide character C string manipulation
//
//============================================================
#include "mui_wcs.h"

// standard C++ headers
#include <cctype>
#include <cstdint>
#include <cwctype>
#include <algorithm>

//============================================================
//  mui_wcscpy
//============================================================

size_t mui_wcscpy(wchar_t *dst, const wchar_t *src)
{
	if (!dst || !src)
		return 0;

	size_t result = 0;
	while (src[result] != L'\0')
	{
		dst[result] = src[result];
		result++;
	}

	dst[result] = L'\0';

	return result;
}

wchar_t *mui_wcscpy(const wchar_t *src)
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


//============================================================
//  mui_wcsncpy
//============================================================

size_t mui_wcsncpy(wchar_t *dst, const wchar_t *src, const size_t count)
{
	if (!dst || !src)
		return 0;

	size_t result = 0;
	while (result < count && src[result] != L'\0')
	{
		dst[result] = src[result];
		result++;
	}

	dst[result] = L'\0';

	return result;
}

wchar_t *mui_wcsncpy(const wchar_t *src, const size_t count)
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


//============================================================
//  mui_wcschr
//============================================================

wchar_t *mui_wcschr(std::wstring_view str, std::wstring_view delim)
{
	const std::wstring_view::size_type find_pos = str.find_first_of(delim);

	if (find_pos != std::wstring_view::npos)
	{
		return const_cast<wchar_t*>(str.data() + find_pos);
	}

	return nullptr;
}


//============================================================
//  mui_wcstok
//============================================================

std::wstring_view mui_wcstok(std::wstring_view str, std::wstring_view delim)
{
	static std::wstring buffer;
	static std::wstring::size_type saved_pos;
	std::wstring::size_type find_pos;

	if (!str.empty())
	{
		buffer = str;
		saved_pos = 0;
	}

	if (saved_pos > buffer.size())
		return std::wstring_view();

	find_pos = buffer.find_first_not_of(delim, saved_pos);
	if (find_pos != std::wstring::npos)
		saved_pos = find_pos;
	else
		return std::wstring_view();

	const std::wstring_view::size_type token_begin = saved_pos;

	find_pos = buffer.find_first_of(delim, find_pos);
	if (find_pos != std::wstring::npos)
		saved_pos = find_pos;
	else
		saved_pos = buffer.size();

	const std::wstring_view::size_type token_end = saved_pos;

	saved_pos++;

	return std::wstring_view(&buffer[token_begin], token_end - token_begin);
}

//============================================================
//  mui_wcscmp
//============================================================

int mui_wcscmp(std::wstring_view s1, std::wstring_view s2)
{
	auto s1_iter = s1.begin(), s2_iter = s2.begin();

	while (s1_iter != s1.end() && s2_iter != s2.end())
	{
		const wchar_t c1 = *s1_iter++;
		const wchar_t c2 = *s2_iter++;
		const int diff = c1 - c2;
		if (diff)
			return diff;
	}

	if (s1_iter == s1.end() && s2_iter == s2.end())
		return 0;
	else if (s1_iter == s1.end())
		return -1;
	else
		return 1;
}


//============================================================
//  mui_wcsncmp
//============================================================

int mui_wcsncmp(std::wstring_view s1, std::wstring_view s2, int count)
{
	auto s1_iter = s1.begin(), s2_iter = s2.begin();

	for (size_t i = 0; i < count; i++)
	{
		if (s1_iter == s1.end())
			return (s2_iter == s2.end()) ? 0 : -1;
		if (s2_iter == s2.end())
			return 1;

		const wchar_t c1 = *s1_iter++;
		const wchar_t c2 = *s2_iter++;
		const int diff = c1 - c2;
		if (diff)
			return diff;
	}

	return 0;
}


//============================================================
//  mui_wcsicmp
//============================================================

int mui_wcsicmp(std::wstring_view s1, std::wstring_view s2)
{
	auto s1_iter = s1.begin(), s2_iter = s2.begin();

	while (s1_iter != s1.end() && s2_iter != s2.end())
	{
		const wchar_t c1 = std::towupper(*s1_iter++);
		const wchar_t c2 = std::towupper(*s2_iter++);
		const int diff = c1 - c2;
		if (diff)
			return diff;
	}

	if (s1_iter == s1.end() && s2_iter == s2.end())
		return 0;
	else if (s1_iter == s1.end())
		return -1;
	else
		return 1;
}


//============================================================
//  mui_wcsnicmp
//============================================================

int mui_wcsnicmp(std::wstring_view s1, std::wstring_view s2, int count)
{
	auto s1_iter = s1.begin(), s2_iter = s2.begin();

	for (size_t i = 0; i < count; i++)
	{
		if (s1_iter == s1.end())
			return (s2_iter == s2.end()) ? 0 : -1;
		if (s2_iter == s2.end())
			return 1;

		const wchar_t c1 = std::towupper(*s1_iter++);
		const wchar_t c2 = std::towupper(*s2_iter++);
		const int diff = c1 - c2;
		if (diff)
			return diff;
	}

	return 0;
}


//============================================================
//  mui_wscnlen
//============================================================

size_t mui_wcsnlen(const wchar_t* src, const size_t max_count)
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


//============================================================
//  mui_wsclen
//============================================================

size_t mui_wcslen(const wchar_t *src)
{
	return mui_wcsnlen(src, util::MAX_WIDE_STRING_SIZE - 1);
}
