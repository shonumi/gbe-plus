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
	gx_matrix temp_matrix(4, 4);

	switch(lcd_3D_stat.current_gx_command)
	{
		//MTX_MODE
		case 0x10:
			lcd_3D_stat.matrix_mode = (lcd_3D_stat.command_parameters[0] & 0x3);
			break;

		//MTX_IDENTITY:
		case 0x15:
			switch(lcd_3D_stat.matrix_mode)
			{
				case 0x0: gx_projection_matrix.make_identity(4); break;
				case 0x1: gx_position_matrix.make_identity(4); break;
				case 0x2: gx_position_vector_matrix.make_identity(4); break;
				case 0x3: gx_texture_matrix.make_identity(4); break;
			}

			break;

		//MTX_MULT_4x4
		case 0x18:
			for(int a = 0; a < 64;)
			{
				u32 raw_value = lcd_3D_stat.command_parameters[a] | (lcd_3D_stat.command_parameters[a+1] << 8) | (lcd_3D_stat.command_parameters[a+2] << 16) | (lcd_3D_stat.command_parameters[a+3] << 24);
				float result = 0.0;
				
				if(raw_value & 0x80000000) 
				{ 
					u32 p = ((raw_value >> 12) - 1);
					p = (~p & 0x7FFFF);
					result = -1.0 * p;
				}

				else { result = (raw_value >> 12); }
				if((raw_value & 0x7FFFF) != 0) { result += (raw_value & 0xFFF) / 4096.0; }

				u8 x = ((a / 4) % 4);
				u8 y = (a / 16);

				temp_matrix.data[x][y] = result;
				a += 4;
			}

			switch(lcd_3D_stat.matrix_mode)
			{
				case 0x0: gx_projection_matrix = temp_matrix * gx_projection_matrix; break;
				case 0x1: gx_position_matrix = temp_matrix * gx_position_matrix; break;
				case 0x2: gx_position_vector_matrix = temp_matrix * gx_position_vector_matrix; break;
				case 0x3: gx_texture_matrix = temp_matrix * gx_texture_matrix; break;
			}

			break;

		//MTX_MULT_4x3
		case 0x19:
			temp_matrix.make_identity(4);

			for(int a = 0; a < 48;)
			{
				u32 raw_value = lcd_3D_stat.command_parameters[a] | (lcd_3D_stat.command_parameters[a+1] << 8) | (lcd_3D_stat.command_parameters[a+2] << 16) | (lcd_3D_stat.command_parameters[a+3] << 24);
				float result = 0.0;
				
				if(raw_value & 0x80000000) 
				{ 
					u32 p = ((raw_value >> 12) - 1);
					p = (~p & 0x7FFFF);
					result = -1.0 * p;
				}

				else { result = (raw_value >> 12); }
				if((raw_value & 0x7FFFF) != 0) { result += (raw_value & 0xFFF) / 4096.0; }

				u8 x = ((a / 4) % 3);
				u8 y = (a / 12);

				temp_matrix.data[x][y] = result;
				a += 4;
			}

			switch(lcd_3D_stat.matrix_mode)
			{
				case 0x0: gx_projection_matrix = temp_matrix * gx_projection_matrix; break;
				case 0x1: gx_position_matrix = temp_matrix * gx_position_matrix; break;
				case 0x2: gx_position_vector_matrix = temp_matrix * gx_position_vector_matrix; break;
				case 0x3: gx_texture_matrix = temp_matrix * gx_texture_matrix; break;
			}

			break;

		//MTX_MULT_3x3
		case 0x1A:
			temp_matrix.make_identity(4);

			for(int a = 0; a < 36;)
			{
				u32 raw_value = lcd_3D_stat.command_parameters[a] | (lcd_3D_stat.command_parameters[a+1] << 8) | (lcd_3D_stat.command_parameters[a+2] << 16) | (lcd_3D_stat.command_parameters[a+3] << 24);
				float result = 0.0;
				
				if(raw_value & 0x80000000) 
				{ 
					u32 p = ((raw_value >> 12) - 1);
					p = (~p & 0x7FFFF);
					result = -1.0 * p;
				}

				else { result = (raw_value >> 12); }
				if((raw_value & 0x7FFFF) != 0) { result += (raw_value & 0xFFF) / 4096.0; }

				u8 x = ((a / 4) % 2);
				u8 y = (a / 12);

				temp_matrix.data[x][y] = result;
				a += 4;
			}

			switch(lcd_3D_stat.matrix_mode)
			{
				case 0x0: gx_projection_matrix = temp_matrix * gx_projection_matrix; break;
				case 0x1: gx_position_matrix = temp_matrix * gx_position_matrix; break;
				case 0x2: gx_position_vector_matrix = temp_matrix * gx_position_vector_matrix; break;
				case 0x3: gx_texture_matrix = temp_matrix * gx_texture_matrix; break;
			}

			break;

		//VIEWPORT
		case 0x60:
			lcd_3D_stat.view_port_y2 = (lcd_3D_stat.command_parameters[0] & 0xBF);
			lcd_3D_stat.view_port_x2 = lcd_3D_stat.command_parameters[1];
			lcd_3D_stat.view_port_y1 = (lcd_3D_stat.command_parameters[2] & 0xBF);
			lcd_3D_stat.view_port_x1 = lcd_3D_stat.command_parameters[3];
			break;
	}

	lcd_3D_stat.parameter_index = 0;
	lcd_3D_stat.current_gx_command = 0;
	lcd_3D_stat.process_command = false;
}
