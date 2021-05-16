// GB Enhanced Copyright Daniel Baxter 2021
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd.h
// Date : March 10, 2021
// Description : Pokemon Mini LCD emulation
//
// Draws background and sprites to screen
// Responsible for blitting pixel data and limiting frame rate

#include "SDL2/SDL.h"
#include "SDL2/SDL_opengl.h"
#include "mmu.h"

#ifndef PM_LCD
#define PM_LCD

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

	//OpenGL data
	#ifdef GBE_OGL
	SDL_GLContext gl_context;
	GLuint lcd_texture;
	GLuint program_id;
	GLuint vertex_buffer_object, vertex_array_object, element_buffer_object;
	GLfloat ogl_x_scale, ogl_y_scale;
	GLfloat ext_data_1, ext_data_2;
	u32 external_data_usage;
	#endif

	min_lcd_data lcd_stat;

	bool new_frame;
	int max_fullscreen_ratio;

	private:

	void render_map();
	void render_obj();
	void render_frame();

	void opengl_blit();

	//Screen pixel buffer
	std::vector<u32> screen_buffer;
	std::vector<u32> old_buffer;

	int frame_start_time;
	int frame_current_time;
	int fps_count;
	int fps_time;
};

#endif // PM_LCD 
