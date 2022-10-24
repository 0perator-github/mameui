// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_DIRECTINPUT_H
#define MAMEUI_WINAPP_DIRECTINPUT_H

#pragma once

//#define DIRECTINPUT_VERSION 0x0700

extern bool DirectInputInitialize(void);
extern void DirectInputClose(void);

extern BOOL CALLBACK inputEnumDeviceProc(LPCDIDEVICEINSTANCE pdidi, LPVOID pv);

extern HRESULT SetDIDwordProperty(LPDIRECTINPUTDEVICE2 pdev, REFGUID guidProperty, DWORD dwObject, DWORD dwHow, DWORD dwValue);

LPDIRECTINPUT GetDirectInput(void);

#endif // MAMEUI_WINAPP_DIRECTINPUT_H

