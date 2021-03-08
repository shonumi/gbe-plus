// GB Enhanced+ Copyright Daniel Baxter 2017
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : core.cpp
// Date : September 29, 2017
// Description : Debugging interface
//
// DMG debugging via the CLI
// Allows emulator to pause on breakpoints, view and edit memory

#include <iomanip>

#include "common/util.h"

#include "core.h"

/****** Debugger - Allow core to run until a breaking condition occurs ******/
void DMG_core::debug_step()
{
	//Use external interface (GUI) for all debugging
	if(config::use_external_interfaces)
	{
		config::debug_external();
		return;
	}

	//Use CLI for all debugging
	bool printed = false;

	//In continue mode, if breakpoints exist, try to stop on one
	if((db_unit.breakpoints.size() > 0) && (db_unit.last_command == "c"))
	{
		for(int x = 0; x < db_unit.breakpoints.size(); x++)
		{
			//When a BP is matched, display info, wait for next input command
			if(core_cpu.reg.pc == db_unit.breakpoints[x])
			{
				db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.reg.pc);
				core_cpu.opcode = core_mmu.read_u8(core_cpu.reg.pc);

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
				db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.reg.pc);
				core_cpu.opcode = core_mmu.read_u8(core_cpu.reg.pc);

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
		db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.reg.pc);
		core_cpu.opcode = core_mmu.read_u8(core_cpu.reg.pc);

		debug_display();
		debug_process_command();
		printed = true;
	}

	//When running for a given amount of instructions, stop when done.
	else if(db_unit.run_count)
	{
		db_unit.run_count--;

		//Stop run count and re-enter debugging mode
		if(!db_unit.run_count)
		{
			db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.reg.pc);

			debug_display();
			debug_process_command();
			printed = true;
		}
	}

	//Advanced debugging
	#ifdef GBE_DEBUG

	//In continue mode, if a write-breakpoint is triggered, try to stop on one
	else if((db_unit.write_addr.size() > 0) && (db_unit.last_command == "c") && (core_mmu.debug_write))
	{
		for(int x = 0; x < db_unit.write_addr.size(); x++)
		{
			if(db_unit.write_addr[x] == core_mmu.debug_addr)
			{
				db_unit.last_mnemonic = debug_get_mnemonic(db_unit.last_pc);
				core_cpu.opcode = core_mmu.read_u8(db_unit.last_pc);
				debug_display();

				db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.reg.pc);
				core_cpu.opcode = core_mmu.read_u8(core_cpu.reg.pc);
				debug_display();

				debug_process_command();
				printed = true;
			}
		}
	}

	//In continue mode, if a read-breakpoint is triggered, try to stop on one
	else if((db_unit.read_addr.size() > 0) && (db_unit.last_command == "c") && (core_mmu.debug_read))
	{
		for(int x = 0; x < db_unit.read_addr.size(); x++)
		{
			if(db_unit.read_addr[x] == core_mmu.debug_addr)
			{
				db_unit.last_mnemonic = debug_get_mnemonic(db_unit.last_pc);
				core_cpu.opcode = core_mmu.read_u8(db_unit.last_pc);
				debug_display();

				db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.reg.pc);
				core_cpu.opcode = core_mmu.read_u8(core_cpu.reg.pc);
				debug_display();

				debug_process_command();
				printed = true;
			}
		}
	}

	//Reset read-write alerts
	core_mmu.debug_read = false;
	core_mmu.debug_write = false;

	#endif

	//Display every instruction when print all is enabled
	if((!printed) && (db_unit.print_all)) 
	{
		db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.reg.pc);
		core_cpu.opcode = core_mmu.read_u8(core_cpu.reg.pc);
		debug_display();
	}

	//Display current PC when print PC is enabled
	if(db_unit.print_pc) { std::cout<<"PC -> 0x" << core_cpu.reg.pc << " :: " << debug_get_mnemonic(core_cpu.reg.pc) << "\n"; }

	//Update last PC
	db_unit.last_pc = core_cpu.reg.pc;
}

/****** Debugger - Display relevant info to the screen ******/
void DMG_core::debug_display() const
{
	std::cout << std::hex << "CPU::Executing Opcode : 0x" << (u32)core_cpu.opcode << " --> " << db_unit.last_mnemonic << "\n\n";

	//Display CPU registers
	std::cout<< std::hex <<"A  : 0x" << std::setw(2) << std::setfill('0') << (u32)core_cpu.reg.a << "\n";

	std::cout<< std::hex <<"B  : 0x" << std::setw(2) << std::setfill('0') << (u32)core_cpu.reg.b << 
		" -- C  : 0x" << std::setw(2) << std::setfill('0') << (u32)core_cpu.reg.c << "\n";

	std::cout<< std::hex << "D  : 0x" << std::setw(2) << std::setfill('0') << (u32)core_cpu.reg.d << 
		" -- E  : 0x" << std::setw(2) << std::setfill('0') << (u32)core_cpu.reg.e << "\n";

	std::cout<< std::hex << "H  : 0x" << std::setw(2) << std::setfill('0') << (u32)core_cpu.reg.h << 
		" -- L  : 0x" << std::setw(2) << std::setfill('0') << (u32)core_cpu.reg.l << "\n";

	std::cout<< std::hex <<"PC : 0x" << std::setw(4) << std::setfill('0') << (u32)core_cpu.reg.pc <<
		" -- SP  : 0x" << std::setw(4) << std::setfill('0') << (u32)core_cpu.reg.sp << "\n";

	std::string flag_stats = "(";

	if(core_cpu.reg.f & 0x80) { flag_stats += "Z"; }
	else { flag_stats += "."; }

	if(core_cpu.reg.f & 0x40) { flag_stats += "N"; }
	else { flag_stats += "."; }

	if(core_cpu.reg.f & 0x20) { flag_stats += "H"; }
	else { flag_stats += "."; }

	if(core_cpu.reg.f & 0x10) { flag_stats += "C"; }
	else { flag_stats += "."; }

	flag_stats += ")";

	std::cout<< std::hex <<"FLAGS : 0x" << std::setw(2) << std::setfill('0') << (u32)core_cpu.reg.f << "\t" << flag_stats << "\n";

	if(db_unit.display_cycles) { std::cout<<"CYCLES : " << std::dec << core_cpu.debug_cycles << "\n"; }

	std::string ram_bank = "";
	std::string rom_bank = "";
	u32 temp = 0;

	if(core_mmu.cart.mbc_type != DMG_MMU::MBC6) { rom_bank = util::to_hex_str(core_mmu.rom_bank); }

	else
	{
		rom_bank = util::to_hex_str(core_mmu.rom_bank & 0x7F) + " :: " + util::to_hex_str((core_mmu.rom_bank >> 8) & 0x7F);
		ram_bank = util::to_hex_str(core_mmu.bank_bits & 0x7) + " :: " + util::to_hex_str((core_mmu.bank_bits >> 8) & 0x7);
	}
	
	switch(core_mmu.cart.mbc_type)
	{
		case DMG_MMU::MBC1: temp = (core_mmu.cart.multicart) ? core_mmu.bank_bits : 0; break;
		case DMG_MMU::MBC2: temp = 0; break;
		case DMG_MMU::MBC3:
		case DMG_MMU::MBC5:
		case DMG_MMU::MMM01:
		case DMG_MMU::GB_CAMERA:
		case DMG_MMU::HUC1: temp = core_mmu.bank_bits; break;
	}

	if(core_mmu.cart.mbc_type != DMG_MMU::MBC6) { ram_bank = util::to_hex_str(temp); }

	std::cout<< std::hex << "ROM BANK  : " << std::setw(2) << std::setfill('0') << rom_bank << 
	" -- RAM BANK  : " << std::setw(2) << std::setfill('0') << ram_bank << "\n\n";
}

/****** Debugger - Wait for user input, process it to decide what next to do ******/
void DMG_core::debug_process_command()
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

			if(hex_string.size() > 4) { hex_string = hex_string.substr(hex_string.size() - 4); }

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
				db_unit.last_command = "u16";
				std::cout<<"Memory @ " << hex_string << " : 0x" << std::hex << (int)core_mmu.read_u16(mem_location) << "\n";
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
			if((!valid_command) || (reg_index > 0x9))
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

					else if((reg_index < 8) && (valid_value) && (reg_value > 0xFF))
					{
						std::cout<<"\nValue is too large (greater than 0xFF) : 0x" << std::hex << reg_value;
						valid_value = false;
					}

					else if((reg_index >= 8) && (valid_value) && (reg_value > 0xFFFF))
					{
						std::cout<<"\nValue is too large (greater than 0xFFFF) : 0x" << std::hex << reg_value;
						valid_value = false;
					}
				}

				switch(reg_index)
				{
					case 0x0:
						std::cout<<"\nSetting Register A to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.a = reg_value;
						break;

					case 0x1:
						std::cout<<"\nSetting Register B to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.b = reg_value;
						break;

					case 0x2:
						std::cout<<"\nSetting Register C to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.c = reg_value;
						break;

					case 0x3:
						std::cout<<"\nSetting Register D to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.d = reg_value;
						break;

					case 0x4:
						std::cout<<"\nSetting Register E to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.e = reg_value;
						break;

					case 0x5:
						std::cout<<"\nSetting Register H to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.h = reg_value;
						break;

					case 0x6:
						std::cout<<"\nSetting Register L to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.l = reg_value;
						break;

					case 0x7:
						std::cout<<"\nSetting Register F to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.f = reg_value;
						break;

					case 0x8:
						std::cout<<"\nSetting Register SP to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.sp = reg_value;
						break;

					case 0x9:
						std::cout<<"\nSetting Register PC to 0x" << std::hex << reg_value << "\n";
						core_cpu.reg.pc = reg_value;
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
				db_unit.last_command = "br";
				db_unit.read_addr.push_back(mem_location);
				std::cout<<"\nRead Breakpoint added at 0x" << std::hex << mem_location << "\n";
				debug_process_command();
			}
		}

		#endif

		//Disassembles 16 GBZ80 instructions from specified address
		else if((command.substr(0, 2) == "dz") && (command.substr(3, 2) == "0x"))
		{
			valid_command = true;
			bool valid_value = false;
			u32 mem_location = 0;
			std::string hex_string = command.substr(5);

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
				for(u32 x = 0; x < 16; x++)
				{
					std::cout<<"0x" << (mem_location + x) << "\t" << debug_get_mnemonic(mem_location + x) << "\n";
				}

				db_unit.last_command = "dz";
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

		//Run emulation for a given amount of instructions before halting
		else if((command.substr(0, 2) == "ri") && (command.substr(3, 2) == "0x"))
		{
			valid_command = true;
			bool valid_value = false;
			u32 instruction_count = 0;
			std::string hex_string = command.substr(5);

			//Convert hex string into usable u32
			valid_command = util::from_hex_str(hex_string, instruction_count);

			//Request valid input again
			if(!valid_command)
			{
				std::cout<<"\nInvalid memory address : " << command << "\n";
				std::cout<<": ";
				std::getline(std::cin, command);
			}

			else
			{
				std::cout<<"\n";
				db_unit.run_count = instruction_count;
				valid_command = true;
				db_unit.last_command = "ri";
			}
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

		//Display current ROM bank (if any)
		else if(command == "rom")
		{
			std::cout<<"Current ROM Bank: 0x" << core_mmu.rom_bank << "\n";

			valid_command = true;
			db_unit.last_command = "rom";
			debug_process_command();
		}

		//Display current RAM bank (if any)
		else if(command == "ram")
		{
			u32 ram_bank = 0;
	
			switch(core_mmu.cart.mbc_type)
			{
				case DMG_MMU::MBC1: ram_bank = (core_mmu.cart.multicart) ? core_mmu.bank_bits : 0; break;
				case DMG_MMU::MBC2: ram_bank = 0; break;
				case DMG_MMU::MBC3:
				case DMG_MMU::MBC5:
				case DMG_MMU::MMM01:
				case DMG_MMU::GB_CAMERA:
				case DMG_MMU::HUC1: ram_bank = core_mmu.bank_bits; break;
			} 

			std::cout<<"Current RAM Bank: 0x" << ram_bank << "\n";

			valid_command = true;
			db_unit.last_command = "ram";
			debug_process_command();
		}

		//Print help information
		else if(command == "h")
		{
			std::cout<<"n \t\t Run next Fetch-Decode-Execute stage\n";
			std::cout<<"c \t\t Continue until next breakpoint\n";
			std::cout<<"bp \t\t Set breakpoint, format 0x1234\n";
			std::cout<<"bc \t\t Set breakpoint on memory change, format 0x1234 for addr, 0x12 for value\n";

			//Advanced debugging
			#ifdef GBE_DEBUG
			std::cout<<"bw \t\t Set breakpoint on memory write, format 0x1234 for addr\n";
			std::cout<<"br \t\t Set breakpoint on memory read, format 0x1234 for addr\n";
			#endif

			std::cout<<"del \t\t Deletes ALL current breakpoints\n";
			std::cout<<"u8 \t\t Show BYTE @ memory, format 0x1234\n";
			std::cout<<"u8s \t\t Show 16 BYTES @ memory, format 0x1234\n";
			std::cout<<"u16 \t\t Show WORD @ memory, format 0x1234\n";
			std::cout<<"w8 \t\t Write BYTE @ memory, format 0x1234 for addr, 0x12 for value\n";
			std::cout<<"w16 \t\t Write WORD @ memory, format 0x1234 for addr, 0x1234 for value\n";
			std::cout<<"reg \t\t Change register value (0-9) \n";
			std::cout<<"rom \t\t Display current ROM bank (if any) \n";
			std::cout<<"ram \t\t Display current RAM bank (if any) \n";
			std::cout<<"dz \t\t Disassembles some GBZ80 instructions, format 0x1234 for addr\n";
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
std::string DMG_core::debug_get_mnemonic(u32 addr)
{
	addr &= 0xFFFF;
	u8 opcode = core_mmu.read_u8(addr);
	u8 op1 = core_mmu.read_u8(addr + 1);
	u16 op2 = core_mmu.read_u16(addr + 1);

	switch(opcode)
	{
		case 0x0: return "NOP";
		case 0x1: return "LD BC, " + util::to_hex_str(op2);
		case 0x2: return "LD (BC), A";
		case 0x3: return "INC BC";
		case 0x4: return "INC B";
		case 0x5: return "DEC B";
		case 0x6: return "LD B, " + util::to_hex_str(op1);
		case 0x7: return "RLC A";
		case 0x8: return "LD (" + util::to_hex_str(op2) + "), SP";
		case 0x9: return "ADD HL, BC";
		case 0xA: return "LD A, (BC)";
		case 0xB: return "DEC BC";
		case 0xC: return "INC C";
		case 0xD: return "DEC C";
		case 0xE: return "LD C, " + util::to_hex_str(op1);
		case 0xF: return "RRC A";
		case 0x10: return "STOP";
		case 0x11: return "LD DE, " + util::to_hex_str(op2);
		case 0x12: return "LD (DE), A";
		case 0x13: return "INC DE";
		case 0x14: return "INC D";
		case 0x15: return "DEC D";
		case 0x16: return "LD D, " + util::to_hex_str(op1);
		case 0x17: return "RL A";
		case 0x18: return "JR " + util::to_hex_str(op1);
		case 0x19: return "ADD HL, DE";
		case 0x1A: return "LD A, (DE)";
		case 0x1B: return "DEC DE";
		case 0x1C: return "INC E";
		case 0x1D: return "DEC E";
		case 0x1E: return "LD E " + util::to_hex_str(op1);
		case 0x1F: return "RR A";
		case 0x20: return "JR NZ, " + util::to_hex_str(op1);
		case 0x21: return "LD HL, " + util::to_hex_str(op2);
		case 0x22: return "LDI (HL), A";
		case 0x23: return "INC HL";
		case 0x24: return "INC H";
		case 0x25: return "DEC H";
		case 0x26: return "LD H, " + util::to_hex_str(op1);
		case 0x27: return "DAA";
		case 0x28: return "JR Z, " + util::to_hex_str(op1);
		case 0x29: return "ADD HL, HL";
		case 0x2A: return "LDI A, (HL)";
		case 0x2B: return "DEC HL";
		case 0x2C: return "INC L";
		case 0x2D: return "DEC L";
		case 0x2E: return "LD L, " + util::to_hex_str(op1);
		case 0x2F: return "CPL";
		case 0x30: return "JR NC, " + util::to_hex_str(op1);
		case 0x31: return "LD SP, " + util::to_hex_str(op2);
		case 0x32: return "LDD (HL), A";
		case 0x33: return "INC SP";
		case 0x34: return "INC (HL)";
		case 0x35: return "DEC (HL)";
		case 0x36: return "LD (HL), " + util::to_hex_str(op1);
		case 0x37: return "SCF";
		case 0x38: return "JR C, " + util::to_hex_str(op1);
		case 0x39: return "ADD HL, SP";
		case 0x3A: return "LDD A, (HL)";
		case 0x3B: return "DEC SP";
		case 0x3C: return "INC A";
		case 0x3D: return "DEC A";
		case 0x3E: return "LD A, " + util::to_hex_str(op1);
		case 0x3F: return "CCF";
		case 0x40: return "LD B, B";
		case 0x41: return "LD B, C";
		case 0x42: return "LD B, D";
		case 0x43: return "LD B, E";
		case 0x44: return "LD B, H";
		case 0x45: return "LD B, L";
		case 0x46: return "LD B, (HL)";
		case 0x47: return "LD B, A";
		case 0x48: return "LD C, B";
		case 0x49: return "LD C, C";
		case 0x4A: return "LD C, D";
		case 0x4B: return "LD C, E";
		case 0x4C: return "LD C, H";
		case 0x4D: return "LD C, L";
		case 0x4E: return "LD C, (HL)";
		case 0x4F: return "LD C, A";
		case 0x50: return "LD D, B";
		case 0x51: return "LD D, C";
		case 0x52: return "LD D, D";
		case 0x53: return "LD D, E";
		case 0x54: return "LD D, H";
		case 0x55: return "LD D, L";
		case 0x56: return "LD D, (HL)";
		case 0x57: return "LD D, A";
		case 0x58: return "LD E, B";
		case 0x59: return "LD E, C";
		case 0x5A: return "LD E, D";
		case 0x5B: return "LD E, E";
		case 0x5C: return "LD E, H";
		case 0x5D: return "LD E, L";
		case 0x5E: return "LD E, (HL)";
		case 0x5F: return "LD E, A";
		case 0x60: return "LD H, B";
		case 0x61: return "LD H, C";
		case 0x62: return "LD H, D";
		case 0x63: return "LD H, E";
		case 0x64: return "LD H, H";
		case 0x65: return "LD H, L";
		case 0x66: return "LD H, (HL)";
		case 0x67: return "LD H, A";
		case 0x68: return "LD L, B";
		case 0x69: return "LD L, C";
		case 0x6A: return "LD L, D";
		case 0x6B: return "LD L, E";
		case 0x6C: return "LD L, H";
		case 0x6D: return "LD L, L";
		case 0x6E: return "LD L, (HL)";
		case 0x6F: return "LD L, A";
		case 0x70: return "LD (HL), B";
		case 0x71: return "LD (HL), C";
		case 0x72: return "LD (HL), D";
		case 0x73: return "LD (HL), E";
		case 0x74: return "LD (HL), H";
		case 0x75: return "LD (HL), L";
		case 0x76: return "HALT";
		case 0x77: return "LD (HL), A";
		case 0x78: return "LD A, B";
		case 0x79: return "LD A, C";
		case 0x7A: return "LD A, D";
		case 0x7B: return "LD A, E";
		case 0x7C: return "LD A, H";
		case 0x7D: return "LD A, L";
		case 0x7E: return "LD A, (HL)";
		case 0x7F: return "LD A, A";
		case 0x80: return "ADD A, B";
		case 0x81: return "ADD A, C";
		case 0x82: return "ADD A, D";
		case 0x83: return "ADD A, E";
		case 0x84: return "ADD A, H";
		case 0x85: return "ADD A, L";
		case 0x86: return "ADD A, (HL)";
		case 0x87: return "ADD A, A";
		case 0x88: return "ADC A, B";
		case 0x89: return "ADC A, C";
		case 0x8A: return "ADC A, D";
		case 0x8B: return "ADC A, E";
		case 0x8C: return "ADC A, H";
		case 0x8D: return "ADC A, L";
		case 0x8E: return "ADC A, (HL)";
		case 0x8F: return "ADC A, A";
		case 0x90: return "SUB A, B";
		case 0x91: return "SUB A, C";
		case 0x92: return "SUB A, D";
		case 0x93: return "SUB A, E";
		case 0x94: return "SUB A, H";
		case 0x95: return "SUB A, L";
		case 0x96: return "SUB A, (HL)";
		case 0x97: return "SUB A, A";
		case 0x98: return "SBC A, B";
		case 0x99: return "SBC A, C";
		case 0x9A: return "SBC A, D";
		case 0x9B: return "SBC A, E";
		case 0x9C: return "SBC A, H";
		case 0x9D: return "SBC A, L";
		case 0x9E: return "SBC A, (HL)";
		case 0x9F: return "SBC A, A";
		case 0xA0: return "AND B";
		case 0xA1: return "AND C";
		case 0xA2: return "AND D";
		case 0xA3: return "AND E";
		case 0xA4: return "AND H";
		case 0xA5: return "AND L";
		case 0xA6: return "AND (HL)";
		case 0xA7: return "AND A";
		case 0xA8: return "XOR B";
		case 0xA9: return "XOR C";
		case 0xAA: return "XOR D";
		case 0xAB: return "XOR E";
		case 0xAC: return "XOR H";
		case 0xAD: return "XOR L";
		case 0xAE: return "XOR (HL)";
		case 0xAF: return "XOR A";
		case 0xB0: return "OR B";
		case 0xB1: return "OR C";
		case 0xB2: return "OR D";
		case 0xB3: return "OR E";
		case 0xB4: return "OR H";
		case 0xB5: return "OR L";
		case 0xB6: return "OR (HL)";
		case 0xB7: return "OR A";
		case 0xB8: return "CP B";
		case 0xB9: return "CP C";
		case 0xBA: return "CP D";
		case 0xBB: return "CP E";
		case 0xBC: return "CP H";
		case 0xBD: return "CP L";
		case 0xBE: return "CP (HL)";
		case 0xBF: return "CP A";
		case 0xC0: return "RET NZ";
		case 0xC1: return "POP BC";
		case 0xC2: return "JP NZ, " + util::to_hex_str(op2);
		case 0xC3: return "JP " + util::to_hex_str(op2);
		case 0xC4: return "CALL NZ, " + util::to_hex_str(op2);
		case 0xC5: return "PUSH BC";
		case 0xC6: return "ADD A, " + util::to_hex_str(op1);
		case 0xC7: return "RST 0";
		case 0xC8: return "RET Z";
		case 0xC9: return "RET";
		case 0xCA: return "JP Z, " + util::to_hex_str(op2);
		case 0xCC: return "CALL Z, " + util::to_hex_str(op2);
		case 0xCD: return "CALL " + util::to_hex_str(op2);
		case 0xCE: return "ADC A, " + util::to_hex_str(op1);
		case 0xCF: return "RST 8";
		case 0xD0: return "RET NC";
		case 0xD1: return "POP DE";
		case 0xD2: return "JP NC, " + util::to_hex_str(op2);
		case 0xD4: return "CALL NC, " + util::to_hex_str(op2);
		case 0xD5: return "PUSH DE";
		case 0xD6: return "SUB A, " + util::to_hex_str(op1);
		case 0xD7: return "RST 10";
		case 0xD8: return "RET C";
		case 0xD9: return "RETI";
		case 0xDA: return "JP C, " + util::to_hex_str(op2);
		case 0xDC: return "CALL C, " + util::to_hex_str(op2);
		case 0xDE: return "SBC A, " + util::to_hex_str(op1);
		case 0xDF: return "RST 18";
		case 0xE0: return "LDH (" + util::to_hex_str(0xFF00 | op1) + "), A";
		case 0xE1: return "POP HL";
		case 0xE2: return (config::use_external_interfaces) ? "LDH (0xFF00 + C), A" : "LDH (" + util::to_hex_str(0xFF00 | core_cpu.reg.c) + "), A";
		case 0xE5: return "PUSH HL";
		case 0xE6: return "AND " + util::to_hex_str(op1);
		case 0xE7: return "RST 20";
		case 0xE8: return "ADD SP, " + util::to_hex_str(op1);
		case 0xE9: return "JP HL";
		case 0xEA: return "LD (" + util::to_hex_str(op2) + "), A";
		case 0xEE: return "XOR " + util::to_hex_str(op1);
		case 0xEF: return "RST 28";
		case 0xF0: return "LDH A, (" + util::to_hex_str(0xFF00 | op1) + ")";
		case 0xF1: return "POP AF";
		case 0xF2: return (config::use_external_interfaces) ? "LDH A, (0xFF00 + C)" : "LDH A, (" + util::to_hex_str(0xFF00 | core_cpu.reg.c) + ")";
		case 0xF3: return "DI";
		case 0xF5: return "PUSH AF";
		case 0xF6: return "OR " + util::to_hex_str(op1);
		case 0xF7: return "RST 30";
		case 0xF8: return "LDHL SP, " + util::to_hex_str(op1);
		case 0xF9: return "LD SP, HL";
		case 0xFA: return "LD A, (" + util::to_hex_str(op2) + ")";
		case 0xFB: return "EI";
		case 0xFE: return "CP " + util::to_hex_str(op1);
		case 0xFF: return "RST 38";

		//Extended Ops
		case 0xCB:
			switch(op1)
			{
				case 0x00: return "RLC B";
				case 0x01: return "RLC C";
				case 0x02: return "RLC D";
				case 0x03: return "RLC E";
				case 0x04: return "RLC H";
				case 0x05: return "RLC L";
				case 0x06: return "RLC (HL)";
				case 0x07: return "RLC A";
				case 0x08: return "RLC B";
				case 0x09: return "RRC C";
				case 0x0A: return "RRC D";
				case 0x0B: return "RRC E";
				case 0x0C: return "RRC H";
				case 0x0D: return "RRC L";
				case 0x0E: return "RRC (HL)";
				case 0x0F: return "RRC A";
				case 0x10: return "RL B";
				case 0x11: return "RL C";
				case 0x12: return "RL D";
				case 0x13: return "RL E";
				case 0x14: return "RL H";
				case 0x15: return "RL L";
				case 0x16: return "RL (HL)";
				case 0x17: return "RL A";
				case 0x18: return "RR B";
				case 0x19: return "RR C";
				case 0x1A: return "RR D";
				case 0x1B: return "RR E";
				case 0x1C: return "RR H";
				case 0x1D: return "RR L";
				case 0x1E: return "RR (HL)";
				case 0x1F: return "RR A";
				case 0x20: return "SLA B";
				case 0x21: return "SLA C";
				case 0x22: return "SLA D";
				case 0x23: return "SLA E";
				case 0x24: return "SLA H";
				case 0x25: return "SLA L";
				case 0x26: return "SLA (HL)";
				case 0x27: return "SLA A";
				case 0x28: return "SRA B";
				case 0x29: return "SRA C";
				case 0x2A: return "SRA D";
				case 0x2B: return "SRA E";
				case 0x2C: return "SRA H";
				case 0x2D: return "SRA L";
				case 0x2E: return "SRA (HL)";
				case 0x2F: return "SRA A";
				case 0x30: return "SWAP B";
				case 0x31: return "SWAP C";
				case 0x32: return "SWAP D";
				case 0x33: return "SWAP E";
				case 0x34: return "SWAP H";
				case 0x35: return "SWAP L";
				case 0x36: return "SWAP (HL)";
				case 0x37: return "SWAP A";
				case 0x38: return "SRL B";
				case 0x39: return "SRL C";
				case 0x3A: return "SRL D";
				case 0x3B: return "SRL E";
				case 0x3C: return "SRL H";
				case 0x3D: return "SRL L";
				case 0x3E: return "SRL (HL)";
				case 0x3F: return "SRL A";
				case 0x40: return "BIT 0, B";
				case 0x41: return "BIT 0, C";
				case 0x42: return "BIT 0, D";
				case 0x43: return "BIT 0, E";
				case 0x44: return "BIT 0, H";
				case 0x45: return "BIT 0, L";
				case 0x46: return "BIT 0, (HL)";
				case 0x47: return "BIT 0, A";
				case 0x48: return "BIT 1, B";
				case 0x49: return "BIT 1, C";
				case 0x4A: return "BIT 1, D";
				case 0x4B: return "BIT 1, E";
				case 0x4C: return "BIT 1, H";
				case 0x4D: return "BIT 1, L";
				case 0x4E: return "BIT 1, (HL)";
				case 0x4F: return "BIT 1, A";
				case 0x50: return "BIT 2, B";
				case 0x51: return "BIT 2, C";
				case 0x52: return "BIT 2, D";
				case 0x53: return "BIT 2, E";
				case 0x54: return "BIT 2, H";
				case 0x55: return "BIT 2, L";
				case 0x56: return "BIT 2, (HL)";
				case 0x57: return "BIT 2, A";
				case 0x58: return "BIT 3, B";
				case 0x59: return "BIT 3, C";
				case 0x5A: return "BIT 3, D";
				case 0x5B: return "BIT 3, E";
				case 0x5C: return "BIT 3, H";
				case 0x5D: return "BIT 3, L";
				case 0x5E: return "BIT 3, (HL)";
				case 0x5F: return "BIT 3, A";
				case 0x60: return "BIT 4, B";
				case 0x61: return "BIT 4, C";
				case 0x62: return "BIT 4, D";
				case 0x63: return "BIT 4, E";
				case 0x64: return "BIT 4, H";
				case 0x65: return "BIT 4, L";
				case 0x66: return "BIT 4, (HL)";
				case 0x67: return "BIT 4, A";
				case 0x68: return "BIT 5, B";
				case 0x69: return "BIT 5, C";
				case 0x6A: return "BIT 5, D";
				case 0x6B: return "BIT 5, E";
				case 0x6C: return "BIT 5, H";
				case 0x6D: return "BIT 5, L";
				case 0x6E: return "BIT 5, (HL)";
				case 0x6F: return "BIT 5, A";
				case 0x70: return "BIT 6, B";
				case 0x71: return "BIT 6, C";
				case 0x72: return "BIT 6, D";
				case 0x73: return "BIT 6, E";
				case 0x74: return "BIT 6, H";
				case 0x75: return "BIT 6, L";
				case 0x76: return "BIT 6, (HL)";
				case 0x77: return "BIT 6, A";
				case 0x78: return "BIT 7, B";
				case 0x79: return "BIT 7, C";
				case 0x7A: return "BIT 7, D";
				case 0x7B: return "BIT 7, E";
				case 0x7C: return "BIT 7, H";
				case 0x7D: return "BIT 7, L";
				case 0x7E: return "BIT 7, (HL)";
				case 0x7F: return "BIT 7, A";
				case 0x80: return "RES 0, B";
				case 0x81: return "RES 0, C";
				case 0x82: return "RES 0, D";
				case 0x83: return "RES 0, E";
				case 0x84: return "RES 0, H";
				case 0x85: return "RES 0, L";
				case 0x86: return "RES 0, (HL)";
				case 0x87: return "RES 0, A";
				case 0x88: return "RES 1, B";
				case 0x89: return "RES 1, C";
				case 0x8A: return "RES 1, D";
				case 0x8B: return "RES 1, E";
				case 0x8C: return "RES 1, H";
				case 0x8D: return "RES 1, L";
				case 0x8E: return "RES 1, (HL)";
				case 0x8F: return "RES 1, A";
				case 0x90: return "RES 2, B";
				case 0x91: return "RES 2, C";
				case 0x92: return "RES 2, D";
				case 0x93: return "RES 2, E";
				case 0x94: return "RES 2, H";
				case 0x95: return "RES 2, L";
				case 0x96: return "RES 2, (HL)";
				case 0x97: return "RES 2, A";
				case 0x98: return "RES 3, B";
				case 0x99: return "RES 3, C";
				case 0x9A: return "RES 3, D";
				case 0x9B: return "RES 3, E";
				case 0x9C: return "RES 3, H";
				case 0x9D: return "RES 3, L";
				case 0x9E: return "RES 3, (HL)";
				case 0x9F: return "RES 3, A";
				case 0xA0: return "RES 4, B";
				case 0xA1: return "RES 4, C";
				case 0xA2: return "RES 4, D";
				case 0xA3: return "RES 4, E";
				case 0xA4: return "RES 4, H";
				case 0xA5: return "RES 4, L";
				case 0xA6: return "RES 4, (HL)";
				case 0xA7: return "RES 4, A";
				case 0xA8: return "RES 5, B";
				case 0xA9: return "RES 5, C";
				case 0xAA: return "RES 5, D";
				case 0xAB: return "RES 5, E";
				case 0xAC: return "RES 5, H";
				case 0xAD: return "RES 5, L";
				case 0xAE: return "RES 5, (HL)";
				case 0xAF: return "RES 5, A";
				case 0xB0: return "RES 6, B";
				case 0xB1: return "RES 6, C";
				case 0xB2: return "RES 6, D";
				case 0xB3: return "RES 6, E";
				case 0xB4: return "RES 6, H";
				case 0xB5: return "RES 6, L";
				case 0xB6: return "RES 6, (HL)";
				case 0xB7: return "RES 6, A";
				case 0xB8: return "RES 7, B";
				case 0xB9: return "RES 7, C";
				case 0xBA: return "RES 7, D";
				case 0xBB: return "RES 7, E";
				case 0xBC: return "RES 7, H";
				case 0xBD: return "RES 7, L";
				case 0xBE: return "RES 7, (HL)";
				case 0xBF: return "RES 7, A";
				case 0xC0: return "SET 0, B";
				case 0xC1: return "SET 0, C";
				case 0xC2: return "SET 0, D";
				case 0xC3: return "SET 0, E";
				case 0xC4: return "SET 0, H";
				case 0xC5: return "SET 0, L";
				case 0xC6: return "SET 0, (HL)";
				case 0xC7: return "SET 0, A";
				case 0xC8: return "SET 1, B";
				case 0xC9: return "SET 1, C";
				case 0xCA: return "SET 1, D";
				case 0xCB: return "SET 1, E";
				case 0xCC: return "SET 1, H";
				case 0xCD: return "SET 1, L";
				case 0xCE: return "SET 1, (HL)";
				case 0xCF: return "SET 1, A";
				case 0xD0: return "SET 2, B";
				case 0xD1: return "SET 2, C";
				case 0xD2: return "SET 2, D";
				case 0xD3: return "SET 2, E";
				case 0xD4: return "SET 2, H";
				case 0xD5: return "SET 2, L";
				case 0xD6: return "SET 2, (HL)";
				case 0xD7: return "SET 2, A";
				case 0xD8: return "SET 3, B";
				case 0xD9: return "SET 3, C";
				case 0xDA: return "SET 3, D";
				case 0xDB: return "SET 3, E";
				case 0xDC: return "SET 3, H";
				case 0xDD: return "SET 3, L";
				case 0xDE: return "SET 3, (HL)";
				case 0xDF: return "SET 3, A";
				case 0xE0: return "SET 4, B";
				case 0xE1: return "SET 4, C";
				case 0xE2: return "SET 4, D";
				case 0xE3: return "SET 4, E";
				case 0xE4: return "SET 4, H";
				case 0xE5: return "SET 4, L";
				case 0xE6: return "SET 4, (HL)";
				case 0xE7: return "SET 4, A";
				case 0xE8: return "SET 5, B";
				case 0xE9: return "SET 5, C";
				case 0xEA: return "SET 5, D";
				case 0xEB: return "SET 5, E";
				case 0xEC: return "SET 5, H";
				case 0xED: return "SET 5, L";
				case 0xEE: return "SET 5, (HL)";
				case 0xEF: return "SET 5, A";
				case 0xF0: return "SET 6, B";
				case 0xF1: return "SET 6, C";
				case 0xF2: return "SET 6, D";
				case 0xF3: return "SET 6, E";
				case 0xF4: return "SET 6, H";
				case 0xF5: return "SET 6, L";
				case 0xF6: return "SET 6, (HL)";
				case 0xF7: return "SET 6, A";
				case 0xF8: return "SET 7, B";
				case 0xF9: return "SET 7, C";
				case 0xFA: return "SET 7, D";
				case 0xFB: return "SET 7, E";
				case 0xFC: return "SET 7, H";
				case 0xFD: return "SET 7, L";
				case 0xFE: return "SET 7, (HL)";
				case 0xFF: return "SET 7, A";
			}

		default: return "UNK INSTR";
	}
}
 
