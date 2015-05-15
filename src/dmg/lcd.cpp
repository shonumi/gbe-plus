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

	lcd_clock = 0;
	lcd_mode = 0;

	frame_start_time = 0;
	frame_current_time = 0;
	fps_count = 0;
	fps_time = 0;

	current_scanline = 0;
	scanline_pixel_counter = 0;

	screen_buffer.resize(0x5A00, 0);
	scanline_buffer.resize(0x100, 0);

	//Initialize various LCD status variables
	lcd_stat->lcd_control = 0;
	lcd_stat->lcd_enable = 0;
	lcd_stat->window_enable = false;
	lcd_stat->obj_enable = false;
	lcd_stat->window_map_addr = 0x9800;
	lcd_stat->bg_map_addr = 0x9800;
	lcd_stat->bg_tile_addr = 0x8800;
	lcd_stat->obj_size = 1;

	lcd_stat->bg_scroll_x = 0;
	lcd_stat->bg_scroll_y = 0;
	lcd_stat->window_x = 0;
	lcd_stat->window_y = 0;

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

/****** Compares LY and LYC - Generates STAT Interrupt ******/
void DMG_LCD::scanline_compare()
{
	if(mem_link->memory_map[REG_LY] == mem_link->memory_map[REG_LYC]) 
	{ 
		mem_link->memory_map[REG_STAT] |= 0x4; 
		if(mem_link->memory_map[REG_STAT] & 0x40) { mem_link->memory_map[REG_IF] |= 2; }
	}
	else { mem_link->memory_map[REG_STAT] &= ~0x4; }
}



