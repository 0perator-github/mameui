// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

/***************************************************************************

  directinput.cpp

  Direct Input routines.

 ***************************************************************************/

// standard C++ headers
#include <cstdint>

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
#include "system_services.h"

#include "mui_util.h" // For ErrorMsg

#include "directinput.h"

using namespace mameui::winapi;

/***************************************************************************
    function prototypes
 ***************************************************************************/

/***************************************************************************
    External variables
 ***************************************************************************/

/***************************************************************************
    Internal structures
 ***************************************************************************/

/***************************************************************************
    Internal variables
 ***************************************************************************/

static LPDIRECTINPUT di = nullptr;

static HANDLE hDLL = nullptr;

/***************************************************************************
    External functions
 ***************************************************************************/

/****************************************************************************
 *      DirectInputInitialize
 *
 *      Initialize the DirectInput variables.
 *
 *      This entails the following functions:
 *
 *          DirectInputCreate
 *
 ****************************************************************************/

using dic_proc = HRESULT(WINAPI*)(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUT* ppDI, LPUNKNOWN punkOuter);

bool DirectInputInitialize()
{
	if (hDLL)
		return true;

	/* Turn off error dialog for this call */
	UINT error_mode = SetErrorMode(0);
	hDLL = system_services::load_library(L"dinput.dll");
	SetErrorMode(error_mode);

	if (hDLL == nullptr)
		return false;

	dic_proc dic = (dic_proc)system_services::get_proc_address_utf8((HINSTANCE)hDLL, "DirectInputCreateW");

	if (dic == nullptr)
		return false;

	HRESULT hr = dic(system_services::get_module_handle(0), 0x0700, &di, 0); // setup DIRECT INPUT 7 for the GUI

	if (FAILED(hr))
	{
		hr = dic(system_services::get_module_handle(0), 0x0500, &di, 0); // if failed, try with version 5

		if (FAILED(hr))
		{
			ErrorMsg("DirectInputCreate failed! error=%x\n", (unsigned int)hr);
			di = nullptr;
			return false;
		}
	}
	return true;
}

/****************************************************************************
 *
 *      DirectInputClose
 *
 *      Terminate our usage of DirectInput.
 *
 ****************************************************************************/

void DirectInputClose()
{
	/* Release any lingering IDirectInput object. */
	if (di)
	{
		IDirectInput_Release(di);
		di = nullptr;
	}
}

BOOL CALLBACK inputEnumDeviceProc(LPCDIDEVICEINSTANCE pdidi, LPVOID pv)
{
	GUID *pguidDevice;

	/* report back the instance guid of the device we enumerated */
	if (pv)
	{
		pguidDevice  = (GUID *)pv;
		*pguidDevice = pdidi->guidInstance;
	}

	/* BUGBUG for now, stop after the first device has been found */
	return DIENUM_STOP;
}

HRESULT SetDIDwordProperty(LPDIRECTINPUTDEVICE2 pdev, REFGUID guidProperty, DWORD dwObject, DWORD dwHow, DWORD dwValue)
{
	DIPROPDWORD dipdw;

	dipdw.diph.dwSize       = sizeof(dipdw);
	dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
	dipdw.diph.dwObj        = dwObject;
	dipdw.diph.dwHow        = dwHow;
	dipdw.dwData            = dwValue;

	return IDirectInputDevice2_SetProperty(pdev, guidProperty, &dipdw.diph);
}

LPDIRECTINPUT GetDirectInput(void)
{
	return di;
}
