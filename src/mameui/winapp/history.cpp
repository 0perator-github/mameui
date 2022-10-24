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

static bool create_index(std::string_view datsdir, std::ifstream& fp, int filenum)
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
//      printf("%s = %X\n", it.first.c_str(), int(it.second));
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
		std::string dat_filename = load_datafile_text(fp, key, filenum, m_swInfo[filenum].descriptor);
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
		std::string dat_filename = load_datafile_text(fp, first, filenum, m_gameInfo[filenum].descriptor);
		if (dat_filename.empty() && m_gameInfo[filenum].bClone) // if nothing, and it's a clone, and it's allowed, try the parent
		{
			int g = driver_list::clone(*drv);
			if (g != -1)
			{
				drv = &driver_list::driver(g);
				first = "$info="s + drv->name;
				dat_filename = load_datafile_text(fp, first, filenum, m_gameInfo[filenum].descriptor);
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
		std::string dat_content = load_datafile_text(fp, first, filenum, m_sourceInfo[filenum].descriptor);
		if (!dat_content.empty())
			buffer += std::string(m_sourceInfo[filenum].header) + source + "\n" + dat_content + "\n\n\n";

		fp.close();
	}

	return buffer;
}


// General hardware information
std::string load_driver_geninfo(const game_driver *drv, int drvindex)
{
	machine_config config(*drv, emu_opts.GetGlobalOpts());
	const game_driver *parent = nullptr;
	char name[512];
	bool is_bios = false;
	std::string buffer = "\n**** :GENERAL MACHINE INFO: ****\n\n";

	// List the game info 'flags'
	uint64_t cache = GetDriverCacheLower(drvindex);
	if (extract_bit(cache, 32)) // NOT_WORKING
		buffer += "This game doesn't work properly\n"s;

	if (extract_bit(cache, 39)) // UNEMULATED_PROTECTION
		buffer += "This game has protection which isn't fully emulated.\n"s;

	if (extract_bit(cache, 42)) // IMPERFECT_GRAPHICS
		buffer += "The video emulation isn't 100% accurate.\n"s;

	if (extract_bit(cache, 34)) // WRONG_COLORS
		buffer += "The colors are completely wrong.\n"s;

	if (extract_bit(cache, 40)) // IMPERFECT_COLOR
		buffer += "The colors aren't 100% accurate.\n"s;

	if (extract_bit(cache, 35)) // NO_SOUND
		buffer += "This game lacks sound.\n"s;

	if (extract_bit(cache, 43)) // IMPERFECT_SOUND
		buffer += "The sound emulation isn't 100% accurate.\n"s;

	if (extract_bit(cache, 33)) // SAVE_SUPPORTED
		buffer += "Save state supported.\n"s;
	else
		buffer += "Save state not supported.\n"s;

	if (extract_bit(cache, 10)) // MECHANICAL
		buffer += "This game contains mechanical parts.\n"s;

	if (extract_bit(cache, 11)) // IS_INCOMPLETE
		buffer += "This game was never completed.\n"s;

	if (extract_bit(cache, 9)) // NO_SOUND_HW
		buffer += "This game has no sound hardware.\n"s;

	buffer += "\n"s;

	if (drv->flags & MACHINE_IS_BIOS_ROOT)
		is_bios = true;

	// GAME INFORMATIONS
	snprintf(name, std::size(name), "\nGAME: %s\n", drv->name);
	buffer += name;
	snprintf(name, std::size(name), "%s", drv->type.fullname());
	buffer += name;
	snprintf(name, std::size(name), " (%s %s)\n\nCPU:\n", drv->manufacturer, drv->year);
	buffer += name;
	// iterate over CPUs
	execute_interface_enumerator cpuiter(config.root_device());
	std::unordered_set<std::string> exectags;

	for (device_execute_interface &exec : cpuiter)
	{
		if (!exectags.insert(exec.device().tag()).second)
			continue;

		int count = 1;
		int clock = exec.device().clock();
		const char *cpu_name = exec.device().name();

		for (device_execute_interface &scan : cpuiter)
			if (exec.device().type() == scan.device().type() && !mui_strcmp(cpu_name, scan.device().name()) && clock == scan.device().clock())
				if (exectags.insert(scan.device().tag()).second)
					count++;

		if (count > 1)
		{
			snprintf(name, std::size(name), "%d x ", count);
			buffer += name;
		}

		if (clock >= 1000000)
			snprintf(name, std::size(name), "%s %d.%06d MHz\n", cpu_name, clock / 1000000, clock % 1000000);
		else
			snprintf(name, std::size(name), "%s %d.%03d kHz\n", cpu_name, clock / 1000, clock % 1000);

		buffer += name;
	}

	buffer += "\nSOUND:\n"s;
	int has_sound = 0;
	// iterate over sound chips
	sound_interface_enumerator sounditer(config.root_device());
	std::unordered_set<std::string> soundtags;

	for (device_sound_interface &sound : sounditer)
	{
		if (!soundtags.insert(sound.device().tag()).second)
			continue;

		has_sound = 1;
		int count = 1;
		int clock = sound.device().clock();
		const char *sound_name = sound.device().name();

		for (device_sound_interface &scan : sounditer)
			if (sound.device().type() == scan.device().type() && !mui_strcmp(sound_name, scan.device().name()) && clock == scan.device().clock())
				if (soundtags.insert(scan.device().tag()).second)
					count++;

		if (count > 1)
		{
			snprintf(name, std::size(name), "%d x ", count);
			buffer += name;
		}

		buffer += sound_name;

		if (clock)
		{
			if (clock >= 1000000)
				snprintf(name, std::size(name), " %d.%06d MHz", clock / 1000000, clock % 1000000);
			else
				snprintf(name, std::size(name), " %d.%03d kHz", clock / 1000, clock % 1000);

			buffer += name;
		}

		buffer += "\n"s;
	}

	if (has_sound)
	{
		speaker_device_enumerator audioiter(config.root_device());
		int channels = audioiter.count();

		if(channels == 1)
			snprintf(name, std::size(name), "%d Channel\n", channels);
		else
			snprintf(name, std::size(name), "%d Channels\n", channels);

		buffer += name;
	}

	buffer += "\nVIDEO:\n"s;
	screen_device_enumerator screeniter(config.root_device());
	int scrcount = screeniter.count();

	if (scrcount == 0)
		buffer += "Screenless"s;
	else
	{
		for (screen_device &screen : screeniter)
		{
			if (screen.screen_type() == SCREEN_TYPE_VECTOR)
				buffer += "Vector"s;
			else
			{
				const rectangle &visarea = screen.visible_area();

				if (drv->flags & ORIENTATION_SWAP_XY)
					snprintf(name, std::size(name), "%d x %d (V) %f Hz", visarea.width(), visarea.height(), ATTOSECONDS_TO_HZ(screen.refresh_attoseconds()));
				else
					snprintf(name, std::size(name), "%d x %d (H) %f Hz", visarea.width(), visarea.height(), ATTOSECONDS_TO_HZ(screen.refresh_attoseconds()));

				buffer += name;
			}

			buffer += "\n"s;
		}
	}

	buffer += "\nROM REGION:\n"s;
	int g = driver_list::clone(*drv);

	if (g != -1)
		parent = &driver_list::driver(g);

	for (device_t &device : device_enumerator(config.root_device()))
	{
		for (const rom_entry *region = rom_first_region(device); region; region = rom_next_region(region))
		{
			for (const rom_entry *rom = rom_first_file(region); rom; rom = rom_next_file(rom))
			{
				util::hash_collection hashes(rom->hashdata());

				if (g != -1)
				{
					machine_config pconfig(*parent, emu_opts.GetGlobalOpts());

					for (device_t &device : device_enumerator(pconfig.root_device()))
					{
						for (const rom_entry *pregion = rom_first_region(device); pregion; pregion = rom_next_region(pregion))
						{
							for (const rom_entry *prom = rom_first_file(pregion); prom; prom = rom_next_file(prom))
							{
								util::hash_collection phashes(prom->hashdata());

								if (hashes == phashes)
									break;
							}
						}
					}
				}

				snprintf(name, std::size(name), "%-16s \t", ROM_GETNAME(rom));
				buffer += name;
				snprintf(name, std::size(name), "%09d \t", rom_file_size(rom));
				buffer += name;
				snprintf(name, std::size(name), "%-10s", region->name().c_str());
				buffer += name + "\n"s;
			}
		}
	}

	for (samples_device &device : samples_device_enumerator(config.root_device()))
	{
		samples_iterator sampiter(device);

		if (sampiter.altbasename())
		{
			snprintf(name, std::size(name), "\nSAMPLES (%s):\n", sampiter.altbasename());
			buffer += name;
		}

		std::unordered_set<std::string> already_printed;

		for (const char *samplename = sampiter.first(); samplename; samplename = sampiter.next())
		{
			// filter out duplicates
			if (!already_printed.insert(samplename).second)
				continue;

			// output the sample name
			snprintf(name, std::size(name), "%s.wav\n", samplename);
			buffer += name;
		}
	}

	if (!is_bios)
	{
		int g = driver_list::clone(*drv);

		if (g != -1)
			drv = &driver_list::driver(g);

		buffer += "\nORIGINAL:\n"s;
		buffer += drv->type.fullname();
		buffer += "\n\nCLONES:\n"s;

		for (size_t i = 0; i < driver_list::total(); i++)
		{
			if (!mui_strcmp (drv->name, driver_list::driver(i).parent))
			{
				buffer += driver_list::driver(i).type.fullname() + "\n"s;
			}
		}
	}

	std::string temp = std::string(core_filename_extract_base(drv->type.source(), false));
	std::unique_ptr<char[]> source_file(new char[temp.size() + 1]);
	char tmp[2048];
	(void)mui_strcpy(source_file.get(), temp.c_str());
	snprintf(tmp, std::size(tmp), "\nGENERAL SOURCE INFO: %s\n", temp.c_str());
	buffer += std::string(tmp) + "\nGAMES SUPPORTED:\n"s;

	for (size_t i = 0; i < driver_list::total(); i++)
	{
		std::string t1 = driver_list::driver(i).type.source();
		size_t j = t1.find_last_of("/");
		t1.erase(0, j+1);
		if (!mui_strcmp(source_file.get(), t1.c_str()) && !(DriverIsBios(i)))
			buffer += driver_list::driver(i).type.fullname() + "\n"s;
	}

	return buffer;
}

// This is check that the tables are at least as big as they should be
bool validate_datfiles(void)
{
	bool result = true;
	if (std::size(m_gameInfo) < MAX_HFILES)
	{
		std::cout << "m_gameInfo needs to have at least MAX_HFILES members" << std::endl;
		result = false;
	}

	if (std::size(m_sourceInfo) < MAX_HFILES)
	{
		std::cout << "m_sourceInfo needs to have at least MAX_HFILES members" << std::endl;
		result = false;
	}

	if (std::size(m_swInfo) < MAX_HFILES)
	{
		std::cout << "m_swInfo needs to have at least MAX_HFILES members" << std::endl;
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
