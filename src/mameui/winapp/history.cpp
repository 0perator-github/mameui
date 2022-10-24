// For licensing and usage information, read docs/winui_license.txt
//  MASTER
/***************************************************************************

  history.cpp

 *   history functions.
 *      History database engine
 *      Collect all information on the selected driver, and return it as
 *         a string. Called by winui.cpp

 *      Token parsing by Neil Bradley
 *      Modifications and higher-level functions by John Butler

 *      Further work by Mamesick and Robbbert

 *      Completely rewritten by Robbbert in July 2017
 *      Notes:
 *      - The order listed in m_gameInfo is the order the data is displayed.
 *      - The other tables must have the files in the same places so that
 *             the index numbers line up. Anything with nullptr indicates an
 *             unsupported option (the file doesn't contain the info).
 *      - Each table must contain at least MAX_HFILES members (extra lines
 *             are ignored)
 *      - Software comes first, followed by Game then Source.
***************************************************************************/
// license:BSD-3-Clause
// copyright-holders:Chris Kirmse, Mike Haaland, René Single, Mamesick, Robbbert

// standard C++ headers
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>

// standard windows headers
#include "winapi_common.h"

// MAME headers

// shlwapi.h pulls in objbase.h which defines interface as a struct which conflict with a template declaration in device.h
#ifdef interface
#undef interface // undef interface which is for COM interfaces
#endif
#include "emu.h"
#define interface struct

#include "drivenum.h"
#include "path.h"
#include "romload.h"
#include "screen.h"
#include "speaker.h"

#include "sound/samples.h"

#include "ui/info.h"
#include "ui/moptions.h"
#include "winopts.h"

// MAMEUI headers
#include "mui_cstr.h"
#include "mui_stringtokenizer.h"

#include "bitmask.h"
#include "emu_opts.h"
#include "game_opts.h"
#include "mui_util.h"
#include "mui_opts.h"

#include "history.h"

using namespace mameui::util::string_util;
using namespace std::literals;

/****************************************************************************
 *      struct definitions
 ****************************************************************************/
using HGAMEINFO = struct history_game_info
{
	std::string filename = "";
	std::string header = "";
	std::string descriptor = "";
	bool     bClone = false;    // if nothing found for a clone, try the parent
};

using HSOURCEINFO = struct history_source_info
{
	std::string filename = "";
	std::string header = "";
	std::string descriptor = "";
};

/*************************** START CONFIGURABLE AREA *******************************/
// number of dats we support
#define MAX_HFILES 8
// The order of these is the order they are displayed
const HGAMEINFO m_gameInfo[MAX_HFILES] =
{
	{ "command.dat",  "\n**** :COMMANDS: ****\n\n",         "$cmd",   true },
	{ "gameinit.dat", "\n**** :GAMEINIT: ****\n\n",         "$mame",  true },
	{ "history.xml",  "\n**** :HISTORY: ****\n\n",           "<text>", true },
	{ "mameinfo.dat", "\n**** :MAMEINFO: ****\n\n",         "$mame",  true },
	{ "marp.dat",     "\n**** :MARP HIGH SCORES: ****\n\n", "$marp",  false },
	{ "messinfo.dat", "\n**** :MESSINFO: ****\n\n",         "$mame",  true },
	{ "story.dat",    "\n**** :HIGH SCORES: ****\n\n",      "$story", false },
	{ "sysinfo.dat",  "\n**** :SYSINFO: ****\n\n",          "$bio",   true },
};

const HSOURCEINFO m_sourceInfo[MAX_HFILES] =
{
	{},
	{},
	{},
	{ "mameinfo.dat", "\n***:MAMEINFO DRIVER: ",  "$drv" },
	{},
	{ "messinfo.dat", "\n***:MESSINFO DRIVER: ",  "$drv" },
	{},
	{},
};

const HSOURCEINFO m_swInfo[MAX_HFILES] =
{
	{},
	{},
	{ "history.xml",  "\n**** :HISTORY item: ",     "<text>" },
	{},
	{},
	{},
	{},
	{},
};

/*************************** END CONFIGURABLE AREA *******************************/

uint32_t file_hashes[MAX_HFILES] = { 0 };
std::map<std::string, std::streampos> mymap[MAX_HFILES];


// Function prototypes
static bool create_index_history(std::string_view datsdir, std::ifstream& fp, int filenum);
uint32_t fnv1a_hash(std::ifstream& fp);
static bool create_index(std::string_view datsdir, std::ifstream& fp, int filenum);
void convert_xml(std::string &buf);
static std::string load_datafile_text(std::ifstream &fp, std::string keycode, int filenum, std::string tag);
std::string load_swinfo(const game_driver* drv, std::string_view datsdir, std::string_view software, int filenum);
std::string load_gameinfo(const game_driver *drv, std::string_view datsdir, int filenum);
std::string load_sourceinfo(const game_driver *drv, std::string_view datsdir, int filenum);
std::string load_driver_geninfo(const game_driver *drv, int drvindex);
bool validate_datfiles(void);
std::string GetGameHistory(int driver_index, std::string_view software);

static bool create_index_history(std::string_view datsdir, std::ifstream& fp, int filenum)
{
	using namespace std::string_view_literals;

	const std::string_view tag_system = "<system name="sv;
	const std::string_view tag_item = "<item list="sv;
	const std::string_view quote = "\""sv;

	if (!fp.good()) return false;

	// Get XML version from line 2
	std::string file_line, xml_ver;
	if (std::getline(fp, file_line)) {
		auto start = file_line.find(quote);
		if (start != std::string::npos) {
			start++;
			auto end = file_line.find(quote, start);
			if (end != std::string::npos)
				xml_ver = file_line.substr(start, end - start);
		}
	}

	// Attempt to load existing index
	std::filesystem::path idx_path = std::filesystem::path(datsdir) / "history.idx";
	std::ifstream idx_file(idx_path);
	if (idx_file) {
		std::string idx_ver;
		if (std::getline(idx_file, idx_ver) && idx_ver == xml_ver) {
			while (std::getline(idx_file, file_line)) {
				auto eq_pos = file_line.find('=');
				if (eq_pos != std::string::npos) {
					std::string key = file_line.substr(0, eq_pos);
					int offset = std::stoi(file_line.substr(eq_pos + 1));
					mymap[filenum][key] = offset;
				}
			}
			return true; // Index loaded successfully
		}
	}

	// Rebuild index
	idx_file.close(); // Just in case
	std::streampos key_position = file_line.size() + 2;
	std::streampos text_position = 0;

	while (fp.good()) {
		std::string final_key;
		std::size_t found;

		if ((found = file_line.find(tag_system)) != std::string::npos) {
			auto start = file_line.find(quote);
			if (start != std::string::npos) {
				start++;
				auto end = file_line.find(quote, start);
				if (end != std::string::npos)
					final_key = file_line.substr(start, end - start);
			}
		}
		else if ((found = file_line.find(tag_item)) != std::string::npos) {
			auto start = file_line.find(quote), end = std::string::npos;
			if (start != std::string::npos) {
				start++;
				end = file_line.find(quote, start);
				if (end != std::string::npos) {
					std::string first = file_line.substr(start, end - start);
					start = file_line.find(quote, end + 1);
					if (start != std::string::npos) {
						start++;
						end = file_line.find(quote, start);
						if (end != std::string::npos) {
							std::string second = file_line.substr(start, end - start);
							final_key = first + ":" + second;
						}
					}
				}
			}
		}

		// Seek to next <text> tag
		if (!final_key.empty()) {
			if (key_position > text_position) {
				text_position = key_position;
				while (std::getline(fp, file_line)) {
					auto text_pos = file_line.find("<text>");
					if (text_pos != std::string::npos) {
						text_position += text_pos + 6;
						break;
					}
					text_position += file_line.size() + 2;
				}
			}
			mymap[filenum][final_key] = static_cast<int>(text_position);
		}

		fp.seekg(key_position);
		if (!std::getline(fp, file_line)) break;
		key_position += file_line.size() + 2;
	}

	// Write new index
	std::ofstream out(idx_path, std::ios::out);
	if (!out.is_open()) {
		std::cerr << "Unable to open history.idx for writing.\n";
		return false;
	}

	out << xml_ver << "\n";
	for (const auto& [key, pos] : mymap[filenum]) {
		out << key << "=" << pos << "\n";
	}
	return true;
}

uint32_t fnv1a_hash(std::ifstream& fp)
{
	if (!fp.good())
		return 0;

	fp.clear();
	fp.seekg(0, std::ios::beg);  // Make sure we read from the start

	uint32_t hash = 2166136261u;
	char buffer[4096];

	while (fp.read(buffer, sizeof(buffer)) || fp.gcount())
	{
		for (std::streamsize i = 0; i < fp.gcount(); ++i)
		{
			hash ^= static_cast<unsigned char>(buffer[i]);
			hash *= 16777619u;
		}
	}

	fp.clear();   // Reset flags for future use
	fp.seekg(0);  // Rewind again in case caller needs to reuse

	return hash;
}

static bool create_index(std::string_view datsdir, std::ifstream &fp, int filenum)
{
	if (!fp.good())
		return false;

	// same file as before?
	uint32_t new_hash = fnv1a_hash(fp);
	if (new_hash == file_hashes[filenum])
		return true;

	// new file, it needs to be indexed
	file_hashes[filenum] = new_hash;
	mymap[filenum].clear();

	while (fp)
	{
		std::streampos position = fp.tellg();  // store start of this line
		std::string file_line;
		if (!std::getline(fp, file_line)) break;

		// must start with $ and have exactly one '='
		if ((!file_line.empty() && file_line[0] == '$') && (std::count(file_line.begin(), file_line.end(), '=') == 1))
		{
			file_line.erase(remove_if(file_line.begin(), file_line.end(), ::isspace), file_line.end());
			stringtokenizer tokenizer(file_line, "=,");
			auto token_iterator = tokenizer.begin();

			const std::string &first = token_iterator.advance_as_string();
			if (!first.empty())
			{
				while (token_iterator != tokenizer.end())
				{
					const std::string &second = token_iterator.advance_as_string();
					if (!second.empty())
					{
						std::string key = first + "=" + second;
						mymap[filenum][key] = position;
					}
				}
			}
		}
	}


	// check contents
//  if (filenum == 0)
//  for (auto const &it : mymap[filenum])
//      std::cout << it.first << " = " << std::hex << it.second << std::dec << "\n";
	return true;
}

void convert_xml(std::string &buf)
{
	// Convert XML entities to actual characters
	auto replaceXML = [&](std::string_view xml, std::string_view replacement)
		{
			size_t pos;
			while ((pos = buf.find(xml)) != buf.npos)
			{
				buf.replace(pos, xml.length(), replacement);
			}
		};

	replaceXML("&amp;", "&");
	replaceXML("&apos;", "'");
	replaceXML("&quot;", "\"");
	replaceXML("&lt;", "<");
	replaceXML("&gt;", ">");
}

static std::string load_datafile_text(std::ifstream &fp, std::string keycode, int filenum, std::string tag)
{
	std::string readbuf;

	auto search = mymap[filenum].find(keycode);
	if (search != mymap[filenum].end())
	{
		std::streampos offset = mymap[filenum].find(keycode)->second;
		fp.seekg(offset);
		std::string file_line;

		// read text until buffer is full or end of entry is encountered */
		while (std::getline(fp, file_line))
		{
			// if (filenum == 6) ("*******2: %s\n",file_line.c_str());
			if (file_line == "- CONTRIBUTE -")
				break;

			if (file_line.find("$end")==0)
				break;

			if (file_line.find("</text>") != file_line.npos)
				break;

			if (file_line.find(tag)==0)
				continue;

			readbuf += file_line + "\n";
		}
	}

	return readbuf;
}

std::string load_swinfo(const game_driver* drv, std::string_view datsdir, std::string_view software, int filenum)
{
	// if it's an empty record, exit now
	if (m_swInfo[filenum].filename.empty())
		return {};

	std::string buffer;

	// datafile name
	std::filesystem::path filename = std::filesystem::path(datsdir) / m_swInfo[filenum].filename;
	std::ifstream fp(filename.string());
	if (!fp) // File could not be opened
		return {};

	// Try to open datafile
	bool index_created = (filenum == 2) ? create_index_history(datsdir, fp, filenum) : create_index(datsdir, fp, filenum);
	if (index_created)
	{
		// Split the software string into system and software parts
		auto delimiter_pos = software.find(':');
		if (delimiter_pos == software.npos)
			return {}; // Handle invalid format gracefully

		std::string system(software.substr(0, delimiter_pos));
		std::string software_name(software.substr(delimiter_pos + 1));

		// Construct the key string
		std::string key = "$" + system + "=" + software_name;

		// Get info on software
		std::string dat_filename = load_datafile_text(fp, std::move(key), filenum, m_swInfo[filenum].descriptor);
		if (!dat_filename.empty())
			buffer += std::string(m_swInfo[filenum].header) + software_name + "\n" + dat_filename + "\n\n\n";

		fp.close();
	}

	if (filenum == 2)
		convert_xml(buffer);

	return buffer;
}

std::string load_gameinfo(const game_driver *drv, std::string_view datsdir, int filenum)
{
	// if it's a empty record exit now
	if (m_gameInfo[filenum].filename.empty())
		return {};

	std::string buffer;

	// datafile name
	std::filesystem::path filename = std::filesystem::path(datsdir) / m_gameInfo[filenum].filename;
	std::ifstream fp (filename);
	if (!fp) // File could not be opened
		return {};

	/* try to open datafile */
	bool index_created = (filenum == 2) ? create_index_history(datsdir, fp, filenum) : create_index(datsdir, fp, filenum);
	if (index_created)
	{
		std::string first = "$info="s + drv->name;

		// get info on game
		std::string dat_filename = load_datafile_text(fp, std::move(first), filenum, m_gameInfo[filenum].descriptor);
		if (dat_filename.empty() && m_gameInfo[filenum].bClone) // if nothing, and it's a clone, and it's allowed, try the parent
		{
			int g = driver_list::clone(*drv);
			if (g != -1)
			{
				drv = &driver_list::driver(g);
				first = "$info="s + drv->name;
				dat_filename = load_datafile_text(fp, std::move(first), filenum, m_gameInfo[filenum].descriptor);
			}
		}

		if (!dat_filename.empty())
			buffer += std::string(m_gameInfo[filenum].header) + dat_filename + "\n\n\n";

		fp.close();
	}

	if (filenum == 2)
		convert_xml(buffer);

	return buffer;
}

std::string load_sourceinfo(const game_driver *drv, std::string_view datsdir, int filenum)
{
	// if it's a empty record exit now
	if (m_sourceInfo[filenum].filename.empty())
		return {};

	std::string buffer;

	// datafile name
	std::filesystem::path filename = std::filesystem::path(datsdir) / m_sourceInfo[filenum].filename;
	std::ifstream fp (filename);
	if (!fp) // File could not be opened
		return {};

	std::string source = drv->type.source();
	size_t i = source.find_last_of("/");
	if(i != source.npos)
		source.erase(0,i+1);

	if (create_index(datsdir, fp, filenum))
	{
		std::string first = "$info=" + source;

		// get info on game driver source
		std::string dat_content = load_datafile_text(fp, std::move(first), filenum, m_sourceInfo[filenum].descriptor);
		if (!dat_content.empty())
			buffer += std::string(m_sourceInfo[filenum].header) + source + "\n" + dat_content + "\n\n\n";

		fp.close();
	}

	return buffer;
}


// General hardware information
std::string load_driver_geninfo(const game_driver* drv, int drvindex)
{
	machine_config config(*drv, emu_opts.GetGlobalOpts());
	const game_driver* parent = nullptr;
	bool is_bios = false;
	std::ostringstream general_machine_info;

	// Prepare the header
	general_machine_info << "\n**** :GENERAL MACHINE INFO: ****\n\n";

	// List the game info 'flags'
	uint64_t cache = GetDriverCacheLower(drvindex);
	if (is_flag_set(cache, lower_cache::NOT_WORKING)) // NOT_WORKING
		general_machine_info << "This game doesn't work properly.\n";

	if (is_flag_set(cache, lower_cache::UNEMULATED_PROTECTION)) // UNEMULATED_PROTECTION
		general_machine_info << "This game has protection which isn't fully emulated.\n";

	if (is_flag_set(cache, lower_cache::IMPERFECT_GRAPHICS)) // IMPERFECT_GRAPHICS
		general_machine_info << "The graphics emulation isn't 100% accurate.\n";

	if (is_flag_set(cache, lower_cache::WRONG_COLORS)) // WRONG_COLORS
		general_machine_info << "The colors are completely wrong.\n";

	if (is_flag_set(cache, lower_cache::IMPERFECT_COLOR)) // IMPERFECT_COLOR
		general_machine_info << "The colors are not 100% accurate.\n";

	if (is_flag_set(cache, lower_cache::NO_SOUND)) // NO_SOUND
		general_machine_info << "This game lacks sound.\n";

	if (is_flag_set(cache, lower_cache::IMPERFECT_SOUND)) // IMPERFECT_SOUND
		general_machine_info << "The sound emulation isn't 100% accurate.\n";

	if (is_flag_set(cache, lower_cache::SAVE_SUPPORTED)) // SAVE_SUPPORTED
		general_machine_info << "This game supports save states.\n";
	else
		general_machine_info << "This game does not support save states.\n";

	if (is_flag_set(cache, lower_cache::MECHANICAL)) // MECHANICAL
		general_machine_info << "This game contains mechanical parts.\n";

	if (is_flag_set(cache, lower_cache::IS_INCOMPLETE)) // IS_INCOMPLETE
		general_machine_info << "This game is incomplete.\n";

	if (is_flag_set(cache, lower_cache::NO_SOUND_HW)) // NO_SOUND_HW
		general_machine_info << "This game has no sound hardware.\n";

	if (drv->flags & MACHINE_IS_BIOS_ROOT)
		is_bios = true;

	// GAME INFORMATIONS
	general_machine_info << "\n\nGAME: " << drv->name << "\n" << drv->type.fullname();
	general_machine_info << " (" << drv->manufacturer << " " << drv->year << ")\n\nCPU:\n";

	// iterate over CPUs
	execute_interface_enumerator cpuiter(config.root_device());
	std::unordered_set<std::string> exectags;

	for (device_execute_interface& exec : cpuiter)
	{
		if (!exectags.insert(exec.device().tag()).second)
			continue;

		int count = 1;
		int clock = exec.device().clock();
		const char* cpu_name = exec.device().name();

		for (device_execute_interface& scan : cpuiter)
			if (exec.device().type() == scan.device().type() && !mui_strcmp(cpu_name, scan.device().name()) && clock == scan.device().clock())
				if (exectags.insert(scan.device().tag()).second)
					count++;

		if (count > 1)
			general_machine_info << count << " x ";

		if (clock >= 1000000)
			general_machine_info << cpu_name << " " << clock / 1000000 << "." << std::setfill('0') << std::setw(6) << clock % 1000000 << " MHz\n";
		else
			general_machine_info << cpu_name << " " << clock / 1000 << "." << std::setfill('0') << std::setw(3) << clock % 1000 << " kHz\n";
	}

	general_machine_info << "\nSOUND:\n";
	bool has_sound = false;
	// iterate over sound chips
	sound_interface_enumerator sounditer(config.root_device());
	std::unordered_set<std::string> soundtags;

	for (device_sound_interface& sound : sounditer)
	{
		if (!soundtags.insert(sound.device().tag()).second)
			continue;

		has_sound = true;
		int count = 1;
		int clock = sound.device().clock();
		const char* sound_name = sound.device().name();

		for (device_sound_interface& scan : sounditer)
			if (sound.device().type() == scan.device().type() && !mui_strcmp(sound_name, scan.device().name()) && clock == scan.device().clock())
				if (soundtags.insert(scan.device().tag()).second)
					count++;

		if (count > 1)
			general_machine_info << count << " x ";

		general_machine_info << sound_name;

		if (clock)
		{
			if (clock >= 1000000)
				general_machine_info << " " << clock / 1000000 << "." << std::setfill('0') << std::setw(6) << clock % 1000000 << " MHz\n";
			else
				general_machine_info << " " << clock / 1000 << "." << std::setfill('0') << std::setw(3) << clock % 1000 << " kHz\n";
		}
	}

	if (has_sound)
	{
		speaker_device_enumerator audioiter(config.root_device());
		int channels = audioiter.count();

		if (channels == 1)
			general_machine_info << "1 Channel\n";
		else
			general_machine_info << channels << " Channels\n";
	}
	else
		general_machine_info << "No sound hardware\n";

	general_machine_info << "\nVIDEO:\n";
	screen_device_enumerator screeniter(config.root_device());
	int scrcount = screeniter.count();

	if (scrcount == 0)
		general_machine_info << "Screenless\n";
	else
	{
		for (screen_device& screen : screeniter)
		{
			if (screen.screen_type() == SCREEN_TYPE_VECTOR)
				general_machine_info << "Vector\n";
			else
			{
				const rectangle& visarea = screen.visible_area();

				if (drv->flags & ORIENTATION_SWAP_XY)
					general_machine_info << visarea.width() << " x " << visarea.height() << " (V) " << ATTOSECONDS_TO_HZ(screen.refresh_attoseconds()) << " Hz\n";
				else
					general_machine_info << visarea.width() << " x " << visarea.height() << " (H) " << ATTOSECONDS_TO_HZ(screen.refresh_attoseconds()) << " Hz\n";
			}
		}
	}

	general_machine_info << "\nROM REGION:\n";

	int driver_index = driver_list::clone(*drv);
	if (driver_index < 0)
		general_machine_info << "None\n";
	else
	{
		parent = &driver_list::driver(driver_index);

		for (device_t& device : device_enumerator(config.root_device()))
		{
			for (const rom_entry* region = rom_first_region(device); region; region = rom_next_region(region))
			{
				for (const rom_entry* rom = rom_first_file(region); rom; rom = rom_next_file(rom))
				{
					util::hash_collection hashes(rom->hashdata());

					if (driver_index != -1)
					{
						machine_config pconfig(*parent, emu_opts.GetGlobalOpts());

						for (device_t& device : device_enumerator(pconfig.root_device()))
						{
							for (const rom_entry* pregion = rom_first_region(device); pregion; pregion = rom_next_region(pregion))
							{
								for (const rom_entry* prom = rom_first_file(pregion); prom; prom = rom_next_file(prom))
								{
									util::hash_collection phashes(prom->hashdata());

									if (hashes == phashes)
										break;
								}
							}
						}
					}

					general_machine_info << std::left << std::setfill(' ') << std::setw(16) << ROM_GETNAME(rom) << " \t";
					general_machine_info << std::setfill('0') << std::setw(9) << rom_file_size(rom) << " \t";
					general_machine_info << std::left << std::setfill(' ') << std::setw(10) << region->name() << "\n";
				}
			}
		}
	}

	for (samples_device &device : samples_device_enumerator(config.root_device()))
	{
		samples_iterator sampiter(device);

		if (sampiter.altbasename())
			general_machine_info << "\nSAMPLES: " << sampiter.altbasename() << "\n";


		std::unordered_set<std::string> already_printed;

		for (const char *samplename = sampiter.first(); samplename; samplename = sampiter.next())
		{
			// filter out duplicates
			if (!already_printed.insert(samplename).second)
				continue;

			// output the sample name
			general_machine_info << samplename << ".wav\n";
		}
	}

	if (!is_bios)
	{

		general_machine_info << "\nORIGINAL:\n";

		int clone_index = driver_list::clone(*drv);
		const game_driver& original = (clone_index >= 0) ? driver_list::driver(clone_index) : *drv;

		if (original.flags & MACHINE_IS_BIOS_ROOT)
			general_machine_info << drv->type.fullname() << "\n";  // BIOS, show this driver
		else
			general_machine_info << original.type.fullname() << "\n";

		general_machine_info << "\nCLONES:\n";

		bool has_clones = false;

		driver_index = 0;
		while (driver_index < driver_list::total())
		{
			game_driver const& current_driver = driver_list::driver(driver_index);
			if (mui_strcmp(drv->name, current_driver.parent) == 0)
			{
				if (!has_clones)
					has_clones = true;

				general_machine_info << current_driver.type.fullname() << "\n";
			}
			driver_index++;
		}

		if (!has_clones)
			general_machine_info << "None\n";
	}

	std::string source_file = std::string(core_filename_extract_base(drv->type.source(), false));
	general_machine_info << "\nGENERAL SOURCE INFO: " << source_file << "\n\nGAMES SUPPORTED:\n";

	for (size_t i = 0; i < driver_list::total(); i++)
	{
		std::string current_source = driver_list::driver(i).type.source();
		size_t j = current_source.find_last_of("/");
		current_source.erase(0, j+1);
		if (!mui_strcmp(source_file, current_source) && !(DriverIsBios(i)))
			general_machine_info << driver_list::driver(i).type.fullname() << "\n";
	}

	return general_machine_info.str();
}

// This is check that the tables are at least as big as they should be
bool validate_datfiles(void)
{
	bool result = true;
	if (std::size(m_gameInfo) < MAX_HFILES)
	{
		std::cout << "m_gameInfo needs to have at least MAX_HFILES members" << "\n";
		result = false;
	}

	if (std::size(m_sourceInfo) < MAX_HFILES)
	{
		std::cout << "m_sourceInfo needs to have at least MAX_HFILES members" << "\n";
		result = false;
	}

	if (std::size(m_swInfo) < MAX_HFILES)
	{
		std::cout << "m_swInfo needs to have at least MAX_HFILES members" << "\n";
		result = false;
	}

	return result;
}

bool FindFirstExistingPath(const std::string &paths, std::filesystem::path &out_path)
{
	stringtokenizer tokenizer(paths, ";");
	for (const auto &token: tokenizer)
	{
		if (!token.empty() && std::filesystem::exists(token))
		{
			out_path = token;
			return true;
		}
	}
	return false;
}

void AppendHistoryFiles(const game_driver& drv, const std::string& history_str, std::string_view software, std::string& fullbuf)
{
	if (software.empty() || software.find(':') == software.npos)
	{
		// no software: load gameinfo and sourceinfo only
		for (size_t filenum = 0; filenum < MAX_HFILES; filenum++)
		{
			fullbuf.append(load_gameinfo(&drv, history_str, filenum));
			fullbuf.append(load_sourceinfo(&drv, history_str, filenum));
		}
	}
	else
	{
		// software specified: load swinfo, gameinfo, and sourceinfo
		for (size_t filenum = 0; filenum < MAX_HFILES; filenum++)
		{
			fullbuf.append(load_swinfo(&drv, history_str, software, filenum));
			fullbuf.append(load_gameinfo(&drv, history_str, filenum));
			fullbuf.append(load_sourceinfo(&drv, history_str, filenum));
		}
	}
}

// For all of MAME builds - called by winui.cpp
std::string GetGameHistory(int driver_index, std::string_view software)
{
	if (driver_index < 0)
		return {};

	const game_driver &drv = driver_list::driver(driver_index);
	std::string fullbuf;

	if (!validate_datfiles())
		return "\nUnable to display info due to an internal error.\n\n\n"s;

	std::filesystem::path history_path;
	std::string history_paths = emu_opts.dir_get_value(DIRPATH_HISTORY_PATH);

	if (!FindFirstExistingPath(history_paths, history_path))
		return "\nThe path to your dat files is invalid.\n\n\n"s;

	const std::string history_str = history_path.string();

	AppendHistoryFiles(drv, history_str, software, fullbuf);

	fullbuf.append(load_driver_geninfo(&drv, driver_index));

	return ConvertToWindowsNewlines(fullbuf);
}
