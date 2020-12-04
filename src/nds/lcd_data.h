// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd_data.h
// Date : December 28, 2015
// Description : Core data
//
// Defines the LCD data structures that the MMU will update whenever values are written in memory
// Only the LCD should read values from this namespace. Only the MMU should write values to this namespace.
// 2D and 3D data structures are separated for organizational purposes

#ifndef NDS_LCD_DATA
#define NDS_LCD_DATA

#include <vector>

#include "common.h"

enum nds_sfx_types
{
	NDS_NORMAL,
	NDS_ALPHA_BLEND,
	NDS_BRIGHTNESS_UP,
	NDS_BRIGHTNESS_DOWN,
};

struct ntr_lcd_data
{
	u16 current_scanline;
	u32 lcd_clock;
	u8 lcd_mode;

	u16 lyc_nds9;
	u16 lyc_nds7;

	u32 display_control_a;
	u32 display_control_b;

	u16 display_stat_nds9;
	u16 display_stat_nds7;
	
	u8 bg_mode_a;
	u8 bg_mode_b;

	u8 display_mode_a;
	u8 display_mode_b;

	u8 ext_pal_a;
	u8 ext_pal_b;

	u16 obj_boundary_a;
	u16 obj_boundary_b;

	u16 bg_control_a[4];
	u16 bg_control_b[4];

	bool hblank_interval_free;
	
	u16 master_bright;
	u16 old_master_bright;

	bool forced_blank_a;
	bool forced_blank_b;

	u32 vram_bank_addr[9];
	bool vram_bank_enable[9];

	u16 bg_offset_x_a[4];
	u16 bg_offset_x_b[4];

	u16 bg_offset_y_a[4];
	u16 bg_offset_y_b[4];

	u8 bg_depth_a[4];
	u8 bg_depth_b[4];

	u8 bg_size_a[4];
	u8 bg_size_b[4];

	u16 text_width_a[4];
	u16 text_width_b[4];
	u16 text_height_a[4];
	u16 text_height_b[4];

	u32 bg_base_map_addr_a[4];
	u32 bg_base_map_addr_b[4];

	u32 bg_base_tile_addr_a[4];
	u32 bg_base_tile_addr_b[4];

	u32 bg_bitmap_base_addr_a[2];
	u32 bg_bitmap_base_addr_b[2];
	
	u8 bg_priority_a[4];
	u8 bg_priority_b[4];

	bool bg_enable_a[4];
	bool bg_enable_b[4];
	
	u32 bg_pal_a[256];
	u16 raw_bg_pal_a[256];

	u32 bg_pal_b[256];
	u16 raw_bg_pal_b[256];

	u32 bg_ext_pal_a[0x4000];
	u16 raw_bg_ext_pal_a[0x4000];

	u32 bg_ext_pal_b[0x4000];
	u16 raw_bg_ext_pal_b[0x4000];

	u32 obj_pal_a[256];
	u16 raw_obj_pal_a[256];

	u32 obj_pal_b[256];
	u16 raw_obj_pal_b[256];

	u32 obj_ext_pal_a[4096];
	u16 raw_obj_ext_pal_a[4096];

	u32 obj_ext_pal_b[4096];
	u16 raw_obj_ext_pal_b[4096];

	struct bg_affine_parameters_a
	{
		//Parameters, X-Y reference
		float dx, dmx, dy, dmy;
		float x_ref, y_ref;
		float x_pos, y_pos;

		bool overflow;
	} bg_affine_a[2];

	struct bg_affine_parameters_b
	{
		//Parameters, X-Y reference
		float dx, dmx, dy, dmy;
		float x_ref, y_ref;
		float x_pos, y_pos;

		bool overflow;
	} bg_affine_b[2];

	float obj_affine[256];

	bool sfx_target_a[6][2];
	bool sfx_target_b[6][2];

	nds_sfx_types current_sfx_type_a;
	nds_sfx_types current_sfx_type_b;

	double brightness_coef_a;
	double brightness_coef_b;

	double alpha_coef_a[2];
	double alpha_coef_b[2];

	u16 window_x_a[2][2];
	u16 window_x_b[2][2];

	u16 window_y_a[2][2];
	u16 window_y_b[2][2];

	bool window_enable_a[2];
	bool window_enable_b[2];

	bool obj_win_enable_a;
	bool obj_win_enable_b;

	bool window_in_enable_a[6][2];
	bool window_in_enable_b[6][2];

	bool window_out_enable_a[6][2];
	bool window_out_enable_b[6][2];

	bool window_status_a[256][2];
	bool window_status_b[256][2];
	
	bool window_id_a[256];
	bool window_id_b[256];

	u8 current_window_a;
	u8 current_window_b;

	u32 cap_cnt;
	u8 capture_slot;
	bool cap_started;
	bool cap_finished;

	bool vblank_irq_enable_a;
	bool hblank_irq_enable_a;
	bool vcount_irq_enable_a;

	bool vblank_irq_enable_b;
	bool hblank_irq_enable_b;
	bool vcount_irq_enable_b;

	bool bg_pal_update_a;
	std::vector<bool> bg_pal_update_list_a;

	bool bg_pal_update_b;
	std::vector<bool> bg_pal_update_list_b;

	bool obj_pal_update_a;
	std::vector<bool> obj_pal_update_list_a;

	bool obj_pal_update_b;
	std::vector<bool> obj_pal_update_list_b;

	bool bg_ext_pal_update_a;
	std::vector<bool> bg_ext_pal_update_list_a;

	bool bg_ext_pal_update_b;
	std::vector<bool> bg_ext_pal_update_list_b;

	bool obj_ext_pal_update_a;
	std::vector<bool> obj_ext_pal_update_list_a;

	bool obj_ext_pal_update_b;
	std::vector<bool> obj_ext_pal_update_list_b;

	bool update_bg_control_a;
	bool update_bg_control_b;

	bool oam_update;
	std::vector<bool> oam_update_list;
};

struct ntr_lcd_3D_data
{
	u32 display_control;
	u32 gx_stat;

	u8 current_gx_command;
	u32 current_packed_command;
	u32 fifo_params;
	u8 command_parameters[1280];
	u8 parameter_index;
	u8 buffer_id;
	u8 gx_state;
	bool process_command;
	bool packed_command;

	u8 view_port_x1;
	u8 view_port_x2;
	u8 view_port_y1;
	u8 view_port_y2;

	u8 matrix_mode;
	u8 vertex_mode;
	u8 vertex_list_index;

	u8 hi_fill[256];
	u8 lo_fill[256];

	u32 hi_color[256];
	u32 lo_color[256];

	float hi_line_z[256];
	float lo_line_z[256];

	bool render_polygon;
	bool use_texture;
	bool begin_strips;
	bool update_clip_matrix;

	u32 rear_plane_color;
	u8 rear_plane_alpha;
	u32 vertex_color;
	u32 clip_flags;
	u16 poly_count;
	u16 vert_count;

	float last_x;
	float last_y;
	float last_z;

	s32 poly_min_x;
	s32 poly_max_x;

	//Texture Attribute
	u32 tex_offset;
	u32 pal_base;
	u32 pal_bank_addr;
	u16 tex_src_width;
	u16 tex_src_height;
	u8 tex_format;
	u8 tex_transformation;
	bool tex_color_zero;
	bool repeat_tex_x;
	bool repeat_tex_y;
	bool flip_tex_x;
	bool flip_tex_y;
	std::vector <u32> tex_data;

	//Polygon Attribute
	u8 poly_id;
	u8 poly_alpha;
	u8 poly_mode;
	bool poly_new_depth;

	float tex_coord_x[4];
	float tex_coord_y[4];

	float hi_tx[256];
	float lo_tx[256];

	float hi_ty[256];
	float lo_ty[256];
};

#endif // NDS_LCD_DATA
