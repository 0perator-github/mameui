// license:BSD-3-Clause
// copyright-holders:0perator
#ifndef MAMEUI_LIB_UTIL_MUI_TRIMSTR_H
#define MAMEUI_LIB_UTIL_MUI_TRIMSTR_H

#pragma once

#if 0 // these headers are required when including this header file
// standard C++ headers
#include <cstddef> // optional for size_t
#include <string>
#endif

namespace mameui::util::string_util
{
	// Trims the whitespace from both ends of a string
	[[nodiscard]] inline std::string_view trim_ends(std::string_view str)
	{
		const size_t start = str.find_first_not_of(" \t\r\n");
		if (start == std::string_view::npos)
			return {};

		const size_t end = str.find_last_not_of(" \t\r\n");
		return str.substr(start, end - start + 1);
	}

	// Trims the whitespace from the beginning of a string
	[[nodiscard]] inline std::string_view trim_leading(std::string_view str)
	{
		const size_t start = str.find_first_not_of(" \t\r\n");

		return (start == std::string_view::npos) ? std::string_view{} : str.substr(start);
	}

	// Trims the whitespace from the end of a string
	[[nodiscard]] inline std::string_view trim_trailing(std::string_view str)
	{
		const size_t end = str.find_last_not_of(" \t\r\n");

		return (end == std::string_view::npos) ? std::string_view{} : str.substr(0, end + 1);
	}
}

#endif // MAMEUI_LIB_UTIL_MUI_TRIMSTR_H
