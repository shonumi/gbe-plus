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
	u32 display_control_a;
	u32 display_control_b;
	
	u8 bg_mode_a;
	u8 bg_mode_b;

	u8 display_mode_a;
	u8 display_mode_b;

	u16 bg_control[4];
	bool hblank_interval_free;

	u32 vram_bank_addr[9];
	bool vram_bank_enable[9];

	u16 bg_offset_x[4];
	u16 bg_offset_y[4];
	u8 bg_priority[4];
	u8 bg_depth[4];
	u8 bg_size[4];
	bool bg_enable[4];
	u32 bg_base_map_addr[4];
	u32 bg_base_tile_addr[4];

	u16 bg_pal_a[256];
	u16 raw_bg_pal_a[256];

	bool vblank_irq_enable;
	bool hblank_irq_enable;
	bool vcount_irq_enable;

	bool bg_pal_update_a;
	std::vector<bool> bg_pal_update_list_a;

	bool obj_pal_update_a;
	std::vector<bool> obj_pal_update_list_a;
};

#endif // NDS_LCD_DATA
