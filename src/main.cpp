// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : main.cpp
// Date : August 16, 2014
// Description : Emulator core
//
// This is main. It all begins here ;)

#include "core.h"
#include "config.h"

int main(int argc, char* args[]) 
{

	SDL_Event event;

	Core gba;

	//Parse command-line arguments
	for(int x = 0; x++ < argc - 1;) 
	{ 
		std::string temp_arg = args[x]; 
		config::cli_args.push_back(temp_arg);
	}

	if(!parse_cli_args()) { return 0; }

	//Read specified ROM file
	if(!gba.core_mmu.read_file(config::rom_file)) { return 0; }
	
	//Read BIOS file optionally
	if(config::use_bios) { gba.core_mmu.read_bios(config::bios_file); }

	//Engage the core
	gba.start();
	gba.db_unit.debug_mode = config::use_debugger;

	//Begin running the core
	while(gba.running)
	{
		//Handle SDL Events
		if((gba.core_cpu.controllers.video.current_scanline == 160) && SDL_PollEvent(&event))
		{
			//X out of a window
			if(event.type == SDL_QUIT) { gba.stop(); SDL_Quit(); }

			//Process gamepad or hotkey
			else if((event.type == SDL_KEYDOWN) || (event.type == SDL_KEYUP) 
			|| (event.type == SDL_JOYBUTTONDOWN) || (event.type == SDL_JOYBUTTONUP)
			|| (event.type == SDL_JOYAXISMOTION) || (event.type == SDL_JOYHATMOTION)) { gba.core_pad.handle_input(event); gba.handle_hotkey(event); }
		}

		//Run the CPU
		if(gba.core_cpu.running)
		{	
			if(gba.db_unit.debug_mode) { gba.debug_step(); }

			gba.core_cpu.fetch();
			gba.core_cpu.decode();
			gba.core_cpu.execute();

			gba.core_cpu.handle_interrupt();
		
			//Flush pipeline if necessary
			if(gba.core_cpu.needs_flush) { gba.core_cpu.flush_pipeline(); }

			//Else update the pipeline and PC
			else 
			{ 
				gba.core_cpu.pipeline_pointer = (gba.core_cpu.pipeline_pointer + 1) % 3;
				gba.core_cpu.update_pc(); 
			}
		}

		//Stop emulation
		else { gba.stop(); }
	}

	//Save data if necessary
	gba.core_mmu.save_backup(config::save_file);

	/*
	std::ofstream file("mem.bin", std::ios::binary);
	file.write(reinterpret_cast<char*> (&gba.core_mmu.memory_map[0]), 0x10000000);
	file.close();
	*/

	return 0;
}  
