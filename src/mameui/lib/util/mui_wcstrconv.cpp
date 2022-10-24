// For licensing and usage information, read docs/winui_license.txt
// MASTER
//============================================================================

//============================================================
//
// mui_wcstrconv.cpp
//
// C++ functions for Wide character C string convertion
//
//============================================================

// standard C++ headers
#include <cctype>
#include <cstdint>
#include <cwctype>
#include <new>
#include <string>
#include <string_view>

// standard windows headers
#include <stringapiset.h>

// MAMEUI headers
#include "mui_cstr.h"
#include "mui_wcstr.h"

#include "mui_wcstrconv.h"

using namespace mameui::util::string_util;

//============================================================
// mui_utf8_from_utf16string - strings
//============================================================

int mameui::util::string_util::mui_utf8_from_utf16string(std::string &utf8string, std::wstring_view utf16string, int wchar_count)
{
	if (utf16string.empty() || wchar_count == 0)
		return 0;

	int length = static_cast<int>(utf16string.length());
	if (wchar_count < -1 || wchar_count > length)
		return -1;

	if (wchar_count == -1)
		wchar_count = length;

	const wchar_t* utf16string_ptr = utf16string.data();
	int bytes_required = WideCharToMultiByte(CP_UTF8, 0, utf16string_ptr, wchar_count, nullptr, 0, nullptr, nullptr);
	if (bytes_required <= 0)
		return -1;

	utf8string.resize(bytes_required);

	int bytes_written = WideCharToMultiByte(CP_UTF8, 0, utf16string_ptr, wchar_count, utf8string.data(), bytes_required, nullptr, nullptr);
	if (bytes_written <= 0)
	{
		utf8string.clear();
		return -1;
	}

	utf8string.resize(bytes_written);

	return bytes_written;
}

std::string mameui::util::string_util::mui_utf8_from_utf16string(std::wstring_view utf16string, int wchar_count)
{
	std::string utf8string;

	if (mui_utf8_from_utf16string(utf8string, utf16string, wchar_count) < 0)
		return {};

	return utf8string;
}

//============================================================
// mui_utf8_from_utf16cstring - raw pointers
//============================================================

int mameui::util::string_util::mui_utf8_from_utf16cstring(char *utf8cstring, int byte_count, const wchar_t *utf16cstring, int wchar_count)
{
	if (!utf16cstring || *utf16cstring == L'\0' || wchar_count == 0)
		return 0;

	int length = static_cast<int>(mui_wcslen(utf16cstring));
	if (wchar_count < -1 || wchar_count > length)
		return -1;

	if (wchar_count == -1)
		wchar_count = length;

	int bytes_required = WideCharToMultiByte(CP_UTF8, 0, utf16cstring, wchar_count, nullptr, 0, nullptr, nullptr);
	if (bytes_required <= 0)
		return -1;

	if (byte_count < bytes_required + 1)
		return bytes_required + 1;

	int bytes_written = WideCharToMultiByte(CP_UTF8, 0, utf16cstring, wchar_count, utf8cstring, bytes_required, nullptr, nullptr);
	if (bytes_written <= 0)
	{
		*utf8cstring = '\0';

		return -1;
	}

	utf8cstring[bytes_written] = '\0';

	return bytes_written;
}

int mameui::util::string_util::mui_utf8_from_utf16cstring(char *utf8cstring, const wchar_t *utf16cstring, int wchar_count)
{
	if (!utf16cstring || *utf16cstring == L'\0' || wchar_count == 0)
		return 0;

	int length = static_cast<int>(mui_wcslen(utf16cstring));
	if (wchar_count < -1 || wchar_count > length)
		return -1;

	if (wchar_count == -1)
		wchar_count = length;

	int bytes_required = WideCharToMultiByte(CP_UTF8, 0, utf16cstring, wchar_count, nullptr, 0, nullptr, nullptr);
	if (bytes_required <= 0)
		return -1;

	int bytes_written = WideCharToMultiByte(CP_UTF8, 0, utf16cstring, wchar_count, utf8cstring, bytes_required, nullptr, nullptr);
	if (bytes_written <= 0)
	{
		*utf8cstring = '\0';

		return -1;
	}

	utf8cstring[bytes_written] = '\0';

	return bytes_written;
}

char *mameui::util::string_util::mui_utf8_from_utf16cstring(const wchar_t* utf16cstring, int wchar_count)
{
	if (!utf16cstring || *utf16cstring == L'\0' || wchar_count == 0)
		return nullptr;

	int length = static_cast<int>(mui_wcslen(utf16cstring));
	if (wchar_count < -1 || wchar_count > length)
		return nullptr;

	if (wchar_count == -1)
		wchar_count = length;

	int bytes_required = WideCharToMultiByte(CP_UTF8, 0, utf16cstring, wchar_count, nullptr, 0, nullptr, nullptr);
	if (bytes_required <= 0)
		return nullptr;

	char* utf8cstring = new(std::nothrow) char[bytes_required + 1];
	if (!utf8cstring)
		return nullptr;

	int bytes_written = WideCharToMultiByte(CP_UTF8, 0, utf16cstring, wchar_count, utf8cstring, bytes_required, nullptr, nullptr);
	if (bytes_written <= 0)
	{
		delete[] utf8cstring;
		return nullptr;
	}

	utf8cstring[bytes_written] = '\0';

	return utf8cstring;
}


//============================================================
//  mui_utf16_from_utf8string - strings
//============================================================

int mameui::util::string_util::mui_utf16_from_utf8string(std::wstring &utf16string, std::string_view utf8string, int byte_count)
{
	if (utf8string.empty() || byte_count == 0)
		return 0;

	int length = static_cast<int>(utf8string.length());
	if (byte_count < -1 || byte_count > length)
		return -1;

	if (byte_count == -1)
		byte_count = length;

	const char* utf8string_ptr = utf8string.data();
	int wchars_required = MultiByteToWideChar(CP_UTF8, 0, utf8string_ptr, byte_count, nullptr, 0);
	if (wchars_required <= 0)
		return -1;

	utf16string.resize(wchars_required);

	int wchars_written = MultiByteToWideChar(CP_UTF8, 0, utf8string_ptr, byte_count, utf16string.data(), wchars_required);
	if (wchars_written <= 0)
	{
		utf16string.clear();
		return -1;
	}

	utf16string.resize(wchars_written);

	return wchars_written;
}

std::wstring mameui::util::string_util::mui_utf16_from_utf8string(std::string_view utf8string, int byte_count)
{
	std::wstring utf16string;

	if (mui_utf16_from_utf8string(utf16string, utf8string, byte_count) < 0)
		return {};

	return utf16string;
}


//============================================================
//  mui_utf16_from_utf8cstring - raw pointers
//============================================================

int mameui::util::string_util::mui_utf16_from_utf8cstring(wchar_t* utf16cstring, int wchar_count, const char* utf8cstring, int byte_count)
{
	if (!utf8cstring || *utf8cstring == '\0' || byte_count == 0)
		return 0;

	int length = static_cast<int>(mui_strlen(utf8cstring));
	if (byte_count < -1 || byte_count > length)
		return -1;

	if (byte_count == -1)
		byte_count = length;

	int wchars_required = MultiByteToWideChar(CP_UTF8, 0, utf8cstring, byte_count, nullptr, 0);
	if (wchars_required <= 0)
		return -1;

	if (wchar_count < wchars_required + 1)
		return wchars_required + 1;

	int wchars_written = MultiByteToWideChar(CP_UTF8, 0, utf8cstring, byte_count, utf16cstring, wchar_count);
	if (wchars_written <= 0)
	{
		*utf16cstring = L'\0';

		return -1;
	}

	utf16cstring[wchars_written] = L'\0';

	return wchars_written;
}

int mameui::util::string_util::mui_utf16_from_utf8cstring(wchar_t *utf16cstring, const char *utf8cstring, int byte_count)
{
	if (!utf8cstring || *utf8cstring == '\0' || byte_count == 0)
		return 0;

	int length = static_cast<int>(mui_strlen(utf8cstring));
	if (byte_count < -1 || byte_count > length)
		return -1;

	if (byte_count == -1)
		byte_count = length;

	int wchars_required = MultiByteToWideChar(CP_UTF8, 0, utf8cstring, byte_count, nullptr, 0);
	if (wchars_required <= 0)
		return -1;

	int wchars_written = MultiByteToWideChar(CP_UTF8, 0, utf8cstring, byte_count, utf16cstring, wchars_required);
	if (wchars_written <= 0)
	{
		*utf16cstring = L'\0';

		return -1;
	}

	utf16cstring[wchars_written] = L'\0';

	return wchars_written;
}

wchar_t *mameui::util::string_util::mui_utf16_from_utf8cstring(const char *utf8cstring, int byte_count)
{
	if (!utf8cstring || *utf8cstring == '\0' || byte_count == 0)
		return nullptr;

	int length = static_cast<int>(mui_strlen(utf8cstring));
	if (byte_count < -1 || byte_count > length)
		return nullptr;

	if (byte_count == -1)
		byte_count = length;

	int wchars_required = MultiByteToWideChar(CP_UTF8, 0, utf8cstring, byte_count, nullptr, 0);
	if (wchars_required <= 0)
		return nullptr;

	wchar_t* utf16cstring = new(std::nothrow) wchar_t[wchars_required + 1];
	if (!utf16cstring)
		return nullptr;

	int wchars_written = MultiByteToWideChar(CP_UTF8, 0, utf8cstring, byte_count, utf16cstring, wchars_required);
	if (wchars_written <= 0)
	{
		delete[] utf16cstring;
		return nullptr;
	}

	utf16cstring[wchars_written] = L'\0';

	return utf16cstring;
}
