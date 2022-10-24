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
#include <memory>
#include <mutex>
#include <unordered_set>
#include <string>
#include <optional>
#include <vector>
#include <limits>
#include <cstdint>
#include <algorithm>
#include <iterator>


// standard windows headers

// mame headers

// mameui headers

#include "mui_stringtokenizer.h"

// Explicit template instantiation for stringtokenizer_t<char> and <wchar_t>
// Ensures symbols are available in this TU for linking in projects.
template class mameui::util::stringtokenizer_t<char>;
template class mameui::util::stringtokenizer_t<wchar_t>;
