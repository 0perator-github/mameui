// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

 /***************************************************************************

  mui_audit.cpp

  Audit dialog

***************************************************************************/

// standard C++ headers
#include <filesystem>
#include <iostream>
#include <string>

// standard windows headers
#include "winapi_common.h"
#include <richedit.h>   //MAMEUI: This has to be included last.

// MAME headers

// richedit.h pulls in objbase.h which defines interface as a struct which conflict with a template declaration in device.h
#ifdef interface
#undef interface // undef interface which is for COM interfaces
#endif
#include "emu.h"
#ifndef interface
#define interface struct // define interface as struct again
#endif

#include "drivenum.h"
#include "romload.h"
#include "audit.h"
#include "strconv.h"

#include "ui/moptions.h"
#include "winopts.h"

// MAMEUI headers
#include "mui_wcstrconv.h"

#include "windows_controls.h"
#include "dialog_boxes.h"
#include "menus_other_res.h"
#include "system_services.h"
#include "windows_messages.h"

#include "emu_opts.h"
#include "mui_opts.h"
#include "mui_util.h"
#include "properties.h"
#include "resource.h"
#include "screenshot.h"
#include "winui.h"

#include "mui_audit.h"

using namespace mameui::util::string_util;
using namespace mameui::winapi::controls;
using namespace mameui::winapi;

/***************************************************************************
    function prototypes
 ***************************************************************************/

static DWORD WINAPI AuditThreadProc(LPVOID hDlg);
static INT_PTR CALLBACK AuditWindowProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static HRESULT AuditProcInitDlgMsg(HWND hDlg);
static HRESULT AuditProcCmdMsg(HWND hDlg, WPARAM wParam, LPARAM lParam);
static HRESULT GameAuditProcInitDlgMsg(HWND hDlg);
static HRESULT GameAuditProcTimerMsg(HWND hDlg);
static void ProcessNextRom(void);
static void ProcessNextSample(void);
static void CLIB_DECL DetailsPrintf(int box, const char *fmt, ...) ATTR_PRINTF(2, 3);
static const char *StatusString(int iStatus);

/***************************************************************************
    Internal variables
 ***************************************************************************/

#define MAX_AUDITBOX_TEXT   0x7FFFFFFE

static volatile HWND hAudit;
static volatile int rom_index = 0;
static volatile int roms_correct = 0;
static volatile int roms_incorrect = 0;
static volatile int sample_index = 0;
static volatile int samples_correct = 0;
static volatile int samples_incorrect = 0;
static volatile bool bPaused = false;
static volatile bool bCancel = false;
static int m_choice = 0;
static HANDLE hThread;
static DWORD dwThreadID = 0;

/***************************************************************************
    External functions
 ***************************************************************************/

//static int strcatvprintf(std::string &str, const char *format, va_list args)
//{
//  int result;
//  std::unique_ptr<char[]> buffer;
//  va_list ap_copy;
//
//  va_copy(ap_copy, args);
//  result = vsnprintf(nullptr, 0, format, ap_copy) + 1;
//  va_end(ap_copy);
//  buffer = std::make_unique<char[]>(result);
//
//  result = vsnprintf(buffer.get(), result, format, args);
//  if (buffer)
//      str.append(buffer.get());
//
//  return result;
//}

//static int strcatprintf(std::string &str, const char *format, ...)
//{
//  va_list ap;
//  va_start(ap, format);
//  int retVal = strcatvprintf(str, format, ap);
//  va_end(ap);
//  return retVal;
//}

void AuditDialog(HWND hParent, int choice)
{
	rom_index         = 0;
	roms_correct      = -1; // ___empty must not be counted
	roms_incorrect    = 0;
	sample_index      = 0;
	samples_correct   = -1; // ___empty must not be counted
	samples_incorrect = 0;
	m_choice = choice;

	//RS use Riched32.dll
	HMODULE hModule = system_services::load_library(L"Riched32.dll");
	if( hModule )
	{
		(void)dialog_boxes::dialog_box(system_services::get_module_handle(nullptr),menus::make_int_resource(IDD_AUDIT),hParent,AuditWindowProc, 0L);
		system_services::free_library( hModule );
		hModule = nullptr;
	}
	else
		dialog_boxes::message_box(GetMainWindow(),L"Unable to Load Riched32.dll",L"Error", MB_OK | MB_ICONERROR);
}

void InitGameAudit(int gameIndex)
{
	rom_index = gameIndex;
}

std::wstring_view GetAuditStringView(int audit_result)
{
	switch (audit_result)
	{
	case media_auditor::CORRECT :
	case media_auditor::BEST_AVAILABLE :
	case media_auditor::NONE_NEEDED :
		return L"Yes";

	case media_auditor::NOTFOUND :
	case media_auditor::INCORRECT :
		return L"No";

	default:
		if (audit_result == -1)
			std::cout << "GetAuditString: Audit value -1, try doing a full F5 audit" << "\n";
		else
			std::cout << "GetAuditString: Unknown audit value " << audit_result << "\n";
	}

	return L"?";
}

std::wstring GetAuditString(int audit_result)
{
	return std::wstring(GetAuditStringView(audit_result));
}

bool IsAuditResultKnown(int audit_result)
{
	return true;
}

bool IsAuditResultYes(int audit_result)
{
	return audit_result == media_auditor::CORRECT
		|| audit_result == media_auditor::BEST_AVAILABLE
		|| audit_result == media_auditor::NONE_NEEDED;
}

bool IsAuditResultNo(int audit_result)
{
	return audit_result == media_auditor::NOTFOUND
		|| audit_result == media_auditor::INCORRECT;
}


/***************************************************************************
    Internal functions
 ***************************************************************************/
// Verifies the ROM set while calling SetRomAuditResults
int MameUIVerifyRomSet(int game, bool choice)
{
	driver_enumerator enumerator(emu_opts.GetGlobalOpts(), driver_list::driver(game));
	enumerator.next();
	media_auditor auditor(enumerator);
	media_auditor::summary summary = auditor.audit_media(AUDIT_VALIDATE_FAST);

	std::string summary_string;

	if (summary == media_auditor::NOTFOUND)
	{
		if (m_choice < 2)
			summary_string = util::string_format("%s: Romset NOT FOUND\n", driver_list::driver(game).name);
	}
	else
	{
		std::ostringstream whatever;
		 if (choice)
			auditor.winui_summarize(driver_list::driver(game).name, &whatever); // audit all games
		 else
			auditor.summarize(driver_list::driver(game).name, &whatever); // audit one game
		summary_string = whatever.str();
	}

	// output the summary of the audit
	DetailsPrintf(0, "%s", summary_string.c_str());

	SetRomAuditResults(game, summary);
	return summary;
}

// Verifies the Sample set while calling SetSampleAuditResults
int MameUIVerifySampleSet(int game)
{
	driver_enumerator enumerator(emu_opts.GetGlobalOpts(), driver_list::driver(game));
	enumerator.next();
	media_auditor auditor(enumerator);
	media_auditor::summary summary = auditor.audit_samples();

	std::string summary_string;

	if (summary == media_auditor::NOTFOUND)
		summary_string = util::string_format("%s: Sampleset NOT FOUND\n", driver_list::driver(game).name);
	else
	{
		std::ostringstream whatever;
		auditor.summarize(driver_list::driver(game).name, &whatever);
		summary_string = whatever.str();
	}

	// output the summary of the audit
	DetailsPrintf(1, "%s", summary_string.c_str());

	SetSampleAuditResults(game, summary);
	return summary;
}

static DWORD WINAPI AuditThreadProc(LPVOID hDlg)
{

	while (!bCancel)
	{
		if (!bPaused)
		{
			std::string checking_string;

			if (rom_index != -1)
			{
				checking_string = util::string_format("Checking Set %s - %s", driver_list::driver(rom_index).name, driver_list::driver(rom_index).type.fullname());
				windows::set_window_text_utf8((HWND)hDlg, checking_string.c_str());
				ProcessNextRom();
			}
			else
			if (sample_index != -1)
			{
				checking_string = util::string_format("Checking Set %s - %s", driver_list::driver(sample_index).name, driver_list::driver(sample_index).type.fullname());
				windows::set_window_text_utf8((HWND)hDlg, checking_string.c_str());
				ProcessNextSample();
			}
			else
			{
				windows::set_window_text_utf8((HWND)hDlg, "File Audit");
				(void)windows::enable_window(dialog_boxes::get_dlg_item((HWND)hDlg, IDPAUSE), false);
				ExitThread(1);
			}
		}
	}
	return 0;
}

static HRESULT AuditProcInitDlgMsg(HWND hDlg)
{
	HWND hEdit;

	hAudit = hDlg;
	hEdit = dialog_boxes::get_dlg_item(hAudit, IDC_AUDIT_DETAILS);  //RS 20030613 Set Bkg of RichEdit Ctrl
	if (hEdit)
	{
		(void) windows::send_message(hEdit, EM_SETBKGNDCOLOR, false, windows::get_sys_color(COLOR_BTNFACE));
		(void) windows::send_message(hEdit, EM_SETLIMITTEXT, MAX_AUDITBOX_TEXT, 0);    // MSH - Set to max
	}

	(void)dialog_boxes::send_dlg_item_message(hDlg, IDC_ROMS_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, driver_list::total()));
	(void)dialog_boxes::send_dlg_item_message(hDlg, IDC_SAMPLES_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, driver_list::total()));
	bPaused = false;
	bCancel = false;
	rom_index = 0;
	hThread = CreateThread(nullptr, 0, AuditThreadProc, hDlg, 0, &dwThreadID);

	return true;
}

static HRESULT AuditProcCmdMsg(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDCANCEL:
	{
		bPaused = false;
		if (hThread)
		{
			bCancel = true;
			DWORD dwExitCode = 0;
			if (GetExitCodeThread(hThread, &dwExitCode) && (dwExitCode == STILL_ACTIVE))
				(void)windows::post_message(hDlg, WM_COMMAND, wParam, lParam);
			else
			{
				system_services::close_handle(hThread);
				dialog_boxes::end_dialog(hDlg, 0);
				m_choice = 0;
			}
		}
		return true;
	}
	case IDPAUSE:
	{
		if (bPaused)
		{
			(void)dialog_boxes::send_dlg_item_message(hDlg, IDPAUSE, WM_SETTEXT, 0, (LPARAM)L"Pause");
			bPaused = false;
		}
		else
		{
			(void)dialog_boxes::send_dlg_item_message(hDlg, IDPAUSE, WM_SETTEXT, 0, (LPARAM)L"Continue");
			bPaused = true;
		}
		return true;
	}
	default:
	{   return false;   }
	}
}

static INT_PTR CALLBACK AuditWindowProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
	{   return AuditProcInitDlgMsg(hDlg);   }
	case WM_COMMAND:
	{   return AuditProcCmdMsg(hDlg, wParam, lParam);   }
	default:
	{   return false;   }
	}
}

static HRESULT GameAuditProcInitDlgMsg(HWND hDlg)
{
	FlushFileCaches();
	hAudit = hDlg;
	windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_TITLE), GameInfoTitle(SOFTWARETYPE_GAME, rom_index).c_str());
	SetTimer(hDlg, 0, 1, nullptr);

	return true;
}

static HRESULT GameAuditProcTimerMsg(HWND hDlg)
{
	int iStatus;
	LPCSTR lpStatus;

	KillTimer(hDlg, 0);

	iStatus = MameUIVerifyRomSet(rom_index, 0);
	lpStatus = DriverUsesRoms(rom_index) ? StatusString(iStatus) : "None required";

	windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_ROMS), lpStatus);

	if (DriverUsesSamples(rom_index))
	{
		iStatus = MameUIVerifySampleSet(rom_index);
		lpStatus = StatusString(iStatus);
	}
	else
	{
		lpStatus = "None Required";
	}

	windows::set_window_text_utf8(dialog_boxes::get_dlg_item(hDlg, IDC_PROP_SAMPLES), lpStatus);

	 windows::show_window(hDlg, SW_SHOW);

	return true;
}

INT_PTR CALLBACK GameAuditDialogProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam)    // Callback for the Audit property sheet
{
	switch (Msg)
	{
	case WM_INITDIALOG:
	{   return GameAuditProcInitDlgMsg(hDlg);   }
	case WM_TIMER:
	{   return GameAuditProcTimerMsg(hDlg); }
	default:
	{   return false;   }
	}
}

static void ProcessNextRom()
{
	int retval = 0;

	retval = MameUIVerifyRomSet(rom_index, 1);
	switch (retval)
	{
	case media_auditor::BEST_AVAILABLE: // correct, incorrect or separate count?
	case media_auditor::CORRECT:
	case media_auditor::NONE_NEEDED:
		roms_correct++;
		(void)dialog_boxes::send_dlg_item_message(hAudit, IDC_ROMS_CORRECT, WM_SETTEXT, 0, (LPARAM)std::to_wstring(roms_correct).c_str());
		(void)dialog_boxes::send_dlg_item_message(hAudit, IDC_ROMS_TOTAL, WM_SETTEXT, 0, (LPARAM)std::to_wstring(roms_correct + roms_incorrect).c_str());
		break;

	case media_auditor::NOTFOUND:
	case media_auditor::INCORRECT:
		roms_incorrect++;
		(void)dialog_boxes::send_dlg_item_message(hAudit, IDC_ROMS_INCORRECT, WM_SETTEXT, 0, (LPARAM)std::to_wstring(roms_incorrect).c_str());
		(void)dialog_boxes::send_dlg_item_message(hAudit, IDC_ROMS_TOTAL, WM_SETTEXT, 0, (LPARAM)std::to_wstring(roms_correct + roms_incorrect).c_str());
		break;
	}

	rom_index++;
	(void)dialog_boxes::send_dlg_item_message(hAudit, IDC_ROMS_PROGRESS, PBM_SETPOS, rom_index, 0);

	if (rom_index == driver_list::total())
	{
		sample_index = 0;
		rom_index = -1;
	}
}

static void ProcessNextSample()
{
	int retval = 0;

	retval = MameUIVerifySampleSet(sample_index);

	switch (retval)
	{
	case media_auditor::NOTFOUND:
	case media_auditor::INCORRECT:
		if (DriverUsesSamples(sample_index))
		{
			samples_incorrect++;
			(void)dialog_boxes::send_dlg_item_message(hAudit, IDC_SAMPLES_INCORRECT, WM_SETTEXT, 0, (LPARAM)std::to_wstring(samples_incorrect).c_str());
			(void)dialog_boxes::send_dlg_item_message(hAudit, IDC_SAMPLES_TOTAL, WM_SETTEXT, 0, (LPARAM)std::to_wstring(samples_correct + samples_incorrect).c_str());
		}
		break;
	default:
		if ((DriverUsesSamples(sample_index)) || (m_choice == 1))
		{
			samples_correct++;
			(void)dialog_boxes::send_dlg_item_message(hAudit, IDC_SAMPLES_CORRECT, WM_SETTEXT, 0, (LPARAM)std::to_wstring(samples_correct).c_str());
			(void)dialog_boxes::send_dlg_item_message(hAudit, IDC_SAMPLES_TOTAL, WM_SETTEXT, 0, (LPARAM)std::to_wstring(samples_correct + samples_incorrect).c_str());
		}
		break;
	}

	sample_index++;
	(void)dialog_boxes::send_dlg_item_message(hAudit, IDC_SAMPLES_PROGRESS, PBM_SETPOS, sample_index, 0);

	if (sample_index == driver_list::total())
	{
		DetailsPrintf(1, "Audit complete.\n");
		(void)dialog_boxes::send_dlg_item_message(hAudit, IDCANCEL, WM_SETTEXT, 0, (LPARAM)L"Close");
		sample_index = -1;
	}
}

static void DetailsPrintf(int box, const char *fmt, ...)
{
	//RS 20030613 Different Ids for Property Page and Dialog
	// so see which one's currently instantiated
	HWND hEdit = dialog_boxes::get_dlg_item(hAudit, IDC_AUDIT_DETAILS);
	if (!hEdit)
	{
		if (box == 0)
			hEdit = dialog_boxes::get_dlg_item(hAudit, IDC_AUDIT_DETAILS_PROP0);
		else if (box == 1)
			hEdit = dialog_boxes::get_dlg_item(hAudit, IDC_AUDIT_DETAILS_PROP1);
	}

	if (!hEdit)
	{
		// Auditing via F5 - no window to display the results
//      std::cout << "audit detailsprintf() can't find any audit control" << "\n";
		return;
	}

	va_list args;
	va_list args_copy;


	va_start(args, fmt);
	va_copy(args_copy, args);

	const int len = vsnprintf(nullptr, 0, fmt, args_copy);

	va_end(args_copy);

	if (len < 0)
	{
		va_end(args);
		return;
	}

	std::vector<char> buffer(len + 1);
	vsnprintf(buffer.data(), buffer.size(), fmt, args);

	va_end(args);

	std::wstring audit_details = mui_utf16_from_utf8string(ConvertToWindowsNewlines(buffer.data()));
	int textLength = edit_control::get_text_length(hEdit);
	edit_control::set_sel(hEdit, textLength, textLength);
	edit_control::replace_sel(hEdit, audit_details.c_str());
}

static const char  *StatusString(int iStatus)
{
	static const char *ptr = "Unknown";

	switch (iStatus)
	{
	case media_auditor::CORRECT:
		ptr = "Passed";
		break;

	case media_auditor::BEST_AVAILABLE:
		ptr = "Best available";
		break;

	case media_auditor::NONE_NEEDED:
		ptr = "None Required";
		break;

	case media_auditor::NOTFOUND:
		ptr = "Not found";
		break;

	case media_auditor::INCORRECT:
		ptr = "Failed";
		break;
	}

	return ptr;
}
