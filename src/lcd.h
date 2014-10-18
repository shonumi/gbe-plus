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

	void update_oam();

	struct oam_entries
	{
		//X-Y Coordinates - X (0-511), Y(0-255)
		u16 x;
		u8 y;
	
		//Horizonal and vertical flipping options
		bool h_flip;
		bool v_flip;

		//Shape and size, dimensions
		u8 shape;
		u8 size;
		u8 width;
		u8 height;

		//Misc properties
		u16 tile_number;
		u8 bg_priority;
		u8 bit_depth;
		u8 palette_number;
	} obj[128];
		
	//Screen data
	SDL_Surface* final_screen;
	SDL_Surface* internal_screen;

	//Screen pixel buffer
	std::vector<u32> scanline_buffer;
	std::vector<u32> screen_buffer;

	u32 lcd_clock;
	u32 scanline_pixel_counter;
	u8 current_scanline;

	int frame_start_time;
	int frame_current_time;
	int fps_count;
	int fps_time;

	void render_scanline();
	bool render_sprite_pixel();
	bool render_bg_pixel(u32 bg_control);
};

#endif // GBA_LCD