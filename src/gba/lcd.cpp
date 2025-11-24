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
#include "common/util.h"

/****** LCD Constructor ******/
AGB_LCD::AGB_LCD()
{
	window = NULL;
	reset();
}

/****** LCD Destructor ******/
AGB_LCD::~AGB_LCD()
{
	screen_buffer.clear();
	scanline_buffer.clear();
	SDL_DestroyWindow(window);
	std::cout<<"LCD::Shutdown\n";
}

/****** Reset LCD ******/
void AGB_LCD::reset()
{
	final_screen = NULL;
	original_screen = NULL;
	mem = NULL;

	if((window != NULL) && (config::sdl_render)) { SDL_DestroyWindow(window); }
	window = NULL;

	scanline_buffer.clear();
	screen_buffer.clear();

	lcd_clock = 0;
	lcd_mode = 0;

	frame_start_time = 0;
	frame_current_time = 0;
	fps_count = 0;
	fps_time = 0;

	for(u32 x = 0; x < 60; x++)
	{
		u16 max = (config::max_fps) ? config::max_fps : 60;
		double frame_1 = ((1000.0 / max) * x);
		double frame_2 = ((1000.0 / max) * (x + 1));
		frame_delay[x] = (std::round(frame_2) - std::round(frame_1));
	}

	current_scanline = 0;
	scanline_pixel_counter = 0;

	screen_buffer.resize(0x9600, 0);
	scanline_buffer.resize(0x100, 0);

	//Initialize various LCD status variables
	lcd_stat.oam_update = true;
	for(int x = 0; x < 128; x++) { lcd_stat.oam_update_list[x] = true; }

	lcd_stat.bg_pal_update = true;
	lcd_stat.obj_pal_update = true;

	for(int x = 0; x < 256; x++)
	{
		lcd_stat.bg_pal_update_list[x] = true;
		lcd_stat.obj_pal_update_list[x] = true;
	}

	lcd_stat.frame_base = 0x6000000;
	lcd_stat.bg_mode = 0;
	lcd_stat.hblank_interval_free = false;

	lcd_stat.window_x1[0] = lcd_stat.window_x1[1] = 0xFF;
	lcd_stat.window_y1[0] = lcd_stat.window_y1[1] = 0xFF;
	lcd_stat.window_enable[0] = lcd_stat.window_enable[1] = false;

	lcd_stat.in_window = false;
	lcd_stat.obj_win_enable = false;
	lcd_stat.current_sfx_type = NORMAL;

	//BG2/3 affine parameters
	for(int x = 0; x < 2; x++)
	{
		lcd_stat.bg_affine[x].overflow = false;
		lcd_stat.bg_affine[x].dmx = lcd_stat.bg_affine[x].dy = 0.0;
		lcd_stat.bg_affine[x].dx = lcd_stat.bg_affine[x].dmy = 1.0;
		lcd_stat.bg_affine[x].x_ref = lcd_stat.bg_affine[x].y_ref = 0.0;
		lcd_stat.bg_affine[x].x_pos = lcd_stat.bg_affine[x].y_pos = 0.0;
	}

	//OBJ affine parameters
	for(int x = 0; x < 128; x++)
	{
		lcd_stat.obj_affine[x] = 0.0;
	}

	//BG Flip LUT generation
	for(int x = 0, y = 255; x < 255; x++, y--)
	{
		lcd_stat.bg_flip_lut[x] = (y % 8);
	}

	//BG Tile # LUT generation
	for(int y = 0; y < 256; y++)
	{
		for(int x = 0; x < 256; x++)
		{
			lcd_stat.bg_tile_lut[x][y] = ((y % 8) * 8) + (x % 8);
		}
	}

	//BG Tile Map# LUT generation
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
		lcd_stat.bg_mosiac[x] = false;
		lcd_stat.bg_base_tile_addr[x] = 0x6000000;
		lcd_stat.bg_base_map_addr[x] = 0x6000000;
		lcd_stat.mode_0_width[x] = 256;
		lcd_stat.mode_0_height[x] = 256;
	}

	//Initialize system screen dimensions
	config::sys_width = 240;
	config::sys_height = 160;

	//Initialize GBA subscreen as normal
	config::resize_mode = 0;
	config::request_resize = false;

	max_fullscreen_ratio = 2;
	power_antenna_osd = false;

	try_window_rebuild = false;
}

/****** Initialize LCD with SDL ******/
bool AGB_LCD::init()
{
	//Initialize with SDL rendering software or hardware
	if(config::sdl_render)
	{
		//Initialize all of SDL
		if(SDL_Init(SDL_INIT_VIDEO) == -1)
		{
			std::cout<<"LCD::Error - Could not initialize SDL video\n";
			return false;
		}

		//Setup OpenGL rendering
		if(config::use_opengl)
		{
			if(!opengl_init())
			{
				std::cout<<"LCD::Error - Could not initialize OpenGL\n";
				return false;
			}
		}

		//Set up software rendering
		else
		{
			u32 next_w = config::sys_width;
			u32 next_h = config::sys_height;

			if(config::flags & SDL_WINDOW_FULLSCREEN)
			{
				SDL_DisplayMode current_mode;
				SDL_GetCurrentDisplayMode(0, &current_mode);

				next_w = current_mode.w;
				next_h = current_mode.h;

				double max_width, max_height, ratio = 0.0;

				max_width = (double)next_w / config::sys_width;
				max_height = (double)next_h / config::sys_height;

				if(max_width <= max_height) { ratio = max_width; }
				else { ratio = max_height; }

				max_fullscreen_ratio = ratio;
			}

			window = SDL_CreateWindow("GBE+", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, next_w, next_h, config::flags);
			SDL_GetWindowSize(window, &config::win_width, &config::win_height);
			final_screen = SDL_GetWindowSurface(window);
			original_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, config::sys_width, config::sys_height, 32, 0, 0, 0, 0);
			config::scaling_factor = 1;
		}

		if(final_screen == NULL) { return false; }

		SDL_SetWindowIcon(window, util::load_icon(config::data_path + "icons/gbe_plus.bmp"));
	}

	//Initialize with only a buffer for OpenGL (for external rendering)
	else if((!config::sdl_render) && (config::use_opengl))
	{
		final_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, config::sys_width, config::sys_height, 32, 0, 0, 0, 0);
	}

	std::cout<<"LCD::Initialized\n";

	return true;
}

/****** Updates OAM entries when values in memory change ******/
void AGB_LCD::update_oam()
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
			obj[x].affine_enable = (attribute & 0x100) ? 1 : 0;
			obj[x].type = (attribute & 0x200) ? 1 : 0;
			obj[x].mode = (attribute >> 10) & 0x3;
			obj[x].mosiac = (attribute >> 12) & 0x1;
			obj[x].bit_depth = (attribute & 0x2000) ? 8 : 4;
			obj[x].shape = (attribute >> 14);

			if((obj[x].affine_enable == 0) && (obj[x].type == 1)) { obj[x].visible = false; }
			else if(obj[x].mode == 3) { obj[x].visible = false; }
			else { obj[x].visible = true; }

			//Read and parse Attribute 1
			attribute = mem->read_u16_fast(oam_ptr);
			oam_ptr += 2;

			obj[x].x = (attribute & 0x1FF);
			obj[x].h_flip = (attribute & 0x1000) ? true : false;
			obj[x].v_flip = (attribute & 0x2000) ? true : false;
			obj[x].size = (attribute >> 14);

			if(obj[x].affine_enable) { obj[x].affine_group = (attribute >> 9) & 0x1F; }

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

			//Precalulate OBJ boundaries
			obj[x].left = obj[x].x;
			obj[x].right = (obj[x].x + obj[x].width - 1) & 0x1FF;

			obj[x].top = obj[x].y;
			obj[x].bottom = (obj[x].y + obj[x].height - 1) & 0xFF;

			//Precalculate OBJ wrapping
			if(obj[x].left > obj[x].right) 
			{
				obj[x].x_wrap = true;
				obj[x].x_wrap_val = (obj[x].width - obj[x].right - 1);
			}

			else { obj[x].x_wrap = false; }

			if(obj[x].top > obj[x].bottom)
			{
				obj[x].y_wrap = true;
				obj[x].y_wrap_val = (obj[x].height - obj[x].bottom - 1);
			}

			else { obj[x].y_wrap = false; }

			//Precalculate OBJ base address
			obj[x].addr = 0x6010000 + (obj[x].tile_number << 5);

			//Read and parse OAM affine attribute
			attribute = mem->read_u16_fast(oam_ptr - 2);
			
			//Only update if this attribute is non-zero
			if(attribute)
			{	
				if(attribute & 0x8000) 
				{ 
					u16 temp = ((attribute >> 8) - 1);
					temp = (~temp & 0xFF);
					lcd_stat.obj_affine[x] = -1.0 * temp;
				}

				else { lcd_stat.obj_affine[x] = (attribute >> 8); }

				if((attribute & 0xFF) != 0) { lcd_stat.obj_affine[x] += (attribute & 0xFF) / 256.0; }
			}

			else { lcd_stat.obj_affine[x] = 0.0; }
		}

		else { oam_ptr += 8; }
	}

	//Update OBJ for affine transformations
	update_obj_affine_transformation();

	//Update render list for the current scanline
	update_obj_render_list();
}

/****** Updates the size and position of OBJs from affine transformation ******/
void AGB_LCD::update_obj_affine_transformation()
{
	//Cycle through all OAM entries
	for(int x = 0; x < 128; x++)
	{
		//Determine if affine transformations occur on this OBJ
		if(obj[x].affine_enable)
		{
			//Process regular-sized OBJs
			if(!obj[x].type)
			{
				//Find half width and half height
				obj[x].cw = obj[x].width >> 1;
				obj[x].ch = obj[x].height >> 1;

				//Find OBJ center
				obj[x].cx = obj[x].x + obj[x].cw;
				obj[x].cy = obj[x].y + obj[x].ch;

				if(obj[x].cx > 0xFF) { obj[x].cx -= 0x1FF; }
				if(obj[x].cy > 0xFF) { obj[x].cy -= 0xFF; }
			}

			//Process double-sized OBJs
			else
			{
				//Find half width and half height
				obj[x].cw = obj[x].width >> 1;
				obj[x].ch = obj[x].height >> 1;

				//Find OBJ center
				obj[x].cx = (obj[x].x + obj[x].width);
				obj[x].cy = (obj[x].y + obj[x].height);

				if(obj[x].cx > 0xFF) { obj[x].cx -= 0x1FF; }
				if(obj[x].cy > 0xFF) { obj[x].cy -= 0xFF; }

				obj[x].left = obj[x].x;
				obj[x].top = obj[x].y;

				obj[x].right = (obj[x].left + (obj[x].width << 1) - 1) & 0x1FF;
				obj[x].bottom = (obj[x].top + (obj[x].height << 1) - 1) & 0xFF;

				//Precalculate OBJ wrapping
				if(obj[x].left > obj[x].right)
				{
					obj[x].x_wrap = true;
					obj[x].x_wrap_val = ((obj[x].width << 1) - obj[x].right - 1);
				}

				else { obj[x].x_wrap = false; }

				if(obj[x].top > obj[x].bottom)
				{
					obj[x].y_wrap = true;
					obj[x].y_wrap_val = ((obj[x].height << 1) - obj[x].bottom - 1);
				}

				else { obj[x].y_wrap = false; }
			}
		}
	}
}

/****** Updates a list of OBJs to render on the current scanline ******/
void AGB_LCD::update_obj_render_list()
{
	obj_render_length = 0;

	//Sort them based on BG priorities
	for(int bg = 0; bg < 4; bg++)
	{
		//Cycle through all of the sprites
		for(int x = 0; x < 128; x++)
		{	
			//Check to see if sprite is rendered on the current scanline
			if(!obj[x].visible) { continue; }
			else if((!obj[x].y_wrap) && ((current_scanline < obj[x].top) || (current_scanline > obj[x].bottom))) { continue; }
			else if((obj[x].y_wrap) && ((current_scanline > obj[x].bottom) && (current_scanline < obj[x].top))) { continue; }

			else if(obj[x].bg_priority == bg)
			{
				obj_render_list[obj_render_length++] = x;
			}
		}
	}
}

/****** Updates palette entries when values in memory change ******/
void AGB_LCD::update_palettes()
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
				raw_pal[x][0] = color_bytes;

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
				raw_pal[x][1] = color_bytes;

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

/****** Determines if a sprite pixel should be rendered, and if so draws it to the current scanline pixel ******/
bool AGB_LCD::render_sprite_pixel()
{
	//If sprites are disabled, quit now
	if((lcd_stat.display_control & 0x1000) == 0) { return false; }

	//If no sprites are rendered on this line, quit now
	if(obj_render_length == 0) { return false; }

	u8 sprite_id = 0;
	u32 sprite_tile_addr = 0;
	u32 meta_sprite_tile = 0;
	u8 raw_color = 0;

	u16 sprite_tile_pixel_x = 0;
	u16 sprite_tile_pixel_y = 0;

	bool render_obj;
	bool final_render = false;

	//Cycle through all sprites that are rendering on this pixel, draw them according to their priority
	for(int x = 0; x < obj_render_length; x++)
	{
		sprite_id = obj_render_list[x];
		render_obj = true;

		if((final_render) && (obj[sprite_id].mode != 2)) { continue; }

		//Check to see if current_scanline_pixel is within sprite
		if((!obj[sprite_id].x_wrap) && ((scanline_pixel_counter < obj[sprite_id].left) || (scanline_pixel_counter > obj[sprite_id].right))) { continue; }
		else if((obj[sprite_id].x_wrap) && ((scanline_pixel_counter > obj[sprite_id].right) && (scanline_pixel_counter < obj[sprite_id].left))) { continue; }

		//For bitmap BG Modes 3-5, skip rendering tile numbers lower than 512
		else if((lcd_stat.bg_mode >= 0x3) && (obj[sprite_id].tile_number < 512)) { continue; }

		//Normal sprite rendering
		if(!obj[sprite_id].affine_enable)
		{
			//Determine the internal X-Y coordinates of the sprite's pixel
			sprite_tile_pixel_x = obj[sprite_id].x_wrap ? (scanline_pixel_counter + obj[sprite_id].x_wrap_val) : (scanline_pixel_counter - obj[sprite_id].x);
			sprite_tile_pixel_y = obj[sprite_id].y_wrap ? (current_scanline + obj[sprite_id].y_wrap_val) : (current_scanline - obj[sprite_id].y);

			//Horizontal flip the internal X coordinate
			if(obj[sprite_id].h_flip)
			{
				s16 h_flip = sprite_tile_pixel_x;
				h_flip -= (obj[sprite_id].width - 1);

				if(h_flip < 0) { h_flip *= -1; }

				sprite_tile_pixel_x = h_flip;
			}

			//Vertical flip the internal Y coordinate
			if(obj[sprite_id].v_flip)
			{
				s16 v_flip = sprite_tile_pixel_y;
				v_flip -= (obj[sprite_id].height - 1);

				if(v_flip < 0) { v_flip *= -1; }

				sprite_tile_pixel_y = v_flip;
			}
		}

		//Affine transformation sprite rendering
		else
		{
			u8 index = (obj[sprite_id].affine_group << 2);
			s16 current_x, current_y;

			//Determine current X position relative to the OBJ center X, account for screen wrapping
			if((obj[sprite_id].x_wrap) && (scanline_pixel_counter < obj[sprite_id].right)) { current_x = scanline_pixel_counter - (obj[sprite_id].cx - obj[sprite_id].x_wrap); }
			else { current_x = scanline_pixel_counter - obj[sprite_id].cx; }

			//Determine current Y position relative to the OBJ center Y, account for screen wrapping
			if((obj[sprite_id].y_wrap) && (current_scanline < obj[sprite_id].bottom)) { current_y = current_scanline - (obj[sprite_id].cy - obj[sprite_id].y_wrap); }
			else { current_y = current_scanline - obj[sprite_id].cy; }

			s16 new_x = obj[sprite_id].cw + (lcd_stat.obj_affine[index] * current_x) + (lcd_stat.obj_affine[index+1] * current_y);
			s16 new_y = obj[sprite_id].ch + (lcd_stat.obj_affine[index+2] * current_x) + (lcd_stat.obj_affine[index+3] * current_y);

			//If out of bounds for the transformed sprite, abort rendering
			if((new_x < 0) || (new_y < 0) || (new_x >= obj[sprite_id].width) || (new_y >= obj[sprite_id].height)) { render_obj = false; }
		
			sprite_tile_pixel_x = new_x;
			sprite_tile_pixel_y = new_y;
		}

		//This check is mainly for affine OBJs
		if(render_obj)
		{
			//Handle the mosiac function
			if(obj[sprite_id].mosiac && lcd_stat.obj_mos_hsize) { sprite_tile_pixel_x = ((sprite_tile_pixel_x / lcd_stat.obj_mos_hsize) * lcd_stat.obj_mos_hsize); }
			if(obj[sprite_id].mosiac && lcd_stat.obj_mos_vsize) { sprite_tile_pixel_y = ((sprite_tile_pixel_y / lcd_stat.obj_mos_vsize) * lcd_stat.obj_mos_vsize); }

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
				meta_sprite_tile = (obj[sprite_id].bit_depth == 8) ? ((meta_y * 16) + meta_x) : ((meta_y * 32) + meta_x);
			}

			sprite_tile_addr = obj[sprite_id].addr + (meta_sprite_tile * (obj[sprite_id].bit_depth << 3));

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
					//If this sprite is in OBJ Window mode, do not render it, but set a flag indicating the LCD passed over its pixel
					if(obj[sprite_id].mode == 2) { obj_win_pixel = true; }

					else 
					{
						scanline_buffer[scanline_pixel_counter] = pal[((obj[sprite_id].palette_number * 32) + (raw_color * 2)) >> 1][1];
						last_raw_color = raw_pal[((obj[sprite_id].palette_number * 32) + (raw_color * 2)) >> 1][1];
						last_obj_priority = obj[sprite_id].bg_priority;
						last_obj_mode = obj[sprite_id].mode;
						final_render = true;
					}
				}
			}

			//Grab the byte corresponding to (sprite_tile_pixel), render it as ARGB - 8-bit version
			else
			{
				sprite_tile_addr += sprite_tile_pixel;
				raw_color = mem->memory_map[sprite_tile_addr];

				if(raw_color != 0) 
				{
					//If this sprite is in OBJ Window mode, do not render it, but set a flag indicating the LCD passed over its pixel
					if(obj[sprite_id].mode == 2) { obj_win_pixel = true; }

					else
					{
						scanline_buffer[scanline_pixel_counter] = pal[raw_color][1];
						last_raw_color = raw_pal[raw_color][1];
						last_obj_priority = obj[sprite_id].bg_priority;
						last_obj_mode = obj[sprite_id].mode;
						final_render = true;
					}
				}
			}
		}
	}

	//Return false if nothing was drawn
	return final_render;
}

/****** Determines if a background pixel should be rendered, and if so draws it to the current scanline pixel ******/
bool AGB_LCD::render_bg_pixel(u32 bg_control)
{
	if(!lcd_stat.bg_enable[(bg_control - 0x4000008) >> 1]) { return false; }

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

		//BG Mode 2
		case 0x2:
			//Render BG2 and BG3 as Scaled+Rotation
			if((bg_control == BG2CNT) || (bg_control == BG3CNT)) { return render_bg_mode_1(bg_control); }
			else { return false; }

			break;

		//BG Mode 3
		case 3:
			return render_bg_mode_3(); break;

		//BG Mode 4
		case 4:
			return render_bg_mode_4(); break;

		//BG Mode 5
		case 5:
			return render_bg_mode_5(); break;

		default:
			//std::cout<<"LCD::invalid or unsupported BG Mode : " << std::dec << (lcd_stat.display_control & 0x7);
			return false;
	}
}

/****** Render BG Mode 0 ******/
bool AGB_LCD::render_bg_mode_0(u32 bg_control)
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

	//Handle mosiac tiles
	if(lcd_stat.bg_mosiac[bg_id] && lcd_stat.bg_mos_hsize) { current_tile_pixel_x = ((current_tile_pixel_x / lcd_stat.bg_mos_hsize) * lcd_stat.bg_mos_hsize); }
	if(lcd_stat.bg_mosiac[bg_id] && lcd_stat.bg_mos_vsize) { current_tile_pixel_y = ((current_tile_pixel_y / lcd_stat.bg_mos_vsize) * lcd_stat.bg_mos_vsize); }

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
		last_raw_color = raw_pal[((palette_number * 32) + (raw_color * 2)) >> 1][0];
	}

	//Grab the byte corresponding to (current_tile_pixel), render it as ARGB - 8-bit version
	else
	{
		tile_addr += current_tile_pixel;
		u8 raw_color = mem->memory_map[tile_addr];

		//If the bg color is transparent, abort drawing
		if(raw_color == 0) { return false; }

		scanline_buffer[scanline_pixel_counter] = pal[raw_color][0];
		last_raw_color = raw_pal[raw_color][0];
	}

	return true;
}

/****** Render BG Mode 1 ******/
bool AGB_LCD::render_bg_mode_1(u32 bg_control)
{
	//Set BG ID to determine which BG is being rendered.
	u8 bg_id = (bg_control - 0x4000008) >> 1;
	u8 scale_rot_id = (bg_id == 2) ? 0 : 1;

	//If rendering pixels along a given line, add DX and DY
	lcd_stat.bg_affine[scale_rot_id].x_pos = lcd_stat.bg_affine[scale_rot_id].x_ref + (lcd_stat.bg_affine[scale_rot_id].dx * scanline_pixel_counter);
	lcd_stat.bg_affine[scale_rot_id].y_pos = lcd_stat.bg_affine[scale_rot_id].y_ref + (lcd_stat.bg_affine[scale_rot_id].dy * scanline_pixel_counter);

	//Get BG size in tiles, pixels
	//0 - 128x128, 1 - 256x256, 2 - 512x512, 3 - 1024x1024
	u16 bg_tile_size = (16 << (lcd_stat.bg_control[bg_id] >> 14));
	u16 bg_pixel_size = bg_tile_size << 3;

	//Calculate new X-Y coordinates from scaling+rotation
	double new_x = lcd_stat.bg_affine[scale_rot_id].x_pos;
	double new_y = lcd_stat.bg_affine[scale_rot_id].y_pos;

	//Clip BG if coordinates overflow and overflow flag is not set
	if(!lcd_stat.bg_affine[scale_rot_id].overflow)
	{
		if((new_x >= bg_pixel_size) || (new_x < 0)) { return false; }
		if((new_y >= bg_pixel_size) || (new_y < 0)) { return false; }
	}

	//Wrap BG if coordinates overflow and overflow flag is set
	else 
	{
		while(new_x >= bg_pixel_size) { new_x -= bg_pixel_size; }
		while(new_y >= bg_pixel_size) { new_y -= bg_pixel_size; }
		while(new_x < 0) { new_x += bg_pixel_size; }
		while(new_y < 0) { new_y += bg_pixel_size; } 
	}

	//Determine source pixel X-Y coordinates
	u16 src_x = new_x; 
	u16 src_y = new_y;

	//Handle mosiac tiles
	if(lcd_stat.bg_mosiac[bg_id] && lcd_stat.bg_mos_hsize) { src_x = ((src_x / lcd_stat.bg_mos_hsize) * lcd_stat.bg_mos_hsize); }
	if(lcd_stat.bg_mosiac[bg_id] && lcd_stat.bg_mos_vsize) { src_y = ((src_y / lcd_stat.bg_mos_vsize) * lcd_stat.bg_mos_vsize); }

	//Get current map entry for rendered pixel
	u16 tile_number = ((src_y / 8) * bg_tile_size) + (src_x / 8);

	//Look at the Tile Map #(tile_number), see what Tile # it points to
	u8 map_entry = mem->memory_map[lcd_stat.bg_base_map_addr[bg_id] + tile_number];

	//Get address of Tile #(map_entry)
	u32 tile_addr = lcd_stat.bg_base_tile_addr[bg_id] + (map_entry * 64);

	u8 current_tile_pixel = ((src_y % 8) * 8) + (src_x % 8);

	//Grab the byte corresponding to (current_tile_pixel), render it as ARGB - 8-bit version
	tile_addr += current_tile_pixel;
	u8 raw_color = mem->memory_map[tile_addr];

	//If the bg color is transparent, abort drawing
	if(raw_color == 0) { return false; }

	scanline_buffer[scanline_pixel_counter] = pal[raw_color][0];
	last_raw_color = raw_pal[raw_color][0];

	return true;
}

/****** Render BG Mode 3 ******/
bool AGB_LCD::render_bg_mode_3()
{
	//If rendering pixels along a given line, add DX and DY
	lcd_stat.bg_affine[0].x_pos = lcd_stat.bg_affine[0].x_ref + (lcd_stat.bg_affine[0].dx * scanline_pixel_counter);
	lcd_stat.bg_affine[0].y_pos = lcd_stat.bg_affine[0].y_ref + (lcd_stat.bg_affine[0].dy * scanline_pixel_counter);

	//Clip affine coordinates if out-of-bounds
	if((lcd_stat.bg_affine[0].x_pos >= 240) || (lcd_stat.bg_affine[0].x_pos < 0)) { return false; }
	if((lcd_stat.bg_affine[0].y_pos >= 160) || (lcd_stat.bg_affine[0].y_pos < 0)) { return false; }

	u16 src_x = lcd_stat.bg_affine[0].x_pos; 
	u16 src_y = lcd_stat.bg_affine[0].y_pos;

	//Determine which byte in VRAM to read for color data
	u16 color_bytes = mem->read_u16_fast(0x6000000 + (src_y * 480) + (src_x * 2));
	last_raw_color = color_bytes;

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
bool AGB_LCD::render_bg_mode_4()
{
	//If rendering pixels along a given line, add DX and DY
	lcd_stat.bg_affine[0].x_pos = lcd_stat.bg_affine[0].x_ref + (lcd_stat.bg_affine[0].dx * scanline_pixel_counter);
	lcd_stat.bg_affine[0].y_pos = lcd_stat.bg_affine[0].y_ref + (lcd_stat.bg_affine[0].dy * scanline_pixel_counter);

	//Clip affine coordinates if out-of-bounds
	if((lcd_stat.bg_affine[0].x_pos >= 240) || (lcd_stat.bg_affine[0].x_pos < 0)) { return false; }
	if((lcd_stat.bg_affine[0].y_pos >= 160) || (lcd_stat.bg_affine[0].y_pos < 0)) { return false; }

	u16 src_x = lcd_stat.bg_affine[0].x_pos; 
	u16 src_y = lcd_stat.bg_affine[0].y_pos;

	//Determine which byte in VRAM to read for color data
	u32 bitmap_entry = (lcd_stat.frame_base + (src_y * 240) + src_x);

	u8 raw_color = mem->memory_map[bitmap_entry];
	if(raw_color == 0) { return false; }

	scanline_buffer[scanline_pixel_counter] = pal[raw_color][0];
	last_raw_color = raw_pal[raw_color][0];

	return true;
}

/****** Render BG Mode 5 ******/
bool AGB_LCD::render_bg_mode_5()
{
	//If rendering pixels along a given line, add DX and DY
	lcd_stat.bg_affine[0].x_pos = lcd_stat.bg_affine[0].x_ref + (lcd_stat.bg_affine[0].dx * scanline_pixel_counter);
	lcd_stat.bg_affine[0].y_pos = lcd_stat.bg_affine[0].y_ref + (lcd_stat.bg_affine[0].dy * scanline_pixel_counter);

	//Clip affine coordinates if out-of-bounds
	if((lcd_stat.bg_affine[0].x_pos >= 160) || (lcd_stat.bg_affine[0].x_pos < 0)) { return false; }
	if((lcd_stat.bg_affine[0].y_pos >= 128) || (lcd_stat.bg_affine[0].y_pos < 0)) { return false; }

	u16 src_x = lcd_stat.bg_affine[0].x_pos; 
	u16 src_y = lcd_stat.bg_affine[0].y_pos;

	//Determine which byte in VRAM to read for color data
	u16 color_bytes = mem->read_u16_fast(lcd_stat.frame_base + (src_y * 320) + (src_x * 2));
	last_raw_color = color_bytes;

	//ARGB conversion
	u8 red = ((color_bytes & 0x1F) * 8);
	color_bytes >>= 5;

	u8 green = ((color_bytes & 0x1F) * 8);
	color_bytes >>= 5;

	u8 blue = ((color_bytes & 0x1F) * 8);

	scanline_buffer[scanline_pixel_counter] = 0xFF000000 | (red << 16) | (green << 8) | (blue);

	return true;
}

/****** Render pixels for a given scanline (per-pixel) ******/
void AGB_LCD::render_scanline()
{
	bool obj_render = false;
	bool winout = false;
	lcd_stat.in_window = false;
	lcd_stat.current_window = 0;
	last_obj_priority = 0xFF;
	last_bg_priority = 0x5;
	last_obj_mode = 0;
	last_raw_color = raw_pal[0][0];
	obj_win_pixel = false;
	u8 bg_render_list[4];
	u8 bg_id;

	//Render sprites
	obj_render = render_sprite_pixel();

	bool check_x = false;
	bool check_y = false;

	//Determine window status of this pixel
	if(lcd_stat.window_enable[0])
	{
		if((lcd_stat.window_x1[0] <= lcd_stat.window_x2[0]) && (scanline_pixel_counter >= lcd_stat.window_x1[0]) && (scanline_pixel_counter <= lcd_stat.window_x2[0]))
		{
			check_x = true;
		}

		else if((lcd_stat.window_x1[0] > lcd_stat.window_x2[0]) && ((scanline_pixel_counter >= lcd_stat.window_x1[0]) || (scanline_pixel_counter <= lcd_stat.window_x2[0])))
		{
			check_x = true;
		}

		if((lcd_stat.window_y1[0] <= lcd_stat.window_y2[0]) && (current_scanline >= lcd_stat.window_y1[0]) && (current_scanline < lcd_stat.window_y2[0]))
		{
			check_y = true;
		}

		else if((lcd_stat.window_y1[0] > lcd_stat.window_y2[0]) && ((current_scanline >= lcd_stat.window_y1[0]) || (current_scanline < lcd_stat.window_y2[0])))
		{
			check_y = true;
		}
		
		if(check_x && check_y) { lcd_stat.in_window = true; lcd_stat.current_window = 0; }
	}

	
	if((lcd_stat.window_enable[1]) && (!lcd_stat.in_window))
	{
		check_x = false;
		check_y = false;

		if((lcd_stat.window_x1[1] <= lcd_stat.window_x2[1]) && (scanline_pixel_counter >= lcd_stat.window_x1[1]) && (scanline_pixel_counter <= lcd_stat.window_x2[1]))
		{
			check_x = true;
		}

		else if((lcd_stat.window_x1[1] > lcd_stat.window_x2[1]) && ((scanline_pixel_counter >= lcd_stat.window_x1[1]) || (scanline_pixel_counter <= lcd_stat.window_x2[1])))
		{
			check_x = true;
		}

		if((lcd_stat.window_y1[1] <= lcd_stat.window_y2[1]) && (current_scanline >= lcd_stat.window_y1[1]) && (current_scanline < lcd_stat.window_y2[1]))
		{
			check_y = true;
		}

		else if((lcd_stat.window_y1[1] > lcd_stat.window_y2[1]) && ((current_scanline >= lcd_stat.window_y1[1]) || (current_scanline < lcd_stat.window_y2[1])))
		{
			check_y = true;
		}
		
		if(check_x && check_y) { lcd_stat.in_window = true; lcd_stat.current_window = 1; }
	}
	

	//Turn off OBJ rendering if in/out of a window where OBJ rendering is disabled
	if((lcd_stat.obj_win_enable) && (obj_win_pixel)) { }
	else if((lcd_stat.window_enable[lcd_stat.current_window]) && (!lcd_stat.in_window) && (!lcd_stat.window_out_enable[4][0])) { obj_render = false; }
	else if((lcd_stat.window_enable[lcd_stat.current_window]) && (lcd_stat.in_window) && (!lcd_stat.window_in_enable[4][lcd_stat.current_window])) { obj_render = false; }

	//Also turn off OBJ rendering if OBJ Window is enabled, but rendered pixel is outside any OBJ Window
	else if((!lcd_stat.in_window) && (lcd_stat.obj_win_enable) && (!obj_win_pixel) && (!lcd_stat.window_out_enable[4][0])) { obj_render = false; }

	//Determine WINOUT status
	winout = (lcd_stat.obj_win_enable || lcd_stat.window_enable[0] || lcd_stat.window_enable[1]);

	//Determine BG rendering priority
	for(int x = 0, list_length = 0; x < 4; x++)
	{
		if(lcd_stat.bg_priority[0] == x) { bg_render_list[list_length++] = 0; }
		if(lcd_stat.bg_priority[1] == x) { bg_render_list[list_length++] = 1; }
		if(lcd_stat.bg_priority[2] == x) { bg_render_list[list_length++] = 2; }
		if(lcd_stat.bg_priority[3] == x) { bg_render_list[list_length++] = 3; }
	}

	//Render BGs based on priority (3 is the 'lowest', 0 is the 'highest')
	for(int x = 0; x < 4; x++)
	{
		bg_id = bg_render_list[x];

		//If an OBJ was the last pixel rendered and has the highest priority, stop rendering BGs now
		if((obj_render) && (last_obj_priority <= lcd_stat.bg_priority[bg_id])) { last_bg_priority = 4; return; }

		//If the last BG pixel is outside the current window, and WINOUT disables this BG layer, skip rendering
		else if((winout) && (!lcd_stat.in_window) && (!lcd_stat.window_out_enable[bg_id][0]) && (!obj_win_pixel)) { continue; }

		//If the last BG pixel is inside the current window, and WININ disables this BG layer, skip rendering
		else if((lcd_stat.window_enable[lcd_stat.current_window]) && (lcd_stat.in_window) && (!lcd_stat.window_in_enable[bg_id][lcd_stat.current_window])) { continue; }

		//If the last pixel is inside the OBJ window, and WINOUT disables this BG layer for the OBJ window, skip rendering
		else if((lcd_stat.obj_win_enable) && (obj_win_pixel) && (!lcd_stat.window_out_enable[bg_id][1])) { continue; }

		//Render BG pixel
		else if(render_bg_pixel(BG0CNT + (bg_id << 1))) { last_bg_priority = bg_id; return; }
	}

	//Use BG Palette #0, Color #0 as the backdrop if no BG or OBJ was rendered
	if(!obj_render) { scanline_buffer[scanline_pixel_counter] = pal[0][0]; }
}

/****** Applies the GBA's SFX to a pixel ******/
void AGB_LCD::apply_sfx()
{
	lcd_stat.temp_sfx_type = lcd_stat.current_sfx_type;

	//If doing brightness up/down and the OBJ mode is Semi-Transparent, force alpha blending
	if((last_bg_priority == 4) && (lcd_stat.current_sfx_type != ALPHA_BLEND) && (last_obj_mode == 1)) { lcd_stat.current_sfx_type = ALPHA_BLEND;  }

	//If doing brightness up/down and the last pixel drawn is not a target, abort SFX
	if((!lcd_stat.sfx_target[last_bg_priority][0]) && (lcd_stat.current_sfx_type != ALPHA_BLEND)) { return; }

	//If doing alpha blending outside of the OBJ Window and the last pixel drawn is not a target, abort SFX 
	if((!lcd_stat.sfx_target[last_bg_priority][0]) && (lcd_stat.current_sfx_type == ALPHA_BLEND) && (!obj_win_pixel) && (last_obj_mode != 1)) { return; }

	bool do_sfx = false;

	//Apply SFX if in Window
	if((lcd_stat.window_enable[lcd_stat.current_window]) && (lcd_stat.in_window) && (lcd_stat.window_in_enable[5][lcd_stat.current_window])) { do_sfx = true; }

	//Apply SFX if out of Window
	else if((lcd_stat.window_enable[lcd_stat.current_window]) && (!lcd_stat.in_window) && (lcd_stat.window_out_enable[5][0])) { do_sfx = true; }

	//Apply SFX if in OBJ Window
	else if((lcd_stat.obj_win_enable) && (obj_win_pixel) && (lcd_stat.window_out_enable[5][1])) { do_sfx = true; }

	//Apply SFX if out of OBJ Window
	else if((!lcd_stat.in_window) && (!obj_win_pixel) && (lcd_stat.obj_win_enable) && (lcd_stat.window_out_enable[5][0])) { do_sfx = true; }

	//Apply SFX to whole screen
	else if((!lcd_stat.window_enable[0]) && (!lcd_stat.window_enable[1]) && (!lcd_stat.obj_win_enable)) { do_sfx = true; }

	//Apply SFX (alpha-blending) if OBJ is semi-transparent
	else if(last_obj_mode == 1) { do_sfx = true; }

	if(!do_sfx) { return; }

	//Apply the specified SFX
	switch(lcd_stat.current_sfx_type)
	{
		case ALPHA_BLEND:
			scanline_buffer[scanline_pixel_counter] = alpha_blend();
			break;

		case BRIGHTNESS_UP: 
			scanline_buffer[scanline_pixel_counter] = brightness_up(); 
			break;

		case BRIGHTNESS_DOWN:
			scanline_buffer[scanline_pixel_counter] = brightness_down(); 
			break;
	}

	lcd_stat.current_sfx_type = lcd_stat.temp_sfx_type;
	lcd_stat.temp_sfx_type = NORMAL;
}

/****** SFX - Increase brightness ******/
u32 AGB_LCD::brightness_up()
{
	u16 color_bytes = last_raw_color;
	u16 result = 0;

	//Increase RGB intensities
	u8 red = (color_bytes & 0x1F);
	result = red + ((0x1F - red) * lcd_stat.brightness_coef);
	red = (result > 0x1F) ? 0x1F : result;
	color_bytes >>= 5;

	u8 green = (color_bytes & 0x1F);
	result = green + ((0x1F - green) * lcd_stat.brightness_coef);
	green = (result > 0x1F) ? 0x1F : result;
	color_bytes >>= 5;

	u8 blue = (color_bytes & 0x1F);
	result = blue + ((0x1F - blue) * lcd_stat.brightness_coef);
	blue = (result > 0x1F) ? 0x1F : result;

	//Return 32-bit color
	return 0xFF000000 | (red << 19) | (green << 11) | (blue << 3);
}

/****** SFX - Decrease brightness ******/
u32 AGB_LCD::brightness_down()
{
	u16 color_bytes = last_raw_color;

	//Decrease RGB intensities 
	u8 red = (color_bytes & 0x1F);
	red -= (red * lcd_stat.brightness_coef);
	color_bytes >>= 5;

	u8 green = (color_bytes & 0x1F);
	green -= (green * lcd_stat.brightness_coef);
	color_bytes >>= 5;

	u8 blue = (color_bytes & 0x1F);
	blue -= (blue * lcd_stat.brightness_coef);

	//Return 32-bit color
	return 0xFF000000 | (red << 19) | (green << 11) | (blue << 3);
}

/****** SFX - Alpha blending ******/
u32 AGB_LCD::alpha_blend()
{		
	//Grab the current 32-bit color for this pixel, in case no alpha blending occurs
	u32 final_color = scanline_buffer[scanline_pixel_counter];

	u16 color_1 = last_raw_color;
	u16 color_2 = 0x0;
	u16 result = 0;
	u8 next_bg_priority = 0;
	bool do_blending = false;

	//When blending with an OBJ Window pixel, the 1st target needs to be manually calculated
	if(obj_win_pixel)
	{
		for(int x = 0; x < 4; x++)
		{
			//OBJ is 1st target
			if((last_obj_priority == x) && (lcd_stat.sfx_target[4][0] || (last_obj_mode == 1)) && (!do_blending)) { do_blending = render_sprite_pixel(); last_bg_priority = 4;  }
	
			//BG0 is 1st target
			if((lcd_stat.bg_priority[0] == x) && (lcd_stat.sfx_target[0][0]) && (!do_blending)) { do_blending = render_bg_pixel(BG0CNT); last_bg_priority = 0; }

			//BG1 is 1st target
			if((lcd_stat.bg_priority[1] == x) && (lcd_stat.sfx_target[1][0]) && (!do_blending)) { do_blending = render_bg_pixel(BG1CNT); last_bg_priority = 1; }

			//BG2 is 1st target
			if((lcd_stat.bg_priority[2] == x) && (lcd_stat.sfx_target[2][0]) && (!do_blending)) { do_blending = render_bg_pixel(BG2CNT); last_bg_priority = 2; }

			//BG3 is 1st target
			if((lcd_stat.bg_priority[3] == x) && (lcd_stat.sfx_target[3][0]) && (!do_blending)) { do_blending = render_bg_pixel(BG3CNT); last_bg_priority = 3; }

			if(do_blending) { x = 4; }
		}

		do_blending = false;
		color_1 = last_raw_color;
	}

	//If the BD is the 1st target, abort alpha blending (no pixel technically exists behind it for blending)
	if(last_bg_priority == 5) { return final_color; }

	//If BG0-3 was drawn last but is not the 1st target, abort alpha blending
	if((last_bg_priority < 4) && (!lcd_stat.sfx_target[last_bg_priority][0])) { return final_color; }

	//If no 1st target is set, abort alpha blending unless semi-trasnparent OBJ
	if(((mem->memory_map[BLDCNT] & 0x3F) == 0) && (last_obj_mode != 1)) { return final_color; }

	//Determine which priority to start looking at to grab the 2nd target
	u8 current_bg_priority = (last_bg_priority == 4) ? last_obj_priority : lcd_stat.bg_priority[last_bg_priority];

	//Grab next closest 2nd target, if any
	for(int x = current_bg_priority; x < 4; x++)
	{
		//Blend with OBJ
		if((last_obj_priority == x) && (last_bg_priority != 4) && (!do_blending)) { do_blending = render_sprite_pixel(); next_bg_priority = 4; }
	
		//Blend with BG0
		if((lcd_stat.bg_priority[0] == x) && (last_bg_priority != 0) && (!do_blending)) { do_blending = render_bg_pixel(BG0CNT); next_bg_priority = 0; }

		//Blend with BG1
		if((lcd_stat.bg_priority[1] == x) && (last_bg_priority != 1) && (!do_blending)) { do_blending = render_bg_pixel(BG1CNT); next_bg_priority = 1; }

		//Blend with BG2
		if((lcd_stat.bg_priority[2] == x) && (last_bg_priority != 2) && (!do_blending)) { do_blending = render_bg_pixel(BG2CNT); next_bg_priority = 2; }

		//Blend with BG3
		if((lcd_stat.bg_priority[3] == x) && (last_bg_priority != 3) && (!do_blending)) { do_blending = render_bg_pixel(BG3CNT); next_bg_priority = 3; }

		if(do_blending) { x = 4; }
	}

	//Grab BD as 2nd target if possible
	if((!do_blending) && (lcd_stat.sfx_target[5][1])) { last_raw_color = raw_pal[0][0]; do_blending = true; }

	//If the 2nd target is rendered and not specified for blending, abort 
	if((do_blending) && (!lcd_stat.sfx_target[next_bg_priority][1])) { return final_color; } 

	if(!do_blending) 
	{
		//If no alpha-blending occurs, see if Brightness Increase can be applied (for semi-transparent OBJ only)
		if(lcd_stat.temp_sfx_type == BRIGHTNESS_UP) { return brightness_up(); }

		//If no alpha-blending occurs, see if Brightness Decrease can be applied (for semi-transparent OBJ only)
		else if(lcd_stat.temp_sfx_type == BRIGHTNESS_DOWN) { return brightness_down(); }

		//If no alpha-blending occurs and no fringe cases occur, abort
		else { return final_color; }
	}

	color_2 = last_raw_color;

	//Alpha-blending
	result = ((color_1 & 0x1F) * lcd_stat.alpha_a_coef) + ((color_2 & 0x1F) * lcd_stat.alpha_b_coef);
	u8 red = (result > 0x1F) ? 0x1F : result;
	color_1 >>= 5;
	color_2 >>= 5;

	result = ((color_1 & 0x1F) * lcd_stat.alpha_a_coef) + ((color_2 & 0x1F) * lcd_stat.alpha_b_coef);
	u8 green = (result > 0x1F) ? 0x1F : result;
	color_1 >>= 5;
	color_2 >>= 5;

	result = ((color_1 & 0x1F) * lcd_stat.alpha_a_coef) + ((color_2 & 0x1F) * lcd_stat.alpha_b_coef);
	u8 blue = (result > 0x1F) ? 0x1F : result;
	color_1 >>= 5;
	color_2 >>= 5;

	//Return 32-bit color
	return 0xFF000000 | (red << 19) | (green << 11) | (blue << 3);
}

/****** Immediately draw current buffer to the screen ******/
void AGB_LCD::update()
{
	//Use SDL
	if(config::sdl_render)
	{
		//Lock source surface
		if(SDL_MUSTLOCK(final_screen)){ SDL_LockSurface(final_screen); }
		u32* out_pixel_data = (u32*)final_screen->pixels;

		for(int a = 0; a < 0x9600; a++)
		{
			out_pixel_data[a] = screen_buffer[a];
			if(mem->sub_screen_buffer.size()) { out_pixel_data[0x9600 + a] = mem->sub_screen_buffer[a]; }
		}

		//Unlock source surface
		if(SDL_MUSTLOCK(final_screen)){ SDL_UnlockSurface(final_screen); }
		
		//Display final screen buffer - OpenGL
		if(config::use_opengl) { opengl_blit(); }
				
		//Display final screen buffer - SDL
		else 
		{
			if(SDL_UpdateWindowSurface(window) != 0)
			{
				std::cout<<"LCD::Error - Could not blit\n";

				//Try to make a new the window if the blit failed
				if(!try_window_rebuild)
				{
					try_window_rebuild = true;
					if((window != NULL) && (config::sdl_render)) { SDL_DestroyWindow(window); }
					init();
				}
			}

			else { try_window_rebuild = false; }
		}
	}

	//Use external rendering method (GUI)
	else
	{
		if(!config::use_opengl)
		{
			if(mem->sub_screen_buffer.size())
			{
				for(int a = 0; a < 0x9600; a++) { screen_buffer[0x9600 + a] = mem->sub_screen_buffer[a]; }
			}

			config::render_external_sw(screen_buffer);
		}

		else
		{
			//Lock source surface
			if(SDL_MUSTLOCK(final_screen)){ SDL_LockSurface(final_screen); }
			u32* out_pixel_data = (u32*)final_screen->pixels;

			for(int a = 0; a < 0x9600; a++)
			{
				out_pixel_data[a] = screen_buffer[a];
				if(mem->sub_screen_buffer.size()) { out_pixel_data[0x9600 + a] = mem->sub_screen_buffer[a]; }
			}

			//Unlock source surface
			if(SDL_MUSTLOCK(final_screen)){ SDL_UnlockSurface(final_screen); }

			config::render_external_hw(final_screen);
		}
	}
}

/****** Clears the screen buffer with a given color ******/
void AGB_LCD::clear_screen_buffer(u32 color)
{
	for(u32 x = 0; x < 0x9600; x++) { screen_buffer[x] = color; }
}

/****** Run LCD for one cycle ******/
void AGB_LCD::step()
{
	lcd_clock++;

	//Mode 0 - Scanline rendering
	if(((lcd_clock % 1232) <= 960) && (lcd_clock < 197120)) 
	{
		//Increment scanline count
		if(mem->memory_map[DISPSTAT] & 0x2)
		{
			current_scanline++;
			mem->write_u16_fast(VCOUNT, current_scanline);
			scanline_compare();
		}

		//Change mode
		if(lcd_mode != 0) 
		{
			//Update OAM
			if(lcd_stat.oam_update) { update_oam(); }

			//Update palettes
			if((lcd_stat.bg_pal_update) || (lcd_stat.obj_pal_update)) { update_palettes(); }

			//Toggle HBlank flag OFF
			mem->memory_map[DISPSTAT] &= ~0x2;

			lcd_mode = 0; 
			update_obj_render_list();

			//Update BG affine parameters
			//If Line 0, reset X and Y positions
			if((lcd_stat.bg_mode != 0) && (current_scanline == 0))
			{
				reload_affine_references(BG2CNT);
				reload_affine_references(BG3CNT);
			}

			//If starting any other line, add DMX and DMY to X and Y positions
			else if((lcd_stat.bg_mode != 0) && (current_scanline != 0))
			{
				lcd_stat.bg_affine[0].x_ref += lcd_stat.bg_affine[0].dmx;
				lcd_stat.bg_affine[0].y_ref += lcd_stat.bg_affine[0].dmy;
				lcd_stat.bg_affine[0].x_pos = lcd_stat.bg_affine[0].x_ref;
				lcd_stat.bg_affine[0].y_pos = lcd_stat.bg_affine[0].y_ref;

				lcd_stat.bg_affine[1].x_ref += lcd_stat.bg_affine[1].dmx;
				lcd_stat.bg_affine[1].y_ref += lcd_stat.bg_affine[1].dmy;
				lcd_stat.bg_affine[1].x_pos = lcd_stat.bg_affine[1].x_ref;
				lcd_stat.bg_affine[1].y_pos = lcd_stat.bg_affine[1].y_ref;
			}
		}
	}

	//Mode 1 - H-Blank
	else if(((lcd_clock % 1232) > 960) && (lcd_clock < 197120))
	{
		//Change mode
		if(lcd_mode != 1) 
		{
			//Render scanline data
			for(u32 x = 0; x < 256; x++) 
			{
				render_scanline();
				if(lcd_stat.current_sfx_type != NORMAL) { apply_sfx(); }
				scanline_pixel_counter++;
			}

			//Toggle HBlank flag ON
			mem->memory_map[DISPSTAT] |= 0x2;

			lcd_mode = 1;
			scanline_pixel_counter = 0;

			//Raise HBlank interrupt
			if(mem->memory_map[DISPSTAT] & 0x10) { mem->memory_map[REG_IF] |= 0x2; }

			//Push scanline data to final buffer - Only if Forced Blank is disabled
			if((lcd_stat.display_control & 0x80) == 0)
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
	
			//Start HBlank DMA
			mem->start_blank_dma();
		}

		if((config::use_cheats) && ((current_scanline & 0x7) == 0)) { mem->set_cheats(); }
	}

	//Mode 2 - VBlank
	else
	{
		//Toggle VBlank flag
		if(current_scanline < 227 ) { mem->memory_map[DISPSTAT] |= 0x1; }
		else { mem->memory_map[DISPSTAT] &= ~0x1; }

		//Change mode
		if(lcd_mode != 2) 
		{
			lcd_mode = 2;

			//Check for screen resize - CDZ sub screen
			if((config::request_resize) && (config::resize_mode > 0))
			{
				config::sys_width = 240;
				config::sys_height = 320;
				screen_buffer.clear();
				screen_buffer.resize(0x12C00, 0xFFFFFFFF);
					
				if((window != NULL) && (config::sdl_render)) { SDL_DestroyWindow(window); }
				init();
					
				if(config::sdl_render) { config::request_resize = false; }
			}

			//Check for screen resize - Exit sub screen
			else if(config::request_resize)
			{
				config::sys_width = 240;
				config::sys_height = 160;
				screen_buffer.clear();
				screen_buffer.resize(0x9600, 0xFFFFFFFF);
					
				if((window != NULL) && (config::sdl_render)) { SDL_DestroyWindow(window); }
				init();
					
				if(config::sdl_render) { config::request_resize = false; }

				mem->sub_screen_buffer.clear();
			}

			//Toggle HBlank flag OFF
			mem->memory_map[DISPSTAT] &= ~0x2;

			//Increment scanline count
			current_scanline++;
			mem->write_u16_fast(VCOUNT, current_scanline);
			scanline_compare();

			//Raise VBlank interrupt
			if(mem->memory_map[DISPSTAT] & 0x8) { mem->memory_map[REG_IF] |= 0x1; }

			//Display any OSD messages
			if(config::osd_count)
			{
				config::osd_count--;
				draw_osd_msg(config::osd_message, screen_buffer, 0, 0);
			}

			//Process Power Antenna
			if(power_antenna_osd)
			{
				u8 x_offset = (config::sys_width / 8) - 3;
				u8 y_offset = (config::sys_height / 8) - 1;
				draw_osd_msg(std::string("***"), screen_buffer, x_offset, y_offset);
			}

			//Update subscreen per frame
			if(mem->sub_screen_update) { mem->sub_screen_lock = false; }

			//Jukebox
			if((config::cart_type == AGB_JUKEBOX) && (mem->jukebox.progress)) { mem->process_jukebox(); }

			//Process Turbo Buttons
			if(mem->g_pad->turbo_button_enabled) { mem->g_pad->process_turbo_buttons(); }

			if(mem->g_pad->is_gb_player) { mem->g_pad->process_gb_rumble(); }

			//Use SDL
			if(config::sdl_render)
			{
				//If using SDL and no OpenGL, manually stretch for fullscreen via SDL
				if((config::flags & SDL_WINDOW_FULLSCREEN) && (!config::use_opengl))
				{
					//Lock source surface
					if(SDL_MUSTLOCK(original_screen)){ SDL_LockSurface(original_screen); }
					u32* out_pixel_data = (u32*)original_screen->pixels;

					for(int a = 0; a < 0x9600; a++)
					{
						out_pixel_data[a] = screen_buffer[a];
						if(mem->sub_screen_buffer.size()) { out_pixel_data[0x9600 + a] = mem->sub_screen_buffer[a]; }
					}

					//Unlock source surface
					if(SDL_MUSTLOCK(original_screen)){ SDL_UnlockSurface(original_screen); }
		
					//Blit the original surface to the final stretched one
					SDL_Rect dest_rect;
					dest_rect.w = config::sys_width * max_fullscreen_ratio;
					dest_rect.h = config::sys_height * max_fullscreen_ratio;
					dest_rect.x = ((config::win_width - dest_rect.w) >> 1);
					dest_rect.y = ((config::win_height - dest_rect.h) >> 1);
					SDL_BlitScaled(original_screen, NULL, final_screen, &dest_rect);

					if(SDL_UpdateWindowSurface(window) != 0)
					{
						std::cout<<"LCD::Error - Could not blit\n";

						//Try to make a new the window if the blit failed
						if(!try_window_rebuild)
						{
							try_window_rebuild = true;
							if((window != NULL) && (config::sdl_render)) { SDL_DestroyWindow(window); }
							init();
						}
					}

					else { try_window_rebuild = false; }
				}
					
				//Otherwise, render normally (SDL 1:1, OpenGL handles its own stretching)
				else
				{
					//Lock source surface
					if(SDL_MUSTLOCK(final_screen)){ SDL_LockSurface(final_screen); }
					u32* out_pixel_data = (u32*)final_screen->pixels;

					for(int a = 0; a < 0x9600; a++)
					{
						out_pixel_data[a] = screen_buffer[a];
						if(mem->sub_screen_buffer.size()) { out_pixel_data[0x9600 + a] = mem->sub_screen_buffer[a]; }
					}

					//Unlock source surface
					if(SDL_MUSTLOCK(final_screen)){ SDL_UnlockSurface(final_screen); }
		
					//Display final screen buffer - OpenGL
					if(config::use_opengl) { opengl_blit(); }
				
					//Display final screen buffer - SDL
					else 
					{
						if(SDL_UpdateWindowSurface(window) != 0)
						{
							std::cout<<"LCD::Error - Could not blit\n";

							//Try to make a new the window if the blit failed
							if(!try_window_rebuild)
							{
								try_window_rebuild = true;
								if((window != NULL) && (config::sdl_render)) { SDL_DestroyWindow(window); }
								init();
							}
						}

						else { try_window_rebuild = false; }
					}
				}
			}

			//Use external rendering method (GUI)
			else
			{
				if(!config::use_opengl)
				{
					if(mem->sub_screen_buffer.size())
					{
						for(int a = 0; a < 0x9600; a++) { screen_buffer[0x9600 + a] = mem->sub_screen_buffer[a]; }
					}

					config::render_external_sw(screen_buffer);
				}

				else
				{
					//Lock source surface
					if(SDL_MUSTLOCK(final_screen)){ SDL_LockSurface(final_screen); }
					u32* out_pixel_data = (u32*)final_screen->pixels;

					for(int a = 0; a < 0x9600; a++)
					{
						out_pixel_data[a] = screen_buffer[a];
						if(mem->sub_screen_buffer.size()) { out_pixel_data[0x9600 + a] = mem->sub_screen_buffer[a]; }
					}

					//Unlock source surface
					if(SDL_MUSTLOCK(final_screen)){ SDL_UnlockSurface(final_screen); }

					config::render_external_hw(final_screen);
				}
			}

			//Limit framerate
			if(!config::turbo)
			{
				frame_current_time = SDL_GetTicks();
				int delay = frame_delay[fps_count % 60];
				if((frame_current_time - frame_start_time) < delay) { SDL_Delay(delay - (frame_current_time - frame_start_time));}
				frame_start_time = SDL_GetTicks();
			}

			//Update FPS counter + title
			fps_count++;
			if(((SDL_GetTicks() - fps_time) >= 1000) && (config::sdl_render))
			{ 
				fps_time = SDL_GetTicks(); 
				config::title.str("");
				config::title << "GBE+ " << fps_count << "FPS";
				SDL_SetWindowTitle(window, config::title.str().c_str());
				fps_count = 0; 
			}

			//Process motion controls
			if((config::cart_type == AGB_GYRO_SENSOR) || (config::cart_type == AGB_TILT_SENSOR)) { mem->process_motion(); }

			//Start GB Player Rumble transfers once-per-frame
			if(config::sio_device == SIO_GB_PLAYER_RUMBLE)
			{
				if(!mem->g_pad->is_gb_player) { mem->g_pad->gb_player_count++; }
				if(mem->g_pad->gb_player_timeout < 4) { mem->g_pad->gb_player_timeout++; }
				else { mem->g_pad->stop_rumble(); }
			}

			//Process Play-Yan interrupts
			if((config::cart_type == AGB_PLAY_YAN) && (mem->play_yan.irq_delay)) { mem->process_play_yan_irq(); }
			
			else if((config::cart_type == AGB_PLAY_YAN) && (mem->play_yan.is_video_playing) && (mem->play_yan.is_media_playing)
			&& (!mem->play_yan_get_headphone_status()))
			{
				mem->play_yan.update_video_frame = true;
				mem->process_play_yan_irq();
			}

			//Process Glucoboy interrupts
			else if((config::cart_type == AGB_GLUCOBOY) && (mem->glucoboy.request_interrupt)) { mem->process_glucoboy_irq(); }

			//Process Campho events (audio/video) 
			else if(config::cart_type == AGB_CAMPHO) { mem->process_campho(); }
		}

		//Setup HBlank
		else if((lcd_clock % 1232) == 960) 
		{
			//Toggle HBlank flag ON
			mem->memory_map[DISPSTAT] |= 0x2;

			//Raise HBlank interrupt
			if(mem->memory_map[DISPSTAT] & 0x10) { mem->memory_map[REG_IF] |= 0x2; }
		}

		//Reset LCD clock
		else if(lcd_clock == 280896) 
		{
			//Toggle VBlank flag OFF
			mem->memory_map[DISPSTAT] &= ~0x1;

			//Toggle HBlank flag OFF
			mem->memory_map[DISPSTAT] &= ~0x2;

			lcd_clock = 0; 
			current_scanline = 0; 
			mem->write_u16_fast(VCOUNT, 0);
			scanline_compare();
			scanline_pixel_counter = 0; 
		}

		//Increment Scanline after HBlank
		else if(lcd_clock % 1232 == 0)
		{
			//Toggle HBlank flag OFF
			mem->memory_map[DISPSTAT] &= ~0x2;

			current_scanline++;
			mem->write_u16_fast(VCOUNT, current_scanline);
			scanline_compare();
		}
	}
}

/****** Compare VCOUNT to LYC ******/
void AGB_LCD::scanline_compare()
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

/****** Grabs the most current X-Y references for affine backgrounds ******/
void AGB_LCD::reload_affine_references(u32 bg_control)
{
	u32 x_raw, y_raw;
	u8 aff_id;

	//Read X-Y references from memory, determine which engine it belongs to
	switch(bg_control)
	{
		case BG2CNT:
			x_raw = mem->read_u32_fast(BG2X_L);
			y_raw = mem->read_u32_fast(BG2Y_L);
			aff_id = 0;
			break;

		case BG3CNT:
			x_raw = mem->read_u32_fast(BG3X_L);
			y_raw = mem->read_u32_fast(BG3Y_L);
			aff_id = 1;
			break;

		default: return;
	}
			
	//Get X reference point
	if(x_raw & 0x8000000) 
	{ 
		u32 x = ((x_raw >> 8) - 1);
		x = (~x & 0x7FFFF);
		
		lcd_stat.bg_affine[aff_id].x_ref = -1.0 * x;
	}
	
	else { lcd_stat.bg_affine[aff_id].x_ref = (x_raw >> 8) & 0x7FFFF; }
	
	if((x_raw & 0xFF) != 0) { lcd_stat.bg_affine[aff_id].x_ref += (x_raw & 0xFF) / 256.0; }

	//Set current X position as the new reference point
	lcd_stat.bg_affine[aff_id].x_pos = lcd_stat.bg_affine[aff_id].x_ref;

	//Get X reference point
	if(y_raw & 0x8000000) 
	{ 
		u32 y = ((y_raw >> 8) - 1);
		y = (~y & 0x7FFFF);
		
		lcd_stat.bg_affine[aff_id].y_ref = -1.0 * y;
	}
	
	else { lcd_stat.bg_affine[aff_id].y_ref = (y_raw >> 8) & 0x7FFFF; }
	
	if((y_raw & 0xFF) != 0) { lcd_stat.bg_affine[aff_id].y_ref += (y_raw & 0xFF) / 256.0; }

	//Set current Y position as the new reference point
	lcd_stat.bg_affine[aff_id].y_pos = lcd_stat.bg_affine[aff_id].y_ref;
}

/****** Read LCD data from save state ******/
bool AGB_LCD::lcd_read(u32 offset, std::string filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary);
	
	if(!file.is_open()) { return false; }

	//Go to offset
	file.seekg(offset);

	//Serialize LCD data from save state
	file.read((char*)&lcd_stat, sizeof(lcd_stat));

	//Serialize OBJ data from save state
	for(int x = 0; x < 128; x++)
	{
		file.read((char*)&obj[x], sizeof(obj[x]));
		file.read((char*)&obj_render_list[x], sizeof(obj_render_list[x]));
	}

	//Serialize Misc LCD data from save state
	file.read((char*)&lcd_mode, sizeof(lcd_mode));
	file.read((char*)&current_scanline, sizeof(current_scanline));
	file.read((char*)&lcd_clock, sizeof(lcd_clock));
	file.read((char*)&obj_render_length, sizeof(obj_render_length));
	file.read((char*)&last_obj_priority, sizeof(last_obj_priority));
	file.read((char*)&last_obj_mode, sizeof(last_obj_mode));
	file.read((char*)&last_bg_priority, sizeof(last_bg_priority));
	file.read((char*)&last_raw_color, sizeof(last_raw_color));
	file.read((char*)&obj_win_pixel, sizeof(obj_win_pixel));
	file.read((char*)&scanline_pixel_counter, sizeof(scanline_pixel_counter));

	for(int x = 0; x < 256; x++)
	{
		for(int y = 0; y < 2; y++)
		{
			file.read((char*)&pal[x][y], sizeof(pal[x][y]));
			file.read((char*)&raw_pal[x][y], sizeof(raw_pal[x][y]));
		}
	}

	for(int x = 0; x < 4; x++)
	{
		file.read((char*)&bg_offset_x[x], sizeof(bg_offset_x[x]));
		file.read((char*)&bg_offset_y[x], sizeof(bg_offset_y[x]));
	}

	file.close();
	return true;
}

/****** Read LCD data from save state ******/
bool AGB_LCD::lcd_write(std::string filename)
{
	std::ofstream file(filename.c_str(), std::ios::binary | std::ios::app);
	
	if(!file.is_open()) { return false; }

	//Serialize LCD data to save state
	file.write((char*)&lcd_stat, sizeof(lcd_stat));

	//Serialize OBJ data to save state
	for(int x = 0; x < 128; x++)
	{
		file.write((char*)&obj[x], sizeof(obj[x]));
		file.write((char*)&obj_render_list[x], sizeof(obj_render_list[x]));
	}

	//Serialize Misc LCD data to save state
	file.write((char*)&lcd_mode, sizeof(lcd_mode));
	file.write((char*)&current_scanline, sizeof(current_scanline));
	file.write((char*)&lcd_clock, sizeof(lcd_clock));
	file.write((char*)&obj_render_length, sizeof(obj_render_length));
	file.write((char*)&last_obj_priority, sizeof(last_obj_priority));
	file.write((char*)&last_obj_mode, sizeof(last_obj_mode));
	file.write((char*)&last_bg_priority, sizeof(last_bg_priority));
	file.write((char*)&last_raw_color, sizeof(last_raw_color));
	file.write((char*)&obj_win_pixel, sizeof(obj_win_pixel));
	file.write((char*)&scanline_pixel_counter, sizeof(scanline_pixel_counter));

	for(int x = 0; x < 256; x++)
	{
		for(int y = 0; y < 2; y++)
		{
			file.write((char*)&pal[x][y], sizeof(pal[x][y]));
			file.write((char*)&raw_pal[x][y], sizeof(raw_pal[x][y]));
		}
	}

	for(int x = 0; x < 4; x++)
	{
		file.write((char*)&bg_offset_x[x], sizeof(bg_offset_x[x]));
		file.write((char*)&bg_offset_y[x], sizeof(bg_offset_y[x]));
	}

	file.close();
	return true;
}
