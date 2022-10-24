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
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>
#include <string>
#include <unordered_set>
#include <vector>

// standard windows headers

// mame headers

// mameui headers
#include "mui_stringtokenizer.h"

// Explicit template instantiation for stringtokenizer_t<char> and <wchar_t>
// Ensures symbols are available in this TU for linking in projects.
template class mameui::util::string_util::stringtokenizer_t<char>;
template class mameui::util::string_util::stringtokenizer_t<wchar_t>;
