// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd.h
// Date : May 15, 2014
// Description : Game Boy LCD emulation
//
// Draws background, window, and sprites to screen
// Responsible for blitting pixel data and limiting frame rate

#ifndef GB_LCD
#define GB_LCD

#include "SDL.h"
#include "SDL_opengl.h"
#include "mmu.h"

#include "common/gx_util.h"

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

	void render_scanline(u8 line, u8 type);
	u32 get_scanline_pixel(u8 pixel);

	//Serialize data for save state loading/saving
	bool lcd_read(u32 offset, std::string filename);
	bool lcd_write(std::string filename);

	//Screen data
	SDL_Window *window;
	SDL_Surface* final_screen;
	SDL_Surface* original_screen;

	dmg_lcd_data lcd_stat;

	int max_fullscreen_ratio;

	bool power_antenna_osd;

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
		u8 vram_bank;
		u8 color_palette_number;
		
	} obj[40];

	u8 obj_render_list[10];
	int obj_render_length;

	//Screen pixel buffer
	std::vector<u32> scanline_buffer;
	std::vector<u32> screen_buffer;
	std::vector<u8> scanline_raw;
	std::vector<u8> scanline_priority;
	std::vector<u32> stretched_buffer;

	int frame_start_time;
	int frame_current_time;
	int fps_count;
	int fps_time;
	int frame_delay[60];

	bool try_window_rebuild;

	//OAM updates
	void update_oam();
	void update_obj_render_list();

	//GBC color palette updates
	void update_bg_colors();
	void update_obj_colors();

	//Per-scanline rendering - DMG (B/W)
	void render_dmg_scanline();
	void render_dmg_bg_scanline();
	void render_dmg_win_scanline();
	void render_dmg_obj_scanline();

	//Per-scanline rendering - GBC
	void render_gbc_scanline();
	void render_gbc_bg_scanline();
	void render_gbc_win_scanline();
	void render_gbc_obj_scanline();

	//Per-pixel rendering - DMG (B/W)
	bool render_dmg_obj_pixel();
	bool render_dmg_bg_pixel();

	void scanline_compare();
};

#endif // GB_LCD 
