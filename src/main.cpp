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
#include "common/config.h"

int main(int argc, char* args[]) 
{

	core_emu* gbe_plus = NULL;

	//Parse .ini options
	if(!parse_ini_file()) { return 0; }

	//Parse command-line arguments
	//These will override .ini options!
	for(int x = 0; x++ < argc - 1;) 
	{ 
		std::string temp_arg = args[x]; 
		config::cli_args.push_back(temp_arg);
	}
	
	if(!parse_cli_args()) { return 0; }

	//Start the appropiate system core - DMG/GBC or GBA
	if(config::gb_type == 3) { gbe_plus = new AGB_core(); }
	else { gbe_plus = new DMG_core(); }

	//Read specified ROM file
	if(!gbe_plus->read_file(config::rom_file)) { return 0; }
	
	//Read BIOS file optionally
	if(config::use_bios) 
	{ 
		if(!gbe_plus->read_bios(config::bios_file)) { return 0; } 
	}

	//Engage the core
	gbe_plus->start();
	gbe_plus->db_unit.debug_mode = config::use_debugger;

	if(gbe_plus->db_unit.debug_mode) { SDL_CloseAudio(); }

	//Disbale mouse cursor in SDL, it's annoying
	SDL_ShowCursor(SDL_DISABLE);

	//Set program window caption
	SDL_WM_SetCaption("GBE+", NULL);

	//Actually run the core
	gbe_plus->run_core();

	return 0;
}  
