
// standard windows headers
#include <stringapiset.h>

// MAME/MAMEUI headers
#include "mui_str.h"
#include "mui_wcs.h"
#include "mui_wcsconv.h"

//============================================================
//  mui_utf8_from_wcstring
//============================================================

int mui_utf8_from_wcstring(char *utf8string, int dest_size, const wchar_t *wcstring)
{
	if (!wcstring || wcstring[0] == L'\0'|| !utf8string)
		return 0;

	if (dest_size < 1)
	{
		dest_size = WideCharToMultiByte(CP_UTF8, 0, wcstring, -1, NULL, 0, NULL, NULL);
		if (dest_size < 1)
			return 0;
	}

	dest_size = WideCharToMultiByte(CP_UTF8, 0, wcstring, dest_size, utf8string, dest_size, NULL, NULL);

	return (dest_size < 1) ? 0 : dest_size;
}

int mui_utf8_from_wcstring(char *utf8string, const wchar_t *wcstring)
{
	return mui_utf8_from_wcstring(utf8string, 0, wcstring);
}

char *mui_utf8_from_wcstring(const wchar_t *wcstring)
{
	if (!wcstring || wcstring[0] == L'\0')
		return 0;

	const size_t utf8s_len = mui_wcslen(wcstring);
	char *utf8string = (!utf8s_len) ? 0 : new char[utf8s_len + 1];

	if (!utf8string)
		return 0;

	if (!mui_utf8_from_wcstring(utf8string, utf8s_len + 1, wcstring))
	{
		delete[] utf8string;
		utf8string = 0;
	}

	return utf8string;
}


//============================================================
//  mui_wcstring_from_utf8
//============================================================

int mui_wcstring_from_utf8(wchar_t* wcstring, int dest_size, const char* utf8string)
{
	if (!utf8string || utf8string[0] == '\0'|| !wcstring)
		return 0;

	if (dest_size < 1)
	{
		dest_size = MultiByteToWideChar(CP_UTF8, 0, utf8string, -1, NULL, 0);
		if (dest_size < 1)
			return 0;
	}

	dest_size = MultiByteToWideChar(CP_UTF8, 0, utf8string, dest_size, wcstring, dest_size);

	return (dest_size < 1) ? 0 : dest_size;
}

int mui_wcstring_from_utf8(wchar_t *wcstring, const char *utf8string)
{
	return mui_wcstring_from_utf8(wcstring, 0, utf8string);
}

wchar_t *mui_wcstring_from_utf8(const char *utf8string)
{

	if (!utf8string || utf8string[0] == '\0')
		return 0;

	const size_t wcs_len = mui_strnlen(utf8string, util::MAX_WIDE_STRING_SIZE - 1);
	wchar_t *wcstring = (!wcs_len) ? 0 : new wchar_t[wcs_len + 1];

	if (!wcstring)
		return 0;

	if (!mui_wcstring_from_utf8(wcstring, wcs_len + 1, utf8string))
	{
		delete[] wcstring;
		wcstring = 0;
	}


	return wcstring;
}
