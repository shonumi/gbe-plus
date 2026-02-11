// GB Enhanced+ Copyright Daniel Baxter 2020
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : core.cpp
// Date : December 03, 2020
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
MIN_core::MIN_core()
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
	db_unit.vb_count = 0;

	db_unit.breakpoints.clear();
	db_unit.watchpoint_addr.clear();
	db_unit.watchpoint_val.clear();

	//Advanced debugging
	#ifdef GBE_DEBUG
	db_unit.write_addr.clear();
	db_unit.read_addr.clear();
	#endif

	std::cout<<"GBE::Launching MIN core\n";

	//OSD
	config::osd_message = "MIN CORE INIT";
	config::osd_count = 180;
}

/****** Start the core ******/
void MIN_core::start()
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
void MIN_core::stop()
{
	running = false;
	core_cpu.running = false;
	db_unit.debug_mode = false;
}

/****** Shutdown core's components ******/
void MIN_core::shutdown()
{
	core_mmu.MIN_MMU::~MIN_MMU();
	core_cpu.S1C88::~S1C88();
}

/****** Reset the core ******/
void MIN_core::reset()
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
	core_mmu.set_lcd_data(&core_cpu.controllers.video.lcd_stat);

	//Link APU and MMU
	core_cpu.controllers.audio.mem = &core_mmu;
	core_mmu.set_apu_data(&core_cpu.controllers.audio.apu_stat);

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
void MIN_core::load_state(u8 slot)
{
	std::string id = (slot > 0) ? util::to_str(slot) : "";

	std::string state_file = config::rom_file + ".ss";
	state_file += id;

	u32 offset = 0;

	//Check if save state is accessible
	std::ifstream test(state_file.c_str());
	
	if(!test.good())
	{
		config::osd_message = "NO SS " + util::to_str(slot);
		config::osd_count = 180;
		return;
	}

	if(!get_save_state_info(offset, state_file)) { return; }
	offset += sizeof(MIN_SAVE_STATE_VERSION);
	offset += sizeof(config::gb_type);

	if(!core_cpu.cpu_read(offset, state_file)) { return; }
	offset += core_cpu.size();

	if(!core_mmu.mmu_read(offset, state_file)) { return; }
	offset += core_mmu.size();

	if(!core_cpu.controllers.audio.apu_read(offset, state_file)) { return; }
	offset += core_cpu.controllers.audio.size();

	if(!core_cpu.controllers.video.lcd_read(offset, state_file)) { return; }

	std::cout<<"GBE::Loaded state " << state_file << "\n";

	//OSD
	config::osd_message = "LOADED SS " + util::to_str(slot);
	config::osd_count = 180;
}

/****** Saves a save state ******/
void MIN_core::save_state(u8 slot)
{
	std::string id = (slot > 0) ? util::to_str(slot) : "";

	std::string state_file = config::rom_file + ".ss";
	state_file += id;

	if(!set_save_state_info(state_file)) { return; }
	if(!core_cpu.cpu_write(state_file)) { return; }
	if(!core_mmu.mmu_write(state_file)) { return; }
	if(!core_cpu.controllers.audio.apu_write(state_file)) { return; }
	if(!core_cpu.controllers.video.lcd_write(state_file)) { return; }

	std::cout<<"GBE::Saved state " << state_file << "\n";

	//OSD
	config::osd_message = "SAVE SS " + util::to_str(slot);
	config::osd_count = 180;
}

/****** Gets the save state info (Version + System Type) ******/
bool MIN_core::get_save_state_info(u32 offset, std::string filename)
{
	u32 version = 0;
	u8 system_type = 0;

	std::ifstream file(filename.c_str(), std::ios::binary);
	if(!file.is_open()) { return false; }

	file.seekg(offset);
	file.read((char*)&version, sizeof(version));
	file.read((char*)&system_type, sizeof(system_type));
	file.close();

	if(system_type != config::gb_type)
	{
		std::cout<<"GBE::Error - Save State " <<  filename << " has incorrect system type. Cannot load save.\n";
		return false;
	}

	if(version != MIN_SAVE_STATE_VERSION)
	{
		std::cout<<"GBE::Error - Save State " <<  filename << " has outdated version number. Cannot load save.\n";
		return false;
	}

	return true;
}

/****** Sets the save state info (Version + System Type) ******/
bool MIN_core::set_save_state_info(std::string filename)
{
	std::ofstream file(filename.c_str(), std::ios::binary | std::ios::trunc);
	if(!file.is_open()) { return false; }

	file.write((char*)&MIN_SAVE_STATE_VERSION, sizeof(MIN_SAVE_STATE_VERSION));
	file.write((char*)&config::gb_type, sizeof(config::gb_type));
	file.close();

	return true;
}

/****** Run the core in a loop until exit ******/
void MIN_core::run_core()
{
	//Begin running the core
	while(running)
	{
		//Handle SDL Events
		if((core_cpu.controllers.video.lcd_stat.prc_counter == 1) && SDL_PollEvent(&event))
		{
			//X out of a window
			if(event.type == SDL_QUIT) { stop(); SDL_Quit(); }

			//Process gamepad or hotkey
			else if((event.type == SDL_KEYDOWN) || (event.type == SDL_KEYUP) 
			|| (event.type == SDL_JOYBUTTONDOWN) || (event.type == SDL_JOYBUTTONUP)
			|| (event.type == SDL_JOYAXISMOTION) || (event.type == SDL_JOYHATMOTION))
			{
				core_pad.handle_input(event);
				handle_hotkey(event);
				process_keypad_irqs();

				//Handle Shock Sensor
				if(core_pad.send_shock_irq)
				{
					core_mmu.update_irq_flags(SHOCK_SENSOR_IRQ);
					core_pad.send_shock_irq = false;
				}
			}

			//Hotplug joypad
			else if((event.type == SDL_JOYDEVICEADDED) && (!core_pad.joy_init)) { core_pad.init(); }
			else if((event.type == SDL_JOYDEVICEREMOVED) && (core_pad.joy_init)) { core_pad.close_joystick(); }
		}

		//Run the CPU
		if(core_cpu.running)
		{
			//Receive byte from another instance of GBE+ via netplay - Manage sync
			if(core_mmu.ir_stat.connected[core_mmu.ir_stat.network_id])
			{
				//Normal IR operations
				if(core_mmu.ir_stat.network_id < 10)
				{
					//Perform syncing operations when hard sync is enabled
					if((config::netplay_hard_sync) && (core_mmu.ir_stat.sync_timeout > 0)) { hard_sync(); }

					//Receive bytes normally
					core_mmu.recv_byte();
				}

				//Process remote signals (TV remote)
				else
				{
					core_mmu.ir_stat.remote_signal_delay -= core_cpu.system_cycles;
					core_mmu.process_remote_signal();
				}
			}

			//Reset system cycles for next instruction
			core_cpu.debug_cycles += core_cpu.system_cycles;
			core_cpu.system_cycles = 0;

			core_cpu.handle_interrupt();

			if(db_unit.debug_mode) { debug_step(); }

			core_cpu.execute();
			core_cpu.clock_system();
		}

		//Stop emulation
		else { stop(); }
	}

	//Shutdown core
	shutdown();
}

/****** Run core for 1 instruction ******/
void MIN_core::step()
{
	//Run the CPU
	if(core_cpu.running)
	{	
		//Reset system cycles for next instruction
		core_cpu.system_cycles = 0;

		core_cpu.handle_interrupt();

		if(db_unit.debug_mode) { debug_step(); }

		core_cpu.execute();
		core_cpu.clock_system();
	}
}

/****** Process hotkey input ******/
void MIN_core::handle_hotkey(SDL_Event& event)
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

	//Switch current netplay connection on F3 
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F3) && (core_mmu.ir_stat.sync_timeout == 0))
	{
		core_mmu.ir_stat.network_id++;
		if(core_mmu.ir_stat.network_id >= 10) { core_mmu.ir_stat.network_id = 0; }
		
		core_mmu.ir_stat.sync_balance = 4;

		//OSD
		if(core_mmu.ir_stat.network_id != config::netplay_id)
		{
			config::osd_message = "P" + util::to_str(core_mmu.ir_stat.network_id + 1) + " LINKED";
			core_mmu.ir_stat.try_connection = true;
		}

		else
		{
			config::osd_message = "NO LINK";
		}

		config::osd_count = 180;
	}

	//Generate random IR signals on F4 - Used to imitate IR sources like TV remotes
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F4))
	{
		core_mmu.ir_stat.connected[10] = true;
		core_mmu.ir_stat.temp_id = core_mmu.ir_stat.network_id;
		core_mmu.ir_stat.network_id = 10;

		srand(SDL_GetTicks());

		//Generate between 32 - 64 random ON/OFF IR signals
		core_mmu.ir_stat.remote_signal_size = (rand() % 64) + 1;
		if(core_mmu.ir_stat.remote_signal_size < 32) { core_mmu.ir_stat.remote_signal_size += 32; }

		//Generate ON/OFF signals lasting between 128 - 256 cycles
		for(u32 x = 0; x < (core_mmu.ir_stat.remote_signal_size * 2); x++)
		{
			core_mmu.ir_stat.remote_signal_cycles[x] = (rand() % 256) + 1;
			if(core_mmu.ir_stat.remote_signal_cycles[x] < 128) { core_mmu.ir_stat.remote_signal_cycles[x] += 128; }
		}

		core_mmu.ir_stat.remote_signal_size *= 2;
		core_mmu.ir_stat.remote_signal_delay = core_mmu.ir_stat.remote_signal_cycles[0];
		core_mmu.ir_stat.remote_signal_index = 0;
	}	

	//Pause and wait for netplay connection on F5
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F5))
	{
		start_netplay();	
	}

	//Disconnect netplay connection on F6
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F6))
	{
		stop_netplay();
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

	//Pause emulation
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_PAUSE))
	{
		config::pause_emu = true;
		SDL_PauseAudio(1);
		std::cout<<"EMU::Paused\n";

		if((config::sdl_render) && (core_cpu.controllers.video.window != NULL))
		{
			config::title.str("GBE+ Paused");
			SDL_SetWindowTitle(core_cpu.controllers.video.window, config::title.str().c_str());
		}

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
	}
		
	//Reset emulation on F8
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F8)) { reset(); }

	//Screenshot on F9
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F9)) 
	{
		std::string save_name = config::ss_path;
		std::string hex_ticks = util::to_hex_str(SDL_GetTicks()).substr(2);

		//Filename = Date + Ticks
		while(hex_ticks.length() < 8) { hex_ticks = "0" + hex_ticks; }
		save_name += (util::get_long_date(false) + "_" + hex_ticks);

		util::save_image(core_cpu.controllers.video.final_screen, save_name);

		//OSD
		config::osd_message = "SAVED SCREENSHOT";
		config::osd_count = 180;
	}

	//Toggle Fullscreen on F12
	else if((event.type == SDL_KEYUP) && (event.key.keysym.sym == SDLK_F12))
	{
		u32 next_w = config::sys_width;
		u32 next_h = config::sys_height;

		//Unset fullscreen
		if(config::flags & SDL_WINDOW_FULLSCREEN)
		{
			config::flags &= ~SDL_WINDOW_FULLSCREEN;
			config::scaling_factor = config::old_scaling_factor;
		}

		//Set fullscreen
		else
		{
			config::flags |= SDL_WINDOW_FULLSCREEN;
			config::old_scaling_factor = config::scaling_factor;

			SDL_DisplayMode current_mode;
			SDL_GetCurrentDisplayMode(0, &current_mode);

			next_w = current_mode.w;
			next_h = current_mode.h;
		}

		//Destroy old window
		SDL_DestroyWindow(core_cpu.controllers.video.window);

		//Initialize new window - SDL
		if(!config::use_opengl)
		{
			core_cpu.controllers.video.window = SDL_CreateWindow("GBE+", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, next_w, next_h, config::flags);
			SDL_GetWindowSize(core_cpu.controllers.video.window, &config::win_width, &config::win_height);
			core_cpu.controllers.video.final_screen = SDL_GetWindowSurface(core_cpu.controllers.video.window);

			//Find the maximum fullscreen dimensions that maintain the original aspect ratio
			if(config::flags & SDL_WINDOW_FULLSCREEN)
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
}

/****** Process hotkey input - Use exsternally when not using SDL ******/
void MIN_core::handle_hotkey(int input, bool pressed)
{
	//Toggle turbo on
	if((input == config::hotkey_turbo) && (pressed)) { config::turbo = true; }

	//Toggle turbo off
	else if((input == config::hotkey_turbo) && (!pressed)) { config::turbo = false; }

	//Switch current netplay connection on F3 
	else if((input == SDLK_F3) && (core_mmu.ir_stat.sync_timeout == 0) && (pressed))
	{
		core_mmu.ir_stat.network_id++;
		if(core_mmu.ir_stat.network_id >= 10) { core_mmu.ir_stat.network_id = 0; }
		
		core_mmu.ir_stat.sync_balance = 4;
		core_mmu.ir_stat.try_connection = true;

		//OSD
		if(core_mmu.ir_stat.network_id != config::netplay_id)
		{
			config::osd_message = "P" + util::to_str(core_mmu.ir_stat.network_id + 1) + " LINKED";
		}

		else
		{
			config::osd_message = "NO LINK";
		}

		config::osd_count = 180;
	}
}

/****** Processs any keypad IRQs that need to fire after user input ******/
void MIN_core::process_keypad_irqs()
{
	//Fire IRQs only when emulating button presses
	if((core_pad.key_input != 0xFF) && (core_pad.send_keypad_irq))
	{
		//A Button IRQ
		if(~core_pad.key_input & 0x1) { core_mmu.update_irq_flags(A_KEY_IRQ); }

		//B Button IRQ
		if(~core_pad.key_input & 0x2) { core_mmu.update_irq_flags(B_KEY_IRQ); }

		//C Button IRQ
		if(~core_pad.key_input & 0x4) { core_mmu.update_irq_flags(C_KEY_IRQ); }

		//Up Button IRQ
		if(~core_pad.key_input & 0x8) { core_mmu.update_irq_flags(UP_KEY_IRQ); }

		//Down Button IRQ
		if(~core_pad.key_input & 0x10) { core_mmu.update_irq_flags(DOWN_KEY_IRQ); }

		//Left Button IRQ
		if(~core_pad.key_input & 0x20) { core_mmu.update_irq_flags(LEFT_KEY_IRQ); }

		//Right Button IRQ
		if(~core_pad.key_input & 0x40) { core_mmu.update_irq_flags(RIGHT_KEY_IRQ); }

		//Power Button IRQ
		if(~core_pad.key_input & 0x80) { core_mmu.update_irq_flags(POWER_KEY_IRQ); }
	}
}

/****** Updates the core's volume ******/
void MIN_core::update_volume(u8 volume)
{
	config::volume = volume;
	core_cpu.controllers.audio.apu_stat.channel_master_volume = config::volume;
}

/****** Feeds key input from an external source (useful for TAS) ******/
void MIN_core::feed_key_input(int sdl_key, bool pressed)
{
	core_pad.process_keyboard(sdl_key, pressed);
	handle_hotkey(sdl_key, pressed);
	process_keypad_irqs();

	//Handle Shock Sensor
	if(core_pad.send_shock_irq)
	{
		core_mmu.update_irq_flags(SHOCK_SENSOR_IRQ);
		core_pad.send_shock_irq = false;
	}
}

/****** Return a CPU register ******/
u32 MIN_core::ex_get_reg(u8 reg_index)
{
	switch(reg_index)
	{
		case 0x0: return core_cpu.reg.a;
		case 0x1: return core_cpu.reg.b;
		case 0x2: return core_cpu.reg.h;
		case 0x3: return core_cpu.reg.l;
		case 0x4: return core_cpu.reg.br;
		case 0x5: return core_cpu.reg.sp;
		case 0x6: return core_cpu.reg.pc;
		case 0x7: return core_cpu.reg.sc;
		case 0x8: return core_cpu.reg.cc;
		default: return 0;
	}
}

/****** Read binary file to memory ******/
bool MIN_core::read_file(std::string filename) { return core_mmu.read_file(filename); }

/****** Read BIOS file into memory ******/
bool MIN_core::read_bios(std::string filename) 
{
	return core_mmu.read_bios(config::bios_file);
}

/****** Read firmware file into memory (not applicable) ******/
bool MIN_core::read_firmware(std::string filename) { return true; }

/****** Returns a byte from core memory ******/
u8 MIN_core::ex_read_u8(u16 address) { return core_mmu.read_u8(address); }

/****** Writes a byte to core memory ******/
void MIN_core::ex_write_u8(u16 address, u8 value) { core_mmu.write_u8(address, value); }

/****** Starts netplay connection ******/
void MIN_core::start_netplay() { }

/****** Perform hard sync for netplay ******/
void MIN_core::hard_sync()
{
	core_mmu.ir_stat.sync_counter += core_cpu.system_cycles;
	core_mmu.ir_stat.sync_timeout -= core_cpu.system_cycles;
	core_mmu.ir_stat.sync_balance -= core_cpu.system_cycles;

	//Stop Hard Sync
	if(core_mmu.ir_stat.sync_timeout <= 0)
	{
		core_mmu.ir_stat.sync_timeout = 0;
		core_mmu.stop_sync();
		return;
	}

	//Once this Pokemon Mini has reached a specified amount of cycles, freeze until the other Pokemon Mini has finished that many cycles
	if(core_mmu.ir_stat.sync_balance <= 0)
	{
		core_mmu.request_sync();
		u32 current_time = SDL_GetTicks();
		u32 timeout = 0;

		while(core_mmu.ir_stat.sync)
		{
			core_mmu.recv_byte();

			//Timeout if 1 second passes
			timeout = SDL_GetTicks();
							
			if((timeout - current_time) >= 1000)
			{
				core_mmu.ir_stat.sync_timeout = 0;
				core_mmu.stop_sync();
				return;
			}						
		}
	}
}

/****** Stops netplay connection ******/
void MIN_core::stop_netplay()
{
	//Only attempt to disconnect if connected at all
	if(core_mmu.ir_stat.connected[core_mmu.ir_stat.network_id])
	{
		core_mmu.disconnect_ir();
		std::cout<<"IR::Netplay connection terminated. Restart to reconnect.\n";
	}
}

/****** Returns miscellaneous data from the core ******/
u32 MIN_core::get_core_data(u32 core_index)
{
	u32 result = 0;
	return result;
}
