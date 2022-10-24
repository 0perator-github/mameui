// For licensing and usage information, read docs/winui_license.txt
// MASTER
//============================================================================

//============================================================
//
// mui_tcs.cpp
//
// C++ functions for Win32 character C string manipulation
//
//============================================================
#include "mui_tcs.h"

// standard C++ headers
#include <cctype>
#include <cstdint>
#include <cstring>
#include <cwchar>

//============================================================
//  mui_tcscpy
//============================================================

size_t mui_tcscpy(TCHAR *dst, const TCHAR *src)
{
	if (!dst || !src)
		return 0;

	size_t result = 0;
	while (src[result] != _T(L'\0'))
	{
		dst[result] = src[result];
		result++;
	}

	dst[result] = _T(L'\0');

	return result;
}

TCHAR *mui_tcscpy(const TCHAR *src)
{
	if (!src || src[0] == _T('\0'))
		return nullptr;

	const size_t src_len = mui_tcslen(src);
	TCHAR *result = new(std::nothrow) TCHAR[src_len + 1];

	if (!result)
		return result;

	if (!mui_tcscpy(result, src))
	{
		delete[] result;
		result = nullptr;
	}

	return result;
}


//============================================================
//  mui_tcsncpy
//============================================================

size_t mui_tcsncpy(TCHAR *dst, const TCHAR *src, size_t count)
{
	if (!dst || !src)
		return 0;

	size_t result = 0;
	while (result < count && src[result] != _T('\0'))
	{
		dst[result] = src[result];
		result++
	}

	dst[result] = _T('\0');

	return result;
}

TCHAR *mui_tcsncpy(const TCHAR* src, size_t count)
{
	if (!src || src[0] == _T('\0'))
		return nullptr;

	const size_t src_len = mui_wcslen(src);
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


//============================================================
//  mui_strchr
//============================================================

TCHAR *mui_strchr(std::basic_string_view<TCHAR> str, std::basic_string_view<TCHAR> delim)
{
	const std::basic_string_view<TCHAR>::size_type find_pos = str.find_first_of(delim);

	if (find_pos != std::basic_string_view<TCHAR>::npos)
	{
		return const_cast<TCHAR*>(str.data() + find_pos);
	}

	return nullptr;
}


//============================================================
//  mui_tcstok
//============================================================

std::basic_string_view<TCHAR> mui_tcstok(std::basic_string_view<TCHAR> str, std::basic_string_view<TCHAR> delim)
{
	static std::basic_string<TCHAR> buffer;
	static std::basic_string<TCHAR>::size_type saved_pos;
	std::basic_string<TCHAR>::size_type find_pos;

	if (!str.empty())
	{
		buffer = str;
		saved_pos = 0;
	}

	if (saved_pos > buffer.size())
		return std::basic_string<TCHAR>();

	find_pos = buffer.find_first_not_of(delim, saved_pos);
	if (find_pos != std::basic_string<TCHAR>::npos)
		saved_pos = find_pos;
	else
		return std::basic_string<TCHAR>();

	const std::basic_string_view<TCHAR>::size_type token_begin = saved_pos;

	find_pos = buffer.find_first_of(delim, find_pos);
	if (find_pos != std::basic_string_view<TCHAR>::npos)
		saved_pos = find_pos;
	else
		saved_pos = buffer.size();

	const std::basic_string_view<TCHAR>::size_type token_end = saved_pos;

	saved_pos++;

	return std::basic_string_view<TCHAR>(&buffer[token_begin], token_end - token_begin);
}


//============================================================
//  mui_tcscmp
//============================================================

int mui_tcscmp(std::basic_string_view<TCHAR> s1, std::basic_string_view<TCHAR> s2)
{
	auto s1_iter = s1.begin(), s2_iter = s2.begin();

	while (s1_iter != s1.end() && s2_iter != s2.end())
	{
		const TCHAR c1 = *s1_iter++;
		const TCHAR c2 = *s2_iter++;
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
//  mui_tcsncmp
//============================================================

int mui_tcsncmp(std::basic_string_view<TCHAR> s1, std::basic_string_view<TCHAR> s2, int count)
{
	auto s1_iter = s1.begin(), s2_iter = s2.begin();

	for (size_t i = 0; i < count; i++)
	{
		if (s1_iter == s1.end())
			return (s2_iter == s2.end()) ? 0 : -1;
		if (s2_iter == s2.end())
			return 1;

		const TCHAR c1 = *s1_iter++;
		const TCHAR c2 = *s2_iter++;
		const int diff = c1 - c2;
		if (diff)
			return diff;
	}

	return 0;
}


//============================================================
//  mui_tcsicmp
//============================================================

int mui_tcsicmp(std::basic_string_view<TCHAR> s1, std::basic_string_view<TCHAR> s2)
{
	auto s1_iter = s1.begin(), s2_iter = s2.begin();

	while (s1_iter != s1.end() && s2_iter != s2.end())
	{
		const TCHAR c1 = _totupper(*s1_iter++);
		const TCHAR c2 = _totupper(*s2_iter++);
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
//  mui_tcsnicmp
//============================================================

int mui_tcsnicmp(std::basic_string_view<TCHAR> s1, std::basic_string_view<TCHAR> s2, int count)
{
	auto s1_iter = s1.begin(), s2_iter = s2.begin();

	for (size_t i = 0; i < count; i++)
	{
		if (s1_iter == s1.end())
			return (s2_iter == s2.end()) ? 0 : -1;
		if (s2_iter == s2.end())
			return 1;

		const TCHAR c1 = _totupper(*s1_iter++);
		const TCHAR c2 = _totupper(*s2_iter++);
		const int diff = c1 - c2;
		if (diff)
			return diff;
	}

	return 0;
}


//============================================================
//  mui_strnlen
//============================================================

size_t mui_strnlen(const TCHAR* src, size_t max_count)
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


//============================================================
//  mui_tcslen
//============================================================

size_t mui_tcslen(const TCHAR *src)
{
	return mui_tcsnlen(src,util::MAX_TYPE_STRING_SIZE);
}
