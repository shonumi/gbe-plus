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

	u8 bg_priority = lcd_stat.bg_priority_a[0] + 1;

	//Grab data from the previous buffer
	u16 current_buffer = (lcd_3D_stat.buffer_id);


	//Push 3D screen buffer data to scanline buffer
	u16 gx_index = (256 * lcd_stat.current_scanline);

	for(u32 x = 0; x < 256; x++)
	{
		if(!render_buffer_a[x] || (bg_priority < render_buffer_a[x]))
		{
			if(gx_render_buffer[current_buffer][gx_index + x])
			{
				render_buffer_a[x] = bg_priority;
				scanline_buffer_a[x] = gx_screen_buffer[current_buffer][gx_index + x];
			}
		}
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
	float plot_z[4];
	s32 buffer_index = 0;
	u8 vert_count = 0;
	gx_matrix temp_matrix;
	gx_matrix vert_matrix;
	gx_matrix clip_matrix;

	//Determine what kind of polygon to render
	switch(lcd_3D_stat.vertex_mode)
	{
		//Triangles
		case 0x0:
			if(gx_triangles.empty())
			{
				lcd_3D_stat.render_polygon = false;
				return;
			}

			vert_matrix = gx_triangles.back();
			vert_count = 3;
			break;

		//Quads
		case 0x1:
			if(gx_quads.empty())
			{
				lcd_3D_stat.render_polygon = false;
				return;
			}

			vert_matrix = gx_quads.back();
			vert_count = 4;
			break;

		//Triangle Strips
		case 0x2:
			if(gx_tri_strips.empty())
			{
				lcd_3D_stat.render_polygon = false;
				return;
			}

			vert_matrix = gx_tri_strips.back();
			vert_count = 3;
			break;

		//Quad Strips
		case 0x3:
			if(gx_quad_strips.empty())
			{
				lcd_3D_stat.render_polygon = false;
				return;
			}

			vert_matrix = gx_quad_strips.back();
			vert_count = 4;
			break;
	}

	//Translate all vertices to screen coordinates
	for(u8 x = 0; x < vert_count; x++)
	{
		clip_matrix = last_pos_matrix[x] * gx_projection_matrix;

		temp_matrix.resize(4, 1);
		temp_matrix.data[0][0] = vert_matrix.data[x][0];
		temp_matrix.data[1][0] = vert_matrix.data[x][1];
		temp_matrix.data[2][0] = vert_matrix.data[x][2];
		temp_matrix.data[3][0] = 1.0;

		//Generate NDS XY screen coordinate from clip matrix
		temp_matrix = temp_matrix * clip_matrix;
 		plot_x[x] = round(((temp_matrix.data[0][0] + temp_matrix.data[3][0]) * viewport_width) / ((2 * temp_matrix.data[3][0]) + lcd_3D_stat.view_port_x1));
  		plot_y[x] = round(((-temp_matrix.data[1][0] + temp_matrix.data[3][0]) * viewport_height) / ((2 * temp_matrix.data[3][0]) + lcd_3D_stat.view_port_y1));

		//Get Z coordinate, use existing data from vertex
		plot_z[x] = temp_matrix.data[2][0];
		if(temp_matrix.data[3][0]) { plot_z[x] /= temp_matrix.data[3][0]; }

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

	//Reset hi and lo fill coordinates
	for(int x = 0; x < 256; x++)
	{
		lcd_3D_stat.hi_fill[x] = 0xFF;
		lcd_3D_stat.lo_fill[x] = 0;

		lcd_3D_stat.hi_color[x] = 0;
		lcd_3D_stat.lo_color[x] = 0;

		lcd_3D_stat.hi_tx[x] = 0;
		lcd_3D_stat.lo_tx[x] = 0;

		lcd_3D_stat.hi_ty[x] = 0;
		lcd_3D_stat.lo_ty[x] = 0;
	}

	//Find minimum and maximum X values for polygon
	float x_min = plot_x[0];
	float x_max = plot_x[0];

	for(u8 x = 1; x < vert_count; x++)
	{
		if(plot_x[x] < x_min) { x_min = plot_x[x]; }
		if(plot_x[x] > x_max) { x_max = plot_x[x]; }
	}

	lcd_3D_stat.poly_min_x = round(x_min);
	lcd_3D_stat.poly_max_x = round(x_max);

	if(lcd_3D_stat.poly_min_x < 0) { lcd_3D_stat.poly_min_x = 0; }
	if(lcd_3D_stat.poly_max_x < 0) { lcd_3D_stat.poly_max_x = 0; }

	if(lcd_3D_stat.poly_min_x > 255) { lcd_3D_stat.poly_min_x = 255; }
	if(lcd_3D_stat.poly_max_x > 255) { lcd_3D_stat.poly_max_x = 255; }

	//Draw lines for all polygons
	for(u8 x = 0; x < vert_count; x++)
	{
		u8 next_index = x + 1;
		if(next_index == vert_count) { next_index = 0; }

		float x_dist = (plot_x[next_index] - plot_x[x]);
		float y_dist = (plot_y[next_index] - plot_y[x]);
		float z_dist = (plot_z[next_index] - plot_z[x]);

		float x_inc = 0.0;
		float y_inc = 0.0;
		float z_inc = 0.0;

		float x_coord = plot_x[x];
		float y_coord = plot_y[x];
		float z_coord = plot_z[x];

		s32 xy_start = 0;
		s32 xy_end = 0;
		s32 xy_len = 0;

		u32 c1 = vert_colors[x];
		u32 c2 = vert_colors[next_index];
		u32 c3 = 0;
		float c_ratio = 0.0;
		float c_inc = 0.0;

		float tx_inc = 0.0;
		float ty_inc = 0.0;

		float tx = lcd_3D_stat.tex_coord_x[x];
		float ty = lcd_3D_stat.tex_coord_y[x];

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

				xy_start = plot_y[x];
				xy_end = plot_y[next_index];
			}

			//Gentle slope, X = 1
			else
			{
				x_inc = (x_dist > 0) ? 1.0 : -1.0;
				y_inc = (y_dist / x_dist);

				if((y_dist < 0) && (y_inc > 0)) { y_inc *= -1.0; }
				else if((y_dist > 0) && (y_inc < 0)) { y_inc *= -1.0; }

				xy_start = plot_x[x];
				xy_end = plot_x[next_index];
			}
		}

		else if(x_dist == 0)
		{
			x_inc = 0.0;
			y_inc = (y_dist > 0) ? 1.0 : -1.0;

			xy_start = plot_y[x];
			xy_end = plot_y[next_index];
		}

		else if(y_dist == 0)
		{
			x_inc = (x_dist > 0) ? 1.0 : -1.0;
			y_inc = 0.0;

			xy_start = plot_x[x];
			xy_end = plot_x[next_index];
		}

		xy_len = abs(xy_end - xy_start);

		if(xy_len != 0)
		{
			z_inc = (plot_z[next_index] - plot_z[x]) / xy_len;
			c_inc = 1.0 / xy_len;

			tx_inc = (lcd_3D_stat.tex_coord_x[next_index] - lcd_3D_stat.tex_coord_x[x]) / xy_len;
			ty_inc = (lcd_3D_stat.tex_coord_y[next_index] - lcd_3D_stat.tex_coord_y[x]) / xy_len;
		}

		while(xy_len)
		{
			c3 = interpolate_rgb(c1, c2, c_ratio);

			//Only draw on-screen objects
			if((x_coord >= 0) && (x_coord <= 255) && (y_coord >= 0) && (y_coord <= 191))
			{
				//Convert plot points to buffer index
				buffer_index = (round(y_coord) * 256) + round(x_coord);
			}

			//Set fill coordinates
			if((x_coord >= 0) && (x_coord <= 255))
			{
				s32 temp_y = round(y_coord);
				s32 temp_x = round(x_coord);

				//Keep fill coordinates on-screen if they extend vertically				
				if(temp_y > 0xC0) { temp_y = 0xC0; }
				else if(temp_y < 0) { temp_y = 0; }

				if(lcd_3D_stat.hi_fill[temp_x] > temp_y)
				{
					lcd_3D_stat.hi_fill[temp_x] = temp_y;
					lcd_3D_stat.hi_line_z[temp_x] = z_coord;
					lcd_3D_stat.hi_color[temp_x] = c3;

					lcd_3D_stat.hi_tx[temp_x] = tx;
					lcd_3D_stat.hi_ty[temp_x] = ty;
				}

				if(lcd_3D_stat.lo_fill[temp_x] < temp_y)
				{
					lcd_3D_stat.lo_fill[temp_x] = temp_y;
					lcd_3D_stat.lo_line_z[temp_x] = z_coord;
					lcd_3D_stat.lo_color[temp_x] = c3;

					lcd_3D_stat.lo_tx[temp_x] = tx;
					lcd_3D_stat.lo_ty[temp_x] = ty;
				}
			} 

			x_coord += x_inc;
			y_coord += y_inc;
			z_coord += z_inc;
			c_ratio += c_inc;

			tx += tx_inc;
			ty += ty_inc;

			xy_len--;
		}
	}

	//Fill in polygon
	switch(lcd_3D_stat.vertex_mode)
	{
		//Triangles
		//Triangle Strips
		case 0x0:
		case 0x2:
			//Textured color fill
			if(lcd_3D_stat.use_texture) { fill_poly_textured(); }

			//Solid color fill
			else if((vert_colors[0] == vert_colors[1]) && (vert_colors[0] == vert_colors[2])) { fill_poly_solid(); }
			
			//Interpolated color fill
			else { fill_poly_interpolated(); }

			break;

		//Quads
		//Quad Strips
		case 0x1:
		case 0x3:
			//Textured color fill
			if(lcd_3D_stat.use_texture) { fill_poly_textured(); }

			//Solid color fill
			else if((vert_colors[0] == vert_colors[1]) && (vert_colors[0] == vert_colors[2]) && (vert_colors[0] == vert_colors[3])) { fill_poly_solid(); }

			//Interpolated color fill
			else { fill_poly_interpolated(); }

			break;
	}

	lcd_3D_stat.render_polygon = false;
	lcd_3D_stat.clip_flags = 0;
}

/****** NDS 3D Software Renderer - Fills a given poly with a solid color ******/
void NTR_LCD::fill_poly_solid()
{
	u8 y_coord = 0;
	u32 buffer_index = 0;

	for(u32 x = lcd_3D_stat.poly_min_x; x < lcd_3D_stat.poly_max_x; x++)
	{
		float z_start = 0.0;
		float z_end = 0.0;
		float z_inc = 0.0;

		//Calculate Z start and end fill coordinates
		z_start = lcd_3D_stat.hi_line_z[x];
		z_end = lcd_3D_stat.lo_line_z[x];
		
		z_inc = z_end - z_start;
		if((lcd_3D_stat.lo_fill[x] - lcd_3D_stat.hi_fill[x]) != 0) { z_inc /= float(lcd_3D_stat.lo_fill[x] - lcd_3D_stat.hi_fill[x]); }

		y_coord = lcd_3D_stat.hi_fill[x];

		while(y_coord < lcd_3D_stat.lo_fill[x])
		{
			//Convert plot points to buffer index
			buffer_index = (y_coord * 256) + x;

			//Check Z buffer if drawing is applicable
			if(z_start < gx_z_buffer[buffer_index])
			{
				gx_screen_buffer[(lcd_3D_stat.buffer_id + 1) & 0x1][buffer_index] = vert_colors[0];
				gx_render_buffer[(lcd_3D_stat.buffer_id + 1) & 0x1][buffer_index] = 1;
				gx_z_buffer[buffer_index] = z_start;
			}

			y_coord++;
			z_start += z_inc; 
		}
	}
}

/****** NDS 3D Software Renderer - Fills a given poly with interpolated colors from its vertices ******/
void NTR_LCD::fill_poly_interpolated()
{
	u8 y_coord = 0;
	u32 buffer_index = 0;

	for(u32 x = lcd_3D_stat.poly_min_x; x < lcd_3D_stat.poly_max_x; x++)
	{
		float z_start = 0.0;
		float z_end = 0.0;
		float z_inc = 0.0;

		u32 c1 = lcd_3D_stat.hi_color[x];
		u32 c2 = lcd_3D_stat.lo_color[x];
		float c_inc = 0;
		float c_ratio = 0;

		//Calculate Z start and end fill coordinates
		z_start = lcd_3D_stat.hi_line_z[x];
		z_end = lcd_3D_stat.lo_line_z[x];
		
		z_inc = z_end - z_start;

		if((lcd_3D_stat.lo_fill[x] - lcd_3D_stat.hi_fill[x]) != 0)
		{
			z_inc /= float(lcd_3D_stat.lo_fill[x] - lcd_3D_stat.hi_fill[x]);
			c_inc = 1.0 / (lcd_3D_stat.lo_fill[x] - lcd_3D_stat.hi_fill[x]);
		}

		y_coord = lcd_3D_stat.hi_fill[x];

		while(y_coord < lcd_3D_stat.lo_fill[x])
		{
			//Convert plot points to buffer index
			buffer_index = (y_coord * 256) + x;

			//Check Z buffer if drawing is applicable
			if(z_start < gx_z_buffer[buffer_index])
			{
				gx_screen_buffer[(lcd_3D_stat.buffer_id + 1) & 0x1][buffer_index] = interpolate_rgb(c1, c2, c_ratio);
				gx_render_buffer[(lcd_3D_stat.buffer_id + 1) & 0x1][buffer_index] = 1;
				gx_z_buffer[buffer_index] = z_start;
			}

			y_coord++;
			z_start += z_inc;
			c_ratio += c_inc;
		}
	}
}

/****** NDS 3D Software Renderer - Fills a given poly with color from a texture ******/
void NTR_LCD::fill_poly_textured()
{
	u8 y_coord = 0;
	u32 buffer_index = 0;
	u32 texel_index = 0;
	u32 texel = 0;

	u32 tex_size = lcd_3D_stat.tex_data.size();
	u32 tw = lcd_3D_stat.tex_src_width;

	u8 slot = (lcd_3D_stat.tex_offset >> 17);

	//Calculate VRAM address of texture
	u32 tex_addr = (mem->vram_tex_slot[slot] + (lcd_3D_stat.tex_offset & 0x1FFFF));

	//Generate pixel data from VRAM
	switch(lcd_3D_stat.tex_format)
	{
		case 0x1: gen_tex_1(tex_addr); break;
		case 0x2: gen_tex_2(tex_addr); break;
		case 0x3: gen_tex_3(tex_addr); break;
		case 0x4: gen_tex_4(tex_addr); break;
		case 0x6: gen_tex_6(tex_addr); break;
		case 0x7: gen_tex_7(tex_addr); break;
	}

	for(u32 x = lcd_3D_stat.poly_min_x; x < lcd_3D_stat.poly_max_x; x++)
	{
		float z_start = 0.0;
		float z_end = 0.0;
		float z_inc = 0.0;

		float tx1 = lcd_3D_stat.hi_tx[x];
		float tx2 = lcd_3D_stat.lo_tx[x];

		float ty1 = lcd_3D_stat.hi_ty[x];
		float ty2 = lcd_3D_stat.lo_ty[x];

		float tx_inc = tx2 - tx1;
		float ty_inc = ty2 - ty1;

		//Calculate Z start and end fill coordinates
		z_start = lcd_3D_stat.hi_line_z[x];
		z_end = lcd_3D_stat.lo_line_z[x];
		
		z_inc = z_end - z_start;

		if((lcd_3D_stat.lo_fill[x] - lcd_3D_stat.hi_fill[x]) != 0)
		{
			z_inc /= float(lcd_3D_stat.lo_fill[x] - lcd_3D_stat.hi_fill[x]);
			tx_inc /= float(lcd_3D_stat.lo_fill[x] - lcd_3D_stat.hi_fill[x]);
			ty_inc /= float(lcd_3D_stat.lo_fill[x] - lcd_3D_stat.hi_fill[x]);
		}

		y_coord = lcd_3D_stat.hi_fill[x];

		while(y_coord < lcd_3D_stat.lo_fill[x])
		{
			//Convert plot points to buffer index
			buffer_index = (y_coord * 256) + x;

			//Calculate texel postion
			texel_index = u32(u32(ty1) * tw) + u32(tx1);

			//Check Z buffer if drawing is applicable
			//Make sure texel exists as well
			if((z_start <= gx_z_buffer[buffer_index]) && (texel_index < tex_size) && (texel_index >= 0))
			{
				texel = lcd_3D_stat.tex_data[texel_index];

				//Draw texel if not transparent
				if(texel & 0xFF000000)
				{
					gx_screen_buffer[(lcd_3D_stat.buffer_id + 1) & 0x1][buffer_index] = lcd_3D_stat.tex_data[texel_index];
					gx_render_buffer[(lcd_3D_stat.buffer_id + 1) & 0x1][buffer_index] = 1;
					gx_z_buffer[buffer_index] = z_start;
				}
			}

			y_coord++;
			z_start += z_inc;

			tx1 += tx_inc;
			ty1 += ty_inc;
		}
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
			case 0x2:
				if(((lcd_3D_stat.poly_count + 1) < 2048) && ((lcd_3D_stat.vert_count + 3) < 6144))
				{
					current_matrix.resize(3, 3);
					gx_tri_strips.push_back(current_matrix);
					lcd_3D_stat.poly_count++;
					lcd_3D_stat.vert_count += 3;
					status = true;
				}

				else { status = false; }

				break;

			//Quad Strips
			case 0x3:
				if(((lcd_3D_stat.poly_count + 1) < 2048) && ((lcd_3D_stat.vert_count + 4) < 6144))
				{
					current_matrix.resize(4, 4);
					gx_quad_strips.push_back(current_matrix);
					lcd_3D_stat.poly_count++;
					lcd_3D_stat.vert_count += 4;
					status = true;
				}

				else { status = false; }

				break;
		}		
	}

	return status;
}	

/****** Parses and processes commands sent to the NDS 3D engine ******/
void NTR_LCD::process_gx_command()
{
	gx_matrix temp_matrix(4, 4);
	bool poly_draw = true;

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
					if(projection_sp < 1)
					{
						gx_projection_stack[projection_sp++] = gx_projection_matrix;
						lcd_3D_stat.gx_stat |= 0x2000;
					}

					else { gx_projection_stack[projection_sp] = gx_projection_matrix; }

					break;

				case 0x1:
				case 0x2:
					if(position_sp < 30) { gx_position_stack[position_sp++] = gx_position_matrix; }
					else { gx_position_stack[position_sp] = gx_position_matrix; }

					if(vector_sp < 30) { gx_vector_stack[vector_sp++] = gx_vector_matrix; }
					else { gx_vector_stack[vector_sp] = gx_vector_matrix; }

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
					if(projection_sp == 1)
					{
						gx_projection_matrix = gx_projection_stack[projection_sp--];
						lcd_3D_stat.gx_stat &= ~0x2000;
					}

					else { gx_projection_matrix = gx_projection_stack[projection_sp]; }

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
					if(lcd_3D_stat.current_gx_command == 0x17) { gx_projection_matrix = temp_matrix; }
					else { gx_projection_matrix = temp_matrix * gx_projection_matrix; }
					break;

				case 0x1:
					if(lcd_3D_stat.current_gx_command == 0x17) { gx_position_matrix = temp_matrix; }
					else { gx_position_matrix = temp_matrix * gx_position_matrix; }
					break;

				case 0x2:
					if(lcd_3D_stat.current_gx_command == 0x17)
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
					if(lcd_3D_stat.current_gx_command == 0x17) { gx_texture_matrix = temp_matrix; }
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

		//TEXCOORD
		case 0x22:
			{
				float result = 0.0;

				//Texture X
				u16 tx = read_param_u16(2);

				if(tx & 0x8000)
				{
					u16 p = ((tx >> 4) - 1);
					p = (~p & 0xF);
					result = -1.0 * p;
				}

				else { result = (tx >> 4); }
				if((tx & 0xF) != 0) { result += (tx & 0xF) / 16.0; }

				lcd_3D_stat.tex_coord_x[lcd_3D_stat.vertex_list_index] = result;

				//Texture Y
				u16 ty = read_param_u16(0);

				if(ty & 0x8000)
				{
					u16 p = ((ty >> 4) - 1);
					p = (~p & 0xF);
					result = -1.0 * p;
				}

				else { result = (ty >> 4); }
				if((ty & 0xF) != 0) { result += (ty & 0xF) / 16.0; }

				lcd_3D_stat.tex_coord_y[lcd_3D_stat.vertex_list_index] = result;

				//Set texture status
				lcd_3D_stat.use_texture = true;
			}

			break;

		//VTX_16
		case 0x23:
			//Push new polygon if necessary
			if(lcd_3D_stat.vertex_list_index == 0)
			{
				if(!poly_push(temp_matrix)) { poly_draw = false; }
			}

			if(poly_draw)
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

				std::vector<gx_matrix>* poly_list = NULL;
				u8 real_index = lcd_3D_stat.vertex_list_index;

				switch(lcd_3D_stat.vertex_mode)
				{
					//Triangles
					case 0x0:
						list_size = 3;
						poly_list = &gx_triangles;
						break;

					//Quads
					case 0x1:
						list_size = 4;
						poly_list = &gx_quads;
						break;

					//Triangle Strips
					case 0x2:
						{
							list_size = 3;
							poly_list = &gx_tri_strips;

							//Calculate last Triangle Strip position in vector
							u16 last_tri = gx_tri_strips.size() - 2;

							//When starting a new Triangle Strip, use previous 2 vertices from last defined strip if necessary
							if(lcd_3D_stat.begin_strips && !real_index)
							{
								//New V0 = Old V1 
								gx_tri_strips.back().data[0][0] = gx_tri_strips[last_tri].data[1][0];
								gx_tri_strips.back().data[0][1] = gx_tri_strips[last_tri].data[1][1];
								gx_tri_strips.back().data[0][2] = gx_tri_strips[last_tri].data[1][2];
								vert_colors[0] = vert_colors[1];
								lcd_3D_stat.tex_coord_x[0] = lcd_3D_stat.tex_coord_x[1]; 
								lcd_3D_stat.tex_coord_y[0] = lcd_3D_stat.tex_coord_y[1]; 

								//New V1 = Old V2
								gx_tri_strips.back().data[1][0] = gx_tri_strips[last_tri].data[2][0];
								gx_tri_strips.back().data[1][1] = gx_tri_strips[last_tri].data[2][1];
								gx_tri_strips.back().data[1][2] = gx_tri_strips[last_tri].data[2][2];
								vert_colors[1] = vert_colors[2];
								lcd_3D_stat.tex_coord_x[1] = lcd_3D_stat.tex_coord_x[2]; 
								lcd_3D_stat.tex_coord_y[1] = lcd_3D_stat.tex_coord_y[2]; 

								real_index = 2;
								lcd_3D_stat.vertex_list_index = 2;
							}

							lcd_3D_stat.begin_strips = true;
						}

						break;

					//Quad Strips
					case 0x3:
						{
							list_size = 4;
							poly_list = &gx_quad_strips;

							//Calculate last Quad Strip position in vector
							u16 last_quad = gx_quad_strips.size() - 2;

							//When starting a new Quad Strip, use previous 2 vertices from last defined strip if necessary
							if(lcd_3D_stat.begin_strips && !real_index)
							{
								//New V0 = Old V3
								gx_quad_strips.back().data[0][0] = gx_quad_strips[last_quad].data[3][0];
								gx_quad_strips.back().data[0][1] = gx_quad_strips[last_quad].data[3][1];
								gx_quad_strips.back().data[0][2] = gx_quad_strips[last_quad].data[3][2];
								vert_colors[0] = vert_colors[3];
								lcd_3D_stat.tex_coord_x[0] = lcd_3D_stat.tex_coord_x[3]; 
								lcd_3D_stat.tex_coord_y[0] = lcd_3D_stat.tex_coord_y[3]; 

								//New V1 = Old V2
								gx_quad_strips.back().data[1][0] = gx_quad_strips[last_quad].data[2][0];
								gx_quad_strips.back().data[1][1] = gx_quad_strips[last_quad].data[2][1];
								gx_quad_strips.back().data[1][2] = gx_quad_strips[last_quad].data[2][2];
								vert_colors[1] = vert_colors[2];
								lcd_3D_stat.tex_coord_x[1] = lcd_3D_stat.tex_coord_x[2]; 
								lcd_3D_stat.tex_coord_y[1] = lcd_3D_stat.tex_coord_y[2]; 

								real_index = 2;
								lcd_3D_stat.vertex_list_index = 2;
							}

							lcd_3D_stat.begin_strips = true;

							//Calculate correct internal vertex index for quad strips (V2 is last vertex, V3 is 2nd to last)
							if(real_index == 2) { real_index = 3; }
							else if(real_index == 3) { real_index = 2; }
						}

						break;
				}

				poly_list->back().data[real_index][0] = temp_result[1];
				poly_list->back().data[real_index][1] = temp_result[0];
				poly_list->back().data[real_index][2] = temp_result[3];

				lcd_3D_stat.last_x = temp_result[1];
				lcd_3D_stat.last_y = temp_result[0];
				lcd_3D_stat.last_z = temp_result[3];

				last_pos_matrix[real_index] = gx_position_matrix;

				//Set vertex color
				vert_colors[lcd_3D_stat.vertex_list_index] = lcd_3D_stat.vertex_color;

				lcd_3D_stat.vertex_list_index++;

				if(lcd_3D_stat.vertex_list_index == list_size)
				{
					lcd_3D_stat.vertex_list_index = 0;
					lcd_3D_stat.render_polygon = true;

					//Render geometry now if command was sent by GX FIFO
					if(mem->gx_command) { render_geometry(); }
				}
			}

			break;

		//VTX_10
		case 0x24:
			//Push new polygon if necessary
			if(lcd_3D_stat.vertex_list_index == 0)
			{
				if(!poly_push(temp_matrix)) { poly_draw = false; }
			}

			if(poly_draw)
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

				std::vector<gx_matrix>* poly_list = NULL;
				u8 real_index = lcd_3D_stat.vertex_list_index;

				switch(lcd_3D_stat.vertex_mode)
				{
					//Triangles
					case 0x0:
						list_size = 3;
						poly_list = &gx_triangles;
						break;

					//Quads
					case 0x1:
						list_size = 4;
						poly_list = &gx_quads;
						break;

					//Triangle Strips
					case 0x2:
						{
							list_size = 3;
							poly_list = &gx_tri_strips;

							//Calculate last Triangle Strip position in vector
							u16 last_tri = gx_tri_strips.size() - 2;

							//When starting a new Triangle Strip, use previous 2 vertices from last defined strip if necessary
							if(lcd_3D_stat.begin_strips && !real_index)
							{
								//New V0 = Old V1 
								gx_tri_strips.back().data[0][0] = gx_tri_strips[last_tri].data[1][0];
								gx_tri_strips.back().data[0][1] = gx_tri_strips[last_tri].data[1][1];
								gx_tri_strips.back().data[0][2] = gx_tri_strips[last_tri].data[1][2];
								vert_colors[0] = vert_colors[1];
								lcd_3D_stat.tex_coord_x[0] = lcd_3D_stat.tex_coord_x[1]; 
								lcd_3D_stat.tex_coord_y[0] = lcd_3D_stat.tex_coord_y[1]; 

								//New V1 = Old V2
								gx_tri_strips.back().data[1][0] = gx_tri_strips[last_tri].data[2][0];
								gx_tri_strips.back().data[1][1] = gx_tri_strips[last_tri].data[2][1];
								gx_tri_strips.back().data[1][2] = gx_tri_strips[last_tri].data[2][2];
								vert_colors[1] = vert_colors[2];
								lcd_3D_stat.tex_coord_x[1] = lcd_3D_stat.tex_coord_x[2]; 
								lcd_3D_stat.tex_coord_y[1] = lcd_3D_stat.tex_coord_y[2]; 

								real_index = 2;
								lcd_3D_stat.vertex_list_index = 2;
							}

							lcd_3D_stat.begin_strips = true;
						}

						break;

					//Quad Strips
					case 0x3:
						{
							list_size = 4;
							poly_list = &gx_quad_strips;

							//Calculate last Quad Strip position in vector
							u16 last_quad = gx_quad_strips.size() - 2;

							//When starting a new Quad Strip, use previous 2 vertices from last defined strip if necessary
							if(lcd_3D_stat.begin_strips && !real_index)
							{
								//New V0 = Old V3
								gx_quad_strips.back().data[0][0] = gx_quad_strips[last_quad].data[3][0];
								gx_quad_strips.back().data[0][1] = gx_quad_strips[last_quad].data[3][1];
								gx_quad_strips.back().data[0][2] = gx_quad_strips[last_quad].data[3][2];
								vert_colors[0] = vert_colors[3];
								lcd_3D_stat.tex_coord_x[0] = lcd_3D_stat.tex_coord_x[3]; 
								lcd_3D_stat.tex_coord_y[0] = lcd_3D_stat.tex_coord_y[3]; 

								//New V1 = Old V2
								gx_quad_strips.back().data[1][0] = gx_quad_strips[last_quad].data[2][0];
								gx_quad_strips.back().data[1][1] = gx_quad_strips[last_quad].data[2][1];
								gx_quad_strips.back().data[1][2] = gx_quad_strips[last_quad].data[2][2];
								vert_colors[1] = vert_colors[2];
								lcd_3D_stat.tex_coord_x[1] = lcd_3D_stat.tex_coord_x[2]; 
								lcd_3D_stat.tex_coord_y[1] = lcd_3D_stat.tex_coord_y[2]; 

								real_index = 2;
								lcd_3D_stat.vertex_list_index = 2;
							}

							lcd_3D_stat.begin_strips = true;

							//Calculate correct internal vertex index for quad strips (V2 is last vertex, V3 is 2nd to last)
							if(real_index == 2) { real_index = 3; }
							else if(real_index == 3) { real_index = 2; }
						}

						break;
				}

				poly_list->back().data[real_index][0] = temp_result[1];
				poly_list->back().data[real_index][1] = temp_result[0];
				poly_list->back().data[real_index][2] = temp_result[3];

				lcd_3D_stat.last_x = temp_result[1];
				lcd_3D_stat.last_y = temp_result[0];
				lcd_3D_stat.last_z = temp_result[3];

				last_pos_matrix[real_index] = gx_position_matrix;

				//Set vertex color
				vert_colors[lcd_3D_stat.vertex_list_index] = lcd_3D_stat.vertex_color;

				lcd_3D_stat.vertex_list_index++;

				if(lcd_3D_stat.vertex_list_index == list_size)
				{
					lcd_3D_stat.vertex_list_index = 0;
					lcd_3D_stat.render_polygon = true;

					//Render geometry now if command was sent by GX FIFO
					if(mem->gx_command) { render_geometry(); }
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
				if(!poly_push(temp_matrix)) { poly_draw = false; }
			}

			if(poly_draw)
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

				std::vector<gx_matrix>* poly_list = NULL;
				u8 real_index = lcd_3D_stat.vertex_list_index;

				switch(lcd_3D_stat.vertex_mode)
				{
					//Triangles
					case 0x0:
						list_size = 3;
						poly_list = &gx_triangles;
						break;

					//Quads
					case 0x1:
						list_size = 4;
						poly_list = &gx_quads;
						break;

					//Triangle Strips
					case 0x2:
						{
							list_size = 3;
							poly_list = &gx_tri_strips;

							//Calculate last Triangle Strip position in vector
							u16 last_tri = gx_tri_strips.size() - 2;

							//When starting a new Triangle Strip, use previous 2 vertices from last defined strip if necessary
							if(lcd_3D_stat.begin_strips && !real_index)
							{
								//New V0 = Old V1 
								gx_tri_strips.back().data[0][0] = gx_tri_strips[last_tri].data[1][0];
								gx_tri_strips.back().data[0][1] = gx_tri_strips[last_tri].data[1][1];
								gx_tri_strips.back().data[0][2] = gx_tri_strips[last_tri].data[1][2];
								vert_colors[0] = vert_colors[1];
								lcd_3D_stat.tex_coord_x[0] = lcd_3D_stat.tex_coord_x[1]; 
								lcd_3D_stat.tex_coord_y[0] = lcd_3D_stat.tex_coord_y[1];

								//New V1 = Old V2
								gx_tri_strips.back().data[1][0] = gx_tri_strips[last_tri].data[2][0];
								gx_tri_strips.back().data[1][1] = gx_tri_strips[last_tri].data[2][1];
								gx_tri_strips.back().data[1][2] = gx_tri_strips[last_tri].data[2][2];
								vert_colors[1] = vert_colors[2];
								lcd_3D_stat.tex_coord_x[1] = lcd_3D_stat.tex_coord_x[2]; 
								lcd_3D_stat.tex_coord_y[1] = lcd_3D_stat.tex_coord_y[2]; 

								real_index = 2;
								lcd_3D_stat.vertex_list_index = 2;
							}

							lcd_3D_stat.begin_strips = true;
						}

						break;

					//Quad Strips
					case 0x3:
						{
							list_size = 4;
							poly_list = &gx_quad_strips;

							//Calculate last Quad Strip position in vector
							u16 last_quad = gx_quad_strips.size() - 2;

							//When starting a new Quad Strip, use previous 2 vertices from last defined strip if necessary
							if(lcd_3D_stat.begin_strips && !real_index)
							{
								//New V0 = Old V3
								gx_quad_strips.back().data[0][0] = gx_quad_strips[last_quad].data[3][0];
								gx_quad_strips.back().data[0][1] = gx_quad_strips[last_quad].data[3][1];
								gx_quad_strips.back().data[0][2] = gx_quad_strips[last_quad].data[3][2];
								vert_colors[0] = vert_colors[3];
								lcd_3D_stat.tex_coord_x[0] = lcd_3D_stat.tex_coord_x[3]; 
								lcd_3D_stat.tex_coord_y[0] = lcd_3D_stat.tex_coord_y[3];

								//New V1 = Old V2
								gx_quad_strips.back().data[1][0] = gx_quad_strips[last_quad].data[2][0];
								gx_quad_strips.back().data[1][1] = gx_quad_strips[last_quad].data[2][1];
								gx_quad_strips.back().data[1][2] = gx_quad_strips[last_quad].data[2][2];
								vert_colors[1] = vert_colors[2];
								lcd_3D_stat.tex_coord_x[1] = lcd_3D_stat.tex_coord_x[2]; 
								lcd_3D_stat.tex_coord_y[1] = lcd_3D_stat.tex_coord_y[2]; 

								real_index = 2;
								lcd_3D_stat.vertex_list_index = 2;
							}

							lcd_3D_stat.begin_strips = true;

							//Calculate correct internal vertex index for quad strips (V2 is last vertex, V3 is 2nd to last)
							if(real_index == 2) { real_index = 3; }
							else if(real_index == 3) { real_index = 2; }
						}

						break;
				}

				//XY
				if(lcd_3D_stat.current_gx_command == 0x25)
				{
					poly_list->back().data[real_index][0] = temp_result[0];
					poly_list->back().data[real_index][1] = temp_result[1];
					poly_list->back().data[real_index][2] = lcd_3D_stat.last_z;

					lcd_3D_stat.last_x = temp_result[0];
					lcd_3D_stat.last_y = temp_result[1];
				}

				//XZ
				if(lcd_3D_stat.current_gx_command == 0x26)
				{
					poly_list->back().data[real_index][0] = temp_result[0];
					poly_list->back().data[real_index][1] = lcd_3D_stat.last_y;
					poly_list->back().data[real_index][2] = temp_result[1];

					lcd_3D_stat.last_x = temp_result[0];
					lcd_3D_stat.last_z = temp_result[1];
				}

				//YZ
				if(lcd_3D_stat.current_gx_command == 0x27)
				{
					poly_list->back().data[real_index][0] = lcd_3D_stat.last_x;
					poly_list->back().data[real_index][1] = temp_result[0];
					poly_list->back().data[real_index][2] = temp_result[1];

					lcd_3D_stat.last_y = temp_result[0];
					lcd_3D_stat.last_z = temp_result[1];
				}

				last_pos_matrix[real_index] = gx_position_matrix;

				//Set vertex color
				vert_colors[lcd_3D_stat.vertex_list_index] = lcd_3D_stat.vertex_color;

				lcd_3D_stat.vertex_list_index++;

				if(lcd_3D_stat.vertex_list_index == list_size)
				{
					lcd_3D_stat.vertex_list_index = 0;
					lcd_3D_stat.render_polygon = true;

					//Render geometry now if command was sent by GX FIFO
					if(mem->gx_command) { render_geometry(); }
				}
			}

			break;

		//TEXIMAGE_PARAM
		case 0x2A:
			{
				u32 raw_value = read_param_u32(0);
				
				lcd_3D_stat.tex_offset = ((raw_value & 0xFFFF) << 3);
				lcd_3D_stat.tex_src_width = 8 << ((raw_value >> 20) & 0x7);
				lcd_3D_stat.tex_src_height = 8 << ((raw_value >> 23) & 0x7);
				lcd_3D_stat.tex_format = ((raw_value >> 26) & 0x7);
				lcd_3D_stat.tex_transformation = (raw_value >> 30);

				u8 slot = (lcd_3D_stat.tex_offset >> 17);

				//Calculate VRAM address of texture
				u32 tex_addr = (mem->vram_tex_slot[slot] + (lcd_3D_stat.tex_offset & 0x1FFFF));

				//Generate pixel data from VRAM
				switch(lcd_3D_stat.tex_format)
				{
					case 0x1: gen_tex_1(tex_addr); break;
					case 0x2: gen_tex_2(tex_addr); break;
					case 0x3: gen_tex_3(tex_addr); break;
					case 0x4: gen_tex_4(tex_addr); break;
					case 0x6: gen_tex_6(tex_addr); break;
					case 0x7: gen_tex_7(tex_addr); break;
				}
			}

			break;

		//PLTT_BASE
		case 0x2B:
			lcd_3D_stat.pal_base = read_param_u32(0) & 0x1FFF;
			break;	

		//BEGIN_VTXS:
		case 0x40:
			//If, for some reason a polygon was not completed, start over now
			if(lcd_3D_stat.vertex_list_index)
			{
				switch(lcd_3D_stat.vertex_mode)
				{
					case 0x0: gx_triangles.pop_back(); break;
					case 0x1: gx_quads.pop_back(); break;
					case 0x3: gx_quad_strips.pop_back(); break;
				}

				lcd_3D_stat.vertex_list_index = 0;
			}

			//Reset texture status
			lcd_3D_stat.use_texture = false;

			lcd_3D_stat.vertex_mode = (lcd_3D_stat.command_parameters[3] & 0x3);

			break;

		//END_VTXS:
		case 0x41:
			lcd_3D_stat.begin_strips = false;
			break;

		//VIEWPORT
		case 0x60:
			lcd_3D_stat.view_port_y2 = (lcd_3D_stat.command_parameters[0] & 0xBF);
			lcd_3D_stat.view_port_x2 = lcd_3D_stat.command_parameters[1];
			lcd_3D_stat.view_port_y1 = (lcd_3D_stat.command_parameters[2] & 0xBF);
			lcd_3D_stat.view_port_x1 = lcd_3D_stat.command_parameters[3];
			break;
	}

	//Process GXFIFO commands
	if(!mem->nds9_gx_fifo.empty())
	{
		mem->nds9_gx_fifo.pop();
		
		if(!mem->nds9_gx_fifo.empty())
		{
			//Update parameters
			lcd_3D_stat.fifo_params >>= 8;
			u8 length = (lcd_3D_stat.fifo_params & 0xFF);

			for(u32 x = 0; x < length; x++)
			{
				u8 y = (x * 4);

				lcd_3D_stat.command_parameters[y] = lcd_3D_stat.command_parameters[lcd_3D_stat.parameter_index++];
				lcd_3D_stat.command_parameters[y + 1] = lcd_3D_stat.command_parameters[lcd_3D_stat.parameter_index++];
				lcd_3D_stat.command_parameters[y + 2] = lcd_3D_stat.command_parameters[lcd_3D_stat.parameter_index++];
				lcd_3D_stat.command_parameters[y + 3] = lcd_3D_stat.command_parameters[lcd_3D_stat.parameter_index++];
			}

			lcd_3D_stat.current_gx_command = mem->nds9_gx_fifo.front();
			process_gx_command();
		}
	}

	//GXFIFO half empty IRQ
	if((lcd_3D_stat.gx_stat & 0xC0000000) == 0x40000000) { mem->nds9_if |= 0x200000; }

	//GXFIFO empty status
	if(mem->nds9_gx_fifo.empty())
	{
		lcd_3D_stat.gx_stat |= 0x4000000;
						
		//GXFIFO empty IRQ
		if((lcd_3D_stat.gx_stat & 0xC0000000) == 0x80000000) { mem->nds9_if |= 0x200000; }
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

/****** Interpolates 2 32-bit colors ******/
u32 NTR_LCD::interpolate_rgb(u32 color_1, u32 color_2, float ratio)
{
	u8 r1 = (color_1 >> 16);
	u8 r2 = (color_2 >> 16);
	int r = ((r2 - r1) * ratio) + r1;

	u8 g1 = (color_1 >> 8);
	u8 g2 = (color_2 >> 8);
	int g = ((g2 - g1) * ratio) + g1; 

	u8 b1 = (color_1 & 0xFF);
	u8 b2 = (color_2 & 0xFF);
	int b = ((b2 - b1) * ratio) + b1; 
	
	return 0xFF000000 | (r << 16) | (g << 8) | (b);
}

/****** Generates pixel data fram VRAM for A3I5 textures ******/
void NTR_LCD::gen_tex_1(u32 address)
{
	lcd_3D_stat.tex_data.clear();
	u32 tex_size = (lcd_3D_stat.tex_src_width * lcd_3D_stat.tex_src_height);

	//Generate temporary palette
	u32 pal_addr = lcd_3D_stat.pal_bank_addr + (lcd_3D_stat.pal_base * 0x8);
	u32 tex_pal[32];

	for(u32 x = 0; x < 32; x++)
	{
		tex_pal[x] = get_rgb15(mem->read_u16_fast(pal_addr));
		pal_addr += 2;
	}

	//First palette color is used for transparency
	tex_pal[0] &= ~0xFF000000;

	while(tex_size)
	{
		u8 index = mem->memory_map[address++];
		lcd_3D_stat.tex_data.push_back(tex_pal[index & 0x1F]);
		tex_size--;
	}
}

/****** Generates pixel data from VRAM for 4 color textures ******/
void NTR_LCD::gen_tex_2(u32 address)
{
	lcd_3D_stat.tex_data.clear();
	u32 tex_size = (lcd_3D_stat.tex_src_width * lcd_3D_stat.tex_src_height);

	//Generate temporary palette
	u32 pal_addr = lcd_3D_stat.pal_bank_addr + (lcd_3D_stat.pal_base * 0x8);
	u32 tex_pal[4];

	for(u32 x = 0; x < 4; x++)
	{
		tex_pal[x] = get_rgb15(mem->read_u16_fast(pal_addr));
		pal_addr += 2;
	}

	while(tex_size)
	{
		u8 index = mem->memory_map[address++];
		lcd_3D_stat.tex_data.push_back(tex_pal[index & 0x3]);
		lcd_3D_stat.tex_data.push_back(tex_pal[(index >> 2) & 0x3]);
		lcd_3D_stat.tex_data.push_back(tex_pal[(index >> 4) & 0x3]);
		lcd_3D_stat.tex_data.push_back(tex_pal[(index >> 6) & 0x3]);
		tex_size -= 4;
	}
}

/****** Generates pixel data from VRAM for 16 color textures ******/
void NTR_LCD::gen_tex_3(u32 address)
{
	lcd_3D_stat.tex_data.clear();
	u32 tex_size = (lcd_3D_stat.tex_src_width * lcd_3D_stat.tex_src_height);

	//Generate temporary palette
	u32 pal_addr = lcd_3D_stat.pal_bank_addr + (lcd_3D_stat.pal_base * 0x10);
	u32 tex_pal[16];

	for(u32 x = 0; x < 16; x++)
	{
		tex_pal[x] = get_rgb15(mem->read_u16_fast(pal_addr));
		pal_addr += 2;
	}

	//First palette color is used for transparency
	tex_pal[0] &= ~0xFF000000;

	while(tex_size)
	{
		u8 index = mem->memory_map[address++];
		lcd_3D_stat.tex_data.push_back(tex_pal[index & 0xF]);
		lcd_3D_stat.tex_data.push_back(tex_pal[index >> 4]);
		tex_size -= 2;
	}
}

/****** Generates pixel data from VRAM for 256 color textures ******/
void NTR_LCD::gen_tex_4(u32 address)
{
	lcd_3D_stat.tex_data.clear();
	u32 tex_size = (lcd_3D_stat.tex_src_width * lcd_3D_stat.tex_src_height);

	//Generate temporary palette
	u32 pal_addr = lcd_3D_stat.pal_bank_addr + (lcd_3D_stat.pal_base * 0x10);
	u32 tex_pal[256];

	for(u32 x = 0; x < 256; x++)
	{
		tex_pal[x] = get_rgb15(mem->read_u16_fast(pal_addr));
		pal_addr += 2;
	}

	//First palette color is used for transparency
	tex_pal[0] &= ~0xFF000000;

	while(tex_size)
	{
		u8 index = mem->memory_map[address++];
		lcd_3D_stat.tex_data.push_back(tex_pal[index]);
		tex_size--;
	}
}

/****** Generates pixel data fram VRAM for A5I3 textures ******/
void NTR_LCD::gen_tex_6(u32 address)
{
	lcd_3D_stat.tex_data.clear();
	u32 tex_size = (lcd_3D_stat.tex_src_width * lcd_3D_stat.tex_src_height);

	//Generate temporary palette
	u32 pal_addr = lcd_3D_stat.pal_bank_addr + (lcd_3D_stat.pal_base * 0x8);
	u32 tex_pal[8];

	for(u32 x = 0; x < 8; x++)
	{
		tex_pal[x] = get_rgb15(mem->read_u16_fast(pal_addr));
		pal_addr += 2;
	}

	//First palette color is used for transparency
	tex_pal[0] &= ~0xFF000000;

	while(tex_size)
	{
		u8 index = mem->memory_map[address++];
		lcd_3D_stat.tex_data.push_back(tex_pal[index & 0x7]);
		tex_size--;
	}
}

/****** Generates pixel data from VRAM for Direct Color textures ******/
void NTR_LCD::gen_tex_7(u32 address)
{
	lcd_3D_stat.tex_data.clear();
	u32 tex_size = (lcd_3D_stat.tex_src_width * lcd_3D_stat.tex_src_height);

	while(tex_size)
	{
		u16 color = mem->read_u16_fast(address);
		u32 final_color = (color & 0x8000) ? get_rgb15(color) : (get_rgb15(color) & ~0xFF000000);
		lcd_3D_stat.tex_data.push_back(final_color);

		address += 2;
		tex_size--;
	}
}
