// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : config.cpp
// Date : September 20, 2014
// Description : GBE+ configuration options
//
// Parses command-line arguments to configure GBE options

#include <iostream>

#include "config.h"

namespace config
{
	std::string rom_file = "";
	std::string bios_file = "";
	std::string save_file = "";
	std::vector <std::string> cli_args;
	bool use_debugger = false;

	//Default keyboard bindings - GBA
	//Arrow Z = A button, X = B button, START = Return, Select = Space
	//UP, LEFT, DOWN, RIGHT = Arrow keys
	//A key = Left Shoulder, S key = Right Shoulder
	int agb_key_a = 122; int agb_key_b = 120; int agb_key_start = 13; int agb_key_select = 32;
	int agb_key_r_trigger = 115; int agb_key_l_trigger = 97;
	int agb_key_left = 276; int agb_key_right = 275; int agb_key_down = 274; int agb_key_up = 273;

	//Default joystick bindings - GBA
	int agb_joy_a = 100; int agb_joy_b = 102; int agb_joy_start = 107; int agb_joy_select = 106;
	int agb_joy_r_trigger = 105; int agb_joy_l_trigger = 104;
	int agb_joy_left = 200; int agb_joy_right = 201; int agb_joy_up = 202; int agb_joy_down = 203;

	//Default keyboard bindings
	//Arrow Z = A button, X = B button, START = Return, Select = Space
	//UP, LEFT, DOWN, RIGHT = Arrow keys
	int dmg_key_a = 122; int dmg_key_b = 120; int dmg_key_start = 13; int dmg_key_select = 32; 
	int dmg_key_left = 276; int dmg_key_right = 275; int dmg_key_down = 274; int dmg_key_up = 273;

	//Default joystick bindings
	int dmg_joy_a = 101; int dmg_joy_b = 100; int dmg_joy_start = 109; int dmg_joy_select = 108;
	int dmg_joy_left = 200; int dmg_joy_right = 201; int dmg_joy_up = 202; int dmg_joy_down = 203;

	//Default joystick dead-zone
	int dead_zone = 16000;

	u32 flags = 0;
	bool pause_emu = false;
	bool use_bios = false;
	bool use_opengl = false;
	bool turbo = false;

	u8 scaling_factor = 1;

	std::stringstream title;

	//Emulated Gameboy type
	//TODO - Make this an enum
	//0 - DMG, 1 - DMG on GBC, 2 - GBC, 3 - GBA, 4 - NDS????
	u8 gb_type = 0;

	//Default Gameboy BG palettes
	u32 DMG_BG_PAL[4];
	u32 DMG_OBJ_PAL[4][2];
}

/****** Parse arguments passed from the command-line ******/
bool parse_cli_args()
{
	config::DMG_BG_PAL[0] = config::DMG_OBJ_PAL[0][0] = config::DMG_OBJ_PAL[0][1] = 0xFFFFFFFF;
	config::DMG_BG_PAL[1] = config::DMG_OBJ_PAL[1][0] = config::DMG_OBJ_PAL[1][1] = 0xFFC0C0C0;
	config::DMG_BG_PAL[2] = config::DMG_OBJ_PAL[2][0] = config::DMG_OBJ_PAL[2][1] = 0xFF606060;
	config::DMG_BG_PAL[3] = config::DMG_OBJ_PAL[3][0] = config::DMG_OBJ_PAL[3][1] = 0xFF000000;

	//If no arguments were passed, cannot run without ROM file
	if(config::cli_args.size() < 1) 
	{
		std::cout<<"Error::No ROM file in arguments \n";
		return false;
	}

	else 
	{
		//ROM file is always first argument
		config::rom_file = config::cli_args[0];
		config::save_file = config::rom_file + ".sav";

		//Determine Gameboy type based on file name
		//Note, DMG and GBC games are automatically detected in the Gameboy MMU, so only check for GBA types here
		std::size_t dot = config::rom_file.find_last_of(".");
		std::string ext = config::rom_file.substr(dot);

		if(ext == ".gba") { config::gb_type = 3; } 

		//Parse the rest of the arguments if any		
		for(int x = 1; x < config::cli_args.size(); x++)
		{	
			//Run GBE+ in debug mode
			if((config::cli_args[x] == "-d") || (config::cli_args[x] == "--debug")) { config::use_debugger = true; }

			//Load GBA BIOS
			else if((config::cli_args[x] == "-b") || (config::cli_args[x] == "--bios")) 
			{
				if((++x) == config::cli_args.size()) { std::cout<<"Error::No BIOS file in arguments\n"; }

				else 
				{ 
					config::use_bios = true; 
					config::bios_file = config::cli_args[x];
				}
			}

			//Use OpenGL for screen drawing
			else if(config::cli_args[x] == "--opengl") { config::use_opengl = true; }

			//Scale screen by 2x
			else if(config::cli_args[x] == "--2x") { config::scaling_factor = 2; }

			//Scale screen by 3x
			else if(config::cli_args[x] == "--3x") { config::scaling_factor = 3; }

			//Scale screen by 4x
			else if(config::cli_args[x] == "--4x") { config::scaling_factor = 4; }

			//Scale screen by 5x
			else if(config::cli_args[x] == "--5x") { config::scaling_factor = 5; }

			//Scale screen by 6x
			else if(config::cli_args[x] == "--6x") { config::scaling_factor = 6; }

			else
			{
				std::cout<<"Error::Unknown argument - " << config::cli_args[x] << "\n";
				return false;
			}
		}

		return true;
	}
} 
