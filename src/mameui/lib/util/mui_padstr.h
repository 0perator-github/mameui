// license:BSD-3-Clause
// copyright-holders:0perator
#ifndef MAMEUI_LIB_UTIL_MUI_PADSTR_H
#define MAMEUI_LIB_UTIL_MUI_PADSTR_H

#pragma once

#include "mui_trimstr.h"
#if 0 // these headers are required when including this header file
// standard C++ headers
#include <cstddef> // optional for size_t
#include <string>
#endif

namespace mameui::util::string_util
{
	namespace pad_defaults
	{
		// Default values for padding functions
		constexpr bool add_space = false;
		constexpr char pad_char = '*';
		constexpr size_t total_length = 40;
	}

	// Pads a string to the center with optional total length, custom padding character, and surrounding space
	[[nodiscard]] inline std::string pad_to_center(std::string name, size_t total_length = pad_defaults::total_length, char pad_char = pad_defaults::pad_char, bool add_space = pad_defaults::add_space)
	{
		if (name.empty())
			return {};

		name = trim_ends(name);
		if (add_space) { name = std::string(" ") + name + " "; }

		const size_t name_length = name.length();
		if (name_length >= total_length)
			return name;

		const size_t total_padding = total_length - name_length;
		const size_t left_padding = total_padding / 2;
		const size_t right_padding = total_padding - left_padding;

		const std::string left_pad(left_padding, pad_char);
		const std::string right_pad(right_padding, pad_char);

		name = left_pad + name + right_pad;

		return name;
	}

	[[nodiscard]] inline std::string pad_to_center(const std::string& name, size_t total_length)
	{
		return pad_to_center(name, total_length, pad_defaults::pad_char, pad_defaults::add_space);
	}

	[[nodiscard]] inline std::string pad_to_center(const std::string& name, size_t total_length, char pad_char)
	{
		return pad_to_center(name, total_length, pad_char, pad_defaults::add_space);
	}

	[[nodiscard]] inline std::string pad_to_center(const std::string& name, char pad_char, bool add_space)
	{
		return pad_to_center(name, pad_defaults::total_length, pad_char, add_space);
	}

	[[nodiscard]] inline std::string pad_to_center(const std::string& name, size_t total_length, bool add_space)
	{
		return pad_to_center(name, total_length, pad_defaults::pad_char, pad_defaults::add_space);
	}

	[[nodiscard]] inline std::string pad_to_center(const std::string& name, bool add_space)
	{
		return pad_to_center(name, pad_defaults::total_length, pad_defaults::pad_char, add_space);
	}


	// Pads a string to the left with optional total length, custom padding character, and surrounding space
	[[nodiscard]] inline std::string pad_to_left(std::string name, size_t total_length = 40, char pad_char = '*', bool add_space = false)
	{
		if (name.empty())
			return {};

		name = trim_ends(name);
		if (add_space) { name = std::string(" ") + name + " "; }

		const size_t name_length = name.length();
		if (name_length >= total_length)
			return name;

		const std::string pad(total_length - name_length, pad_char);
		name.append(pad);

		return name;
	}

	[[nodiscard]] inline std::string pad_to_left(const std::string &name, size_t total_length)
	{
		return pad_to_left(name, total_length, pad_defaults::pad_char, pad_defaults::add_space);
	}

	[[nodiscard]] inline std::string pad_to_left(const std::string &name, size_t total_length, char pad_char)
	{
		return pad_to_left(name, total_length, pad_char, pad_defaults::add_space);
	}

	[[nodiscard]] inline std::string pad_to_left(const std::string &name, char pad_char, bool add_space)
	{
		return pad_to_left(name, pad_defaults::total_length, pad_char, add_space);
	}

	[[nodiscard]] inline std::string pad_to_left(const std::string &name, size_t total_length, bool add_space)
	{
		return pad_to_left(name, total_length, pad_defaults::pad_char, pad_defaults::add_space);
	}

	[[nodiscard]] inline std::string pad_to_left(const std::string &name, bool add_space)
	{
		return pad_to_left(name, pad_defaults::total_length, pad_defaults::pad_char, add_space);
	}


	// Pads a string to the right with optional total length, custom padding character, and surrounding space
	[[nodiscard]] inline std::string pad_to_right(std::string name, size_t total_length = 40, char pad_char = '*', bool add_space = false)
	{
		if (name.empty())
			return {};

		name = trim_ends(name);
		if (add_space) { name = std::string(" ") + name + " "; }

		const size_t name_length = name.length();
		if (name_length >= total_length)
			return name;

		const std::string pad(total_length - name_length, pad_char);
		name = pad + name;

		return name;
	}

	[[nodiscard]] inline std::string pad_to_right(const std::string &name, size_t total_length)
	{
		return pad_to_right(name, total_length, pad_defaults::pad_char, pad_defaults::add_space);
	}

	[[nodiscard]] inline std::string pad_to_right(const std::string &name, size_t total_length, char pad_char)
	{
		return pad_to_right(name, total_length, pad_char, pad_defaults::add_space);
	}

	[[nodiscard]] inline std::string pad_to_right(const std::string &name, char pad_char, bool add_space)
	{
		return pad_to_right(name, pad_defaults::total_length, pad_char, add_space);
	}

	[[nodiscard]] inline std::string pad_to_right(const std::string &name, size_t total_length, bool add_space)
	{
		return pad_to_right(name, total_length, pad_defaults::pad_char, add_space);
	}

	[[nodiscard]] inline std::string pad_to_right(const std::string &name, bool add_space)
	{
		return pad_to_right(name, pad_defaults::total_length, pad_defaults::pad_char, add_space);
	}
}

#endif // MAMEUI_LIB_UTIL_MUI_PADSTR_H
