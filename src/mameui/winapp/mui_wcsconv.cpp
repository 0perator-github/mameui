// For licensing and usage information, read docs/winui_license.txt
// MASTER
//============================================================================

//============================================================
//
// mui_wcsconv.cpp
//
// C++ functions for Wide character C string convertion
//
//============================================================
#include "mui_wcsconv.h"

// standard windows headers
#include <stringapiset.h>

// MAMEUI headers
#include "mui_str.h"
#include "mui_wcs.h"

//============================================================
//  mui_utf8_from_wcstring
//============================================================

int mui_utf8_from_wcstring(char* utf8string, int dest_size, const wchar_t* wcstring)
{
	if (!wcstring || wcstring[0] == L'\0')
		return 0;

	if (dest_size < 1)
	{
		dest_size = WideCharToMultiByte(CP_UTF8, 0, wcstring, -1, NULL, 0, NULL, NULL);
		if (dest_size < 1)
			return 0;
	}

	bool buffer_created = false;

	if (!utf8string)
	{
		utf8string = new char[dest_size];
		if (!utf8string)
			return 0;
		buffer_created = true;
	}

	dest_size = WideCharToMultiByte(CP_UTF8, 0, wcstring, dest_size, utf8string, dest_size, NULL, NULL);
	if (dest_size < 1 && buffer_created)
	{
		delete[] utf8string;
		return 0;
	}

	return dest_size;
}

int mui_utf8_from_wcstring(char *utf8string, const wchar_t *wcstring)
{
	return mui_utf8_from_wcstring(utf8string, 0, wcstring);
}

char *mui_utf8_from_wcstring(const wchar_t *wcstring)
{
	if (!wcstring || wcstring[0] == L'\0')
		return nullptr;

	size_t utf8s_len = mui_wcslen(wcstring);
	char *utf8string = (!utf8s_len) ? nullptr : new(std::nothrow) char[utf8s_len + 1];
	if (!utf8string)
		return nullptr;

	utf8s_len = mui_utf8_from_wcstring(utf8string, utf8s_len + 1, wcstring);
	if (!utf8s_len)
	{
		delete[] utf8string;
		utf8string = nullptr;
	}

	return utf8string;
}


//============================================================
//  mui_wcstring_from_utf8
//============================================================

int mui_wcstring_from_utf8(wchar_t* wcstring, int dest_size, const char* utf8string)
{
	if (!utf8string || utf8string[0] == '\0')
		return 0;

	if (dest_size < 1)
	{
		dest_size = MultiByteToWideChar(CP_UTF8, 0, utf8string, -1, NULL, 0);
		if (dest_size < 1)
			return 0;
	}

	bool buffer_created = false;

	if (!wcstring)
	{
		wcstring = new wchar_t[dest_size];
		if (!wcstring)
			return 0;
		buffer_created = true;
	}

	dest_size = MultiByteToWideChar(CP_UTF8, 0, utf8string, dest_size, wcstring, dest_size);
	if (dest_size < 1 && buffer_created)
	{
		delete[] wcstring;
		return 0;
	}

	return dest_size;
}

int mui_wcstring_from_utf8(wchar_t *wcstring, const char *utf8string)
{
	return mui_wcstring_from_utf8(wcstring, 0, utf8string);
}

wchar_t *mui_wcstring_from_utf8(const char *utf8string)
{
	if (!utf8string || utf8string[0] == '\0')
		return nullptr;

	size_t wcs_len = mui_strnlen(utf8string, util::MAX_WIDE_STRING_SIZE - 1);
	wchar_t *wcstring = (!wcs_len) ? nullptr : new(std::nothrow) wchar_t[wcs_len + 1];

	if (!wcstring)
		return nullptr;

	wcs_len = mui_wcstring_from_utf8(wcstring, wcs_len + 1, utf8string);
	if (!wcs_len)
	{
		delete[] wcstring;
		wcstring = nullptr;
	}

	return wcstring;
}
