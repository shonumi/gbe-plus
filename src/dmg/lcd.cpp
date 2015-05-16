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
	screen_buffer.clear();
	scanline_buffer.clear();
	std::cout<<"LCD::Shutdown\n";
}

/****** Reset LCD ******/
void DMG_LCD::reset()
{
	final_screen = NULL;
	mem = NULL;

	scanline_buffer.clear();
	screen_buffer.clear();

	frame_start_time = 0;
	frame_current_time = 0;
	fps_count = 0;
	fps_time = 0;

	screen_buffer.resize(0x5A00, 0);
	scanline_buffer.resize(0x100, 0);

	//Initialize various LCD status variables
	lcd_stat.lcd_control = 0;
	lcd_stat.lcd_enable = 0;
	lcd_stat.window_enable = false;
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

	lcd_stat.oam_update = false;
	lcd_stat.oam_update_list.resize(64, false);

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
	else { final_screen = SDL_SetVideoMode(240, 160, 32, SDL_SWSURFACE); }

	if(final_screen == NULL) { return false; }

	std::cout<<"LCD::Initialized\n";

	return true;
}

/****** Compares LY and LYC - Generates STAT interrupt ******/
void DMG_LCD::scanline_compare()
{
	if(mem->memory_map[REG_LY] == mem->memory_map[REG_LYC]) 
	{ 
		mem->memory_map[REG_STAT] |= 0x4; 
		if(mem->memory_map[REG_STAT] & 0x40) { mem->memory_map[REG_IF] |= 2; }
	}
	else { mem->memory_map[REG_STAT] &= ~0x4; }
}

/****** Render pixels for a given scanline (per-scanline) - DMG version ******/
void DMG_LCD::render_dmg_scanline() { }

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

	//Perform LCD operations only when LCD is enabled
	if(lcd_stat.lcd_enable)
	{
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
					if(mem->memory_map[REG_STAT] & 0x20) { mem->memory_map[REG_IF] |= 2; }
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

					//Render scanline when first entering Mode 0
					render_dmg_scanline();

					//HBlank STAT INT
					if(mem->memory_map[REG_STAT] & 0x08) { mem->memory_map[REG_IF] |= 2; }
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
				lcd_stat.vblank_clock = (lcd_stat.lcd_clock - 65664);
					
				//VBlank STAT INT
				if(mem->memory_map[REG_STAT] & 0x10) { mem->memory_map[REG_IF] |= 2; }

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
					if(lcd_stat.current_scanline == 154) 
					{ 
						lcd_stat.lcd_clock -= 70224;
						lcd_stat.current_scanline = 0;

						mem->memory_map[REG_LY] = lcd_stat.current_scanline;
						scanline_compare();
					}
				}
			}
		}
	}

	mem->memory_map[REG_STAT] = (mem->memory_map[REG_STAT] & ~0x3) | lcd_stat.lcd_mode;
}


