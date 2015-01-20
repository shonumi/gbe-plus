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

#include "common.h"

struct lcd_data
{
	u16 display_control;
	u16 bg_control[4];

	u16 bg_offset_x[4];
	u16 bg_offset_y[4];
	u8 bg_priority[4];

	u16 mode_0_width[4];
	u16 mode_0_height[4];
};

#endif // GBA_LCD_DATA