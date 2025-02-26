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
 *             the index numbers line up. Anything with NULL indicates an
 *             unsupported option (the file doesn't contain the info).
 *      - Each table must contain at least MAX_HFILES members (extra lines
 *             are ignored)
 *      - Software comes first, followed by Game then Source.
***************************************************************************/
// license:BSD-3-Clause
// copyright-holders:Chris Kirmse, Mike Haaland, René Single, Mamesick, Robbbert

// standard C++ headers
#include <string>
#include <fstream>
#include <iostream>

// standard windows headers

// MAME headers
#include "mameheaders.h"

// MAMEUI headers
#include "mui_util.h"
#include "mui_opts.h"
#include "emu_opts.h"
#include "mui_str.h"

#include "history.h"

using namespace std::literals;

/****************************************************************************
 *      struct definitions
 ****************************************************************************/
typedef struct
{
	LPCSTR   filename;
	LPCSTR   header;
	LPCSTR   descriptor;
	bool     bClone;    // if nothing found for a clone, try the parent
}
HGAMEINFO;

typedef struct
{
	LPCSTR   filename;
	LPCSTR   header;
	LPCSTR   descriptor;
}
HSOURCEINFO;

/*************************** START CONFIGURABLE AREA *******************************/
// number of dats we support
#define MAX_HFILES 8
// The order of these is the order they are displayed
const HGAMEINFO m_gameInfo[MAX_HFILES] =
{
	{ "history.dat",  "\n**** :HISTORY: ****\n\n",          "$bio",   1 },
	{ "sysinfo.dat",  "\n**** :SYSINFO: ****\n\n",          "$bio",   1 },
	{ "messinfo.dat", "\n**** :MESSINFO: ****\n\n",         "$mame",  1 },
	{ "mameinfo.dat", "\n**** :MAMEINFO: ****\n\n",         "$mame",  1 },
	{ "gameinit.dat", "\n**** :GAMEINIT: ****\n\n",         "$mame",  1 },
	{ "command.dat",  "\n**** :COMMANDS: ****\n\n",         "$cmd",   1 },
	{ "story.dat",    "\n**** :HIGH SCORES: ****\n\n",      "$story", 0 },
	{ "marp.dat",     "\n**** :MARP HIGH SCORES: ****\n\n", "$marp",  0 },
};

const HSOURCEINFO m_sourceInfo[MAX_HFILES] =
{
	{ NULL },
	{ NULL },
	{ "messinfo.dat", "\n***:MESSINFO DRIVER: ",  "$drv" },
	{ "mameinfo.dat", "\n***:MAMEINFO DRIVER: ",  "$drv" },
	{ NULL },
	{ NULL },
	{ NULL },
	{ NULL },
};

const HSOURCEINFO m_swInfo[MAX_HFILES] =
{
	{ "history.dat",  "\n**** :HISTORY item: ",     "$bio" },
	{ NULL },
	{ NULL },
	{ NULL },
	{ NULL },
	{ NULL },
	{ NULL },
	{ NULL },
};

/*************************** END CONFIGURABLE AREA *******************************/

int file_sizes[MAX_HFILES] = { 0, };
std::map<std::string, std::streampos> mymap[MAX_HFILES];

static bool create_index(std::ifstream &fp, int filenum)
{
	if (!fp.good())
		return false;
	// get file size
	fp.seekg(0, std::ios::end);
	size_t file_size = fp.tellg();
	// same file as before?
	if (file_size == file_sizes[filenum])
		return true;
	// new file, it needs to be indexed
	mymap[filenum].clear();
	file_sizes[filenum] = file_size;
	fp.seekg(0);
	std::string file_line, first, second;
	std::getline(fp, file_line);
	int position = file_line.size() + 2; // tellg is buggy, this works and is faster
	while (fp.good())
	{
		char t1 = file_line[0];
		if ((std::count(file_line.begin(),file_line.end(),'=') == 1) && (t1 == '$')) // line must start with $ and contain one =
		{
			std::string first, second;
			// now start by removing all spaces
			file_line.erase(remove_if(file_line.begin(), file_line.end(), ::isspace), file_line.end());
			std::unique_ptr<char[]> s(new char [file_line.length()+1]);
			(void)mui_strcpy(s.get(), file_line.c_str());

			first = mui_strtok(s.get(), "=");  // get first part of key
			second = mui_strtok("", ",");    // get second part
			while (!second.empty())
			{
				// store into index
				mymap[filenum][first + "="s + second] = position;
				second = mui_strtok("", ",");
			}
		}
		std::getline(fp, file_line);
		position += (file_line.size() + 2);
	}
	// check contents
//  if (filenum == 0)
//  for (auto const &it : mymap[filenum])
//      printf("%s = %X\n", it.first.c_str(), int(it.second));
	return true;
}

static std::string load_datafile_text(std::ifstream &fp, std::string keycode, int filenum, const char *tag)
{
	std::string readbuf;

	auto search = mymap[filenum].find(keycode);
	if (search != mymap[filenum].end())
	{
		std::streampos offset = mymap[filenum].find(keycode)->second;
		fp.seekg(offset);
		std::string file_line;

		/* read text until buffer is full or end of entry is encountered */
		while (std::getline(fp, file_line))
		{
			//printf("%s\n",file_line.c_str());
			if (file_line.find("$end")==0)
				break;

			if (file_line.find(tag)==0)
				continue;

			readbuf += file_line + "\n"s;
		}
	}

	return readbuf;
}

std::string load_swinfo(const game_driver *drv, const char* datsdir, std::string software, int filenum)
{
	std::string buffer;
	// if it's a NULL record exit now
	if (!m_swInfo[filenum].filename)
		return buffer;

	// datafile name
	std::string buf, filename = datsdir + "\\"s + m_swInfo[filenum].filename;
	std::ifstream fp (filename);

	/* try to open datafile */
	if (create_index(fp, filenum))
	{
		size_t i = software.find(":");
		std::string ssys = software.substr(0, i);
		std::string ssoft = software.substr(i+1);
		std::string first = "$"s + ssys + std::string("=") + ssoft;
		// get info on software
		buf = load_datafile_text(fp, first, filenum, m_swInfo[filenum].descriptor);

		if (!buf.empty())
			buffer += std::string(m_swInfo[filenum].header) + ssoft + "\n"s + buf + "\n\n\n"s;

		fp.close();
	}

	return buffer;
}

std::string load_gameinfo(const game_driver *drv, const char* datsdir, int filenum)
{
	std::string buffer;
	// if it's a NULL record exit now
	if (!m_gameInfo[filenum].filename)
		return buffer;

	// datafile name
	std::string buf, filename = datsdir + "\\"s + m_gameInfo[filenum].filename;
	std::ifstream fp (filename);

	/* try to open datafile */
	if (create_index(fp, filenum))
	{
		std::string first = "$info="s + drv->name;
		// get info on game
		buf = load_datafile_text(fp, first, filenum, m_gameInfo[filenum].descriptor);

		// if nothing, and it's a clone, and it's allowed, try the parent
		if (buf.empty() && m_gameInfo[filenum].bClone)
		{
			int g = driver_list::clone(*drv);
			if (g != -1)
			{
				drv = &driver_list::driver(g);
				first = "$info="s + drv->name;
				buf = load_datafile_text(fp, first, filenum, m_gameInfo[filenum].descriptor);
			}
		}

		if (!buf.empty())
			buffer += std::string(m_gameInfo[filenum].header) + buf + "\n\n\n"s;

		fp.close();
	}

	return buffer;
}

std::string load_sourceinfo(const game_driver *drv, const char* datsdir, int filenum)
{
	std::string buffer;
	// if it's a NULL record exit now
	if (!m_sourceInfo[filenum].filename)
		return buffer;

	// datafile name
	std::string buf, filename = datsdir + "\\"s + m_sourceInfo[filenum].filename;
	std::ifstream fp (filename);

	std::string source = drv->type.source();
	size_t i = source.find_last_of("/");
	source.erase(0,i+1);

	if (create_index(fp, filenum))
	{
		std::string first = "$info="s + source;
		// get info on game driver source
		buf = load_datafile_text(fp, first, filenum, m_sourceInfo[filenum].descriptor);

		if (!buf.empty())
			buffer += std::string(m_sourceInfo[filenum].header) + source + "\n"s + buf + "\n\n\n"s;

		fp.close();
	}

	return buffer;
}


// General hardware information
std::string load_driver_geninfo(const game_driver *drv, int drvindex)
{
	machine_config config(*drv, MameUIGlobal());
	const game_driver *parent = NULL;
	char name[512];
	bool is_bios = false;
	std::string buffer = "\n**** :GENERAL MACHINE INFO: ****\n\n";

	/* List the game info 'flags' */
	uint32_t cache = GetDriverCacheLower(drvindex);
	if (BIT(cache, 6))
		buffer += "This game doesn't work properly\n"s;

	if (BIT(cache, 22))
		buffer += "This game has protection which isn't fully emulated.\n"s;

	if (BIT(cache, 18))
		buffer += "The video emulation isn't 100% accurate.\n"s;

	if (BIT(cache, 21))
		buffer += "The colors are completely wrong.\n"s;

	if (BIT(cache, 20))
		buffer += "The colors aren't 100% accurate.\n"s;

	if (BIT(cache, 17))
		buffer += "This game lacks sound.\n"s;

	if (BIT(cache, 16))
		buffer += "The sound emulation isn't 100% accurate.\n"s;

	if (BIT(cache, 7))
		buffer += "Save state not supported.\n"s;

	if (BIT(cache, 14))
		buffer += "This game contains mechanical parts.\n"s;

	if (BIT(cache, 15))
		buffer += "This game was never completed.\n"s;

	if (BIT(cache, 13))
		buffer += "This game has no sound hardware.\n"s;

	buffer += "\n"s;

	if (drv->flags & MACHINE_IS_BIOS_ROOT)
		is_bios = true;

	/* GAME INFORMATIONS */
	snprintf(name, std::size(name), "\nGAME: %s\n", drv->name);
	buffer += name;
	snprintf(name, std::size(name), "%s", drv->type.fullname());
	buffer += name;
	snprintf(name, std::size(name), " (%s %s)\n\nCPU:\n", drv->manufacturer, drv->year);
	buffer += name;
	/* iterate over CPUs */
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
	/* iterate over sound chips */
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
					machine_config pconfig(*parent, MameUIGlobal());

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


// For all of MAME builds - called by winui.cpp
char * GetGameHistory(int driver_index, std::string software)
{
	std::string fullbuf;
	if (driver_index < 0)
			return ConvertToWindowsNewlines(fullbuf.c_str());

	if (validate_datfiles())
	{
		// Get the path to dat files
		std::string t = dir_get_value(DIRMAP_HISTORY_PATH);
		std::unique_ptr<char[]> buf(new char [t.size()+1]);
		(void)mui_strcpy(buf.get(), t.c_str());
		// only want first path
		const char* datsdir = strtok(buf.get(), ";");
		// validate software
		bool sw_valid = false;
		if (!software.empty())
		{
			size_t i = software.find(':');
			sw_valid = (i != std::string::npos) ? true : false;
		}

		if (datsdir && osd::directory::open(datsdir))
		{
			for (size_t filenum = 0; filenum < MAX_HFILES; filenum++)
			{
				if (sw_valid)
					fullbuf.append(load_swinfo(&driver_list::driver(driver_index), datsdir, software, filenum));
				fullbuf.append(load_gameinfo(&driver_list::driver(driver_index), datsdir, filenum));
				fullbuf.append(load_sourceinfo(&driver_list::driver(driver_index), datsdir, filenum));
			}
		}
		else
			fullbuf = "\nThe path to your dat files is invalid.\n\n\n"s;
	}
	else
		fullbuf = "\nUnable to display info due to an internal error.\n\n\n"s;

	fullbuf.append(load_driver_geninfo(&driver_list::driver(driver_index), driver_index));

	return ConvertToWindowsNewlines(fullbuf.c_str());
}

// For Arcade-only builds
char * GetGameHistory(int driver_index)
{
	std::string fullbuf;
	if (driver_index < 0)
			return ConvertToWindowsNewlines(fullbuf.c_str());

	if (validate_datfiles())
	{
		std::string t = dir_get_value(DIRMAP_HISTORY_PATH);
		std::unique_ptr<char[]> buf(new char[t.size() + 1]);
		(void)mui_strcpy(buf.get(), t.c_str());
		// only want first path
		const char* datsdir = strtok(buf.get(), ";");

		if (datsdir && osd::directory::open(datsdir))
		{
			for (size_t filenum = 0; filenum < MAX_HFILES; filenum++)
			{
				fullbuf.append(load_gameinfo(&driver_list::driver(driver_index), datsdir, filenum));
				fullbuf.append(load_sourceinfo(&driver_list::driver(driver_index), datsdir, filenum));
			}
		}
		else
			fullbuf = "\nThe path to your dat files is invalid.\n\n\n";
	}
	else
		fullbuf = "\nUnable to display info due to an internal error.\n\n\n";

	fullbuf.append(load_driver_geninfo(&driver_list::driver(driver_index), driver_index));

	return ConvertToWindowsNewlines(fullbuf.c_str());
}

