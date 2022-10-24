// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_DIRWATCHER_H
#define MAMEUI_WINAPP_DIRWATCHER_H

#pragma once

#include <string>
typedef struct DirWatcher_t DirWatcher, *PDIRWATCHER;

PDIRWATCHER DirWatcher_Init(HWND hwndTarget, UINT nMessage);
BOOL DirWatcher_Watch(PDIRWATCHER pWatcher, WORD nIndex, const std::string t, BOOL bWatchSubtrees);
void DirWatcher_Free(PDIRWATCHER pWatcher);

#endif // MAMEUI_WINAPP_DIRWATCHER_H
