// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

// standard C++ headers
#include <algorithm>
#include <memory>
#include <sstream>
#include <string>

// standard windows headers
#include "windows.h"

// MAME headers
#include "mameheaders.h"

// MAMEUI headers
#include "mui_str.h"
#include "mui_wcsconv.h"

#include "winapi_controls.h"
#include "winapi_dialog_boxes.h"
#include "winapi_storage.h"
#include "winapi_system_services.h"
#include "winapi_windows.h"

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

struct DirWatcherEntry_t
{
	PDirWatcherEntry pNext;
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

	std::shared_ptr<std::string> szDirPath;
};


struct DirWatcher_t
{
	HMODULE hKernelModule;
	READDIRECTORYCHANGESFUNC pfnReadDirectoryChanges;

	HWND hwndTarget;
	UINT nMessage;

	HANDLE hRequestEvent;
	HANDLE hResponseEvent;
	HANDLE hThread;
	CRITICAL_SECTION crit;
	PDirWatcherEntry pEntries;

	// These are posted externally
	bool bQuit;
	bool bWatchSubtree;
	WORD nIndex;
	std::shared_ptr<std::string> pszPathList;
};



static void DirWatcher_SetupWatch(PDirWatcher pWatcher, PDirWatcherEntry pEntry)
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



static void DirWatcher_FreeEntry(PDirWatcherEntry pEntry)
{
	if (pEntry)
	{
		if (pEntry->hDir)
			(void)system_services::close_handle(pEntry->hDir);
		if (pEntry->szDirPath)
			pEntry->szDirPath.reset();
		pEntry.reset();
	}
}



static bool DirWatcher_WatchDirectory(PDirWatcher pWatcher, int nIndex, int nSubIndex, std::shared_ptr<std::string> pszPath, bool bWatchSubtree)
{
	PDirWatcherEntry pEntry;
	HANDLE hDir;
	
	pEntry = std::make_shared<DirWatcherEntry>();
	if (!pEntry)
		return 0;

	if (!pszPath)
	{
		DirWatcher_FreeEntry(pEntry);
		return 0;
	}

	pEntry->szDirPath = pszPath;
	pEntry->overlapped.hEvent = pWatcher->hRequestEvent;

	hDir = storage::create_file_utf8(pszPath->c_str(), FILE_LIST_DIRECTORY,
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



static void DirWatcher_Signal(PDirWatcher pWatcher, PDirWatcherEntry pEntry)
{
	std::string file_path, file_name;
	bool bPause = 0;
	HANDLE hFile;

		int nLength = WideCharToMultiByte(CP_ACP, 0, pEntry->u.notify.FileName, -1, 0, 0, 0, 0);
		file_name.reserve(nLength);
		(void)WideCharToMultiByte(CP_ACP, 0, pEntry->u.notify.FileName, -1, &file_name[0], nLength, 0, 0);

	// get the full path to this new file
	file_path = *(pEntry->szDirPath) + "\\" + file_name;

	// attempt to busy wait until any result other than ERROR_SHARING_VIOLATION
	// is generated
	int nTries = 100;
	do
	{
		hFile = storage::create_file_utf8(file_path.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
		if (hFile != INVALID_HANDLE_VALUE)
			system_services::close_handle(hFile);

		bPause = (nTries--) && (hFile == INVALID_HANDLE_VALUE) && (GetLastError() == ERROR_SHARING_VIOLATION);
		if (bPause)
			Sleep(10);
	}
	while(bPause);

	// send the message (assuming that we have a target)
	if (pWatcher->hwndTarget)
	{
		std::unique_ptr<const wchar_t[]> wcs_filename(mui_wcstring_from_utf8(file_name.c_str()));
		WPARAM Indexes = (pEntry->nIndex << 16) | (pEntry->nSubIndex << 0);

		if( !wcs_filename)
			return;
		(void)windows::send_message(pWatcher->hwndTarget, pWatcher->nMessage, Indexes, (LPARAM)(LPCWSTR)wcs_filename.get());
	}

	DirWatcher_SetupWatch(pWatcher, pEntry);
}



static DWORD WINAPI DirWatcher_ThreadProc(LPVOID lpParameter)
{
	auto pWatcher = std::shared_ptr<DirWatcher>(static_cast<DirWatcher*>(lpParameter));
	PDirWatcherEntry pEntry;
	std::shared_ptr<PDirWatcherEntry> ppEntry;

	do
	{
		system_services::wait_for_single_object(pWatcher->hRequestEvent, INFINITE);

		if (pWatcher->pszPathList)
		{
			// remove any entries with the same nIndex
			ppEntry = std::make_shared<PDirWatcherEntry>(pWatcher->pEntries);
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
					ppEntry = std::make_shared<PDirWatcherEntry>((*ppEntry)->pNext);
				}
			}

			// allocate our own copy of the path list
			std::string token;
			std::istringstream tokenStream(*pWatcher->pszPathList);
			for (size_t sub_index=0;std::getline(tokenStream, token, ';');sub_index++)
			{
				if (!token.empty())
				{
					(void)DirWatcher_WatchDirectory(pWatcher, pWatcher->nIndex, sub_index, std::make_shared<std::string>(token), pWatcher->bWatchSubtree);
				}
			}

			pWatcher->pszPathList.reset();
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



PDirWatcher DirWatcher_Init(HWND hwndTarget, UINT nMessage)
{
	DWORD nThreadID = 0;
	auto pWatcher = std::make_shared<DirWatcher>();
	LPSECURITY_ATTRIBUTES security_attribute = 0;
	const wchar_t *event_name = 0;

	// This feature does not exist on Win9x
//	if (GetVersion() >= 0x80000000)
	if (!system_services::is_windows7_or_greater())
		goto error;

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

	pWatcher->hThread = system_services::create_thread(security_attribute, 0, DirWatcher_ThreadProc, static_cast<LPVOID>(pWatcher.get()), 0, &nThreadID);

	pWatcher->hwndTarget = hwndTarget;
	pWatcher->nMessage = nMessage;
	return pWatcher;

error:
	if (pWatcher)
		DirWatcher_Free(pWatcher);
	return 0;
}



bool DirWatcher_Watch(PDirWatcher pWatcher, WORD nIndex, std::shared_ptr<std::string> pszPathList, bool bWatchSubtrees)
{
	EnterCriticalSection(&pWatcher->crit);

	pWatcher->nIndex = nIndex;
	pWatcher->pszPathList = pszPathList;
	pWatcher->bWatchSubtree = bWatchSubtrees;
	(void)system_services::set_event(pWatcher->hRequestEvent);

	(void)system_services::wait_for_single_object(pWatcher->hResponseEvent, INFINITE);
	LeaveCriticalSection(&pWatcher->crit);
	return true;
}



void DirWatcher_Free(PDirWatcher pWatcher)
{
	PDirWatcherEntry pEntry;
	PDirWatcherEntry pNextEntry;

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
		(void)FreeLibrary(pWatcher->hKernelModule);
	if (pWatcher->hRequestEvent)
		(void)system_services::close_handle(pWatcher->hRequestEvent);
	if (pWatcher->hResponseEvent)
		(void)system_services::close_handle(pWatcher->hResponseEvent);
	pWatcher.reset();
}

