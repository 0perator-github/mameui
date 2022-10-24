-- license:BSD-3-Clause
-- copyright-holders:MAMEdev Team, Robbbert, 0perator

---------------------------------------------------------------------------
--
--   winapp.lua
--
--   Rules for building MAMEUI - An integrated Windows UI for MAME
--
---------------------------------------------------------------------------

project ("osd_" .. _OPTIONS["osd"])
	dofile("winapp_cfg.lua")

project (_OPTIONS["target"] .. "_winapp")
	uuid (os.uuid(_OPTIONS["target"] .. "_winapp"))
	kind (LIBTYPE)

	removeflags {
		"SingleOutputDir",
	}

	dofile("../osd/windows_cfg.lua")
	dofile("winapp_cfg.lua")

	includedirs {
		MAME_DIR .. "src/emu",
		MAME_DIR .. "src/devices",
		MAME_DIR .. "src/frontend/mame",
		MAME_DIR .. "src/lib",
		MAME_DIR .. "src/lib/util",
		MAME_DIR .. "src/osd",
		MAME_DIR .. "src/osd/windows",
		MAME_DIR .. "3rdparty",
		MAME_DIR .. "src/mameui/winapp",
	}

	files {				
		MAME_DIR .. "src/mameui/winapp/winapi_controls.cpp",
		MAME_DIR .. "src/mameui/winapp/winapi_controls.h",
		MAME_DIR .. "src/mameui/winapp/winapi_dialog_boxes.cpp",
		MAME_DIR .. "src/mameui/winapp/winapi_dialog_boxes.h",
		MAME_DIR .. "src/mameui/winapp/winapi_gdi.cpp",
		MAME_DIR .. "src/mameui/winapp/winapi_gdi.h",
		MAME_DIR .. "src/mameui/winapp/winapi_input.cpp",
		MAME_DIR .. "src/mameui/winapp/winapi_input.h",
		MAME_DIR .. "src/mameui/winapp/winapi_menus.cpp",
		MAME_DIR .. "src/mameui/winapp/winapi_menus.h",
		MAME_DIR .. "src/mameui/winapp/winapi_shell.cpp",
		MAME_DIR .. "src/mameui/winapp/winapi_shell.h",
		MAME_DIR .. "src/mameui/winapp/winapi_storage.cpp",
		MAME_DIR .. "src/mameui/winapp/winapi_storage.h",
		MAME_DIR .. "src/mameui/winapp/winapi_system_services.cpp",
		MAME_DIR .. "src/mameui/winapp/winapi_system_services.h",
		MAME_DIR .. "src/mameui/winapp/winapi_windows.cpp",
		MAME_DIR .. "src/mameui/winapp/winapi_windows.h",
		
		MAME_DIR .. "src/mameui/winapp/mui_str.cpp",
		MAME_DIR .. "src/mameui/winapp/mui_str.h",
--		MAME_DIR .. "src/mameui/winapp/mui_tcs.cpp",	-- TCHAR is very unpopular and makes it hard to debug, but it's 
--		MAME_DIR .. "src/mameui/winapp/mui_tcs.h",		-- not useless. So I figured i'd keep an implamentation for now...
--		MAME_DIR .. "src/mameui/winapp/mui_tcsconv.cpp",
--		MAME_DIR .. "src/mameui/winapp/mui_tcsconv.h",
		MAME_DIR .. "src/mameui/winapp/mui_wcs.cpp",
		MAME_DIR .. "src/mameui/winapp/mui_wcs.h",
		MAME_DIR .. "src/mameui/winapp/mui_wcsconv.cpp",
		MAME_DIR .. "src/mameui/winapp/mui_wcsconv.h",

		MAME_DIR .. "src/mameui/winapp/bitmask.cpp",
		MAME_DIR .. "src/mameui/winapp/bitmask.h",
		MAME_DIR .. "src/mameui/winapp/columnedit.cpp",
		MAME_DIR .. "src/mameui/winapp/columnedit.h",
		MAME_DIR .. "src/mameui/winapp/datamap.cpp",
		MAME_DIR .. "src/mameui/winapp/datamap.h",
		MAME_DIR .. "src/mameui/winapp/dialogs.cpp",
		MAME_DIR .. "src/mameui/winapp/dialogs.h",
		MAME_DIR .. "src/mameui/winapp/dijoystick.cpp",
		MAME_DIR .. "src/mameui/winapp/dijoystick.h",
		MAME_DIR .. "src/mameui/winapp/directinput.cpp",
		MAME_DIR .. "src/mameui/winapp/directinput.h",
		MAME_DIR .. "src/mameui/winapp/directories.cpp",
		MAME_DIR .. "src/mameui/winapp/directories.h",
		MAME_DIR .. "src/mameui/winapp/dirwatch.cpp",
		MAME_DIR .. "src/mameui/winapp/dirwatch.h",
		MAME_DIR .. "src/mameui/winapp/emu_opts.cpp",
		MAME_DIR .. "src/mameui/winapp/emu_opts.h",
		MAME_DIR .. "src/mameui/winapp/help.cpp",
		MAME_DIR .. "src/mameui/winapp/help.h",
		MAME_DIR .. "src/mameui/winapp/helpids.cpp",
		MAME_DIR .. "src/mameui/winapp/history.cpp",
		MAME_DIR .. "src/mameui/winapp/history.h",
		MAME_DIR .. "src/mameui/winapp/layout.cpp",
		MAME_DIR .. "src/mameui/winapp/mameheaders.h",
		MAME_DIR .. "src/mameui/winapp/mui_audit.cpp",
		MAME_DIR .. "src/mameui/winapp/mui_audit.h",
		MAME_DIR .. "src/mameui/winapp/mui_main.cpp",
		MAME_DIR .. "src/mameui/winapp/mui_opts.cpp",
		MAME_DIR .. "src/mameui/winapp/mui_opts.h",
		MAME_DIR .. "src/mameui/winapp/mui_util.cpp",
		MAME_DIR .. "src/mameui/winapp/mui_util.h",
		MAME_DIR .. "src/mameui/winapp/picker.cpp",
		MAME_DIR .. "src/mameui/winapp/picker.h",
		MAME_DIR .. "src/mameui/winapp/properties.cpp",
		MAME_DIR .. "src/mameui/winapp/properties.h",
		MAME_DIR .. "src/mameui/winapp/resource.h",
		MAME_DIR .. "src/mameui/winapp/screenshot.cpp",
		MAME_DIR .. "src/mameui/winapp/screenshot.h",
		MAME_DIR .. "src/mameui/winapp/splitters.cpp",
		MAME_DIR .. "src/mameui/winapp/splitters.h",
		MAME_DIR .. "src/mameui/winapp/tabview.cpp",
		MAME_DIR .. "src/mameui/winapp/tabview.h",
		MAME_DIR .. "src/mameui/winapp/treeview.cpp",
		MAME_DIR .. "src/mameui/winapp/treeview.h",
		MAME_DIR .. "src/mameui/winapp/ui_opts.cpp",
		MAME_DIR .. "src/mameui/winapp/ui_opts.h",
		MAME_DIR .. "src/mameui/winapp/winui.cpp",
		MAME_DIR .. "src/mameui/winapp/winui.h",
		MAME_DIR .. "src/mameui/winapp/winui_shared.cpp",
		MAME_DIR .. "src/mameui/winapp/winui_shared.h",
	}

--  if (_OPTIONS["subtarget"] == "mess") then
		files {
			MAME_DIR .. "src/mameui/winapp/messui.cpp",
			MAME_DIR .. "src/mameui/winapp/messui.h",
			MAME_DIR .. "src/mameui/winapp/softwarelist.cpp",
			MAME_DIR .. "src/mameui/winapp/softwarelist.h",
			MAME_DIR .. "src/mameui/winapp/softwarepicker.cpp",
			MAME_DIR .. "src/mameui/winapp/softwarepicker.h",
			MAME_DIR .. "src/mameui/winapp/swconfig.cpp",
			MAME_DIR .. "src/mameui/winapp/swconfig.h",
		}
--  end

	  pchsource(MAME_DIR .. "src/mameui/winapp/game_opts.h")
	  pchsource(MAME_DIR .. "src/mameui/winapp/swconfig.cpp")
	  pchsource(MAME_DIR .. "src/mameui/winapp/winui.cpp")

	  pchsource(MAME_DIR .. "src/mameui/winapp/dialogs.cpp")
	  pchsource(MAME_DIR .. "src/mameui/winapp/dijoystick.cpp")
	  pchsource(MAME_DIR .. "src/mameui/winapp/emu_opts.cpp")
	  pchsource(MAME_DIR .. "src/mameui/winapp/history.cpp")
	  pchsource(MAME_DIR .. "src/mameui/winapp/layout.cpp")
	  pchsource(MAME_DIR .. "src/mameui/winapp/messui.cpp")
	  pchsource(MAME_DIR .. "src/mameui/winapp/mui_audit.cpp")
	  pchsource(MAME_DIR .. "src/mameui/winapp/mui_util.cpp")
	  pchsource(MAME_DIR .. "src/mameui/winapp/newui.cpp")
	  pchsource(MAME_DIR .. "src/mameui/winapp/properties")
	  pchsource(MAME_DIR .. "src/mameui/winapp/softwarepicker.cpp")
	  pchsource(MAME_DIR .. "src/mameui/winapp/treeview.cpp")
