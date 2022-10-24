// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_DIRWATCHER_H
#define MAMEUI_WINAPP_DIRWATCHER_H

#pragma once

struct dir_watcher;
using DirWatcher = dir_watcher;
using PDIRWATCHER = dir_watcher*;

PDIRWATCHER DirWatcher_Init(HWND hwndTarget, UINT nMessage);
bool DirWatcher_Watch(PDIRWATCHER pWatcher, WORD nIndex, const std::string t, bool bWatchSubtrees);
void DirWatcher_Free(PDIRWATCHER pWatcher);

#endif // MAMEUI_WINAPP_DIRWATCHER_H
