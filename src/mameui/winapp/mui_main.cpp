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
	LPWSTR lpUnicodeCmdLine = system_services::get_commandline();
	int pNumArgs;

	shell::commandline_to_argv(lpUnicodeCmdLine, &pNumArgs);
	if (pNumArgs > 1)
	{

		return mame_main(pNumArgs, &lpCmdLine);
	}
	else
		return MameUIMain(hInstance, lpUnicodeCmdLine, nCmdShow);
}
