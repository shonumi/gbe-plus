// GB Enhanced Copyright Daniel Baxter 2013
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd.cpp
// Date : August 16, 2014
// Description : Game Boy Advance LCD emulation
//
// Draws background, window, and sprites to screen
// Responsible for blitting pixel data and limiting frame rate

#include <cmath>

#include "lcd.h"

/****** LCD Constructor ******/
LCD::LCD()
{
	reset();
}

/****** LCD Destructor ******/
LCD::~LCD()
{
	screen_buffer.clear();
	scanline_buffer.clear();
	std::cout<<"LCD::Shutdown\n";
}

/****** Reset LCD ******/
void LCD::reset()
{
	final_screen = NULL;
	mem = NULL;

	scanline_buffer.clear();
	screen_buffer.clear();

	lcd_clock = 0;
	lcd_mode = 0;

	frame_start_time = 0;
	frame_current_time = 0;
	fps_count = 0;
	fps_time = 0;

	current_scanline = 0;
	scanline_pixel_counter = 0;

	screen_buffer.resize(0x9600, 0);
	scanline_buffer.resize(0x100, 0);

	bg_params[0].bg_lut.resize(0x10000, 0xFFFFFFFF);

	//Initialize various LCD status variables
	lcd_stat.oam_update = false;
	lcd_stat.oam_update_list.resize(128, false);

	lcd_stat.bg_pal_update = true;
	lcd_stat.bg_pal_update_list.resize(256, true);

	lcd_stat.obj_pal_update = true;
	lcd_stat.obj_pal_update_list.resize(256, true);

	lcd_stat.bg_params_update = true;

	lcd_stat.frame_base = 0x6000000;
	lcd_stat.bg_mode = 0;

	for(int x = 0, y = 255; x < 255; x++, y--)
	{
		lcd_stat.bg_flip_lut[x] = (y % 8);
	}

	for(int y = 0; y < 256; y++)
	{
		for(int x = 0; x < 256; x++)
		{
			lcd_stat.bg_tile_lut[x][y] = ((y % 8) * 8) + (x % 8);
		}
	}

	for(int y = 0; y < 256; y++)
	{
		for(int x = 0; x < 256; x++)
		{
			lcd_stat.bg_num_lut[x][y] = ((y / 8) * 32) + (x / 8);
		}
	}


	for(int x = 0; x < 512; x++)
	{
		lcd_stat.screen_offset_lut[x] = (x > 255) ? 0x800 : 0x0;
	}

	for(int x = 0; x < 4; x++)
	{
		lcd_stat.bg_control[x] = 0;
		lcd_stat.bg_enable[x] = false;
		lcd_stat.bg_offset_x[x] = 0;
		lcd_stat.bg_offset_y[x] = 0;
		lcd_stat.bg_priority[x] = 0;
		lcd_stat.bg_depth[x] = 4;
		lcd_stat.bg_size[x] = 0;
		lcd_stat.bg_base_tile_addr[x] = 0x6000000;
		lcd_stat.bg_base_map_addr[x] = 0x6000000;
		lcd_stat.mode_0_width[x] = 256;
		lcd_stat.mode_0_height[x] = 256;
	}
}

/****** Initialize LCD with SDL ******/
bool LCD::init()
{
	if(SDL_Init(SDL_INIT_EVERYTHING) == -1)
	{
		std::cout<<"LCD::Error - Could not initialize SDL\n";
		return false;
	}

	if(config::use_opengl) { opengl_init(); }
	else { final_screen = SDL_SetVideoMode(240, 160, 32, SDL_SWSURFACE); }

	if(final_screen == NULL) { return false; }

	std::cout<<"LCD::Initialized\n";

	return true;
}

/****** Updates OAM entries when values in memory change ******/
void LCD::update_oam()
{
	lcd_stat.oam_update = false;
	
	u32 oam_ptr = 0x7000000;
	u16 attribute = 0;

	for(int x = 0; x < 128; x++)
	{
		//Update if OAM entry has changed
		if(lcd_stat.oam_update_list[x])
		{
			lcd_stat.oam_update_list[x] = false;

			//Read and parse Attribute 0
			attribute = mem->read_u16_fast(oam_ptr);
			oam_ptr += 2;

			obj[x].y = (attribute & 0xFF);
			obj[x].rotate_scale = (attribute & 0x100) ? 1 : 0;
			obj[x].type = (attribute & 0x200) ? 1 : 0;
			obj[x].bit_depth = (attribute & 0x2000) ? 8 : 4;
			obj[x].shape = (attribute >> 14);
			if((obj[x].rotate_scale == 0) && (obj[x].type == 1)) { obj[x].visible = false; }
			else { obj[x].visible = true; }

			//Read and parse Attribute 1
			attribute = mem->read_u16_fast(oam_ptr);
			oam_ptr += 2;

			obj[x].x = (attribute & 0x1FF);
			obj[x].h_flip = (attribute & 0x1000) ? true : false;
			obj[x].v_flip = (attribute & 0x2000) ? true : false;
			obj[x].size = (attribute >> 14);

			//Read and parse Attribute 2
			attribute = mem->read_u16_fast(oam_ptr);
			oam_ptr += 4;

			obj[x].tile_number = (attribute & 0x3FF);
			obj[x].bg_priority = ((attribute >> 10) & 0x3);
			obj[x].palette_number = ((attribute >> 12) & 0xF); 

			//Determine dimensions of the sprite
			switch(obj[x].size)
			{
				//Size 0 - 8x8, 16x8, 8x16
				case 0x0:
					if(obj[x].shape == 0) { obj[x].width = 8; obj[x].height = 8; }
					else if(obj[x].shape == 1) { obj[x].width = 16; obj[x].height = 8; }
					else if(obj[x].shape == 2) { obj[x].width = 8; obj[x].height = 16; }
					break;

				//Size 1 - 16x16, 32x8, 8x32
				case 0x1:
					if(obj[x].shape == 0) { obj[x].width = 16; obj[x].height = 16; }
					else if(obj[x].shape == 1) { obj[x].width = 32; obj[x].height = 8; }
					else if(obj[x].shape == 2) { obj[x].width = 8; obj[x].height = 32; }
					break;

				//Size 2 - 32x32, 32x16, 16x32
				case 0x2:
					if(obj[x].shape == 0) { obj[x].width = 32; obj[x].height = 32; }
					else if(obj[x].shape == 1) { obj[x].width = 32; obj[x].height = 16; }
					else if(obj[x].shape == 2) { obj[x].width = 16; obj[x].height = 32; }
					break;

				//Size 3 - 64x64, 64x32, 32x64
				case 0x3:
					if(obj[x].shape == 0) { obj[x].width = 64; obj[x].height = 64; }
					else if(obj[x].shape == 1) { obj[x].width = 64; obj[x].height = 32; }
					else if(obj[x].shape == 2) { obj[x].width = 32; obj[x].height = 64; }
					break;
			}

			//Set double-size
			if((obj[x].rotate_scale == 1) && (obj[x].type == 1)) { obj[x].x += (obj[x].width >> 1); obj[x].y += (obj[x].height >> 1); }
		}
	}

	//Update render list for the current scanline
	update_obj_render_list();
}

/****** Updates a list of OBJs to render on the current scanline ******/
void LCD::update_obj_render_list()
{
	obj_render_length = 0;

	//Cycle through all of the sprites
	for(int x = 0; x < 128; x++)
	{
		//Check to see if sprite is rendered on the current scanline
		if((obj[x].visible) && (current_scanline >= obj[x].y) && (current_scanline <= (obj[x].y + obj[x].height - 1)))
		{
			obj_render_list[obj_render_length++] = x;
		}
	}
}

/****** Updates palette entries when values in memory change ******/
void LCD::update_palettes()
{
	//Update BG palettes
	if(lcd_stat.bg_pal_update)
	{
		lcd_stat.bg_pal_update = false;

		//Cycle through all updates to BG palettes
		for(int x = 0; x < 256; x++)
		{
			//If this palette has been updated, convert to ARGB
			if(lcd_stat.bg_pal_update_list[x])
			{
				lcd_stat.bg_pal_update_list[x] = false;

				u16 color_bytes = mem->read_u16_fast(0x5000000 + (x << 1));

				u8 red = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 green = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 blue = ((color_bytes & 0x1F) << 3);

				pal[x][0] =  0xFF000000 | (red << 16) | (green << 8) | (blue);
			}
		}
	}

	//Update OBJ palettes
	if(lcd_stat.obj_pal_update)
	{
		lcd_stat.obj_pal_update = false;

		//Cycle through all updates to OBJ palettes
		for(int x = 0; x < 256; x++)
		{
			//If this palette has been updated, convert to ARGB
			if(lcd_stat.obj_pal_update_list[x])
			{
				lcd_stat.obj_pal_update_list[x] = false;

				u16 color_bytes = mem->read_u16_fast(0x5000200 + (x << 1));

				u8 red = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 green = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 blue = ((color_bytes & 0x1F) << 3);

				pal[x][1] =  0xFF000000 | (red << 16) | (green << 8) | (blue);
			}
		}
	}
}

/****** Updates BG scaling + rotation parameters when values change in memory ******/
void LCD::update_bg_params()
{
	//TODO - BG3 parameters
	u32 bg2_param_addr = BG2PA;

	//Update BG2 parameters
	for(int x = 0; x < 4; x++)
	{
		u16 raw_value = mem->read_u16_fast(bg2_param_addr);
		bg2_param_addr += 2;
		
		//Grab the fractional and integer portions, respectively
		double final_value = 0.0;
		if((raw_value & 0xFF) != 0) { final_value = (raw_value & 0xFF) / 256.0; }
		final_value += (raw_value >> 8) & 0x7F;
		if(raw_value & 0x8000) { final_value *= -1.0; }

		switch(x)
		{ 
			case 0: bg_params[0].a = final_value; break;
			case 1: bg_params[0].b = final_value; break;
			case 2: bg_params[0].c = final_value; break;
			case 3: bg_params[0].d = final_value; break;
		}
	}

	u32 x_raw = mem->read_u32_fast(BG2X_L);
	u32 y_raw = mem->read_u32_fast(BG2Y_L);
	bg_params[0].x_ref = 0.0;
	bg_params[0].y_ref = 0.0;

	//Update BG2 X reference, integer then fraction
	//Note: The reference points are 19-bit signed 2's complement, not mentioned anywhere in docs...
	if(x_raw & 0x8000000) 
	{ 
		u16 x = (((x_raw >> 8) & 0x7FFFF) - 1);
		x = ~x;
		bg_params[0].x_ref = -1.0 * x;
	}
	else { bg_params[0].x_ref = (x_raw >> 8) & 0x7FFFF; }
	if((x_raw & 0xFF) != 0) { bg_params[0].x_ref += (x_raw & 0xFF) / 256.0; }
	
	//Update BG2 Y reference, integer then fraction
	if(y_raw & 0x8000000) 
	{ 
		u16 y = (((y_raw >> 8) & 0x7FFFF) - 1);
		y = ~y;
		bg_params[0].y_ref = -1.0 * y;
	}
	else { bg_params[0].y_ref = (y_raw >> 8) & 0x7FFFF; }
	if((y_raw & 0xFF) != 0) { bg_params[0].y_ref += (y_raw & 0xFF) / 256.0; }
	
	//Get BG2 size in pixels
	u16 bg_size = (16 << (lcd_stat.bg_control[2] >> 14)) << 3;

	double out_x = bg_params[0].x_ref; 
	double out_y = bg_params[0].y_ref;

	double temp_x = 0.0;
	double temp_y = 0.0;

	u16 src_x = 0;
	u16 src_y = 0;

	//Create a precalculated LUT (look-up table) of all transformed positions
	//For example, when rendering at (120, 80), we need to determine what pixel was rotated/scaled into this position
	for(int x = 0; x < 0x9600; x++, src_x++)
	{
		//Increment reference points by DMX and DMY for calculations
		if((x % 240 == 0) && (x > 0))
		{
			bg_params[0].x_ref += bg_params[0].b;
			bg_params[0].y_ref += bg_params[0].d;

			out_x = bg_params[0].x_ref;
			out_y = bg_params[0].y_ref;

			src_x = 0;
			src_y++;
		}

		//Calculate old position
		u32 old_pos = (src_y * 240) + src_x;

		//Calculate new position using new coordinate space, store to LUT
		out_x += bg_params[0].a;
		out_y += bg_params[0].c;

		//Round results to nearest integer
		temp_x = (out_x > 0) ? floor(out_x + 0.5) : ceil(out_x - 0.5);
		temp_y = (out_y > 0) ? floor(out_y + 0.5) : ceil(out_y - 0.5);

		//Cheap way to do modulus on rounded decimals
		while(temp_x >= bg_size) { temp_x -= bg_size; }
		while(temp_y >= bg_size) { temp_y -= bg_size; }

		u32 new_pos = (temp_y * bg_size) + temp_x;
		
		if(old_pos < 0x9600) { bg_params[0].bg_lut[old_pos] = new_pos; }
	}

	lcd_stat.bg_params_update = false;
}

/****** Determines if a sprite pixel should be rendered, and if so draws it to the current scanline pixel ******/
bool LCD::render_sprite_pixel()
{
	//If sprites are disabled, quit now
	if((lcd_stat.display_control & 0x1000) == 0) { return false; }

	//If no sprites are rendered on this line, quit now
	if(obj_render_list == 0) { return false; }

	u8 sprite_id = 0;
	u32 sprite_tile_addr = 0;
	u32 meta_sprite_tile = 0;
	u8 raw_color = 0;

	//Cycle through all sprites that are rendering on this pixel, draw them according to their priority
	for(int x = 0; x < obj_render_length; x++)
	{
		sprite_id = obj_render_list[x];

		//Check to see if current_scanline_pixel is within sprite
		if((scanline_pixel_counter >= obj[sprite_id].x) && (scanline_pixel_counter <= (obj[sprite_id].x + obj[sprite_id].width - 1))) 
		{
			//Determine the internal X-Y coordinates of the sprite's pixel
			u16 sprite_tile_pixel_x = scanline_pixel_counter - obj[sprite_id].x;
			u16 sprite_tile_pixel_y = current_scanline - obj[sprite_id].y;

			//Horizontal flip the internal X coordinate
			if(obj[sprite_id].h_flip)
			{
				u16 h_flip = sprite_tile_pixel_x;
				u8 width = obj[sprite_id].width;
				u8 f_width = (width - 1);
			
				//Horizontal flipping
				if(h_flip < (width/2)) 
				{
					h_flip = ((f_width - h_flip) - h_flip);
					sprite_tile_pixel_x += h_flip;
				}

				else
				{
					h_flip = f_width - (2 * (f_width - h_flip));
					sprite_tile_pixel_x -= h_flip;
				}
			}

			//Vertical flip the internal Y coordinate
			if(obj[sprite_id].v_flip)
			{
				u16 v_flip = sprite_tile_pixel_y;
				u8 height = obj[sprite_id].height;
				u8 f_height = (height - 1);
			
	
				//Vertical flipping
				if(v_flip < (height/2)) 
				{
					v_flip = ((f_height - v_flip) - v_flip);
					sprite_tile_pixel_y += v_flip;
				}

				else
				{
					v_flip = f_height - (2 * (f_height - v_flip));
					sprite_tile_pixel_y -= v_flip;
				}
			}

			//Determine meta x-coordinate of rendered sprite pixel
			u8 meta_x = (sprite_tile_pixel_x / 8);

			//Determine meta Y-coordinate of rendered sprite pixel
			u8 meta_y = (sprite_tile_pixel_y / 8);

			//Determine which 8x8 section to draw pixel from, and what tile that actually represents in VRAM
			if(lcd_stat.display_control & 0x40)
			{
				meta_sprite_tile = (meta_y * (obj[sprite_id].width/8)) + meta_x;	
			}

			else
			{
				meta_sprite_tile = (meta_y * 32) + meta_x;
			}

			sprite_tile_addr = 0x6010000 + (obj[sprite_id].tile_number << 5) +  (meta_sprite_tile * (obj[sprite_id].bit_depth << 3));

			meta_x = (sprite_tile_pixel_x % 8);
			meta_y = (sprite_tile_pixel_y % 8);

			u8 sprite_tile_pixel = (meta_y * 8) + meta_x;

			//Grab the byte corresponding to (sprite_tile_pixel), render it as ARGB - 4-bit version
			if(obj[sprite_id].bit_depth == 4)
			{
				sprite_tile_addr += (sprite_tile_pixel >> 1);
				raw_color = mem->memory_map[sprite_tile_addr];

				if((sprite_tile_pixel % 2) == 0) { raw_color &= 0xF; }
				else { raw_color >>= 4; }

				if(raw_color != 0) 
				{
					scanline_buffer[scanline_pixel_counter] = pal[((obj[sprite_id].palette_number * 32) + (raw_color * 2)) >> 1][1];
					last_obj_priority = obj[sprite_id].bg_priority;
					return true;
				}
			}

			//Grab the byte corresponding to (sprite_tile_pixel), render it as ARGB - 8-bit version
			else
			{
				sprite_tile_addr += sprite_tile_pixel;
				raw_color = mem->memory_map[sprite_tile_addr];

				if(raw_color != 0) 
				{
					scanline_buffer[scanline_pixel_counter] = pal[raw_color][1];
					last_obj_priority = obj[sprite_id].bg_priority;
					return true;
				}
			}
		}
	}

	//Return false if nothing was drawn
	return false;
}

/****** Determines if a background pixel should be rendered, and if so draws it to the current scanline pixel ******/
bool LCD::render_bg_pixel(u32 bg_control)
{
	if((!lcd_stat.bg_enable[0]) && (bg_control == BG0CNT)) { return false; }
	else if((!lcd_stat.bg_enable[1]) && (bg_control == BG1CNT)) { return false; }
	else if((!lcd_stat.bg_enable[2]) && (bg_control == BG2CNT)) { return false; }
	else if((!lcd_stat.bg_enable[3]) && (bg_control == BG3CNT)) { return false; }

	//Render BG pixel according to current BG Mode
	switch(lcd_stat.bg_mode)
	{
		//BG Mode 0
		case 0:
			return render_bg_mode_0(bg_control); break;

		//BG Mode 1
		case 1:
			//Render BG2 as Scaled+Rotation
			if(bg_control == BG2CNT) { return render_bg_mode_1(bg_control); }

			//BG3 is never drawn in Mode 1
			else if(bg_control == BG3CNT) { return false; }

			//Render BG0 and BG1 as Text (same as Mode 0)
			else { return render_bg_mode_0(bg_control); } 

			break;

		//BG Mode 3
		case 3:
			return render_bg_mode_3(); break;

		//BG Mode 4
		case 4:
			return render_bg_mode_4(); break;

		default:
			std::cout<<"LCD::invalid or unsupported BG Mode : " << std::dec << (lcd_stat.display_control & 0x7);
			return false;
	}
}

/****** Render BG Mode 0 ******/
bool LCD::render_bg_mode_0(u32 bg_control)
{
	//Set BG ID to determine which BG is being rendered.
	u8 bg_id = (bg_control - 0x4000008) >> 1;
	
	//BG offset
	u16 screen_offset = 0;

	//Determine meta x-coordinate of rendered BG pixel
	u16 meta_x = ((scanline_pixel_counter + lcd_stat.bg_offset_x[bg_id]) % lcd_stat.mode_0_width[bg_id]);

	//Determine meta Y-coordinate of rendered BG pixel
	u16 meta_y = ((current_scanline + lcd_stat.bg_offset_y[bg_id]) % lcd_stat.mode_0_height[bg_id]);
	
	//Determine the address offset for the screen
	switch(lcd_stat.bg_size[bg_id])
	{
		//Size 0 - 256x256
		case 0x0: break;

		//Size 1 - 512x256
		case 0x1: 
			screen_offset = lcd_stat.screen_offset_lut[meta_x];
			break;

		//Size 2 - 256x512
		case 0x2:
			screen_offset = lcd_stat.screen_offset_lut[meta_y];
			break;

		//Size 3 - 512x512
		case 0x3:
			screen_offset = (meta_y > 255) ? (lcd_stat.screen_offset_lut[meta_x] | 0x1000) : lcd_stat.screen_offset_lut[meta_x];
			break;
	}

	//Add screen offset to current BG map base address
	u32 map_base_addr = lcd_stat.bg_base_map_addr[bg_id] + screen_offset;

	//Determine the X-Y coordinates of the BG's tile on the tile map
	u16 current_tile_pixel_x = ((scanline_pixel_counter + lcd_stat.bg_offset_x[bg_id]) % 256);
	u16 current_tile_pixel_y = ((current_scanline + lcd_stat.bg_offset_y[bg_id]) % 256);

	//Get current map entry for rendered pixel
	u16 tile_number = lcd_stat.bg_num_lut[current_tile_pixel_x][current_tile_pixel_y];

	//Grab the map's data
	u16 map_data = mem->read_u16_fast(map_base_addr + (tile_number * 2));

	//Look at the Tile Map #(tile_number), see what Tile # it points to
	u16 map_entry = map_data & 0x3FF;

	//Grab horizontal and vertical flipping options
	u8 flip_options = (map_data >> 10) & 0x3;

	//Grab the Palette number of the tiles
	u8 palette_number = (map_data >> 12);

	//Get address of Tile #(map_entry)
	u32 tile_addr = lcd_stat.bg_base_tile_addr[bg_id] + (map_entry * (lcd_stat.bg_depth[bg_id] << 3));

	switch(flip_options)
	{
		case 0x0: break;

		//Horizontal flip
		case 0x1: 
			current_tile_pixel_x = lcd_stat.bg_flip_lut[current_tile_pixel_x];
			break;

		//Vertical flip
		case 0x2:
			current_tile_pixel_y = lcd_stat.bg_flip_lut[current_tile_pixel_y];
			break;

		//Horizontal + vertical flip
		case 0x3:
			current_tile_pixel_x = lcd_stat.bg_flip_lut[current_tile_pixel_x];
			current_tile_pixel_y = lcd_stat.bg_flip_lut[current_tile_pixel_y];
			break;
	}

	u8 current_tile_pixel = lcd_stat.bg_tile_lut[current_tile_pixel_x][current_tile_pixel_y];

	//Grab the byte corresponding to (current_tile_pixel), render it as ARGB - 4-bit version
	if(lcd_stat.bg_depth[bg_id] == 4)
	{
		tile_addr += (current_tile_pixel >> 1);
		u8 raw_color = mem->memory_map[tile_addr];

		if((current_tile_pixel % 2) == 0) { raw_color &= 0xF; }
		else { raw_color >>= 4; }

		//If the bg color is transparent, abort drawing
		if(raw_color == 0) { return false; }

		scanline_buffer[scanline_pixel_counter] = pal[((palette_number * 32) + (raw_color * 2)) >> 1][0];
	}

	//Grab the byte corresponding to (current_tile_pixel), render it as ARGB - 8-bit version
	else
	{
		tile_addr += current_tile_pixel;
		u8 raw_color = mem->memory_map[tile_addr];

		//If the bg color is transparent, abort drawing
		if(raw_color == 0) { return false; }

		scanline_buffer[scanline_pixel_counter] = pal[raw_color][0];
	}

	return true;
}

/****** Render BG Mode 1 ******/
bool LCD::render_bg_mode_1(u32 bg_control)
{
	//Set BG ID to determine which BG is being rendered.
	u8 bg_id = (bg_control - 0x4000008) >> 1;

	//Get BG size in tiles, pixels
	//0 - 128x128, 1 - 256x256, 2 - 512x512, 3 - 1024x1024
	u16 bg_tile_size = (16 << (lcd_stat.bg_control[2] >> 14));
	u16 bg_pixel_size = bg_tile_size << 3;

	//Determine source pixel X-Y coordinates
	u16 src_x = scanline_pixel_counter; 
	u16 src_y = current_scanline;

	u32 current_pos = (src_y * 240) + src_x;
	current_pos = bg_params[0].bg_lut[current_pos];

	src_y = current_pos / bg_pixel_size;
	src_x = current_pos % bg_pixel_size;

	//Get current map entry for rendered pixel
	u16 tile_number = ((src_y / 8) * bg_tile_size) + (src_x / 8);

	//Look at the Tile Map #(tile_number), see what Tile # it points to
	u16 map_entry = mem->read_u16_fast(lcd_stat.bg_base_map_addr[bg_id] + tile_number) & 0xFF;

	//Get address of Tile #(map_entry)
	u32 tile_addr = lcd_stat.bg_base_tile_addr[bg_id] + (map_entry * 64);

	u8 current_tile_pixel = ((src_y % 8) * 8) + (src_x % 8);

	//Grab the byte corresponding to (current_tile_pixel), render it as ARGB - 8-bit version
	tile_addr += current_tile_pixel;
	u8 raw_color = mem->memory_map[tile_addr];

	//If the bg color is transparent, abort drawing
	if(raw_color == 0) { return false; }

	scanline_buffer[scanline_pixel_counter] = pal[raw_color][0];

	return true;
}

/****** Render BG Mode 3 ******/
bool LCD::render_bg_mode_3()
{
	//Determine which byte in VRAM to read for color data
	u16 color_bytes = mem->read_u16_fast(0x6000000 + (current_scanline * 480) + (scanline_pixel_counter * 2));

	//ARGB conversion
	u8 red = ((color_bytes & 0x1F) * 8);
	color_bytes >>= 5;

	u8 green = ((color_bytes & 0x1F) * 8);
	color_bytes >>= 5;

	u8 blue = ((color_bytes & 0x1F) * 8);

	scanline_buffer[scanline_pixel_counter] = 0xFF000000 | (red << 16) | (green << 8) | (blue);

	return true;
}

/****** Render BG Mode 4 ******/
bool LCD::render_bg_mode_4()
{
	//Determine which byte in VRAM to read for color data
	u32 bitmap_entry = (lcd_stat.frame_base + (current_scanline * 240) + scanline_pixel_counter);

	u8 raw_color = mem->memory_map[bitmap_entry];
	if(raw_color == 0) { return false; }

	scanline_buffer[scanline_pixel_counter] = pal[raw_color][0];

	return true;
}

/****** Render pixels for a given scanline (per-pixel) ******/
void LCD::render_scanline()
{
	bool obj_render = false;
	last_obj_priority = 0xFF;

	//Use BG Palette #0, Color #0 as the backdrop
	scanline_buffer[scanline_pixel_counter] = pal[0][0];

	//Render sprites
	obj_render = render_sprite_pixel();

	//Render BGs based on priority (3 is the 'lowest', 0 is the 'highest')
	for(int x = 0; x < 4; x++)
	{
		if(lcd_stat.bg_priority[0] == x) 
		{
			if((obj_render) && (last_obj_priority <= x)) { return; }
			if(render_bg_pixel(BG0CNT)) { return; } 
		}

		if(lcd_stat.bg_priority[1] == x) 
		{
			if((obj_render) && (last_obj_priority <= x)) { return; }
			if(render_bg_pixel(BG1CNT)) { return; } 
		}

		if(lcd_stat.bg_priority[2] == x) 
		{
			if((obj_render) && (last_obj_priority <= x)) { return; }
			if(render_bg_pixel(BG2CNT)) { return; } 
		}

		if(lcd_stat.bg_priority[3] == x) 
		{
			if((obj_render) && (last_obj_priority <= x)) { return; }
			if(render_bg_pixel(BG3CNT)) { return; } 
		}
	}
}

/****** Run LCD for one cycle ******/
void LCD::step()
{
	lcd_clock++;

	//Mode 0 - Scanline rendering
	if(((lcd_clock % 1232) <= 960) && (lcd_clock < 197120)) 
	{
		//Toggle HBlank flag OFF
		mem->memory_map[DISPSTAT] &= ~0x2;

		//Change mode
		if(lcd_mode != 0) 
		{ 
			lcd_mode = 0; 
			update_obj_render_list();

			//Update BG scaling + rotation parameters
			if(lcd_stat.bg_params_update) { update_bg_params(); }
		}

		//Render scanline data (per-pixel every 4 cycles)
		if((lcd_clock % 4) == 0) 
		{
			//Update OAM
			if(lcd_stat.oam_update) { update_oam(); }

			//Update palettes
			if((lcd_stat.bg_pal_update) || (lcd_stat.obj_pal_update)) { update_palettes(); }

			render_scanline();
			scanline_pixel_counter++;
		}
	}

	//Mode 1 - H-Blank
	else if(((lcd_clock % 1232) > 960) && (lcd_clock < 197120))
	{
		//Toggle HBlank flag ON
		mem->memory_map[DISPSTAT] |= 0x2;

		//Change mode
		if(lcd_mode != 1) 
		{ 
			lcd_mode = 1;
			scanline_pixel_counter = 0;

			//Raise HBlank interrupt
			if(mem->memory_map[DISPSTAT] & 0x10) { mem->memory_map[REG_IF] |= 0x2; }

			//Push scanline data to final buffer - Only if Forced Blank is disabled
			if((mem->memory_map[DISPCNT] & 0x80) == 0)
			{
				for(int x = 0, y = (240 * current_scanline); x < 240; x++, y++)
				{
					screen_buffer[y] = scanline_buffer[x];
				}
			}

			//Draw all-white during Forced Blank
			else
			{
				for(int x = 0, y = (240 * current_scanline); x < 240; x++, y++)
				{
					screen_buffer[y] = 0xFFFFFFFF;
				}
			}

			//Increment scanline count
			current_scanline++;
			mem->write_u16_fast(VCOUNT, current_scanline);
			scanline_compare();
	
			//Start HBlank DMA
			mem->start_blank_dma();
		}
	}

	//Mode 2 - VBlank
	else
	{
		//Toggle VBlank flag ON
		mem->memory_map[DISPSTAT] |= 0x1;

		//Toggle HBlank flag OFF
		mem->memory_map[DISPSTAT] &= ~0x2;

		//Change mode
		if(lcd_mode != 2) 
		{ 
			lcd_mode = 2;

			//Raise VBlank interrupt
			if(mem->memory_map[DISPSTAT] & 0x8) { mem->memory_map[REG_IF] |= 0x1; }

			//Render final buffer - Only if Forced Blank is disabled
			if((mem->memory_map[DISPCNT] & 0x80) == 0)
			{
				//Lock source surface
				if(SDL_MUSTLOCK(final_screen)){ SDL_LockSurface(final_screen); }
				u32* out_pixel_data = (u32*)final_screen->pixels;

				for(int a = 0; a < 0x9600; a++) { out_pixel_data[a] = screen_buffer[a]; }

				//Unlock source surface
				if(SDL_MUSTLOCK(final_screen)){ SDL_UnlockSurface(final_screen); }
		
				//Display final screen buffer - OpenGL
				if(config::use_opengl) { opengl_blit(); }
				
				//Display final screen buffer - SDL
				else 
				{
					if(SDL_Flip(final_screen) == -1) { std::cout<<"LCD::Error - Could not blit\n"; }
				}
			}

			//Limit framerate
			if(!config::turbo)
			{
				frame_current_time = SDL_GetTicks();
				if((frame_current_time - frame_start_time) < 16) { SDL_Delay(16 - (frame_current_time - frame_start_time));}
				frame_start_time = SDL_GetTicks();
			}

			//Update FPS counter + title
			fps_count++;
			if((SDL_GetTicks() - fps_time) >= 1000) 
			{ 
				fps_time = SDL_GetTicks(); 
				config::title.str("");
				config::title << "GBE+ " << fps_count << "FPS";
				SDL_WM_SetCaption(config::title.str().c_str(), NULL);
				fps_count = 0; 
			}
		}

		//Setup HBlank stuff (no HBlank IRQs in VBlank!!)
		if((lcd_clock % 1232) == 960) 
		{
			//Toggle HBlank flag ON
			mem->memory_map[DISPSTAT] |= 0x2;

			current_scanline++;
			mem->write_u16_fast(VCOUNT, current_scanline);
			scanline_compare();
		}

		//Reset LCD clock
		if(lcd_clock == 280896) 
		{
			//Toggle VBlank flag OFF
			mem->memory_map[DISPSTAT] &= ~0x1;

			//Toggle HBlank flag OFF
			mem->memory_map[DISPSTAT] &= ~0x2;

			lcd_clock = 0; 
			current_scanline = 0; 
			scanline_compare();
			scanline_pixel_counter = 0; 
			mem->write_u16_fast(VCOUNT, 0);
		}
	}
}

/****** Compare VCOUNT to LYC ******/
void LCD::scanline_compare()
{
	u16 disp_stat = mem->read_u16_fast(DISPSTAT);
	u8 lyc = disp_stat >> 8;
	
	//Raise VCOUNT interrupt
	if(current_scanline == lyc)
	{
		if(mem->memory_map[DISPSTAT] & 0x20) 
		{ 
			mem->memory_map[REG_IF] |= 0x4; 
		}

		//Toggle VCOUNT flag ON
		disp_stat |= 0x4;
		mem->write_u16_fast(DISPSTAT, disp_stat);
	}

	else
	{
		//Toggle VCOUNT flag OFF
		disp_stat &= ~0x4;
		mem->write_u16_fast(DISPSTAT, disp_stat);
	}
}
		