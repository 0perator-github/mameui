// For licensing and usage information, read docs/winui_license.txt
// MASTER
//****************************************************************************

#ifndef MAMEUI_WINAPP_GAMEOPTS_H
#define MAMEUI_WINAPP_GAMEOPTS_H

#pragma once

using namespace mameui::util;

using lower_cache = struct lower_cache_flags
{
	enum type : uint64_t
	{
		// --- Machine Driver Flags ---

		// Bit 0–2: screen orientation
		MASK_ORIENTATION    = 7ul << 0,

		FLIP_X              = 1ul << 0,
		FLIP_Y              = 1ul << 1,
		SWAP_XY             = 1ul << 2,
		ROT0                = 0ul << 0, // default orientation
		ROT90               = FLIP_X | SWAP_XY,
		ROT180              = FLIP_X | FLIP_Y,
		ROT270              = FLIP_Y | SWAP_XY,

		// Bit 3–4: system type
		MASK_SYSTEMTYPE     = 3ul << 3,

		SYSTEMTYPE_ARCADE   = 0ul << 3, // default system type
		SYSTEMTYPE_CONSOLE  = 1ul << 3,
		SYSTEMTYPE_COMPUTER = 2ul << 3,
		SYSTEMTYPE_OTHER    = 3ul << 3,

		// Bit 5–11: status and feature flags
		NO_COCKTAIL         = 1ul << 5,
		IS_BIOS_ROOT        = 1ul << 6,
		REQUIRES_ARTWORK    = 1ul << 7,
		UNOFFICIAL          = 1ul << 8,
		NO_SOUND_HW         = 1ul << 9,
		MECHANICAL          = 1ul << 10,
		IS_INCOMPLETE       = 1ul << 11,

		// --- Device Emulation And Feature Flags ---

		// Bit 32-33: Emulation features
		NOT_WORKING          = 1ull << 32,
		SAVE_SUPPORTED       = 1ull << 33,

		// Bit 34-38: Unemulated features
		WRONG_COLORS         = 1ull << 34,
		NO_SOUND             = 1ull << 35,
		NODEVICE_MICROPHONE  = 1ull << 36,
		NODEVICE_PRINTER     = 1ull << 37,
		NODEVICE_LAN         = 1ull << 38,

		// Imperfect features
		UNEMULATED_PROTECTION = 1ull << 39,
		IMPERFECT_COLOR       = 1ull << 40,
		NO_GRAPHICS           = 1ull << 41,
		IMPERFECT_GRAPHICS    = 1ull << 42,
		IMPERFECT_SOUND       = 1ull << 43,
		IMPERFECT_CONTROLS    = 1ull << 44,
		IMPERFECT_TIMING      = 1ull << 45
	};

};

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
		uint64_t cache_lower;
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
		using mameui::util::string_util::stringtokenizer;
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
			auto token_ulong = token_iterator.advance_as_ulong();
			if (token_uint) m_list[driver_index].cache_lower = *token_ulong;

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
		std::fill(m_list.begin(), m_list.end(), driver_options{ 0u, -1, -1, 0ul, 0u, 0u, 0u });
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

	uint64_t cache_lower(uint32_t index)
	{
		if (index < m_total)
			return m_list[index].cache_lower;
		else
			return 0ul;
	}

	void cache_lower(uint32_t index, uint64_t val)
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

	uint64_t build_cache_flags(const game_driver &drv, const ui::machine_static_info &info)
	{
		uint64_t flags = 0;

		// --- Machine Flags ---

		set_bit<uint64_t>(flags, (drv.flags & ORIENTATION_SWAP_XY) != 0, lower_cache::SWAP_XY);


		// Set system type bits
		machine_flags::type machine = info.machine_flags();
		switch (machine & machine_flags::MASK_SYSTEMTYPE)
		{
		case machine_flags::SYSTEMTYPE_CONSOLE:  flags |= lower_cache::SYSTEMTYPE_CONSOLE; break;
		case machine_flags::SYSTEMTYPE_COMPUTER: flags |= lower_cache::SYSTEMTYPE_COMPUTER; break;
		case machine_flags::SYSTEMTYPE_OTHER:    flags |= lower_cache::SYSTEMTYPE_OTHER; break;
		default: break; // Default is arcade: 00, so no bits set
		}

		// --- Machine Features ---
		set_bit<uint64_t>(flags, (machine & machine_flags::NO_COCKTAIL) != 0, lower_cache::NO_COCKTAIL);
		set_bit<uint64_t>(flags, (machine & machine_flags::IS_BIOS_ROOT) != 0, lower_cache::IS_BIOS_ROOT);
		set_bit<uint64_t>(flags, (machine & machine_flags::REQUIRES_ARTWORK) != 0, lower_cache::REQUIRES_ARTWORK);
		set_bit<uint64_t>(flags, (machine & machine_flags::UNOFFICIAL) != 0, lower_cache::UNOFFICIAL);
		set_bit<uint64_t>(flags, (machine & machine_flags::NO_SOUND_HW) != 0, lower_cache::NO_SOUND_HW);
		set_bit<uint64_t>(flags, (machine & machine_flags::MECHANICAL) != 0, lower_cache::MECHANICAL);
		set_bit<uint64_t>(flags, (machine & machine_flags::IS_INCOMPLETE) != 0, lower_cache::IS_INCOMPLETE);

		// --- Emulated Features ---
		device_t::flags_type emulation = info.emulation_flags();
		set_bit<uint64_t>(flags, (emulation & device_t::flags::NOT_WORKING) != 0, lower_cache::NOT_WORKING);
		set_bit<uint64_t>(flags, (emulation & device_t::flags::SAVE_UNSUPPORTED) == 0, lower_cache::SAVE_SUPPORTED);

		// --- Imperfect Features ---
		device_t::feature_type imperfect = info.imperfect_features();
		set_bit<uint64_t>(flags, (imperfect & device_t::feature::SOUND) != 0, lower_cache::IMPERFECT_SOUND);
		set_bit<uint64_t>(flags, (imperfect & device_t::feature::GRAPHICS) != 0, lower_cache::IMPERFECT_GRAPHICS);
		set_bit<uint64_t>(flags, (imperfect & device_t::feature::PALETTE) != 0, lower_cache::IMPERFECT_COLOR);
		set_bit<uint64_t>(flags, (imperfect & device_t::feature::PROTECTION) != 0, lower_cache::UNEMULATED_PROTECTION);
		set_bit<uint64_t>(flags, (imperfect & device_t::feature::CONTROLS) != 0, lower_cache::IMPERFECT_CONTROLS);

		// --- Unemulated Features ---
		device_t::feature_type unemulated = info.unemulated_features();
		set_bit<uint64_t>(flags, (unemulated & device_t::feature::SOUND) != 0, lower_cache::NO_SOUND);
		set_bit<uint64_t>(flags, (unemulated & device_t::feature::GRAPHICS) != 0, lower_cache::NO_GRAPHICS);
		set_bit<uint64_t>(flags, (unemulated & device_t::feature::PALETTE) != 0, lower_cache::WRONG_COLORS);

		return flags;
	}

	void load_file(std::string filename)
	{
		std::ifstream infile(filename);

		m_rebuild = (m_force_rebuild) ? true : create_index(infile);
		if (m_rebuild)
		{
			// rebuild the cache
			std::cout << "game_opts.h::load_file : Rebuilding cache" << "\n";

			windows_options dummy;
			ui_options ui_opts;

			for (uint32_t i = 0; i < m_total; ++i)
			{
				const game_driver &drv = driver_list::driver(i);
				machine_config config(drv, dummy);
				ui::machine_static_info info(ui_opts, config);

				uint64_t flags = build_cache_flags(drv, info);
				m_list[i].cache_lower = flags;
				m_list[i].cache_upper = 0;
			}

			std::cout << "game_opts.h::load_file : Finished Rebuilding cache" << "\n";
			if (m_force_rebuild) m_force_rebuild = false;
		}
	}

	void save_file(std::string filename)
	{
		std::ofstream outfile(filename, std::ios::out | std::ios::trunc);
		if (!outfile.is_open())
		{
			std::cerr << "game_opts.h::save_file : Error opening file for writing: " << filename << "\n";
			return;
		}

		outfile << m_total << "," << m_version << "\n";
		outfile << "YOU CAN SAFELY DELETE THIS FILE TO RESET THE GAME STATS.\n\n$start\n";

		for (uint32_t i = 0; i < m_total; ++i)
		{
			const game_driver &drv = driver_list::driver(i);
			const driver_options &entry = m_list[i];
			outfile
				// 1:Game name
				<< drv.name << "\t"
				// 2:Game number
				<< i << ","
				// 3:Rom
				<< entry.rom << ","
				// 4:Sample
				<< entry.sample << ","
				// 5:Cache(New)
				<< entry.cache_lower << ","
				// 6:Cache(Legacy)
				<< entry.cache_upper << ","
				// 7:Play Count
				<< entry.play_count << ","
				// 8:Play Time
				<< entry.play_time << "\n";
		}
	}
};

#endif // MAMEUI_WINAPP_GAMEOPTS_H
