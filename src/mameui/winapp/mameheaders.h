// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

//============================================================
//
//  mameheaders.h - MAME headers
//
//============================================================

#ifndef MAMEUI_WINAPP_MAMEHEADERS_H
#define MAMEUI_WINAPP_MAMEHEADERS_H

#pragma once

// emu
// ----

// shlwapi.h pulls in objbase.h which defines interface as a struct which conflict with a template declaration in device.h
#ifdef interface 
#undef interface // undef interface which is for COM interfaces
#endif

#include "emu.h"
#include "emucore.h"
#include "audit.h"
#include "drivenum.h"
#include "main.h"
#include "natkeyboard.h"
#include "romload.h"
#include "screen.h"
#include "softlist_dev.h"
#include "speaker.h"

// devices
// --------

#include "imagedev/cassette.h"
#include "machine/ram.h"
#include "sound/samples.h"

// frontend
// ---------

#include "language.h"
#include "mame.h"
#include "mameopts.h"
#include "ui/info.h"
#include "ui/moptions.h"
#include "ui/ui.h"

// lib
// ----

#include "util/corestr.h"
#include "util/hash.h"
#include "util/path.h"
#include "util/png.h"
#include "util/unzip.h"
#include "util/zippath.h"

// osd
// ----

#include "modules/font/font_module.h"
#include "modules/input/input_module.h"
#include "modules/lib/osdobj_common.h"
#include "modules/monitor/monitor_module.h"
#include "osdcore.h"
#include "strconv.h"
#include "windows/winutf8.h"
#include "windows/window.h"
#include "windows/winmain.h"
#include "windows/winopts.h"

#endif // MAMEUI_WINAPP_MAMEHEADERS_H
