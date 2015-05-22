// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd.h
// Date : May 15, 2014
// Description : Game Boy LCD emulation
//
// Draws background, window, and sprites to screen
// Responsible for blitting pixel data and limiting frame rate

#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"
#include "mmu.h"

#ifndef GB_LCD
#define GB_LCD

class DMG_LCD
{
	public:
	
	//Link to memory map
	DMG_MMU* mem;

	//Core Functions
	DMG_LCD();
	~DMG_LCD();

	void step(int cpu_clock);
	void reset();
	bool init();
	bool opengl_init();

	//Screen data
	SDL_Surface* final_screen;
	GLuint lcd_texture;

	dmg_lcd_data lcd_stat;

	private:

	struct oam_entries
	{
		//X-Y Coordinates
		u8 x;
		u8 y;
	
		//Horizonal and vertical flipping options
		bool h_flip;
		bool v_flip;

		//Dimensions
		u8 height;

		//Misc properties
		u8 tile_number;
		u8 bg_priority;
		u8 bit_depth;
		u8 palette_number;
		
	} obj[40];

	u8 obj_render_list[8];
	u8 obj_render_length;

	//Screen pixel buffer
	std::vector<u32> scanline_buffer;
	std::vector<u32> screen_buffer;

	int frame_start_time;
	int frame_current_time;
	int fps_count;
	int fps_time;

	void update_oam();
	void update_obj_render_list();

	void render_dmg_scanline();
	bool render_dmg_sprite_pixel();
	bool render_dmg_bg_pixel();

	void scanline_compare();

	void opengl_blit();
};

#endif // GB_LCD 
