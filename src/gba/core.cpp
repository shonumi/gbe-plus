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
	db_unit.last_command = "n";

	std::cout<<"GBE::Launching GBA core\n";
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
	core_mmu.read_file(config::rom_file);

	//Start everything all over again
	start();
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

/****** Debugger - Allow core to run until a breaking condition occurs ******/
void AGB_core::debug_step()
{
	bool printed = false;

	//In continue mode, if breakpoints exist, try to stop on one
	if((db_unit.breakpoints.size() > 0) && (db_unit.last_command == "c"))
	{
		for(int x = 0; x < db_unit.breakpoints.size(); x++)
		{
			//When a BP is matched, display info, wait for next input command
			if(core_cpu.reg.r15 == db_unit.breakpoints[x])
			{
				debug_display();
				debug_process_command();
				printed = true;
			}
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
void AGB_core::debug_display() const
{
	//Display current CPU action
	switch(core_cpu.debug_message)
	{
		case 0xFF:
			std::cout << "Filling pipeline\n"; break;
		case 0x0: 
			std::cout << std::hex << "CPU::Executing THUMB_1 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x1:
			std::cout << std::hex << "CPU::Executing THUMB_2 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x2:
			std::cout << std::hex << "CPU::Executing THUMB_3 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x3:
			std::cout << std::hex << "CPU::Executing THUMB_4 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x4:
			std::cout << std::hex << "CPU::Executing THUMB_5 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x5:
			std::cout << std::hex << "CPU::Executing THUMB_6 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x6:
			std::cout << std::hex << "CPU::Executing THUMB_7 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x7:
			std::cout << std::hex << "CPU::Executing THUMB_8 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x8:
			std::cout << std::hex << "CPU::Executing THUMB_9 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x9:
			std::cout << std::hex << "CPU::Executing THUMB_10 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0xA:
			std::cout << std::hex << "CPU::Executing THUMB_11 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0xB:
			std::cout << std::hex << "CPU::Executing THUMB_12 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0xC:
			std::cout << std::hex << "CPU::Executing THUMB_13 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0xD:
			std::cout << std::hex << "CPU::Executing THUMB_14 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0xE:
			std::cout << std::hex << "CPU::Executing THUMB_15 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0xF:
			std::cout << std::hex << "CPU::Executing THUMB_16 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x10:
			std::cout << std::hex << "CPU::Executing THUMB_17 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x11:
			std::cout << std::hex << "CPU::Executing THUMB_18 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x12:
			std::cout << std::hex << "CPU::Executing THUMB_19 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x13:
			std::cout << std::hex << "Unknown THUMB Instruction : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x14:
			std::cout << std::hex << "CPU::Executing ARM_3 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x15:
			std::cout << std::hex << "CPU::Executing ARM_4 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x16:
			std::cout << std::hex << "CPU::Executing ARM_5 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x17:
			std::cout << std::hex << "CPU::Executing ARM_6 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x18:
			std::cout << std::hex << "CPU::Executing ARM_7 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x19:
			std::cout << std::hex << "CPU::Executing ARM_9 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x1A:
			std::cout << std::hex << "CPU::Executing ARM_10 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x1B:
			std::cout << std::hex << "CPU::Executing ARM_11 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x1C:
			std::cout << std::hex << "CPU::Executing ARM_12 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x1D:
			std::cout << std::hex << "CPU::Executing ARM_13 : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x1E:
			std::cout << std::hex << "Unknown ARM Instruction : 0x" << core_cpu.debug_code << "\n\n"; break;
		case 0x1F:
			std::cout << std::hex << "CPU::Skipping ARM Instruction : 0x" << core_cpu.debug_code << "\n\n"; break;
	}

	//Display CPU registers
	std::cout<< std::hex <<"R0 : 0x" << std::setw(8) << std::setfill('0') << core_cpu.get_reg(0) << 
		" -- R4  : 0x" << std::setw(8) << std::setfill('0') << core_cpu.get_reg(4) << 
		" -- R8  : 0x" << std::setw(8) << std::setfill('0') << core_cpu.get_reg(8) << 
		" -- R12 : 0x" << std::setw(8) << std::setfill('0') << core_cpu.get_reg(12) << "\n";

	std::cout<< std::hex <<"R1 : 0x" << std::setw(8) << std::setfill('0') << core_cpu.get_reg(1) << 
		" -- R5  : 0x" << std::setw(8) << std::setfill('0') << core_cpu.get_reg(5) << 
		" -- R9  : 0x" << std::setw(8) << std::setfill('0') << core_cpu.get_reg(9) << 
		" -- R13 : 0x" << std::setw(8) << std::setfill('0') << core_cpu.get_reg(13) << "\n";

	std::cout<< std::hex <<"R2 : 0x" << std::setw(8) << std::setfill('0') << core_cpu.get_reg(2) << 
		" -- R6  : 0x" << std::setw(8) << std::setfill('0') << core_cpu.get_reg(6) << 
		" -- R10 : 0x" << std::setw(8) << std::setfill('0') << core_cpu.get_reg(10) << 
		" -- R14 : 0x" << std::setw(8) << std::setfill('0') << core_cpu.get_reg(14) << "\n";

	std::cout<< std::hex <<"R3 : 0x" << std::setw(8) << std::setfill('0') << core_cpu.get_reg(3) << 
		" -- R7  : 0x" << std::setw(8) << std::setfill('0') << core_cpu.get_reg(7) << 
		" -- R11 : 0x" << std::setw(8) << std::setfill('0') << core_cpu.get_reg(11) << 
		" -- R15 : 0x" << std::setw(8) << std::setfill('0') << core_cpu.get_reg(15) << "\n";

	//Grab CPSR Flags and status
	std::string cpsr_stats = "(";

	if(core_cpu.reg.cpsr & CPSR_N_FLAG) { cpsr_stats += "N"; }
	else { cpsr_stats += "."; }

	if(core_cpu.reg.cpsr & CPSR_Z_FLAG) { cpsr_stats += "Z"; }
	else { cpsr_stats += "."; }

	if(core_cpu.reg.cpsr & CPSR_C_FLAG) { cpsr_stats += "C"; }
	else { cpsr_stats += "."; }

	if(core_cpu.reg.cpsr & CPSR_V_FLAG) { cpsr_stats += "V"; }
	else { cpsr_stats += "."; }

	cpsr_stats += "  ";

	if(core_cpu.reg.cpsr & CPSR_IRQ) { cpsr_stats += "I"; }
	else { cpsr_stats += "."; }

	if(core_cpu.reg.cpsr & CPSR_FIQ) { cpsr_stats += "F"; }
	else { cpsr_stats += "."; }

	if(core_cpu.reg.cpsr & CPSR_STATE) { cpsr_stats += "T)"; }
	else { cpsr_stats += ".)"; }

	std::cout<< std::hex <<"CPSR : 0x" << std::setw(8) << std::setfill('0') << core_cpu.reg.cpsr << "\t" << cpsr_stats << "\n";

	//Display current CPU cycles
	if(db_unit.display_cycles) { std::cout<<"Current CPU cycles : " << std::dec << core_cpu.debug_cycles << "\n"; }
}

/****** Debugger - Wait for user input, process it to decide what next to do ******/
void AGB_core::debug_process_command()
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

		//Show memory - 1 byte
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
				db_unit.last_command = "u8";
				std::cout<<"Memory @ " << hex_string << " : 0x" << std::hex << (int)core_mmu.read_u8(mem_location) << "\n";
				debug_process_command();
			}
		}

		//Show memory - 2 bytes
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
				db_unit.last_command = "u16";
				std::cout<<"Memory @ " << hex_string << " : 0x" << std::hex << (int)core_mmu.read_u16(mem_location) << "\n";
				debug_process_command();
			}
		}

		//Show memory - 4 bytes
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
				db_unit.last_command = "u32";
				std::cout<<"Memory @ " << hex_string << " : 0x" << std::hex << (int)core_mmu.read_u32(mem_location) << "\n";
				debug_process_command();
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

				db_unit.last_command = "w8";
				core_mmu.write_u8(mem_location, mem_value);
				debug_process_command();
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

				db_unit.last_command = "w16";
				core_mmu.write_u16(mem_location, mem_value);
				debug_process_command();
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

				db_unit.last_command = "w32";
				core_mmu.write_u32(mem_location, mem_value);
				debug_process_command();
			}
		}

		//Write to register
		else if(command.substr(0, 3) == "reg")
		{
			valid_command = true;
			bool valid_value = false;
			u32 reg_index = 0;
			u32 reg_value = 0;
			std::string reg_string = command.substr(4);

			//Convert string into a usable u32
			valid_command = util::from_str(reg_string, reg_index);

			//Request valid input again
			if((!valid_command) || (reg_index > 0x24))
			{
				std::cout<<"\nInvalid register : " << command << "\n";
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
				
					valid_value = util::from_hex_str(command.substr(2), reg_value);
				
					if(!valid_value)
					{
						std::cout<<"\nInvalid value : " << command << "\n";
					}
				}

				switch(reg_index)
				{
					case 0x0:
						std::cout<<"\nSetting Register R0 to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r0 = reg_value;
						break;

					case 0x1:
						std::cout<<"\nSetting Register R1 to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r1 = reg_value;
						break;

					case 0x2:
						std::cout<<"\nSetting Register R2 to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r2 = reg_value;
						break;

					case 0x3:
						std::cout<<"\nSetting Register R3 to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r3 = reg_value;
						break;

					case 0x4:
						std::cout<<"\nSetting Register R4 to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r4 = reg_value;
						break;

					case 0x5:
						std::cout<<"\nSetting Register R5 to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r5 = reg_value;
						break;

					case 0x6:
						std::cout<<"\nSetting Register R6 to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r6 = reg_value;
						break;

					case 0x7:
						std::cout<<"\nSetting Register R7 to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r7 = reg_value;
						break;

					case 0x8:
						std::cout<<"\nSetting Register R8 to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r8 = reg_value;
						break;

					case 0x9:
						std::cout<<"\nSetting Register R9 to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r9 = reg_value;
						break;

					case 0xA:
						std::cout<<"\nSetting Register R10 to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r10 = reg_value;
						break;

					case 0xB:
						std::cout<<"\nSetting Register R11 to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r11 = reg_value;
						break;

					case 0xC:
						std::cout<<"\nSetting Register R12 to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r12 = reg_value;
						break;

					case 0xD:
						std::cout<<"\nSetting Register R13 to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r13 = reg_value;
						break;

					case 0xE:
						std::cout<<"\nSetting Register R14 to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r14 = reg_value;
						break;

					case 0xF:
						std::cout<<"\nSetting Register R15 to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r15 = reg_value;
						break;

					case 0x10:
						std::cout<<"\nSetting Register CPSR to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.cpsr = reg_value;
						break;

					case 0x11:
						std::cout<<"\nSetting Register R8 (FIQ) to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r8_fiq = reg_value;
						break;

					case 0x12:
						std::cout<<"\nSetting Register R9 (FIQ) to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r9_fiq = reg_value;
						break;

					case 0x13:
						std::cout<<"\nSetting Register R10 (FIQ) to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r10_fiq = reg_value;
						break;

					case 0x14:
						std::cout<<"\nSetting Register R11 (FIQ) to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r11_fiq = reg_value;
						break;

					case 0x15:
						std::cout<<"\nSetting Register R12 (FIQ) to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r12_fiq = reg_value;
						break;

					case 0x16:
						std::cout<<"\nSetting Register R13 (FIQ) to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r13_fiq = reg_value;
						break;

					case 0x17:
						std::cout<<"\nSetting Register R14 (FIQ) to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r14_fiq = reg_value;
						break;

					case 0x18:
						std::cout<<"\nSetting Register SPSR (FIQ) to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.spsr_fiq = reg_value;
						break;

					case 0x19:
						std::cout<<"\nSetting Register R13 (SVC) to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r13_svc = reg_value;
						break;

					case 0x1A:
						std::cout<<"\nSetting Register R14 (SVC) to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r14_svc = reg_value;
						break;

					case 0x1B:
						std::cout<<"\nSetting Register SPSR (SVC) to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.spsr_svc = reg_value;
						break;

					case 0x1C:
						std::cout<<"\nSetting Register R13 (ABT) to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r13_abt = reg_value;
						break;

					case 0x1D:
						std::cout<<"\nSetting Register R14 (ABT) to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r14_abt = reg_value;
						break;

					case 0x1E:
						std::cout<<"\nSetting Register SPSR (ABT) to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.spsr_abt = reg_value;
						break;

					case 0x1F:
						std::cout<<"\nSetting Register R13 (IRQ) to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r13_irq = reg_value;
						break;

					case 0x20:
						std::cout<<"\nSetting Register R14 (IRQ) to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r14_irq = reg_value;
						break;

					case 0x21:
						std::cout<<"\nSetting Register SPSR (IRQ) to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.spsr_irq = reg_value;
						break;

					case 0x22:
						std::cout<<"\nSetting Register R13 (UND) to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r13_und = reg_value;
						break;

					case 0x23:
						std::cout<<"\nSetting Register R14 (UND) to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.r14_und = reg_value;
						break;

					case 0x24:
						std::cout<<"\nSetting Register SPSR (UND) to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.spsr_und = reg_value;
						break;
				}
				

				db_unit.last_command = "reg";
				debug_display();
				debug_process_command();
			}
		}

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
			std::cout<<"bp \t\t Set breakpoint, format 0x1234ABCD\n";
			std::cout<<"u8 \t\t Show BYTE @ memory, format 0x1234ABCD\n";
			std::cout<<"u16 \t\t Show HALFWORD @ memory, format 0x1234ABCD\n";
			std::cout<<"u32 \t\t Show WORD @ memory, format 0x1234ABCD\n";
			std::cout<<"w8 \t\t Write BYTE @ memory, format 0x1234ABCD for addr, 0x12 for value\n";
			std::cout<<"w16 \t\t Write HALFWORD @ memory, format 0x1234ABCD for addr, 0x1234 for value\n";
			std::cout<<"w32 \t\t Write WORD @ memory, format 0x1234ABCD for addr, 0x1234ABCD for value\n";
			std::cout<<"reg \t\t Change register value (0-36) \n";
			std::cout<<"dq \t\t Quit the debugger\n";
			std::cout<<"dc \t\t Toggle CPU cycle display\n";
			std::cout<<"cr \t\t Reset CPU cycle counter\n";
			std::cout<<"rs \t\t Reset emulation\n";
			std::cout<<"pa \t\t Toggles printing all instructions to screen\n";
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

/****** Returns a string with the mnemonic assembly instruction ******/
std::string AGB_core::debug_get_mnemonic(u32 addr) { return " "; }
	
/****** Process hotkey input ******/
void AGB_core::handle_hotkey(SDL_Event& event)
{
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
