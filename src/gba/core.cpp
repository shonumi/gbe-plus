// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : core.cpp
// Date : August 16, 2014
// Description : Emulator core
//
// The "Core" is an abstraction for all of emulated components
// It controls the large-scale behavior of the CPU, LCD/GPU, MMU, and APU/DSP
// Can start, stop, and reset emulator

#include <iomanip>
#include <ctime>
#include <sstream>

#include "common/util.h"

#include "core.h"

/****** Core Constructor ******/
AGB_core::AGB_core()
{
	//Link CPU and MMU
	core_cpu.mem = &core_mmu;

	//Link LCD and MMU
	core_cpu.controllers.video.mem = &core_mmu;
	core_mmu.set_lcd_data(&core_cpu.controllers.video.lcd_stat);

	//Link APU and MMU
	core_cpu.controllers.audio.mem = &core_mmu;
	core_mmu.set_apu_data(&core_cpu.controllers.audio.apu_stat);

	//Link MMU and GamePad
	core_cpu.mem->g_pad = &core_pad;

	//Link MMU and CPU's timers
	core_mmu.timer = &core_cpu.controllers.timer;

	db_unit.debug_mode = false;
	db_unit.display_cycles = false;
	db_unit.print_all = false;
	db_unit.print_pc = false;
	db_unit.last_command = "n";

	db_unit.breakpoints.clear();
	db_unit.watchpoint_addr.clear();
	db_unit.watchpoint_val.clear();

	std::cout<<"GBE::Launching GBA core\n";

	//OSD
	config::osd_message = "GBA CORE INIT";
	config::osd_count = 180;
}

/****** Start the core ******/
void AGB_core::start()
{
	running = true;
	core_cpu.running = true;

	//Initialize video output
	if(!core_cpu.controllers.video.init())
	{
		running = false;
		core_cpu.running = false;
	}

	//Initialize audio output
	if(!core_cpu.controllers.audio.init())
	{
		running = false;
		core_cpu.running = false;
	}

	//Initialize the GamePad
	core_pad.init();
}

/****** Stop the core ******/
void AGB_core::stop()
{
	//Handle CPU Sleep mode
	if(core_cpu.sleep) { sleep(); }

	//Handle Hard Reset
	else if(core_cpu.needs_reset) { reset(); }

	//Or stop completely
	else
	{
		running = false;
		core_cpu.running = false;
		db_unit.debug_mode = false;
	}
}

/****** Shutdown core's components ******/
void AGB_core::shutdown()
{
	core_mmu.AGB_MMU::~AGB_MMU();
	core_cpu.ARM7::~ARM7();
}

/****** Force the core to sleep ******/
void AGB_core::sleep()
{
	//White out LCD
	core_cpu.controllers.video.clear_screen_buffer(0xFFFFFFFF);
	core_cpu.controllers.video.update();

	//Wait for L+R+Select input
	bool l_r_select = false;

	while(!l_r_select)
	{
		SDL_PollEvent(&event);

		if((event.type == SDL_KEYDOWN) || (event.type == SDL_KEYUP) 
		|| (event.type == SDL_JOYBUTTONDOWN) || (event.type == SDL_JOYBUTTONUP)
		|| (event.type == SDL_JOYAXISMOTION) || (event.type == SDL_JOYHATMOTION)) { core_pad.handle_input(event); handle_hotkey(event); }

		if(((core_pad.key_input & 0x4) == 0) && ((core_pad.key_input & 0x100) == 0) && ((core_pad.key_input & 0x200) == 0)) { l_r_select = true; }

		SDL_Delay(50);
		core_cpu.controllers.video.update();
	}

	core_cpu.sleep = false;
	core_cpu.running = true;
}

/****** Reset the core ******/
void AGB_core::reset()
{
	bool can_reset = true;

	core_cpu.reset();
	core_cpu.controllers.video.reset();
	core_cpu.controllers.audio.reset();
	core_mmu.reset();

	//Link CPU and MMU
	core_cpu.mem = &core_mmu;

	//Link LCD and MMU
	core_cpu.controllers.video.mem = &core_mmu;

	//Link APU and MMU
	core_cpu.controllers.audio.mem = &core_mmu;

	//Link MMU and GamePad
	core_cpu.mem->g_pad = &core_pad;

	//Link MMU and CPU's timers
	core_mmu.timer = &core_cpu.controllers.timer;

	//Re-read specified ROM file
	if(!core_mmu.read_file(config::rom_file)) { can_reset = false; }

	//Re-read BIOS file
	if((config::use_bios) && (!read_bios(config::bios_file))) { can_reset = false; }

	//Start everything all over again
	if(can_reset) { start(); }
	else { running = false; }
}

/****** Loads a save state ******/
void AGB_core::load_state(u8 slot)
{
	std::string id = (slot > 0) ? util::to_str(slot) : "";

	std::string state_file = config::rom_file + ".ss";
	state_file += id;

	u32 offset = 0;

	if(!core_cpu.cpu_read(offset, state_file)) { return; }
	offset += core_cpu.size();

	if(!core_mmu.mmu_read(offset, state_file)) { return; }
	offset += core_mmu.size();

	if(!core_cpu.controllers.audio.apu_read(offset, state_file)) { return; }
	offset += core_cpu.controllers.audio.size();

	if(!core_cpu.controllers.video.lcd_read(offset, state_file)) { return; }

	std::cout<<"GBE::Loaded state " << state_file << "\n";

	//OSD
	config::osd_message = "LOADED STATE " + util::to_str(slot);
	config::osd_count = 180;
}

/****** Saves a save state ******/
void AGB_core::save_state(u8 slot)
{
	std::string id = (slot > 0) ? util::to_str(slot) : "";

	std::string state_file = config::rom_file + ".ss";
	state_file += id;

	if(!core_cpu.cpu_write(state_file)) { return; }
	if(!core_mmu.mmu_write(state_file)) { return; }
	if(!core_cpu.controllers.audio.apu_write(state_file)) { return; }
	if(!core_cpu.controllers.video.lcd_write(state_file)) { return; }

	std::cout<<"GBE::Saved state " << state_file << "\n";

	//OSD
	config::osd_message = "SAVED STATE " + util::to_str(slot);
	config::osd_count = 180;
}

/****** Run the core in a loop until exit ******/
void AGB_core::run_core()
{
	//Begin running the core
	while(running)
	{
		//Handle SDL Events
		if((core_cpu.controllers.video.current_scanline == 160) && SDL_PollEvent(&event))
		{
			//X out of a window
			if(event.type == SDL_QUIT) { stop(); SDL_Quit(); }

			//Process gamepad or hotkey
			else if((event.type == SDL_KEYDOWN) || (event.type == SDL_KEYUP) 
			|| (event.type == SDL_JOYBUTTONDOWN) || (event.type == SDL_JOYBUTTONUP)
			|| (event.type == SDL_JOYAXISMOTION) || (event.type == SDL_JOYHATMOTION)) { core_pad.handle_input(event); handle_hotkey(event); }
		}

		//Run the CPU
		if(core_cpu.running)
		{	
			if(db_unit.debug_mode) { debug_step(); }

			core_cpu.fetch();
			core_cpu.decode();
			core_cpu.execute();

			core_cpu.handle_interrupt();
		
			//Flush pipeline if necessary
			if(core_cpu.needs_flush) { core_cpu.flush_pipeline(); }

			//Else update the pipeline and PC
			else 
			{ 
				core_cpu.pipeline_pointer = (core_cpu.pipeline_pointer + 1) % 3;
				core_cpu.update_pc(); 
			}
		}

		//Stop emulation
		else { stop(); }
	}

	//Shutdown core
	shutdown();
}

/****** Run core for 1 instruction ******/
void AGB_core::step()
{
	//Run the CPU
	if(core_cpu.running)
	{	
		core_cpu.fetch();
		core_cpu.decode();
		core_cpu.execute();

		core_cpu.handle_interrupt();
		
		//Flush pipeline if necessary
		if(core_cpu.needs_flush) { core_cpu.flush_pipeline(); }

		//Else update the pipeline and PC
		else 
		{ 
			core_cpu.pipeline_pointer = (core_cpu.pipeline_pointer + 1) % 3;
			core_cpu.update_pc(); 
		}
	}
}
	
/****** Process hotkey input ******/
void AGB_core::handle_hotkey(SDL_Event& event)
{
	//Disallow key repeats
	if(event.key.repeat) { return; }

	//Quit on Q or ESC
	if((event.type == SDL_KEYDOWN) && ((event.key.keysym.sym == SDLK_q) || (event.key.keysym.sym == SDLK_ESCAPE)))
	{
		running = false; 
		SDL_Quit();
	}

	//Mute or unmute sound on M
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == config::hotkey_mute) && (!config::use_external_interfaces))
	{
		if(config::volume == 0)
		{
			update_volume(config::old_volume);
		}

		else
		{
			config::old_volume = config::volume;
			update_volume(0);
		}
	}

	//Quick save state on F1
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F1)) 
	{
		save_state(0);
	}

	//Quick load save state on F2
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F2)) 
	{
		load_state(0);
	}

	//Start CLI debugger on F7
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F7) && (!config::use_external_interfaces))
	{
		//Start a new CLI debugger session or interrupt an existing one in Continue Mode 
		if((!db_unit.debug_mode) || ((db_unit.debug_mode) && (db_unit.last_command == "c")))
		{
			db_unit.debug_mode = true;
			db_unit.last_command = "n";
			db_unit.last_mnemonic = "";
		}
	}

	//Screenshot on F9
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F9)) 
	{
		std::stringstream save_stream;
		std::string save_name = config::ss_path;

		//Prefix SDL Ticks to screenshot name
		save_stream << SDL_GetTicks();
		save_name += save_stream.str();
		save_stream.str(std::string());

		//Append random number to screenshot name
		srand(SDL_GetTicks());
		save_stream << rand() % 1024 << rand() % 1024 << rand() % 1024;
		save_name += save_stream.str() + ".bmp";
	
		SDL_SaveBMP(core_cpu.controllers.video.final_screen, save_name.c_str());

		//OSD
		config::osd_message = "SAVED SCREENSHOT";
		config::osd_count = 180;
	}

	//Toggle Fullscreen on F12
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F12))
	{
		//Unset fullscreen
		if(config::flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
		{
			config::flags &= ~SDL_WINDOW_FULLSCREEN_DESKTOP;
			config::scaling_factor = config::old_scaling_factor;
		}

		//Set fullscreen
		else
		{
			config::flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
			config::old_scaling_factor = config::scaling_factor;
		}

		//Destroy old window
		SDL_DestroyWindow(core_cpu.controllers.video.window);

		//Initialize new window - SDL
		if(!config::use_opengl)
		{
			core_cpu.controllers.video.window = SDL_CreateWindow("GBE+", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, config::sys_width, config::sys_height, config::flags);
			core_cpu.controllers.video.final_screen = SDL_GetWindowSurface(core_cpu.controllers.video.window);
			SDL_GetWindowSize(core_cpu.controllers.video.window, &config::win_width, &config::win_height);

			//Find the maximum fullscreen dimensions that maintain the original aspect ratio
			if(config::flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
			{
				double max_width, max_height, ratio = 0.0;

				max_width = (double)config::win_width / config::sys_width;
				max_height = (double)config::win_height / config::sys_height;

				if(max_width <= max_height) { ratio = max_width; }
				else { ratio = max_height; }

				core_cpu.controllers.video.max_fullscreen_ratio = ratio;
			}
		}

		//Initialize new window - OpenGL
		else
		{
			core_cpu.controllers.video.opengl_init();
		}
	}

	//Pause emulation
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_PAUSE))
	{
		config::pause_emu = true;
		SDL_PauseAudio(1);
		std::cout<<"EMU::Paused\n";

		//Delay until pause key is hit again
		while(config::pause_emu)
		{
			SDL_Delay(50);
			if((SDL_PollEvent(&event)) && (event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_PAUSE))
			{
				config::pause_emu = false;
				SDL_PauseAudio(0);
				std::cout<<"EMU::Unpaused\n";
			}
		}
	}

	//Toggle turbo on
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == config::hotkey_turbo))
	{
		config::turbo = true;
		if((config::sdl_render) && (config::use_opengl)) { SDL_GL_SetSwapInterval(0); }
	}

	//Toggle turbo off
	else if((event.type == SDL_KEYUP) && (event.key.keysym.sym == config::hotkey_turbo))
	{
		config::turbo = false;
		if((config::sdl_render) && (config::use_opengl)) { SDL_GL_SetSwapInterval(1); }
	}
		
	//Reset emulation on F8
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F8)) { reset(); }
}

/****** Process hotkey input - Use exsternally when not using SDL ******/
void AGB_core::handle_hotkey(int input, bool pressed)
{
	//Toggle turbo on
	if((input == config::hotkey_turbo) && (pressed)) { config::turbo = true; }

	//Toggle turbo off
	else if((input == config::hotkey_turbo) && (!pressed)) { config::turbo = false; }
}

/****** Updates the core's volume ******/
void AGB_core::update_volume(u8 volume)
{
	config::volume = volume;
	u8 sound_control = core_mmu.memory_map[SNDCNT_H];

	switch(sound_control & 0x3)
	{
		case 0x0: core_cpu.controllers.audio.apu_stat.channel_master_volume = (config::volume >> 4); break;
		case 0x1: core_cpu.controllers.audio.apu_stat.channel_master_volume = (config::volume >> 3); break;
		case 0x2: core_cpu.controllers.audio.apu_stat.channel_master_volume = (config::volume >> 2); break;
		case 0x3: std::cout<<"MMU::Setting prohibited Sound Channel master volume - 0x3\n"; break;
	}

	core_cpu.controllers.audio.apu_stat.dma[0].master_volume = (sound_control & 0x4) ? config::volume : (config::volume >> 1);
	core_cpu.controllers.audio.apu_stat.dma[1].master_volume = (sound_control & 0x8) ? config::volume : (config::volume >> 1);
}

/****** Feeds key input from an external source (useful for TAS) ******/
void AGB_core::feed_key_input(int sdl_key, bool pressed)
{
	core_pad.process_keyboard(sdl_key, pressed);
	handle_hotkey(sdl_key, pressed);
}

/****** Return a CPU register ******/
u32 AGB_core::ex_get_reg(u8 reg_index)
{
	switch(reg_index)
	{
		case 0x0: return core_cpu.reg.r0;
		case 0x1: return core_cpu.reg.r1;
		case 0x2: return core_cpu.reg.r2;
		case 0x3: return core_cpu.reg.r3;
		case 0x4: return core_cpu.reg.r4;
		case 0x5: return core_cpu.reg.r5;
		case 0x6: return core_cpu.reg.r6;
		case 0x7: return core_cpu.reg.r7;
		case 0x8: return core_cpu.reg.r8;
		case 0x9: return core_cpu.reg.r9;
		case 0xA: return core_cpu.reg.r10;
		case 0xB: return core_cpu.reg.r11;
		case 0xC: return core_cpu.reg.r12;
		case 0xD: return core_cpu.reg.r13;
		case 0xE: return core_cpu.reg.r14;
		case 0xF: return core_cpu.reg.r15;
		case 0x10: return core_cpu.reg.cpsr;
		case 0x11: return core_cpu.reg.r8_fiq;
		case 0x12: return core_cpu.reg.r9_fiq;
		case 0x13: return core_cpu.reg.r10_fiq;
		case 0x14: return core_cpu.reg.r11_fiq;
		case 0x15: return core_cpu.reg.r12_fiq;
		case 0x16: return core_cpu.reg.r13_fiq;
		case 0x17: return core_cpu.reg.r14_fiq;
		case 0x18: return core_cpu.reg.spsr_fiq;
		case 0x19: return core_cpu.reg.r13_svc;
		case 0x1A: return core_cpu.reg.r14_svc;
		case 0x1B: return core_cpu.reg.spsr_svc;
		case 0x1C: return core_cpu.reg.r13_abt;
		case 0x1D: return core_cpu.reg.r14_abt;
		case 0x1E: return core_cpu.reg.spsr_abt;
		case 0x1F: return core_cpu.reg.r13_irq;
		case 0x20: return core_cpu.reg.r14_irq;
		case 0x21: return core_cpu.reg.spsr_irq;
		case 0x22: return core_cpu.reg.r13_und;
		case 0x23: return core_cpu.reg.r14_und;
		case 0x24: return core_cpu.reg.spsr_und;
	}
}

/****** Read binary file to memory ******/
bool AGB_core::read_file(std::string filename) { return core_mmu.read_file(filename); }

/****** Read BIOS file into memory ******/
bool AGB_core::read_bios(std::string filename) 
{
	core_cpu.reg.r15 = 0;
	return core_mmu.read_bios(config::bios_file);
}

/****** Read firmware file into memory (not applicable) ******/
bool AGB_core::read_firmware(std::string filename) { return true; }

/****** Returns a byte from core memory ******/
u8 AGB_core::ex_read_u8(u16 address) { return core_mmu.read_u8(address); }

/****** Writes a byte to core memory ******/
void AGB_core::ex_write_u8(u16 address, u8 value) { core_mmu.write_u8(address, value); }

/****** Dumps selected OBJ to a file ******/
void AGB_core::dump_obj(int obj_index) { }

/****** Dumps selected BG tile to a file ******/
void AGB_core::dump_bg(int bg_index) { }

/****** Grabs the OBJ palette ******/
u32* AGB_core::get_obj_palette(int pal_index) 
{
	return NULL; 
}

/****** Grabs the BG palette ******/
u32* AGB_core::get_bg_palette(int pal_index)
{
	return NULL;
}

/****** Grabs the hash for a specific tile ******/
std::string AGB_core::get_hash(u32 addr, u8 gfx_type) { }

/****** Starts netplay connection ******/
void AGB_core::start_netplay() { }

/****** Stops netplay connection ******/
void AGB_core::stop_netplay() { }

/****** Returns miscellaneous data from the core ******/
u32 AGB_core::get_core_data(u32 core_index)
{
	u32 result = 0;

	switch(core_index)
	{
		//Joypad state
		case 0x0:
			result = ~(core_pad.key_input);
			result &= 0x3FF;
			break;
	}

	return result;
}
