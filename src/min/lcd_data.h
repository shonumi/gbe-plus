// GB Enhanced+ Copyright Daniel Baxter 2021
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd_data.h
// Date : March 11, 2021
// Description : Core data
//
// Defines the LCD data structures that the MMU will update whenever values are written in memory
// Only the LCD should read values from this namespace. Only the MMU should write values to this namespace.

#ifndef PM_LCD_DATA
#define PM_LCD_DATA

#include "common.h"

struct min_lcd_data
{
	u8 prc_counter;
	u8 prc_rate;
	u8 prc_rate_div;
	u32 prc_clock;
	u8 prc_mode;
	u8 prc_copy_wait;

	u8 map_size;
	u32 map_addr;
	u32 obj_addr;

	u8 scroll_x;
	u8 scroll_y;

	bool invert_map;
	bool enable_map;
	bool enable_obj;
	bool enable_copy;
	bool force_update;
};

#endif // PM_LCD_DATA