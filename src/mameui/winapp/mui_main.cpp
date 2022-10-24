// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

/***************************************************************************

  mui_main.cpp

***************************************************************************/

// standard C++ headers
#include <filesystem>

// standard windows headers
#include "winapi_common.h"

// MAME headers
#include "winmain.h"

// MAMEUI headers
#include "dialog_boxes.h"
#include "system_services.h"
#include "windows_shell.h"

#include "screenshot.h"
#include "winui.h"

using namespace mameui::winapi;

#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
int main(int argc, char* argv[])
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
{
	try
	{
		LPWSTR lpUnicodeCmdLine = system_services::get_commandline();
		int pNumArgs = 0;

		shell::commandline_to_argv(lpUnicodeCmdLine, &pNumArgs);
		if (pNumArgs > 1)
			return mame_main(pNumArgs, &lpCmdLine);
		else
			return MameUIMain(hInstance, lpUnicodeCmdLine, nCmdShow);
	}
	catch (const std::filesystem::filesystem_error& e)
	{
		dialog_boxes::message_box_utf8(nullptr,
			(std::string("Filesystem error:\n") + e.what()).c_str(),
			"MAMEUI Startup Error",
			MB_ICONERROR | MB_OK);
		return EXIT_FAILURE;
	}
	catch (const std::exception& e)
	{
		dialog_boxes::message_box_utf8(nullptr,
			(std::string("Unhandled exception:\n") + e.what()).c_str(),
			"MAMEUI Error",
			MB_ICONERROR | MB_OK);
		return EXIT_FAILURE;
	}
	catch (...)
	{
		dialog_boxes::message_box_utf8(nullptr,
			"Unknown fatal error occurred.",
			"MAMEUI Error",
			MB_ICONERROR | MB_OK);
		return EXIT_FAILURE;
	}
}

