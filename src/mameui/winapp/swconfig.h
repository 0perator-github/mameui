// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************
//============================================================
//
//  swconfig.h
//
//============================================================

#ifndef MAMEUI_WINAPP_SWCONFIG_H
#define MAMEUI_WINAPP_SWCONFIG_H

//============================================================
//  TYPE DEFINITIONS
//============================================================

using software_config = struct software_config
{
	int driver_index;
	const game_driver *gamedrv;
	machine_config *mconfig;
};

//============================================================
//  PROTOTYPES
//============================================================

software_config *software_config_alloc(int driver_index); //, hashfile_error_func error_proc);
void software_config_free(software_config *config);

#endif // MAMEUI_WINAPP_SWCONFIG_H
