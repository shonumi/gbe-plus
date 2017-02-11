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
	u16 current_scanline;
	u32 lcd_clock;
	u8 lcd_mode;

	u16 lyc_a;
	u16 lyc_b;

	u32 display_control_a;
	u32 display_control_b;

	u16 display_stat_a;
	u16 display_stat_b;
	
	u8 bg_mode_a;
	u8 bg_mode_b;

	u8 display_mode_a;
	u8 display_mode_b;

	u16 bg_control_a[4];
	u16 bg_control_b[4];

	bool hblank_interval_free;

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

	u8 bg_priority_a[4];
	u8 bg_priority_b[4];

	bool bg_enable_a[4];
	bool bg_enable_b[4];
	
	u32 bg_pal_a[256];
	u16 raw_bg_pal_a[256];

	u32 bg_pal_b[256];
	u16 raw_bg_pal_b[256];

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

	bool update_bg_control_a;
	bool update_bg_control_b;
};

#endif // NDS_LCD_DATA
