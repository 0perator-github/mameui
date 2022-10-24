// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

/***************************************************************************

  dijoystick.cpp

 ***************************************************************************/

// standard C++ headers
#include <cwchar>
#include <filesystem>
#include <sstream>

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
#include "mui_wcstr.h"

#include "directinput.h"
#include "mui_util.h" // For ErrorMsg
#include "screenshot.h"
#include "winui.h"

#include "dijoystick.h"

using namespace mameui::util::string_util;

/***************************************************************************
    function prototypes
 ***************************************************************************/

static int  DIJoystick_init(void);
static void DIJoystick_exit(void);
static void DIJoystick_poll_joysticks(void);
static int  DIJoystick_is_joy_pressed(int joycode);
static bool DIJoystick_Available(void);

static bool CALLBACK DIJoystick_EnumDeviceProc(LPDIDEVICEINSTANCEW pdidi, LPVOID pv);

/***************************************************************************
    External variables
 ***************************************************************************/

const struct osd_joystick_callbacks  DIJoystick =
{
	DIJoystick_init,                /* init              */
	DIJoystick_exit,                /* exit              */
	DIJoystick_is_joy_pressed,      /* joy_pressed       */
	DIJoystick_poll_joysticks,      /* poll_joysticks    */
	DIJoystick_Available,           /* Available         */
};

/***************************************************************************
    Internal structures
 ***************************************************************************/

constexpr auto MAX_PHYSICAL_JOYSTICKS = 20;
constexpr auto MAX_AXES = 20;

struct joystick_axis
{
	GUID guid;
	std::wstring name;

	int offset; /* offset in dijoystate */
};

struct joystick_entry
{
	bool use_joystick;

	GUID guidDevice;
	std::wstring name;

	bool is_light_gun;

	LPDIRECTINPUTDEVICE2 did;

	DWORD num_axes;
	joystick_axis axes[MAX_AXES];

	DWORD num_pov;
	DWORD num_buttons;

	DIJOYSTATE dijs;

};

struct directinput_joystick
{
	int   use_count; /* the gui and game can both init/exit us, so keep track */
	bool  m_bCoinSlot;

	DWORD num_joysticks;
	joystick_entry joysticks[MAX_PHYSICAL_JOYSTICKS]; /* actual joystick data! */
};

/* internal functions needing our declarations */
static BOOL CALLBACK DIJoystick_EnumAxisObjectsProc(LPCDIDEVICEOBJECTINSTANCEW lpddoi, LPVOID pvRef);
static BOOL CALLBACK DIJoystick_EnumPOVObjectsProc(LPCDIDEVICEOBJECTINSTANCEW lpddoi, LPVOID pvRef);
static BOOL CALLBACK DIJoystick_EnumButtonObjectsProc(LPCDIDEVICEOBJECTINSTANCEW lpddoi, LPVOID pvRef);
static void ClearJoyState(DIJOYSTATE *pdijs);
static void InitJoystick(joystick_entry *joystick);
static void ExitJoystick(joystick_entry *joystick);
const char * DirectXDecodeError(HRESULT errorval);

/***************************************************************************
    Internal variables
 ***************************************************************************/

static struct directinput_joystick   This;

static const GUID guidnullptr = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};

/***************************************************************************
    External OSD functions
 ***************************************************************************/
/*
    put here anything you need to do when the program is started. Return 0 if
    initialization was successful, nonzero otherwise.
*/
static int DIJoystick_init(void)
{
	This.use_count++;
	This.num_joysticks = 0;

	LPDIRECTINPUT di = GetDirectInput();
	if (di == nullptr)
	{
		ErrorMsg("DirectInput not initialized");
		return 1;
	}

	/* enumerate for joystick devices */
	HRESULT hr = IDirectInput_EnumDevices(di, DIDEVTYPE_JOYSTICK, (LPDIENUMDEVICESCALLBACK)DIJoystick_EnumDeviceProc, nullptr, DIEDFL_ATTACHEDONLY );
	if (FAILED(hr))
	{
		ErrorMsg("DirectInput EnumDevices() failed: %s", DirectXDecodeError(hr));
		return 0;
	}

	/* create each joystick device, enumerate each joystick for axes, etc */
	for (DWORD i = 0; i < This.num_joysticks; i++)
		InitJoystick(&This.joysticks[i]);

	/* Are there any joysticks attached? */
	if (This.num_joysticks < 1)
		/*ErrorMsg("DirectInput EnumDevices didn't find any joysticks");*/
		return 1;

	return 0;
}

/*
    put here cleanup routines to be executed when the program is terminated.
*/
static void DIJoystick_exit(void)
{
	This.use_count--;
	if (This.use_count > 0)
		return;

	for (DWORD i = 0; i < This.num_joysticks; i++)
		ExitJoystick(&This.joysticks[i]);

	This.num_joysticks = 0;
}

static void DIJoystick_poll_joysticks(void)
{
	HRESULT hr;
	This.m_bCoinSlot = 0;

	for (DWORD i = 0; i < This.num_joysticks; i++)
	{
		/* start by clearing the structure, then fill it in if possible */

		ClearJoyState(&This.joysticks[i].dijs);

		if (This.joysticks[i].did == nullptr)
			continue;

		if (This.joysticks[i].use_joystick == false)
			continue;

		hr = IDirectInputDevice2_Poll(This.joysticks[i].did);

		hr = IDirectInputDevice2_GetDeviceState(This.joysticks[i].did,sizeof(DIJOYSTATE), &This.joysticks[i].dijs);
		if (FAILED(hr))
		{
			if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
				hr = IDirectInputDevice2_Acquire(This.joysticks[i].did);

			continue;
		}
	}
}

/*
    check if the DIJoystick is moved in the specified direction, defined in
    osdepend.h. Return 0 if it is not pressed, nonzero otherwise.
*/

static int DIJoystick_is_joy_pressed(int joycode)
{
	int dz = 60;

	// Get joystick number (joy_num) from the joycode and validate it
	int joy_num = GET_JOYCODE_JOY(joycode) - 1;
	if (joy_num < 0 || joy_num >= This.num_joysticks)
		return 0;

	joystick_entry& joystick = This.joysticks[joy_num];
	if (!joystick.use_joystick)
		return 0;

	DIJOYSTATE& dijs = joystick.dijs;

	// Get stick type from the joycode
	int joy_stick = GET_JOYCODE_STICK(joycode);

	// Handle Button case
	if (joy_stick == JOYCODE_STICK_BTN)
	{
		int joy_btn = GET_JOYCODE_BUTTON(joycode) - 1;
		if (joy_btn < 0 || joy_btn >= joystick.num_buttons || joy_btn >= 32)
			return 0;

		// Ensure the correct direction is a button
		if (GET_JOYCODE_DIR(joycode) != JOYCODE_DIR_BTN)
			return 0;

		return dijs.rgbButtons[joy_btn] != 0;
	}

	// Handle POV (Point of View) case
	if (joy_stick == JOYCODE_STICK_POV)
	{
		int joy_pov = GET_JOYCODE_BUTTON(joycode) / 4;
		if (joy_pov < 0 || joy_pov >= joystick.num_pov || joy_pov >= 4)
			return 0;

		int pov_value = dijs.rgdwPOV[joy_pov];
		if (LOWORD(pov_value) == 0xffff)
			return 0;

		int angle = (pov_value + 27000) % 36000;
		angle = (36000 - angle) % 36000;
		angle /= 100;

		// Calculate axis values from the angle
		int axis_value = (GET_JOYCODE_BUTTON(joycode) % 2 == 0) ?
			128 + static_cast<int>(127 * cos(2 * M_PI * angle / 360.0)) :  // x-axis
			128 + static_cast<int>(127 * sin(2 * M_PI * angle / 360.0));   // y-axis

		// Check for direction
		int joy_dir = GET_JOYCODE_DIR(joycode);
		return (joy_dir == 1) ? axis_value <= (128 - dz) : axis_value >= (128 + dz);
	}

	// Handle Axis case
	int joy_axis = GET_JOYCODE_AXIS(joycode) - 1;
	if (joy_axis < 0 || joy_axis >= joystick.num_axes || joy_axis >= MAX_AXES)
		return 0;

	// Fetch axis value using offset from joystick state
	int value = *reinterpret_cast<int*>((byte*)&dijs + joystick.axes[joy_axis].offset);

	// Check for direction
	int joy_dir = GET_JOYCODE_DIR(joycode);
	return (joy_dir == JOYCODE_DIR_NEG) ?
		value <= (128 - dz) : value >= (128 + dz);
}

static bool DIJoystick_Available(void)
{
	static bool bBeenHere = false;
	static bool bAvailable = false;
	LPDIRECTINPUT di = GetDirectInput();

	if (di == 0)
		return false;

	if (bBeenHere == false)
		bBeenHere = true;
	else
		return bAvailable;

	/* enumerate for joystick devices */
	GUID guidDevice = guidnullptr;
	HRESULT hr = IDirectInput_EnumDevices(di, DIDEVTYPE_JOYSTICK, inputEnumDeviceProc, &guidDevice, DIEDFL_ATTACHEDONLY );
	if (FAILED(hr))
	{
		return false;
	}

	/* Are there any joysticks attached? */
	if (IsEqualGUID(guidDevice, guidnullptr))
		return false;

	LPDIRECTINPUTDEVICE didTemp;
	hr = IDirectInput_CreateDevice(di, guidDevice, &didTemp, nullptr);
	if (FAILED(hr))
		return false;

	/* Determine if DX5 is available by a QI for a DX5 interface. */
	LPDIRECTINPUTDEVICE didJoystick;
	hr = IDirectInputDevice_QueryInterface(didTemp, IID_IDirectInputDevice2, (void**)&didJoystick);
	if (FAILED(hr))
		bAvailable = false;
	else
	{
		bAvailable = true;
		IDirectInputDevice_Release(didJoystick);
	}

	/* dispose of the temp interface */
	IDirectInputDevice_Release(didTemp);

	return bAvailable;
}

int DIJoystick_GetNumPhysicalJoysticks()
{
	return This.num_joysticks;
}

std::wstring DIJoystick_GetPhysicalJoystickName(int num_joystick)
{
	return This.joysticks[num_joystick].name;
}

int DIJoystick_GetNumPhysicalJoystickAxes(int num_joystick)
{
	return This.joysticks[num_joystick].num_axes;
}

std::wstring DIJoystick_GetPhysicalJoystickAxisName(int num_joystick, int num_axis)
{
	return This.joysticks[num_joystick].axes[num_axis].name;
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

bool CALLBACK DIJoystick_EnumDeviceProc(LPDIDEVICEINSTANCEW pdidi, LPVOID pv)
{
	if (This.num_joysticks >= MAX_PHYSICAL_JOYSTICKS)
		return DIENUM_STOP;

	std::wostringstream joystick_name;
	joystick_name << pdidi->tszProductName << L" (" << pdidi->tszInstanceName << L")";
	This.joysticks[This.num_joysticks].name = joystick_name.str();
	This.num_joysticks++;

	return DIENUM_CONTINUE;
}

static BOOL CALLBACK DIJoystick_EnumAxisObjectsProc(LPCDIDEVICEOBJECTINSTANCEW lpddoi, LPVOID pvRef)
{
	joystick_entry *joystick = (joystick_entry *)pvRef;
	if (joystick->num_axes >= MAX_AXES)
		return DIENUM_STOP;

	joystick->axes[joystick->num_axes].guid = lpddoi->guidType;
	joystick->axes[joystick->num_axes].name = lpddoi->tszName;
	joystick->axes[joystick->num_axes].offset = lpddoi->dwOfs;

	/*ErrorMsg("got axis %s, offset %i",lpddoi->tszName, lpddoi->dwOfs);*/

	DIPROPRANGE diprg;
	diprg.diph.dwSize = sizeof(diprg);
	diprg.diph.dwHeaderSize = sizeof(diprg.diph);
	diprg.diph.dwObj = lpddoi->dwOfs;
	diprg.diph.dwHow = DIPH_BYOFFSET;
	diprg.lMin = 0;
	diprg.lMax = 255;

	HRESULT hr = IDirectInputDevice2_SetProperty(joystick->did, DIPROP_RANGE, &diprg.diph);
	if (FAILED(hr)) /* if this fails, don't use this axis */
	{
		joystick->axes[joystick->num_axes].name.clear();
		return DIENUM_CONTINUE;
	}

#ifdef JOY_DEBUG
	if (FAILED(hr))
		ErrorMsg("DirectInput SetProperty() joystick axis %s failed - %s\n", joystick->axes[joystick->num_axes].name.c_str(), DirectXDecodeError(hr));
#endif

	/* Set axis dead zone to 0; we need accurate #'s for analog joystick reading. */

	hr = SetDIDwordProperty(joystick->did, DIPROP_DEADZONE, lpddoi->dwOfs, DIPH_BYOFFSET, 0);

#ifdef JOY_DEBUG
	if (FAILED(hr))
		ErrorMsg("DirectInput SetProperty() joystick axis %s dead zone failed - %s\n", joystick->axes[joystick->num_axes].name.c_str(), DirectXDecodeError(hr));
#endif

	joystick->num_axes++;

	return DIENUM_CONTINUE;
}

static BOOL CALLBACK DIJoystick_EnumPOVObjectsProc(LPCDIDEVICEOBJECTINSTANCEW lpddoi, LPVOID pvRef)
{
	joystick_entry *joystick = (joystick_entry *)pvRef;
	joystick->num_pov++;

	return DIENUM_CONTINUE;
}

static BOOL CALLBACK DIJoystick_EnumButtonObjectsProc(LPCDIDEVICEOBJECTINSTANCEW lpddoi, LPVOID pvRef)
{
	joystick_entry *joystick = (joystick_entry *)pvRef;
	joystick->num_buttons++;

	return DIENUM_CONTINUE;
}

static void ClearJoyState(DIJOYSTATE *pdijs)
{
	*pdijs = {};
	pdijs->lX = 128;
	pdijs->lY = 128;
	pdijs->lZ = 128;
	pdijs->lRx = 128;
	pdijs->lRy = 128;
	pdijs->lRz = 128;
	pdijs->rglSlider[0] = 128;
	pdijs->rglSlider[1] = 128;
	pdijs->rgdwPOV[0] = -1;
	pdijs->rgdwPOV[1] = -1;
	pdijs->rgdwPOV[2] = -1;
	pdijs->rgdwPOV[3] = -1;
}

static void InitJoystick(joystick_entry *joystick)
{
	joystick->use_joystick = false;
	joystick->did = nullptr;
	joystick->num_axes = 0;
	joystick->is_light_gun = (!mui_wcscmp(joystick->name, L"ACT LABS GS (ACT LABS GS)"));

	/* get a did1 interface first... */
	LPDIRECTINPUT di = GetDirectInput();
	LPDIRECTINPUTDEVICE didTemp;
	HRESULT hr = IDirectInput_CreateDevice(di, joystick->guidDevice, &didTemp, nullptr);
	if (FAILED(hr))
	{
		ErrorMsg("DirectInput CreateDevice() joystick failed: %s\n", DirectXDecodeError(hr));
		return;
	}

	/* get a did2 interface to work with polling (most) joysticks */
	hr = IDirectInputDevice_QueryInterface(didTemp, IID_IDirectInputDevice2, (void**)&joystick->did);

	/* dispose of the temp interface */
	IDirectInputDevice_Release(didTemp);

	/* check result of getting the did2 */
	if (FAILED(hr))
	{
		/* no error message because this happens in dx3 */
		/* ErrorMsg("DirectInput QueryInterface joystick failed\n"); */
		joystick->did = nullptr;
		return;
	}


	hr = IDirectInputDevice2_SetCooperativeLevel(joystick->did, GetMainWindow(), DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
	if (FAILED(hr))
	{
		ErrorMsg("DirectInput SetCooperativeLevel() joystick failed: %s\n", DirectXDecodeError(hr));
		return;
	}


	hr = IDirectInputDevice2_SetDataFormat(joystick->did, &c_dfDIJoystick);
	if (FAILED(hr))
	{
		ErrorMsg("DirectInput SetDataFormat() joystick failed: %s\n", DirectXDecodeError(hr));
		return;
	}

	if (joystick->is_light_gun)
	{
		/* setup light gun to report raw screen pixel data */

		DIPROPHEADER diphead{ sizeof(DIPROPDWORD) };
		diphead.dwHeaderSize = sizeof(DIPROPHEADER);
		diphead.dwObj = 0;
		diphead.dwHow = DIPH_DEVICE;

		DIPROPDWORD diprop{ diphead };
		diprop.dwData = DIPROPCALIBRATIONMODE_RAW;

		IDirectInputDevice2_SetProperty(joystick->did, DIPROP_CALIBRATIONMODE, &diprop.diph);
	}
	else
	{
		/* enumerate our axes */
		hr = IDirectInputDevice_EnumObjects(joystick->did, DIJoystick_EnumAxisObjectsProc, joystick, DIDFT_AXIS);
		if (FAILED(hr))
		{
			ErrorMsg("DirectInput EnumObjects() Axes failed: %s\n", DirectXDecodeError(hr));
			return;
		}

		/* enumerate our POV hats */
		joystick->num_pov = 0;
		hr = IDirectInputDevice_EnumObjects(joystick->did, DIJoystick_EnumPOVObjectsProc, joystick, DIDFT_POV);
		if (FAILED(hr))
		{
			ErrorMsg("DirectInput EnumObjects() POVs failed: %s\n", DirectXDecodeError(hr));
			return;
		}
	}

	/* enumerate our buttons */

	joystick->num_buttons = 0;
	hr = IDirectInputDevice_EnumObjects(joystick->did, DIJoystick_EnumButtonObjectsProc, joystick, DIDFT_BUTTON);
	if (FAILED(hr))
	{
		ErrorMsg("DirectInput EnumObjects() Buttons failed: %s\n", DirectXDecodeError(hr));
		return;
	}

	hr = IDirectInputDevice2_Acquire(joystick->did);
	if (FAILED(hr))
	{
		ErrorMsg("DirectInputDevice Acquire joystick failed!\n");
		return;
	}

	/* start by clearing the structures */

	ClearJoyState(&joystick->dijs);

	joystick->use_joystick = true;
}

static void ExitJoystick(joystick_entry *joystick)
{
	if (joystick->did)
	{
		IDirectInputDevice_Unacquire(joystick->did);
		IDirectInputDevice_Release(joystick->did);
	}

	for (DWORD i = 0; i < joystick->num_axes; i++)
		joystick->axes[i].name.clear();

	joystick->name.clear();
}

/***************************************************************************
    DXdecode stuff
 ***************************************************************************/

struct directx_return_code
{
	HRESULT     hr;
	const char* szError;
};
using ERRORCODE = struct directx_return_code;
using LPERRORCODE = ERRORCODE*;

#include <ddraw.h>
static const ERRORCODE g_ErrorCode[] =
{
	{   DDERR_ALREADYINITIALIZED,           "DDERR_ALREADYINITIALIZED"},
	{   DDERR_CANNOTATTACHSURFACE,          "DDERR_CANNOTATTACHSURFACE"},
	{   DDERR_CANNOTDETACHSURFACE,          "DDERR_CANNOTDETACHSURFACE"},
	{   DDERR_CURRENTLYNOTAVAIL,            "DDERR_CURRENTLYNOTAVAIL"},
	{   DDERR_EXCEPTION,                    "DDERR_EXCEPTION"},
	{   DDERR_GENERIC,                      "DDERR_GENERIC"},
	{   DDERR_HEIGHTALIGN,                  "DDERR_HEIGHTALIGN"},
	{   DDERR_INCOMPATIBLEPRIMARY,          "DDERR_INCOMPATIBLEPRIMARY"},
	{   DDERR_INVALIDCAPS,                  "DDERR_INVALIDCAPS"},
	{   DDERR_INVALIDCLIPLIST,              "DDERR_INVALIDCLIPLIST"},
	{   DDERR_INVALIDMODE,                  "DDERR_INVALIDMODE"},
	{   DDERR_INVALIDOBJECT,                "DDERR_INVALIDOBJECT"},
	{   DDERR_INVALIDPARAMS,                "DDERR_INVALIDPARAMS"},
	{   DDERR_INVALIDPIXELFORMAT,           "DDERR_INVALIDPIXELFORMAT"},
	{   DDERR_INVALIDRECT,                  "DDERR_INVALIDRECT"},
	{   DDERR_LOCKEDSURFACES,               "DDERR_LOCKEDSURFACES"},
	{   DDERR_NO3D,                         "DDERR_NO3D"},
	{   DDERR_NOALPHAHW,                    "DDERR_NOALPHAHW"},
	{   DDERR_NOCLIPLIST,                   "DDERR_NOCLIPLIST"},
	{   DDERR_NOCOLORCONVHW,                "DDERR_NOCOLORCONVHW"},
	{   DDERR_NOCOOPERATIVELEVELSET,        "DDERR_NOCOOPERATIVELEVELSET"},
	{   DDERR_NOCOLORKEY,                   "DDERR_NOCOLORKEY"},
	{   DDERR_NOCOLORKEYHW,                 "DDERR_NOCOLORKEYHW"},
	{   DDERR_NODIRECTDRAWSUPPORT,          "DDERR_NODIRECTDRAWSUPPORT"},
	{   DDERR_NOEXCLUSIVEMODE,              "DDERR_NOEXCLUSIVEMODE"},
	{   DDERR_NOFLIPHW,                     "DDERR_NOFLIPHW"},
	{   DDERR_NOGDI,                        "DDERR_NOGDI"},
	{   DDERR_NOMIRRORHW,                   "DDERR_NOMIRRORHW"},
	{   DDERR_NOTFOUND,                     "DDERR_NOTFOUND"},
	{   DDERR_NOOVERLAYHW,                  "DDERR_NOOVERLAYHW"},
	{   DDERR_NORASTEROPHW,                 "DDERR_NORASTEROPHW"},
	{   DDERR_NOROTATIONHW,                 "DDERR_NOROTATIONHW"},
	{   DDERR_NOSTRETCHHW,                  "DDERR_NOSTRETCHHW"},
	{   DDERR_NOT4BITCOLOR,                 "DDERR_NOT4BITCOLOR"},
	{   DDERR_NOT4BITCOLORINDEX,            "DDERR_NOT4BITCOLORINDEX"},
	{   DDERR_NOT8BITCOLOR,                 "DDERR_NOT8BITCOLOR"},
	{   DDERR_NOTEXTUREHW,                  "DDERR_NOTEXTUREHW"},
	{   DDERR_NOVSYNCHW,                    "DDERR_NOVSYNCHW"},
	{   DDERR_NOZBUFFERHW,                  "DDERR_NOZBUFFERHW"},
	{   DDERR_NOZOVERLAYHW,                 "DDERR_NOZOVERLAYHW"},
	{   DDERR_OUTOFCAPS,                    "DDERR_OUTOFCAPS"},
	{   DDERR_OUTOFMEMORY,                  "DDERR_OUTOFMEMORY"},
	{   DDERR_OUTOFVIDEOMEMORY,             "DDERR_OUTOFVIDEOMEMORY"},
	{   DDERR_OVERLAYCANTCLIP,              "DDERR_OVERLAYCANTCLIP"},
	{   DDERR_OVERLAYCOLORKEYONLYONEACTIVE, "DDERR_OVERLAYCOLORKEYONLYONEACTIVE"},
	{   DDERR_PALETTEBUSY,                  "DDERR_PALETTEBUSY"},
	{   DDERR_COLORKEYNOTSET,               "DDERR_COLORKEYNOTSET"},
	{   DDERR_SURFACEALREADYATTACHED,       "DDERR_SURFACEALREADYATTACHED"},
	{   DDERR_SURFACEALREADYDEPENDENT,      "DDERR_SURFACEALREADYDEPENDENT"},
	{   DDERR_SURFACEBUSY,                  "DDERR_SURFACEBUSY"},
	{   DDERR_CANTLOCKSURFACE,              "DDERR_CANTLOCKSURFACE"},
	{   DDERR_SURFACEISOBSCURED,            "DDERR_SURFACEISOBSCURED"},
	{   DDERR_SURFACELOST,                  "DDERR_SURFACELOST"},
	{   DDERR_SURFACENOTATTACHED,           "DDERR_SURFACENOTATTACHED"},
	{   DDERR_TOOBIGHEIGHT,                 "DDERR_TOOBIGHEIGHT"},
	{   DDERR_TOOBIGSIZE,                   "DDERR_TOOBIGSIZE"},
	{   DDERR_TOOBIGWIDTH,                  "DDERR_TOOBIGWIDTH"},
	{   DDERR_UNSUPPORTED,                  "DDERR_UNSUPPORTED"},
	{   DDERR_UNSUPPORTEDFORMAT,            "DDERR_UNSUPPORTEDFORMAT"},
	{   DDERR_UNSUPPORTEDMASK,              "DDERR_UNSUPPORTEDMASK"},
	{   DDERR_VERTICALBLANKINPROGRESS,      "DDERR_VERTICALBLANKINPROGRESS"},
	{   DDERR_WASSTILLDRAWING,              "DDERR_WASSTILLDRAWING"},
	{   DDERR_XALIGN,                       "DDERR_XALIGN"},
	{   DDERR_INVALIDDIRECTDRAWGUID,        "DDERR_INVALIDDIRECTDRAWGUID"},
	{   DDERR_DIRECTDRAWALREADYCREATED,     "DDERR_DIRECTDRAWALREADYCREATED"},
	{   DDERR_NODIRECTDRAWHW,               "DDERR_NODIRECTDRAWHW"},
	{   DDERR_PRIMARYSURFACEALREADYEXISTS,  "DDERR_PRIMARYSURFACEALREADYEXISTS"},
	{   DDERR_NOEMULATION,                  "DDERR_NOEMULATION"},
	{   DDERR_REGIONTOOSMALL,               "DDERR_REGIONTOOSMALL"},
	{   DDERR_CLIPPERISUSINGHWND,           "DDERR_CLIPPERISUSINGHWND"},
	{   DDERR_NOCLIPPERATTACHED,            "DDERR_NOCLIPPERATTACHED"},
	{   DDERR_NOHWND,                       "DDERR_NOHWND"},
	{   DDERR_HWNDSUBCLASSED,               "DDERR_HWNDSUBCLASSED"},
	{   DDERR_HWNDALREADYSET,               "DDERR_HWNDALREADYSET"},
	{   DDERR_NOPALETTEATTACHED,            "DDERR_NOPALETTEATTACHED"},
	{   DDERR_NOPALETTEHW,                  "DDERR_NOPALETTEHW"},
	{   DDERR_BLTFASTCANTCLIP,              "DDERR_BLTFASTCANTCLIP"},
	{   DDERR_NOBLTHW,                      "DDERR_NOBLTHW"},
	{   DDERR_NODDROPSHW,                   "DDERR_NODDROPSHW"},
	{   DDERR_OVERLAYNOTVISIBLE,            "DDERR_OVERLAYNOTVISIBLE"},
	{   DDERR_NOOVERLAYDEST,                "DDERR_NOOVERLAYDEST"},
	{   DDERR_INVALIDPOSITION,              "DDERR_INVALIDPOSITION"},
	{   DDERR_NOTAOVERLAYSURFACE,           "DDERR_NOTAOVERLAYSURFACE"},
	{   DDERR_EXCLUSIVEMODEALREADYSET,      "DDERR_EXCLUSIVEMODEALREADYSET"},
	{   DDERR_NOTFLIPPABLE,                 "DDERR_NOTFLIPPABLE"},
	{   DDERR_CANTDUPLICATE,                "DDERR_CANTDUPLICATE"},
	{   DDERR_NOTLOCKED,                    "DDERR_NOTLOCKED"},
	{   DDERR_CANTCREATEDC,                 "DDERR_CANTCREATEDC"},
	{   DDERR_NODC,                         "DDERR_NODC"},
	{   DDERR_WRONGMODE,                    "DDERR_WRONGMODE"},
	{   DDERR_IMPLICITLYCREATED,            "DDERR_IMPLICITLYCREATED"},
	{   DDERR_NOTPALETTIZED,                "DDERR_NOTPALETTIZED"},
	{   DDERR_UNSUPPORTEDMODE,              "DDERR_UNSUPPORTEDMODE"},
	{   DDERR_NOMIPMAPHW,                   "DDERR_NOMIPMAPHW"},
	{   DDERR_INVALIDSURFACETYPE,           "DDERR_INVALIDSURFACETYPE"},

	{   DDERR_NOOPTIMIZEHW,                 "DDERR_NOOPTIMIZEHW"},
	{   DDERR_NOTLOADED,                    "DDERR_NOTLOADED"},

	{   DDERR_DCALREADYCREATED,             "DDERR_DCALREADYCREATED"},

	{   DDERR_NONONLOCALVIDMEM,             "DDERR_NONONLOCALVIDMEM"},
	{   DDERR_CANTPAGELOCK,                 "DDERR_CANTPAGELOCK"},
	{   DDERR_CANTPAGEUNLOCK,               "DDERR_CANTPAGEUNLOCK"},
	{   DDERR_NOTPAGELOCKED,                "DDERR_NOTPAGELOCKED"},

	{   DDERR_MOREDATA,                     "DDERR_MOREDATA"},
	{   DDERR_VIDEONOTACTIVE,               "DDERR_VIDEONOTACTIVE"},
	{   DDERR_DEVICEDOESNTOWNSURFACE,       "DDERR_DEVICEDOESNTOWNSURFACE"},
	{   DDERR_NOTINITIALIZED,               "DDERR_NOTINITIALIZED"},

	{   DIERR_OLDDIRECTINPUTVERSION,        "DIERR_OLDDIRECTINPUTVERSION" },
	{   DIERR_BETADIRECTINPUTVERSION,       "DIERR_BETADIRECTINPUTVERSION" },
	{   DIERR_BADDRIVERVER,                 "DIERR_BADDRIVERVER" },
	{   DIERR_DEVICENOTREG,                 "DIERR_DEVICENOTREG" },
	{   DIERR_NOTFOUND,                     "DIERR_NOTFOUND" },
	{   DIERR_OBJECTNOTFOUND,               "DIERR_OBJECTNOTFOUND" },
	{   DIERR_INVALIDPARAM,                 "DIERR_INVALIDPARAM" },
	{   DIERR_NOINTERFACE,                  "DIERR_NOINTERFACE" },
	{   DIERR_GENERIC,                      "DIERR_GENERIC" },
	{   DIERR_OUTOFMEMORY,                  "DIERR_OUTOFMEMORY" },
	{   DIERR_UNSUPPORTED,                  "DIERR_UNSUPPORTED" },
	{   DIERR_NOTINITIALIZED,               "DIERR_NOTINITIALIZED" },
	{   DIERR_ALREADYINITIALIZED,           "DIERR_ALREADYINITIALIZED" },
	{   DIERR_NOAGGREGATION,                "DIERR_NOAGGREGATION" },
	{   DIERR_OTHERAPPHASPRIO,              "DIERR_OTHERAPPHASPRIO" },
	{   DIERR_INPUTLOST,                    "DIERR_INPUTLOST" },
	{   DIERR_ACQUIRED,                     "DIERR_ACQUIRED" },
	{   DIERR_NOTACQUIRED,                  "DIERR_NOTACQUIRED" },
	{   DIERR_READONLY,                     "DIERR_READONLY" },
	{   DIERR_HANDLEEXISTS,                 "DIERR_HANDLEEXISTS" },
	{   E_PENDING,                          "E_PENDING" },
	{   (HRESULT)DIERR_INSUFFICIENTPRIVS,   "DIERR_INSUFFICIENTPRIVS" },
	{   (HRESULT)DIERR_DEVICEFULL,          "DIERR_DEVICEFULL" },
	{   (HRESULT)DIERR_MOREDATA,            "DIERR_MOREDATA" },
	{   (HRESULT)DIERR_NOTDOWNLOADED,       "DIERR_NOTDOWNLOADED" },
	{   (HRESULT)DIERR_HASEFFECTS,          "DIERR_HASEFFECTS" },
	{   (HRESULT)DIERR_NOTEXCLUSIVEACQUIRED,"DIERR_NOTEXCLUSIVEACQUIRED" },
	{   (HRESULT)DIERR_INCOMPLETEEFFECT,    "DIERR_INCOMPLETEEFFECT" },
	{   (HRESULT)DIERR_NOTBUFFERED,         "DIERR_NOTBUFFERED" },
	{   (HRESULT)DIERR_EFFECTPLAYING,       "DIERR_EFFECTPLAYING" },
	//{   (HRESULT)DIERR_UNPLUGGED,           "DIERR_UNPLUGGED" },

	{   E_NOINTERFACE,                      "E_NOINTERFACE" }

};


/****************************************************************************
   DirectXDecodeError:  Return a string description of the given DirectX
   error code.
*****************************************************************************/
const char * DirectXDecodeError(HRESULT errorval)
{
	static char tmp[64];

	for (size_t i = 0; i < std::size(g_ErrorCode); i++)
	{
		if (g_ErrorCode[i].hr == errorval)
		{
			return g_ErrorCode[i].szError;
		}
	}

	std::ostringstream error_message("UNKNOWN: 0x", std::ostringstream::ate);
	error_message << std::hex << uint32_t(errorval) << std::flush;
	error_message.str().copy(tmp, 63);

	return tmp;
}

