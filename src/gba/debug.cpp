// GB Enhanced+ Copyright Daniel Baxter 2017
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : core.cpp
// Date : September 30, 2017
// Description : Debugging interface
//
// GBA debugging via the CLI
// Allows emulator to pause on breakpoints, view and edit memory

#include <iomanip>

#include "common/util.h"

#include "core.h"
 
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

	//Display current PC when print PC is enabled
	if(db_unit.print_pc) { std::cout<<"PC -> 0x" << core_cpu.reg.r15 << "\n"; }
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

		//Delete all breakpoints
		else if(command.substr(0, 3) == "del")
		{
			valid_command = true;
			
			db_unit.breakpoints.clear();
			db_unit.watchpoint_addr.clear();
			db_unit.watchpoint_val.clear();
			db_unit.watchpoint_old_val.clear();

			//Advanced debugging
			#ifdef GBE_DEBUG
			db_unit.write_addr.clear();
			db_unit.read_addr.clear();
			#endif
			
			std::cout<<"\nBreakpoints deleted\n";
			debug_process_command();
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

		//Show memory - 16 bytes
		else if((command.substr(0, 3) == "u8s") && (command.substr(4, 2) == "0x"))
		{
			valid_command = true;
			u32 mem_location = 0;
			std::string hex_string = command.substr(6);

			if(hex_string.size() > 4) { hex_string = hex_string.substr(hex_string.size() - 4); }

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
				db_unit.last_command = "u8s";
				
				for(u32 x = 0; x < 16; x++)
				{
					std::cout<<"Memory @ " << util::to_hex_str(mem_location + x) << " : 0x" << std::hex << (int)core_mmu.read_u8(mem_location + x) << "\n";
				}
				
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

		//Print every PC to the screen
		else if(command == "pc")
		{
			if(db_unit.print_pc)
			{
				std::cout<<"\nPrint-PC turned off\n";
				db_unit.print_pc = false;
			}

			else
			{
				std::cout<<"\nPrint-PC turned on\n";
				db_unit.print_pc = true;
			}

			valid_command = true;
			db_unit.last_command = "pc";
			debug_process_command();
		}

		//Print help information
		else if(command == "h")
		{
			std::cout<<"n \t\t Run next Fetch-Decode-Execute stage\n";
			std::cout<<"c \t\t Continue until next breakpoint\n";
			std::cout<<"bp \t\t Set breakpoint, format 0x1234ABCD\n";
			std::cout<<"bc \t\t Set breakpoint on memory change, format 0x1234ABCD for addr, 0x12 for value\n";
			std::cout<<"del \t\t Deletes ALL current breakpoints\n";
			std::cout<<"u8 \t\t Show BYTE @ memory, format 0x1234ABCD\n";
			std::cout<<"u8s \t\t Show 16 BYTES @ memory, format 0x1234\n";
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
			std::cout<<"pc \t\t Toggles printing all Program Counter values to screen\n";
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
