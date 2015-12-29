// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd_data.h
// Date : December 28, 2015
// Description : Core data
//
// Defines the LCD data structures that the MMU will update whenever values are written in memory
// Only the LCD should read values from this namespace. Only the MMU should write values to this namespace.

#ifndef NDS_LCD_DATA
#define NDS_LCD_DATA

#include <vector>

#include "common.h"

struct ntr_lcd_data
{
	u16 display_control;
	u16 bg_control[4];
	u8 vram_control[9];
	u8 bg_mode;
	bool hblank_interval_free;

	u16 bg_offset_x[4];
	u16 bg_offset_y[4];
	u8 bg_priority[4];
	u8 bg_depth[4];
	u8 bg_size[4];
	bool bg_enable[4];
	u32 bg_base_map_addr[4];
	u32 bg_base_tile_addr[4];

	u32 pal[256][2];
	u16 raw_pal[256][2];

	bool bg_pal_update;
	std::vector<bool> bg_pal_update_list;

	bool obj_pal_update;
	std::vector<bool> obj_pal_update_list;
};

#endif // NDS_LCD_DATA
