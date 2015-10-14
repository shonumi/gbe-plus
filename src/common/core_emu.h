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

#include <SDL/SDL.h>
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
	virtual	void handle_hotkey(SDL_Event& event) = 0;
	virtual void update_volume(u8 volume) = 0;
	virtual void feed_key_input(int sdl_key, bool pressed) = 0;

	//Core debugging
	virtual	void debug_step() = 0;
	virtual	void debug_display() const = 0;
	virtual	void debug_process_command() = 0;

	//MMU related functions
	virtual bool read_file(std::string filename) = 0;
	virtual bool read_bios(std::string filename) = 0;
	virtual u8 ex_read_u8(u16 address) = 0;
	virtual void ex_write_u8(u16 address, u8 value) = 0;

	//CGFX interface
	virtual void dump_obj(int obj_index) = 0;
	virtual void dump_bg(int bg_index) = 0;
	virtual u32* get_obj_palette(int pal_index) = 0;
	virtual u32* get_bg_palette(int pal_index) = 0;

	bool running;
	SDL_Event event;
	
	struct debugging
	{
		bool debug_mode;
		bool display_cycles;
		bool print_all;
		std::vector <u32> breakpoints;
		std::string last_command;
	} db_unit;
};

#endif // CORE_EMU