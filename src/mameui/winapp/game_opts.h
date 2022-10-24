// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

#ifndef MAMEUI_WINAPP_GAMEOPTS_H
#define MAMEUI_WINAPP_GAMEOPTS_H

#pragma once

using namespace mameui::util;

class winui_game_options
{
	uint32_t m_total;
	uint32_t m_cache;
	uint32_t m_version;
	bool m_rebuild;

	struct driver_options
	{
		uint32_t game_number;
		int rom;
		int sample;
		uint32_t cache_lower;
		uint32_t cache_upper;
		uint32_t play_count;
		uint32_t play_time;
	};

	std::vector<driver_options> m_list;

	// convert audit cache - normally only 1 digit, although we can do 2. If the input is -1, it is treated as invalid and -1 is returned.
	int convert_to_int(const char* inp)
	{
		if (!inp)
			return -1;
		int c = inp[0];
		if (c < 0x30 || c > 0x39)
			return -1;
		int oup = c - 0x30;
		c = inp[1];
		if (c < 0x30 || c > 0x39)
			return oup;
		else
			return oup * 10 + (c - 0x30);
	}

	// convert all other numbers, up to end-of-string/invalid-character. If number is too large, return 0.
	uint32_t convert_to_uint(const char* inp)
	{
		if (!inp)
			return 0;
		uint32_t oup = 0;
		for (int i = 0; i < 11; i++)
		{
			int c = inp[i];
			if (c >= 0x30 && c <= 0x39)
				oup = oup * 10 + (c - 0x30);
			else
				return oup;
		}
		return 0; // numeric overflow
	}

	// true = recache needed
	bool create_index(std::ifstream &fp)
	{
		// Does file exist?
		if (!fp.good())
			return true;

		// Check first line is ok
		std::string file_line;
		std::getline(fp, file_line);
		if (file_line.empty())
			return true;

		// Rudimentary schema check
		stringtokenizer tokenizer(file_line, ",");

		auto cursor = tokenizer.begin();
		if (!cursor.has_next())
			return true;  // File is corrupt

		// Check that the total number of systems match
		auto total_check = cursor.next_token_uint();
		if (*total_check == m_total)
			m_rebuild = false; // Everything is fine so far

		// Second token should be the version number
		auto version_check = cursor.next_token_uint();
		if (!version_check)
			return true; // File is corrupt or old version

		// Check that versions match
		if (*version_check != m_version)
			return true; // File is old version, complete rebuild necessary

		// All good, read file to memory. If rebuild needed, it will overwrite cache, but we need the rest.
		while (std::getline(fp, file_line) && file_line != "$start");

		while (std::getline(fp, file_line))
		{
			int driver_index = -1; // Must be int for validity check below

			// Tab is the delimiter for the first part of the line
			tokenizer.set_input(file_line, "\t");
			
			// First token is the system name
			const char *token = cursor.next_token_cstr();
			if (token)
				driver_index = driver_list::find(token);

			if (driver_index > -1)
			{
				// Now use comma as delimiter for the rest of the line
				tokenizer.set_delimiters(",");
				++cursor; // Skip the system name

				// Get ROM
				token = cursor.next_token_cstr();
				if (token)
					m_list[driver_index].rom = convert_to_int(token);

				// Get Sample
				token = cursor.next_token_cstr();
				if (token)
					m_list[driver_index].sample = convert_to_int(token);

				// Get cache lower bits
				token = cursor.next_token_cstr();
				if (token)
					m_list[driver_index].cache_lower = convert_to_uint(token);

				// Get cache upper bits
				token = cursor.next_token_cstr();
				if (token)
					m_list[driver_index].cache_upper = convert_to_uint(token);

				// Get play count
				token = cursor.next_token_cstr();
				if (token)
					m_list[driver_index].play_count = convert_to_uint(token);

				// Get play time (no need to change delimiter here, the line ends)
				token = cursor.next_token_cstr();
				if (token)
					m_list[driver_index].play_time = convert_to_uint(token);
			}
		}

		fp.close();
		return m_rebuild;
	}

public:
	// construction - runs before main
	winui_game_options()
	{
		m_version = 1;
		m_rebuild = true;
		m_total = driver_list::total();
		m_list.resize(m_total);
		std::fill(m_list.begin(), m_list.end(), driver_options{ 0, -1, -1, 0, 0, 0, 0 });
	}

	int rom(uint32_t index)
	{
		if (index < m_total)
			return m_list[index].rom;
		else
			return -1;
	}

	void rom(uint32_t index, int val)
	{
		if (index < m_total)
			m_list[index].rom = val;
	}

	int sample(uint32_t index)
	{
		if (index < m_total)
			return m_list[index].sample;
		else
			return -1;
	}

	void sample(uint32_t index, int val)
	{
		if (index < m_total)
			m_list[index].sample = val;
	}

	uint32_t cache_lower(uint32_t index)
	{
		if (index < m_total)
			return m_list[index].cache_lower;
		else
			return 0;
	}

	void cache_lower(uint32_t index, uint32_t val)
	{
		if (index < m_total)
			m_list[index].cache_lower = val;
	}

	uint32_t cache_upper(uint32_t index)
	{
		if (index < m_total)
			return m_list[index].cache_upper;
		else
			return 0;
	}

	void cache_upper(uint32_t index, uint32_t val)
	{
		if (index < m_total)
			m_list[index].cache_upper = val;
	}

	uint32_t play_count(uint32_t index)
	{
		if (index < m_total)
			return m_list[index].play_count;
		else
			return 0;
	}

	void play_count(uint32_t index, int val)
	{
		if (index < m_total)
			m_list[index].play_count = val;
	}

	uint32_t play_time(uint32_t index)
	{
		if (index < m_total)
			return m_list[index].play_time;
		else
			return 0;
	}

	void play_time(uint32_t index, int val)
	{
		if (index < m_total)
			m_list[index].play_time = val;
	}

	bool rebuild()
	{
		return m_rebuild;
	}

	void force_rebuild()
	{
		m_rebuild = true;
	}

	void load_file(std::string filename)
	{
		std::ifstream infile (filename);
		if (create_index(infile))
		{
			// rebuild the cache
			device_t::feature_type ft;
			const game_driver *drv = 0;
			windows_options dummy;
			ui_options ui_opts;
			std::cout << "game_opts.h::load_file : Rebuilding cache" << std::endl;
			for (uint32_t i = 0; i < m_total; i++)
			{
				uint32_t t = 0;
				// BITS 0,1 = arcade, console, computer, other
				drv = &driver_list::driver(i);
				machine_config config(*drv, dummy);
				ui::machine_static_info const info(ui_opts, config);
				if ((info.machine_flags() & MACHINE_TYPE_MASK) == MACHINE_TYPE_CONSOLE)
					t = 1;
				else
				if ((info.machine_flags() & MACHINE_TYPE_MASK) == MACHINE_TYPE_COMPUTER)
					t = 2;
				else
				if ((info.machine_flags() & MACHINE_TYPE_MASK) == MACHINE_TYPE_OTHER)
					t = 3;
				m_cache = t;
				// BIT 2 = SWAP_XY
				t = (drv->flags & ORIENTATION_SWAP_XY) ? 0x0004 : 0;
				m_cache |= t;
				// BIT 6 = NOT_WORKING
				t = (info.machine_flags() & MACHINE_NOT_WORKING) ? 0x0040 : 0;
				m_cache |= t;
				// BIT 7 = SUPPORTS_SAVE
				t = (info.machine_flags() & MACHINE_SUPPORTS_SAVE) ? 0: 0x0080;
				m_cache |= t;
				// BIT 8 = NO_COCKTAIL
				t = (info.machine_flags() & MACHINE_NO_COCKTAIL) ? 0x0100 : 0;
				m_cache |= t;
				// BIT 9 = IS_BIOS_ROOT
				t = (info.machine_flags() & MACHINE_IS_BIOS_ROOT) ? 0x0200 : 0;
				m_cache |= t;
				// BIT 10 = REQUIRES_ARTWORK
				t = (info.machine_flags() & MACHINE_REQUIRES_ARTWORK) ? 0x0400 : 0;
				m_cache |= t;
				// BIT 11 = UNOFFICIAL
				t = (info.machine_flags() & MACHINE_UNOFFICIAL) ? 0x1000 : 0;
				m_cache |= t;
				// BIT 12 = NO_SOUND_HW
				t = (info.machine_flags() & MACHINE_NO_SOUND_HW) ? 0x2000 : 0;
				m_cache |= t;
				// BIT 13 = MECHANICAL
				t = (info.machine_flags() & MACHINE_MECHANICAL) ? 0x4000 : 0;
				m_cache |= t;
				// BIT 14 = IS_INCOMPLETE
				t = (info.machine_flags() & MACHINE_IS_INCOMPLETE) ? 0x8000 : 0;
				m_cache |= t;

				ft = info.imperfect_features();
				// BIT 15 = IMPERFECT_SOUND
				t = (ft & device_t::feature::SOUND) ? 0x10000 : 0;
				m_cache |= t;
				// BIT 17 = IMPERFECT_GRAPHICS
				t = (ft & device_t::feature::GRAPHICS) ? 0x40000 : 0;
				m_cache |= t;
				// BIT 19 = IMPERFECT_COLOUR
				t = (ft & device_t::feature::PALETTE) ? 0x100000 : 0;
				m_cache |= t;
				// BIT 21 = PROTECTION
				t = (ft & device_t::feature::PROTECTION) ? 0x400000 : 0;
				m_cache |= t;
				// BIT 22 = IMPERFECT_CONTROLS
				t = (ft & device_t::feature::CONTROLS) ? 0x800000 : 0;
				m_cache |= t;

				ft = info.unemulated_features();
				// BIT 16 = NO_SOUND
				t = (ft & device_t::feature::SOUND) ? 0x20000 : 0;
				m_cache |= t;
				// BIT 18 = NO_GRAPHICS
				t = (ft & device_t::feature::GRAPHICS) ? 0x80000 : 0;
				m_cache |= t;
				// BIT 20 = NO_COLOUR
				t = (ft & device_t::feature::PALETTE) ? 0x200000 : 0;
				m_cache |= t;

				m_list[i].cache_lower = m_cache;
				m_list[i].cache_upper = 0;
			}
			std::cout << "game_opts.h::load_file : Finished Rebuilding cache" << std::endl;
		}
	}

	void save_file(std::string filename)
	{
		std::ofstream outfile;
		std::ostringstream oss_initstring;
		oss_initstring << m_total << "," << m_version << "\n";
		oss_initstring << "YOU CAN SAFELY DELETE THIS FILE TO RESET THE GAME STATS.\n\n$start\n";

		for (uint32_t i = 0; i < m_total; i++)
		{
			// 1:Game name
			oss_initstring << driver_list::driver(i).name << "\t";
			// 2:Game number(for debugging only)
			oss_initstring << m_list[i].game_number << ",";
			// 3:Rom
			oss_initstring << m_list[i].rom << ",";
			// 4:Sample
			oss_initstring << m_list[i].sample << ",";
			// 5:Cache(New)
			oss_initstring << m_list[i].cache_lower << ",";
			// 6:Cache(Legacy)
			oss_initstring << m_list[i].cache_upper << ",";
			// 7:Play Count
			oss_initstring << m_list[i].play_count << ",";
			// 8:Play Time
			oss_initstring << m_list[i].play_time << "\n";
		}

		outfile = std::ofstream(filename, std::ios::out | std::ios::trunc);
		outfile << oss_initstring.str();
		outfile.close();

		return;
	}
};

#endif // MAMEUI_WINAPP_GAMEOPTS_H
