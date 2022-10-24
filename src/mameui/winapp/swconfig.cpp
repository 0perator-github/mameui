//	For licensing and usage information, read docs/winui_license.txt
//	===========================================================================

//	===========================================================================
//	swconfig.cpp - Software configuration management
//	===========================================================================

// standard C++ headers
#include <filesystem>

// standard windows headers

// MAME headers
#include "emu.h"

#include "drivenum.h"

#include "ui/moptions.h"
#include "winopts.h"

// MAMEUI headers
#include "emu_opts.h"

#include "swconfig.h"

//	===========================================================================
//	FUNCTIONS
//	===========================================================================

//	-------------------------------------------------------
//	software_config_alloc - allocate a software_config structure
//	Parameters:
//		driver_index - index of the driver to allocate
//	Returns:
//		software_config * - pointer to the allocated software_config structure
//	-------------------------------------------------------

software_config *software_config_alloc(int driver_index) //, hashfile_error_func error_proc)
{
	software_config *config;

	// allocate the software_config
	config = new software_config{};

	// allocate the machine config
	windows_options o;
	emu_opts.load_options(o, SOFTWARETYPE_GAME, driver_index, 1);  // need software loaded via optional slots
	config->mconfig = new machine_config(driver_list::driver(driver_index), o);

	// other stuff
	config->driver_index = driver_index;
	config->gamedrv = &driver_list::driver(driver_index);

	return config;
}

//	-------------------------------------------------------
//	software_config_free - releases a software_config structure
//	from memory
//	Parameters:
//		config - pointer to the software_config structure
//	-------------------------------------------------------

void software_config_free(software_config *config)
{
	if (config->mconfig)
	{
		delete config->mconfig;
	}

	//if (config->hashfile != nullptr)
	//{
	//    hashfile_close(config->hashfile);
	//    config->hashfile = nullptr;
	//}

	delete config;
}
