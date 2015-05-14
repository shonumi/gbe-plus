// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd_data.h
// Date : May 14, 2015
// Description : Core data
//
// Defines the LCD data structures that the MMU will update whenever values are written in memory
// Only the LCD should read values from this namespace. Only the MMU should write values to this namespace.

#ifndef GB_LCD_DATA
#define GB_LCD_DATA

#include <vector>

#include "common.h" 

struct dmg_lcd_data
{
	u8 lcd_control;
	bool lcd_enable;
	bool window_enable;
	u16 window_map_addr;
	u16 bg_map_addr;
	u16 bg_tile_addr;

	u8 bg_scroll_x;
	u8 bg_scroll_y;
	u8 window_x;
	u8 window_y;

	u8 signed_tile_lut[256];
};

#endif // GB_LCD_DATA
