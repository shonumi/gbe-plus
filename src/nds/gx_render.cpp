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
	u8 bg_priority = lcd_stat.bg_priority_a[0] + 1;

	//Abort rendering if this BG is disabled
	if(!lcd_stat.bg_enable_a[0]) { return; }

	//Abort rendering if BGs with high priority have already completely rendered a scanline
	if(full_scanline_render_a) { return; }

	bool full_render = true;

	//Software rendering of polygons
	//Triangles only atm
	if(lcd_3D_stat.render_polygon)
	{
		std::cout<<"RENDER\n";

		//Calculate origin coordinates based on viewport dimensions
		u8 viewport_width = (lcd_3D_stat.view_port_x2 - lcd_3D_stat.view_port_x1);
		u8 viewport_height = (lcd_3D_stat.view_port_y2 - lcd_3D_stat.view_port_y1);

		//Plot points used for screen rendering
		u8 plot_x1 = 0;
		u8 plot_x2 = 0;
		u8 plot_y1 = 0;
		u8 plot_y2 = 0;
		u16 buffer_index = 0;
		gx_matrix temp_matrix;
		gx_matrix clip_matrix = gx_position_matrix * gx_projection_matrix;

		//Render lines between all vertices
		for(u8 x = 0; x < 3; x++)
		{
			temp_matrix.resize(1, 4);
			temp_matrix.data[0][0] = gx_triangles[0].data[x][0];
			temp_matrix.data[0][1] = gx_triangles[0].data[x][1];
			temp_matrix.data[0][2] = gx_triangles[0].data[x][2];
			temp_matrix.data[0][3] = 1.0;
			temp_matrix = temp_matrix * clip_matrix;

 			plot_x1 = ((temp_matrix.data[0][0] + temp_matrix.data[0][3]) * viewport_width) / ((2 * temp_matrix.data[0][3]) + lcd_3D_stat.view_port_x1);
  			plot_y1 = ((temp_matrix.data[0][1] + temp_matrix.data[0][3]) * viewport_height) / ((2 * temp_matrix.data[0][3]) + lcd_3D_stat.view_port_y1);

			//Convert plot points to buffer index
			buffer_index = (plot_y1 * 256) + plot_x1;
			if(buffer_index < 0xC000) { gx_screen_buffer[buffer_index] = 0xFFFFFFFF; }
		}

		lcd_3D_stat.render_polygon = false;
	}

	//Push 3D screen buffer data to scanline buffer
	u16 gx_index = (256 * lcd_stat.current_scanline);

	for(u32 x = 0; x < 256; x++)
	{
		scanline_buffer_a[x] = gx_screen_buffer[gx_index + x];
		render_buffer_a[x] = bg_priority;
	} 
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

		//MTX_PUSH
		case 0x11:
			switch(lcd_3D_stat.matrix_mode)
			{
				case 0x0:
					gx_projection_stack[0] = gx_projection_matrix;
					break;

				case 0x1:
				case 0x2:
					gx_position_stack[position_sp++] = gx_position_matrix;
					gx_vector_stack[vector_sp++] = gx_vector_matrix;

					break;

				case 0x3:
					gx_texture_stack[0] = gx_texture_matrix;
					break;
			}

			break;

		//MTX_POP
		case 0x12:
			switch(lcd_3D_stat.matrix_mode)
			{
				case 0x0:
					gx_projection_matrix = gx_projection_stack[0];
					break;

				case 0x1:
				case 0x2:
					{
						s8 test_range = 0;
						s8 offset = (lcd_3D_stat.command_parameters[3] & 0x1F);

						if(offset == 0) { offset++; }
						if((lcd_3D_stat.command_parameters[3] & 0x20) == 0) { offset *= -1; }
						test_range = offset + position_sp;
					
						if((test_range >= 0) && (test_range <= 30))
						{
							position_sp = test_range;
							vector_sp = test_range;

							gx_position_matrix = gx_position_stack[position_sp];
							gx_vector_matrix = gx_vector_stack[vector_sp];

						}
					}

					break;

				case 0x3:
					gx_texture_matrix = gx_texture_stack[0];
					break;
			}

			break;

		//MTX_IDENTITY:
		case 0x15:
			switch(lcd_3D_stat.matrix_mode)
			{
				case 0x0:
					gx_projection_matrix.make_identity(4);
					break;

				case 0x1:
					gx_position_matrix.make_identity(4);
					break;

				case 0x2:
					gx_position_matrix.make_identity(4);
					gx_vector_matrix.make_identity(4);
					break;

				case 0x3:
					gx_texture_matrix.make_identity(4);
					break;
			}

			break;

		//MTX_MULT_4x4
		case 0x18:
			for(int a = 0; a < 64;)
			{
				u32 raw_value = lcd_3D_stat.command_parameters[a+3] | (lcd_3D_stat.command_parameters[a+2] << 8) | (lcd_3D_stat.command_parameters[a+1] << 16) | (lcd_3D_stat.command_parameters[a] << 24);
				float result = 0.0;
				
				if(raw_value & 0x80000000) 
				{ 
					u32 p = ((raw_value >> 12) - 1);
					p = (~p & 0x7FFFF);
					result = -1.0 * p;
				}

				else { result = (raw_value >> 12); }
				if((raw_value & 0xFFF) != 0) { result += (raw_value & 0xFFF) / 4096.0; }

				u8 x = ((a / 4) % 4);
				u8 y = (a / 16);

				temp_matrix.data[x][y] = result;
				a += 4;
			}

			switch(lcd_3D_stat.matrix_mode)
			{
				case 0x0:
					gx_projection_matrix = temp_matrix * gx_projection_matrix;
					break;

				case 0x1:
					gx_position_matrix = temp_matrix * gx_position_matrix;
					break;

				case 0x2:
					gx_position_matrix = temp_matrix * gx_position_matrix;
					gx_vector_matrix = temp_matrix * gx_vector_matrix;
					break;

				case 0x3:
					gx_texture_matrix = temp_matrix * gx_texture_matrix;
					break;
			}

			break;

		//MTX_MULT_4x3
		case 0x19:
			temp_matrix.make_identity(4);

			for(int a = 0; a < 48;)
			{
				u32 raw_value = lcd_3D_stat.command_parameters[a+3] | (lcd_3D_stat.command_parameters[a+2] << 8) | (lcd_3D_stat.command_parameters[a+1] << 16) | (lcd_3D_stat.command_parameters[a] << 24);
				float result = 0.0;
				
				if(raw_value & 0x80000000) 
				{ 
					u32 p = ((raw_value >> 12) - 1);
					p = (~p & 0x7FFFF);
					result = -1.0 * p;
				}

				else { result = (raw_value >> 12); }
				if((raw_value & 0xFFF) != 0) { result += (raw_value & 0xFFF) / 4096.0; }

				u8 x = ((a / 4) % 3);
				u8 y = (a / 12);

				temp_matrix.data[x][y] = result;
				a += 4;
			}

			switch(lcd_3D_stat.matrix_mode)
			{
				case 0x0:
					gx_projection_matrix = temp_matrix * gx_projection_matrix;
					break;

				case 0x1:
					gx_position_matrix = temp_matrix * gx_position_matrix;
					break;

				case 0x2:
					gx_position_matrix = temp_matrix * gx_position_matrix;
					gx_vector_matrix = temp_matrix * gx_vector_matrix;
					break;

				case 0x3:
					gx_texture_matrix = temp_matrix * gx_texture_matrix;
					break;
			}

			break;

		//MTX_MULT_3x3
		case 0x1A:
			temp_matrix.make_identity(4);

			for(int a = 0; a < 36;)
			{
				u32 raw_value = lcd_3D_stat.command_parameters[a+3] | (lcd_3D_stat.command_parameters[a+2] << 8) | (lcd_3D_stat.command_parameters[a+1] << 16) | (lcd_3D_stat.command_parameters[a] << 24);
				float result = 0.0;
				
				if(raw_value & 0x80000000) 
				{ 
					u32 p = ((raw_value >> 12) - 1);
					p = (~p & 0x7FFFF);
					result = -1.0 * p;
				}

				else { result = (raw_value >> 12); }
				if((raw_value & 0xFFF) != 0) { result += (raw_value & 0xFFF) / 4096.0; }

				u8 x = ((a / 4) % 2);
				u8 y = (a / 12);

				temp_matrix.data[x][y] = result;
				a += 4;
			}

			switch(lcd_3D_stat.matrix_mode)
			{
				case 0x0:
					gx_projection_matrix = temp_matrix * gx_projection_matrix;
					break;

				case 0x1:
					gx_position_matrix = temp_matrix * gx_position_matrix;
					break;

				case 0x2:
					gx_position_matrix = temp_matrix * gx_position_matrix;
					gx_vector_matrix = temp_matrix * gx_vector_matrix;
					break;

				case 0x3:
					gx_texture_matrix = temp_matrix * gx_texture_matrix;
					break;
			}

			break;

		//MTX_SCALE
		//MTX_TRANS
		case 0x1B:
		case 0x1C:
			temp_matrix.make_identity(4);

			for(int a = 0; a < 12;)
			{
				u32 raw_value = lcd_3D_stat.command_parameters[a+3] | (lcd_3D_stat.command_parameters[a+2] << 8) | (lcd_3D_stat.command_parameters[a+1] << 16) | (lcd_3D_stat.command_parameters[a] << 24);
				float result = 0.0;
				
				if(raw_value & 0x80000000) 
				{ 
					u32 p = ((raw_value >> 12) - 1);
					p = (~p & 0x7FFFF);
					result = -1.0 * p;
				}

				else { result = (raw_value >> 12); }
				if((raw_value & 0xFFF) != 0) { result += (raw_value & 0xFFF) / 4096.0; }

				u8 x = (a / 4);

				if(lcd_3D_stat.current_gx_command == 0x1B) { temp_matrix.data[x][x] = result; }
				else { temp_matrix.data[x][3] = result; }

				a += 4;
			}

			switch(lcd_3D_stat.matrix_mode)
			{
				case 0x0:
					gx_projection_matrix = temp_matrix * gx_projection_matrix;
					break;

				case 0x1:
					gx_position_matrix = temp_matrix * gx_position_matrix;
					break;

				case 0x2:
					gx_position_matrix = temp_matrix * gx_position_matrix;
					if(lcd_3D_stat.current_gx_command == 0x1C) { gx_vector_matrix = temp_matrix * gx_vector_matrix; }
					break;

				case 0x3:
					gx_texture_matrix = temp_matrix * gx_texture_matrix;
					break;
			}

			break;

		//VTX_16
		case 0x23:
			//Push new polygon if necessary
			if(lcd_3D_stat.vertex_list_index == 0)
			{
				switch(lcd_3D_stat.vertex_mode)
				{
					//Triangles
					case 0x0:
						temp_matrix.resize(3, 3);
						gx_triangles.push_back(temp_matrix);
						break;

					//Quads
					case 0x1:
						temp_matrix.resize(4, 4);
						gx_quads.push_back(temp_matrix);
						break;
				}
			}

			{
				float temp_result[4];
				u8 list_size = 0;

				for(int a = 0; a < 8;)
				{
					u16 raw_value = lcd_3D_stat.command_parameters[a+1] | (lcd_3D_stat.command_parameters[a] << 8);
					float result = 0.0;
				
					if(raw_value & 0x8000) 
					{ 
						u16 p = ((raw_value >> 12) - 1);
						p = (~p & 0x7);
						result = -1.0 * p;
					}

					else { result = (raw_value >> 12); }
					if((raw_value & 0xFFF) != 0) { result += (raw_value & 0xFFF) / 4096.0; }

					temp_result[a/2] = result;
					a += 2;
				}

				switch(lcd_3D_stat.vertex_mode)
				{
					//Triangles
					case 0x0:
						gx_triangles.back().data[lcd_3D_stat.vertex_list_index][0] = temp_result[1];
						gx_triangles.back().data[lcd_3D_stat.vertex_list_index][1] = temp_result[0];
						gx_triangles.back().data[lcd_3D_stat.vertex_list_index][2] = temp_result[3];
						list_size = 3;
						break;

					//Quads
					case 0x1:
						gx_quads.back().data[lcd_3D_stat.vertex_list_index][0] = temp_result[1];
						gx_quads.back().data[lcd_3D_stat.vertex_list_index][1] = temp_result[0];
						gx_quads.back().data[lcd_3D_stat.vertex_list_index][2] = temp_result[3];
						list_size = 4;
						break;

				}

				lcd_3D_stat.vertex_list_index++;

				if(lcd_3D_stat.vertex_list_index == list_size)
				{
					lcd_3D_stat.vertex_list_index = 0;
					lcd_3D_stat.render_polygon = true;
				}
			}

			break;

		//BEGIN_VTXS:
		case 0x40:
			lcd_3D_stat.vertex_mode = (lcd_3D_stat.command_parameters[3] & 0x3);
			break;

		//END_VTXS:
		case 0x41:
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
