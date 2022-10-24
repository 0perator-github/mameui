// For licensing and usage information, read docs/winui_license.txt
// MASTER
//============================================================================

//============================================================
//
// mui_tcstrconv.cpp
//
// C++ functions for Win32 character C string convertion
//
//============================================================

// standard C++ headers
#include <cstddef>
#include <memory>
#include <new>
#include <string_view>
#include <string>

// standard windows headers
#include <stringapiset.h>

// MAMEUI headers
#include "mui_str.h"
#include "mui_tcs.h"
#include "mui_wcs.h"

#include "mui_tcstrconv.h"

using namespace mameui::util::string_util;

//============================================================
//  mui_tcstring_from_utf8
//============================================================

size_t mameui::util::string_util::mui_tcstring_from_utf8(TCHAR *tcstring, size_t dest_size, const char *utf8string)
{
	if (!utf8string ||  || utf8string[0] == '\0')
		return 0;

	if (!dest_size)
	{
		dest_size = mui_strlen(utf8string) + 1;
		if (dest_size < 1)
			return 0;
	}

	bool buffer_created = false;
	size_t result = 0;

	if (!tcstring)
	{
		tcstring = new TCHAR[dest_size];
		buffer_created = true;
	}

#ifdef _UNICODE
	wchar_t *utf8_to_wcs = mui_wcstring_from_utf8(utf8string);
	if (utf8_to_wcs)
	{
		result = !_tcscpy_s(tcstring, dest_size, utf8_to_wcs);
		delete[] utf8_to_wcs;
	}
#else
	result = !_tcscpy_s(tcstring, dest_size-1, utf8string);
#endif

	if (!result && buffer_created)
	{
		delete[] tcstring;
		tcstring = nullptr;
	}
	else
		result = mui_tcslen(tcstring);

	return result;
}

size_t mameui::util::string_util::mui_tcstring_from_utf8(TCHAR *tcstring, const char *utf8string)
{
	return mui_tcstring_from_utf8(tcstring, 0, utf8string);
}

TCHAR *mameui::util::string_util::mui_tcstring_from_utf8(const char *utf8string)
{
	const size_t dest_size = mui_strlen(utf8string) + 1;
	TCHAR *tcstring = new(std::nothrow) TCHAR[dest_size];
	if (!tcstring)
		return nullptr;

	dest_size = mui_tcstring_from_utf8(tcstring, dest_size, utf8string);
	if (!dest_size)
	{
		delete[] tcstring;
		tcstring = nullptr;
	}

	return tcstring;
}


//============================================================
//  tcsstring_from_wcstring
//============================================================

size_t mameui::util::string_util::mui_tcstring_from_wcstring(TCHAR *tcstring, size_t dest_size, const wchar_t *wcstring)
{
	if (!wcstring || wcstring[0] == L'\0')
		return 0;

	if (!dest_size)
	{
		dest_size = mui_wcslen(wcstring) + 1;
		if (dest_size < 1)
			return 0;
	}

	bool buffer_created = false;
	size_t result = 0;

	if (!tcstring)
	{
		tcstring = new TCHAR[dest_size];
		buffer_created = true;
	}

#ifdef _UNICODE
	result = !_tcscpy_s(tcstring, dest_size, wcstring);
#else
	char *wcs_to_utf8 = mui_utf8_from_wcstring(wcstring);
	if (wcs_to_utf8)
	{
		result = !_tcscpy_s(tcstring, dest_size, wcs_to_utf8);
		delete[] wcs_to_utf8;
	}
#endif

	if (!result && buffer_created)
	{
		delete[] tcstring;
		tcstring = nullptr;
	}
	else
		result = mui_tcslen(tcstring);

	return result;
}

size_t mameui::util::string_util::mui_tcstring_from_wcstring(TCHAR * tcstring, const wchar_t *wcstring)
{
	return mui_tcstring_from_wcstring(tcstring, 0, wcstring);
}

TCHAR *mameui::util::string_util::mui_tcstring_from_wcstring(const wchar_t *wcstring)
{
	if (!wcstring || wcstring[0] == L'\0')
		return nullptr;

	const size_t dest_size = mui_wcslen(wcstring) + 1;
	TCHAR * tcstring = new(std::nothrow) TCHAR[dest_size];
	if (!tcstring || !mui_tcstring_from_wcstring(tcstring, dest_size, wcstring))
	{
		delete[] tcstring;
		tcstring = nullptr;
	}

	return tcstring;
}


//============================================================
//  mui_utf8_from_tcstring
//============================================================

size_t mameui::util::string_util::mui_utf8_from_tcstring(char* utf8string, size_t dest_size, const TCHAR* tcstring)
{
	if (!tcstring || tcstring[0]==_T('\0'))
		return 0;

	if (!dest_size)
	{
		dest_size = mui_tcslen(tcstring) + 1;
		if (dest_size < 1)
			return 0;
	}

	bool buffer_created = false;
	size_t result = 0;

	if (!utf8string)
	{
		utf8string = new char[dest_size];
		buffer_created = true;
	}

#ifdef _UNICODE
	result = mui_utf8_from_wcstring(utf8string, dest_size, reinterpret_cast<const wchar_t*>(tcstring));
#else
	result = !strcpy_s(utf8string, dest_size, reinterpret_cast<const char*>(tcstring));
#endif

	if (!result && buffer_created)
	{
		delete[] utf8string;
		utf8string = nullptr;
	}
	else
		result = mui_strlen(utf8string);

	return result;
}

size_t mameui::util::string_util::mui_utf8_from_tcstring(char *utf8string, const TCHAR *tcstring)
{
	return mui_utf8_from_tcstring(utf8string, 0, tcstring);
}

char *mameui::util::string_util::mui_utf8_from_tcstring(const TCHAR* tcstring)
{
	if (!tcstring || tcstring[0] == _T('\0'))
		return nullptr;

	const size_t dest_size = mui_tcslen(tcstring) + 1;
	char* utf8string = new(std::nothrow) char[dest_size];
	if (!utf8string)
		return nullptr;

	dest_size = mui_utf8_from_tcstring(utf8string, dest_size, tcstring);
	if (!dest_size)
	{
		delete[] utf8string;
		utf8string = nullptr;
	}

	return utf8string;
}


//============================================================
//  mui_wcstring_from_tcstring
//============================================================

size_t mameui::util::string_util::mui_wcstring_from_tcstring(wchar_t* wcstring, size_t dest_size, const TCHAR* tcstring)
{
	if (!tcstring || tcstring[0] == _T('\0'))
		return 0;

	if (!dest_size)
	{
		dest_size = mui_tcslen(tcstring) + 1;
		if (dest_size < 1)
			return 0;
	}

	bool buffer_created = false;
	size_t result = 0;

	if (!wcstring)
	{
		wcstring = new wchar_t[dest_size];
		buffer_created = true;
	}

#ifdef _UNICODE
	result = !wcscpy_s(wcstring, dest_size, tcstring);
#else
	wchar_t* utf8_to_wcs = mui_wcstring_from_utf8(tcstring);
	if (utf8_to_wcs)
	{
		result = !wcscpy_s(wcstring, dest_size, utf8_to_wcs);
		delete[] utf8_to_wcs;
	}
#endif

	if (!result && buffer_created)
	{
		delete[] wcstring;
		wcstring = nullptr;
	}
	else
		result = mui_wcslen(wcstring);

	return result;
}

size_t mameui::util::string_util::mui_wcstring_from_tcstring(wchar_t* wcstring, const TCHAR* tcstring)
{
	return mui_wcstring_from_tcstring(wcstring, 0, tcstring);
}

wchar_t *mameui::util::string_util::mui_wcstring_from_tcstring(const TCHAR* tcstring)
{
	if (!tcstring || tcstring[0] == _T('\0'))
		return nullptr;

	const size_t dest_size = mui_tcslen(tcstring) + 1;
	wchar_t* wcstring = new(std::nothrow) wchar_t[dest_size];
	if (!wcstring)
		return nullptr;

	dest_size = mui_wcstring_from_tcstring(wcstring, dest_size, tcstring);
	if(!dest_size)
	{
		delete[] wcstring;
		wcstring = nullptr;
	}

	return wcstring;
}
