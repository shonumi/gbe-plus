// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : config.h
// Date : September 20, 2014
// Description : GBE+ configuration options
//
// Parses command-line arguments to configure GBE options

#ifndef EMU_CONFIG
#define EMU_CONFIG

#include <vector>
#include <string>
#include <sstream>

#include "common.h"

bool parse_cli_args();

namespace config
{ 
	extern std::string rom_file;
	extern std::string bios_file;
	extern std::string save_file;
	extern std::vector <std::string> cli_args;
	extern bool use_debugger;
	extern int agb_key_a, agb_key_b, agb_key_start, agb_key_select, agb_key_up, agb_key_down, agb_key_left, agb_key_right, agb_key_r_trigger, agb_key_l_trigger;
	extern int agb_joy_a, agb_joy_b, agb_joy_start, agb_joy_select, agb_joy_up, agb_joy_down, agb_joy_left, agb_joy_right, agb_joy_r_trigger, agb_joy_l_trigger;
	extern int dmg_key_a, dmg_key_b, dmg_key_start, dmg_key_select, dmg_key_up, dmg_key_down, dmg_key_left, dmg_key_right;
	extern int dmg_joy_a, dmg_joy_b, dmg_joy_start, dmg_joy_select, dmg_joy_up, dmg_joy_down, dmg_joy_left, dmg_joy_right;
	extern int dead_zone;
	extern u32 flags;
	extern bool pause_emu;
	extern bool use_bios;
	extern bool use_opengl;
	extern bool turbo;
	extern u8 scaling_factor;
	extern std::stringstream title;
	extern u8 gb_type;
	extern u32 DMG_BG_PAL[4];
	extern u32 DMG_OBJ_PAL[4][2];
}

#endif // EMU_CONFIG