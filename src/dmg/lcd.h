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

#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"
#include "mmu.h"
#include "custom_graphics_data.h"

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
	void opengl_init();

	//Custom GFX functions
	bool load_manifest(std::string filename);
	bool load_image_data();

	void dump_dmg_obj(u8 obj_index);
	void dump_dmg_bg(u16 bg_index);

	void dump_gbc_obj(u8 obj_index);
	void dump_gbc_bg(u16 bg_index);

	void update_dmg_obj_hash(u8 obj_index);
	void update_dmg_bg_hash(u16 bg_index);

	void update_gbc_obj_hash(u8 obj_index);
	void update_gbc_bg_hash(u16 map_addr);

	bool has_hash(std::string hash);

	//Screen data
	SDL_Surface* final_screen;
	GLuint lcd_texture;

	dmg_lcd_data lcd_stat;
	dmg_cgfx_data cgfx_stat;

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
	std::vector<u32> hd_screen_buffer;
	std::vector<u32> scanline_raw;
	std::vector<u32> scanline_priority;

	int frame_start_time;
	int frame_current_time;
	int fps_count;
	int fps_time;

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

	//Per-scanline rendering - DMG (CGFX)
	void render_cgfx_dmg_obj_scanline(u8 sprite_id);
	void render_cgfx_dmg_bg_scanline(u16 bg_id);

	//Per-scanline rendering - GBC
	void render_gbc_scanline();
	void render_gbc_bg_scanline();
	void render_gbc_win_scanline();
	void render_gbc_obj_scanline();

	//Per-scanline rendering - DMG (CGFX)
	void render_cgfx_gbc_obj_scanline(u8 sprite_id);
	void render_cgfx_gbc_bg_scanline(u16 bg_id);

	//Per-pixel rendering - DMG (B/W)
	bool render_dmg_obj_pixel();
	bool render_dmg_bg_pixel();

	//GBC DMAs
	void hdma();
	void gdma();

	void scanline_compare();

	void opengl_blit();
};

#endif // GB_LCD 
