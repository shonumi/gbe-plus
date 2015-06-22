// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : config.cpp
// Date : September 20, 2014
// Description : GBE+ configuration options
//
// Parses command-line arguments to configure GBE options

#include <iostream>
#include <fstream>

#include "config.h"

namespace config
{
	std::string rom_file = "";
	std::string bios_file = "";
	std::string save_file = "";
	std::string dmg_bios_path = "";
	std::string gbc_bios_path = "";
	std::string agb_bios_path = "";
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
	int agb_joy_a = 100; int agb_joy_b = 101; int agb_joy_start = 107; int agb_joy_select = 106;
	int agb_joy_r_trigger = 105; int agb_joy_l_trigger = 104;
	int agb_joy_left = 200; int agb_joy_right = 201; int agb_joy_up = 202; int agb_joy_down = 203;

	//Default keyboard bindings
	//Arrow Z = A button, X = B button, START = Return, Select = Space
	//UP, LEFT, DOWN, RIGHT = Arrow keys
	int dmg_key_a = 122; int dmg_key_b = 120; int dmg_key_start = 13; int dmg_key_select = 32; 
	int dmg_key_left = 276; int dmg_key_right = 275; int dmg_key_down = 274; int dmg_key_up = 273;

	//Default joystick bindings
	int dmg_joy_a = 100; int dmg_joy_b = 101; int dmg_joy_start = 107; int dmg_joy_select = 106;
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

	bool sdl_render = true;

	void (*render_external)(std::vector<u32>&);

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
		std::cout<<"GBE::Error - No ROM file in arguments \n";
		return false;
	}

	else 
	{
		//Parse the rest of the arguments if any		
		for(int x = 1; x < config::cli_args.size(); x++)
		{	
			//Run GBE+ in debug mode
			if((config::cli_args[x] == "-d") || (config::cli_args[x] == "--debug")) { config::use_debugger = true; }

			//Load GBA BIOS
			else if((config::cli_args[x] == "-b") || (config::cli_args[x] == "--bios")) 
			{
				if((++x) == config::cli_args.size()) { std::cout<<"GBE::Error - No BIOS file in arguments\n"; }

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
				std::cout<<"GBE::Error - Unknown argument - " << config::cli_args[x] << "\n";
				return false;
			}
		}

		return true;
	}
}

/****** Parse ROM filename and save file ******/
void parse_filenames()
{
	//ROM file is always first argument
	config::rom_file = config::cli_args[0];
	config::save_file = config::rom_file + ".sav";

	//Determine Gameboy type based on file name
	//Note, DMG and GBC games are automatically detected in the Gameboy MMU, so only check for GBA types here
	std::size_t dot = config::rom_file.find_last_of(".");
	std::string ext = config::rom_file.substr(dot);

	if(ext == ".gba") { config::gb_type = 3; }
} 

/****** Parse optins from the .ini file ******/
bool parse_ini_file()
{
	std::ifstream file("gbe.ini", std::ios::in); 
	std::string input_line = "";
	std::string line_char = "";

	//Clear existing .ini parameters
	std::vector <std::string> ini_opts;
	ini_opts.clear();

	if(!file.is_open())
	{
		std::cout<<"GBE::Error - Could not open gbe.ini configuration file. Check file path or permissions. \n";
		return false; 
	}

	//Cycle through whole file, line-by-line
	while(getline(file, input_line))
	{
		line_char = input_line[0];
		bool ignore = false;	
	
		//Check if line starts with [ - if not, skip line
		if(line_char == "[")
		{
			std::string line_item = "";

			//Cycle through line, character-by-character
			for(int x = 0; ++x < input_line.length();)
			{
				line_char = input_line[x];

				//Check for single-quotes, don't parse ":" or "]" within them
				if((line_char == "'") && (!ignore)) { ignore = true; }
				else if((line_char == "'") && (ignore)) { ignore = false; }

				//Check the character for item limiter : or ] - Push to Vector
				else if(((line_char == ":") || (line_char == "]")) && (!ignore)) 
				{
					ini_opts.push_back(line_item);
					line_item = ""; 
				}

				else { line_item += line_char; }
			}
		}
	}
	
	file.close();

	//Cycle through all items in the .ini file
	//Set options as appropiate

	int size = ini_opts.size();
	int output = 0;
	std::string ini_item = "";

	for(int x = 0; x < size; x++)
	{
		ini_item = ini_opts[x];

		//Use BIOS
		if(ini_item == "#use_bios")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::stringstream temp_stream(ini_item);
				temp_stream >> output;

				if(output == 1) { config::use_bios = true; }
				else { config::use_bios = false; }
			}

			else 
			{ 
				std::cout<<"GBE::Error - Could not parse gbe.ini (#use_bios) \n";
				return false;
			}
		}

		//DMG BIOS path
		else if(ini_item == "#dmg_bios_path")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { config::dmg_bios_path = ini_item; }
				else { config::dmg_bios_path = ""; x--;}
 
			}

			else { config::dmg_bios_path = ""; }
		}

		//GBC BIOS path
		else if(ini_item == "#gbc_bios_path")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { config::gbc_bios_path = ini_item; }
				else { config::gbc_bios_path = ""; x--;}
 
			}

			else { config::gbc_bios_path = ""; }
		}

		//GBA BIOS path
		else if(ini_item == "#agb_bios_path")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { config::agb_bios_path = ini_item; }
				else { config::agb_bios_path = ""; x--;}
 
			}

			else { config::agb_bios_path = ""; }
		}

		//Use OpenGL
		else if(ini_item == "#use_opengl")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::stringstream temp_stream(ini_item);
				temp_stream >> output;

				if(output == 1) { config::use_opengl = true; }
				else { config::use_opengl = false; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#use_opengl) \n";
				return false;
			}
		}

		//Use gamepad dead zone
		else if(ini_item == "#dead_zone")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::stringstream temp_stream(ini_item);
				temp_stream >> output;

				if((output >= 0) && (output <= 32767)) { config::dead_zone = output; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#dead_zone) \n";
				return false;
			}
		}

		//Scaling factor
		else if(ini_item == "#scaling_factor")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::stringstream temp_stream(ini_item);
				temp_stream >> output;

				if((output >= 1) && (output <= 10)) { config::scaling_factor = output; }
				else { config::scaling_factor = 1; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#scaling_factor) \n";
				return false;
			}
		}


		//DMG-GBC keyboard controls
		else if(ini_item == "#dmg_key_controls")
		{
			if((x + 8) < size)
			{
				std::stringstream temp_stream;

				//A
				temp_stream << ini_opts[++x];
				temp_stream >> config::dmg_key_a;
				temp_stream.clear();
				temp_stream.str(std::string());

				//B
				temp_stream << ini_opts[++x];
				temp_stream >> config::dmg_key_b;
				temp_stream.clear();
				temp_stream.str(std::string());

				//START
				temp_stream << ini_opts[++x];
				temp_stream >> config::dmg_key_start;
				temp_stream.clear();
				temp_stream.str(std::string());

				//SELECT
				temp_stream << ini_opts[++x];
				temp_stream >> config::dmg_key_select;
				temp_stream.clear();
				temp_stream.str(std::string());

				//LEFT
				temp_stream << ini_opts[++x];
				temp_stream >> config::dmg_key_left;
				temp_stream.clear();
				temp_stream.str(std::string());

				//RIGHT
				temp_stream << ini_opts[++x];
				temp_stream >> config::dmg_key_right;
				temp_stream.clear();
				temp_stream.str(std::string());

				//UP
				temp_stream << ini_opts[++x];
				temp_stream >> config::dmg_key_up;
				temp_stream.clear();
				temp_stream.str(std::string());

				//DOWN
				temp_stream << ini_opts[++x];
				temp_stream >> config::dmg_key_down;
				temp_stream.clear();
				temp_stream.str(std::string());
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#dmg_key_controls) \n";
				return false;
			}
		}

		//DMG-GBC gamepad controls
		else if(ini_item == "#dmg_joy_controls")
		{
			if((x + 8) < size)
			{
				std::stringstream temp_stream;

				//A
				temp_stream << ini_opts[++x];
				temp_stream >> config::dmg_joy_a;
				temp_stream.clear();
				temp_stream.str(std::string());

				//B
				temp_stream << ini_opts[++x];
				temp_stream >> config::dmg_joy_b;
				temp_stream.clear();
				temp_stream.str(std::string());

				//START
				temp_stream << ini_opts[++x];
				temp_stream >> config::dmg_joy_start;
				temp_stream.clear();
				temp_stream.str(std::string());

				//SELECT
				temp_stream << ini_opts[++x];
				temp_stream >> config::dmg_joy_select;
				temp_stream.clear();
				temp_stream.str(std::string());

				//LEFT
				temp_stream << ini_opts[++x];
				temp_stream >> config::dmg_joy_left;
				temp_stream.clear();
				temp_stream.str(std::string());

				//RIGHT
				temp_stream << ini_opts[++x];
				temp_stream >> config::dmg_joy_right;
				temp_stream.clear();
				temp_stream.str(std::string());

				//UP
				temp_stream << ini_opts[++x];
				temp_stream >> config::dmg_joy_up;
				temp_stream.clear();
				temp_stream.str(std::string());

				//DOWN
				temp_stream << ini_opts[++x];
				temp_stream >> config::dmg_joy_down;
				temp_stream.clear();
				temp_stream.str(std::string());
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#dmg_joy_controls) \n";
				return false;
			}
		}

		//GBA keyboard controls
		else if(ini_item == "#agb_key_controls")
		{
			if((x + 10) < size)
			{
				std::stringstream temp_stream;

				//A
				temp_stream << ini_opts[++x];
				temp_stream >> config::agb_key_a;
				temp_stream.clear();
				temp_stream.str(std::string());

				//B
				temp_stream << ini_opts[++x];
				temp_stream >> config::agb_key_b;
				temp_stream.clear();
				temp_stream.str(std::string());

				//START
				temp_stream << ini_opts[++x];
				temp_stream >> config::agb_key_start;
				temp_stream.clear();
				temp_stream.str(std::string());

				//SELECT
				temp_stream << ini_opts[++x];
				temp_stream >> config::agb_key_select;
				temp_stream.clear();
				temp_stream.str(std::string());

				//LEFT
				temp_stream << ini_opts[++x];
				temp_stream >> config::agb_key_left;
				temp_stream.clear();
				temp_stream.str(std::string());

				//RIGHT
				temp_stream << ini_opts[++x];
				temp_stream >> config::agb_key_right;
				temp_stream.clear();
				temp_stream.str(std::string());

				//UP
				temp_stream << ini_opts[++x];
				temp_stream >> config::agb_key_up;
				temp_stream.clear();
				temp_stream.str(std::string());

				//DOWN
				temp_stream << ini_opts[++x];
				temp_stream >> config::agb_key_down;
				temp_stream.clear();
				temp_stream.str(std::string());

				//LEFT TRIGGER
				temp_stream << ini_opts[++x];
				temp_stream >> config::agb_key_l_trigger;
				temp_stream.clear();
				temp_stream.str(std::string());

				//RIGHT TRIGGER
				temp_stream << ini_opts[++x];
				temp_stream >> config::agb_key_r_trigger;
				temp_stream.clear();
				temp_stream.str(std::string());
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#agb_key_controls) \n";
				return false;
			}
		}

		//GBA gamepad controls
		else if(ini_item == "#agb_joy_controls")
		{
			if((x + 10) < size)
			{
				std::stringstream temp_stream;

				//A
				temp_stream << ini_opts[++x];
				temp_stream >> config::agb_joy_a;
				temp_stream.clear();
				temp_stream.str(std::string());

				//B
				temp_stream << ini_opts[++x];
				temp_stream >> config::agb_joy_b;
				temp_stream.clear();
				temp_stream.str(std::string());

				//START
				temp_stream << ini_opts[++x];
				temp_stream >> config::agb_joy_start;
				temp_stream.clear();
				temp_stream.str(std::string());

				//SELECT
				temp_stream << ini_opts[++x];
				temp_stream >> config::agb_joy_select;
				temp_stream.clear();
				temp_stream.str(std::string());

				//LEFT
				temp_stream << ini_opts[++x];
				temp_stream >> config::agb_joy_left;
				temp_stream.clear();
				temp_stream.str(std::string());

				//RIGHT
				temp_stream << ini_opts[++x];
				temp_stream >> config::agb_joy_right;
				temp_stream.clear();
				temp_stream.str(std::string());

				//UP
				temp_stream << ini_opts[++x];
				temp_stream >> config::agb_joy_up;
				temp_stream.clear();
				temp_stream.str(std::string());

				//DOWN
				temp_stream << ini_opts[++x];
				temp_stream >> config::agb_joy_down;
				temp_stream.clear();
				temp_stream.str(std::string());

				//LEFT TRIGGER
				temp_stream << ini_opts[++x];
				temp_stream >> config::agb_joy_l_trigger;
				temp_stream.clear();
				temp_stream.str(std::string());

				//RIGHT TRIGGER
				temp_stream << ini_opts[++x];
				temp_stream >> config::agb_joy_r_trigger;
				temp_stream.clear();
				temp_stream.str(std::string());
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#agb_joy_controls) \n";
				return false;
			}
		}
	}

	return true;
}
