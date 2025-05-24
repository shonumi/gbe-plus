// GB Enhanced+ Copyright Daniel Baxter 2020
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : core.h
// Date : December 02, 2020
// Description : Emulator core
//
// The "Core" is an abstraction for all of emulated components
// It controls the large-scale behavior of the CPU, LCD/GPU, MMU, and APU/DSP
// Can start, stop, and reset emulator
// Also contains a debugging unit


#ifndef PM_CORE
#define PM_CORE

#include "common/core_emu.h"
#include "mmu.h"
#include "s1c88.h"

class MIN_core : virtual public core_emu
{
	public:
		MIN_core();
		~MIN_core();

		//Core control
		void start();
		void stop();
		void reset();
		void shutdown();
		void step();
		void handle_hotkey(SDL_Event& event);
		void handle_hotkey(int input, bool pressed);
		void process_keypad_irqs();
		void update_volume(u8 volume);
		void feed_key_input(int sdl_key, bool pressed);
		void save_state(u8 slot);
		void load_state(u8 slot);
		bool get_save_state_info(u32 offset, std::string filename);
		bool set_save_state_info(std::string filename);
		void run_core();

		//Core debugging
		void debug_step();
		void debug_display() const;
		void debug_process_command();
		std::string debug_get_mnemonic(u32 addr);

		//CPU related functions
		u32 ex_get_reg(u8 reg_index);

		//MMU related functions
		bool read_file(std::string filename);
		bool read_bios(std::string filename);
		bool read_firmware(std::string filename);
		u8 ex_read_u8(u16 address);
		void ex_write_u8(u16 address, u8 value);

		//Netplay interface
		void start_netplay();
		void stop_netplay();
		void hard_sync();

		//Misc
		u32 get_core_data(u32 core_index);

		MIN_MMU core_mmu;
		S1C88 core_cpu;
		MIN_GamePad core_pad;
};
		
#endif // PM_CORE 
