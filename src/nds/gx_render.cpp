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

/****** Copies rendered 3D scene to scanline buffer ******/
void NTR_LCD::render_bg_3D()
{
	u8 bg_priority = lcd_stat.bg_priority_a[0] + 1;

	//Abort rendering if this BG is disabled
	if(!lcd_stat.bg_enable_a[0]) { return; }

	//Abort rendering if BGs with high priority have already completely rendered a scanline
	if(full_scanline_render_a) { return; }

	bool full_render = true;

	//Grab data from the previous buffer
	u16 current_buffer = (lcd_3D_stat.buffer_id + 1);
	current_buffer &= 0x1;

	//Push 3D screen buffer data to scanline buffer
	u16 gx_index = (256 * lcd_stat.current_scanline);

	for(u32 x = 0; x < 256; x++)
	{
		scanline_buffer_a[x] = gx_screen_buffer[current_buffer][gx_index + x];
		render_buffer_a[x] = bg_priority;
	} 
}

/****** Renders geometry to the 3D screen buffers ******/
void NTR_LCD::render_geometry()
{
	//Calculate origin coordinates based on viewport dimensions
	u8 viewport_width = (lcd_3D_stat.view_port_x2 - lcd_3D_stat.view_port_x1);
	u8 viewport_height = (lcd_3D_stat.view_port_y2 - lcd_3D_stat.view_port_y1);

	//Plot points used for screen rendering
	float plot_x[4];
	float plot_y[4];
	s32 buffer_index = 0;
	u8 vert_count = 0;
	gx_matrix temp_matrix;
	gx_matrix vert_matrix;
	gx_matrix clip_matrix = gx_position_matrix * gx_projection_matrix;

	//Determine what kind of polygon to render
	switch(lcd_3D_stat.vertex_mode)
	{
		//Triangles
		case 0x0:
			vert_matrix = gx_triangles.back();
			vert_count = 3;
			break;

		//Quads
		case 0x1:
			vert_matrix = gx_quads.back();
			vert_count = 4;
			break;

		//Triangle Strips
		case 0x2:
			return;

		//Quad Strips
		case 0x3:
			return;
	}

	//Translate all vertices to screen coordinates
	for(u8 x = 0; x < vert_count; x++)
	{
		temp_matrix.resize(4, 1);
		temp_matrix.data[0][0] = vert_matrix.data[x][0];
		temp_matrix.data[1][0] = vert_matrix.data[x][1];
		temp_matrix.data[2][0] = vert_matrix.data[x][2];
		temp_matrix.data[3][0] = 1.0;
		temp_matrix = temp_matrix * clip_matrix;

 		plot_x[x] = ((temp_matrix.data[0][0] + temp_matrix.data[3][0]) * viewport_width) / ((2 * temp_matrix.data[3][0]) + lcd_3D_stat.view_port_x1);
  		plot_y[x] = ((-temp_matrix.data[1][0] + temp_matrix.data[3][0]) * viewport_height) / ((2 * temp_matrix.data[3][0]) + lcd_3D_stat.view_port_y1);
	}

	//Draw lines for all polygons
	for(u8 x = 0; x < vert_count; x++)
	{
		u8 next_index = x + 1;
		if(next_index == vert_count) { next_index = 0; }

		float x_dist = (plot_x[next_index] - plot_x[x]);
		float y_dist = (plot_y[next_index] - plot_y[x]);
		float x_inc = 0.0;
		float y_inc = 0.0;
		float x_coord = plot_x[x];
		float y_coord = plot_y[x];

		u16 xy_start = 0;
		u16 xy_end = 0;

		if((x_dist != 0) && (y_dist != 0))
		{
			float s = (y_dist / x_dist);
			if(s < 0.0) { s *= -1.0; }

			//Steep slope, Y = 1
			if(s > 1.0)
			{
				y_inc = (y_dist > 0) ? 1.0 : -1.0;
				x_inc = (x_dist / y_dist);
					
				if((x_dist < 0) && (x_inc > 0)) { x_inc *= -1.0; }
				else if((x_dist > 0) && (x_inc < 0)) { x_inc *= -1.0; }
					
				if(y_dist < 0) { y_dist *= -1.0;}
				xy_end = y_dist;
			}

			//Gentle slope, X = 1
			else
			{
				x_inc = (x_dist > 0) ? 1.0 : -1.0;
				y_inc = (y_dist / x_dist);

				if((y_dist < 0) && (y_inc > 0)) { y_inc *= -1.0; }
				else if((y_dist > 0) && (y_inc < 0)) { y_inc *= -1.0; }

				if(x_dist < 0) { x_dist *= -1.0;}
				xy_end = x_dist;
			}
		}

		else if(x_dist == 0)
		{
			x_inc = 0.0;
			y_inc = (y_dist > 0) ? 1.0 : -1.0;
			if(y_dist < 0) { y_dist *= -1.0;}
			xy_end = y_dist;
		}

		else if(y_dist == 0)
		{
			x_inc = (x_dist > 0) ? 1.0 : -1.0;
			y_inc = 0.0;
			if(x_dist < 0) { x_dist *= -1.0;}
			xy_end = x_dist;
		}

		while(xy_start != xy_end)
		{
			//Only draw on-screen objects
			if((x_coord >= 0) && (x_coord < 256) && (y_coord >= 0) && (y_coord <= 192))
			{
				//Convert plot points to buffer index
				buffer_index = ((s32)y_coord * 256) + (s32)x_coord;
				gx_screen_buffer[lcd_3D_stat.buffer_id][buffer_index] = vert_colors[x];
			}

			x_coord += x_inc;
			y_coord += y_inc;
			xy_start++;
		}
	}

	lcd_3D_stat.render_polygon = false;
}

/****** Parses and processes commands sent to the NDS 3D engine ******/
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

		//VERT_COLOR
		case 0x20:
			{
				u32 color_bytes = lcd_3D_stat.command_parameters[3] | (lcd_3D_stat.command_parameters[2] << 8) | (lcd_3D_stat.command_parameters[1] << 16) | (lcd_3D_stat.command_parameters[0] << 24);

				u8 red = (color_bytes & 0x1F);
				red = (!red) ? 0 : ((red * 2) + 1);
				red <<= 2;
				color_bytes >>= 5;

				u8 green = (color_bytes & 0x1F);
				green = (!green) ? 0 : ((green * 2) + 1);
				green <<= 2;
				color_bytes >>= 5;

				u8 blue = (color_bytes & 0x1F);
				blue = (!blue) ? 0 : ((blue * 2) + 1);
				blue <<= 2;

				lcd_3D_stat.vertex_color = 0xFF000000 | (red << 16) | (green << 8) | (blue);
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

					//Triangle Strips
					//Quad Strips
					case 0x2:
					case 0x3:
						lcd_3D_stat.parameter_index = 0;
						lcd_3D_stat.current_gx_command = 0;
						lcd_3D_stat.process_command = false;
						return;
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

				//Set vertex color
				vert_colors[lcd_3D_stat.vertex_list_index] = lcd_3D_stat.vertex_color;

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
