
// standard C++ headers
#include <cctype>
#include <cstdint>
#include <cstring>
#include <cwchar>

// MAME/MAMEUI headers
#include "mui_tcs.h"


//============================================================
//  mui_tcscpy
//============================================================

size_t mui_tcscpy(TCHAR *dst, const TCHAR *src)
{
	size_t result = 0;

	if (!src || src[0] == T('\0'))
		return result;

	for (; src[result] != T('\0'); result++)
		dst[result] = src[result];

	dst[result] = T('\0');

	return result;
}

TCHAR *mui_tcscpy(const TCHAR *src)
{
	if (!src || src[0] == _T('\0'))
		return 0;

	const size_t src_len = mui_tcslen(src);
	TCHAR *result = (!src_len) ? 0 : new TCHAR[src_len + 1];

	if (!result)
		return result;

	if (mui_tcscpy(result, src))
	{
		delete[] result;
		result = 0;
	}

	return result;
}


//============================================================
//  mui_tcsncpy
//============================================================

size_t mui_tcsncpy(TCHAR *dst, const TCHAR *src, size_t count)
{
	size_t result = 0;

	if (!src || src[0] == _T('\0'))
		return result;

	for (; result != count && src[result] != _T('\0'); result++)
		dst[result] = src[result];

	dst[result] = _T('\0');

	return result;
}

TCHAR *mui_tcsncpy(const TCHAR* src, size_t count)
{
	if (!src || src[0] == _T('\0'))
		return 0;

	const size_t src_len = mui_tcslen(src);
	TCHAR *result = (!src_len) ? 0 : new TCHAR[src_len + 1];

	if (!result)
		return result;

	if (!mui_tcsncpy(result, src, count))
	{
		delete[] result;
		result = 0;
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
		return const_cast<TCHAR*>(str.begin() + find_pos);
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
		return _T("");

	find_pos = buffer.find_first_not_of(delim, saved_pos);
	if (find_pos != std::wstring::npos)
		saved_pos = find_pos;
	else
		return _T("");

	const std::wstring_view::size_type token_begin = saved_pos;

	find_pos = buffer.find_first_of(delim, find_pos);
	if (find_pos != std::wstring::npos)
		saved_pos = find_pos;
	else
		saved_pos = buffer.size();

	const std::wstring_view::size_type token_end = saved_pos;

	saved_pos++;

	return buffer.substr(token_begin, token_end - token_begin);
}


//============================================================
//  mui_tcscmp
//============================================================

int mui_tcscmp(std::basic_string_view<TCHAR> s1, std::basic_string_view<TCHAR> s2)
{
	auto s1_iter = s1.begin(), s2_iter = s2.begin();

	while (true)
	{
		if (s1.end() == s1_iter)
			return (s2.end() == s2_iter) ? 0 : -1;
		else if (s2.end() == s2_iter)
			return 1;

		const int c1 = uint16_t(*s1_iter++);
		const int c2 = uint16_t(*s2_iter++);
		const int diff = c1 - c2;
		if (diff)
			return diff;
	}
}


//============================================================
//  mui_tcsncmp
//============================================================

int mui_tcsncmp(std::basic_string_view<TCHAR> s1, std::basic_string_view<TCHAR> s2, int count)
{
	auto s1_iter = s1.begin(), s2_iter = s2.begin();

	for (size_t i = 0; i < count; i++)
	{
		if (s1.size() == i)
			return (s2.size() == i) ? 0 : -1;
		else if (s2.size() == i)
			return 1;

		const int c1 = uint16_t(*s1_iter++);
		const int c2 = uint16_t(*s2_iter++);
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

	while (true)
	{
		if (s1.end() == s1_iter)
			return (s2.end() == s2_iter) ? 0 : -1;
		else if (s2.end() == s2_iter)
			return 1;

		const int c1 = toupper(uint16_t(*s1_iter++));
		const int c2 = toupper(uint16_t(*s2_iter++));
		const int diff = c1 - c2;
		if (diff)
			return diff;
	}
}


//============================================================
//  mui_tcsnicmp
//============================================================

int mui_tcsnicmp(std::basic_string_view<TCHAR> s1, std::basic_string_view<TCHAR> s2, int count)
{
	auto s1_iter = s1.begin(), s2_iter = s2.begin();

	for (size_t i = 0; i < count; i++)
	{
		if (s1.size() == i)
			return (s2.size() == i) ? 0 : -1;
		else if (s2.size() == i)
			return 1;

		const int c1 = toupper(uint16_t(*s1_iter++));
		const int c2 = toupper(uint16_t(*s2_iter++));
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

	if (max_count > SIZE_MAX)
		return result;

	if (src && src[0] != _T('\0'))
	{
		for (; src[result] != _T('\0'); result++)
		{
			if (result == max_count)
				break;
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
