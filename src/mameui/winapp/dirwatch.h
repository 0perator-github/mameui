// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_DIRWATCHER_H
#define MAMEUI_WINAPP_DIRWATCHER_H

#pragma once

struct dir_watcher;
using DirWatcher = dir_watcher;
using PDirWatcher = dir_watcher*;

PDirWatcher DirWatcher_Init(HWND hwndTarget, UINT nMessage);
bool DirWatcher_Watch(PDirWatcher pWatcher, WORD nIndex, const std::string_view path_list, bool bWatchSubtrees);
void DirWatcher_Free(PDirWatcher pWatcher);

#endif // MAMEUI_WINAPP_DIRWATCHER_H
