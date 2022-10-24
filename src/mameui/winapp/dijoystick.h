// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_DIJOYSTICK_H
#define MAMEUI_WINAPP_DIJOYSTICK_H

#pragma once

//--------------------------------------
//  limits:
//  - 7 joysticks
//  - 15 sticks on each joystick (15?)
//  - 63 buttons on each joystick
//
//  - 256 total inputs
//
//
//   1 1 1 1 1 1
//   5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-----------+---------+-----+
//  |Dir|Axis/Button|   Stick | Joy |
//  +---+-----------+---------+-----+
//
//    Stick:  0 for buttons 1 for axis
//    Joy:    1 for Mouse/track buttons
//--------------------------------------
template <typename T1,typename T2,typename T3,typename T4>
constexpr auto JOYCODE(T1 joy, T2 stick, T3 axis_or_button, T4 dir) {
	return ((((dir) & 0x03) << 14) | (((axis_or_button) & 0x3f) << 8) | (((stick) & 0x1f) << 3) | (((joy) & 0x07) << 0));
}

template <typename T1>
constexpr auto GET_JOYCODE_JOY(T1 code) {
	return (((code) >> 0) & 0x07);
}

template <typename T1>
constexpr auto GET_JOYCODE_STICK(T1 code) {
	return (((code) >> 3) & 0x1f);
}

template <typename T1>
constexpr auto GET_JOYCODE_AXIS(T1 code) {
	return (((code) >> 8) & 0x3f);
}

template <typename T1>
constexpr auto GET_JOYCODE_BUTTON(T1 code) {
	return GET_JOYCODE_AXIS(code);
}

template <typename T1>
constexpr auto GET_JOYCODE_DIR(T1 code) {
	return (((code) >> 14) & 0x03);
}

constexpr auto JOYCODE_STICK_BTN = 0;
constexpr auto JOYCODE_STICK_AXIS = 1;
constexpr auto JOYCODE_STICK_POV = 2;

constexpr auto JOYCODE_DIR_BTN = 0;
constexpr auto JOYCODE_DIR_NEG = 1;
constexpr auto JOYCODE_DIR_POS = 2;

using DIJoystickCallbacks = struct osd_joystick_callbacks
{
	int  (*init)(void);
	void (*exit)(void);
	int  (*is_joy_pressed)(int joycode);
	void (*poll_joysticks)(void);
	bool (*Available)(void);
};

extern const DIJoystickCallbacks DIJoystick;

int DIJoystick_GetNumPhysicalJoysticks(void);
std::wstring DIJoystick_GetPhysicalJoystickName(int num_joystick);

int DIJoystick_GetNumPhysicalJoystickAxes(int num_joystick);
std::wstring DIJoystick_GetPhysicalJoystickAxisName(int num_joystick, int num_axis);

#endif // MAMEUI_WINAPP_DIJOYSTICK_H
