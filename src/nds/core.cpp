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

#ifdef GBE_IMAGE_FORMATS
#include <SDL_image.h>
#endif

#include "common/util.h"

#include "core.h"

/****** Core Constructor ******/
NTR_core::NTR_core()
{
	//Link CPUs and MMU
	core_cpu_nds9.mem = &core_mmu;
	core_cpu_nds7.mem = &core_mmu;

	core_mmu.set_nds7_pc(&core_cpu_nds7.reg.r15);
	core_mmu.set_nds9_pc(&core_cpu_nds9.reg.r15);

	//Link LCD and MMU
	core_cpu_nds9.controllers.video.mem = &core_mmu;
	core_mmu.set_lcd_data(&core_cpu_nds9.controllers.video.lcd_stat);
	core_mmu.set_lcd_3D_data(&core_cpu_nds9.controllers.video.lcd_3D_stat);

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

	//Reset CPU sync
	cpu_sync_cycles = 0.0;
	core_cpu_nds9.re_sync = true;
	core_cpu_nds7.re_sync = false;

	nds9_debug = true;
	arm_debug = true;

	//Load Virtual Cursor
	if(config::vc_enable) { load_virtual_cursor(); }

	std::cout<<"GBE::Launching NDS core\n";

	//OSD
	config::osd_message = "NDS CORE INIT";
	config::osd_count = 180;
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

	get_core_data(3);
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
	core_mmu.set_lcd_3D_data(&core_cpu_nds9.controllers.video.lcd_3D_stat);

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
			|| (event.type == SDL_MOUSEMOTION))
			{
				core_pad.handle_input(event);
				handle_hotkey(event);

				//Trigger Joypad Interrupt if necessary
				if(core_pad.joypad_irq)
				{
					core_mmu.nds9_if |= 0x1000;
					core_mmu.nds7_if |= 0x1000;
				}
			}

			//Hotplug joypad
			else if((event.type == SDL_JOYDEVICEADDED) && (!core_pad.joy_init)) { core_pad.init(); }
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
							core_mmu.nds9_temp_if |= core_mmu.nds9_if;

							//Match up bits in IE and IF to exit halt
							if(core_mmu.nds9_if & core_mmu.nds9_ie)
							{
								core_mmu.nds9_if = core_mmu.nds9_temp_if;
								core_cpu_nds9.idle_state = 0;
								
								if((core_mmu.nds9_ime & 0x1) && ((core_cpu_nds9.reg.cpsr & CPSR_IRQ) == 0))
								{
									core_cpu_nds9.reg.r15 -= (core_cpu_nds9.arm_mode == NTR_ARM9::ARM) ? 4 : 0;
								}

								else { core_cpu_nds9.last_idle_state = 0; }
							}

							else { core_mmu.nds9_if = 0; }

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
									x = 100;

									if((core_mmu.nds9_ime & 0x1) && ((core_cpu_nds9.reg.cpsr & CPSR_IRQ) == 0) && (core_mmu.nds9_ie & core_mmu.nds9_if))
									{
										core_cpu_nds9.reg.r15 -= (core_cpu_nds9.arm_mode == NTR_ARM9::ARM) ? 4 : 0;
									}

									else { core_cpu_nds9.last_idle_state = 0; }
								}

								//Execute any other pending IRQs that happen during IntrWait or VBlankIntrWait
								else if((core_mmu.nds9_ie & (1 << x)) && (core_mmu.nds9_if & (1 << x)))
								{
									core_cpu_nds9.idle_state = 0;
									x = 100;

									if((core_mmu.nds9_ime & 0x1) && ((core_cpu_nds9.reg.cpsr & CPSR_IRQ) == 0))
									{
										core_cpu_nds9.reg.r15 -= (core_cpu_nds9.arm_mode == NTR_ARM9::ARM) ? 8 : 2;
									}

									else { core_cpu_nds9.last_idle_state = 0; }
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
							core_mmu.nds7_temp_if |= core_mmu.nds7_if;

							//Match up bits in IE and IF to exit halt
							if(core_mmu.nds7_if & core_mmu.nds7_ie)
							{
								core_mmu.nds7_if = core_mmu.nds7_temp_if;
								core_cpu_nds7.idle_state = 0;
								
								if((core_mmu.nds7_ime & 0x1) && ((core_cpu_nds7.reg.cpsr & CPSR_IRQ) == 0))
								{
									core_cpu_nds7.reg.r15 -= (core_cpu_nds7.arm_mode == NTR_ARM7::ARM) ? 4 : 0;
								}

								else { core_cpu_nds7.last_idle_state = 0; }
							}

							else { core_mmu.nds7_if = 0; }

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
									x = 100;

									if((core_mmu.nds7_ime & 0x1) && ((core_cpu_nds7.reg.cpsr & CPSR_IRQ) == 0) && (core_mmu.nds7_ie & core_mmu.nds7_if))
									{
										core_cpu_nds7.reg.r15 -= (core_cpu_nds7.arm_mode == NTR_ARM7::ARM) ? 4 : 0;
									}

									else { core_cpu_nds7.last_idle_state = 0; }
								}

								//Execute any other pending IRQs that happen during IntrWait or VBlankIntrWait
								else if((core_mmu.nds7_ie & (1 << x)) && (core_mmu.nds7_if & (1 << x)))
								{
									core_cpu_nds7.idle_state = 0;
									x = 100;

									if((core_mmu.nds7_ime & 0x1) && ((core_cpu_nds7.reg.cpsr & CPSR_IRQ) == 0))
									{
										core_cpu_nds7.reg.r15 -= (core_cpu_nds7.arm_mode == NTR_ARM7::ARM) ? 8 : 2;
									}

									else { core_cpu_nds7.last_idle_state = 0; }
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
	
		#ifdef GBE_IMAGE_FORMATS
		save_name += save_stream.str() + ".png";
		IMG_SavePNG(core_cpu_nds9.controllers.video.final_screen, save_name.c_str());
		#endif
		
		#ifndef GBE_IMAGE_FORMATS
		save_name += save_stream.str() + ".bmp";
		SDL_SaveBMP(core_cpu_nds9.controllers.video.final_screen, save_name.c_str());
		#endif

		//OSD
		config::osd_message = "SAVED SCREENSHOT";
		config::osd_count = 180;
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
		SDL_DestroyWindow(core_cpu_nds9.controllers.video.window);

		//Initialize new window - SDL
		if(!config::use_opengl)
		{
			core_cpu_nds9.controllers.video.window = SDL_CreateWindow("GBE+", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, next_w, next_h, config::flags);
			SDL_GetWindowSize(core_cpu_nds9.controllers.video.window, &config::win_width, &config::win_height);
			core_cpu_nds9.controllers.video.final_screen = SDL_GetWindowSurface(core_cpu_nds9.controllers.video.window);

			//Find the maximum fullscreen dimensions that maintain the original aspect ratio
			if(config::flags & SDL_WINDOW_FULLSCREEN)
			{
				double max_width, max_height, ratio = 0.0;

				max_width = (double)config::win_width / config::sys_width;
				max_height = (double)config::win_height / config::sys_height;

				if(max_width <= max_height) { ratio = max_width; }
				else { ratio = max_height; }

				core_cpu_nds9.controllers.video.max_fullscreen_ratio = ratio;
				core_pad.sdl_fs_ratio = ratio;
			}
		}

		//Initialize new window - OpenGL
		else
		{
			core_cpu_nds9.controllers.video.opengl_init();
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

	//Start IR communications
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F3))
	{
		core_mmu.ntr_027.start_comms = true;
	}

	//Reset emulation on F8
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F8)) { reset(); }

	//Toggle vertical or horizontal mode
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == config::hotkey_shift_screen))
	{
		if(config::lcd_config & 0x2) { config::resize_mode = 0; }
		else { config::resize_mode = 1; }
		
		config::request_resize = true;
	}
		
	//Toggle swap NDS screens
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == config::hotkey_swap_screen))
	{
		if(config::lcd_config & 0x1) { config::lcd_config &= ~0x1; }
		else { config::lcd_config |= 0x1; }
	}

	//Initiate various communication functions
	//HCV-1000 - Swipe barcode
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F11))
	{
		switch(core_mmu.current_slot2_device)
		{
			case NTR_MMU::SLOT2_HCV_1000:
				core_mmu.hcv.cnt &= ~0x80;
				break;
		}
	}
}

/****** Process hotkey input - Use exsternally when not using SDL ******/
void NTR_core::handle_hotkey(int input, bool pressed)
{
	//Toggle turbo on
	if((input == config::hotkey_turbo) && (pressed)) { config::turbo = true; }

	//Toggle turbo off
	else if((input == config::hotkey_turbo) && (!pressed)) { config::turbo = false; }

	//Toggle swap NDS screens on F4
	else if((input == config::hotkey_swap_screen) && (pressed))
	{
		if(config::lcd_config & 0x1) { config::lcd_config &= ~0x1; }
		else { config::lcd_config |= 0x1; }
	}

	//Toggle vertical or horizontal mode
	else if((input == config::hotkey_shift_screen) && (pressed))
	{
		if(config::lcd_config & 0x2) { config::resize_mode = 0; }
		else { config::resize_mode = 1; }
		
		config::request_resize = true;
	}

	//Initiate various communication functions
	//HCV-1000 - Swipe barcode
	else if((input == SDLK_F11) && (pressed))
	{
		switch(core_mmu.current_slot2_device)
		{
			case NTR_MMU::SLOT2_HCV_1000:
				core_mmu.slot2_hcv_load_barcode(config::external_card_file);
				core_mmu.hcv.cnt &= ~0x80;
				break;
		}
	}
}

/****** Updates the core's volume ******/
void NTR_core::update_volume(u8 volume)
{
	config::volume = volume;
}

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
u32 NTR_core::ex_get_reg(u8 reg_index) { return 0; }

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

	return true;
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

		//Result from save cart icon
		case 0x3:
			{
				std::string cache_path = config::data_path + "icons/cache/";
				std::string full_file_path = cache_path + core_mmu.header.game_code;

				u32 icon_base = core_mmu.read_cart_u32(0x68);
				u32 icon_crc = core_mmu.read_cart_u16(icon_base + 2);
				full_file_path += util::to_hex_str(icon_crc) + ".bmp";

				result = core_cpu_nds9.controllers.video.save_cart_icon(full_file_path);
			}

			break;
	}

	return result;
}
	