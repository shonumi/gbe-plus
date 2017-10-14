// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : core.cpp
// Date : September 16, 2015
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
NTR_core::NTR_core()
{
	//Link CPUs and MMU
	core_cpu_nds9.mem = &core_mmu;
	core_cpu_nds7.mem = &core_mmu;

	//Link LCD and MMU
	core_cpu_nds9.controllers.video.mem = &core_mmu;
	core_mmu.set_lcd_data(&core_cpu_nds9.controllers.video.lcd_stat);

	//Link APU and MMU
	core_cpu_nds7.controllers.audio.mem = &core_mmu;
	core_mmu.set_apu_data(&core_cpu_nds7.controllers.audio.apu_stat);

	//Link MMU and GamePad
	core_mmu.g_pad = &core_pad;
	core_pad.nds7_input_irq = &core_mmu.nds7_if;
	core_pad.nds9_input_irq = &core_mmu.nds9_if;

	//Link MMU and CPU timers
	core_mmu.nds9_timer = &core_cpu_nds9.controllers.timer;
	core_mmu.nds7_timer = &core_cpu_nds7.controllers.timer;

	db_unit.debug_mode = false;
	//db_unit.display_cycles = false;
	db_unit.last_command = "n";

	db_unit.breakpoints.clear();
	db_unit.watchpoint_addr.clear();
	db_unit.watchpoint_val.clear();

	//Reset CPU sync
	cpu_sync_cycles = 0.0;
	core_cpu_nds9.re_sync = true;
	core_cpu_nds7.re_sync = false;

	nds9_debug = true;

	std::cout<<"GBE::Launching NDS core\n";
}

/****** Start the core ******/
void NTR_core::start()
{
	running = true;
	core_cpu_nds9.running = true;
	core_cpu_nds7.running = true;

	//Initialize video output
	if(!core_cpu_nds9.controllers.video.init())
	{
		running = false;
		core_cpu_nds9.running = false;
	}

	//Initialize audio output
	if(!core_cpu_nds7.controllers.audio.init())
	{
		running = false;
		core_cpu_nds7.running = false;
	}

	//Initialize the GamePad
	core_pad.init();
}

/****** Stop the core ******/
void NTR_core::stop()
{
	running = false;
	core_cpu_nds9.running = false;
	core_cpu_nds7.running = false;
	db_unit.debug_mode = false;
}

/****** Shutdown core's components ******/
void NTR_core::shutdown() 
{ 
	core_mmu.NTR_MMU::~NTR_MMU();
	core_cpu_nds9.NTR_ARM9::~NTR_ARM9();
	core_cpu_nds7.NTR_ARM7::~NTR_ARM7();
}

/****** Reset the core ******/
void NTR_core::reset()
{
	bool can_reset = true;

	core_cpu_nds9.controllers.video.reset();
	core_cpu_nds7.controllers.audio.reset();

	core_cpu_nds9.reset();
	core_cpu_nds7.reset();
	core_mmu.reset();
	
	//Re-read specified ROM file
	if(!core_mmu.read_file(config::rom_file)) { can_reset = false; }

	//Re-read BIOS file
	if((config::use_bios) && (!read_bios(""))) { can_reset = false; }

	//Re-read firmware file
	if((config::use_firmware) && (!read_firmware(config::nds_firmware_path))) { can_reset = false; }

	//Link CPUs and MMU
	core_cpu_nds9.mem = &core_mmu;
	core_cpu_nds7.mem = &core_mmu;

	if(!config::use_bios || !config::use_firmware)
	{
		core_cpu_nds9.reg.r12 = core_cpu_nds9.reg.r14 = core_cpu_nds9.reg.r15 = core_mmu.header.arm9_entry_addr;
		core_cpu_nds7.reg.r12 = core_cpu_nds7.reg.r14 = core_cpu_nds7.reg.r15 = core_mmu.header.arm7_entry_addr;
	}

	//Link LCD and MMU
	core_cpu_nds9.controllers.video.mem = &core_mmu;
	core_mmu.set_lcd_data(&core_cpu_nds9.controllers.video.lcd_stat);

	//Link APU and MMU
	core_cpu_nds7.controllers.audio.mem = &core_mmu;
	core_mmu.set_apu_data(&core_cpu_nds7.controllers.audio.apu_stat);

	//Link MMU and GamePad
	core_mmu.g_pad = &core_pad;
	core_pad.nds7_input_irq = &core_mmu.nds7_if;
	core_pad.nds9_input_irq = &core_mmu.nds9_if;

	//Link MMU and CPU's timers
	core_mmu.nds9_timer = &core_cpu_nds9.controllers.timer;
	core_mmu.nds7_timer = &core_cpu_nds7.controllers.timer;

	//Reset CPU sync
	cpu_sync_cycles = 0.0;
	core_cpu_nds9.re_sync = true;
	core_cpu_nds7.re_sync = false;

	//Start everything all over again
	if(can_reset) { start(); }
	else { running = false; }
}

/****** Loads a save state ******/
void NTR_core::load_state(u8 slot) { }

/****** Saves a save state ******/
void NTR_core::save_state(u8 slot) { }

/****** Run the core in a loop until exit ******/
void NTR_core::run_core()
{
	//Reaneble cursor for this core since it's actually useful for the touchscreen
	SDL_ShowCursor(SDL_ENABLE);

	if(!config::use_bios || !config::use_firmware)
	{
		//Point ARM9 PC to entry address
		core_cpu_nds9.reg.r15 = core_mmu.header.arm9_entry_addr;

		//Point ARM7 PC to entry address
		core_cpu_nds7.reg.r15 = core_mmu.header.arm7_entry_addr;
	}

	//Begin running the core
	while(running)
	{
		//Handle SDL Events
		if((core_cpu_nds9.controllers.video.lcd_stat.current_scanline == 192) && SDL_PollEvent(&event))
		{
			//X out of a window
			if(event.type == SDL_QUIT) { stop(); SDL_Quit(); }

			//Process gamepad, mouse, or hotkey
			else if((event.type == SDL_KEYDOWN) || (event.type == SDL_KEYUP) 
			|| (event.type == SDL_JOYBUTTONDOWN) || (event.type == SDL_JOYBUTTONUP)
			|| (event.type == SDL_JOYAXISMOTION) || (event.type == SDL_JOYHATMOTION)
			|| (event.type == SDL_MOUSEBUTTONDOWN) || (event.type == SDL_MOUSEBUTTONUP)
			|| (event.type == SDL_MOUSEMOTION)) { core_pad.handle_input(event); handle_hotkey(event); }
		}

		//Run the CPU
		if((core_cpu_nds9.running) && (core_cpu_nds7.running))
		{	
			if(db_unit.debug_mode) { debug_step(); }

			//Run NDS9
			if(core_cpu_nds9.re_sync)
			{
				core_cpu_nds9.sync_cycles = 0;

				//Check to see if CPU is paused or idle for any reason
				if(core_cpu_nds9.idle_state)
				{
					core_cpu_nds9.system_cycles += 8;

					switch(core_cpu_nds9.idle_state)
					{
						//Halt SWI
						case 0x1:
							//Match up bits in IE and IF to exit halt
							for(u32 x = 0; x < 21; x++)
							{
								if((core_mmu.nds9_ie & (1 << x)) && (core_mmu.nds9_if & (1 << x)))
								{
									core_cpu_nds9.idle_state = 0;
									core_cpu_nds9.reg.r15 -= (core_cpu_nds9.arm_mode == NTR_ARM9::ARM) ? 4 : 0;
								}
							}

							break;

						//WaitByLoop SWI
						case 0x2:
							core_cpu_nds9.swi_waitbyloop_count--;
							if(core_cpu_nds9.swi_waitbyloop_count & 0x80000000) { core_cpu_nds9.idle_state = 0; }
							break;

						//IntrWait, VBlankIntrWait
						case 0x3:
							//If R0 == 0, quit on any IRQ
							if((core_cpu_nds9.reg.r0 == 0) && (core_mmu.nds9_if)) { core_cpu_nds9.idle_state = 0; }

							//Otherwise, match up bits in IE and IF
							for(int x = 0; x < 21; x++)
							{
								//When there is a match check to see if IntrWait or VBlankIntrWait can quit
								if((core_mmu.nds9_if & (1 << x)) && (core_mmu.nds9_temp_if & (1 << x)))
								{
									core_cpu_nds9.idle_state = 0;
									core_cpu_nds9.reg.r15 -= (core_cpu_nds9.arm_mode == NTR_ARM9::ARM) ? 4 : 0;
								}

								//Execute any other pending IRQs that happen during IntrWait or VBlankIntrWait
								else if((core_mmu.nds9_ie & (1 << x)) && (core_mmu.nds9_if & (1 << x)))
								{
									core_cpu_nds9.idle_state = 0;
									core_cpu_nds9.reg.r15 -= (core_cpu_nds9.arm_mode == NTR_ARM9::ARM) ? 8 : 2;
								}

							}

							//Clear IF flags to wait for new one
							if(core_cpu_nds9.idle_state) { core_mmu.nds9_if &= ~core_mmu.nds9_temp_if; }

							break;
					}

					if(!core_cpu_nds9.idle_state)
					{
						core_cpu_nds9.handle_interrupt();
		
						//Flush pipeline if necessary
						if(core_cpu_nds9.needs_flush) { core_cpu_nds9.flush_pipeline(); }
					}
				}

				//Otherwise, handle normal CPU operations
				else
				{
					core_cpu_nds9.handle_interrupt();

					core_cpu_nds9.fetch();
					core_cpu_nds9.decode();
					core_cpu_nds9.execute();
		
					//Flush pipeline if necessary
					if(core_cpu_nds9.needs_flush)
					{
						core_cpu_nds9.flush_pipeline();
						core_cpu_nds9.last_instr_branch = true;
					}

					//Else update the pipeline and PC
					else
					{ 
						core_cpu_nds9.pipeline_pointer = (core_cpu_nds9.pipeline_pointer + 1) % 3;
						core_cpu_nds9.update_pc();
						core_cpu_nds9.last_instr_branch = false;
					}
				}

				//Clock system components
				core_cpu_nds9.clock_system();

				//Determine if NDS7 needs to run in order to sync
				cpu_sync_cycles -= core_cpu_nds9.sync_cycles;	

				if(cpu_sync_cycles <= 0)
				{
					core_cpu_nds9.re_sync = false;
					core_cpu_nds7.re_sync = true;
					cpu_sync_cycles *= -1.0;
					core_mmu.access_mode = 0;
				}

				core_cpu_nds9.thumb_long_branch = false;
			}

			//Run NDS7
			if(core_cpu_nds7.re_sync)
			{
				core_cpu_nds7.sync_cycles = 0;

				//Check to see if CPU is paused or idle for any reason
				if(core_cpu_nds7.idle_state)
				{
					core_cpu_nds7.system_cycles += 8;

					switch(core_cpu_nds7.idle_state)
					{
						//Halt SWI
						case 0x1:
							//Match up bits in IE and IF to exit halt
							for(u32 x = 0; x < 24; x++)
							{
								if((core_mmu.nds7_ie & (1 << x)) && (core_mmu.nds7_if & (1 << x)))
								{
									core_cpu_nds7.idle_state = 0;
									core_cpu_nds7.reg.r15 -= (core_cpu_nds7.arm_mode == NTR_ARM7::ARM) ? 4 : 0;
								}
							}

							break;

						//WaitByLoop SWI
						case 0x2:
							core_cpu_nds7.swi_waitbyloop_count--;
							if(core_cpu_nds7.swi_waitbyloop_count & 0x80000000) { core_cpu_nds7.idle_state = 0; }
							break;

						//IntrWait, VBlankIntrWait
						case 0x3:
							//Match up bits in IE and IF
							for(int x = 0; x < 24; x++)
							{
								//When there is a match check to see if IntrWait or VBlankIntrWait can quit
								if((core_mmu.nds7_if & (1 << x)) && (core_mmu.nds7_temp_if & (1 << x)))
								{
									core_cpu_nds7.idle_state = 0;
									core_cpu_nds7.reg.r15 -= (core_cpu_nds7.arm_mode == NTR_ARM7::ARM) ? 4 : 0;
								}

								//Execute any other pending IRQs that happen during IntrWait or VBlankIntrWait
								else if((core_mmu.nds7_ie & (1 << x)) && (core_mmu.nds7_if & (1 << x)))
								{
									core_cpu_nds7.idle_state = 0;
									core_cpu_nds7.reg.r15 -= (core_cpu_nds7.arm_mode == NTR_ARM7::ARM) ? 8 : 2;
								}

							}

							//Clear IF flags to wait for new one
							if(core_cpu_nds7.idle_state) { core_mmu.nds7_if &= ~core_mmu.nds7_temp_if; }

							break;
					}

					if(!core_cpu_nds7.idle_state)
					{
						core_cpu_nds7.handle_interrupt();
		
						//Flush pipeline if necessary
						if(core_cpu_nds7.needs_flush) { core_cpu_nds7.flush_pipeline(); }
					}
				}

				//Otherwise, handle normal CPU operations
				else
				{
					core_cpu_nds7.handle_interrupt();

					core_cpu_nds7.fetch();
					core_cpu_nds7.decode();
					core_cpu_nds7.execute();
		
					//Flush pipeline if necessary
					if(core_cpu_nds7.needs_flush)
					{
						core_cpu_nds7.flush_pipeline();
						core_cpu_nds7.last_instr_branch = true;
					}

					//Else update the pipeline and PC
					else
					{ 
						core_cpu_nds7.pipeline_pointer = (core_cpu_nds7.pipeline_pointer + 1) % 3;
						core_cpu_nds7.update_pc();
						core_cpu_nds7.last_instr_branch = false;
					}
				}

				//Clock system components
				core_cpu_nds7.clock_system();

				//Determine if NDS9 needs to run in order to sync
				cpu_sync_cycles -= core_cpu_nds7.sync_cycles;

				if(cpu_sync_cycles <= 0)
				{
					core_cpu_nds7.re_sync = false;
					core_cpu_nds9.re_sync = true;
					cpu_sync_cycles *= -1.0;
					core_mmu.access_mode = 1;
				}

				core_cpu_nds7.thumb_long_branch = false;
			}
		}

		//Stop emulation
		else { stop(); }
	}

	//Shutdown core
	shutdown();
}

/****** Run core for 1 instruction ******/
void NTR_core::step()
{
	//Run the CPU
	if((core_cpu_nds9.running) && (core_cpu_nds7.running))
	{	
		if(db_unit.debug_mode) { debug_step(); }

		//Run NDS9
		if(core_cpu_nds9.re_sync)
		{
			core_cpu_nds9.sync_cycles = 0;

			//Check to see if CPU is paused or idle for any reason
			if(core_cpu_nds9.idle_state)
			{
				core_cpu_nds9.system_cycles += 8;

				switch(core_cpu_nds9.idle_state)
				{
					//Halt SWI
					case 0x1:
						//Match up bits in IE and IF to exit halt
						for(u32 x = 0; x < 21; x++)
						{
							if((core_mmu.nds9_ie & (1 << x)) && (core_mmu.nds9_if & (1 << x))) { core_cpu_nds9.idle_state = 0; }
						}

						break;

					//WaitByLoop SWI
					case 0x2:
						core_cpu_nds9.swi_waitbyloop_count--;
						if(core_cpu_nds9.swi_waitbyloop_count & 0x80000000) { core_cpu_nds9.idle_state = 0; }
						break;

					//IntrWait, VBlankIntrWait
					case 0x3:
						//If R0 == 0, quit on any IRQ
						if((core_cpu_nds9.reg.r0 == 0) && (core_mmu.nds9_if)) { core_cpu_nds9.idle_state = 0; }

						//Otherwise, match up bits in IE and IF
						for(int x = 0; x < 21; x++)
						{
							//When there is a match check to see if IntrWait can quit
							if((core_mmu.nds9_if & (1 << x)) && (core_mmu.nds9_temp_if & (1 << x)))
							{
								core_cpu_nds9.idle_state = 0;
							}

							//Execute any pending IRQs outside of IntrWait or VBlankIntrWait
							if((core_mmu.nds9_ie & (1 << x)) && (core_mmu.nds9_if & (1 << x)))
							{
								core_cpu_nds9.last_idle_state = core_cpu_nds9.idle_state;
								core_cpu_nds9.idle_state = 0;
							}

							//Clear IF flags to wait for new flags if necessary
							if(core_cpu_nds9.idle_state) { core_mmu.nds9_if &= ~core_mmu.nds9_temp_if; }
						}

						break;
				}

				if(!core_cpu_nds9.idle_state)
				{
					core_cpu_nds9.handle_interrupt();
		
					//Flush pipeline if necessary
					if(core_cpu_nds9.needs_flush) { core_cpu_nds9.flush_pipeline(); }
				}
			}

			//Otherwise, handle normal CPU operations
			else
			{
				core_cpu_nds9.fetch();
				core_cpu_nds9.decode();
				core_cpu_nds9.execute();

				core_cpu_nds9.handle_interrupt();
		
				//Flush pipeline if necessary
				if(core_cpu_nds9.needs_flush) { core_cpu_nds9.flush_pipeline(); }

				//Else update the pipeline and PC
				else
				{ 
					core_cpu_nds9.pipeline_pointer = (core_cpu_nds9.pipeline_pointer + 1) % 3;
					core_cpu_nds9.update_pc();
				}
			}

			//Clock system components
			core_cpu_nds9.clock_system();

			//Determine if NDS7 needs to run in order to sync
			cpu_sync_cycles -= core_cpu_nds9.sync_cycles;	

			if(cpu_sync_cycles <= 0)
			{
				core_cpu_nds9.re_sync = false;
				core_cpu_nds7.re_sync = true;
				cpu_sync_cycles *= -1.0;
				core_mmu.access_mode = 0;
			}
		}

		//Run NDS7
		if(core_cpu_nds7.re_sync)
		{
			core_cpu_nds7.sync_cycles = 0;

			//Check to see if CPU is paused or idle for any reason
			if(core_cpu_nds7.idle_state)
			{
				core_cpu_nds7.system_cycles += 8;

				switch(core_cpu_nds7.idle_state)
				{
					//Halt SWI
					case 0x1:
						//Match up bits in IE and IF to exit halt
						for(u32 x = 0; x < 24; x++)
						{
							if((core_mmu.nds7_ie & (1 << x)) && (core_mmu.nds7_if & (1 << x))) { core_cpu_nds7.idle_state = 0; }
						}

						break;

					//WaitByLoop SWI
					case 0x2:
						core_cpu_nds7.swi_waitbyloop_count--;
						if(core_cpu_nds7.swi_waitbyloop_count & 0x80000000) { core_cpu_nds7.idle_state = 0; }
						break;

					//IntrWait, VBlankIntrWait
					case 0x3:
						//Match up bits in IE and IF
						for(int x = 0; x < 24; x++)
						{
							//When there is a match check to see if IntrWait can quit
							if((core_mmu.nds7_if & (1 << x)) && (core_mmu.nds7_temp_if & (1 << x)))
							{
								core_cpu_nds7.idle_state = 0;
							}

							//Execute any pending IRQs outside of IntrWait or VBlankIntrWait
							if((core_mmu.nds7_ie & (1 << x)) && (core_mmu.nds7_if & (1 << x)))
							{
								core_cpu_nds7.last_idle_state = core_cpu_nds7.idle_state;
								core_cpu_nds7.idle_state = 0;
							}

							//Clear IF flags to wait for new flags if necessary
							if(core_cpu_nds7.idle_state) { core_mmu.nds7_if &= ~core_mmu.nds7_temp_if; }
						}

						break;
				}

				if(!core_cpu_nds7.idle_state)
				{
					core_cpu_nds7.handle_interrupt();
		
					//Flush pipeline if necessary
					if(core_cpu_nds7.needs_flush) { core_cpu_nds7.flush_pipeline(); }
				}
			}

			//Otherwise, handle normal CPU operations
			else
			{
				core_cpu_nds7.fetch();
				core_cpu_nds7.decode();
				core_cpu_nds7.execute();

				core_cpu_nds7.handle_interrupt();
		
				//Flush pipeline if necessary
				if(core_cpu_nds7.needs_flush) { core_cpu_nds7.flush_pipeline(); }

				//Else update the pipeline and PC
				else
				{ 
					core_cpu_nds7.pipeline_pointer = (core_cpu_nds7.pipeline_pointer + 1) % 3;
					core_cpu_nds7.update_pc();
				}
			}

			//Clock system components
			core_cpu_nds7.clock_system();

			//Determine if NDS9 needs to run in order to sync
			cpu_sync_cycles -= core_cpu_nds7.sync_cycles;

			if(cpu_sync_cycles <= 0)
			{
				core_cpu_nds7.re_sync = false;
				core_cpu_nds9.re_sync = true;
				cpu_sync_cycles *= -1.0;
				core_mmu.access_mode = 1;
			}
		}
	}
}

/****** Debugger - Allow core to run until a breaking condition occurs ******/
void NTR_core::debug_step()
{
	bool printed = false;

	//Select NDS9 or NDS7 PC when looking for a break condition
	u32 pc = nds9_debug ? core_cpu_nds9.reg.r15 : core_cpu_nds7.reg.r15;

	//In continue mode, if breakpoints exist, try to stop on one
	if((db_unit.breakpoints.size() > 0) && (db_unit.last_command == "c"))
	{
		for(int x = 0; x < db_unit.breakpoints.size(); x++)
		{
			//When a BP is matched, display info, wait for next input command
			if(pc == db_unit.breakpoints[x])
			{
				debug_display();
				debug_process_command();
				printed = true;
			}
		}

	}

	//In continue mode, if a watch point is triggered, try to stop on one
	else if((db_unit.watchpoint_addr.size() > 0) && (db_unit.last_command == "c"))
	{
		for(int x = 0; x < db_unit.watchpoint_addr.size(); x++)
		{
			//When a watchpoint is triggered, display info, wait for next input command
			if((core_mmu.read_u8(db_unit.watchpoint_addr[x]) == db_unit.watchpoint_val[x])
			&& (core_mmu.read_u8(db_unit.watchpoint_addr[x]) != db_unit.watchpoint_old_val[x]))
			{
				std::cout<<"Watchpoint Triggered: 0x" << std::hex << db_unit.watchpoint_addr[x] << " -- Value: 0x" << (u16)db_unit.watchpoint_val[x] << "\n";
	
				debug_display();
				debug_process_command();
				printed = true;
			}

			db_unit.watchpoint_old_val[x] = core_mmu.read_u8(db_unit.watchpoint_addr[x]);
		}
	}

	//When in next instruction mode, simply display info, wait for next input command
	else if(db_unit.last_command == "n")
	{
		debug_display();
		debug_process_command();
		printed = true;
	}

	//Display every instruction when print all is enabled
	if((!printed) && (db_unit.print_all)) { debug_display(); } 
}

/****** Debugger - Display relevant info to the screen ******/
void NTR_core::debug_display() const
{
	//Select NDS9 or NDS7 debugging info
	u32 debug_code = nds9_debug ? core_cpu_nds9.debug_code : core_cpu_nds7.debug_code;
	u8 debug_message = nds9_debug ? core_cpu_nds9.debug_message : core_cpu_nds7.debug_message;
	u32 cpu_regs[16];
	u32 cpsr = nds9_debug ? core_cpu_nds9.reg.cpsr : core_cpu_nds7.reg.cpsr;

	for(u32 x = 0; x < 16; x++)
	{
		cpu_regs[x] = nds9_debug ? core_cpu_nds9.get_reg(x) : core_cpu_nds7.get_reg(x);
	}

	//Display current CPU action
	switch(debug_message)
	{
		case 0xFF:
			std::cout << "Filling pipeline\n"; break;
		case 0x0: 
			std::cout << std::hex << "CPU::Executing THUMB_1 : 0x" << debug_code << "\n\n"; break;
		case 0x1:
			std::cout << std::hex << "CPU::Executing THUMB_2 : 0x" << debug_code << "\n\n"; break;
		case 0x2:
			std::cout << std::hex << "CPU::Executing THUMB_3 : 0x" << debug_code << "\n\n"; break;
		case 0x3:
			std::cout << std::hex << "CPU::Executing THUMB_4 : 0x" << debug_code << "\n\n"; break;
		case 0x4:
			std::cout << std::hex << "CPU::Executing THUMB_5 : 0x" << debug_code << "\n\n"; break;
		case 0x5:
			std::cout << std::hex << "CPU::Executing THUMB_6 : 0x" << debug_code << "\n\n"; break;
		case 0x6:
			std::cout << std::hex << "CPU::Executing THUMB_7 : 0x" << debug_code << "\n\n"; break;
		case 0x7:
			std::cout << std::hex << "CPU::Executing THUMB_8 : 0x" << debug_code << "\n\n"; break;
		case 0x8:
			std::cout << std::hex << "CPU::Executing THUMB_9 : 0x" << debug_code << "\n\n"; break;
		case 0x9:
			std::cout << std::hex << "CPU::Executing THUMB_10 : 0x" << debug_code << "\n\n"; break;
		case 0xA:
			std::cout << std::hex << "CPU::Executing THUMB_11 : 0x" << debug_code << "\n\n"; break;
		case 0xB:
			std::cout << std::hex << "CPU::Executing THUMB_12 : 0x" << debug_code << "\n\n"; break;
		case 0xC:
			std::cout << std::hex << "CPU::Executing THUMB_13 : 0x" << debug_code << "\n\n"; break;
		case 0xD:
			std::cout << std::hex << "CPU::Executing THUMB_14 : 0x" << debug_code << "\n\n"; break;
		case 0xE:
			std::cout << std::hex << "CPU::Executing THUMB_15 : 0x" << debug_code << "\n\n"; break;
		case 0xF:
			std::cout << std::hex << "CPU::Executing THUMB_16 : 0x" << debug_code << "\n\n"; break;
		case 0x10:
			std::cout << std::hex << "CPU::Executing THUMB_17 : 0x" << debug_code << "\n\n"; break;
		case 0x11:
			std::cout << std::hex << "CPU::Executing THUMB_18 : 0x" << debug_code << "\n\n"; break;
		case 0x12:
			std::cout << std::hex << "CPU::Executing THUMB_19 : 0x" << debug_code << "\n\n"; break;
		case 0x13:
			std::cout << std::hex << "Unknown THUMB Instruction : 0x" << debug_code << "\n\n"; break;
		case 0x14:
			std::cout << std::hex << "CPU::Executing ARM_3 : 0x" << debug_code << "\n\n"; break;
		case 0x15:
			std::cout << std::hex << "CPU::Executing ARM_4 : 0x" << debug_code << "\n\n"; break;
		case 0x16:
			std::cout << std::hex << "CPU::Executing ARM_5 : 0x" << debug_code << "\n\n"; break;
		case 0x17:
			std::cout << std::hex << "CPU::Executing ARM_6 : 0x" << debug_code << "\n\n"; break;
		case 0x18:
			std::cout << std::hex << "CPU::Executing ARM_7 : 0x" << debug_code << "\n\n"; break;
		case 0x19:
			std::cout << std::hex << "CPU::Executing ARM_9 : 0x" << debug_code << "\n\n"; break;
		case 0x1A:
			std::cout << std::hex << "CPU::Executing ARM_10 : 0x" << debug_code << "\n\n"; break;
		case 0x1B:
			std::cout << std::hex << "CPU::Executing ARM_11 : 0x" << debug_code << "\n\n"; break;
		case 0x1C:
			std::cout << std::hex << "CPU::Executing ARM_12 : 0x" << debug_code << "\n\n"; break;
		case 0x1D:
			std::cout << std::hex << "CPU::Executing ARM_13 : 0x" << debug_code << "\n\n"; break;
		case 0x1E:
			std::cout << std::hex << "CPU::Executing ARM Coprocessor Register Transfer : 0x" << debug_code << "\n\n"; break;
		case 0x1F:
			std::cout << std::hex << "CPU::Executing ARM Coprocessor Data Transfer : 0x" << debug_code << "\n\n"; break;
		case 0x20:
			std::cout << std::hex << "CPU::Executing ARM Coprocessor Data Operation : 0x" << debug_code << "\n\n"; break;
		case 0x21:
			std::cout << std::hex << "CPU::Executing ARM CLZ : 0x" << debug_code << "\n\n"; break;
		case 0x22:
			std::cout << std::hex << "CPU::Executing ARM QADD-QSUB : 0x" << debug_code << "\n\n"; break;
		case 0x23:
			std::cout << std::hex << "Unknown ARM Instruction : 0x" << debug_code << "\n\n"; break;
		case 0x24:
			std::cout << std::hex << "CPU::Skipping ARM Instruction : 0x" << debug_code << "\n\n"; break;
	}

	//Display CPU registers
	std::cout<< std::hex <<"R0 : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[0] << 
		" -- R4  : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[4] << 
		" -- R8  : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[8] << 
		" -- R12 : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[12] << "\n";

	std::cout<< std::hex <<"R1 : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[1] << 
		" -- R5  : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[5] << 
		" -- R9  : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[9] << 
		" -- R13 : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[13] << "\n";

	std::cout<< std::hex <<"R2 : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[2] << 
		" -- R6  : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[6] << 
		" -- R10 : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[10] << 
		" -- R14 : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[14] << "\n";

	std::cout<< std::hex <<"R3 : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[3] << 
		" -- R7  : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[7] << 
		" -- R11 : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[11] << 
		" -- R15 : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[15] << "\n";

	//Grab CPSR Flags and status
	std::string cpsr_stats = "(";

	if(cpsr & CPSR_N_FLAG) { cpsr_stats += "N"; }
	else { cpsr_stats += "."; }

	if(cpsr & CPSR_Z_FLAG) { cpsr_stats += "Z"; }
	else { cpsr_stats += "."; }

	if(cpsr & CPSR_C_FLAG) { cpsr_stats += "C"; }
	else { cpsr_stats += "."; }

	if(cpsr & CPSR_V_FLAG) { cpsr_stats += "V"; }
	else { cpsr_stats += "."; }

	cpsr_stats += "  ";

	if(cpsr & CPSR_IRQ) { cpsr_stats += "I"; }
	else { cpsr_stats += "."; }

	if(cpsr & CPSR_FIQ) { cpsr_stats += "F"; }
	else { cpsr_stats += "."; }

	if(cpsr & CPSR_STATE) { cpsr_stats += "T)"; }
	else { cpsr_stats += ".)"; }

	std::cout<< std::hex <<"CPSR : 0x" << std::setw(8) << std::setfill('0') << cpsr << "\t" << cpsr_stats << "\n";

	/*
	//Display current CPU cycles
	if(db_unit.display_cycles) { std::cout<<"Current CPU cycles : " << std::dec << core_cpu_nds9.debug_cycles << "\n"; }
	*/
}

/****** Returns a string with the mnemonic assembly instruction ******/
std::string NTR_core::debug_get_mnemonic(u32 addr) { }

/****** Debugger - Wait for user input, process it to decide what next to do ******/
void NTR_core::debug_process_command()
{
	std::string command = "";
	std::cout<< ": ";
	std::getline(std::cin, command);

	bool valid_command = false;

	while(!valid_command)
	{
		//Proceed with next instruction
		//Nothing really needs to be done really, so make sure to print a new-line
		if(command == "n") { valid_command = true; std::cout<<"\n"; db_unit.last_command = "n"; }

		//Continue until breaking condition occurs
		else if(command == "c") { valid_command = true; std::cout<<"\n"; db_unit.last_command = "c"; }

		//Quit the emulator
		else if(command == "q") { valid_command = true; stop(); std::cout<<"\n"; }

		//Quit the debugger
		else if(command == "dq") { valid_command = true; db_unit.debug_mode = false; std::cout<<"\n"; }

		//Switch between NDS9 and NDS7 debugging
		else if(command == "sc")
		{
			valid_command = true;
			db_unit.last_command = "sc";

			if(nds9_debug)
			{
				nds9_debug = false;
				std::cout<<"Switching to NDS7...\n";
			}

			else
			{
				nds9_debug = true;
				std::cout<<"Switching to NDS9...\n";
			}

			debug_display();
			debug_process_command();
		}

		//Add breakpoint
		else if((command.substr(0, 2) == "bp") && (command.substr(3, 2) == "0x"))
		{
			valid_command = true;
			u32 bp = 0;
			std::string hex_string = command.substr(5);

			//Convert hex string into usable u32
			valid_command = util::from_hex_str(hex_string, bp);
		
			//Request valid input again
			if(!valid_command)
			{
				std::cout<<"\nInvalid memory address : " << command << "\n";
				std::cout<<": ";
				std::getline(std::cin, command);
			}

			else 
			{
				db_unit.breakpoints.push_back(bp);
				db_unit.last_command = "bp";
				std::cout<<"\nBreakpoint added at 0x" << std::hex << bp << "\n";
				debug_process_command();
			}
		}

		//Show memory - byte
		else if((command.substr(0, 2) == "u8") && (command.substr(3, 2) == "0x"))
		{
			valid_command = true;
			u32 mem_location = 0;
			std::string hex_string = command.substr(5);

			//Convert hex string into usable u32
			valid_command = util::from_hex_str(hex_string, mem_location);

			//Request valid input again
			if(!valid_command)
			{
				std::cout<<"\nInvalid memory address : " << command << "\n";
				std::cout<<": ";
				std::getline(std::cin, command);
			}

			else
			{
				u8 last_access = core_mmu.access_mode;
				core_mmu.access_mode = nds9_debug;

				db_unit.last_command = "u8";
				std::cout<<"Memory @ " << hex_string << " : 0x" << std::hex << (int)core_mmu.read_u8(mem_location) << "\n";
				debug_process_command();

				core_mmu.access_mode = last_access;
			}
		}

		//Show memory - halfword
		else if((command.substr(0, 3) == "u16") && (command.substr(4, 2) == "0x"))
		{
			valid_command = true;
			u32 mem_location = 0;
			std::string hex_string = command.substr(6);

			//Convert hex string into usable u32
			valid_command = util::from_hex_str(hex_string, mem_location);

			//Request valid input again
			if(!valid_command)
			{
				std::cout<<"\nInvalid memory address : " << command << "\n";
				std::cout<<": ";
				std::getline(std::cin, command);
			}

			else
			{
				u8 last_access = core_mmu.access_mode;
				core_mmu.access_mode = nds9_debug;

				db_unit.last_command = "u16";
				std::cout<<"Memory @ " << hex_string << " : 0x" << std::hex << (int)core_mmu.read_u16(mem_location) << "\n";
				debug_process_command();

				core_mmu.access_mode = last_access;
			}
		}

		//Show memory - word
		else if((command.substr(0, 3) == "u32") && (command.substr(4, 2) == "0x"))
		{
			valid_command = true;
			u32 mem_location = 0;
			std::string hex_string = command.substr(6);

			//Convert hex string into usable u32
			valid_command = util::from_hex_str(hex_string, mem_location);

			//Request valid input again
			if(!valid_command)
			{
				std::cout<<"\nInvalid memory address : " << command << "\n";
				std::cout<<": ";
				std::getline(std::cin, command);
			}

			else
			{
				u8 last_access = core_mmu.access_mode;
				core_mmu.access_mode = nds9_debug;

				db_unit.last_command = "u32";
				std::cout<<"Memory @ " << hex_string << " : 0x" << std::hex << (int)core_mmu.read_u32(mem_location) << "\n";
				debug_process_command();

				core_mmu.access_mode = last_access;
			}
		}

		//Write memory - 1 byte
		else if((command.substr(0, 2) == "w8") && (command.substr(3, 2) == "0x"))
		{
			valid_command = true;
			bool valid_value = false;
			u32 mem_location = 0;
			u32 mem_value = 0;
			std::string hex_string = command.substr(5);

			//Convert hex string into usable u32
			valid_command = util::from_hex_str(hex_string, mem_location);

			//Request valid input again
			if(!valid_command)
			{
				std::cout<<"\nInvalid memory address : " << command << "\n";
				std::cout<<": ";
				std::getline(std::cin, command);
			}

			else
			{
				//Request value
				while(!valid_value)
				{
					std::cout<<"\nInput value: ";
					std::getline(std::cin, command);
				
					valid_value = util::from_hex_str(command.substr(2), mem_value);
				
					if(!valid_value)
					{
						std::cout<<"\nInvalid value : " << command << "\n";
					}

					else if((valid_value) && (mem_value > 0xFF))
					{
						std::cout<<"\nValue is too large (greater than 0xFF) : 0x" << std::hex << mem_value;
						valid_value = false;
					}
				}

				std::cout<<"\n";

				u8 last_access = core_mmu.access_mode;
				core_mmu.access_mode = nds9_debug;

				db_unit.last_command = "w8";
				core_mmu.write_u8(mem_location, mem_value);
				debug_process_command();

				core_mmu.access_mode = last_access;
			}
		}

		//Write memory - 2 bytes
		else if((command.substr(0, 3) == "w16") && (command.substr(4, 2) == "0x"))
		{
			valid_command = true;
			bool valid_value = false;
			u32 mem_location = 0;
			u32 mem_value = 0;
			std::string hex_string = command.substr(6);

			//Convert hex string into usable u32
			valid_command = util::from_hex_str(hex_string, mem_location);

			//Request valid input again
			if(!valid_command)
			{
				std::cout<<"\nInvalid memory address : " << command << "\n";
				std::cout<<": ";
				std::getline(std::cin, command);
			}

			else
			{
				//Request value
				while(!valid_value)
				{
					std::cout<<"\nInput value: ";
					std::getline(std::cin, command);
				
					valid_value = util::from_hex_str(command.substr(2), mem_value);
				
					if(!valid_value)
					{
						std::cout<<"\nInvalid value : " << command << "\n";
					}

					else if((valid_value) && (mem_value > 0xFFFF))
					{
						std::cout<<"\nValue is too large (greater than 0xFFFF) : 0x" << std::hex << mem_value;
						valid_value = false;
					}
				}

				std::cout<<"\n";

				u8 last_access = core_mmu.access_mode;
				core_mmu.access_mode = nds9_debug;

				db_unit.last_command = "w16";
				core_mmu.write_u16(mem_location, mem_value);
				debug_process_command();

				core_mmu.access_mode = last_access;
			}
		}

		//Write memory - 4 bytes
		else if((command.substr(0, 3) == "w32") && (command.substr(4, 2) == "0x"))
		{
			valid_command = true;
			bool valid_value = false;
			u32 mem_location = 0;
			u32 mem_value = 0;
			std::string hex_string = command.substr(6);

			//Convert hex string into usable u32
			valid_command = util::from_hex_str(hex_string, mem_location);

			//Request valid input again
			if(!valid_command)
			{
				std::cout<<"\nInvalid memory address : " << command << "\n";
				std::cout<<": ";
				std::getline(std::cin, command);
			}

			else
			{
				//Request value
				while(!valid_value)
				{
					std::cout<<"\nInput value: ";
					std::getline(std::cin, command);
				
					valid_value = util::from_hex_str(command.substr(2), mem_value);
				
					if(!valid_value)
					{
						std::cout<<"\nInvalid value : " << command << "\n";
					}
				}

				std::cout<<"\n";

				u8 last_access = core_mmu.access_mode;
				core_mmu.access_mode = nds9_debug;

				db_unit.last_command = "w32";
				core_mmu.write_u32(mem_location, mem_value);
				debug_process_command();

				core_mmu.access_mode = last_access;
			}
		}

		//Break on memory change
		else if((command.substr(0, 2) == "bc") && (command.substr(3, 2) == "0x"))
		{
			valid_command = true;
			bool valid_value = false;
			u32 mem_location = 0;
			u32 mem_value = 0;
			std::string hex_string = command.substr(5);

			//Convert hex string into usable u32
			valid_command = util::from_hex_str(hex_string, mem_location);

			//Request valid input again
			if(!valid_command)
			{
				std::cout<<"\nInvalid memory address : " << command << "\n";
				std::cout<<": ";
				std::getline(std::cin, command);
			}

			else
			{
				//Request value
				while(!valid_value)
				{
					std::cout<<"\nWatch value: ";
					std::getline(std::cin, command);
				
					valid_value = util::from_hex_str(command.substr(2), mem_value);
				
					if(!valid_value)
					{
						std::cout<<"\nInvalid value : " << command << "\n";
					}

					else if((valid_value) && (mem_value > 0xFF))
					{
						std::cout<<"\nValue is too large (greater than 0xFF) : 0x" << std::hex << mem_value;
						valid_value = false;
					}
				}

				std::cout<<"\n";

				db_unit.last_command = "bc";
				db_unit.watchpoint_addr.push_back(mem_location);
				db_unit.watchpoint_val.push_back(mem_value);
				db_unit.watchpoint_old_val.push_back(core_mmu.read_u8(mem_location));
				debug_process_command();
			}
		}

		/*
		//Toggle display of CPU cycles
		else if(command == "dc")
		{
			if(db_unit.display_cycles) 
			{
				std::cout<<"\nDisplay CPU cycles disabled\n";
				db_unit.display_cycles = false;
			}
		
			else
			{
				std::cout<<"\nDisplay CPU cycles enabled\n";
				db_unit.display_cycles = true;
			}

			valid_command = true;
			db_unit.last_command = "dc";
			debug_process_command();
		}

		//Reset CPU cycles counter
		else if(command == "cr")
		{
			std::cout<<"\nCPU cycle counter reset to 0\n";

			valid_command = true;
			core_cpu.debug_cycles = 0;
			debug_process_command();
		}
		*/

		//Reset the emulator
		else if(command == "rs")
		{
			std::cout<<"\nManual reset\n";
			reset();
			debug_display();

			valid_command = true;
			db_unit.last_command = "rs";
			debug_process_command();
		}

		//Print all instructions to the screen
		else if(command == "pa")
		{
			if(db_unit.print_all)
			{
				std::cout<<"\nPrint-All turned off\n";
				db_unit.print_all = false;
			}

			else
			{
				std::cout<<"\nPrint-All turned on\n";
				db_unit.print_all = true;
			}

			valid_command = true;
			db_unit.last_command = "pa";
			debug_process_command();
		}

		//Print help information
		else if(command == "h")
		{
			std::cout<<"n \t\t Run next Fetch-Decode-Execute stage\n";
			std::cout<<"c \t\t Continue until next breakpoint\n";
			std::cout<<"sc \t\t Switch CPU (NDS9 or NDS7)\n";
			std::cout<<"bp \t\t Set breakpoint, format 0x1234ABCD\n";
			std::cout<<"bc \t\t Set breakpoint on memory change, format 0x1234ABCD for addr, 0x12 for value\n";
			std::cout<<"u8 \t\t Show BYTE @ memory, format 0x1234ABCD\n";
			std::cout<<"u16 \t\t Show HALFWORD @ memory, format 0x1234ABCD\n";
			std::cout<<"u32 \t\t Show WORD @ memory, format 0x1234ABCD\n";
			std::cout<<"dq \t\t Quit the debugger\n";
			std::cout<<"dc \t\t Toggle CPU cycle display\n";
			std::cout<<"cr \t\t Reset CPU cycle counter\n";
			std::cout<<"rs \t\t Reset emulation\n";
			std::cout<<"q \t\t Quit GBE+\n\n";

			valid_command = true;
			db_unit.last_command = "h";
			debug_process_command();
		}	

		//Request valid input again
		else if(!valid_command)
		{
			std::cout<<"\nInvalid input : " << command << " - Type h for help\n";
			std::cout<<": ";
			std::getline(std::cin, command);
		}
	}
}
	
/****** Process hotkey input ******/
void NTR_core::handle_hotkey(SDL_Event& event)
{
	//Quit on Q or ESC
	if((event.type == SDL_KEYDOWN) && ((event.key.keysym.sym == SDLK_q) || (event.key.keysym.sym == SDLK_ESCAPE)))
	{
		running = false; 
		SDL_Quit();
	}

	//Screenshot on F9
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F9)) 
	{
		std::stringstream save_stream;
		std::string save_name = "";

		//Prefix SDL Ticks to screenshot name
		save_stream << SDL_GetTicks();
		save_name += save_stream.str();
		save_stream.str(std::string());

		//Append random number to screenshot name
		srand(SDL_GetTicks());
		save_stream << rand() % 1024 << rand() % 1024 << rand() % 1024;
		save_name += save_stream.str() + ".bmp";
	
		SDL_SaveBMP(core_cpu_nds9.controllers.video.final_screen, save_name.c_str());
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

	/*
	//Toggle Fullscreen on F12
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F12))
	{
		//Switch flags
		if(config::flags == 0x80000000) { config::flags = 0; }
		else { config::flags = 0x80000000; }

		//Initialize the screen
		if(!config::use_opengl)
		{
			ccore_cpu_nds9.controllers.video.final_screen = SDL_SetVideoMode(256, 384, 32, SDL_SWSURFACE | config::flags);
		}

		else
		{
			core_cpu.controllers.video.opengl_init();
		}
	}
	*/

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
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_TAB)) { config::turbo = true; }

	//Toggle turbo off
	else if((event.type == SDL_KEYUP) && (event.key.keysym.sym == SDLK_TAB)) { config::turbo = false; }
		
	//Reset emulation on F8
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F8)) { reset(); }
}

/****** Process hotkey input - Use exsternally when not using SDL ******/
void NTR_core::handle_hotkey(int input, bool pressed)
{
	//Toggle turbo on
	if((input == config::hotkey_turbo) && (pressed)) { config::turbo = true; }

	//Toggle turbo off
	else if((input == config::hotkey_turbo) && (!pressed)) { config::turbo = false; }
}

/****** Updates the core's volume ******/
void NTR_core::update_volume(u8 volume) { }

/****** Feeds key input from an external source (useful for TAS) ******/
void NTR_core::feed_key_input(int sdl_key, bool pressed)
{
	//Process normal events
	if((sdl_key & 0x70000) == 0)
	{ 
		core_pad.process_keyboard(sdl_key, pressed);
		handle_hotkey(sdl_key, pressed);
	}

	//Process mouse events
	else { core_pad.process_mouse(sdl_key, pressed); }
}

/****** Return a CPU register ******/
u32 NTR_core::ex_get_reg(u8 reg_index) { }

/****** Read binary file to memory ******/
bool NTR_core::read_file(std::string filename) { return core_mmu.read_file(filename); }

/****** Read BIOS file into memory ******/
bool NTR_core::read_bios(std::string filename)
{
	//Note, the string given into NTR_core::read_bios is a dummy
	if(!core_mmu.read_bios_nds7(config::nds7_bios_path) || !core_mmu.read_bios_nds9(config::nds9_bios_path))
	{
		return false;
	}
}

/****** Read firmware file into memory ******/
bool NTR_core::read_firmware(std::string filename)
{
	return core_mmu.read_firmware(filename);
}

/****** Returns a byte from core memory ******/
u8 NTR_core::ex_read_u8(u16 address) { return core_mmu.read_u8(address); }

/****** Writes a byte to core memory ******/
void NTR_core::ex_write_u8(u16 address, u8 value) { core_mmu.write_u8(address, value); }

/****** Dumps selected OBJ to a file ******/
void NTR_core::dump_obj(int obj_index) { }

/****** Dumps selected BG tile to a file ******/
void NTR_core::dump_bg(int bg_index) { }

/****** Grabs the OBJ palette ******/
u32* NTR_core::get_obj_palette(int pal_index) { }

/****** Grabs the BG palette ******/
u32* NTR_core::get_bg_palette(int pal_index) { }

/****** Grabs the hash for a specific tile ******/
std::string NTR_core::get_hash(u32 addr, u8 gfx_type) { }

/****** Starts netplay connection ******/
void NTR_core::start_netplay() { }

/****** Stops netplay connection ******/
void NTR_core::stop_netplay() { }

/****** Returns miscellaneous data from the core ******/
u32 NTR_core::get_core_data(u32 core_index)
{
	u32 result = 0;

	switch(core_index)
	{
		//Joypad state
		case 0x0:
			result = ~(core_pad.key_input);
			result &= 0x3FF;
			break;

		//Ext Input state
		case 0x1:
			result = ~(core_pad.ext_key_input);
			result &= 0x7F;
			break;

		//Touch by mouse
		case 0x2:
			result = (core_pad.touch_by_mouse) ? 1 : 0;
			break;
	}

	return result;
}
