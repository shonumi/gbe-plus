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
#include "common/gx_util.h"

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

	bool get_cart_icon(SDL_Surface* nds_icon);
	bool save_cart_icon(std::string nds_icon_file);

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
	ntr_lcd_3D_data lcd_3D_stat;

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
	void update_obj_affine_transformation();
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
	std::vector< std::vector<u32> > gx_screen_buffer;

	//Render buffer
	std::vector<u8> render_buffer_a;
	std::vector<u8> render_buffer_b;
	std::vector<u8> gx_render_buffer;
	std::vector<float> gx_z_buffer;

	//Other buffers
	std::vector<u8> sfx_buffer;

	bool full_scanline_render_a;
	bool full_scanline_render_b;

	u32 scanline_pixel_counter;

	int frame_start_time;
	int frame_current_time;
	int fps_count;
	int fps_time;

	u8 inv_lut[8];
	u16 screen_offset_lut[512];

	//3D Polygons
	std::vector<gx_matrix> gx_triangles;
	std::vector<gx_matrix> gx_quads;

	//Matrix Stacks
	std::vector<gx_matrix> gx_projection_stack;
	std::vector<gx_matrix> gx_position_stack;
	std::vector<gx_matrix> gx_vector_stack;
	std::vector<gx_matrix> gx_texture_stack;

	u8 position_sp;
	u8 vector_sp;

	//Vertex properties and attributes
	u32 vert_colors[4];

	//Matrices
	gx_matrix gx_projection_matrix;
	gx_matrix gx_position_matrix;
	gx_matrix gx_vector_matrix;
	gx_matrix gx_texture_matrix;

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

	//3D functions
	void render_bg_3D();
	void render_geometry();
	void process_gx_command();
	void fill_poly_solid();
	bool poly_push(gx_matrix &current_matrix);
	u32 read_param_u32(u8 index);
	u16 read_param_u16(u8 index);
	u32 get_rgb15(u16 color_bytes);

	//SFX functions
	void apply_sfx(u32 bg_control);
	void brightness_up(u32 bg_control);
	void brightness_down(u32 bg_control);
};

#endif // NDS_LCD
