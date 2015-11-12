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
#include "SDL/SDL_opengl.h"
#include "mmu.h"

#ifndef GBA_LCD
#define GBA_LCD

class AGB_LCD
{
	public:
	
	//Link to memory map
	AGB_MMU* mem;

	u8 lcd_mode;
	u8 current_scanline;

	//Core Functions
	AGB_LCD();
	~AGB_LCD();

	void step();
	void reset();
	bool init();
	void opengl_init();
	void update();
	void clear_screen_buffer(u32 color);

	//Screen data
	SDL_Surface* final_screen;
	GLuint lcd_texture;

	agb_lcd_data lcd_stat;

	private:

	void update_oam();
	void update_palettes();
	void update_obj_render_list();

	void opengl_blit();

	struct oam_entries
	{
		//X-Y Coordinates - X (0-511), Y(0-255)
		u16 x;
		u8 y;

		u16 right;
		u16 left;

		u8 top;
		u8 bottom;
	
		//Horizonal and vertical flipping options
		bool h_flip;
		bool v_flip;

		//Shape and size, dimensions
		u8 shape;
		u8 size;
		u8 width;
		u8 height;

		//Misc properties
		u32 addr;
		u16 tile_number;
		u8 bg_priority;
		u8 bit_depth;
		u8 palette_number;
		u8 type;
		u8 mode;
		u8 rotate_scale;
		bool visible;
		bool mosiac;
	} obj[128];

	u8 obj_render_list[128];
	u8 obj_render_length;
	u8 last_obj_priority;
	u8 last_obj_mode;
	u8 last_bg_priority;
	u16 last_raw_color;
	bool obj_win_pixel;

	u32 pal[256][2];
	u16 raw_pal[256][2];
	u16 bg_offset_x[4];
	u16 bg_offset_y[4];

	//Screen pixel buffer
	std::vector<u32> scanline_buffer;
	std::vector<u32> screen_buffer;

	u32 lcd_clock;
	u32 scanline_pixel_counter;

	int frame_start_time;
	int frame_current_time;
	int fps_count;
	int fps_time;

	void render_scanline();
	bool render_sprite_pixel();
	bool render_bg_pixel(u32 bg_control);
	bool render_bg_mode_0(u32 bg_control);
	bool render_bg_mode_1(u32 bg_control);
	bool render_bg_mode_3();
	bool render_bg_mode_4();
	bool render_bg_mode_5();
	void scanline_compare();

	void apply_sfx();
	u32 brightness_up();
	u32 brightness_down();
	u32 alpha_blend();
};

#endif // GBA_LCD