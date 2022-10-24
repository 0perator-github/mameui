// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_DIRWATCHER_H
#define MAMEUI_WINAPP_DIRWATCHER_H

#pragma once

struct DirWatcher_t;
using DirWatcher = DirWatcher_t;
using PDirWatcher = std::shared_ptr<DirWatcher_t>;

struct DirWatcherEntry_t;
using DirWatcherEntry = DirWatcherEntry_t;
using PDirWatcherEntry = std::shared_ptr < DirWatcherEntry_t>;

PDirWatcher DirWatcher_Init(HWND hwndTarget, UINT nMessage);
bool DirWatcher_Watch(PDirWatcher pWatcher, WORD nIndex, std::shared_ptr<std::string> pszPathList, bool bWatchSubtrees);
void DirWatcher_Free(PDirWatcher pWatcher);

#endif // MAMEUI_WINAPP_DIRWATCHER_H
