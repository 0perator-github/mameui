// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************
#ifndef WINUI
#define WINUI

#include <windows.h>
#include "winui.h"


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

	extern int mui_main(int argc, char* argv[]);

	if (__argc > 1)
		return mui_main(__argc, __argv);
	else
		return MameUIMain(hInstance, GetCommandLineW(), nCmdShow);
}

#endif
