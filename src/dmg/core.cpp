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
	//core_cpu.controllers.audio.mem = &core_mmu;
	//core_mmu.set_apu_data(&core_cpu.controllers.audio.apu_stat);

	//Link MMU and GamePad
	core_cpu.mem->g_pad = &core_pad;

	//Link MMU and CPU's timers
	//core_mmu.timer = &core_cpu.controllers.timer;

	db_unit.debug_mode = false;
	db_unit.display_cycles = false;
	db_unit.last_command = "n";
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
	//if(!core_cpu.controllers.audio.init())
	//{
	//	running = false;
	//	core_cpu.running = false;
	//}

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

/****** Reset the core ******/
void DMG_core::reset()
{
	core_cpu.reset();
	core_cpu.controllers.video.reset();
	//core_cpu.controllers.audio.reset();
	core_mmu.reset();

	//Link CPU and MMU
	core_cpu.mem = &core_mmu;

	//Link LCD and MMU
	core_cpu.controllers.video.mem = &core_mmu;

	//Link APU and MMU
	//core_cpu.controllers.audio.mem = &core_mmu;

	//Link MMU and GamePad
	core_cpu.mem->g_pad = &core_pad;

	//Link MMU and CPU's timers
	//core_mmu.timer = &core_cpu.controllers.timer;

	//Re-read specified ROM file
	core_mmu.read_file(config::rom_file);

	//Start everything all over again
	start();
}

/****** Run the core in a loop until exit ******/
void DMG_core::run_core()
{
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
			if(db_unit.debug_mode) { debug_step(); }

			core_cpu.cycles = 0;

			//Handle Interrupts
			core_cpu.handle_interrupts();
	
			//Halt CPU if necessary
			if(core_cpu.halt == true) { core_cpu.cycles += 4; }

			//Process Opcodes
			else 
			{
				core_cpu.opcode = core_mmu.read_u8(core_cpu.reg.pc++);
				core_cpu.exec_op(core_cpu.opcode);
			}

			//Update LCD
			core_cpu.controllers.video.step(core_cpu.cycles);

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
						core_mmu.memory_map[REG_IF] |= 0x04;
						core_mmu.memory_map[REG_TIMA] = core_mmu.memory_map[REG_TMA];
					}	

				}
			}

		}

		//Stop emulation
		else { stop(); }
	}

	//Shutdown core's components
	core_mmu.DMG_MMU::~DMG_MMU();
	core_cpu.Z80::~Z80();
}

/****** Debugger - Allow core to run until a breaking condition occurs ******/
void DMG_core::debug_step()
{
	//In continue mode, if breakpoints exist, try to stop on one
	if((db_unit.breakpoints.size() > 0) && (db_unit.last_command == "c"))
	{
		for(int x = 0; x < db_unit.breakpoints.size(); x++)
		{
			//When a BP is matched, display info, wait for next input command
			if(core_cpu.reg.pc == db_unit.breakpoints[x])
			{
				debug_display();
				debug_process_command();
			}
		}

	}

	//When in next instruction mode, simply display info, wait for next input command
	else if(db_unit.last_command == "n")
	{
		debug_display();
		debug_process_command();
	}
}

/****** Debugger - Display relevant info to the screen ******/
void DMG_core::debug_display() const
{
	std::cout << std::hex << "CPU::Executing Opcode : 0x" << (u32)core_cpu.opcode << "\n\n";

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
		std::string save_name = "";

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

/****** Read binary file to memory ******/
bool DMG_core::read_file(std::string filename) { return core_mmu.read_file(filename); }

/****** Read BIOS file into memory ******/
bool DMG_core::read_bios(std::string filename) { return core_mmu.read_bios(config::bios_file); }
 