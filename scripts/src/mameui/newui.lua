-- license:BSD-3-Clause
-- copyright-holders:MAMEdev Team, Robbbert, 0perator

---------------------------------------------------------------------------
--
--   newui.lua
--
--   Rules for building NewUI - an optional windows-style menubar
--
---------------------------------------------------------------------------

project ("osd_" .. _OPTIONS["osd"])
	includedirs {
--      MAME_DIR .. "src/devices",
--      MAME_DIR .. "src/emu",
		MAME_DIR .. "src/frontend/mame",
--      MAME_DIR .. "src/mameui/winapp",
--      MAME_DIR .. "src/osd",
--      MAME_DIR .. "src/osd/windows",
	}

	if (_OPTIONS["USE_NEWUI"] == "1") then
		files {
			MAME_DIR .. "src/mameui/winapp/mui_strconv.cpp",
			MAME_DIR .. "src/mameui/winapp/mui_strconv.h",
			MAME_DIR .. "src/mameui/winapp/newui.cpp",
			MAME_DIR .. "src/mameui/winapp/newui.h",
		}
	else
		files {
			MAME_DIR .. "src/osd/windows/winmenu.cpp",
		}
	end
