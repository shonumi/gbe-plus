// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd_data.h
// Date : January 20, 2015
// Description : Core data
//
// Defines the LCD data structures that the MMU will update whenever values are written in memory
// Only the LCD should read values from this namespace. Only the MMU should write values to this namespace.

#ifndef GBA_LCD_DATA
#define GBA_LCD_DATA

#include <vector>

#include "common.h"

struct lcd_data
{
	u16 display_control;
	u16 bg_control[4];
	u32 frame_base;
	u8 bg_mode;

	u16 bg_offset_x[4];
	u16 bg_offset_y[4];
	u8 bg_priority[4];
	u8 bg_depth[4];
	u8 bg_size[4];
	bool bg_enable[4];
	u32 bg_base_map_addr[4];
	u32 bg_base_tile_addr[4];

	bool window_enable[2];
	bool window_in_enable[6][2];
	bool windown_out_enable[6][2];

	u16 window_x1[2];
	u16 window_x2[2];
	u16 window_y1[2];
	u16 window_y2[2];

	u16 mode_0_width[4];
	u16 mode_0_height[4];

	u8 bg_flip_lut[256];
	u16 bg_tile_lut[256][256];
	u16 bg_num_lut[256][256];
	u16 screen_offset_lut[512];

	bool oam_update;
	std::vector<bool> oam_update_list;

	bool bg_pal_update;
	std::vector<bool> bg_pal_update_list;

	bool obj_pal_update;
	std::vector<bool> obj_pal_update_list;

	bool bg_params_update;
};

#endif // GBA_LCD_DATA
