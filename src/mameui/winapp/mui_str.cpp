
// standard C++ headers
#include <cctype>
#include <cstdint>
#include <cstring>

#include <algorithm>

// MAME/MAMEUI headers
#include "mui_str.h"


//============================================================
//  mui_strcpy
//============================================================

size_t mui_strcpy(char* dst, const char* src)
{
	size_t result = 0;

	if (!src || src[0] == '\0')
		return result;

	for (; src[result] != '\0'; result++)
		dst[result] = src[result];

	dst[result] = '\0';

	return result;
}

char* mui_strcpy(const char* src)
{
	if (!src || src[0] == '\0')
		return 0;

	const size_t src_len = mui_strlen(src);
	char* result = (!src_len) ? 0 : new char[src_len + 1];

	if (!result)
		return result;

	if (!mui_strcpy(result, src))
	{
		delete[] result;
		result = 0;
	}

	return result;
}


//============================================================
//  mui_strncpy
//============================================================

size_t mui_strncpy(char* dst, const char* src, const size_t count)
{
	size_t result = 0;

	if (!src || src[0] == '\0')
		return result;

	for (; result != count && src[result] != '\0'; result++)
		dst[result] = src[result];

	dst[result] = '\0';

	return result;
}

char* mui_strncpy(const char *src, size_t count)
{
	if (!src || src[0] == '\0')
		return 0;

	const size_t src_len = mui_strlen(src);
	char* result = (!src_len) ? 0 : new char[src_len + 1];

	if (!result)
		return result;

	if (!mui_strncpy(result, src, count))
	{
		delete[] result;
		result = 0;
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
		return const_cast<char*>(str.begin() + find_pos);
	}

	return nullptr;
}


//============================================================
//  mui_strtok
//============================================================

std::string mui_strtok(std::string_view str, std::string_view delim)
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
		return "";

	find_pos = buffer.find_first_not_of(delim, saved_pos);
	if (find_pos != std::string::npos)
		saved_pos = find_pos;
	else
		return "";

	const std::string_view::size_type token_begin = saved_pos;

	find_pos = buffer.find_first_of(delim, find_pos);
	if (find_pos != std::string::npos)
		saved_pos = find_pos;
	else
		saved_pos = buffer.size();

	const std::string_view::size_type token_end = saved_pos;

	saved_pos++;

	return buffer.substr(token_begin, token_end - token_begin);
}


//============================================================
//  mui_strcmp
//============================================================

int mui_strcmp(std::string_view s1, std::string_view s2)
{
	auto s1_iter = s1.begin(), s2_iter = s2.begin();

	while (true)
	{
		if (s1.end() == s1_iter)
			return (s2.end() == s2_iter) ? 0 : -1;
		else if (s2.end() == s2_iter)
			return 1;

		const int c1 = uint8_t(*s1_iter++);
		const int c2 = uint8_t(*s2_iter++);
		const int diff = c1 - c2;
		if (diff)
			return diff;
	}
}


//============================================================
//  mui_strncmp
//============================================================

int mui_strncmp(std::string_view s1, std::string_view s2, int count)
{
	auto s1_iter = s1.begin(), s2_iter = s2.begin();

	for (size_t i = 0; i < count; i++)
	{
		if (s1.size() == i)
			return (s2.size() == i) ? 0 : -1;
		else if (s2.size() == i)
			return 1;

		const int c1 = uint8_t(*s1_iter++);
		const int c2 = uint8_t(*s2_iter++);
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

	while (true)
	{
		if (s1.end() == s1_iter)
			return (s2.end() == s2_iter) ? 0 : -1;
		else if (s2.end() == s2_iter)
			return 1;

		const int c1 = toupper(uint8_t(*s1_iter++));
		const int c2 = toupper(uint8_t(*s2_iter++));
		const int diff = c1 - c2;
		if (diff)
			return diff;
	}
}


//============================================================
//  mui_strnicmp
//============================================================

int mui_strnicmp(std::string_view s1, std::string_view s2, int count)
{
	auto s1_iter = s1.begin(), s2_iter = s2.begin();

	for (size_t i = 0; i < count; i++)
	{
		if (s1.size() == i)
			return (s2.size() == i) ? 0 : -1;
		else if (s2.size() == i)
			return 1;

		const int c1 = toupper(uint8_t(*s1_iter++));
		const int c2 = toupper(uint8_t(*s2_iter++));
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

	if (max_count > SIZE_MAX)
		return result;

	if (src && src[0] != '\0')
	{
		for (; src[result] != '\0'; result++)
		{
			if (result == max_count)
				break;
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
