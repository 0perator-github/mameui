// For licensing and usage information, read docs/winui_license.txt
// MASTER
//============================================================================

//============================================================
//
// mui_stringtokenizer.cpp
//
// C++ functions for C string manipulation
//
//============================================================

// standard C++ headers
#include <string>
#include <string_view>
#include <unordered_set>
#include <iterator>

// standard windows headers

// mame headers

// mameui headers

#include "mui_stringtokenizer.h"

template class mui_stringtokenizer_t<char>;
template class mui_stringtokenizer_t<wchar_t>;
