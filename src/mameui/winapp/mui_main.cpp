// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

// standard windows headers
#include "windows.h"
#include "commctrl.h"
#include "commdlg.h"
#include "shellapi.h"

// MAME headers
#include "mameheaders.h"

// MAMEUI headers
#include "screenshot.h"
#include "winui.h"

#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
int main(int argc, char* argv[])
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
{
	LPWSTR lpUnicodeCmdLine = GetCommandLineW();
	int pNumArgs;

	CommandLineToArgvW(lpUnicodeCmdLine, &pNumArgs);
	if (pNumArgs > 1)
	{

		return mame_main(pNumArgs, &lpCmdLine);
	}
	else
		return MameUIMain(hInstance, lpUnicodeCmdLine, nCmdShow);
}
