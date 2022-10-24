// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_FILE_H
#define MAMEUI_WINAPP_FILE_H

#pragma once

// from windows fileio.c
extern void set_pathlist(int file_type,const char *new_rawpath);

constexpr auto OSD_FILETYPE_ICON = 1001;

#endif // MAMEUI_WINAPP_FILE_H
