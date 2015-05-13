// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : main.cpp
// Date : August 16, 2014
// Description : Emulator core
//
// This is main. It all begins here ;)

#include "gba/core.h"
#include "common/config.h"

int main(int argc, char* args[]) 
{

	core_emu* gba = new AGB_core;

	//Parse command-line arguments
	for(int x = 0; x++ < argc - 1;) 
	{ 
		std::string temp_arg = args[x]; 
		config::cli_args.push_back(temp_arg);
	}

	if(!parse_cli_args()) { return 0; }

	//Read specified ROM file
	if(!gba->read_file(config::rom_file)) { return 0; }
	
	//Read BIOS file optionally
	if(config::use_bios) { gba->read_bios(config::bios_file); }

	//Engage the core
	gba->start();
	gba->db_unit.debug_mode = config::use_debugger;

	if(gba->db_unit.debug_mode) { SDL_CloseAudio(); }

	//Disbale mouse cursor in SDL, it's annoying
	SDL_ShowCursor(SDL_DISABLE);

	//Set program window caption
	SDL_WM_SetCaption("GBE+", NULL);

	//Actually run the core
	gba->run_core();

	return 0;
}  
