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
#include <optional>
#include <vector>
#include <memory>
#include <limits>
#include <iterator>
#include <cstdint>

// standard windows headers

// mame headers

// mameui headers

#include "mui_stringtokenizer.h"

// Explicit template instantiation for stringtokenizer_t<char> and <wchar_t>
// Ensures symbols are available in this TU for linking in projects.
template class mameui::util::stringtokenizer_t<char>;
template class mameui::util::stringtokenizer_t<wchar_t>;
