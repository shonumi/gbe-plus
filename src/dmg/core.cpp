// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : core.cpp
// Date : May 17, 2015
// Description : Emulator core
//
// The "Core" is an abstraction for all of emulated components
// It controls the large-scale behavior of the CPU, LCD/GPU, MMU, and APU/DSP
// Can start, stop, and reset emulator

#include <iomanip>
#include <ctime>
#include <sstream>

#include "common/util.h"
#include "common/cgfx_common.h"

#include "core.h"

/****** Core Constructor ******/
DMG_core::DMG_core()
{
	//Link CPU and MMU
	core_cpu.mem = &core_mmu;

	//Link LCD and MMU
	core_cpu.controllers.video.mem = &core_mmu;
	core_mmu.set_lcd_data(&core_cpu.controllers.video.lcd_stat);
	core_mmu.set_cgfx_data(&core_cpu.controllers.video.cgfx_stat);

	//Link APU and MMU
	core_cpu.controllers.audio.mem = &core_mmu;
	core_mmu.set_apu_data(&core_cpu.controllers.audio.apu_stat);

	//Link SIO and MMU
	core_cpu.controllers.serial_io.mem = &core_mmu;
	core_mmu.set_sio_data(&core_cpu.controllers.serial_io.sio_stat);

	//Link MMU and GamePad
	core_cpu.mem->g_pad = &core_pad;

	db_unit.debug_mode = false;
	db_unit.display_cycles = false;
	db_unit.print_all = false;
	db_unit.last_command = "n";
	db_unit.last_mnemonic = "";
	db_unit.last_pc = 0;

	db_unit.breakpoints.clear();
	db_unit.watchpoint_addr.clear();
	db_unit.watchpoint_val.clear();

	std::cout<<"GBE::Launching DMG-GBC core\n";

	//OSD
	config::osd_message = "DMG GBC CORE INIT";
	config::osd_count = 180;
}

/****** Start the core ******/
void DMG_core::start()
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

	//Initialize SIO
	core_cpu.controllers.serial_io.init();

	//Initialize the GamePad
	core_pad.init();
}

/****** Stop the core ******/
void DMG_core::stop()
{
	running = false;
	core_cpu.running = false;
	db_unit.debug_mode = false;
}

/****** Shutdown core's components ******/
void DMG_core::shutdown()
{
	core_mmu.DMG_MMU::~DMG_MMU();
	core_cpu.Z80::~Z80();
	config::gba_enhance = false;
}

/****** Reset the core ******/
void DMG_core::reset()
{
	bool can_reset = true;

	core_cpu.reset();
	core_cpu.controllers.video.reset();
	core_cpu.controllers.audio.reset();
	core_cpu.controllers.serial_io.reset();
	core_mmu.reset();

	//Link CPU and MMU
	core_cpu.mem = &core_mmu;

	//Link LCD and MMU
	core_cpu.controllers.video.mem = &core_mmu;

	//Link APU and MMU
	core_cpu.controllers.audio.mem = &core_mmu;

	//Link SIO and MMU
	core_cpu.controllers.serial_io.mem = &core_mmu;

	//Link MMU and GamePad
	core_cpu.mem->g_pad = &core_pad;

	//Re-read specified ROM file
	if(!core_mmu.read_file(config::rom_file)) { can_reset = false; }

	//Re-read BIOS file
	if((config::use_bios) && (!read_bios(config::bios_file))) { can_reset = false; }

	//Start everything all over again
	if(can_reset) { start(); }
	else { running = false; }
}

/****** Loads a save state ******/
void DMG_core::load_state(u8 slot)
{
	std::string id = (slot > 0) ? util::to_str(slot) : "";

	std::string state_file = config::rom_file + ".ss";
	state_file += id;

	u32 offset = 0;

	//Offset 0, size 43
	if(!core_cpu.cpu_read(offset, state_file)) { return; }
	offset += core_cpu.size();	

	//Offset 43, size 213047
	if(!core_mmu.mmu_read(offset, state_file)) { return; }
	offset += core_mmu.size();

	//Offset 213090, size 320
	if(!core_cpu.controllers.audio.apu_read(offset, state_file)) { return; }
	offset += core_cpu.controllers.audio.size();

	//Offset 213410
	if(!core_cpu.controllers.video.lcd_read(offset, state_file)) { return; }

	std::cout<<"GBE::Loaded state " << state_file << "\n";

	//Invalidate current CGFX
	if(cgfx::load_cgfx) { core_cpu.controllers.video.invalidate_cgfx(); }

	//OSD
	config::osd_message = "LOADED STATE " + util::to_str(slot);
	config::osd_count = 180;
}

/****** Saves a save state ******/
void DMG_core::save_state(u8 slot)
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
void DMG_core::run_core()
{
	if(config::gb_type == 2) { core_cpu.reg.a = 0x11; }

	//Begin running the core
	while(running)
	{
		//Handle SDL Events
		if((core_cpu.controllers.video.lcd_stat.current_scanline == 144) && SDL_PollEvent(&event))
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
			//Receive byte from another instance of GBE+ via netplay
			if(core_cpu.controllers.serial_io.sio_stat.connected)
			{
				//Perform syncing operations when hard sync is enabled
				if(config::netplay_hard_sync)
				{
					core_cpu.controllers.serial_io.sio_stat.sync_counter += (core_cpu.double_speed) ? (core_cpu.cycles >> 1) : core_cpu.cycles;

					//Once this Game Boy has reached a specified amount of cycles, freeze until the other Game Boy finished that many cycles
					if(core_cpu.controllers.serial_io.sio_stat.sync_counter >= core_cpu.controllers.serial_io.sio_stat.sync_clock)
					{
						core_cpu.controllers.serial_io.request_sync();
						u32 current_time = SDL_GetTicks();
						u32 timeout = 0;

						while(core_cpu.controllers.serial_io.sio_stat.sync)
						{
							core_cpu.controllers.serial_io.receive_byte();
							if(core_cpu.controllers.serial_io.is_master) { core_cpu.controllers.serial_io.four_player_request_sync(); }

							//Timeout if 10 seconds passes
							timeout = SDL_GetTicks();
							
							if((timeout - current_time) >= 10000)
							{
								core_cpu.controllers.serial_io.reset();
							}						
						}
					}
				}

				//Send IR signal for GBC games
				if(core_mmu.ir_send) { core_cpu.controllers.serial_io.send_ir_signal(); }

				//Receive bytes normally
				core_cpu.controllers.serial_io.receive_byte();

				//Fade IR signal after a certain amount of time
				if(core_mmu.ir_counter > 0)
				{
					core_mmu.ir_counter -= core_cpu.cycles;

					if(core_mmu.ir_counter <= 0)
					{
						core_mmu.ir_counter = 0;
						core_mmu.memory_map[REG_RP] &= ~0x2;
					}
				}
			}

			core_cpu.debug_cycles += core_cpu.cycles;
			core_cpu.cycles = 0;

			//Handle Interrupts
			core_cpu.handle_interrupts();

			if(db_unit.debug_mode) { debug_step(); }
	
			//Halt CPU if necessary
			if(core_cpu.halt == true)
			{
				//Normal HALT mode
				if(core_cpu.interrupt || !core_cpu.skip_instruction) { core_cpu.cycles += 4; }

				//HALT bug
				else if(core_cpu.skip_instruction)
				{
					//Exit HALT mode
					core_cpu.halt = false;
					core_cpu.skip_instruction = false;

					//Execute next opcode, but do not increment PC
					core_cpu.opcode = core_mmu.read_u8(core_cpu.reg.pc);
					core_cpu.exec_op(core_cpu.opcode);
				}
			}

			//Process Opcodes
			else 
			{
				core_cpu.opcode = core_mmu.read_u8(core_cpu.reg.pc++);
				core_cpu.exec_op(core_cpu.opcode);
			}

			//Update LCD
			if(core_cpu.double_speed) { core_cpu.controllers.video.step(core_cpu.cycles >> 1); }
			else { core_cpu.controllers.video.step(core_cpu.cycles); }

			//Update DIV timer - Every 4 M clocks
			core_cpu.div_counter += core_cpu.cycles;
		
			if(core_cpu.div_counter >= 256) 
			{
				core_cpu.div_counter -= 256;
				core_mmu.memory_map[REG_DIV]++;
			}

			//Update TIMA timer
			if(core_mmu.memory_map[REG_TAC] & 0x4) 
			{	
				core_cpu.tima_counter += core_cpu.cycles;

				switch(core_mmu.memory_map[REG_TAC] & 0x3)
				{
					case 0x00: core_cpu.tima_speed = 1024; break;
					case 0x01: core_cpu.tima_speed = 16; break;
					case 0x02: core_cpu.tima_speed = 64; break;
					case 0x03: core_cpu.tima_speed = 256; break;
				}
	
				if(core_cpu.tima_counter >= core_cpu.tima_speed)
				{
					core_mmu.memory_map[REG_TIMA]++;
					core_cpu.tima_counter -= core_cpu.tima_speed;

					if(core_mmu.memory_map[REG_TIMA] == 0)
					{
						core_mmu.memory_map[IF_FLAG] |= 0x04;
						core_mmu.memory_map[REG_TIMA] = core_mmu.memory_map[REG_TMA];
					}	

				}
			}

			//Update serial input-output operations
			if(core_cpu.controllers.serial_io.sio_stat.shifts_left != 0)
			{
				core_cpu.controllers.serial_io.sio_stat.shift_counter += (core_cpu.double_speed) ? (core_cpu.cycles >> 1) : core_cpu.cycles;

				if((core_cpu.controllers.serial_io.barcode_boy.send_data) && ((core_mmu.memory_map[REG_SC] & 0x80) == 0))
				{
					core_cpu.controllers.serial_io.sio_stat.shifts_left = 8;
					core_cpu.controllers.serial_io.sio_stat.shift_counter = 0;
				}	

				//After SIO clocks, perform SIO operations now
				if(core_cpu.controllers.serial_io.sio_stat.shift_counter >= core_cpu.controllers.serial_io.sio_stat.shift_clock)
				{
					//Shift bit out from SB, transfer it
					core_mmu.memory_map[REG_SB] <<= 1;

					core_cpu.controllers.serial_io.sio_stat.shift_counter -= core_cpu.controllers.serial_io.sio_stat.shift_clock;
					core_cpu.controllers.serial_io.sio_stat.shifts_left--;

					//Complete the transfer
					if(core_cpu.controllers.serial_io.sio_stat.shifts_left == 0)
					{
						//Reset Bit 7 in SC
						core_mmu.memory_map[REG_SC] &= ~0x80;

						core_cpu.controllers.serial_io.sio_stat.active_transfer = false;

						switch(core_cpu.controllers.serial_io.sio_stat.sio_type)
						{
							//Process normal SIO communications
							case NO_GB_DEVICE:
							case GB_LINK:
								//Emulate disconnected link cable (on an internal clock) with no netplay
								if((core_cpu.controllers.serial_io.sio_stat.internal_clock)
								&& (!config::use_netplay || !core_cpu.controllers.serial_io.sio_stat.connected))
								{
									core_mmu.memory_map[REG_SB] = 0xFF;
									core_mmu.memory_map[IF_FLAG] |= 0x08;
								}

								//Send byte to another instance of GBE+ via netplay
								if(core_cpu.controllers.serial_io.sio_stat.connected) { core_cpu.controllers.serial_io.send_byte(); }
						
								break;

							//Process GB Printer communications
							case GB_PRINTER:
								core_cpu.controllers.serial_io.printer_process();
								break;

							//Process GB Mobile Adapter communications
							case GB_MOBILE_ADAPTER:
								core_cpu.controllers.serial_io.mobile_adapter_process();
								break;

							//Process Bardigun card scanner communications
							case GB_BARDIGUN_SCANNER:
								core_cpu.controllers.serial_io.bardigun_process();
								break;

							//Process Barcode Boy communications
							case GB_BARCODE_BOY:
								core_cpu.controllers.serial_io.barcode_boy_process();

								if(core_cpu.controllers.serial_io.barcode_boy.send_data)
								{
									core_mmu.memory_map[REG_SB] = core_cpu.controllers.serial_io.barcode_boy.byte;
									core_mmu.memory_map[IF_FLAG] |= 0x08;
								}
									
								break;

							//Process 4 Player communications
							case GB_FOUR_PLAYER_ADAPTER:
								core_cpu.controllers.serial_io.four_player_process();
								break;
						}

						switch(core_cpu.controllers.serial_io.sio_stat.ir_type)
						{
							//Process Full Changer communications
							case GBC_FULL_CHANGER:
								core_cpu.controllers.serial_io.full_changer_process();
								break;

							//Process Pokemon Pikachu 2 communications
							//Process Pocket Sakura communications
							case GBC_POKEMON_PIKACHU_2:
							case GBC_POCKET_SAKURA:
								core_cpu.controllers.serial_io.pocket_ir_process();
								break;

							//Process TV Remote commnications
							case GBC_TV_REMOTE:
								core_cpu.controllers.serial_io.tv_remote_process();
								break;
						}
					}
				}
			}
		}

		//Stop emulation
		else { stop(); }
	}
	
	//Shutdown core
	shutdown();
}

/****** Manually run core for 1 instruction ******/
void DMG_core::step()
{
	//Run the CPU
	if(core_cpu.running)
	{
		//Receive byte from another instance of GBE+ via netplay
		if(core_cpu.controllers.serial_io.sio_stat.connected)
		{
			//Perform syncing operations when hard sync is enabled
			if(config::netplay_hard_sync)
			{
				core_cpu.controllers.serial_io.sio_stat.sync_counter += (core_cpu.double_speed) ? (core_cpu.cycles >> 1) : core_cpu.cycles;;

				//Once this Game Boy has reached a specified amount of cycles, freeze until the other Game Boy finished that many cycles
				if(core_cpu.controllers.serial_io.sio_stat.sync_counter >= core_cpu.controllers.serial_io.sio_stat.sync_clock)
				{
					core_cpu.controllers.serial_io.request_sync();
					u32 current_time = SDL_GetTicks();
					u32 timeout = 0;

					while(core_cpu.controllers.serial_io.sio_stat.sync)
					{
						core_cpu.controllers.serial_io.receive_byte();
						if(core_cpu.controllers.serial_io.is_master) { core_cpu.controllers.serial_io.four_player_request_sync(); }

						//Timeout if 10 seconds passes
						timeout = SDL_GetTicks();
							
						if((timeout - current_time) >= 10000)
						{
							core_cpu.controllers.serial_io.reset();
						}						
					}
				}
			}

			//Send IR signal for GBC games
			if(core_mmu.ir_send) { core_cpu.controllers.serial_io.send_ir_signal(); }

			//Receive bytes normally
			core_cpu.controllers.serial_io.receive_byte();

			//Fade IR signal after a certain amount of time
			if(core_mmu.ir_counter > 0)
			{
				core_mmu.ir_counter -= core_cpu.cycles;

				if(core_mmu.ir_counter <= 0)
				{
					core_mmu.ir_counter = 0;
					core_mmu.memory_map[REG_RP] &= ~0x2;
				}
			}
		}

		core_cpu.debug_cycles += core_cpu.cycles;
		core_cpu.cycles = 0;

		//Handle Interrupts
		core_cpu.handle_interrupts();
	
		//Halt CPU if necessary
		if(core_cpu.halt == true)
		{
			//Normal HALT mode
			if(core_cpu.interrupt || !core_cpu.skip_instruction) { core_cpu.cycles += 4; }

			//HALT bug
			else if(core_cpu.skip_instruction)
			{
				//Exit HALT mode
				core_cpu.halt = false;
				core_cpu.skip_instruction = false;

				//Execute next opcode, but do not increment PC
				core_cpu.opcode = core_mmu.read_u8(core_cpu.reg.pc);
				core_cpu.exec_op(core_cpu.opcode);
			}
		}

		//Process Opcodes
		else 
		{
			core_cpu.opcode = core_mmu.read_u8(core_cpu.reg.pc++);
			core_cpu.exec_op(core_cpu.opcode);
		}

		//Update LCD
		if(core_cpu.double_speed) { core_cpu.controllers.video.step(core_cpu.cycles >> 1); }
		else { core_cpu.controllers.video.step(core_cpu.cycles); }

		//Update DIV timer - Every 4 M clocks
		core_cpu.div_counter += core_cpu.cycles;
		
		if(core_cpu.div_counter >= 256) 
		{
			core_cpu.div_counter -= 256;
			core_mmu.memory_map[REG_DIV]++;
		}

		//Update TIMA timer
		if(core_mmu.memory_map[REG_TAC] & 0x4) 
		{	
			core_cpu.tima_counter += core_cpu.cycles;

			switch(core_mmu.memory_map[REG_TAC] & 0x3)
			{
				case 0x00: core_cpu.tima_speed = 1024; break;
				case 0x01: core_cpu.tima_speed = 16; break;
				case 0x02: core_cpu.tima_speed = 64; break;
				case 0x03: core_cpu.tima_speed = 256; break;
			}
	
			if(core_cpu.tima_counter >= core_cpu.tima_speed)
			{
				core_mmu.memory_map[REG_TIMA]++;
				core_cpu.tima_counter -= core_cpu.tima_speed;

				if(core_mmu.memory_map[REG_TIMA] == 0)
				{
					core_mmu.memory_map[IF_FLAG] |= 0x04;
					core_mmu.memory_map[REG_TIMA] = core_mmu.memory_map[REG_TMA];
				}	

			}
		}

		//Update serial input-output operations
		if(core_cpu.controllers.serial_io.sio_stat.shifts_left != 0)
		{
			core_cpu.controllers.serial_io.sio_stat.shift_counter += (core_cpu.double_speed) ? (core_cpu.cycles >> 1) : core_cpu.cycles;;

			if((core_cpu.controllers.serial_io.barcode_boy.send_data) && ((core_mmu.memory_map[REG_SC] & 0x80) == 0))
			{
				core_cpu.controllers.serial_io.sio_stat.shifts_left = 8;
				core_cpu.controllers.serial_io.sio_stat.shift_counter = 0;
			}	

			//After SIO clocks, perform SIO operations now
			if(core_cpu.controllers.serial_io.sio_stat.shift_counter >= core_cpu.controllers.serial_io.sio_stat.shift_clock)
			{
				//Shift bit out from SB, transfer it
				core_mmu.memory_map[REG_SB] <<= 1;

				core_cpu.controllers.serial_io.sio_stat.shift_counter -= core_cpu.controllers.serial_io.sio_stat.shift_clock;
				core_cpu.controllers.serial_io.sio_stat.shifts_left--;

				//Complete the transfer
				if(core_cpu.controllers.serial_io.sio_stat.shifts_left == 0)
				{
					//Reset Bit 7 in SC
					core_mmu.memory_map[REG_SC] &= ~0x80;

					core_cpu.controllers.serial_io.sio_stat.active_transfer = false;

					switch(core_cpu.controllers.serial_io.sio_stat.sio_type)
					{
						//Process normal SIO communications
						case NO_GB_DEVICE:
						case GB_LINK:
							//Emulate disconnected link cable (on an internal clock) with no netplay
							if((core_cpu.controllers.serial_io.sio_stat.internal_clock)
							&& (!config::use_netplay || !core_cpu.controllers.serial_io.sio_stat.connected))
							{
								core_mmu.memory_map[REG_SB] = 0xFF;
								core_mmu.memory_map[IF_FLAG] |= 0x08;
							}

							//Send byte to another instance of GBE+ via netplay
							if(core_cpu.controllers.serial_io.sio_stat.connected) { core_cpu.controllers.serial_io.send_byte(); }
						
							break;

						//Process GB Printer communications
						case GB_PRINTER:
							core_cpu.controllers.serial_io.printer_process();
							break;

						//Process GB Mobile Adapter communications
						case GB_MOBILE_ADAPTER:
							core_cpu.controllers.serial_io.mobile_adapter_process();
							break;

						//Process Bardigun card scanner communications
						case GB_BARDIGUN_SCANNER:
							core_cpu.controllers.serial_io.bardigun_process();
							break;

						//Process Barcode Boy communications
						case GB_BARCODE_BOY:
							core_cpu.controllers.serial_io.barcode_boy_process();

							if(core_cpu.controllers.serial_io.barcode_boy.send_data)
							{
								core_mmu.memory_map[REG_SB] = core_cpu.controllers.serial_io.barcode_boy.byte;
								core_mmu.memory_map[IF_FLAG] |= 0x08;
							}
								
							break;

						//Process 4 Player communications
						case GB_FOUR_PLAYER_ADAPTER:
							core_cpu.controllers.serial_io.four_player_process();
							break;
					}

					switch(core_cpu.controllers.serial_io.sio_stat.ir_type)
					{
						//Process Full Changer communications
						case GBC_FULL_CHANGER:
							core_cpu.controllers.serial_io.full_changer_process();
							break;

						//Process Pokemon Pikachu 2 communications
						//Process Pocket Sakura communications
						case GBC_POKEMON_PIKACHU_2:
						case GBC_POCKET_SAKURA:
							core_cpu.controllers.serial_io.pocket_ir_process();
							break;

						//Process TV Remote commnications
						case GBC_TV_REMOTE:
							core_cpu.controllers.serial_io.tv_remote_process();
							break;
					}
				}
			}
		}
	}
}
	
/****** Process hotkey input ******/
void DMG_core::handle_hotkey(SDL_Event& event)
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
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == config::hotkey_turbo)) { config::turbo = true; }

	//Toggle turbo off
	else if((event.type == SDL_KEYUP) && (event.key.keysym.sym == config::hotkey_turbo)) { config::turbo = false; }
		
	//Reset emulation on F8
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F8)) { reset(); }

	//GB Camera load/unload external picture into VRAM
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == config::hotkey_camera))
	{
		//Set data into VRAM
		if((core_mmu.cart.mbc_type == DMG_MMU::GB_CAMERA) && (!core_mmu.cart.cam_lock))
		{
			if(core_mmu.cam_load_snapshot(config::external_camera_file)) { std::cout<<"GBE::Loaded external camera file\n"; }
			core_mmu.cart.cam_lock = true;
		}

		//Clear data from VRAM
		else if((core_mmu.cart.mbc_type == DMG_MMU::GB_CAMERA) && (core_mmu.cart.cam_lock))
		{
			//Clear VRAM - 0x9000 to 0x9800
			for(u32 x = 0x9000; x < 0x9800; x++) { core_mmu.write_u8(x, 0x0); }

			//Clear VRAM - 0x8800 to 0x8900
			for(u32 x = 0x8800; x < 0x8900; x++) { core_mmu.write_u8(x, 0x0); }

			//Clear VRAM - 0x8000 to 0x8500
			for(u32 x = 0x8000; x < 0x8500; x++) { core_mmu.write_u8(x, 0x0); }

			//Clear SRAM
			for(u32 x = 0; x < core_mmu.cart.cam_buffer.size(); x++) { core_mmu.random_access_bank[0][0x100 + x] = 0x0; }
			
			std::cout<<"GBE::Erased external camera file from VRAM\n";

			core_mmu.cart.cam_lock = false;
		}
	}

	//Initiate various communication functions
	//Bardigun + Barcode Boy - Reswipe card
	//Full Changer - Draw Cosmic Character
	//TV Remote - Send signal
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F3))
	{
		switch(core_cpu.controllers.serial_io.sio_stat.sio_type)
		{

			//Bardigun reswipe card
			case GB_BARDIGUN_SCANNER:
				core_cpu.controllers.serial_io.bardigun_scanner.current_state = BARDIGUN_INACTIVE;
				core_cpu.controllers.serial_io.bardigun_scanner.inactive_counter = 0x500;
				core_cpu.controllers.serial_io.bardigun_scanner.barcode_pointer = 0;
				break;

			//Barcode Boy reswipe card
			case GB_BARCODE_BOY:
				if(core_cpu.controllers.serial_io.barcode_boy.current_state == BARCODE_BOY_ACTIVE)
				{
					core_cpu.controllers.serial_io.barcode_boy.current_state = BARCODE_BOY_SEND_BARCODE;
					core_cpu.controllers.serial_io.barcode_boy.send_data = true;

					core_cpu.controllers.serial_io.sio_stat.shifts_left = 8;
					core_cpu.controllers.serial_io.sio_stat.shift_counter = 0;
				}

				break;
		}

		switch(core_cpu.controllers.serial_io.sio_stat.ir_type)
		{
			//Full Changer - Draw Cosmic Character
			case GBC_FULL_CHANGER:
				core_cpu.controllers.serial_io.full_changer.delay_counter = (core_cpu.controllers.serial_io.full_changer.current_character * 72);
				core_mmu.ir_trigger = 1;
				break;

			//Pokemon Pikachu 2 - Send Watts
			//Pocket Sakura - Send Points
			case GBC_POKEMON_PIKACHU_2:
			case GBC_POCKET_SAKURA:
				core_mmu.ir_trigger = 1;
				break;

			//TV Remote - Send signal
			case GBC_TV_REMOTE:
				core_mmu.ir_trigger = 1;
				break;
		}
	}
}

/****** Process hotkey input - Use exsternally when not using SDL ******/
void DMG_core::handle_hotkey(int input, bool pressed)
{
	//Toggle turbo on
	if((input == config::hotkey_turbo) && (pressed)) { config::turbo = true; }

	//Toggle turbo off
	else if((input == config::hotkey_turbo) && (!pressed)) { config::turbo = false; }

	//GB Camera load/unload external picture into VRAM
	else if((input == config::hotkey_camera) && (pressed))
	{
		//Set data into VRAM
		if((core_mmu.cart.mbc_type == DMG_MMU::GB_CAMERA) && (!core_mmu.cart.cam_lock))
		{
			if(core_mmu.cam_load_snapshot(config::external_camera_file)) { std::cout<<"GBE::Loaded external camera file\n"; }
			core_mmu.cart.cam_lock = true;
		}

		//Clear data from VRAM
		else if((core_mmu.cart.mbc_type == DMG_MMU::GB_CAMERA) && (core_mmu.cart.cam_lock))
		{
			//Clear VRAM - 0x9000 to 0x9800
			for(u32 x = 0x9000; x < 0x9800; x++) { core_mmu.write_u8(x, 0x0); }

			//Clear VRAM - 0x8800 to 0x8900
			for(u32 x = 0x8800; x < 0x8900; x++) { core_mmu.write_u8(x, 0x0); }

			//Clear VRAM - 0x8000 to 0x8500
			for(u32 x = 0x8000; x < 0x8500; x++) { core_mmu.write_u8(x, 0x0); }

			//Clear SRAM
			for(u32 x = 0; x < core_mmu.cart.cam_buffer.size(); x++) { core_mmu.random_access_bank[0][0x100 + x] = 0x0; }
			
			std::cout<<"GBE::Erased external camera file from VRAM\n";

			core_mmu.cart.cam_lock = false;
		}
	}

	//Initiate various communication functions
	//Bardigun + Barcode Boy - Reswipe card
	//Full Changer - Draw Cosmic Character
	//TV Remote - Send Signal
	else if((input == SDLK_F3) && (pressed))
	{
		switch(core_cpu.controllers.serial_io.sio_stat.sio_type)
		{

			//Bardigun reswipe card
			case GB_BARDIGUN_SCANNER:
				core_cpu.controllers.serial_io.bardigun_scanner.current_state = BARDIGUN_INACTIVE;
				core_cpu.controllers.serial_io.bardigun_scanner.inactive_counter = 0x500;
				core_cpu.controllers.serial_io.bardigun_scanner.barcode_pointer = 0;
				break;

			//Barcode Boy reswipe card
			case GB_BARCODE_BOY:
				if(core_cpu.controllers.serial_io.barcode_boy.current_state == BARCODE_BOY_ACTIVE)
				{
					core_cpu.controllers.serial_io.barcode_boy.current_state = BARCODE_BOY_SEND_BARCODE;
					core_cpu.controllers.serial_io.barcode_boy.send_data = true;

					core_cpu.controllers.serial_io.sio_stat.shifts_left = 8;
					core_cpu.controllers.serial_io.sio_stat.shift_counter = 0;
				}

				break;
		}

		switch(core_cpu.controllers.serial_io.sio_stat.ir_type)
		{
			//Full Changer draw Cosmic Character
			case GBC_FULL_CHANGER:
				core_cpu.controllers.serial_io.full_changer.delay_counter = (core_cpu.controllers.serial_io.full_changer.current_character * 72);
				core_mmu.ir_trigger = 1;
				break;

			//Pokemon Pikachu 2 - Send Watts
			//Pocket Sakura - Send Points
			case GBC_POKEMON_PIKACHU_2:
			case GBC_POCKET_SAKURA:
				core_mmu.ir_trigger = 1;
				break;

			//TV Remote - Send signal
			case GBC_TV_REMOTE:
				core_mmu.ir_trigger = 1;
				break;
		}
	}
}

/****** Updates the core's volume ******/
void DMG_core::update_volume(u8 volume)
{
	config::volume = volume;
	core_cpu.controllers.audio.apu_stat.channel_master_volume = (config::volume >> 2);
}

/****** Feeds key input from an external source (useful for TAS) ******/
void DMG_core::feed_key_input(int sdl_key, bool pressed)
{
	core_pad.process_keyboard(sdl_key, pressed);
	handle_hotkey(sdl_key, pressed);
}

/****** Return a CPU register ******/
u32 DMG_core::ex_get_reg(u8 reg_index)
{
	switch(reg_index)
	{
		case 0x0: return core_cpu.reg.a;
		case 0x1: return core_cpu.reg.b;
		case 0x2: return core_cpu.reg.c;
		case 0x3: return core_cpu.reg.d;
		case 0x4: return core_cpu.reg.e;
		case 0x5: return core_cpu.reg.h;
		case 0x6: return core_cpu.reg.l;
		case 0x7: return core_cpu.reg.f;
		case 0x8: return core_cpu.reg.sp;
		case 0x9: return core_cpu.reg.pc;
		default: return 0;
	}
}

/****** Read binary file to memory ******/
bool DMG_core::read_file(std::string filename) { return core_mmu.read_file(filename); }

/****** Read BIOS file into memory ******/
bool DMG_core::read_bios(std::string filename) { return core_mmu.read_bios(config::bios_file); }

/****** Read firmware file into memory (not applicable) ******/
bool DMG_core::read_firmware(std::string filename) { return true; }

/****** Returns a byte from core memory ******/
u8 DMG_core::ex_read_u8(u16 address) { return core_mmu.read_u8(address); }

/****** Writes a byte to core memory ******/
void DMG_core::ex_write_u8(u16 address, u8 value) { core_mmu.write_u8(address, value); }

/****** Dumps selected OBJ to a file ******/
void DMG_core::dump_obj(int obj_index)
{
	//DMG OBJs
	if(config::gb_type < 2) { core_cpu.controllers.video.dump_dmg_obj(obj_index); }

	//GBC OBJs
	else { core_cpu.controllers.video.dump_gbc_obj(obj_index); }
}

/****** Dumps selected BG tile to a file ******/
void DMG_core::dump_bg(int bg_index)
{
	//DMG BG tiles
	if(config::gb_type < 2) { core_cpu.controllers.video.dump_dmg_bg(bg_index); }

	//GBC BG tiles
	else{ core_cpu.controllers.video.dump_gbc_bg(bg_index); }
}

/****** Grabs the OBJ palette ******/
u32* DMG_core::get_obj_palette(int pal_index)
{
	return &core_cpu.controllers.video.lcd_stat.obj_colors_final[0][pal_index];
}

/****** Grabs the BG palette ******/
u32* DMG_core::get_bg_palette(int pal_index)
{
	return &core_cpu.controllers.video.lcd_stat.bg_colors_final[0][pal_index];
}

/****** Grabs the hash for a specific tile ******/
std::string DMG_core::get_hash(u32 addr, u8 gfx_type)
{
	return core_cpu.controllers.video.get_hash(addr, gfx_type);
}

/****** Starts netplay connection ******/
void DMG_core::start_netplay()
{
	//Do nothing if netplay is not enabled
	if(!config::use_netplay) { return; }

	//Wait 10 seconds before timing out
	u32 time_out = 0;

	while(time_out < 10000)
	{
		time_out += 100;
		if((time_out % 1000) == 0) { std::cout<<"SIO::Netplay is waiting to establish remote connection...\n"; }

		SDL_Delay(100);

		//Process network connections
		core_cpu.controllers.serial_io.process_network_communication();

		//Check again if the GBE+ instances connected, exit waiting if not the 4-Player adapter
		if((core_cpu.controllers.serial_io.sio_stat.connected) && (core_cpu.controllers.serial_io.sio_stat.sio_type != GB_FOUR_PLAYER_ADAPTER)) { break; }
	}

	if(!core_cpu.controllers.serial_io.sio_stat.connected) { std::cout<<"SIO::No netplay connection established\n"; }
	else { std::cout<<"SIO::Netplay connection established\n"; }
}

/****** Stops netplay connection ******/
void DMG_core::stop_netplay()
{
	//Only attempt to disconnect if connected at all
	if(core_cpu.controllers.serial_io.sio_stat.connected)
	{
		core_cpu.controllers.serial_io.reset();
		std::cout<<"SIO::Netplay connection terminated. Restart to reconnect.\n";
	}
}

/****** Returns miscellaneous data from the core ******/
u32 DMG_core::get_core_data(u32 core_index)
{
	u32 result = 0;

	switch(core_index)
	{
		//Joypad state
		case 0x0:
			result = ~((core_pad.p15 << 4) | core_pad.p14);
			result &= 0xFF;
			break;

		//Load card data
		case 0x1:
			if(core_cpu.controllers.serial_io.sio_stat.sio_type == GB_BARDIGUN_SCANNER)
			{
				bool card_result = core_cpu.controllers.serial_io.bardigun_load_barcode(config::external_card_file);
				result = card_result ? 1 : 0;
			}

			else if(core_cpu.controllers.serial_io.sio_stat.sio_type == GB_BARCODE_BOY)
			{
				bool card_result = core_cpu.controllers.serial_io.barcode_boy_load_barcode(config::external_card_file);
				result = card_result ? 1 : 0;
			}

			break;

		//Invalidate CGFX
		case 0x2:
			core_cpu.controllers.video.invalidate_cgfx();
			result = 1;
			break;
	}

	return result;
}
