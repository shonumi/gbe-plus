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

#include <cmath>

/****** Copies rendered 3D scene to scanline buffer ******/
void NTR_LCD::render_bg_3D()
{
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
		render_buffer_a[x] = gx_render_buffer[gx_index + x];
	} 
}

/****** Renders geometry to the 3D screen buffers ******/
void NTR_LCD::render_geometry()
{
	u8 bg_priority = lcd_stat.bg_priority_a[0] + 1;

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
			lcd_3D_stat.render_polygon = false;
			return;

		//Quad Strips
		case 0x3:
			lcd_3D_stat.render_polygon = false;
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

		//Check for wonky coordinates
		if(std::isnan(plot_x[x])) { lcd_3D_stat.render_polygon = false; return; }
		if(std::isinf(plot_x[x])) { lcd_3D_stat.render_polygon = false; return; }

		if(std::isnan(plot_y[x])) { lcd_3D_stat.render_polygon = false; return; }
		if(std::isinf(plot_y[x])) { lcd_3D_stat.render_polygon = false; return; }

		//Check if coordinates need to be clipped to the view volume
		if((plot_x[x] < 0) || (plot_x[x] > 255) || (plot_y[x] < 0) || (plot_y[x] > 192))
		{
			lcd_3D_stat.clip_flags |= (1 << x);
		}
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

		s32 xy_start = 0;
		s32 xy_end = 0;

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
				gx_render_buffer[buffer_index] = bg_priority;
			}

			x_coord += x_inc;
			y_coord += y_inc;
			xy_start++;
		}
	}

	//Fill in polygon
	switch(lcd_3D_stat.vertex_mode)
	{
		//Triangles
		case 0x0:
			//Solid color fill
			if((vert_colors[0] == vert_colors[1]) && (vert_colors[0] == vert_colors[2])) { fill_tri_solid(plot_x, plot_y); }
			break;

		//Quads
		case 0x1:
			//Solid color fill
			if((vert_colors[0] == vert_colors[1]) && (vert_colors[0] == vert_colors[2]) && (vert_colors[0] == vert_colors[3])) { fill_quad_solid(plot_x, plot_y); }
			break;

		//Triangle Strips
		case 0x2:
			lcd_3D_stat.render_polygon = false;
			return;

		//Quad Strips
		case 0x3:
			lcd_3D_stat.render_polygon = false;
			return;
	}

	lcd_3D_stat.render_polygon = false;
	lcd_3D_stat.clip_flags = 0;
}

/****** NDS 3D Software Renderer - Fills a triangle with a solid color ******/
void NTR_LCD::fill_tri_solid(float* px, float* py)
{
	//Vertex IDs
	u8 v0 = 0;
	u8 v1 = 0;
	u8 v2 = 0;

	//Sort IDS
	//V0 = furthest left
	if(px[1] < px[v0]) { v0 = 1; }
	if((px[1] == px[v0]) && (py[1] < py[v0])) { v0 = 1; }

	if(px[2] < px[v0]) { v0 = 2; }
	if((px[2] == px[v0]) && (py[2] < py[v0])) { v0 = 2; }

	//V2 = furthest right
	if(px[1] > px[v2]) { v2 = 1; }
	if((px[1] == px[v2]) && (py[1] < py[v2])) { v2 = 1; }

	if(px[2] > px[v2]) { v2 = 2; }
	if((px[2] == px[v2]) && (py[2] < py[v2])) { v2 = 2; }

	//V3 = last point
	if((v0 + v2) == 3) { v1 = 0; }
	else if((v0 + v2) == 2) { v1 = 1; }
	else { v1 = 2; }

	//Calculate Y deltas between vertices
	float y_delta_v1 = (py[v1] - py[v0]) / (px[v1] - px[v0]);
	float y_delta_v2 = (py[v2] - py[v0]) / (px[v2] - px[v0]);

	//Calculate boundaries
	float v_bound_1 = py[v0];
	float v_bound_2 = py[v0];
	u16 side_bound = (px[v1] > 255) ? 255 : px[v1];

	s32 x_coord = px[v0];
	s32 y_coord = py[v0];

	u32 buffer_index = 0;
	s32 low = 0;

	for(int x = 0; x < 2; x++)
	{
		//Draw 2nd half of triangle
		if(x == 1)
		{
			y_delta_v1 = (py[v2] - py[v1]) / (px[v2] - px[v1]);
			y_delta_v2 = (py[v2] - py[v0]) / (px[v2] - px[v0]);
	
			side_bound = px[v2];
		}

		while(x_coord != side_bound)
		{
			//Determine which boundary is top and bottom
			if(v_bound_1 < v_bound_2)
			{
				y_coord = v_bound_1;
				low = v_bound_2;
			}

			else
			{
				y_coord = v_bound_2;
				low = v_bound_1;
			}

			while(y_coord != low)
			{
				//Only draw on-screen objects
				if((x_coord >= 0) && (x_coord < 256) && (y_coord >= 0) && (y_coord <= 192))
				{
					//Convert plot points to buffer index
					buffer_index = ((s32)y_coord * 256) + (s32)x_coord;
					gx_screen_buffer[lcd_3D_stat.buffer_id][buffer_index] = vert_colors[x];
				}

				y_coord++;
			}

			v_bound_1 += y_delta_v1;
			v_bound_2 += y_delta_v2; 
			x_coord++;
		}
	}
}

/****** NDS 3D Software Renderer - Fills a quads with a solid color ******/
void NTR_LCD::fill_quad_solid(float* px, float* py)
{
	//Vertex IDs
	u8 v0 = 0;
	u8 v1 = 1;
	u8 v2 = 2;
	u8 v3 = 3;
	u8 temp = 0;

	s32 max = 0;
	s32 min = 0;

	//Bubble-sort based on X coordinate
	for(u8 x = 0; x < 3; x++)
	{
		if(px[v1] < px[v0]) { temp = v0; v0 = v1; v1 = temp; }
		if(px[v2] < px[v1]) { temp = v1; v1 = v2; v2 = temp; }
		if(px[v3] < px[v2]) { temp = v2; v2 = v3; v3 = temp; }
	}

	//Grab minimum and maximum X coordinates
	min = px[v0];
	max = px[v3];

	//Correctly sort V0-V1 and V2-V3 based on Y coordinate
	if((px[v0] == px[v1]) && (py[v1] < py[v0])) { temp = v0; v0 = v1; v1 = temp; }
	if((px[v1] == px[v2]) && (py[v2] < py[v1])) { temp = v1; v1 = v2; v2 = temp; }
	if((px[v2] == px[v3]) && (py[v3] < py[v2])) { temp = v2; v2 = v3; v3 = temp; }
	
	if(py[v1] < py[v0]) { temp = v0; v0 = v1; v1 = temp; }
	if(py[v3] < py[v2]) { temp = v2; v2 = v3; v3 = temp; }

	float y_delta_a[3];
	float y_delta_b[3];
	float v_bound_a = 0.0;
	float v_bound_b = 0.0;

	s32 p0 = 0;
	s32 p1 = 0;
	s32 plot_x = min;
	s32 plot_y = 0;
	s32 vert = 0;
	u32 buffer_index = 0;

	//Calculate various slopes
	if(px[v0] == px[v1])
	{
		y_delta_a[0] = (px[v2] - px[v0]) ? ((py[v2] - py[v0]) / (px[v2] - px[v0])) : 0;
		y_delta_b[0] = (px[v3] - px[v1]) ? ((py[v3] - py[v1]) / (px[v3] - px[v1])) : 0;
		p0 = px[v0] + 1;
		v_bound_a = py[v0];
		v_bound_b = py[v1];
		
		y_delta_a[1] = y_delta_a[0];
		y_delta_b[1] = y_delta_b[0];
	}

	else if(px[v0] > px[v1])
	{
		y_delta_a[0] = (px[v0] - px[v1]) ? ((py[v0] - py[v1]) / (px[v0] - px[v1])) : 0;
		y_delta_b[0] = (px[v3] - px[v1]) ? ((py[v3] - py[v1]) / (px[v3] - px[v1])) : 0;
		p0 = px[v0] + 1;
		v_bound_a = py[v1];
		v_bound_b = py[v1];
		
		y_delta_a[1] = (px[v0] - px[v2]) ? ((py[v0] - py[v2]) / (px[v0] - px[v2])) : 0;
		y_delta_b[1] = y_delta_b[0];
	}

	else
	{
		y_delta_a[0] = (px[v2] - px[v0]) ? ((py[v2] - py[v0]) / (px[v2] - px[v0])) : 0;
		y_delta_b[0] = (px[v0] - px[v1]) ? ((py[v0] - py[v1]) / (px[v0] - px[v1])) : 0;
		p0 = px[v1] + 1;
		v_bound_a = py[v0];
		v_bound_b = py[v0];
		
		y_delta_a[1] = y_delta_a[0];
		y_delta_b[1] = (px[v3] - px[v1]) ? ((py[v3] - py[v1]) / (px[v3] - px[v1])) : 0;
	}

	if(px[v2] == px[v3])
	{
		p1 = px[v2] + 1;
		y_delta_a[2] = y_delta_a[1];
		y_delta_b[2] = y_delta_b[1];
	}

	else if(px[v2] > px[v3])
	{
		p1 = px[v3] + 1;
		y_delta_a[2] = y_delta_a[1];
		y_delta_b[2] = (px[v2] - px[v3]) ? ((py[v2] - py[v3]) / (px[v2] - px[v3])) : 0;
	}

	else
	{
		p1 = px[v2] + 1;
		y_delta_a[2] = (px[v3] - px[v2]) ? ((py[v3] - py[v2]) / (px[v3] - px[v2])) : 0;
		y_delta_b[2] = y_delta_b[1];
	}

	//Draw to screen
	while(plot_x <= max)
	{
		//Determine upper and lower draw boundaries
		plot_y = (v_bound_a < v_bound_b) ? v_bound_a : v_bound_b;		
		vert = (v_bound_a > v_bound_b) ? v_bound_a : v_bound_b;

		while(plot_y <= vert)
		{
			//Only draw on-screen objects
			if((plot_x >= 0) && (plot_x < 256) && (plot_y >= 0) && (plot_y < 192))
			{
				//Convert plot points to buffer index
				buffer_index = (plot_y * 256) + plot_x;
				gx_screen_buffer[lcd_3D_stat.buffer_id][buffer_index] = vert_colors[0];
			}
	
			plot_y++;
		}

		plot_x++;

		//Change slopes
		if(plot_x == p0)
		{
			y_delta_a[0] = y_delta_a[1];
			y_delta_b[0] = y_delta_b[1];
		}

		if(plot_x == p1)
		{
			y_delta_a[0] = y_delta_a[2];
			y_delta_b[0] = y_delta_b[2];
		}
		
		v_bound_a += y_delta_a[0];
		v_bound_b += y_delta_b[0];
	}
}

/****** Determines if a polygon can be pushed to internal rendering list ******/
bool NTR_LCD::poly_push(gx_matrix &current_matrix)
{
	bool status = false;

	//Limit 3D engine to 2048 polygons and 6144 vertices
	if((lcd_3D_stat.poly_count >= 2048) || (lcd_3D_stat.vert_count >= 6144)) { status = false; }

	else
	{
		switch(lcd_3D_stat.vertex_mode)
		{
			//Triangles
			case 0x0:
				if(((lcd_3D_stat.poly_count + 1) < 2048) && ((lcd_3D_stat.vert_count + 3) < 6144))
				{
					current_matrix.resize(3, 3);
					gx_triangles.push_back(current_matrix);
					lcd_3D_stat.poly_count++;
					lcd_3D_stat.vert_count += 3;
					status = true;
				}

				else { status = false; }

				break;

			//Quads
			case 0x1:
				if(((lcd_3D_stat.poly_count + 1) < 2048) && ((lcd_3D_stat.vert_count + 4) < 6144))
				{
					current_matrix.resize(4, 4);
					gx_quads.push_back(current_matrix);
					lcd_3D_stat.poly_count++;
					lcd_3D_stat.vert_count += 4;
					status = true;
				}

				else { status = false; }

				break;


			//Triangle Strips
			//Quad Strips
			case 0x2:
			case 0x3:
				status = false;
				break;
		}		
	}

	if(!status)
	{
		lcd_3D_stat.parameter_index = 0;
		lcd_3D_stat.current_gx_command = 0;
		lcd_3D_stat.process_command = false;
	}

	return status;
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

		//MTX_STORE
		case 0x13:
			switch(lcd_3D_stat.matrix_mode)
			{
				case 0x0:
					gx_projection_stack[0] = gx_projection_matrix;
					break;

				case 0x1:
				case 0x2:
					{
						u8 offset = (lcd_3D_stat.command_parameters[3] & 0x1F);
						
						if(offset <= 30)
						{
							gx_position_stack[offset] = gx_position_matrix;
							gx_vector_stack[offset] = gx_vector_matrix;
						}
					}

					break;

				case 0x3:
					gx_texture_stack[0] = gx_texture_matrix;
					break;
			}

			break;

		//MTX_RESTORE
		case 0x14:
			switch(lcd_3D_stat.matrix_mode)
			{
				case 0x0:
					gx_projection_matrix = gx_projection_stack[0];
					break;

				case 0x1:
				case 0x2:
					{
						u8 offset = (lcd_3D_stat.command_parameters[3] & 0x1F);
						
						if(offset <= 30)
						{
							gx_position_matrix = gx_position_stack[offset];
							gx_vector_matrix = gx_vector_stack[offset];
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

		//MTX_LOAD_4x4
		//MTX_MULT_4x4
		case 0x16:
		case 0x18:
			for(int a = 0; a < 64;)
			{
				u32 raw_value = read_param_u32(a);
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
					if(lcd_3D_stat.current_gx_command == 0x16) { gx_projection_matrix = temp_matrix; }
					else { gx_projection_matrix = temp_matrix * gx_projection_matrix; }
					break;

				case 0x1:
					if(lcd_3D_stat.current_gx_command == 0x16) { gx_position_matrix = temp_matrix; }
					else { gx_position_matrix = temp_matrix * gx_position_matrix; }
					break;

				case 0x2:
					if(lcd_3D_stat.current_gx_command == 0x16)
					{
						gx_position_matrix = temp_matrix;
						gx_vector_matrix = temp_matrix;
					}

					else
					{
						gx_position_matrix = temp_matrix * gx_position_matrix;
						gx_vector_matrix = temp_matrix * gx_vector_matrix;
					}

					break;

				case 0x3:
					if(lcd_3D_stat.current_gx_command == 0x16) { gx_texture_matrix = temp_matrix; }
					else { gx_texture_matrix = temp_matrix * gx_texture_matrix; }
					break;
			}

			break;

		//MTX_LOAD_4x3
		//MTX_MULT_4x3
		case 0x17:
		case 0x19:
			temp_matrix.make_identity(4);

			for(int a = 0; a < 48;)
			{
				u32 raw_value = read_param_u32(a);
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
					if(lcd_3D_stat.current_gx_command == 0x16) { gx_projection_matrix = temp_matrix; }
					else { gx_projection_matrix = temp_matrix * gx_projection_matrix; }
					break;

				case 0x1:
					if(lcd_3D_stat.current_gx_command == 0x16) { gx_position_matrix = temp_matrix; }
					else { gx_position_matrix = temp_matrix * gx_position_matrix; }
					break;

				case 0x2:
					if(lcd_3D_stat.current_gx_command == 0x16)
					{
						gx_position_matrix = temp_matrix;
						gx_vector_matrix = temp_matrix;
					}

					else
					{
						gx_position_matrix = temp_matrix * gx_position_matrix;
						gx_vector_matrix = temp_matrix * gx_vector_matrix;
					}

					break;

				case 0x3:
					if(lcd_3D_stat.current_gx_command == 0x16) { gx_texture_matrix = temp_matrix; }
					else { gx_texture_matrix = temp_matrix * gx_texture_matrix; }
					break;
			}

			break;

		//MTX_MULT_3x3
		case 0x1A:
			temp_matrix.make_identity(4);

			for(int a = 0; a < 36;)
			{
				u32 raw_value = read_param_u32(a);
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
				u32 raw_value = read_param_u32(a);
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
				u32 color_bytes = read_param_u32(0);
				lcd_3D_stat.vertex_color = get_rgb15(color_bytes);
			}

			break;
			
		//VTX_16
		case 0x23:
			//Push new polygon if necessary
			if(lcd_3D_stat.vertex_list_index == 0)
			{
				if(!poly_push(temp_matrix)) { return; }
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

						lcd_3D_stat.last_x = gx_triangles.back().data[lcd_3D_stat.vertex_list_index][0];
						lcd_3D_stat.last_y = gx_triangles.back().data[lcd_3D_stat.vertex_list_index][1];
						lcd_3D_stat.last_z = gx_triangles.back().data[lcd_3D_stat.vertex_list_index][2];


						break;

					//Quads
					case 0x1:
						gx_quads.back().data[lcd_3D_stat.vertex_list_index][0] = temp_result[1];
						gx_quads.back().data[lcd_3D_stat.vertex_list_index][1] = temp_result[0];
						gx_quads.back().data[lcd_3D_stat.vertex_list_index][2] = temp_result[3];
						list_size = 4;

						lcd_3D_stat.last_x = gx_quads.back().data[lcd_3D_stat.vertex_list_index][0];
						lcd_3D_stat.last_y = gx_quads.back().data[lcd_3D_stat.vertex_list_index][1];
						lcd_3D_stat.last_z = gx_quads.back().data[lcd_3D_stat.vertex_list_index][2];

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

		//VTX_10
		case 0x24:
			//Push new polygon if necessary
			if(lcd_3D_stat.vertex_list_index == 0)
			{
				if(!poly_push(temp_matrix)) { return; }
			}

			{
				float temp_result[3];
				u8 list_size = 0;
				u32 raw_value = read_param_u32(0);

				for(int a = 0; a < 3; a++)
				{
					u16 value = (raw_value >> (10 * a)) & 0x3FF;
					
					float result = 0.0;
				
					if(value & 0x200) 
					{ 
						u16 p = ((value >> 6) - 1);
						p = (~p & 0x7);
						result = -1.0 * p;
					}

					else { result = (value >> 6); }
					if((value & 0x3F) != 0) { result += (value & 0x3F) / 64.0; }

					temp_result[a] = result;
				}

				switch(lcd_3D_stat.vertex_mode)
				{
					//Triangles
					case 0x0:
						gx_triangles.back().data[lcd_3D_stat.vertex_list_index][0] = temp_result[0];
						gx_triangles.back().data[lcd_3D_stat.vertex_list_index][1] = temp_result[1];
						gx_triangles.back().data[lcd_3D_stat.vertex_list_index][2] = temp_result[2];
						list_size = 3;

						lcd_3D_stat.last_x = gx_triangles.back().data[lcd_3D_stat.vertex_list_index][0];
						lcd_3D_stat.last_y = gx_triangles.back().data[lcd_3D_stat.vertex_list_index][1];
						lcd_3D_stat.last_z = gx_triangles.back().data[lcd_3D_stat.vertex_list_index][2];

						break;

					//Quads
					case 0x1:
						gx_quads.back().data[lcd_3D_stat.vertex_list_index][0] = temp_result[0];
						gx_quads.back().data[lcd_3D_stat.vertex_list_index][1] = temp_result[1];
						gx_quads.back().data[lcd_3D_stat.vertex_list_index][2] = temp_result[2];
						list_size = 4;

						lcd_3D_stat.last_x = gx_quads.back().data[lcd_3D_stat.vertex_list_index][0];
						lcd_3D_stat.last_y = gx_quads.back().data[lcd_3D_stat.vertex_list_index][1];
						lcd_3D_stat.last_z = gx_quads.back().data[lcd_3D_stat.vertex_list_index][2];

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

		//VTX_XY
		//VTX_XZ
		//VTX_YZ
		case 0x25:
		case 0x26:
		case 0x27:
			//Push new polygon if necessary
			if(lcd_3D_stat.vertex_list_index == 0)
			{
				if(!poly_push(temp_matrix)) { return; }
			}

			{
				float temp_result[2];
				u8 list_size = 0;
				u32 raw_value = read_param_u32(0);

				for(int a = 0; a < 2; a++)
				{
					u16 value = (raw_value >> (16 * a)) & 0xFFFF;
					
					float result = 0.0;
				
					if(value & 0x8000) 
					{ 
						u16 p = ((value >> 12) - 1);
						p = (~p & 0x7);
						result = -1.0 * p;
					}

					else { result = (value >> 12); }
					if((value & 0xFFF) != 0) { result += (value & 0xFFF) / 4096.0; }

					temp_result[a] = result;
				}

				switch(lcd_3D_stat.vertex_mode)
				{
					//Triangles
					case 0x0:
						list_size = 3;

						//XY
						if(lcd_3D_stat.current_gx_command == 0x25)
						{
							gx_triangles.back().data[lcd_3D_stat.vertex_list_index][0] = temp_result[0];
							gx_triangles.back().data[lcd_3D_stat.vertex_list_index][1] = temp_result[1];
							gx_triangles.back().data[lcd_3D_stat.vertex_list_index][2] = lcd_3D_stat.last_z;

							lcd_3D_stat.last_x = gx_triangles.back().data[lcd_3D_stat.vertex_list_index][0];
							lcd_3D_stat.last_y = gx_triangles.back().data[lcd_3D_stat.vertex_list_index][1];
						}

						//XZ
						if(lcd_3D_stat.current_gx_command == 0x26)
						{
							gx_triangles.back().data[lcd_3D_stat.vertex_list_index][0] = temp_result[0];
							gx_triangles.back().data[lcd_3D_stat.vertex_list_index][1] = lcd_3D_stat.last_y;
							gx_triangles.back().data[lcd_3D_stat.vertex_list_index][2] = temp_result[1];

							lcd_3D_stat.last_x = gx_triangles.back().data[lcd_3D_stat.vertex_list_index][0];
							lcd_3D_stat.last_z = gx_triangles.back().data[lcd_3D_stat.vertex_list_index][1];
						}

						//YZ
						if(lcd_3D_stat.current_gx_command == 0x27)
						{
							gx_triangles.back().data[lcd_3D_stat.vertex_list_index][0] = lcd_3D_stat.last_x;
							gx_triangles.back().data[lcd_3D_stat.vertex_list_index][1] = temp_result[0];
							gx_triangles.back().data[lcd_3D_stat.vertex_list_index][2] = temp_result[1];

							lcd_3D_stat.last_y = gx_triangles.back().data[lcd_3D_stat.vertex_list_index][0];
							lcd_3D_stat.last_z = gx_triangles.back().data[lcd_3D_stat.vertex_list_index][1];
						}
						
						break;

					//Quads
					case 0x1:
						list_size = 4;

						//XY
						if(lcd_3D_stat.current_gx_command == 0x25)
						{
							gx_quads.back().data[lcd_3D_stat.vertex_list_index][0] = temp_result[0];
							gx_quads.back().data[lcd_3D_stat.vertex_list_index][1] = temp_result[1];
							gx_quads.back().data[lcd_3D_stat.vertex_list_index][2] = lcd_3D_stat.last_z;

							lcd_3D_stat.last_x = gx_quads.back().data[lcd_3D_stat.vertex_list_index][0];
							lcd_3D_stat.last_y = gx_quads.back().data[lcd_3D_stat.vertex_list_index][1];
						}

						//XZ
						if(lcd_3D_stat.current_gx_command == 0x26)
						{
							gx_quads.back().data[lcd_3D_stat.vertex_list_index][0] = temp_result[0];
							gx_quads.back().data[lcd_3D_stat.vertex_list_index][1] = lcd_3D_stat.last_y;
							gx_quads.back().data[lcd_3D_stat.vertex_list_index][2] = temp_result[1];

							lcd_3D_stat.last_x = gx_quads.back().data[lcd_3D_stat.vertex_list_index][0];
							lcd_3D_stat.last_z = gx_quads.back().data[lcd_3D_stat.vertex_list_index][1];
						}

						//YZ
						if(lcd_3D_stat.current_gx_command == 0x27)
						{
							gx_quads.back().data[lcd_3D_stat.vertex_list_index][0] = lcd_3D_stat.last_x;
							gx_quads.back().data[lcd_3D_stat.vertex_list_index][1] = temp_result[0];
							gx_quads.back().data[lcd_3D_stat.vertex_list_index][2] = temp_result[1];

							lcd_3D_stat.last_y = gx_quads.back().data[lcd_3D_stat.vertex_list_index][0];
							lcd_3D_stat.last_z = gx_quads.back().data[lcd_3D_stat.vertex_list_index][1];
						}

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

/****** Reads GX command parameters as a 32-bit value *****/
u32 NTR_LCD::read_param_u32(u8 i)
{
	u32 val = lcd_3D_stat.command_parameters[i+3] | (lcd_3D_stat.command_parameters[i+2] << 8) | (lcd_3D_stat.command_parameters[i+1] << 16) | (lcd_3D_stat.command_parameters[i] << 24);
	return val;
}

/****** Reads GX command parameters as a 32-bit value *****/
u16 NTR_LCD::read_param_u16(u8 i)
{
	u16 val = lcd_3D_stat.command_parameters[i+1] | (lcd_3D_stat.command_parameters[i] << 8);
	return val;
}	

/****** Returns 32-bit color from RGB15 format ******/
u32 NTR_LCD::get_rgb15(u16 color_bytes)
{
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

	return 0xFF000000 | (red << 16) | (green << 8) | (blue);
}