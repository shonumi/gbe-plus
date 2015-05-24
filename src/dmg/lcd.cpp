// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd.h
// Date : May 15, 2014
// Description : Game Boy LCD emulation
//
// Draws background, window, and sprites to screen
// Responsible for blitting pixel data and limiting frame rate

#include "lcd.h"

/****** LCD Constructor ******/
DMG_LCD::DMG_LCD()
{
	reset();
}

/****** LCD Destructor ******/
DMG_LCD::~DMG_LCD()
{
	std::cout<<"LCD::Shutdown\n";
}

/****** Reset LCD ******/
void DMG_LCD::reset()
{
	final_screen = NULL;
	mem = NULL;

	frame_start_time = 0;
	frame_current_time = 0;
	fps_count = 0;
	fps_time = 0;

	//Initialize various LCD status variables
	lcd_stat.lcd_control = 0;
	lcd_stat.lcd_enable = 0;
	lcd_stat.window_enable = false;
	lcd_stat.bg_enable = false;
	lcd_stat.obj_enable = false;
	lcd_stat.window_map_addr = 0x9800;
	lcd_stat.bg_map_addr = 0x9800;
	lcd_stat.bg_tile_addr = 0x8800;
	lcd_stat.obj_size = 1;

	lcd_stat.lcd_mode = 2;
	lcd_stat.lcd_clock = 0;
	lcd_stat.vblank_clock = 0;

	lcd_stat.current_scanline = 0;
	lcd_stat.scanline_pixel_counter = 0;

	lcd_stat.bg_scroll_x = 0;
	lcd_stat.bg_scroll_y = 0;
	lcd_stat.window_x = 0;
	lcd_stat.window_y = 0;

	lcd_stat.oam_update = true;
	lcd_stat.oam_update_list.resize(40, true);

	//Signed tile lookup generation
	for(int x = 0; x < 256; x++)
	{
		u8 tile_number = x;

		if(tile_number <= 127)
		{
			tile_number += 128;
			lcd_stat.signed_tile_lut[x] = tile_number;
		}

		else 
		{ 
			tile_number -= 128;
			lcd_stat.signed_tile_lut[x] = tile_number;
		}
	}
}

/****** Initialize LCD with SDL ******/
bool DMG_LCD::init()
{
	if(SDL_Init(SDL_INIT_EVERYTHING) == -1)
	{
		std::cout<<"LCD::Error - Could not initialize SDL\n";
		return false;
	}

	if(config::use_opengl) { std::cout<<"LCD::Error - OpenGL not implemented yet\n"; return false; }
	else { final_screen = SDL_SetVideoMode(160, 144, 32, SDL_SWSURFACE); }

	if(final_screen == NULL) { return false; }

	std::cout<<"LCD::Initialized\n";

	return true;
}

/****** Initialize LCD with OpenGL ******/
bool DMG_LCD::opengl_init() { }

/****** Compares LY and LYC - Generates STAT interrupt ******/
void DMG_LCD::scanline_compare()
{
	if(mem->memory_map[REG_LY] == mem->memory_map[REG_LYC]) 
	{ 
		mem->memory_map[REG_STAT] |= 0x4; 
		if(mem->memory_map[REG_STAT] & 0x40) { mem->memory_map[IF_FLAG] |= 2; }
	}
	else { mem->memory_map[REG_STAT] &= ~0x4; }
}

/****** Updates OAM entries when values in memory change ******/
void DMG_LCD::update_oam()
{
	lcd_stat.oam_update = false;

	u16 oam_ptr = 0xFE00;
	u8 attribute = 0;

	for(int x = 0; x < 40; x++)
	{
		//Update if OAM entry has changed
		if(lcd_stat.oam_update_list[x])
		{
			lcd_stat.oam_update_list[x] = false;

			obj[x].height = 8;

			//Read and parse Attribute 0
			attribute = mem->memory_map[oam_ptr++];
			obj[x].y = (attribute - 16);

			//Read and parse Attribute 1
			attribute = mem->memory_map[oam_ptr++];
			obj[x].x = (attribute - 8);

			//Read and parse Attribute 2
			obj[x].tile_number = mem->memory_map[oam_ptr++];

			//Read and parse Attribute 3
			attribute = mem->memory_map[oam_ptr++];
			obj[x].palette_number = (attribute & 0x10) ? 1 : 0;
			obj[x].h_flip = (attribute & 0x20) ? true : false;
			obj[x].v_flip = (attribute & 0x40) ? true : false;
			obj[x].bg_priority = (attribute & 0x80) ? 1 : 0;
		}

		else { oam_ptr+= 4; }
	}	

	//Update render list for the current scanline
	update_obj_render_list();	
}

/****** Updates a list of OBJs to render on the current scanline ******/
void DMG_LCD::update_obj_render_list()
{
	obj_render_length = -1;
	u8 scanline_pixel = 0;

	u8 obj_x_sort[40];
	u8 obj_sort_length = 0;

	//Cycle through all of the sprites
	for(int x = 0; x < 40; x++)
	{
		//Check to see if sprite is rendered on the current scanline
		if((lcd_stat.current_scanline >= obj[x].y) && (lcd_stat.current_scanline < (obj[x].y + 8)))
		{
			obj_x_sort[obj_sort_length++] = x;
		}
	}

	//Sort them based on X coordinate
	for(int scanline_pixel = 0; scanline_pixel < 256; scanline_pixel++)
	{
		for(int x = 0; x < obj_sort_length; x++)
		{
			u8 sprite_id = obj_x_sort[x];

			if(obj[sprite_id].x == scanline_pixel) 
			{
				obj_render_length++;
				obj_render_list[obj_render_length] = sprite_id; 
			}

			//Enforce 8 sprite-per-scanline limit
			if(obj_render_length == 7) { return; }
		}
	}
}

/****** Render pixels for a given scanline (per-scanline) - DMG version ******/
void DMG_LCD::render_dmg_scanline() 
{
	//Draw background pixel data
	if(lcd_stat.bg_enable) { render_dmg_bg_scanline(); }

	//Draw sprite pixel data
	if(lcd_stat.obj_enable) { render_dmg_sprite_scanline(); }

	//Push scanline buffer to screen buffer
	for(int x = 0; x < 160; x++)
	{
		screen_buffer[(160 * lcd_stat.current_scanline) + x] = scanline_buffer[x];
	}
}

/****** Renders pixels for the BG (per-scanline) - DMG version ******/
void DMG_LCD::render_dmg_bg_scanline()
{
	//Determine where to start drawing
	u8 rendered_scanline = lcd_stat.current_scanline + mem->memory_map[REG_SY];
	lcd_stat.scanline_pixel_counter = (0x100 - lcd_stat.bg_scroll_x);

	//Determine which tiles we should generate to get the scanline data - integer division ftw :p
	u16 tile_lower_range = (rendered_scanline / 8) * 32;
	u16 tile_upper_range = tile_lower_range + 32;

	//Determine which line of the tiles to generate pixels for this scanline
	u8 tile_line = rendered_scanline % 8;

	//Generate background pixel data for selected tiles
	for(int x = tile_lower_range; x < tile_upper_range; x++)
	{
		u8 map_entry = mem->read_u8(lcd_stat.bg_map_addr + x);
		u8 tile_pixel = 0;

		//Convert tile number to signed if necessary
		if(lcd_stat.bg_tile_addr == 0x8800) { map_entry = lcd_stat.signed_tile_lut[map_entry]; }

		//Calculate the address of the 8x1 pixel data based on map entry
		u16 tile_addr = (lcd_stat.bg_tile_addr + (map_entry << 4) + (tile_line << 1));

		//Grab bytes from VRAM representing 8x1 pixel data
		u16 tile_data = mem->read_u16(tile_addr);

		for(int y = 7; y >= 0; y--)
		{
			//Calculate raw value of the tile's pixel
			tile_pixel = ((tile_data >> 8) & (1 << y)) ? 2 : 0;
			tile_pixel |= (tile_data & (1 << y)) ? 1 : 0;

			//Set the raw color of the BG
			scanline_raw[lcd_stat.scanline_pixel_counter] = tile_pixel;
				
			switch(lcd_stat.bgp[tile_pixel])
			{
				case 0: 
					scanline_buffer[lcd_stat.scanline_pixel_counter++] = config::DMG_BG_PAL[0];
					break;

				case 1: 
					scanline_buffer[lcd_stat.scanline_pixel_counter++] = config::DMG_BG_PAL[1];
					break;

				case 2: 
					scanline_buffer[lcd_stat.scanline_pixel_counter++] = config::DMG_BG_PAL[2];
					break;

				case 3: 
					scanline_buffer[lcd_stat.scanline_pixel_counter++] = config::DMG_BG_PAL[3];
					break;
			}
		}
	}
}

/****** Renders pixels for OBJs (per-scanline) - DMG version ******/
void DMG_LCD::render_dmg_sprite_scanline()
{
	//If no sprites are rendered on this line, quit now
	if(obj_render_length < 0) { return; }

	u8 sprite_id = 0;
	u16 sprite_tile_addr = 0;
	u8 raw_color = 0;
	u8 sprite_count = 0;

	//Cycle through all sprites that are rendering on this pixel, draw them according to their priority
	for(int x = obj_render_length; x >= 0; x--)
	{
		u8 sprite_id = obj_render_list[x];

		//Set the current pixel to start obj rendering
		lcd_stat.scanline_pixel_counter = obj[sprite_id].x;
		
		//Determine which line of the tiles to generate pixels for this scanline
		u8 tile_line = (lcd_stat.current_scanline - obj[sprite_id].y);
		u8 tile_pixel = 0;

		//Calculate the address of the 8x1 pixel data based on map entry
		u16 tile_addr = (0x8000 + (obj[sprite_id].tile_number << 4) + (tile_line << 1));

		//Grab bytes from VRAM representing 8x1 pixel data
		u16 tile_data = mem->read_u16(tile_addr);

		for(int y = 7; y >= 0; y--)
		{
			bool draw_obj_pixel = true;

			//Calculate raw value of the tile's pixel
			tile_pixel = ((tile_data >> 8) & (1 << y)) ? 2 : 0;
			tile_pixel |= (tile_data & (1 << y)) ? 1 : 0;

			//Only render sprite if it's displayed at all
			if(obj[sprite_id].x >= 160) { draw_obj_pixel = false; }

			//If raw color is zero, this is the sprite's transparency, abort rendering this pixel
			else if(tile_pixel == 0) { draw_obj_pixel = false; }

			//If sprite is below BG and BG raw color is non-zero, abort rendering this pixel
			else if((obj[sprite_id].bg_priority == 1) && (scanline_raw[lcd_stat.scanline_pixel_counter] != 0)) { draw_obj_pixel = false; }
				
			//Render sprite pixel
			if(draw_obj_pixel)
			{
				switch(lcd_stat.obp[tile_pixel][obj[sprite_id].palette_number])
				{
					case 0: 
						scanline_buffer[lcd_stat.scanline_pixel_counter++] = config::DMG_BG_PAL[0];
						break;

					case 1: 
						scanline_buffer[lcd_stat.scanline_pixel_counter++] = config::DMG_BG_PAL[1];
						break;

					case 2: 
						scanline_buffer[lcd_stat.scanline_pixel_counter++] = config::DMG_BG_PAL[2];
						break;

					case 3: 
						scanline_buffer[lcd_stat.scanline_pixel_counter++] = config::DMG_BG_PAL[3];
						break;
				}
			}

			//Move onto next pixel in scanline to see if sprite rendering occurs
			else { lcd_stat.scanline_pixel_counter++; }

			//Stop if rendering goes offscreen
			if(lcd_stat.scanline_pixel_counter >= 160) { y = 8; break; }
		}
	}
}

/****** Execute LCD operations ******/
void DMG_LCD::step(int cpu_clock) 
{
	/*if(mem->gpu_reset_ticks) { gpu_clock = 0; mem->gpu_reset_ticks = false; }

	//Enable LCD
	if((mem->memory_map[REG_LCDC] & 0x80) && (!lcd_enabled)) 
	{ 
		lcd_enabled = true;
		gpu_mode = 2;
	}
 
	//Update background tile
	if(mem->gpu_update_bg_tile)
	{
		if(config::gb_type != 2) { update_bg_tile(); }
		else { update_gbc_bg_tile(); }
		mem->gpu_update_bg_tile = false;
	}

	//Update sprites
	if(mem->gpu_update_sprite)
	{
		generate_sprites();
		mem->gpu_update_sprite = false;
	}

	*/


	lcd_stat.lcd_clock += cpu_clock;

	//Modes 0, 2, and 3 - Outside of VBlank
	if(lcd_stat.lcd_clock < 65664)
	{
		//Mode 2 - OAM Read
		if((lcd_stat.lcd_clock % 456) < 80)
		{
			if(lcd_stat.lcd_mode != 2)
			{
				//Increment scanline when entering Mode 2, signifies the end of HBlank
				//When coming from VBlank, this must part must be ignored
				if(lcd_stat.lcd_mode != 1)
				{
					lcd_stat.current_scanline++;
					mem->memory_map[REG_LY] = lcd_stat.current_scanline;
					scanline_compare();
				}

				lcd_stat.lcd_mode = 2;

				//OAM STAT INT
				if(mem->memory_map[REG_STAT] & 0x20) { mem->memory_map[IF_FLAG] |= 2; }
			}
		}

		//Mode 3 - VRAM Read
		else if((lcd_stat.lcd_clock % 456) < 252)
		{
			if(lcd_stat.lcd_mode != 3) { lcd_stat.lcd_mode = 3; }
		}

		//Mode 0 - HBlank
		else
		{
			if(lcd_stat.lcd_mode != 0)
			{
				lcd_stat.lcd_mode = 0;

				//Update OAM
				if(lcd_stat.oam_update) { update_oam(); }
				else { update_obj_render_list(); }

				//Render scanline when first entering Mode 0
				render_dmg_scanline();

				//HBlank STAT INT
				if(mem->memory_map[REG_STAT] & 0x08) { mem->memory_map[IF_FLAG] |= 2; }
			}
		}
	}

	//Mode 1 - VBlank
	else
	{
		//Entering VBlank
		if(lcd_stat.lcd_mode != 1)
		{
			lcd_stat.lcd_mode = 1;

			//Setup the VBlank clock to count 10 scanlines
			lcd_stat.vblank_clock = lcd_stat.lcd_clock - 65664;
					
			//VBlank STAT INT
			if(mem->memory_map[REG_STAT] & 0x10) { mem->memory_map[IF_FLAG] |= 2; }

			//VBlank INT
			mem->memory_map[IF_FLAG] |= 1;

			//Render final screen buffer
			if(lcd_stat.lcd_enable)
			{
				//Lock source surface
				if(SDL_MUSTLOCK(final_screen)){ SDL_LockSurface(final_screen); }
				u32* out_pixel_data = (u32*)final_screen->pixels;

				for(int a = 0; a < 0x5A00; a++) { out_pixel_data[a] = screen_buffer[a]; }

				//Unlock source surface
				if(SDL_MUSTLOCK(final_screen)){ SDL_UnlockSurface(final_screen); }
		
				//Display final screen buffer - OpenGL
				if(config::use_opengl) {  }
				
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

		//Processing VBlank
		else
		{
			lcd_stat.vblank_clock += cpu_clock;

			//Increment scanline count
			if(lcd_stat.vblank_clock >= 456)
			{
				lcd_stat.vblank_clock -= 456;
				lcd_stat.current_scanline++;

				mem->memory_map[REG_LY] = lcd_stat.current_scanline;
				scanline_compare();

				//After 10 lines, VBlank is done, returns to top screen in Mode 2
				if(lcd_stat.current_scanline == 153) 
				{
					lcd_stat.lcd_clock -= 70224;
					lcd_stat.current_scanline = 0;

					mem->memory_map[REG_LY] = lcd_stat.current_scanline;
					scanline_compare();
				}
			}
		}
	}

	mem->memory_map[REG_STAT] = (mem->memory_map[REG_STAT] & ~0x3) | lcd_stat.lcd_mode;
}


