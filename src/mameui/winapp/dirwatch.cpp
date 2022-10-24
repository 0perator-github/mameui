// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

/***************************************************************************

  dirwatch.cpp

***************************************************************************/

// standard C++ headers
#include <algorithm>
#include <filesystem>

// standard windows headers
#include "winapi_common.h"

// MAME headers

// shlwapi.h pulls in objbase.h which defines interface as a struct which conflict with a template declaration in device.h
#ifdef interface 
#undef interface // undef interface which is for COM interfaces
#endif
#include "emu.h"
#define interface struct

// MAMEUI headers
#include "mui_cstr.h"
#include "mui_wcstrconv.h"

#include "windows_controls.h"
#include "dialog_boxes.h"
#include "data_access_storage.h"
#include "system_services.h"
#include "windows_messages.h"

#include "mui_util.h"
#include "screenshot.h"
#include "winui.h"

#include "dirwatch.h"

using namespace mameui::winapi;
using namespace mameui::winapi::controls;

using READDIRECTORYCHANGESFUNC = bool (WINAPI*)(HANDLE hDirectory, LPVOID lpBuffer,
	DWORD nBufferLength, bool bWatchSubtree, DWORD dwNotifyFilter,
	LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped,
	LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

using DirWatcherEntry = struct dir_watcher_entry;
struct dir_watcher_entry
{
	DirWatcherEntry *pNext;
	HANDLE hDir;
	WORD nIndex;
	WORD nSubIndex;
	bool bWatchSubtree;
	OVERLAPPED overlapped;

	union
	{
		FILE_NOTIFY_INFORMATION notify;
		BYTE buffer[1024];
	} u;

	char *szDirPath;
};

struct dir_watcher
{
	HMODULE hKernelModule;
	READDIRECTORYCHANGESFUNC pfnReadDirectoryChanges;

	HWND hwndTarget;
	UINT nMessage;

	HANDLE hRequestEvent;
	HANDLE hResponseEvent;
	HANDLE hThread;
	CRITICAL_SECTION crit;
	DirWatcherEntry* pEntries;

	// These are posted externally
	bool bQuit;
	bool bWatchSubtree;
	WORD nIndex;
	LPCSTR pszPathList;
};



static void DirWatcher_SetupWatch(PDIRWATCHER pWatcher, DirWatcherEntry *pEntry)
{
	DWORD nDummy;

	pEntry->u = {};

	pWatcher->pfnReadDirectoryChanges(pEntry->hDir,
		&pEntry->u,
		sizeof(pEntry->u),
		pEntry->bWatchSubtree,
		FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE,
		&nDummy,
		&pEntry->overlapped,
		0);
}



static void DirWatcher_FreeEntry(DirWatcherEntry *pEntry)
{
	if (pEntry)
	{
		if (pEntry->hDir)
			(void)system_services::close_handle(pEntry->hDir);
		if (pEntry->szDirPath)
			delete[] pEntry->szDirPath;
		delete pEntry;
	}
}



static bool DirWatcher_WatchDirectory(PDIRWATCHER pWatcher, int nIndex, int nSubIndex, LPCSTR pszPath, bool bWatchSubtree)
{
	DirWatcherEntry *pEntry;
	HANDLE hDir;
	
	pEntry = new DirWatcherEntry{};
	if (!pEntry)
		return 0;

	const size_t path_size = mui_strlen(pszPath) + 1;
	pEntry->szDirPath = new char[path_size];
	if (!pEntry->szDirPath)
	{
		DirWatcher_FreeEntry(pEntry);
		return 0;
	}

	(void)mui_strcpy(pEntry->szDirPath, pszPath);
	pEntry->overlapped.hEvent = pWatcher->hRequestEvent;

	hDir = storage::create_file_utf8(pszPath, FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		0, OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, 0);
	if (!hDir || (hDir == INVALID_HANDLE_VALUE))
	{
		DirWatcher_FreeEntry(pEntry);
		return 0;
	}

	// Populate the entry
	pEntry->hDir = hDir;
	pEntry->bWatchSubtree = bWatchSubtree;
	pEntry->nIndex = nIndex;
	pEntry->nSubIndex = nSubIndex;

	// Link in the entry
	pEntry->pNext = pWatcher->pEntries;
	pWatcher->pEntries = pEntry;

	DirWatcher_SetupWatch(pWatcher, pEntry);

	return 1;
}



static void DirWatcher_Signal(PDIRWATCHER pWatcher, DirWatcherEntry *pEntry)
{
	std::filesystem::path file_path(pEntry->szDirPath);
	bool bPause = 0;
	HANDLE hFile;

	// get the full path to this new file
	file_path /= pEntry->u.notify.FileName;
	std::wstring file_name = file_path.wstring();

	// attempt to busy wait until any result other than ERROR_SHARING_VIOLATION
	// is generated
	int nTries = 100;
	do
	{
		hFile = storage::create_file(file_name.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
		if (hFile != INVALID_HANDLE_VALUE)
			system_services::close_handle(hFile);

		bPause = (nTries--) && (hFile == INVALID_HANDLE_VALUE) && (system_services::get_last_error() == ERROR_SHARING_VIOLATION);
		if (bPause)
			Sleep(10);
	}
	while(bPause);

	// send the message (assuming that we have a target)
	if (pWatcher->hwndTarget)
	{
		(void)windows::send_message(pWatcher->hwndTarget, pWatcher->nMessage, (pEntry->nIndex << 16) | (pEntry->nSubIndex << 0), (LPARAM)(LPCWSTR)file_name.c_str());
	}

	DirWatcher_SetupWatch(pWatcher, pEntry);
}



static DWORD WINAPI DirWatcher_ThreadProc(LPVOID lpParameter)
{
	LPSTR pszPathList, s;
	int nSubIndex = 0;
	PDIRWATCHER pWatcher = (PDIRWATCHER) lpParameter;
	DirWatcherEntry *pEntry;
	DirWatcherEntry **ppEntry;

	do
	{
		system_services::wait_for_single_object(pWatcher->hRequestEvent, INFINITE);

		if (pWatcher->pszPathList)
		{
			// remove any entries with the same nIndex
			ppEntry = &pWatcher->pEntries;
			while(*ppEntry)
			{
				if ((*ppEntry)->nIndex == pWatcher->nIndex)
				{
					pEntry = *ppEntry;
					*ppEntry = pEntry->pNext;
					DirWatcher_FreeEntry(pEntry);
				}
				else
				{
					ppEntry = &(*ppEntry)->pNext;
				}
			}

			// allocate our own copy of the path list
			const size_t list_size = mui_strlen(pWatcher->pszPathList) + 1;
			pszPathList = (LPSTR) new char[list_size];
			(void)mui_strcpy(pszPathList,pWatcher->pszPathList);

			nSubIndex = 0;
			do
			{
				s = strchr(pszPathList, ';');
				if (s)
					*s = '\0';

				if (*pszPathList)
				{
					(void)DirWatcher_WatchDirectory(pWatcher, pWatcher->nIndex,
						nSubIndex++, pszPathList, pWatcher->bWatchSubtree);
				}

				pszPathList = s ? s + 1 : 0;
			}
			while(pszPathList);

			pWatcher->pszPathList = 0;
			pWatcher->bWatchSubtree = false;
		}
		else
		{
			// we have to go through the list and find what has been hit
			for (pEntry = pWatcher->pEntries; pEntry; pEntry = pEntry->pNext)
			{
				if (pEntry->u.notify.Action != 0)
				{
					DirWatcher_Signal(pWatcher, pEntry);
				}
			}
		}

		(void)system_services::set_event(pWatcher->hResponseEvent);
	}
	while(!pWatcher->bQuit);
	return 0;
}



PDIRWATCHER DirWatcher_Init(HWND hwndTarget, UINT nMessage)
{
	DWORD nThreadID = 0;
	PDIRWATCHER pWatcher = 0;
	LPSECURITY_ATTRIBUTES security_attribute = 0;
	const wchar_t *event_name = 0;

	// This feature does not exist on Win9x
//	if (GetVersion() >= 0x80000000)
	if (!system_services::is_windows7_or_greater())
		goto error;

	pWatcher = new DirWatcher{};
	if (!pWatcher)
		goto error;

	system_services::initialize_critical_section(&pWatcher->crit);

	pWatcher->hKernelModule = system_services::load_library(L"kernel32.dll");
	if (!pWatcher->hKernelModule)
		goto error;

	pWatcher->pfnReadDirectoryChanges = (READDIRECTORYCHANGESFUNC)system_services::get_proc_address(pWatcher->hKernelModule, L"ReadDirectoryChangesW");
	if (!pWatcher->pfnReadDirectoryChanges)
		goto error;

	pWatcher->hRequestEvent = system_services::create_event(security_attribute, false, false, event_name);
	if (!pWatcher->hRequestEvent)
		goto error;

	pWatcher->hResponseEvent = system_services::create_event(security_attribute, false, false, event_name);
	if (!pWatcher->hRequestEvent)
		goto error;

	pWatcher->hThread = system_services::create_thread(security_attribute, 0, DirWatcher_ThreadProc, pWatcher, 0, &nThreadID);

	pWatcher->hwndTarget = hwndTarget;
	pWatcher->nMessage = nMessage;
	return pWatcher;

error:
	if (pWatcher)
		DirWatcher_Free(pWatcher);
	return 0;
}



bool DirWatcher_Watch(PDIRWATCHER pWatcher, WORD nIndex, const std::string pszPathList, bool bWatchSubtrees)
{
	EnterCriticalSection(&pWatcher->crit);

	pWatcher->nIndex = nIndex;
	pWatcher->pszPathList = pszPathList.c_str();
	pWatcher->bWatchSubtree = bWatchSubtrees;
	(void)system_services::set_event(pWatcher->hRequestEvent);

	(void)system_services::wait_for_single_object(pWatcher->hResponseEvent, INFINITE);
	LeaveCriticalSection(&pWatcher->crit);
	return true;
}



void DirWatcher_Free(PDIRWATCHER pWatcher)
{
	DirWatcherEntry *pEntry;
	DirWatcherEntry *pNextEntry;

	if (pWatcher->hThread)
	{
		EnterCriticalSection(&pWatcher->crit);
		pWatcher->bQuit = true;
		(void)system_services::set_event(pWatcher->hRequestEvent);
		(void)system_services::wait_for_single_object(pWatcher->hThread, 1000);
		LeaveCriticalSection(&pWatcher->crit);
		(void)system_services::close_handle(pWatcher->hThread);
	}

	DeleteCriticalSection(&pWatcher->crit);

	pEntry = pWatcher->pEntries;
	while(pEntry)
	{
		pNextEntry = pEntry->pNext;
		DirWatcher_FreeEntry(pEntry);
		pEntry = pNextEntry;
	}

	if (pWatcher->hKernelModule)
		(void)system_services::free_library(pWatcher->hKernelModule);
	if (pWatcher->hResponseEvent)
		(void)system_services::close_handle(pWatcher->hResponseEvent);
	delete pWatcher;
}

