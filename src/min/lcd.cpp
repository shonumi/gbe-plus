// GB Enhanced Copyright Daniel Baxter 2021
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd.cpp
// Date : March 11, 2021
// Description : Pokemon Mini LCD emulation
//
// Draws background, window, and sprites to screen
// Responsible for blitting pixel data and limiting frame rate

#include <cmath>

#include "lcd.h"
#include "common/util.h"

/****** LCD Constructor ******/
MIN_LCD::MIN_LCD()
{
	window = NULL;
	reset();
}

/****** LCD Destructor ******/
MIN_LCD::~MIN_LCD()
{
	screen_buffer.clear();
	SDL_DestroyWindow(window);

	std::cout<<"LCD::Shutdown\n";
}

/****** Reset LCD ******/
void MIN_LCD::reset()
{
	final_screen = NULL;
	original_screen = NULL;
	mem = NULL;

	if((window != NULL) && (config::sdl_render)) { SDL_DestroyWindow(window); }
	window = NULL;

	screen_buffer.clear();
	screen_buffer.resize(0x1800, 0xFFFFFFFF);

	old_buffer.clear();
	old_buffer.resize(0x1800, 0xFFFFFFFF);

	new_frame = false;

	lcd_stat.prc_counter = 0;
	lcd_stat.prc_clock = 0;
	lcd_stat.prc_copy_wait = 0;

	lcd_stat.force_update = false;
	lcd_stat.sed_enabled = true;

	frame_start_time = 0;
	frame_current_time = 0;
	fps_count = 0;
	fps_time = 0;

	//Initialize system screen dimensions
	config::sys_width = 96;
	config::sys_height = 64;

	max_fullscreen_ratio = 2;
}

/****** Initialize LCD with SDL ******/
bool MIN_LCD::init()
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
			window = SDL_CreateWindow("GBE+", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, config::sys_width, config::sys_height, config::flags);
			SDL_GetWindowSize(window, &config::win_width, &config::win_height);
			final_screen = SDL_GetWindowSurface(window);
			original_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, config::sys_width, config::sys_height, 32, 0, 0, 0, 0);
			config::scaling_factor = 1;
		}

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

/****** Update LCD and render pixels ******/
void MIN_LCD::update()
{
	//Only render if SED1565 is enabled
	if(!lcd_stat.sed_enabled) { return; }

	//Render map
	if(lcd_stat.enable_map || lcd_stat.force_update) { render_map(); }

	//Render sprites
	if(lcd_stat.enable_obj) { render_obj(); }

	//Render pixel for a new frame if necessary
	if(new_frame) { render_frame(); }

	//Display any OSD messages
	if(config::osd_count)
	{
		config::osd_count--;
		draw_osd_msg(config::osd_message, screen_buffer, 0, 0);
	}

	//Use SDL
	if(config::sdl_render)
	{
		//If using SDL and no OpenGL, manually stretch for fullscreen via SDL
		if((config::flags & SDL_WINDOW_FULLSCREEN_DESKTOP) && (!config::use_opengl))
		{
			//Lock source surface
			if(SDL_MUSTLOCK(original_screen)){ SDL_LockSurface(original_screen); }
			u32* out_pixel_data = (u32*)original_screen->pixels;

			for(int a = 0; a < 0x1800; a++)
			{
				out_pixel_data[a] = screen_buffer[a];
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

			if(SDL_UpdateWindowSurface(window) != 0) { std::cout<<"LCD::Error - Could not blit\n"; }
		}
					
		//Otherwise, render normally (SDL 1:1, OpenGL handles its own stretching)
		else
		{
			//Lock source surface
			if(SDL_MUSTLOCK(final_screen)){ SDL_LockSurface(final_screen); }
			u32* out_pixel_data = (u32*)final_screen->pixels;

			for(int a = 0; a < 0x1800; a++)
			{
				out_pixel_data[a] = screen_buffer[a];
			}

			//Unlock source surface
			if(SDL_MUSTLOCK(final_screen)){ SDL_UnlockSurface(final_screen); }
		
			//Display final screen buffer - OpenGL
			if(config::use_opengl) { opengl_blit(); }
				
			//Display final screen buffer - SDL
			else 
			{
				if(SDL_UpdateWindowSurface(window) != 0) { std::cout<<"LCD::Error - Could not blit\n"; }
			}
		}
	}

	//Use external rendering method (GUI)
	else
	{
		if(!config::use_opengl)
		{
			config::render_external_sw(screen_buffer);
		}

		else
		{
			//Lock source surface
			if(SDL_MUSTLOCK(final_screen)){ SDL_LockSurface(final_screen); }
			u32* out_pixel_data = (u32*)final_screen->pixels;

			for(int a = 0; a < 0x1800; a++)
			{
				out_pixel_data[a] = screen_buffer[a];
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
		if((frame_current_time - frame_start_time) < 14) { SDL_Delay(14 - (frame_current_time - frame_start_time));}
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
}

/****** Renders a new frame of the Pokemon Mini PRC map ******/
void MIN_LCD::render_map()
{
	lcd_stat.force_update = false;

	//Calculate dimensions of the map
	u8 map_w = 0;
	u8 map_h = 0;

	u32 scr_w = 0;
	u32 scr_h = 0;

	switch(lcd_stat.map_size)
	{
		case 0:
			map_w = 12;
			map_h = 16;
			break;

		case 1:
			map_w = 16;
			map_h = 12;
			break;

		case 2:
			map_w = 24;
			map_h = 8;
			break;

		case 3:
			map_w = 24;
			map_h = 16;
			break;
	}

	scr_w = map_w * 8;
	scr_h = map_h * 8;

	//Create a temp map before plotting pixel data
	std::vector <u8> temp_map;
	temp_map.resize((scr_w * scr_h), 0x00);

	u32 tile_number = 0;
	u32 tile_addr = 0;
	u8 tile_byte = 0;
	u8 tile_bit = 0;

	u32 start_pos = 0;
	u32 buffer_pos = 0;
	u32 src_pos = 0;

	u32 px = 0;
	u32 py = 0;

	u32 vx = 0;
	u32 vy = 0;

	//Cycle through all tiles in tile map an calculate ON/OFF pixels
	for(u32 y = 0; y < map_h; y++)
	{
		for(u32 x = 0; x < map_w; x++)
		{
			tile_number = mem->memory_map[0x1360 + (map_w * y) + x];
			tile_addr = lcd_stat.map_addr + (tile_number * 8);
			tile_byte = mem->memory_map[tile_addr++];
			tile_bit = 0;

			start_pos = (scr_w * y * 8) + (x * 8);

			//Render 64 pixels for a given tile
			for(u32 i = 0; i < 64; i++)
			{
				py = i & 0x7;
				px = i >> 3;

				buffer_pos = start_pos + (scr_w * py) + px;

				if(tile_byte & (1 << tile_bit)) { temp_map[buffer_pos] = 0x1; }
				else { temp_map[buffer_pos] = 0x00; }

				tile_bit++;

				if(tile_bit == 8)
				{
					tile_bit = 0;
					tile_byte = mem->memory_map[tile_addr++];
				}
			}
		}
	}

	px = 0;
	py = 0;

	//Copy data from temp map to GDRAM
	for(u32 index = 0; index < 0x300; index++)
	{
		u8 pix_byte = 0;
		u8 pix_mask = 1;

		vx = px + lcd_stat.scroll_x;
		vy = py + lcd_stat.scroll_y;

		src_pos = (scr_w * vy) + vx;
		
		for(u32 y = 0; y < 8; y++)
		{
			if((vx >= scr_w) || (vy >= scr_h)) { }
			else if(temp_map[src_pos] && !lcd_stat.invert_map) { pix_byte |= pix_mask; }
			else if(!temp_map[src_pos] && lcd_stat.invert_map) { pix_byte |= pix_mask; }

			pix_mask <<= 1;
			src_pos += scr_w;
			vy++;
		}

		mem->memory_map[0x1000 + index] = pix_byte;
		px++;

		if(px == 96)
		{
			px = 0;
			py += 8;
		}
	}
}

/****** Renders Pokemon Mini PRC sprites ******/
void MIN_LCD::render_obj()
{
	s16 obj_x = 0;
	s16 obj_y = 0;
	u8 tile_number = 0;
	u8 tile_attr = 0;

	bool enable_obj = false;
	bool invert = false;
	bool v_flip = false;
	bool h_flip = false;

	u32 obj_buffer[256];

	//Cycle through all 24 sprites, start with the last one
	for(int index = 23; index >= 0; index--)
	{
		//Grab sprite attributes
		u32 attr_addr = 0x1300 + (4 * index);
		obj_x = mem->memory_map[attr_addr++] - 16;
		obj_y = mem->memory_map[attr_addr++] - 16;
		tile_number = mem->memory_map[attr_addr++];
		tile_attr = mem->memory_map[attr_addr];

		enable_obj = (tile_attr & 0x8) ? true : false;
		invert = (tile_attr & 0x4) ? true : false;
		v_flip = (tile_attr & 0x2) ? true : false;
		h_flip = (tile_attr & 0x1) ? true : false;

		u32 on_pixel = (invert) ? 0 : 1;
		u32 off_pixel = (invert) ? 1 : 0;
		u32 clr_pixel = 2;

		if(!enable_obj) { continue; }

		//Start drawing 16x16 segments to a temporary buffer
		//Draw them as 8x8 portions
		for(u8 obj_index = 0; obj_index < 4; obj_index++)
		{
			u32 obj_mask_addr = lcd_stat.obj_addr + (tile_number * 64) + ((obj_index >> 1) * 32);
			if(obj_index & 0x1) { obj_mask_addr += 8; }

			u32 obj_tile_addr = obj_mask_addr + 16;

			for(u32 x = 0; x < 8; x++)
			{
				u8 mask_byte = ~mem->memory_map[obj_mask_addr++];
				u8 tile_byte = mem->memory_map[obj_tile_addr++];
				u8 tile_bit = 0;

				u8 buffer_pos = (obj_index > 1) ? 8 : 0;
				if(obj_index & 0x1) { buffer_pos += 128; }
				buffer_pos += x;

				for(u32 y = 0; y < 8; y++)
				{
					//If mask is opaque, draw on or off pixel
					if(mask_byte & (1 << y))
					{
						obj_buffer[buffer_pos] = (tile_byte & (1 << y)) ? on_pixel : off_pixel;
					}

					//If mask is transparent, draw nothing
					else
					{
						obj_buffer[buffer_pos] = clr_pixel;
					}

					buffer_pos += 16;
				}
			}
		}

		//Horizontal flip
		if(h_flip)
		{
			u32 h_pos = 0;
			u32 temp_pix = 0;

			for(u32 y = 0; y < 16; y++)
			{
				u8 pix_pos = (y * 16) + 15;

				for(u32 x = 0; x < 8; x++)
				{
					h_pos = (y * 16) + x;

					temp_pix = obj_buffer[pix_pos];
					obj_buffer[pix_pos] = obj_buffer[h_pos];
					obj_buffer[h_pos] = temp_pix;

					pix_pos--;
				}
			}
		}

		//Vertical flip
		if(v_flip)
		{
			u32 v_pos = 0;
			u32 temp_pix = 0;

			for(u32 x = 0; x < 16; x++)
			{
				u8 pix_pos = x + 240;

				for(u32 y = 0; y < 8; y++)
				{
					v_pos = (y * 16) + x;

					temp_pix = obj_buffer[pix_pos];
					obj_buffer[pix_pos] = obj_buffer[v_pos];
					obj_buffer[v_pos] = temp_pix;

					pix_pos -= 16;
				}
			}
		}
					
		u32 px = 0;
		u32 py = 0;

		u32 vx = 0;
		u32 vy = 0;

		u32 buffer_pos = 0;
		u32 src_pos = 0;

		//Copy data to GDRAM
		for(u32 ram_index = 0; ram_index < 32; ram_index++)
		{
			vx = px + obj_x;
			vy = py + obj_y;

			src_pos = (16 * py) + px;
			buffer_pos = 0x1000 + vx + ((vy >> 3) * 96);

			u8 pix_mask = 1 << (vy & 0x7);
			u8 pix_byte = mem->memory_map[buffer_pos];

			for(u32 y = 0; y < 8; y++)
			{
				if((vx < 0) || (vy < 0) || (vx > 95) || (vy > 63) || (obj_buffer[src_pos] == 2)) { }
				else if(obj_buffer[src_pos] == 1) { pix_byte |= pix_mask; }
				else if(obj_buffer[src_pos] == 0) { pix_byte &= ~pix_mask; }

				vy++;
				src_pos += 16;

				pix_mask <<= 1;

				if(!pix_mask)
				{
					pix_mask = 1;
					mem->memory_map[buffer_pos] = pix_byte;

					buffer_pos = 0x1000 + vx + ((vy >> 3) * 96);
					pix_byte = mem->memory_map[buffer_pos];
				}
			}

			mem->memory_map[buffer_pos] = pix_byte;

			px++;

			if(px == 16)
			{
				px = 0;
				py += 8;
			}
		}
	}
}

/****** Renders the final framebuffer for the Pokemon Mini ******/
void MIN_LCD::render_frame()
{
	u32 on_pixel = 0xFF000000;
	u32 mid_pixel = 0xFF808080;
	u32 off_pixel = 0xFFFFFFFF;
	u32 temp_pixel = 0;

	u32 px = 0;
	u32 py = 0;

	u32 buffer_pos = 0;

	//Read bytes from GDRAM and draw them to the framebuffer
	for(u32 index = 0; index < 0x300; index++)
	{
		u8 pix_byte = mem->memory_map[0x1000 + index];
		u8 pix_mask = 1;

		buffer_pos = (96 * py) + px;

		for(u32 y = 0; y < 8; y++)
		{
			if(pix_byte & pix_mask) { screen_buffer[buffer_pos] = on_pixel; }
			else { screen_buffer[buffer_pos] = off_pixel; }

			//3-color blending
			if(config::min_config & 0x1)
			{
				temp_pixel = screen_buffer[buffer_pos];

				if(((temp_pixel == off_pixel) && (old_buffer[buffer_pos] == on_pixel))
				|| ((temp_pixel == on_pixel) && (old_buffer[buffer_pos] == off_pixel)))
				{
					screen_buffer[buffer_pos] = mid_pixel;
				}

				old_buffer[buffer_pos] = temp_pixel;
			}

			pix_mask <<= 1;
			buffer_pos += 96;
		}

		px++;

		if(px == 96)
		{
			px = 0;
			py += 8;
		}
	}
}
