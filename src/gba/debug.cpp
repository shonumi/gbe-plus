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
				db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.debug_code, false);

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

				db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.debug_code, false);
	
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
		db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.debug_code, false);

		debug_display();
		debug_process_command();
		printed = true;
	}

	//Advanced debugging
	#ifdef GBE_DEBUG

	//In continue mode, if a write-breakpoint is triggered, try to stop on one
	else if((db_unit.write_addr.size() > 0) && (db_unit.last_command == "c") && (core_mmu.debug_write))
	{
		for(int x = 0; x < db_unit.write_addr.size(); x++)
		{
			for(int y = 0; y < 4; y++)
			{
				if(db_unit.write_addr[x] == core_mmu.debug_addr[y])
				{
					db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.debug_code, false);

					debug_display();
					debug_process_command();
					printed = true;
					break;
				}
			}
		}
	}

	//In continue mode, if a read-breakpoint is triggered, try to stop on one
	else if((db_unit.read_addr.size() > 0) && (db_unit.last_command == "c") && (core_mmu.debug_read))
	{
		for(int x = 0; x < db_unit.read_addr.size(); x++)
		{
			for(int y = 0; y < 4; y++)
			{
				if(db_unit.read_addr[x] == core_mmu.debug_addr[y])
				{
					db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.debug_code, false);

					debug_display();
					debug_process_command();
					printed = true;
					break;
				}
			}
		}
	}

	//Reset read-write alerts
	core_mmu.debug_read = false;
	core_mmu.debug_write = false;
	core_mmu.debug_addr[0] = 0;
	core_mmu.debug_addr[1] = 0;
	core_mmu.debug_addr[2] = 0;
	core_mmu.debug_addr[3] = 0;

	#endif

	//Display every instruction when print all is enabled
	if((!printed) && (db_unit.print_all))
	{
		db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.debug_code, false);
		debug_display();
	}

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
			std::cout << std::hex << "CPU::Executing THUMB_1 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x1:
			std::cout << std::hex << "CPU::Executing THUMB_2 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x2:
			std::cout << std::hex << "CPU::Executing THUMB_3 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x3:
			std::cout << std::hex << "CPU::Executing THUMB_4 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x4:
			std::cout << std::hex << "CPU::Executing THUMB_5 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x5:
			std::cout << std::hex << "CPU::Executing THUMB_6 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x6:
			std::cout << std::hex << "CPU::Executing THUMB_7 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x7:
			std::cout << std::hex << "CPU::Executing THUMB_8 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x8:
			std::cout << std::hex << "CPU::Executing THUMB_9 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x9:
			std::cout << std::hex << "CPU::Executing THUMB_10 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0xA:
			std::cout << std::hex << "CPU::Executing THUMB_11 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0xB:
			std::cout << std::hex << "CPU::Executing THUMB_12 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0xC:
			std::cout << std::hex << "CPU::Executing THUMB_13 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0xD:
			std::cout << std::hex << "CPU::Executing THUMB_14 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0xE:
			std::cout << std::hex << "CPU::Executing THUMB_15 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0xF:
			std::cout << std::hex << "CPU::Executing THUMB_16 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x10:
			std::cout << std::hex << "CPU::Executing THUMB_17 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x11:
			std::cout << std::hex << "CPU::Executing THUMB_18 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x12:
			std::cout << std::hex << "CPU::Executing THUMB_19 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x13:
			std::cout << std::hex << "Unknown THUMB Instruction : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x14:
			std::cout << std::hex << "CPU::Executing ARM_3 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x15:
			std::cout << std::hex << "CPU::Executing ARM_4 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x16:
			std::cout << std::hex << "CPU::Executing ARM_5 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x17:
			std::cout << std::hex << "CPU::Executing ARM_6 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x18:
			std::cout << std::hex << "CPU::Executing ARM_7 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x19:
			std::cout << std::hex << "CPU::Executing ARM_9 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x1A:
			std::cout << std::hex << "CPU::Executing ARM_10 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x1B:
			std::cout << std::hex << "CPU::Executing ARM_11 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x1C:
			std::cout << std::hex << "CPU::Executing ARM_12 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x1D:
			std::cout << std::hex << "CPU::Executing ARM_13 : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x1E:
			std::cout << std::hex << "Unknown ARM Instruction : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x1F:
			std::cout << std::hex << "CPU::Skipping ARM Instruction : 0x" << core_cpu.debug_code << " :: " << db_unit.last_mnemonic << "\n\n"; break;
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

		//Load save state
		else if(command.substr(0, 2) == "ls")
		{
			bool valid_value = false;
			u32 slot = 0;
			std::string slot_string = command.substr(3);

			//Convert string into a usable u32
			valid_value = util::from_str(slot_string, slot);

			if(!valid_value)
			{
				std::cout<<"\nInvalid save state slot : " << slot_string << "\n";
			}

			else
			{
				if(slot >= 10) { std::cout<<"Save state slot too high\n"; }

				else
				{
					std::cout<<"Loading Save State " << slot_string << "\n";
					load_state(slot);
				}
			}

			valid_command = true;
			db_unit.last_command = "ls";
			debug_process_command();
		}

		//Make save state
		else if(command.substr(0, 2) == "ss")
		{
			bool valid_value = false;
			u32 slot = 0;
			std::string slot_string = command.substr(3);

			//Convert string into a usable u32
			valid_value = util::from_str(slot_string, slot);

			if(!valid_value)
			{
				std::cout<<"\nInvalid save state slot : " << slot_string << "\n";
			}

			else
			{
				if(slot >= 10) { std::cout<<"Save state slot too high\n"; }

				else
				{
					std::cout<<"Saving State " << slot_string << "\n";
					save_state(slot);
				}
			}

			valid_command = true;
			db_unit.last_command = "ss";
			debug_process_command();
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

		//Advanced debugging
		#ifdef GBE_DEBUG

		//Set write breakpoint
		else if((command.substr(0, 2) == "bw") && (command.substr(3, 2) == "0x"))
		{
			valid_command = true;
			u32 mem_location = 0;
			std::string hex_string = command.substr(5);

			if(hex_string.size() > 10) { hex_string = hex_string.substr(hex_string.size() - 10); }

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
				db_unit.last_command = "bw";
				db_unit.write_addr.push_back(mem_location);
				std::cout<<"\nWrite Breakpoint added at 0x" << std::hex << mem_location << "\n";
				debug_process_command();
			}
		}

		//Set write breakpoint
		else if((command.substr(0, 2) == "br") && (command.substr(3, 2) == "0x"))
		{
			valid_command = true;
			u32 mem_location = 0;
			std::string hex_string = command.substr(5);

			if(hex_string.size() > 10) { hex_string = hex_string.substr(hex_string.size() - 10); }

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
				db_unit.last_command = "br";
				db_unit.read_addr.push_back(mem_location);
				std::cout<<"\nRead Breakpoint added at 0x" << std::hex << mem_location << "\n";
				debug_process_command();
			}
		}

		#endif

		//Disassembles 16 THUMB instructions from specified address
		else if((command.substr(0, 2) == "dt") && (command.substr(3, 2) == "0x"))
		{
			valid_command = true;
			bool valid_value = false;
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
				u8 dbg_msg = core_cpu.debug_message;
				core_cpu.debug_message = 0x00;

				for(u32 x = 0; x < 16; x++)
				{
					u32 addr = (mem_location + (x * 2));
					u32 opcode = core_mmu.read_u16(addr);

					std::cout<<"0x" << addr << "\t" << debug_get_mnemonic(opcode, false) << "\n";
				}

				db_unit.last_command = "dt";
				debug_process_command();

				core_cpu.debug_message = dbg_msg;
			}
		}

		//Disassembles 16 ARM instructions from specified address
		else if((command.substr(0, 2) == "da") && (command.substr(3, 2) == "0x"))
		{
			valid_command = true;
			bool valid_value = false;
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
				u8 dbg_msg = core_cpu.debug_message;
				core_cpu.debug_message = 0xFF;

				for(u32 x = 0; x < 16; x++)
				{
					u32 addr = (mem_location + (x * 4));
					u32 opcode = core_mmu.read_u32(addr);

					std::cout<<"0x" << addr << "\t" << debug_get_mnemonic(opcode, false) << "\n";
				}

				db_unit.last_command = "da";
				debug_process_command();

				core_cpu.debug_message = dbg_msg;
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

			//Advanced debugging
			#ifdef GBE_DEBUG
			std::cout<<"bw \t\t Set breakpoint on memory write, format 0x1234ABCD for addr\n";
			std::cout<<"br \t\t Set breakpoint on memory read, format 0x1234ABCD for addr\n";
			#endif

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
			std::cout<<"ls \t\t Loads a given save state (0-9)\n";
			std::cout<<"ss \t\t Saves a given save state (0-9)\n"; 
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

/****** Returns a string with the mnemonic assembly instruction ******/
std::string AGB_core::debug_get_mnemonic(u32 data, bool is_addr)
{
	bool arm_debug = (core_cpu.debug_message > 0x13) ? true : false;
	std::string instr = "";

	u32 opcode = 0;
	u32 addr = data;
	if(is_addr) { opcode = (arm_debug) ? core_mmu.read_u32(addr) : core_mmu.read_u16(addr); }
	else { opcode = data; }

	//Get ARM mnemonic
	if(arm_debug)
	{
		std::string cond_code = "";
		u8 cond_bytes = ((opcode >> 28) & 0xF);

		switch(cond_bytes)
		{
			case 0x0: cond_code = "EQ"; break;
			case 0x1: cond_code = "NE"; break;
			case 0x2: cond_code = "CS"; break;
			case 0x3: cond_code = "CC"; break;
			case 0x4: cond_code = "MI"; break;
			case 0x5: cond_code = "PL"; break;
			case 0x6: cond_code = "VS"; break;
			case 0x7: cond_code = "VC"; break;
			case 0x8: cond_code = "HI"; break;
			case 0x9: cond_code = "LS"; break;
			case 0xA: cond_code = "GE"; break;
			case 0xB: cond_code = "LT"; break;
			case 0xC: cond_code = "GT"; break;
			case 0xD: cond_code = "LE"; break;
			case 0xE: cond_code = ""; break;
			case 0xF: cond_code = ""; break;
		}

		//ARM.13 SWI opcodes
		if((opcode & 0xF000000) == 0xF000000)
		{
			instr = "SWI" + cond_code + " " + util::to_hex_str((opcode >> 16) & 0xFF);
		}

		//ARM.4 B, BL, BLX opcodes
		else if((opcode & 0xE000000) == 0xA000000)
		{
			u8 op = (opcode >> 24) & 0x1;
			if((opcode >> 28) == 0xF) { op = 2; }

			u32 offset = (opcode & 0xFFFFFF);
			offset <<= 2;
			if(offset & 0x2000000) { offset |= 0xFC000000; }
			offset += (addr + 8);

			if((op == 2) && (opcode & 0x1000000)) { offset += 2; }

			switch(op)
			{
				case 0x0: instr = "B" + cond_code + " " + util::to_hex_str(offset); break;
				case 0x1: instr = "BL" + cond_code + " " + util::to_hex_str(offset); break;
				case 0x2: instr = "BLX" + cond_code + " " + util::to_hex_str(offset) + " (ARMv5)"; break;
			}
		}

		//ARM.3 B and BX opcodes
		else if((opcode & 0xFFFFFD0) == 0x12FFF10)
		{
			u8 op = (opcode >> 4) & 0xF;
			u8 rn = (opcode & 0xF);

			switch(op)
			{
				case 0x1: instr = "B" + cond_code + " R" + util::to_str(rn); break;
				case 0x3: instr = "BX" + cond_code + " R" + util::to_str(rn); break;
			}
		}

		//ARM.9 Single Data Transfer opcodes
		else if((opcode & 0xC000000) == 0x4000000)
		{
			u8 op = ((opcode >> 20) & 0x1);
			u8 rd = ((opcode >> 12) & 0xF);
			u8 rn = ((opcode >> 16) & 0xF);
			u8 is_imm = ((opcode >> 25) & 0x1);
			u8 is_up = ((opcode >> 23) & 0x1);
			u8 is_byte = ((opcode >> 22) & 0x1);
			std::string immediate = "";

			if(is_imm)
			{
				u8 shift = ((opcode >> 7) & 0x1F);
				u8 shift_type = ((opcode >> 5) & 0x3);
				u8 rm = (opcode & 0xF);

				immediate = "R" + util::to_str(rm) + " ";

				switch(shift_type)
				{
					case 0x0: immediate += "LSL #" + util::to_str(shift); break;
					case 0x1: immediate += "LSR #" + util::to_str(shift); break;
					case 0x2: immediate += "ASR #" + util::to_str(shift); break;
					case 0x3: immediate += "ROR #" + util::to_str(shift); break;
				}
			}

			else { immediate = util::to_hex_str(opcode & 0xFFF); }

			if(!is_up) { immediate = "- " + immediate; }
			else { immediate = "+ " + immediate; }
				
			switch(op)
			{
				case 0x0:
					instr = "STR" + cond_code;
					if(is_byte) { instr += "B"; }
					instr += " R" + util::to_str(rd) + ",";
					instr += " [R" + util::to_str(rn) + " " + immediate + "]";
					break;

				case 0x1:
					instr = "LDR" + cond_code;
					if(is_byte) { instr += "B"; }
					instr += " R" + util::to_str(rd) + ",";
					instr += " [R" + util::to_str(rn) + " " + immediate + "]";
					break;
			}
		}

		//ARM.12 Swap opcodes
		else if((opcode & 0xFB00FF0) == 0x1000090)
		{
			u8 is_byte = ((opcode >> 22) & 0x1);
			u8 rm = (opcode & 0xF);
			u8 rd = ((opcode >> 12) & 0xF);
			u8 rn = ((opcode >> 16) & 0xF);

			instr = "SWP" + cond_code;
			if(is_byte) { instr += "B"; }
			instr += " R" + util::to_str(rd) + ", R" + util::to_str(rm) + ", [R" + util::to_str(rn) + "]";
		}

		//ARM.6 PSR opcodes
		else if(((opcode & 0xFB00000) == 0x3200000)
		|| ((opcode & 0xF900FF0) == 0x1000000))
		{
			u8 op = ((opcode >> 21) & 0x1);
			u8 is_imm = ((opcode >> 25) & 0x1);
			std::string psr = ((opcode >> 22) & 0x1) ? "SPSR" : "CPSR";

			//MRS
			if(opcode == 0)
			{
				instr = "MRS" + cond_code + ", R" + util::to_str(((opcode >> 12) & 0xF)) + ", " + psr;
			}

			//MSR
			else
			{
				std::string immediate = "";
				std::string flags = "_";
				
				if((opcode >> 19) & 0x1) { flags += "f"; }
				if((opcode >> 18) & 0x1) { flags += "s"; }
				if((opcode >> 17) & 0x1) { flags += "x"; }
				if((opcode >> 16) & 0x1) { flags += "c"; }
				
				if(is_imm)
				{
					u8 ror = (opcode >> 8) & 0xF;
					ror *= 2;

					immediate = util::to_hex_str(opcode & 0xFF) + " ROR #" + util::to_str(ror);
				}

				else { immediate = "R" + util::to_str((opcode >> 12) & 0xF); }

				instr = "MSR " + psr + ", " + immediate;
			}
		}

		//ARM.10 Halfword Signed Transfers
		else if(((opcode & 0xE400F90) == 0x90)
		|| ((opcode & 0xE400090) == 0x400090))
		{
			u8 op = ((opcode >> 5) & 0x3);
			u8 is_pre = ((opcode >> 24) & 0x1); 
			u8 is_up = ((opcode >> 23) & 0x1);
			u8 is_imm = ((opcode >> 22) & 0x1);
			u8 is_write_back = ((opcode >> 21) & 0x1);
			u8 is_load = ((opcode >> 20) & 0x1);
			u8 rn = ((opcode >> 16) & 0xF);
			u8 rd = ((opcode >> 12) & 0xF);
			std::string immediate = "";

			if(is_imm)
			{
				u8 imm = ((opcode >> 8) & 0xF);
				imm |= (opcode & 0xF);
				immediate = util::to_hex_str(imm);
			}

			else { immediate = "R" + util::to_str(opcode & 0xF); }

			if(is_load) { op += 0x10; }

			switch(op)
			{
				case 0x1: instr = "STR" + cond_code + "H R" + util::to_str(rd) + ", "; break;
				case 0x2: instr = "LDR" + cond_code + "D R" + util::to_str(rd) + ", "; break;
				case 0x3: instr = "STR" + cond_code + "D R" + util::to_str(rd) + ", "; break;
				case 0x11: instr = "LDR" + cond_code + "H R" + util::to_str(rd) + ", "; break;
				case 0x12: instr = "LDR" + cond_code + "SB R" + util::to_str(rd) + ", "; break;
				case 0x13: instr = "LDR" + cond_code + "SH R" + util::to_str(rd) + ", "; break;
			}

			if(is_pre)
			{
				instr += "[R" + util::to_str(rn);
				if(is_up) { instr += " + "; }
				else { instr += " - "; }
				instr += immediate + "]";
				if(is_write_back) { instr += "{!}"; }
			}

			else
			{
				instr += "[R" + util::to_str(rn) + "], ";
				if(is_up) { instr += " + "; }
				else { instr += " - "; }
				instr += immediate;
			}
		}

		//ARM.11 Block Transfer opcodes
		else if((opcode & 0xE000000) == 0x8000000)
		{
			u8 op = ((opcode >> 20) & 0x1);
			u8 is_pre = ((opcode >> 24) & 0x1); 
			u8 is_up = ((opcode >> 23) & 0x1);
			u8 is_psr = ((opcode >> 22) & 0x1);
			u8 is_write_back = ((opcode >> 21) & 0x1);
			u8 rn = ((opcode >> 16) & 0xF);
			u16 list = (opcode & 0xFFFF);
			u8 last_reg = 0;

			std::string amod = "";
			std::string rlist = "";

			if((is_pre) && (is_up)) { amod = "IB"; }
			else if((!is_pre) && (is_up)) { amod = "IA"; }
			else if((is_pre) && (!is_up)) { amod = "DB"; }
			else { amod = "DA"; }

			for(u32 x = 0; x < 16; x++)
			{
				if((list >> x) & 0x1) { last_reg = x; }
			}

			for(u32 x = 0; x < 16; x++)
			{
				if((list >> x) & 0x1)
				{
					rlist += "R" + util::to_str(x);
					if(x != last_reg) { rlist += ", "; }
				}
			}

			instr = (op) ? "LDM" : "STM";
			instr += cond_code + amod + " R" + util::to_str(rn);
			if(is_write_back) { instr += "{!}"; }
			instr += ", {" + rlist + "}";
			if(is_psr) { instr += "{^}"; }
		}

		//ARM.7 Multiply opcodes
		else if(((opcode & 0xFC000F0) == 0x90)
		|| ((opcode & 0xF8000F0) == 0x800090))
		{
			u8 op = ((opcode >> 21) & 0xF);
			u8 is_set_cond = ((opcode >> 20) & 0x1);
			u8 rd = ((opcode >> 16) & 0xF);
			u8 rn = ((opcode >> 12) & 0xF);
			u8 rs = ((opcode >> 8) & 0xF);
			u8 rm = (opcode & 0xF);

			switch(op)
			{
				case 0x0:
					instr = "MUL" + cond_code;
					if(is_set_cond) { instr += "{S}"; }
					instr += " R" + util::to_str(rd) + ", R" + util::to_str(rm) + ", R" + util::to_str(rs);
					break;

				case 0x1:
					instr = "MLA" + cond_code;
					if(is_set_cond) { instr += "{S}"; }
					instr += " R" + util::to_str(rd) + ", R" + util::to_str(rm) + ", R" + util::to_str(rs) + ", R" + util::to_str(rn);
					break;

				case 0x4:
					instr = "UMULL" + cond_code;
					if(is_set_cond) { instr += "{S}"; }
					instr += " R" + util::to_str(rn) + ", R" + util::to_str(rd) + ", R" + util::to_str(rm) + ", R" + util::to_str(rs);
					break;

				case 0x5:
					instr = "UMLAL" + cond_code;
					if(is_set_cond) { instr += "{S}"; }
					instr += " R" + util::to_str(rn) + ", R" + util::to_str(rd) + ", R" + util::to_str(rm) + ", R" + util::to_str(rs);
					break;

				case 0x6:
					instr = "SMULL" + cond_code;
					if(is_set_cond) { instr += "{S}"; }
					instr += " R" + util::to_str(rn) + ", R" + util::to_str(rd) + ", R" + util::to_str(rm) + ", R" + util::to_str(rs);
					break;

				case 0x7:
					instr = "SMLAL" + cond_code;
					if(is_set_cond) { instr += "{S}"; }
					instr += " R" + util::to_str(rn) + ", R" + util::to_str(rd) + ", R" + util::to_str(rm) + ", R" + util::to_str(rs);
					break;

				case 0x8:
					instr = "SMLAxy" + cond_code;
					instr += " R" + util::to_str(rd) + ", R" + util::to_str(rm) + ", R" + util::to_str(rs) + ", R" + util::to_str(rn);
					instr += " (ARMv5)";
					break;

				case 0x9:
					if(opcode & 0x20)
					{
						instr = "SMULWy" + cond_code;
						instr += " R" + util::to_str(rd) + ", R" + util::to_str(rm) + ", R" + util::to_str(rs);
						instr += " (ARMv5)";
					}

					else
					{
						instr = "SMLAWy" + cond_code;
						instr += " R" + util::to_str(rd) + ", R" + util::to_str(rm) + ", R" + util::to_str(rs) + ", R" + util::to_str(rn);
						instr += " (ARMv5)";
					}
 
					break;

				case 0xA:
					instr = "SMLALxy" + cond_code;
					instr += " R" + util::to_str(rn) + ", R" + util::to_str(rd) + ", R" + util::to_str(rm) + ", R" + util::to_str(rs);
					instr += " (ARMv5)";
					break;

				case 0xB:
					instr = "SMULxy" + cond_code;
					instr += " R" + util::to_str(rd) + ", R" + util::to_str(rm) + ", R" + util::to_str(rs);
					instr += " (ARMv5)";
					break;
			}
		}

		//CLZ opcodes
		else if((opcode & 0xFFF0FF0) == 0x16F0F10)
		{
			u8 rd = ((opcode >> 12) & 0xF);
			u8 rm = (opcode & 0xF);

			instr = "CLZ" + cond_code + " R" + util::to_str(rd) + ", R" + util::to_str(rm);
			instr += " (ARMv5)";
		}

		//QALU opcodes
		else if((opcode & 0xF900FF0) == 0x1000050)
		{
			u8 op = ((opcode >> 20) & 0xF);
			u8 rn = ((opcode >> 16) & 0xF);
			u8 rd = ((opcode >> 12) & 0xF);
			u8 rm = (opcode & 0xF);

			switch(op)
			{
				case 0x0: instr = "QADD" + cond_code; break;
				case 0x2: instr = "QSUB" + cond_code; break;
				case 0x4: instr = "QDADD" + cond_code; break;
				case 0x6: instr = "QDSUB" + cond_code; break;
			}

			instr += " R" + util::to_str(rd) + ", R" + util::to_str(rm) + ", R" + util::to_str(rn);
			instr += " (ARMv5)";
		}

		//ARM.5 ALU opcodes
		else if(((opcode & 0xE000010) == 0) || ((opcode & 0xE000010) == 0x10) || ((opcode & 0xE000000) == 0x2000000))
		{
			u8 op = ((opcode >> 21) & 0xF);
			u8 is_imm = ((opcode >> 25) & 0x1);
			u8 is_set_cond = ((opcode >> 20) & 0x1);
			u8 rn = ((opcode >> 16) & 0xF);
			u8 rd = ((opcode >> 12) & 0xF);
			u8 imm = 0;
			u8 shift = 0;
			std::string op2 = "";
			std::string s = (is_set_cond) ? "{S}" : "";
			std::string p = (is_set_cond) ? "{P}" : "";

			if(is_imm)
			{
				imm = (opcode & 0xFF);
				shift = ((opcode >> 8) & 0xF);
				shift *= 2;
				op2 = util::to_hex_str(imm) + " ROR #" + util::to_str(shift);
			}

			else
			{
				u8 rm = (opcode & 0xF);
				u8 shift_type = ((opcode >> 5) & 0x3);

				op2 = "R" + util::to_str(rm) + " ";
				
				switch(shift_type)
				{
					case 0x0: op2 += "LSL"; break;
					case 0x1: op2 += "LSR"; break;
					case 0x2: op2 += "ASR"; break;
					case 0x3: op2 += "ROR"; break;
				}

				if(opcode & 0x10)
				{
					u8 rs = ((opcode >> 8) & 0xF);
					op2 += " R" + util::to_str(rs);
				}

				else
				{
					u8 shift_amount = (((opcode) >> 7) & 0x1F);
					op2 += " " + util::to_hex_str(shift_amount);
				}
			}

			switch(op)
			{
				case 0x0: instr = "AND" + cond_code + s + " R" + util::to_str(rd) + ", R" + util::to_str(rn) + ", " + op2; break;
				case 0x1: instr = "EOR" + cond_code + s + " R" + util::to_str(rd) + ", R" + util::to_str(rn) + ", " + op2; break;
				case 0x2: instr = "SUB" + cond_code + s + " R" + util::to_str(rd) + ", R" + util::to_str(rn) + ", " + op2; break;
				case 0x3: instr = "RSB" + cond_code + s + " R" + util::to_str(rd) + ", R" + util::to_str(rn) + ", " + op2; break;
				case 0x4: instr = "ADD" + cond_code + s + " R" + util::to_str(rd) + ", R" + util::to_str(rn) + ", " + op2; break;
				case 0x5: instr = "ADC" + cond_code + s + " R" + util::to_str(rd) + ", R" + util::to_str(rn) + ", " + op2; break;
				case 0x6: instr = "SBC" + cond_code + s + " R" + util::to_str(rd) + ", R" + util::to_str(rn) + ", " + op2; break;
				case 0x7: instr = "RSC" + cond_code + s + " R" + util::to_str(rd) + ", R" + util::to_str(rn) + ", " + op2; break;
				case 0x8: instr = "TST" + cond_code + p + " R" + util::to_str(rn) + ", " + op2; break;
				case 0x9: instr = "TEQ" + cond_code + p + " R" + util::to_str(rn) + ", " + op2; break;
				case 0xA: instr = "CMP" + cond_code + p + " R" + util::to_str(rn) + ", " + op2; break;
				case 0xB: instr = "CMN" + cond_code + p + " R" + util::to_str(rn) + ", " + op2; break;
				case 0xC: instr = "ORR" + cond_code + s + " R" + util::to_str(rd) + ", R" + util::to_str(rn) + ", " + op2; break;
				case 0xD: instr = "MOV" + cond_code + s + " R" + util::to_str(rd) + ", " + op2; break;
				case 0xE: instr = "BIC" + cond_code + s + " R" + util::to_str(rd) + ", R" + util::to_str(rn) + ", " + op2; break;
				case 0xF: instr = "MVN" + cond_code + s + " R" + util::to_str(rd) + ", " + op2; break;
			}
		}

		//ARM CoDataTrans opcodes
		else if((opcode & 0xE000000) == 0xC000000)
		{
			u8 op = ((opcode >> 20) & 0x1);
			u8 rn = ((opcode >> 16) & 0xF);
			u8 cd = ((opcode >> 12) & 0xF);
			u8 pn = ((opcode >> 8) & 0xF);
			u8 is_up = ((opcode >> 23) & 0x1);
			u8 is_pre = ((opcode >> 24) & 0x1);
			u8 is_write_back = ((opcode >> 21) & 0x1);

			std::string l = ((opcode >> 22) & 0x1) ? "{L}" : "";
			std::string immediate = util::to_hex_str((opcode & 0xF) * 4);

			switch(op)
			{
				case 0x0:
					if((opcode >> 28) == 0xF) { instr = "STC2" + l; }
					else { instr = "STC" + cond_code + l; }
					break;

				case 0x1:
					if((opcode >> 28) == 0xF) { instr = "LDC2" + l; }
					else { instr = "LCD" + cond_code + l; }
					break;
			}

			instr += " P" + util::to_str(pn) + ", C" + util::to_str(cd) + ", ";

			if(is_pre)
			{
				instr += "[R" + util::to_str(rn);
				if(is_up) { instr += " + "; }
				else { instr += " - "; }
				instr += immediate + "]";
				if(is_write_back) { instr += "{!}"; }
			}

			else
			{
				instr += "[R" + util::to_str(rn) + "], ";
				if(is_up) { instr += " + "; }
				else { instr += " - "; }
				instr += immediate;
			}

			instr += " (ARMv5)";
		}

		//ARM CoRegTrans opcodes
		else if((opcode & 0xF000010) == 0xE000010)
		{
			u8 op = ((opcode >> 20) & 0x1);
			u8 cp_opc = ((opcode >> 21) & 0x7);
			u8 cn = ((opcode >> 16) & 0xF);
			u8 rd = ((opcode >> 12) & 0xF);
			u8 pn = ((opcode >> 8) & 0xF);
			u8 cp = ((opcode >> 5) & 0x7);
			u8 cm = (opcode & 0xF);

			switch(op)
			{
				case 0x0:
					if((opcode >> 28) == 0xF) { instr = "MCR2"; }
					else { instr = "MCR" + cond_code; }
					break;

				case 0x1:
					if((opcode >> 28) == 0xF) { instr = "MRC2"; }
					else { instr = "MRC" + cond_code; }
					break;
			}

			instr += " P" + util::to_str(pn) + ", <" + util::to_str(cp_opc) + ">, R" + util::to_str(rd) + ", C" + util::to_str(cn) + ", C" + util::to_str(cm) + "{" + util::to_str(cp) + "}";
			instr += " (ARMv5)";
		}

		//ARM CoDataOp opcodes
		else if((opcode & 0xF000010) == 0xE000000)
		{
			u8 cp_opc = ((opcode >> 20) & 0xF);
			u8 cn = ((opcode >> 16) & 0xF);
			u8 cd = ((opcode >> 12) & 0xF);
			u8 pn = ((opcode >> 8) & 0xF);
			u8 cp = ((opcode >> 5) & 0x7);
			u8 cm = (opcode & 0xF);

			if((opcode >> 28) == 0xF) { instr = "CDP2"; }
			else { instr = "CDP" + cond_code; }

			instr += " P" + util::to_str(pn) + ", <" + util::to_str(cp_opc) + ">, C" + util::to_str(cd) + ", C" + util::to_str(cn) + ", C" + util::to_str(cm) + "{" + util::to_str(cp) + "}";
			instr += " (ARMv5)";
		}
	}

	//Get THUMB mnemonic
	else
	{
		//THUMB.17 SWI opcodes
		if((opcode & 0xFF00) == 0xDF00)
		{
			instr = "SWI " + util::to_hex_str(opcode & 0xFF);
		}

		//THUMB.13 ADD SP opcodes
		else if((opcode & 0xFF00) == 0xB000)
		{
			instr = "ADD SP, ";
			u16 offset = ((opcode & 0x7F) << 2);
			
			if(opcode & 0x80) {instr += "-" + util::to_hex_str(offset); }
			else { instr += util::to_hex_str(offset); }
		}

		//THUMB.14 PUSH-POP opcodes
		else if((opcode & 0xF600) == 0xB400)
		{
			instr = (opcode & 0x800) ? "POP " : "PUSH ";
			u8 r_list = (opcode & 0xFF);
			u8 last_reg = 0;

			for(u32 x = 0; x < 8; x++)
			{
				if((r_list >> x) & 0x1) { last_reg = x; }
			}

			instr += "{";

			for(u32 x = 0; x < 8; x++)
			{
				if((r_list >> x) & 0x1)
				{
					instr += "R" + util::to_str(x);
					if(x != last_reg) { instr += ", "; }
				}
			}

			instr += "}";

			if((opcode & 0x100) && (opcode & 0x800)) { instr += "{R15}"; }
			else if(opcode & 0x800) { instr += "{R14}"; }
		}

		//THUMB.7 Load-Store Reg Offsets opcodes
		else if((opcode & 0xF200) == 0x5000)
		{
			u8 op = ((opcode >> 10) & 0x3);

			instr = (op & 0x2) ? "LDR" : "STR";
			if(op & 0x1) { instr += "B"; }

			instr += " R" + util::to_str(opcode & 0x7) + ",";
			instr += " [R" + util::to_str((opcode >> 3) & 0x7) + "],";
			instr += " R" + util::to_str((opcode >> 6) & 0x7);
		}

		//THUMB.8 Load-Store Sign Extended opcodes
		else if((opcode & 0xF200) == 5200)
		{
			u8 op = ((opcode >> 10) & 0x3);

			switch(op)
			{
				case 0x0: instr = "STRH"; break;
				case 0x1: instr = "LDSB"; break;
				case 0x2: instr = "LDRH"; break;
				case 0x3: instr = "LDSH"; break;
			}

			instr += " R" + util::to_str(opcode & 0x7) + ",";
			instr += " [R" + util::to_str((opcode >> 3) & 0x7) + "],";
			instr += " R" + util::to_str((opcode >> 6) & 0x7);
		}

		//THUMB.10 Load-Store Halfword opcodes
		else if((opcode & 0xF000) == 0x8000)
		{
			u8 op = ((opcode >> 11) & 0x1);
			u8 rd = (opcode & 0x7);
			u8 rb = ((opcode >> 3) & 0x7);
			u8 offset = ((opcode >> 6) & 0x1F);
			offset *= 2;

			switch(op)
			{
				case 0x0: instr = "STRH R" + util::to_str(rd) + ", [R" + util::to_str(rb) + ", " + util::to_hex_str(offset) + "]"; break;
				case 0x1: instr = "LDRH R" + util::to_str(rd) + ", [R" + util::to_str(rb) + ", " + util::to_hex_str(offset) + "]"; break;
			}
		}

		//THUMB.11 Load-Store SP relative opcodes
		else if((opcode & 0xF000) == 0x9000)
		{
			u8 op = ((opcode >> 11) & 0x1);
			u8 rd = ((opcode >> 8) & 0x7);
			u16 offset = (opcode & 0xFF);
			offset *= 4;

			switch(op)
			{
				case 0x0: instr = "STR R" + util::to_str(rd) + ", [SP, " + util::to_hex_str(offset) + "]"; break;
				case 0x1: instr = "LDR R" + util::to_str(rd) + ", [SP, " + util::to_hex_str(offset) + "]"; break;
			}
		}

		//THUMB.12 Get relative address opcodes
		else if((opcode & 0xF000) == 0xA000)
		{
			u8 op = ((opcode >> 11) & 0x1);
			u8 rd = ((opcode >> 8) & 0x7);
			u16 offset = (opcode & 0xFF);
			offset *= 4;

			switch(op)
			{
				case 0x0: instr = "ADD R" + util::to_str(rd) + ", PC, " + util::to_hex_str(offset); break;
				case 0x1: instr = "ADD R" + util::to_str(rd) + ", SP, " + util::to_hex_str(offset); break;
			}
		}

		//THUMB.15 LDM-STM opcodes
		else if((opcode & 0xF000) == 0xC000)
		{
			instr = (opcode & 0x800) ? "LDMIA " : "STMIA ";
			u8 r_list = (opcode & 0xFF);
			u8 rb = ((opcode >> 8) & 0x7);
			u8 last_reg = 0;

			for(u32 x = 0; x < 8; x++)
			{
				if((r_list >> x) & 0x1) { last_reg = x; }
			}
				
			instr += util::to_str(rb) + " {";

			for(u32 x = 0; x < 8; x++)
			{
				if((r_list >> x) & 0x1)
				{
					instr += "R" + util::to_str(x);
					if(x != last_reg) { instr += ", "; }
				}
			}

			instr += "}";
		}

		//THUMB.16 Conditional Branch opcodes
		else if((opcode & 0xF000) == 0xD000)
		{
			u8 op = ((opcode >> 8) & 0xF);
			
			u32 offset = (opcode & 0xFF);
			offset <<= 1;			
			if(offset & 0x100) { offset |= 0xFFFFFE00; }
			offset += (addr + 4);

			switch(op)
			{
				case 0x0: instr = "BEQ "; break;
				case 0x1: instr = "BNE "; break;
				case 0x2: instr = "BCS "; break;
				case 0x3: instr = "BCC "; break;
				case 0x4: instr = "BMI "; break;
				case 0x5: instr = "BPL "; break;
				case 0x6: instr = "BVS "; break;
				case 0x7: instr = "BVC "; break;
				case 0x8: instr = "BHI "; break;
				case 0x9: instr = "BLS "; break;
				case 0xA: instr = "BGE "; break;
				case 0xB: instr = "BLT "; break;
				case 0xC: instr = "BGT "; break;
				case 0xD: instr = "BLE "; break;
				default: instr = "UNDEF Branch"; break;
			}

			instr += util::to_hex_str(offset);
		}
	
		//THUMB.4 ALU opcodes
		else if((opcode & 0xFC00) == 0x4000)
		{
			u8 op = ((opcode >> 6) & 0xF);

			switch(op)
			{
				case 0x0: instr = "AND "; break;
				case 0x1: instr = "EOR "; break;
				case 0x2: instr = "LSL "; break;
				case 0x3: instr = "LSR "; break;
				case 0x4: instr = "ASR "; break;
				case 0x5: instr = "ADC "; break;
				case 0x6: instr = "SBC "; break;
				case 0x7: instr = "ROR "; break;
				case 0x8: instr = "TST "; break;
				case 0x9: instr = "NEG "; break;
				case 0xA: instr = "CMP "; break;
				case 0xB: instr = "CMN "; break;
				case 0xC: instr = "ORR "; break;
				case 0xD: instr = "MUL "; break;
				case 0xE: instr = "BIC "; break;
				case 0xF: instr = "MVN "; break;
			}

			instr += "R" + util::to_str(opcode & 0x7) + ", ";
			instr += "R" + util::to_str((opcode >> 3) & 0x7);
		}

		//THUMB.5 High Reg opcodes
		else if((opcode & 0xFC00) == 0x4400)
		{
			u8 op = ((opcode >> 8) & 0x3);
			u8 sr_msb = (opcode & 0x40) ? 1 : 0;
			u8 dr_msb = (opcode & 0x80) ? 1 : 0;

			u8 rs = sr_msb ? 0x8 : 0x0;
			u8 rd = dr_msb ? 0x8 : 0x0;

			rs |= ((opcode >> 3) & 0x7);
			rd |= (opcode & 0x7);

			switch(op)
			{
				case 0x0: instr = "ADD"; break;
				case 0x1: instr = "CMP"; break;
				case 0x2: instr = "MOV"; break;
				case 0x3: instr = dr_msb ? "BLX" : "BX"; break;
			}

			if(op < 3)
			{
				instr += " R" + util::to_str(rd) + ",";
				instr += " R" + util::to_str(rs);
			}

			else { instr += " R" + util::to_str(rs); }
		}

		//THUMB.18 Unconditional Branch opcodes
		else if((opcode & 0xF800) == 0xE000)
		{
			u32 offset = (opcode & 0x7FF);
			offset <<= 1;
			if(offset & 0x800) { offset |= 0xFFFFF000; }
			offset += (addr + 4);

			instr = "B " + util::to_hex_str(offset);
		}

		//THUMB.6 Load PC Relative opcodes
		else if((opcode & 0xF800) == 0x4800)
		{
			u32 offset = (opcode & 0xFF);
			offset <<= 2;
			offset += (addr + 4);

			instr = "LDR R" + util::to_str((opcode >> 8) & 0x7) + ", [" + util::to_hex_str(offset) + "]";
		}

		//THUMB.2 ADD-SUB opcodes
		else if((opcode & 0xF800) == 0x1800)
		{
			u8 op = ((opcode >> 9) & 0x3);
			u8 rd = (opcode & 0x7);
			u8 rs = ((opcode >> 3) & 0x7);
			u8 rn = ((opcode >> 6) & 0x7);

			switch(op)
			{
				case 0x0: instr = "ADD R" + util::to_str(rd) + ", R" + util::to_str(rs) + ", R" + util::to_str(rn); break;
				case 0x1: instr = "SUB R" + util::to_str(rd) + ", R" + util::to_str(rs) + ", R" + util::to_str(rn); break;
				case 0x2: instr = "ADD R" + util::to_str(rd) + ", R" + util::to_str(rs) + ", " + util::to_hex_str(rn); break;
				case 0x3: instr = "SUB R" + util::to_str(rd) + ", R" + util::to_str(rs) + ", " + util::to_hex_str(rn); break;
			}
		}

		//THUMB.1 Move Shifted Register
		else if((opcode & 0xE000) == 0)
		{
			u8 op = ((opcode >> 11) & 0x3);
			u8 rd = (opcode & 0x7);
			u8 rs = ((opcode >> 3) & 0x7);
			u8 offset = ((opcode >> 6) & 0x1F);

			switch(op)
			{
				case 0x0: instr = "LSL R" + util::to_str(rd) + ", R" + util::to_str(rs) + ", " + util::to_str(offset); break;
				case 0x1: instr = "LSR R" + util::to_str(rd) + ", R" + util::to_str(rs) + ", " + util::to_str(offset); break;
				case 0x2: instr = "ASR R" + util::to_str(rd) + ", R" + util::to_str(rs) + ", " + util::to_str(offset); break;
				case 0x3: instr = "Reserved"; break;
			}
		}

		//THUMB.3 MCAS opcodes
		else if((opcode & 0xE000) == 0x2000)
		{
			u8 op = ((opcode >> 11) & 0x3);
			u8 rd = ((opcode >> 8) & 0x7);
			u8 imm = (opcode & 0xFF);

			switch(op)
			{
				case 0x0: instr = "MOV R" + util::to_str(rd) + ", " + util::to_hex_str(imm); break;
				case 0x1: instr = "CMP R" + util::to_str(rd) + ", " + util::to_hex_str(imm); break;
				case 0x2: instr = "ADD R" + util::to_str(rd) + ", " + util::to_hex_str(imm); break;
				case 0x3: instr = "SUB R" + util::to_str(rd) + ", " + util::to_hex_str(imm); break;
			}
		}

		//THUMB.9 Load-Store with offset
		else if((opcode & 0xE000) == 0x6000)
		{
			u8 op = ((opcode >> 11) & 0x3);
			u8 rd = (opcode & 0x7);
			u8 rb = ((opcode >> 3) & 0x7);
			u8 offset = ((opcode >> 6) & 0x1F);

			switch(op)
			{
				case 0x0: instr = "STR R" + util::to_str(rd) + ", [R" + util::to_str(rb) + ", " + util::to_hex_str(offset * 4) + "]"; break;
				case 0x1: instr = "LDR R" + util::to_str(rd) + ", [R" + util::to_str(rb) + ", " + util::to_hex_str(offset * 4) + "]"; break;
				case 0x2: instr = "STRB R" + util::to_str(rd) + ", [R" + util::to_str(rb) + ", " + util::to_hex_str(offset) + "]"; break;
				case 0x3: instr = "LDRB R" + util::to_str(rd) + ", [R" + util::to_str(rb) + ", " + util::to_hex_str(offset) + "]"; break;
			}
		}
	}

	return instr;
}
