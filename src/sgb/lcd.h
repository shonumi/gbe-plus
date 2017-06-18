// GB Enhanced+ Copyright Daniel Baxter 2017
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd.h
// Date : June 16, 2017
// Description : Super Game Boy "LCD" i.e. TV-OUT emulation
//
// Draws background, window, and sprites to screen
// Responsible for blitting pixel data and limiting frame rate

#ifndef SGB_VID
#define SGB_VID

#include "SDL2/SDL.h"
#include "SDL2/SDL_opengl.h"
#include "dmg/mmu.h"

class SGB_LCD
{
	public:
	
	//Link to memory map
	DMG_MMU* mem;

	//Core Functions
	SGB_LCD();
	~SGB_LCD();

	void step(int cpu_clock);
	void reset();
	bool init();
	void opengl_init();

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

	int max_fullscreen_ratio;

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

	int frame_start_time;
	int frame_current_time;
	int fps_count;
	int fps_time;

	//OAM updates
	void update_oam();
	void update_obj_render_list();

	//Per-scanline rendering
	void render_sgb_scanline();
	void render_sgb_bg_scanline();
	void render_sgb_win_scanline();
	void render_sgb_obj_scanline();

	void scanline_compare();

	void opengl_blit();
};

#endif // SGB_VID 
