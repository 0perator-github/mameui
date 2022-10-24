// For licensing and usage information, read docs/winui_license.txt
// ============================================================================

// ============================================================================
// swconfig.cpp - Software configuration management
// ============================================================================

// standard C++ headers
#include <filesystem>
#include <memory>

// standard windows headers

// MAME headers
#include "emu.h"

#include "drivenum.h"

#include "ui/moptions.h"
#include "winopts.h"

// MAMEUI headers
#include "emu_opts.h"

#include "swconfig.h"

// Constructor: Initializes software configuration and loads driver options.
//
// Parameters:
//   driver_index - Index of the driver to allocate.
software_config::software_config(int driver_index)
	: m_driver_index{ driver_index },
	m_game_driver{ nullptr },
	m_machine_config{ nullptr }
{
	// Retrieve and store the target game driver reference
	m_game_driver = &driver_list::driver(driver_index);

	// Load the driver options
	windows_options o;
	emu_opts.load_options(o, SOFTWARETYPE_GAME, driver_index, 1);

	// Securely allocate the machine configuration directly into our unique_ptr
	m_machine_config = std::make_unique<machine_config>(*m_game_driver, o);
}
