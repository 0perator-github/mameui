//	For licensing and usage information, read docs/winui_license.txt
//	MASTER
//	===========================================================================

//	===========================================================================
//	mui_str.cpp - C++ functions for C string manipulation
//	===========================================================================

// standard C++ headers
#include <cctype>
#include <cstdint>
#include <new>
#include <string>
#include <string_view>

#include "mui_cstr.h"

using namespace mameui::util::string_util;

//	-------------------------------------------------------
//	mui_strcpy - copies a character string from src to dst
//	parameters:
//	char *dst - destination string
//	const char *src - source string
//	-------------------------------------------------------

size_t mameui::util::string_util::mui_strcpy(char *dst, const char *src)
{
	if (!src || *src == '\0' || !dst)
		return 0;

	size_t result = 0;
	while (src[result] != '\0')
	{
		if (char_diff_cs(dst[result], src[result]))
			dst[result] = src[result];

		result++;
	}

	dst[result] = '\0';

	return result;
}

//	-------------------------------------------------------
//	mui_strcpy - copies a character string from src to a new allocated
//	string
//	parameters:
//	const char *src - source string
//	-------------------------------------------------------

char *mameui::util::string_util::mui_strcpy(const char *src)
{
	if (!src || src[0] == '\0')
		return nullptr;

	const size_t src_len = mui_strlen(src);
	char *result = new(std::nothrow) char[src_len + 1];

	if (!result)
		return result;

	if (!mui_strcpy(result, src))
	{
		delete[] result;
		result = nullptr;
	}

	return result;
}

//	-------------------------------------------------------
//	mui_strncpy - copies a character string from src to dst, up to
//	a specified count
//	parameters:
//	char *dst - destination string
//	const char *src - source string
//	const size_t count - maximum number of characters to copy
//	-------------------------------------------------------

size_t mameui::util::string_util::mui_strncpy(char *dst, const char *src, const size_t count)
{
	if (!src || *src == '\0' || !dst)
		return 0;

	size_t result = 0;
	while (result < count && src[result] != '\0')
	{
		if (char_diff_cs(dst[result], src[result]))
			dst[result] = src[result];

		result++;
	}

	dst[result] = '\0';

	return result;
}

//	-------------------------------------------------------
//	mui_strncpy - copies a character string from src to a new allocated
//	string, up to count characters
//	parameters:
//	const char *src - source string
//	const size_t count - maximum number of characters to copy
//	-------------------------------------------------------

char *mameui::util::string_util::mui_strncpy(const char *src, const size_t count)
{
	if (!src || src[0] == '\0')
		return nullptr;

	const size_t src_len = mui_strlen(src);
	char *result = (!src_len) ? 0 : new(std::nothrow) char[src_len + 1];

	if (!result)
		return result;

	if (!mui_strncpy(result, src, count))
	{
		delete[] result;
		result = nullptr;
	}

	return result;
}

//	-------------------------------------------------------
//	mui_strchr - finds the first occurrence of a character or substring in a string
//	parameters:
//	std::string &str - the string to search
//	char delim - the character to find
//	-------------------------------------------------------

char *mameui::util::string_util::mui_strchr(std::string &str, char delim)
{
	auto pos = str.find(delim);
	return (pos != std::string::npos && !str.empty()) ? &str[0] + pos : nullptr;
}

char *mameui::util::string_util::mui_strchr(std::string &str, std::string_view delim)
{
	auto pos = str.find(delim);
	return (pos != std::string::npos && !str.empty()) ? &str[0] + pos : nullptr;
}

const char *mameui::util::string_util::mui_strchr(std::string_view str, char delim)
{
	auto pos = str.find(delim);
	return (pos != std::string_view::npos && !str.empty()) ? str.data() + pos : nullptr;
}

const char *mameui::util::string_util::mui_strchr(std::string_view str, std::string_view delim)
{
	auto pos = str.find(delim);
	return (pos != std::string_view::npos && !str.empty()) ? str.data() + pos : nullptr;
}

//	-------------------------------------------------------
//	mui_strcmp - compares two character strings
//	parameters:
//	std::string_view s1 - first string to compare
//	std::string_view s2 - second string to compare
//	-------------------------------------------------------

int mameui::util::string_util::mui_strcmp(std::string_view s1, std::string_view s2)
{
	const size_t s1_size = s1.size(), s2_size = s2.size();

	size_t index = 0;
	while (index < s1_size && index < s2_size)
	{
		int diff = char_diff_cs(s1[index], s2[index]);
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
//	mui_strncmp - compares two character strings up to a specified count
//	parameters:
//	std::string_view s1 - first string to compare
//	std::string_view s2 - second string to compare
//	const size_t count - maximum number of characters to compare
//	-------------------------------------------------------

int mameui::util::string_util::mui_strncmp(std::string_view s1, std::string_view s2, const size_t count)
{
	const size_t s1_size = s1.size(), s2_size = s2.size();

	size_t index = 0;
	while (index < count && index < s1_size && index < s2_size)
	{
		int diff = char_diff_cs(s1[index], s2[index]);
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
//	mui_stricmp - compares two character strings, case-insensitively
//	parameters:
//	std::string_view s1 - first string to compare
//	std::string_view s2 - second string to compare
//	-------------------------------------------------------

int mameui::util::string_util::mui_stricmp(std::string_view s1, std::string_view s2)
{
	const size_t s1_size = s1.size(), s2_size = s2.size();

	size_t index = 0;
	while (index < s1_size && index < s2_size)
	{
		int diff = char_diff_ci(s1[index], s2[index]);
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
//	mui_strnicmp - compares two character strings up to a specified count, case-insensitively
//	parameters:
//	std::string_view s1 - first string to compare
//	std::string_view s2 - second string to compare
//	const size_t count - maximum number of characters to compare
//	-------------------------------------------------------

int mameui::util::string_util::mui_strnicmp(std::string_view s1, std::string_view s2, const size_t count)
{
	const size_t s1_size = s1.size(), s2_size = s2.size();

	size_t index = 0;
	while (index < count && index < s1_size && index < s2_size)
	{
		int diff = char_diff_ci(s1[index], s2[index]);
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
//	mui_strnlen - calculates the length of a character string up to a specified maximum count
//	parameters:
//	const char *src - the source string
//	const size_t max_count - the maximum number of characters to consider
//	-------------------------------------------------------

size_t mameui::util::string_util::mui_strnlen(const char *src, const size_t max_count)
{
	size_t result = 0;

	if (src)
	{
		while (result < max_count && src[result] != '\0')
		{
			result++;
		}
	}

	return result;
}

//	-------------------------------------------------------
//	mui_strlen - calculates the length of a character string
//	parameters:
//	const char *src - the source string
//	-------------------------------------------------------

size_t mameui::util::string_util::mui_strlen(const char *src)
{
	return mui_strnlen(src, MUI_MAX_CHAR_COUNT);
}
