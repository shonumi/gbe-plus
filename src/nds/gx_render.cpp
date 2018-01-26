// GB Enhanced Copyright Daniel Baxter 2018
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gx_render.cpp
// Date : January 25, 2018
// Description : NDS GX emulation
//
// Emulates the NDS 3D graphical hardware via software
// Draws polygons, fills textures, etc.
// Additionally, processes GX commands

#include "lcd.h"
#include "common/util.h"

void NTR_LCD::render_3D()
{

}

void NTR_LCD::process_gx_command()
{
	switch(lcd_3D_stat.current_gx_command)
	{
		//VIEWPORT
		case 0x60:
			lcd_3D_stat.view_port_y2 = lcd_3D_stat.command_parameters[0];
			lcd_3D_stat.view_port_x2 = lcd_3D_stat.command_parameters[1];
			lcd_3D_stat.view_port_y1 = lcd_3D_stat.command_parameters[2];
			lcd_3D_stat.view_port_x1 = lcd_3D_stat.command_parameters[3];
			break;
	}

	lcd_3D_stat.parameter_index = 0;
	lcd_3D_stat.current_gx_command = 0;
	lcd_3D_stat.process_command = false;
}
