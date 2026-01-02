// GB Enhanced Copyright Daniel Baxter 2021
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd.h
// Date : March 10, 2021
// Description : Pokemon Mini LCD emulation
//
// Draws background and sprites to screen
// Responsible for blitting pixel data and limiting frame rate

#ifndef PM_LCD
#define PM_LCD

#include "SDL.h"
#include "mmu.h"

#include "common/gx_util.h"

class MIN_LCD
{
	public:
	
	//Link to memory map
	MIN_MMU* mem;

	//Core Functions
	MIN_LCD();
	~MIN_LCD();

	void update();
	void reset();
	bool init();
	bool opengl_init();

	//Screen data
	SDL_Window* window;
	SDL_Surface* final_screen;
	SDL_Surface* original_screen;

	min_lcd_data lcd_stat;

	bool new_frame;
	int max_fullscreen_ratio;

	u32 on_colors[64];
	u32 off_colors[64];
	u32 mix_colors[64];

	//Serialize data for save state loading/saving
	bool lcd_read(u32 offset, std::string filename);
	bool lcd_write(std::string filename);

	private:

	void render_map();
	void render_obj();
	void render_frame();

	//Screen pixel buffer
	std::vector<u32> screen_buffer;
	std::vector<u32> old_buffer;

	int frame_start_time;
	int frame_current_time;
	int fps_count;
	int fps_time;
	int frame_delay[72];

	bool try_window_rebuild;
};

#endif // PM_LCD 
