// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************
//============================================================
//
//  swconfig.cpp
//
//============================================================

// standard C++ headers

// standard windows headers

// MAME headers
#include "mameheaders.h"

// MAMEUI headers
#include "emu_opts.h"

#include "swconfig.h"

//============================================================
//  IMPLEMENTATION
//============================================================

software_config *software_config_alloc(int driver_index) //, hashfile_error_func error_proc)
{
	software_config *config;

	// allocate the software_config
	config = new software_config{};

	// allocate the machine config
	windows_options o;
	load_options(o, OPTIONS_GAME, driver_index, 1);  // need software loaded via optional slots
	config->mconfig = new machine_config(driver_list::driver(driver_index), o);

	// other stuff
	config->driver_index = driver_index;
	config->gamedrv = &driver_list::driver(driver_index);

	return config;
}



void software_config_free(software_config *config)
{
	if (config->mconfig)
	{
		delete config->mconfig;
	}

	/*if (config->hashfile != NULL)
	{
	    hashfile_close(config->hashfile);
	    config->hashfile = NULL;
	}*/

	delete config;
}
