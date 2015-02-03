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

	//Default keyboard bindings
	//Arrow Z = A button, X = B button, START = Return, Select = Space
	//UP, LEFT, DOWN, RIGHT = Arrow keys
	//A key = Left Shoulder, S key = Right Shoulder
	int key_a = 122; int key_b = 120; int key_start = 13; int key_select = 32;
	int key_r_trigger = 115; int key_l_trigger = 97;
	int key_left = 276; int key_right = 275; int key_down = 274; int key_up = 273;

	//Default joystick bindings
	int joy_a = 100; int joy_b = 102; int joy_start = 107; int joy_select = 106;
	int joy_r_trigger = 105; int joy_l_trigger = 104;
	int joy_left = 200; int joy_right = 201; int joy_up = 202; int joy_down = 203;

	//Default joystick dead-zone
	int dead_zone = 16000;

	u32 flags = 0;
	bool pause_emu = false;
	bool use_bios = false;
	bool use_opengl = false;
	bool turbo = false;
}

/****** Parse arguments passed from the command-line ******/
bool parse_cli_args()
{
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

			else
			{
				std::cout<<"Error::Unknown argument - " << config::cli_args[x] << "\n";
				return false;
			}
		}

		return true;
	}
} 
