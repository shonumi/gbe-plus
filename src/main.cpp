// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : main.cpp
// Date : August 16, 2014
// Description : The emulator
//
// This is main. It all begins here ;)

#include "gba/core.h"
#include "dmg/core.h"
#include "sgb/core.h"
#include "nds/core.h"
#include "common/config.h"

#include <SDL2/SDL_main.h>

int main(int argc, char* args[])
{
	std::cout<<"GBE+ 1.4 [SDL]\n";

	core_emu* gbe_plus = NULL;

	//Start SDL from the main thread now, report specific init errors later in the core
	SDL_Init(SDL_INIT_VIDEO);

	//Grab command-line arguments
	for(int x = 0; x++ < argc - 1;) 
	{ 
		std::string temp_arg = args[x]; 
		config::cli_args.push_back(temp_arg);
		parse_filenames();
	}

	//Parse .ini options
	parse_ini_file();

	//Load OSD font
	load_osd_font();

	//Parse cheat file
	if(config::use_cheats) { parse_cheats_file(false); }

	if(config::mute) { config::volume = 0; }

	//Parse command-line arguments
	//These will override .ini options!
	if(!parse_cli_args()) { return 0; }

	//Get emulated system type from file
	config::gb_type = get_system_type_from_file(config::rom_file);

	//GBA core
	if(config::gb_type == 3)
	{
		gbe_plus = new AGB_core();
	}
	
	//DMG-GBC core
	else if((config::gb_type >= 0) && (config::gb_type <= 2))
	{
		gbe_plus = new DMG_core();
	}

	//Super Game Boy (SGB1 and SGB2)
	else if((config::gb_type == 5) || (config::gb_type == 6))
	{
		gbe_plus = new SGB_core();
	}

	//NDS core
	else
	{
		gbe_plus = new NTR_core();
	}
	
	//Read BIOS file optionally
	if(config::use_bios) 
	{
		//If no bios file was passed from the command-line arguments, defer to .ini options
		if(config::bios_file == "")
		{
			switch(config::gb_type)
			{
				case 0x1 : config::bios_file = config::dmg_bios_path; break;
				case 0x2 : config::bios_file = config::gbc_bios_path; break;
				case 0x3 : config::bios_file = config::agb_bios_path; break;
			}
		}

		if(!gbe_plus->read_bios(config::bios_file)) { return 0; } 
	}

	//Read specified ROM file
	if(!gbe_plus->read_file(config::rom_file)) { return 0; }

	//Read firmware optionally (NDS)
	if((config::use_firmware) && (config::gb_type == 4))
	{
		if(!gbe_plus->read_firmware(config::nds_firmware_path)) { return 0; }
	}

	//Engage the core
	gbe_plus->start();
	gbe_plus->db_unit.debug_mode = config::use_debugger;

	if(gbe_plus->db_unit.debug_mode) { SDL_CloseAudio(); }

	//Disbale mouse cursor in SDL, it's annoying
	SDL_ShowCursor(SDL_DISABLE);

	//Actually run the core
	gbe_plus->run_core();

	return 0;
}  
