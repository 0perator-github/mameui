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

//============================================================
//  mui_wcscpy
//============================================================

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


//============================================================
//  mui_wcsncpy
//============================================================

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


//============================================================
//  mui_wcschr
//============================================================

wchar_t *mameui::util::string_util::mui_wcschr(std::wstring & str, wchar_t delim)
{
	auto pos = str.find(delim);
	return (pos != std::wstring::npos && !str.empty()) ? &str[0] + pos : nullptr;
}

wchar_t *mameui::util::string_util::mui_wcschr(std::wstring &str, std::wstring_view delim)
{
	auto pos = str.find(delim);
	return (pos != std::wstring::npos && !str.empty()) ? &str[0] + pos : nullptr;
}

const wchar_t *mameui::util::string_util::mui_wcschr(std::wstring_view str, wchar_t delim)
{
	auto pos = str.find(delim);
	return (pos != std::wstring_view::npos && !str.empty()) ? str.data() + pos : nullptr;
}

const wchar_t *mameui::util::string_util::mui_wcschr(std::wstring_view str, std::wstring_view delim)
{
	auto pos = str.find(delim);
	return (pos != std::wstring_view::npos && !str.empty()) ? str.data() + pos : nullptr;
}


//============================================================
//  mui_wcscmp
//============================================================

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


//============================================================
//  mui_wcsncmp
//============================================================

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


//============================================================
//  mui_wcsicmp
//============================================================

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


//============================================================
//  mui_wcsnicmp
//============================================================

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


//============================================================
//  mui_wscnlen
//============================================================

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


//============================================================
//  mui_wsclen
//============================================================

size_t mameui::util::string_util::mui_wcslen(const wchar_t *src)
{
	return mui_wcsnlen(src, MUI_MAX_CHAR_COUNT);
}
