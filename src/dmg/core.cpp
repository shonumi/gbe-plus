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

#ifdef GBE_IMAGE_FORMATS
#include <SDL2/SDL_image.h>
#endif

#include "common/util.h"

#include "core.h"

/****** Core Constructor ******/
DMG_core::DMG_core()
{
	//Link CPU and MMU
	core_cpu.mem = &core_mmu;

	//Link LCD and MMU
	core_cpu.controllers.video.mem = &core_mmu;
	core_mmu.set_lcd_data(&core_cpu.controllers.video.lcd_stat);

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
	db_unit.print_pc = false;
	db_unit.last_command = "n";
	db_unit.last_mnemonic = "";
	db_unit.last_pc = 0;
	db_unit.vb_count = 0;

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

	//Check if save state is accessible
	std::ifstream test(state_file.c_str());
	
	if(!test.good())
	{
		config::osd_message = "INVALID SAVE STATE " + util::to_str(slot);
		config::osd_count = 180;
		return;
	}

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
		if(core_cpu.controllers.video.lcd_stat.current_scanline == 144)
		{
			if(SDL_PollEvent(&event))
			{
				//X out of a window
				if(event.type == SDL_QUIT) { stop(); SDL_Quit(); }

				//Process gamepad or hotkey
				else if((event.type == SDL_KEYDOWN) || (event.type == SDL_KEYUP) 
				|| (event.type == SDL_JOYBUTTONDOWN) || (event.type == SDL_JOYBUTTONUP)
				|| (event.type == SDL_JOYAXISMOTION) || (event.type == SDL_JOYHATMOTION)
				|| (event.type == SDL_CONTROLLERSENSORUPDATE))
				{
					core_pad.handle_input(event);
					handle_hotkey(event);

					//Trigger Joypad Interrupt if necessary
					if(core_pad.joypad_irq) { core_mmu.memory_map[IF_FLAG] |= 0x10; }
				}

				//Hotplug joypad
				else if((event.type == SDL_JOYDEVICEADDED) && (!core_pad.joy_init)) { core_pad.init(); }
			}
			
			//Update subscreen if necessary
			if((core_pad.con_update) && (config::sio_device == 14)) { core_cpu.controllers.serial_io.singer_izek_update(); }

			//Perform reset for GB Memory Cartridge
			if((config::cart_type == DMG_GBMEM) && (core_mmu.cart.flash_stat == 0xF0)) { reset(); }

			//Check for new IR connection of HuC carts
			if(core_mmu.ir_stat.try_connection) { core_cpu.controllers.serial_io.process_network_communication(); }
		}

		//Run the CPU
		if(core_cpu.running)
		{
			//Receive byte from another instance of GBE+ via netplay
			if(core_cpu.controllers.serial_io.sio_stat.connected)
			{
				//Perform syncing operations when hard sync is enabled
				if(core_cpu.controllers.serial_io.sio_stat.use_hard_sync)
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
				if(core_mmu.ir_stat.send) { core_cpu.controllers.serial_io.send_ir_signal(); }

				//Receive bytes normally
				core_cpu.controllers.serial_io.receive_byte();

				//Fade IR signal after a certain amount of time
				//End hard sync after a certain amount of time
				if(core_mmu.ir_stat.halt_counter > 0)
				{
					if(core_mmu.ir_stat.fade_counter > 0)
					{
						core_mmu.ir_stat.fade_counter -= core_cpu.cycles;

						if(core_mmu.ir_stat.fade_counter <= 0)
						{
							core_mmu.ir_stat.fade_counter = 0;
							core_mmu.memory_map[REG_RP] &= ~0x2;
						}
					}

					core_mmu.ir_stat.halt_counter -= core_cpu.cycles;

					if(core_mmu.ir_stat.halt_counter <= 0)
					{
						core_mmu.ir_stat.halt_counter = 0;
						core_cpu.controllers.serial_io.stop_sync();
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
				if(core_mmu.div_reset)
				{
					core_mmu.div_reset = false;
					core_cpu.tima_counter = 0;
				}

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

							//Process Power Antenna communications
							case GB_POWER_ANTENNA:
								if(core_cpu.controllers.serial_io.sio_stat.transfer_byte & 0x1)
								{
									core_cpu.controllers.serial_io.power_antenna_on = true;
									core_cpu.controllers.video.power_antenna_osd = true;
									core_mmu.memory_map[REG_SB] = 0xF2;
									core_mmu.memory_map[IF_FLAG] |= 0x08;
								}
								
								else if(core_cpu.controllers.serial_io.sio_stat.transfer_byte == 0)
								{
									core_cpu.controllers.serial_io.power_antenna_on = false;
									core_cpu.controllers.video.power_antenna_osd = false;
									core_mmu.memory_map[REG_SB] = 0xF3;
									core_mmu.memory_map[IF_FLAG] |= 0x08;
								}

								break;

							//Process Singer IZEK communications
							case GB_SINGER_IZEK:
								core_cpu.controllers.serial_io.singer_izek_process();
								break;

							//Process Turbo File GB communications
							case GB_ASCII_TURBO_FILE:
								core_cpu.controllers.serial_io.turbo_file_process();
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

							//Process GB KISS LINK communications
							case GB_KISS_LINK:
								core_mmu.kiss_link.cycles += core_cpu.cycles;
								core_mmu.gb_kiss_link_process();
								break;
						}
					}
				}

				//Process Singer IZEK data communications on external transfers
				if((core_cpu.controllers.serial_io.sio_stat.sio_type == GB_SINGER_IZEK) && (core_cpu.controllers.serial_io.sio_stat.ping_count == 0x02))
				{
					core_cpu.controllers.serial_io.singer_izek_data_process();
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
			if(core_cpu.controllers.serial_io.sio_stat.use_hard_sync)
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
			if(core_mmu.ir_stat.send) { core_cpu.controllers.serial_io.send_ir_signal(); }

			//Receive bytes normally
			core_cpu.controllers.serial_io.receive_byte();

			//Fade IR signal after a certain amount of time
			//End hard sync after a certain amount of time
			if(core_mmu.ir_stat.halt_counter > 0)
			{
				if(core_mmu.ir_stat.fade_counter > 0)
				{
					core_mmu.ir_stat.fade_counter -= core_cpu.cycles;

					if(core_mmu.ir_stat.fade_counter <= 0)
					{
						core_mmu.ir_stat.fade_counter = 0;
						core_mmu.memory_map[REG_RP] &= ~0x2;
					}
				}

				core_mmu.ir_stat.halt_counter -= core_cpu.cycles;

				if(core_mmu.ir_stat.halt_counter <= 0)
				{
					core_mmu.ir_stat.halt_counter = 0;
					core_cpu.controllers.serial_io.stop_sync();
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

						//Process Power Antenna communications
						case GB_POWER_ANTENNA:
							if(core_cpu.controllers.serial_io.sio_stat.transfer_byte & 0x1)
							{
								core_cpu.controllers.serial_io.power_antenna_on = true;
								core_cpu.controllers.video.power_antenna_osd = true;
								core_mmu.memory_map[REG_SB] = 0xF2;
								core_mmu.memory_map[IF_FLAG] |= 0x08;
							}
								
							else if(core_cpu.controllers.serial_io.sio_stat.transfer_byte == 0)
							{
								core_cpu.controllers.serial_io.power_antenna_on = false;
								core_cpu.controllers.video.power_antenna_osd = false;
								core_mmu.memory_map[REG_SB] = 0xF3;
								core_mmu.memory_map[IF_FLAG] |= 0x08;
							}

						//Process Singer IZEK communications
						case GB_SINGER_IZEK:
							core_cpu.controllers.serial_io.singer_izek_process();
							break;

						//Process Turbo File GB communications
						case GB_ASCII_TURBO_FILE:
							core_cpu.controllers.serial_io.turbo_file_process();
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

						//Process GB KISS LINK communications
						case GB_KISS_LINK:
							core_mmu.kiss_link.cycles += core_cpu.cycles;
							core_mmu.gb_kiss_link_process();
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

		#ifdef GBE_IMAGE_FORMATS
		save_name += save_stream.str() + ".png";
		IMG_SavePNG(core_cpu.controllers.video.final_screen, save_name.c_str());
		#endif
		
		#ifndef GBE_IMAGE_FORMATS
		save_name += save_stream.str() + ".bmp";
		SDL_SaveBMP(core_cpu.controllers.video.final_screen, save_name.c_str());
		#endif

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
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F8))
	{
		//If running GB Memory Cartridge, make sure this is a true reset, i.e. boot to the menu program
		if(core_mmu.cart.flash_stat == 0x40)
		{
			core_mmu.cart.flash_stat = 0;
			core_mmu.cart.flash_io_bank = 0;
			config::gb_type = core_mmu.cart.flash_cnt;
		}

		reset();
	}

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
		//HuC-1/HuC-3 - Switch IR connection
		if((config::cart_type == DMG_HUC_IR)
		&& (core_cpu.controllers.serial_io.sio_stat.ir_type != GB_KISS_LINK))
		{
			core_mmu.ir_stat.network_id++;
			core_mmu.ir_stat.network_id &= 0x0F;

			//OSD
			if(core_mmu.ir_stat.network_id != config::netplay_id)
			{
				config::osd_message = "P" + util::to_str(core_mmu.ir_stat.network_id + 1) + " LINKED";
				core_cpu.controllers.serial_io.set_huc_ir_connection();
			}

			else
			{
				config::osd_message = "NO LINK";
			}
			
			config::osd_count = 180;
		}

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
				core_mmu.ir_stat.trigger = 1;
				break;

			//Pokemon Pikachu 2 - Send Watts
			//Pocket Sakura - Send Points
			case GBC_POKEMON_PIKACHU_2:
			case GBC_POCKET_SAKURA:
				core_mmu.ir_stat.trigger = 1;
				break;

			//TV Remote - Send signal
			case GBC_TV_REMOTE:
				core_mmu.ir_stat.trigger = 1;
				break;

			//GB KISS LINK - Start GBF file transfer
			case GB_KISS_LINK:
				core_mmu.kiss_link.cycles = 0;
				core_mmu.kiss_link.input_signals.clear();
				core_mmu.kiss_link.output_signals.clear();
				core_mmu.kiss_link.data.clear();
				core_mmu.kiss_link.stage = GKL_INIT;
				core_mmu.gb_kiss_link_handshake(0xAA);
				break;
		}
	}
}

/****** Process hotkey input - Use externally when not using SDL ******/
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
		//HuC-1/HuC-3 - Switch IR connection
		if((config::cart_type == DMG_HUC_IR)
		&& (core_cpu.controllers.serial_io.sio_stat.ir_type != GB_KISS_LINK))
		{
			core_mmu.ir_stat.network_id++;
			core_mmu.ir_stat.network_id &= 0x0F;

			//OSD
			if(core_mmu.ir_stat.network_id != config::netplay_id)
			{
				config::osd_message = "P" + util::to_str(core_mmu.ir_stat.network_id + 1) + " LINKED";
				core_cpu.controllers.serial_io.set_huc_ir_connection();
			}

			else
			{
				config::osd_message = "NO LINK";
			}
			
			config::osd_count = 180;
		}

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
				core_mmu.ir_stat.trigger = 1;
				break;

			//Pokemon Pikachu 2 - Send Watts
			//Pocket Sakura - Send Points
			case GBC_POKEMON_PIKACHU_2:
			case GBC_POCKET_SAKURA:
				core_mmu.ir_stat.trigger = 1;
				break;

			//TV Remote - Send signal
			case GBC_TV_REMOTE:
				core_mmu.ir_stat.trigger = 1;
				break;

			//GB KISS LINK - Start GBF file transfer
			case GB_KISS_LINK:
				core_mmu.kiss_link.cycles = 0;
				core_mmu.kiss_link.input_signals.clear();
				core_mmu.kiss_link.output_signals.clear();
				core_mmu.kiss_link.data.clear();
				core_mmu.kiss_link.stage = GKL_INIT;
				core_mmu.gb_kiss_link_handshake(0xAA);
				break;
		}
	}

	//Reset emulation on F8
	//Only done when using GB Memory Cartridge via GUI
	else if((input == SDLK_F8) && (pressed) && (config::cart_type == DMG_GBMEM) && (config::use_external_interfaces))
	{
		//If running GB Memory Cartridge, make sure this is a true reset, i.e. boot to the menu program
		if(core_mmu.cart.flash_stat == 0x40)
		{
			core_mmu.cart.flash_stat = 0;
			config::gb_type = core_mmu.cart.flash_cnt;
		}

		reset();
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

/****** Starts netplay connection ******/
void DMG_core::start_netplay()
{
	//Do nothing if netplay is not enabled
	if(!config::use_netplay) { return; }

	#ifdef GBE_NETPLAY

	//Wait 10 seconds before timing out
	u32 time_out = 0;

	if(core_cpu.controllers.serial_io.network_init && !core_cpu.controllers.serial_io.sio_stat.connected
	&& core_cpu.controllers.serial_io.server.connected && core_cpu.controllers.serial_io.sender.connected)
	{
		while(time_out < 10000)
		{
			time_out += 100;
			if((time_out % 1000) == 0) { std::cout<<"SIO::Netplay is waiting to resume remote connection...\n"; }

			SDL_Delay(100);

			//Process network connections
			core_cpu.controllers.serial_io.resume_network_connection();

			//Check again if the GBE+ instances connected, exit waiting if not the 4-Player adapter
			if((core_cpu.controllers.serial_io.sio_stat.connected) && (core_cpu.controllers.serial_io.sio_stat.sio_type != GB_FOUR_PLAYER_ADAPTER))
			{
				std::cout<<"SIO::Netplay connection resumed.\n";
				break;
			}
		}
	}

	if(core_cpu.controllers.serial_io.sio_stat.connected) { return; }

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

	#endif
}

/****** Stops netplay connection ******/
void DMG_core::stop_netplay()
{
	//Only attempt to suspend network connection if connected at all
	if(core_cpu.controllers.serial_io.sio_stat.connected)
	{
		core_cpu.controllers.serial_io.suspend_network_connection();
		std::cout<<"SIO::Netplay connection suspended.\n";
	}
}

/****** Returns miscellaneous data from the core ******/
u32 DMG_core::get_core_data(u32 core_index)
{
	u32 result = 0;

	switch(core_index & 0xFF)
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

		//Grab current scanline pixel
		case 0x3:
			//Use bits 8-15 as index
			result = core_cpu.controllers.video.get_scanline_pixel((core_index >> 8) & 0xFF);
			break;

		//Render DMG BG Scanline
		case 0x4:
			//Use bits 8-15 as index
			core_cpu.controllers.video.render_scanline(((core_index >> 8) & 0xFF), 0);
			result = 1;
			break;

		//Render DMG Window Scanline
		case 0x5:
			//Use bits 8-15 as index
			core_cpu.controllers.video.render_scanline(((core_index >> 8) & 0xFF), 1);
			result = 1;
			break;

		//Render DMG OBJ Scanline
		case 0x6:
			//Use bits 8-15 as index
			core_cpu.controllers.video.render_scanline(((core_index >> 8) & 0xFF), 2);
			result = 1;
			break;

		//Render GBC BG Scanline
		case 0x7:
			//Use bits 8-15 as index
			core_cpu.controllers.video.render_scanline(((core_index >> 8) & 0xFF), 3);
			result = 1;
			break;

		//Render GBC Window Scanline
		case 0x8:
			//Use bits 8-15 as index
			core_cpu.controllers.video.render_scanline(((core_index >> 8) & 0xFF), 4);
			result = 1;
			break;

		//Render GBC OBJ Scanline
		case 0x9:
			//Use bits 8-15 as index
			core_cpu.controllers.video.render_scanline(((core_index >> 8) & 0xFF), 5);
			result = 1;
			break;
	}

	switch(core_index & 0xFF00)
	{
		//Return GBC BG Palette Color
		case 0xFF00:
			//Use Bits 0-7 as index
			{
				u32 y = (core_index & 0xFF) / 8;
				u32 x = (core_index & 0xFF) % 8;

				if(y >= 4) { y = 3; }
				if(x >= 8) { x = 7; }

				result = core_cpu.controllers.video.lcd_stat.bg_colors_final[y][x];
			}
			break;

		//Return GBC OBJ Palette Color
		case 0xFE00:
			//Use Bits 0-7 as index
			{
				u32 y = (core_index & 0xFF) / 8;
				u32 x = (core_index & 0xFF) % 8;

				if(y >= 4) { y = 3; }
				if(x >= 8) { x = 7; }

				result = core_cpu.controllers.video.lcd_stat.obj_colors_final[y][x];
			}
			break;
	}

	return result;
}
