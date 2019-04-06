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

#include "SDL2/SDL.h"
#include "SDL2/SDL_opengl.h"
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
	bool load_meta_data();
	bool find_meta_data();

	void dump_dmg_obj(u8 obj_index);
	void dump_dmg_bg(u16 bg_index);

	void dump_gbc_obj(u8 obj_index);
	void dump_gbc_bg(u16 bg_index);

	void update_dmg_obj_hash(u8 obj_index);
	void update_dmg_bg_hash(u16 bg_index);

	void update_gbc_obj_hash(u8 obj_index);
	void update_gbc_bg_hash(u16 map_addr);

	void render_scanline(u8 line, u8 type);
	u32 get_scanline_pixel(u8 pixel);

	bool has_hash(u16 addr, std::string hash);
	std::string get_hash(u16 addr, u8 gfx_type);
	u32 adjust_pixel_brightness(u32 color, u8 palette_id, u8 gfx_type);
	void invalidate_cgfx();

	//Serialize data for save state loading/saving
	bool lcd_read(u32 offset, std::string filename);
	bool lcd_write(std::string filename);

	//Screen data
	SDL_Window *window;
	SDL_Surface* final_screen;
	SDL_Surface* original_screen;

	//OpenGL data
	SDL_GLContext gl_context;
	GLuint lcd_texture;
	GLuint program_id;
	GLuint vertex_buffer_object, vertex_array_object, element_buffer_object;
	GLfloat ogl_x_scale, ogl_y_scale;
	GLfloat ext_data_1, ext_data_2;
	u32 external_data_usage;

	dmg_lcd_data lcd_stat;
	dmg_cgfx_data cgfx_stat;

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
	std::vector<u32> hd_screen_buffer;
	std::vector<u8> scanline_raw;
	std::vector<u8> scanline_priority;
	std::vector<u32> stretched_buffer;

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
	void render_cgfx_dmg_bg_scanline(u16 bg_id, bool is_bg);

	//Per-scanline rendering - GBC
	void render_gbc_scanline();
	void render_gbc_bg_scanline();
	void render_gbc_win_scanline();
	void render_gbc_obj_scanline();

	//Per-scanline rendering - GBC (CGFX)
	void render_cgfx_gbc_obj_scanline(u8 sprite_id);
	void render_cgfx_gbc_bg_scanline(u16 tile_data, u8 bg_map_attribute, bool is_bg);

	//Per-pixel rendering - DMG (B/W)
	bool render_dmg_obj_pixel();
	bool render_dmg_bg_pixel();

	void scanline_compare();

	void opengl_blit();
};

#endif // GB_LCD 
