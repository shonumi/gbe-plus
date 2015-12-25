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

	//Link MMU and GamePad
	core_cpu.mem->g_pad = &core_pad;

	db_unit.debug_mode = false;
	db_unit.display_cycles = false;
	db_unit.last_command = "n";
	db_unit.last_mnemonic = "";

	std::cout<<"GBE::Launching DMG-GBC core\n";
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

	//Re-read specified ROM file
	core_mmu.read_file(config::rom_file);

	//Start everything all over again
	start();
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
			core_cpu.cycles = 0;

			//Handle Interrupts
			core_cpu.handle_interrupts();

			if(db_unit.debug_mode) { debug_step(); }
	
			//Halt CPU if necessary
			if(core_cpu.halt == true) { core_cpu.cycles += 4; }

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

		}

		//Stop emulation
		else { stop(); }
	}
	
	//Shutdown core
	shutdown();
}

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

	//When in next instruction mode, simply display info, wait for next input command
	else if(db_unit.last_command == "n")
	{
		db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.reg.pc);
		core_cpu.opcode = core_mmu.read_u8(core_cpu.reg.pc);

		debug_display();
		debug_process_command();
		printed = true;
	}

	//Display every instruction when print all is enabled
	if((!printed) && (db_unit.print_all)) 
	{
		db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.reg.pc);
		core_cpu.opcode = core_mmu.read_u8(core_cpu.reg.pc);
		debug_display();
	} 
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

	std::cout<< std::hex <<"FLAGS : 0x" << std::setw(2) << std::setfill('0') << (u32)core_cpu.reg.f << "\t" << flag_stats << "\n\n";
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
			std::string hex_char = "";
			u32 hex_size = (hex_string.size() - 1);

			//Convert hex string into usable u32
			for(int x = hex_size, y = 0; x >= 0; x--, y+=4)
			{
				hex_char = hex_string[x];

				if(hex_char == "0") { bp += (0 << y); }
				else if(hex_char == "1") { bp += (1 << y); }
				else if(hex_char == "2") { bp += (2 << y); }
				else if(hex_char == "3") { bp += (3 << y); }
				else if(hex_char == "4") { bp += (4 << y); }
				else if(hex_char == "5") { bp += (5 << y); }
				else if(hex_char == "6") { bp += (6 << y); }
				else if(hex_char == "7") { bp += (7 << y); }
				else if(hex_char == "8") { bp += (8 << y); }
				else if(hex_char == "9") { bp += (9 << y); }
				else if(hex_char == "A") { bp += (10 << y); }
				else if(hex_char == "a") { bp += (10 << y); }
				else if(hex_char == "B") { bp += (11 << y); }
				else if(hex_char == "b") { bp += (11 << y); }
				else if(hex_char == "C") { bp += (12 << y); }
				else if(hex_char == "c") { bp += (12 << y); }
				else if(hex_char == "D") { bp += (13 << y); }
				else if(hex_char == "d") { bp += (13 << y); }
				else if(hex_char == "E") { bp += (14 << y); }
				else if(hex_char == "e") { bp += (14 << y); }
				else if(hex_char == "F") { bp += (15 << y); }
				else if(hex_char == "f") { bp += (15 << y); }
				else { valid_command = false; }
			}
		
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

		//Show memory
		else if((command.substr(0, 2) == "sm") && (command.substr(3, 2) == "0x"))
		{
			valid_command = true;
			u32 mem_location = 0;
			std::string hex_string = command.substr(5);
			std::string hex_char = "";
			u32 hex_size = (hex_string.size() - 1);

			//Convert hex string into usable u32
			for(int x = hex_size, y = 0; x >= 0; x--, y+=4)
			{
				hex_char = hex_string[x];

				if(hex_char == "0") { mem_location += (0 << y); }
				else if(hex_char == "1") { mem_location += (1 << y); }
				else if(hex_char == "2") { mem_location += (2 << y); }
				else if(hex_char == "3") { mem_location += (3 << y); }
				else if(hex_char == "4") { mem_location += (4 << y); }
				else if(hex_char == "5") { mem_location += (5 << y); }
				else if(hex_char == "6") { mem_location += (6 << y); }
				else if(hex_char == "7") { mem_location += (7 << y); }
				else if(hex_char == "8") { mem_location += (8 << y); }
				else if(hex_char == "9") { mem_location += (9 << y); }
				else if(hex_char == "A") { mem_location += (10 << y); }
				else if(hex_char == "a") { mem_location += (10 << y); }
				else if(hex_char == "B") { mem_location += (11 << y); }
				else if(hex_char == "b") { mem_location += (11 << y); }
				else if(hex_char == "C") { mem_location += (12 << y); }
				else if(hex_char == "c") { mem_location += (12 << y); }
				else if(hex_char == "D") { mem_location += (13 << y); }
				else if(hex_char == "d") { mem_location += (13 << y); }
				else if(hex_char == "E") { mem_location += (14 << y); }
				else if(hex_char == "e") { mem_location += (14 << y); }
				else if(hex_char == "F") { mem_location += (15 << y); }
				else if(hex_char == "f") { mem_location += (15 << y); }
				else { valid_command = false; }
			}

			//Request valid input again
			if(!valid_command)
			{
				std::cout<<"\nInvalid memory address : " << command << "\n";
				std::cout<<": ";
				std::getline(std::cin, command);
			}

			else
			{
				db_unit.last_command = "sm";
				std::cout<<"Memory @ " << hex_string << " : 0x" << std::hex << (int)core_mmu.read_u8(mem_location) << "\n";
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
			//core_cpu.debug_cycles = 0;
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
			std::cout<<"sm \t\t Show memory, format 0x1234ABCD\n";
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
std::string DMG_core::debug_get_mnemonic(u32 addr)
{
	addr &= 0xFFFF;
	u8 opcode = core_mmu.read_u8(addr);
	u8 op1 = core_mmu.read_u8(addr + 1);
	u16 op2 = core_mmu.read_u16(addr + 1);

	switch(opcode)
	{
		case 0x0: return "NOP";
		case 0x1: return "LD BC, " + util::to_hex_str(op1);
		case 0x2: return "LD (BC), A";
		case 0x3: return "INC BC";
		case 0x4: return "INC B";
		case 0x5: return "DEC B";
		case 0x6: return "LD B, " + util::to_hex_str(op1);
		case 0x7: return "RLC A";
		case 0x8: return "LD " + util::to_hex_str(op1) + ", SP";
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
		case 0xC0: return "RET NZ, " + util::to_hex_str(op1);
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
		case 0xCB: return "EXT OPS";
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
		case 0xE2: return "LDH (" + util::to_hex_str(0xFF00 | core_cpu.reg.c) + "), A";
		case 0xE5: return "PUSH HL";
		case 0xE6: return "AND " + util::to_hex_str(op1);
		case 0xE7: return "RST 20";
		case 0xE8: return "ADD SP, " + util::to_hex_str(op1);
		case 0xE9: return "JP HL";
		case 0xEA: return "LD " + util::to_hex_str(op2) + ", A";
		case 0xEE: return "XOR " + util::to_hex_str(op1);
		case 0xEF: return "RST 28";
		case 0xF0: return "LDH A, " + util::to_hex_str(0xFF00 | op1);
		case 0xF1: return "POP AF";
		case 0xF2: return "LDH A, " + util::to_hex_str(0xFF00 | core_cpu.reg.c);
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
		default: return "UNK INSTR";
	}
}
	
/****** Process hotkey input ******/
void DMG_core::handle_hotkey(SDL_Event& event)
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
		//Switch flags
		if(config::flags == 0x80000000) { config::flags = 0; }
		else { config::flags = 0x80000000; }

		//Initialize the screen
		if(!config::use_opengl)
		{
			core_cpu.controllers.video.final_screen = SDL_SetVideoMode(160, 144, 32, SDL_SWSURFACE | config::flags);
		}

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
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_TAB)) { config::turbo = true; }

	//Toggle turbo off
	else if((event.type == SDL_KEYUP) && (event.key.keysym.sym == SDLK_TAB)) { config::turbo = false; }
		
	//Reset emulation on F8
	else if((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_F8)) { reset(); }
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
