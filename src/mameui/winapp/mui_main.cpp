// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

/***************************************************************************

  mui_main.cpp

***************************************************************************/

// standard C++ headers
#include <filesystem>
#include <stdexcept>
#include <string>

// standard windows headers
#include "winapi_common.h"

// MAME headers
#include "winmain.h"

// MAMEUI headers
#include "dialog_boxes.h"
#include "system_services.h"

#include "screenshot.h"
#include "winui.h"

//using namespace mameui::util::string_util;
using namespace mameui::winapi;

template <typename Func>
int run_in_try_catch(Func&& main_logic) {
	try {
		return main_logic();
	}
	catch (const std::filesystem::filesystem_error& e) {
		dialog_boxes::message_box_utf8(nullptr,
			(std::string("Filesystem error:\n") + e.what() + "\nExit Code: 1").c_str(),
			"MAMEUI Startup Error", MB_ICONERROR | MB_OK);
	}
	catch (const std::runtime_error& e) {
		dialog_boxes::message_box_utf8(nullptr,
			(std::string("Runtime error:\n") + e.what() + "\nExit Code: 1").c_str(),
			"MAMEUI Error", MB_ICONERROR | MB_OK);
	}
	catch (const std::exception& e) {
		dialog_boxes::message_box_utf8(nullptr,
			(std::string("Unhandled exception:\n") + e.what() + "\nExit Code: 1").c_str(),
			"MAMEUI Error", MB_ICONERROR | MB_OK);
	}
	catch (...) {
		dialog_boxes::message_box_utf8(nullptr,
			"Unknown fatal error occurred. Exit Code: 1",
			"MAMEUI Error", MB_ICONERROR | MB_OK);
	}
	return EXIT_FAILURE;
}

void detach_if_launched_from_explorer()
{
	DWORD processList;
	DWORD processCount = GetConsoleProcessList(&processList, 1);

	if (processCount == 1)
		FreeConsole();
}

// main - entry point for MAMEUI
int main(int argc, char* argv[])
{
	return run_in_try_catch([&]() {

#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
		// Standard Console / Desktop Platform Execution
		return mame_main(argc, argv);

#else

		// If command line arguments are provided. Run MAME, passing the arguments to it.
		// This allows for command line usage of MAMEUI.
		if (argv && argc > 1)
		{
			return mame_main(argc, argv);
		}
		else
		{
			// Detach from console if launched from Explorer to avoid a lingering console window
			detach_if_launched_from_explorer();

			// If you want to process command line arguments directly, uncomment the following
			// line and pass lpUnicodeCmdLine to MameUIMain	instead of nullptr.
			//LPWSTR lpUnicodeCmdLine = system_services::get_commandline();
			HINSTANCE hInstance = system_services::get_module_handle(nullptr);

			return MameUIMain(hInstance, nullptr, SW_SHOWNORMAL);
		}
#endif
		});
}
