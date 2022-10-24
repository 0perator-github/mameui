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
		MAME_DIR .. "src/devices",
		MAME_DIR .. "src/emu",
		MAME_DIR .. "src/frontend/mame",
		MAME_DIR .. "src/lib",
		MAME_DIR .. "src/lib/util",
		MAME_DIR .. "src/osd",
		MAME_DIR .. "src/osd/windows",
		MAME_DIR .. "3rdparty",

		MAME_DIR .. "src/mameui/lib/util",
		MAME_DIR .. "src/mameui/lib/winapi",
		MAME_DIR .. "src/mameui/winapp",

		MAME_DIR .. "src/tools",
	}

	files {
		MAME_DIR .. "src/mameui/lib/util/mui_cstr.cpp",
		MAME_DIR .. "src/mameui/lib/util/mui_cstr.h",
		MAME_DIR .. "src/mameui/lib/util/mui_padstr.h",
		MAME_DIR .. "src/mameui/lib/util/mui_stringtokenizer.cpp",
		MAME_DIR .. "src/mameui/lib/util/mui_stringtokenizer.h",
		MAME_DIR .. "src/mameui/lib/util/mui_trimstr.h",
--      MAME_DIR .. "src/mameui/lib/util/mui_tcstr.cpp",    -- TCHAR is very unpopular and makes it hard to debug, but it's
--      MAME_DIR .. "src/mameui/lib/util/mui_tcstr.h",      -- not useless. So I figured i'd keep an implamentation for now...
--      MAME_DIR .. "src/mameui/lib/util/mui_tcstrconv.cpp",
--      MAME_DIR .. "src/mameui/lib/util/mui_tcstrconv.h",
		MAME_DIR .. "src/mameui/lib/util/mui_wcstr.cpp",
		MAME_DIR .. "src/mameui/lib/util/mui_wcstr.h",
		MAME_DIR .. "src/mameui/lib/util/mui_wcstrconv.cpp",
		MAME_DIR .. "src/mameui/lib/util/mui_wcstrconv.h",

		MAME_DIR .. "src/mameui/lib/winapi/data_access_storage.cpp",
		MAME_DIR .. "src/mameui/lib/winapi/data_access_storage.h",
		MAME_DIR .. "src/mameui/lib/winapi/dialog_boxes.cpp",
		MAME_DIR .. "src/mameui/lib/winapi/dialog_boxes.h",
		MAME_DIR .. "src/mameui/lib/winapi/menus_other_res.cpp",
		MAME_DIR .. "src/mameui/lib/winapi/menus_other_res.h",
		MAME_DIR .. "src/mameui/lib/winapi/processes_threads.cpp",
		MAME_DIR .. "src/mameui/lib/winapi/processes_threads.h",
		MAME_DIR .. "src/mameui/lib/winapi/system_services.cpp",
		MAME_DIR .. "src/mameui/lib/winapi/system_services.h",
		MAME_DIR .. "src/mameui/lib/winapi/winapi_common.h",
		MAME_DIR .. "src/mameui/lib/winapi/windows_controls.cpp",
		MAME_DIR .. "src/mameui/lib/winapi/windows_controls.h",
		MAME_DIR .. "src/mameui/lib/winapi/windows_gdi.cpp",
		MAME_DIR .. "src/mameui/lib/winapi/windows_gdi.h",
		MAME_DIR .. "src/mameui/lib/winapi/windows_input.cpp",
		MAME_DIR .. "src/mameui/lib/winapi/windows_input.h",
		MAME_DIR .. "src/mameui/lib/winapi/windows_messages.cpp",
		MAME_DIR .. "src/mameui/lib/winapi/windows_messages.h",
		MAME_DIR .. "src/mameui/lib/winapi/windows_registry.cpp",
		MAME_DIR .. "src/mameui/lib/winapi/windows_registry.h",
		MAME_DIR .. "src/mameui/lib/winapi/windows_shell.cpp",
		MAME_DIR .. "src/mameui/lib/winapi/windows_shell.h",

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
		MAME_DIR .. "src/mameui/winapp/gamepicker.cpp",
		MAME_DIR .. "src/mameui/winapp/gamepicker.h",
		MAME_DIR .. "src/mameui/winapp/help.cpp",
		MAME_DIR .. "src/mameui/winapp/help.h",
		MAME_DIR .. "src/mameui/winapp/helpids.cpp",
		MAME_DIR .. "src/mameui/winapp/history.cpp",
		MAME_DIR .. "src/mameui/winapp/history.h",
		MAME_DIR .. "src/mameui/winapp/layout.cpp",
		MAME_DIR .. "src/mameui/winapp/mui_audit.cpp",
		MAME_DIR .. "src/mameui/winapp/mui_audit.h",
		MAME_DIR .. "src/mameui/winapp/mui_main.cpp",
		MAME_DIR .. "src/mameui/winapp/mui_opts.cpp",
		MAME_DIR .. "src/mameui/winapp/mui_opts.h",
		MAME_DIR .. "src/mameui/winapp/mui_util.cpp",
		MAME_DIR .. "src/mameui/winapp/mui_util.h",
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

		MAME_DIR .. "src/tools/image_handler.cpp",
		MAME_DIR .. "src/tools/image_handler.h",
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

pchheader(MAME_DIR .. "src/mameui/lib/winapi/winapi_common.h")
