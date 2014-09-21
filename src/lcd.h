// GB Enhanced Copyright Daniel Baxter 2013
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd.h
// Date : August 16, 2014
// Description : Game Boy Advance LCD emulation
//
// Draws background, window, and sprites to screen
// Responsible for blitting pixel data and limiting frame rate

#include "SDL/SDL.h"
#include "mmu.h"

#ifndef GBA_LCD
#define GBA_LCD

class LCD
{
	public:
	
	//Link to memory map
	MMU* mem;

	u8 lcd_mode;

	//Core Functions
	LCD();
	~LCD();

	void step();
	void reset();
	bool init();

	private:

	//Screen data
	SDL_Surface* final_screen;
	SDL_Surface* internal_screen;

	//Screen pixel buffer
	std::vector<u32> scanline_buffer;
	std::vector<u32> screen_buffer;

	u32 lcd_clock;
	u32 scanline_pixel_counter;
	u32 current_scanline;

	int frame_start_time;
	int frame_current_time;
	int fps_count;
	int fps_time;

	void render_scanline();
};

#endif // GBA_LCD