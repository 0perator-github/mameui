// For licensing and usage information, read docs/winui_license.txt
//  MASTER
//****************************************************************************

#ifndef MAMEUI_WINAPP_HISTORY_H
#define MAMEUI_WINAPP_HISTORY_H

#pragma once

char * GetGameHistory(int driver_index);  // Arcade-only builds (HBMAME, ARCADE)
char * GetGameHistory(int driver_index, std::string software); // Builds with software support (MESSUI, MAMEUI)

#endif // MAMEUI_WINAPP_HISTORY_H
