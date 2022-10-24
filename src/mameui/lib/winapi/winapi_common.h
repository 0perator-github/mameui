// license:BSD-3-Clause
// copyright-holders:0perator

#ifndef MAMEUI_LIB_WINAPI_WINAPI_COMMON_H
#define MAMEUI_LIB_WINAPI_WINAPI_COMMON_H

#pragma once

// Standard Windows headers

// Core Windows headers
#include <windows.h> // Core Windows API functions and definitions
#include <windef.h>  // Windows data types and macros
#include <winuser.h> // User interface components (e.g., windows, menus, dialogs)

// Common Controls Library
#include <commctrl.h> // Common controls like ListView, TreeView, ProgressBar, etc.

// Common Dialogs Library
#include <commdlg.h> // Common dialog boxes like file open/save, color picker, etc.

// Shell Lightweight Utility API
#include <shlwapi.h> // Path and string manipulation functions

// DirectInput (for joystick or other input devices)
#include <dinput.h> // DirectInput API for input device management

// Process Environment API
#include <processenv.h> // Functions for accessing environment variables

// Process and Thread API
#include <processthreadsapi.h> // Functions for creating and managing processes and threads

// Shell API
#include <shellapi.h> // Functions for file operations and shell interactions

// GDI (Graphics Device Interface)
#include <wingdi.h> // Functions for 2D graphics rendering and drawing

// Shell Object API
#include <shlobj.h> // Functions for shell folder operations and special folder paths

#endif // MAMEUI_LIB_WINAPI_WINAPI_COMMON_H
