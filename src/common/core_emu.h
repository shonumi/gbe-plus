// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : core_emu.h
// Date : May 8, 2015
// Description : Abstracted core interface
//
// Emulator core interface that can be used for different systems (GB, GBC, GBA)
// The interface is abstracted, so it can be applied to different consoles

#ifndef CORE_EMU
#define CORE_EMU

#include <SDL.h>
#include <string>
#include <vector>

#include "common/common.h"

class core_emu
{
	public:

	core_emu() {};
	~core_emu() {};

	//Core control
	virtual void start() = 0;
	virtual void stop() = 0;
	virtual void reset() = 0;
	virtual void shutdown() = 0;
	virtual void run_core() = 0;
	virtual void step() = 0;
	virtual	void handle_hotkey(SDL_Event& event) = 0;
	virtual void handle_hotkey(int input, bool pressed) = 0;
	virtual void update_volume(u8 volume) = 0;
	virtual void feed_key_input(int sdl_key, bool pressed) = 0;
	virtual	void save_state(u8 slot) = 0;
	virtual	void load_state(u8 slot) = 0;

	//Core debugging
	virtual	void debug_step() = 0;
	virtual	void debug_display() const = 0;
	virtual	void debug_process_command() = 0;
	virtual std::string debug_get_mnemonic(u32 addr) = 0;

	//CPU related functions
	virtual u32 ex_get_reg(u8 reg_index) = 0;

	//MMU related functions
	virtual bool read_file(std::string filename) = 0;
	virtual bool read_bios(std::string filename) = 0;
	virtual bool read_firmware(std::string filename) = 0;
	virtual u8 ex_read_u8(u16 address) = 0;
	virtual void ex_write_u8(u16 address, u8 value) = 0;

	//Netplay interface
	virtual void start_netplay() = 0;
	virtual void stop_netplay() = 0;

	//Misc
	virtual u32 get_core_data(u32 core_index) = 0;

	bool running;
	SDL_Event event;
	
	struct debugging
	{
		bool debug_mode;
		bool display_cycles;
		bool print_all;
		bool print_pc;
		std::vector <u32> breakpoints;
		std::vector <u32> watchpoint_addr;
		std::vector <u32> watchpoint_val;
		std::vector <u32> watchpoint_old_val;
		std::string last_command;
		std::string last_mnemonic;
		u32 last_pc;
		u32 run_count;
		u8 vb_count;

		//Advanced debugging
		#ifdef GBE_DEBUG
		std::vector <u32> write_addr;
		std::vector <u32> read_addr;
		#endif
	} db_unit;
};

#endif // CORE_EMU