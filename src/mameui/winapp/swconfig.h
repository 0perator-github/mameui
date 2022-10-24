// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************
//============================================================
//
//  swconfig.h
//
//============================================================

#ifndef MAMEUI_WINAPP_SWCONFIG_H
#define MAMEUI_WINAPP_SWCONFIG_H

#pragma once

//============================================================
//  TYPE DEFINITIONS
//============================================================

//typedef struct _software_config software_config;
typedef struct software_config_t
{
	int driver_index;
	const game_driver *gamedrv;
	machine_config *mconfig;
}software_config;



//============================================================
//  PROTOTYPES
//============================================================

software_config *software_config_alloc(int driver_index); //, hashfile_error_func error_proc);
void software_config_free(software_config *config);

#endif // MAMEUI_WINAPP_SWCONFIG_H
