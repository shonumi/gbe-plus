// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : core.h
// Date : August 16, 2014
// Description : Emulator core
//
// The "Core" is an abstraction for all of emulated components
// It controls the large-scale behavior of the CPU, LCD/GPU, MMU, and APU/DSP
// Can start, stop, and reset emulator
// Also contains a debugging unit


#ifndef GBA_CORE
#define GBA_CORE

#include "common/core_emu.h"
#include "mmu.h"
#include "arm7.h"

class AGB_core : virtual public core_emu
{
	public:
		AGB_core();
		~AGB_core();

		//Core control
		void start();
		void stop();
		void reset();
		void handle_hotkey(SDL_Event& event);
		void run_core();

		//Core debugging
		void debug_step();
		void debug_display() const;
		void debug_process_command();

		//MMU related functions
		bool read_file(std::string filename);
		bool read_bios(std::string filename);

		AGB_MMU core_mmu;
		ARM7 core_cpu;
		GamePad core_pad;
};
		
#endif // GBA_CORE