
// standard C++ headers
#include <cctype>
#include <cstdint>
#include <algorithm>

// MAME/MAMEUI headers
#include "mui_wcs.h"

//============================================================
//  mui_wcscpy
//============================================================

size_t mui_wcscpy(wchar_t *dst, const wchar_t *src)
{
	size_t result = 0;

	if (!src || src[0] == L'\0')
		return result;

	for (; src[result] != L'\0'; result++)
		dst[result] = src[result];

	dst[result] = L'\0';

	return result;
}

wchar_t *mui_wcscpy(const wchar_t *src)
{
	if (!src || src[0] == L'\0')
		return 0;

	const size_t src_len = mui_wcslen(src);
	wchar_t* result = (!src_len) ? 0 : new wchar_t[src_len + 1];

	if (!result)
		return result;

	if(!mui_wcscpy(result, src))
	{
		delete[] result;
		result = 0;
	}

	return result;
}


//============================================================
//  mui_wcsncpy
//============================================================

size_t mui_wcsncpy(wchar_t *dst, const wchar_t *src, const size_t count)
{
	size_t result = 0;

	if (!src || src[0] == L'\0')
		return result;

	for (; result != count && src[result] != L'\0'; result++)
		dst[result] = src[result];

	dst[result] = '\0';

	return result;
}

wchar_t *mui_wcsncpy(const wchar_t *src, const size_t count)
{
	if (!src || src[0] == L'\0')
		return 0;

	const size_t src_len = mui_wcslen(src);
	wchar_t* result = (!src_len) ? 0 : new wchar_t[src_len + 1];

	if (!result)
		return result;

	if(!mui_wcsncpy(result, src, count))
	{
		delete[] result;
		result = 0;
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
		return const_cast<wchar_t*>(str.begin() + find_pos);
	}

	return nullptr;
}


//============================================================
//  mui_wcstok
//============================================================

std::wstring_view mui_wcstok(std::wstring_view str, std::wstring_view delim)
{
	static std::wstring buffer;
	static std::string::size_type saved_pos;
	std::wstring::size_type find_pos;

	if (!str.empty())
	{
		buffer = &str[0];
		saved_pos = 0;
	}

	if (saved_pos > buffer.size())
		return L"";

	find_pos = buffer.find_first_not_of(delim, saved_pos);
	if (find_pos != std::wstring::npos)
		saved_pos = find_pos;
	else
		return L"";

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

//wchar_t* mui_wcstok(wchar_t* str, const wchar_t* delim)
//{
//	return wcstok(str,delim);
//}


//============================================================
//  mui_wcscmp
//============================================================

int mui_wcscmp(std::wstring_view s1, std::wstring_view s2)
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
//  mui_wcsncmp
//============================================================

int mui_wcsncmp(std::wstring_view s1, std::wstring_view s2, int count)
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
//  mui_wcsicmp
//============================================================

int mui_wcsicmp(std::wstring_view s1, std::wstring_view s2)
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
//  mui_wcsnicmp
//============================================================

int mui_wcsnicmp(std::wstring_view s1, std::wstring_view s2, int count)
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
//  mui_wscnlen
//============================================================

size_t mui_wcsnlen(const wchar_t* src, const size_t max_count)
{
	size_t result = 0;

	if (max_count > SIZE_MAX)
		return result;

	if (src && src[0] != L'\0')
	{
		for (; src[result] != L'\0'; result++)
		{
			if (result == max_count)
				break;
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
