// For licensing and usage information, read docs/winui_license.txt
// MASTER
//============================================================================

//============================================================
//
// mui_str.cpp
//
// C++ functions for C string manipulation
//
//============================================================
#include "mui_str.h"

// standard C++ headers
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>

//============================================================
//  mui_strcpy
//============================================================

size_t mui_strcpy(char *dst, const char *src)
{
	if (!dst || !src)
		return 0;

	size_t result = 0;
	while (src[result] != '\0')
	{
		dst[result] = src[result];
		result++;
	}

	dst[result] = '\0';

	return result;
}

char *mui_strcpy(const char *src)
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


//============================================================
//  mui_strncpy
//============================================================

size_t mui_strncpy(char *dst, const char *src, const size_t count)
{
	if (!dst || !src)
		return 0;

	size_t result = 0;
	while (result < count && src[result] != '\0')
	{
		dst[result] = src[result];
		result++;
	}

	dst[result] = '\0';

	return result;
}

char* mui_strncpy(const char *src, size_t count)
{
	if (!src || src[0] == L'\0')
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


//============================================================
//  mui_strchr
//============================================================

char *mui_strchr(std::string_view str, std::string_view delim)
{
	const std::string_view::size_type find_pos = str.find_first_of(delim);

	if (find_pos != std::string_view::npos)
	{
		return const_cast<char*>(str.data() + find_pos);
	}

	return nullptr;
}


//============================================================
//  mui_strtok
//============================================================

std::string_view mui_strtok(std::string_view str, std::string_view delim)
{
	static std::string buffer;
	static std::string::size_type saved_pos;
	std::string::size_type find_pos;

	if (!str.empty())
	{
		buffer = str;
		saved_pos = 0;
	}

	if (saved_pos > buffer.size())
		return std::string_view();

	find_pos = buffer.find_first_not_of(delim, saved_pos);
	if (find_pos != std::string::npos)
		saved_pos = find_pos;
	else
		return std::string_view();

	const std::string_view::size_type token_begin = saved_pos;

	find_pos = buffer.find_first_of(delim, find_pos);
	if (find_pos != std::string::npos)
		saved_pos = find_pos;
	else
		saved_pos = buffer.size();

	const std::string_view::size_type token_end = saved_pos;

	saved_pos++;

	return std::string_view(&buffer[token_begin], token_end - token_begin);
}


//============================================================
//  mui_strcmp
//============================================================

int mui_strcmp(std::string_view s1, std::string_view s2)
{
	auto s1_iter = s1.begin(), s2_iter = s2.begin();

	while (s1_iter != s1.end() && s2_iter != s2.end())
	{
		const char c1 = *s1_iter++;
		const char c2 = *s2_iter++;
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
//  mui_strncmp
//============================================================

int mui_strncmp(std::string_view s1, std::string_view s2, int count)
{
	auto s1_iter = s1.begin(), s2_iter = s2.begin();

	for (size_t i = 0; i < count; i++)
	{
		if (s1_iter == s1.end())
			return (s2_iter == s2.end()) ? 0 : -1;
		if (s2_iter == s2.end())
			return 1;

		const char c1 = *s1_iter++;
		const char c2 = *s2_iter++;
		const int diff = c1 - c2;
		if (diff)
			return diff;
	}

	return 0;
}


//============================================================
//  mui_stricmp
//============================================================

int mui_stricmp(std::string_view s1, std::string_view s2)
{
	auto s1_iter = s1.begin(), s2_iter = s2.begin();

	while (s1_iter != s1.end() && s2_iter != s2.end())
	{
		const char c1 = std::toupper(*s1_iter++);
		const char c2 = std::toupper(*s2_iter++);
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
//  mui_strnicmp
//============================================================

int mui_strnicmp(std::string_view s1, std::string_view s2, int count)
{
	auto s1_iter = s1.begin(), s2_iter = s2.begin();

	for (size_t i = 0; i < count; i++)
	{
		if (s1_iter == s1.end())
			return (s2_iter == s2.end()) ? 0 : -1;
		if (s2_iter == s2.end())
			return 1;

		const char c1 = std::toupper(*s1_iter++);
		const char c2 = std::toupper(*s2_iter++);
		const int diff = c1 - c2;
		if (diff)
			return diff;
	}

	return 0;
}


//============================================================
//  mui_strnlen
//============================================================

size_t mui_strnlen(const char* src, size_t max_count)
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


//============================================================
//  mui_strlen
//============================================================

size_t mui_strlen(const char *src)
{
	return mui_strnlen(src,util::MAX_STRING_SIZE);
}
