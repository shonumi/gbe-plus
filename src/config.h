// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : config.h
// Date : September 20, 2014
// Description : GBE+ configuration options
//
// Parses command-line arguments to configure GBE options

#ifndef GBA_CONFIG
#define GBA_CONFIG

#include <vector>
#include <string>

#include "common.h"

bool parse_cli_args();

namespace config
{ 
	extern std::string rom_file;
	extern std::string bios_file;
	extern std::string save_file;
	extern std::vector <std::string> cli_args;
	extern bool use_debugger;
	extern int key_a, key_b, key_start, key_select, key_up, key_down, key_left, key_right, key_r_trigger, key_l_trigger;
	extern int joy_a, joy_b, joy_start, joy_select, joy_up, joy_down, joy_left, joy_right, joy_r_trigger, joy_l_trigger;
	extern int dead_zone;
	extern u32 flags;
	extern bool pause_emu;
	extern bool use_bios;
	extern bool use_opengl;
	extern bool turbo;
	extern u8 scaling_factor;
}

#endif // GBA_CONFIG