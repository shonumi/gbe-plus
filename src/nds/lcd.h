// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd.h
// Date : December 28, 2015
// Description : NDS LCD emulation
//
// Draws background, window, and sprites to screen
// Responsible for blitting pixel data and limiting frame rate

#include "SDL2/SDL.h"
#include "SDL2/SDL_opengl.h"
#include "mmu.h"

#ifndef NDS_LCD
#define NDS_LCD

class NTR_LCD
{
	public:
	
	//Link to memory map
	NTR_MMU* mem;

	u8 lcd_mode;

	//Core Functions
	NTR_LCD();
	~NTR_LCD();

	void step();
	void reset();
	bool init();
	void update();

	//Screen data
	SDL_Window* window;
	SDL_Surface* final_screen;
	SDL_Surface* original_screen;

	SDL_GLContext gl_context;
	GLuint lcd_texture;

	ntr_lcd_data lcd_stat;

	private:

	void update_palettes();

	//Screen pixel buffer
	std::vector<u32> scanline_buffer_a;
	std::vector<u32> scanline_buffer_b;
	std::vector<u32> screen_buffer;

	u32 lcd_clock;
	u32 scanline_pixel_counter;

	int frame_start_time;
	int frame_current_time;
	int fps_count;
	int fps_time;

	void render_scanline();
	bool render_bg_scanline(u32 bg_control);
	bool render_bg_mode_0(u32 bg_control);
	bool render_bg_mode_1(u32 bg_control);
	bool render_bg_mode_3(u32 bg_control);
	bool render_bg_mode_4(u32 bg_control);
	bool render_bg_mode_5(u32 bg_control);
	bool render_bg_mode_6(u32 bg_control);
	void scanline_compare();
};

#endif // NDS_LCD