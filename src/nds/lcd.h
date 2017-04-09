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

	//Core Functions
	NTR_LCD();
	~NTR_LCD();

	void step();
	void reset();
	bool init();
	void opengl_init();
	void update();

	//Screen data
	SDL_Window* window;
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

	ntr_lcd_data lcd_stat;

	private:

	struct oam_entries
	{
		//X-Y Coordinates - X (0-511), Y(0-255)
		u16 x;
		u8 y;

		s16 right;
		s16 left;

		s16 top;
		s16 bottom;
	
		//Horizonal and vertical flipping options
		bool h_flip;
		bool v_flip;

		bool x_wrap;
		bool y_wrap;

		u8 x_wrap_val;
		u8 y_wrap_val;

		//Shape and size, dimensions
		u8 shape;
		u8 size;
		u8 width;
		u8 height;

		//Transformed dimensions via affine
		s32 affine_width;
		s32 affine_height;

		s16 cx, cy;
		s16 cw, ch;

		//Misc properties
		u32 addr;
		u16 tile_number;
		u8 bg_priority;
		u8 bit_depth;
		u8 palette_number;
		u8 type;
		u8 mode;
		u8 affine_enable;
		u8 affine_group;
		bool visible;
		bool mosiac;
	} obj[256];

	void update_palettes();
	void update_oam();
	void update_obj_render_list();

	void opengl_blit();

	//OBJ rendering
	u8 obj_render_list_a[128];
	u8 obj_render_length_a;

	u8 obj_render_list_b[128];
	u8 obj_render_length_b;

	//Screen pixel buffer
	std::vector<u32> scanline_buffer_a;
	std::vector<u32> scanline_buffer_b;
	std::vector<u32> screen_buffer;

	//Render buffer
	std::vector<u8> render_buffer_a;
	std::vector<u8> render_buffer_b;

	bool full_scanline_render_a;
	bool full_scanline_render_b;

	u32 scanline_pixel_counter;

	int frame_start_time;
	int frame_current_time;
	int fps_count;
	int fps_time;

	void render_scanline();
	void render_bg_scanline(u32 bg_control);
	void render_bg_mode_text(u32 bg_control);
	void render_bg_mode_affine(u32 bg_control);
	void render_bg_mode_affine_ext(u32 bg_control);
	void render_bg_mode_bitmap(u32 bg_control);
	void render_bg_mode_direct(u32 bg_control);
	void render_obj_scanline(u32 bg_control);
	void scanline_compare();
	void reload_affine_references(u32 bg_control);
};

#endif // NDS_LCD
