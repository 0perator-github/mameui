// For licensing and usage information, read docs/winui_license.txt
//  MASTER
//****************************************************************************

#ifndef MAMEUI_WINAPP_HISTORY_H
#define MAMEUI_WINAPP_HISTORY_H

#pragma once

std::string GetGameHistory(int driver_index, std::string_view software = ""); // Builds with software support (MESSUI, MAMEUI)

#endif // MAMEUI_WINAPP_HISTORY_H
