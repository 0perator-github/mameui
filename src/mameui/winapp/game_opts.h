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
	uint32_t m_version;
	bool m_rebuild;
	bool m_force_rebuild;

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

	// true = recache needed
	bool create_index(std::ifstream &fp)
	{
		// Does file exist?
		if (!fp.good())
			return true;

		// Check first line is ok
		std::string file_line;
		if (!std::getline(fp, file_line) || file_line.empty())
			return true;

		// Schema check on the first line
		stringtokenizer tokenizer(file_line, ",");
		auto token_iterator = tokenizer.begin();

		// Check that the total number of systems match
		auto total_check = token_iterator.advance_as_uint();
		if (!total_check || *total_check != m_total)
			return true;

		// Check that versions match
		auto version_check = token_iterator.as_uint();
		if (!version_check || *version_check != m_version)
			return true;

		// Skip to $start marker
		while (std::getline(fp, file_line))
		{
			if (file_line == "$start")
				break;
		}

		for (int driver_index = 0; std::getline(fp, file_line); ++driver_index)
		{
			tokenizer.set_input(file_line, "\t");
			token_iterator = tokenizer.begin();

			// System Name
			const char* system_name = token_iterator.c_str(); // don't advance yet
			if (!system_name || *system_name == '\0')
				continue;

			tokenizer.set_delimiters(",");
			++token_iterator; // now advance because we changed delimiters

			// Game Number
			auto token_uint = token_iterator.advance_as_uint();
			if (token_uint) m_list[driver_index].game_number = *token_uint;

			// ROM
			auto token_int = token_iterator.advance_as_int();
			if (token_int) m_list[driver_index].rom = *token_int;
			
			// Sample
			token_int = token_iterator.advance_as_int();
			if (token_int) m_list[driver_index].sample = *token_int;
			
			// Cache Lower
			token_uint = token_iterator.advance_as_uint();
			if (token_uint) m_list[driver_index].cache_lower = *token_uint;

			// Cache Upper
			token_uint = token_iterator.advance_as_uint();
			if (token_uint) m_list[driver_index].cache_upper = *token_uint;
			
			// Play Count
			token_uint = token_iterator.advance_as_uint();
			if (token_uint) m_list[driver_index].play_count = *token_uint;

			// Play Time
			token_uint = token_iterator.as_uint();
			if (token_uint) m_list[driver_index].play_time = *token_uint;
		}

		return false;
	}

public:
	// construction - runs before main
	winui_game_options()
	{
		m_version = 1;
		m_rebuild = true;
		m_force_rebuild = false;
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
		m_force_rebuild = true;
	}

	inline void set_bit(uint32_t& target, bool condition, uint32_t bit_mask)
	{
		if (condition) target |= bit_mask;
	}

	uint32_t build_cache_flags(const game_driver& drv, const ui::machine_static_info& info)
	{
		uint32_t flags = 0;

		// Machine Types
		switch (info.machine_flags() & MACHINE_TYPE_MASK)
		{
		// BITS 0,1 = arcade, console, computer, other
		case MACHINE_TYPE_ARCADE:   break; // t = 0 (default)
		case MACHINE_TYPE_CONSOLE:  flags |= 0x0001; break;
		case MACHINE_TYPE_COMPUTER: flags |= 0x0002; break;
		case MACHINE_TYPE_OTHER:    flags |= 0x0003; break;
		}

		// Machine Features
		set_bit(flags, drv.flags & ORIENTATION_SWAP_XY, 0x0004); // BIT 2 = SWAP_XY
		set_bit(flags, info.machine_flags() & MACHINE_NOT_WORKING, 0x0040); // BIT 6 = NOT_WORKING
		set_bit(flags, info.machine_flags() & MACHINE_SUPPORTS_SAVE, 0x0080); // BIT 7 = SUPPORTS_SAVE
		set_bit(flags, info.machine_flags() & MACHINE_NO_COCKTAIL, 0x0100); // BIT 8 = NO_COCKTAIL
		set_bit(flags, info.machine_flags() & MACHINE_IS_BIOS_ROOT, 0x0200); // BIT 9 = IS_BIOS_ROOT
		set_bit(flags, info.machine_flags() & MACHINE_REQUIRES_ARTWORK, 0x0400); // BIT 10 = REQUIRES_ARTWORK
		set_bit(flags, info.machine_flags() & MACHINE_UNOFFICIAL, 0x1000); // BIT 11 = UNOFFICIAL
		set_bit(flags, info.machine_flags() & MACHINE_NO_SOUND_HW, 0x2000); // BIT 12 = NO_SOUND_HW
		set_bit(flags, info.machine_flags() & MACHINE_MECHANICAL, 0x4000); // BIT 13 = MECHANICAL
		set_bit(flags, info.machine_flags() & MACHINE_IS_INCOMPLETE, 0x8000); // BIT 14 = IS_INCOMPLETE

		// Imperfect Features
		device_t::feature_type ft = info.imperfect_features();
		set_bit(flags, ft & device_t::feature::SOUND, 0x10000); // BIT 15 = IMPERFECT_SOUND
		set_bit(flags, ft & device_t::feature::GRAPHICS, 0x40000); // BIT 17 = IMPERFECT_GRAPHICS
		set_bit(flags, ft & device_t::feature::PALETTE, 0x100000); // BIT 19 = IMPERFECT_COLOR
		set_bit(flags, ft & device_t::feature::PROTECTION, 0x400000); // BIT 21 = PROTECTION
		set_bit(flags, ft & device_t::feature::CONTROLS, 0x800000); // BIT 22 = IMPERFECT_CONTROLS

		// Unemulated Features
		ft = info.unemulated_features();
		set_bit(flags, ft & device_t::feature::SOUND, 0x20000); // BIT 16 = NO_SOUND
		set_bit(flags, ft & device_t::feature::GRAPHICS, 0x80000); // BIT 18 = NO_GRAPHICS
		set_bit(flags, ft & device_t::feature::PALETTE, 0x200000); // BIT 20 = NO_COLOR

		return flags;
	}

	void load_file(std::string filename)
	{
		std::ifstream infile(filename);
		
		m_rebuild = (m_force_rebuild) ? true : create_index(infile);
		if (m_rebuild)
		{
			// rebuild the cache
			std::cout << "game_opts.h::load_file : Rebuilding cache" << std::endl;

			windows_options dummy;
			ui_options ui_opts;

			for (uint32_t i = 0; i < m_total; ++i)
			{
				const game_driver& drv = driver_list::driver(i);
				machine_config config(drv, dummy);
				ui::machine_static_info info(ui_opts, config);

				uint32_t flags = build_cache_flags(drv, info);
				m_list[i].cache_lower = flags;
				m_list[i].cache_upper = 0;
			}

			std::cout << "game_opts.h::load_file : Finished Rebuilding cache" << std::endl;
			if (m_force_rebuild) m_force_rebuild = false;
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
			oss_initstring << i << ",";
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
		if (!outfile.is_open())
		{
			std::cerr << "game_opts.h::save_file : Error opening file for writing: " << filename << std::endl;
			return;
		}
		outfile << oss_initstring.str();
		outfile.close();

		return;
	}
};

#endif // MAMEUI_WINAPP_GAMEOPTS_H
