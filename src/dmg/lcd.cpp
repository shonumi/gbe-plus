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
	lcd_stat.lcd_enable = true;
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

	lcd_stat.on_off = false;

	lcd_stat.update_bg_colors = false;
	lcd_stat.update_obj_colors = false;
	lcd_stat.hdma_in_progress = false;
	lcd_stat.hdma_current_line = 0;
	lcd_stat.hdma_type = 0;

	//Clear GBC color palettes
	for(int x = 0; x < 4; x++)
	{
		for(int y =  0; y < 8; y++)
		{
			lcd_stat.obj_colors_raw[x][y] = 0;
			lcd_stat.obj_colors_final[x][y] = 0;
			lcd_stat.bg_colors_raw[x][y] = 0;
			lcd_stat.bg_colors_final[x][y] = 0;
		}
	}

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

	//8 pixel (horizontal+vertical) flipping lookup generation
	for(int x = 0; x < 8; x++) { lcd_stat.flip_8[x] = (7 - x); }

	//16 pixel (vertical) flipping lookup generation
        for(int x = 0; x < 16; x++) { lcd_stat.flip_16[x] = (15 - x); }
}

/****** Initialize LCD with SDL ******/
bool DMG_LCD::init()
{
	if(SDL_Init(SDL_INIT_EVERYTHING) == -1)
	{
		std::cout<<"LCD::Error - Could not initialize SDL\n";
		return false;
	}

	if(config::use_opengl) {opengl_init(); }
	else { final_screen = SDL_SetVideoMode(160, 144, 32, SDL_SWSURFACE); }

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
			if(lcd_stat.obj_size == 16) { obj[x].tile_number &= ~0x1; }

			//Read and parse Attribute 3
			attribute = mem->memory_map[oam_ptr++];
			obj[x].color_palette_number = (attribute & 0x7);
			obj[x].vram_bank = (attribute & 0x8) ? 1 : 0;
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

	//Update render list for DMG games
	if(config::gb_type != 2)
	{
		//Cycle through all of the sprites
		for(int x = 0; x < 40; x++)
		{
			u8 test_top = ((obj[x].y + lcd_stat.obj_size) > 0x100) ? 0 : obj[x].y;
			u8 test_bottom = (obj[x].y + lcd_stat.obj_size);

			//Check to see if sprite is rendered on the current scanline
			if((lcd_stat.current_scanline >= test_top) && (lcd_stat.current_scanline < test_bottom))
			{
				obj_x_sort[obj_sort_length++] = x;
			}

			if(obj_sort_length == 10) { break; }
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

				//Enforce 10 sprite-per-scanline limit
				if(obj_render_length == 9) { return; }
			}
		}
	}

	//Update render list for GBC games
	else
	{
		//Cycle through all of the sprites
		for(int x = 0; x < 40; x++)
		{
			u8 test_top = ((obj[x].y + lcd_stat.obj_size) > 0x100) ? 0 : obj[x].y;
			u8 test_bottom = (obj[x].y + lcd_stat.obj_size);

			//Check to see if sprite is rendered on the current scanline
			if((lcd_stat.current_scanline >= test_top) && (lcd_stat.current_scanline < test_bottom))
			{
				obj_render_length++;
				obj_render_list[obj_render_length] = x; 
			}

			//Enforce 10 sprite-per-scanline limit
			if(obj_render_length == 9) { break; }
		}
	}
}

/****** Render pixels for a given scanline (per-scanline) - DMG version ******/
void DMG_LCD::render_dmg_scanline() 
{
	//Draw background pixel data
	if(lcd_stat.bg_enable) { render_dmg_bg_scanline(); }

	//Draw window pixel data
	if(lcd_stat.window_enable) { render_dmg_win_scanline(); }

	//Draw sprite pixel data
	if(lcd_stat.obj_enable) { render_dmg_obj_scanline(); }

	//Push scanline buffer to screen buffer
	for(int x = 0; x < 160; x++)
	{
		screen_buffer[(160 * lcd_stat.current_scanline) + x] = scanline_buffer[x];
	}
}

/****** Render pixels for a given scanline (per-scanline) - DMG version ******/
void DMG_LCD::render_gbc_scanline() 
{
	//Draw background pixel data
	if(lcd_stat.bg_enable) { render_gbc_bg_scanline(); }

	//Draw window pixel data
	if(lcd_stat.window_enable) { render_gbc_win_scanline(); }

	//Draw sprite pixel data
	if(lcd_stat.obj_enable) { render_gbc_obj_scanline(); }

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
	u8 rendered_scanline = lcd_stat.current_scanline + lcd_stat.bg_scroll_y;
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

/****** Renders pixels for the BG (per-scanline) - GBC version ******/
void DMG_LCD::render_gbc_bg_scanline()
{
	//Determine where to start drawing
	u8 rendered_scanline = lcd_stat.current_scanline + lcd_stat.bg_scroll_y;
	lcd_stat.scanline_pixel_counter = (0x100 - lcd_stat.bg_scroll_x);

	//Determine which tiles we should generate to get the scanline data - integer division ftw :p
	u16 tile_lower_range = (rendered_scanline / 8) * 32;
	u16 tile_upper_range = tile_lower_range + 32;

	//Generate background pixel data for selected tiles
	for(int x = tile_lower_range; x < tile_upper_range; x++)
	{
		//Always read CHR data from Bank 0
		u8 old_vram_bank = mem->vram_bank;
		mem->vram_bank = 0;

		u8 map_entry = mem->read_u8(lcd_stat.bg_map_addr + x);
		u8 tile_pixel = 0;

		//Read BG Map attributes from Bank 1
		mem->vram_bank = 1;
		u8 bg_map_attribute = mem->read_u8(lcd_stat.bg_map_addr + x);
		u8 bg_palette = bg_map_attribute & 0x7;
		u8 bg_priority = (bg_map_attribute & 0x80) ? 1 : 0;
		mem->vram_bank = (bg_map_attribute & 0x8) ? 1 : 0;

		//Determine which line of the tiles to generate pixels for this scanline
		u8 tile_line = rendered_scanline % 8;
		if(bg_map_attribute & 0x40) { tile_line = lcd_stat.flip_8[tile_line]; }

		//Convert tile number to signed if necessary
		if(lcd_stat.bg_tile_addr == 0x8800) { map_entry = lcd_stat.signed_tile_lut[map_entry]; }

		//Calculate the address of the 8x1 pixel data based on map entry
		u16 tile_addr = (lcd_stat.bg_tile_addr + (map_entry << 4) + (tile_line << 1));

		//Grab bytes from VRAM representing 8x1 pixel data
		u16 tile_data = mem->read_u16(tile_addr);
		mem->vram_bank = old_vram_bank;

		for(int y = 7; y >= 0; y--)
		{
			//Calculate raw value of the tile's pixel
			if(bg_map_attribute & 0x20) 
			{
				tile_pixel = ((tile_data >> 8) & (1 << lcd_stat.flip_8[y])) ? 2 : 0;
				tile_pixel |= (tile_data & (1 << lcd_stat.flip_8[y])) ? 1 : 0;
			}

			else 
			{
				tile_pixel = ((tile_data >> 8) & (1 << y)) ? 2 : 0;
				tile_pixel |= (tile_data & (1 << y)) ? 1 : 0;
			}

			//Set the raw color of the BG
			scanline_raw[lcd_stat.scanline_pixel_counter] = tile_pixel;

			//Set the BG-to-OBJ priority
			scanline_priority[lcd_stat.scanline_pixel_counter] = bg_priority;

			//Set the final color of the BG
			scanline_buffer[lcd_stat.scanline_pixel_counter++] = lcd_stat.bg_colors_final[tile_pixel][bg_palette];
		}
	}
}

/****** Renders pixels for the Window (per-scanline) - DMG version ******/
void DMG_LCD::render_dmg_win_scanline()
{
	//Determine if scanline is within window, if not abort rendering
	if((lcd_stat.current_scanline < lcd_stat.window_y) || (lcd_stat.window_x >= 160)) { return; }

	//Determine where to start drawing
	u8 rendered_scanline = lcd_stat.current_scanline - lcd_stat.window_y;
	lcd_stat.scanline_pixel_counter = lcd_stat.window_x;

	//Determine which tiles we should generate to get the scanline data - integer division ftw :p
	u16 tile_lower_range = (rendered_scanline / 8) * 32;
	u16 tile_upper_range = tile_lower_range + 32;

	//Determine which line of the tiles to generate pixels for this scanline
	u8 tile_line = rendered_scanline % 8;

	//Generate background pixel data for selected tiles
	for(int x = tile_lower_range; x < tile_upper_range; x++)
	{
		u8 map_entry = mem->read_u8(lcd_stat.window_map_addr + x);
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

			//Abort rendering if next pixel is off-screen
			if(lcd_stat.scanline_pixel_counter == 160) { return; }
		}
	}
}

/****** Renders pixels for the Window (per-scanline) - GBC version ******/
void DMG_LCD::render_gbc_win_scanline()
{
	//Determine if scanline is within window, if not abort rendering
	if((lcd_stat.current_scanline < lcd_stat.window_y) || (lcd_stat.window_x >= 160)) { return; }

	//Determine where to start drawing
	u8 rendered_scanline = lcd_stat.current_scanline - lcd_stat.window_y;
	lcd_stat.scanline_pixel_counter = lcd_stat.window_x;

	//Determine which tiles we should generate to get the scanline data - integer division ftw :p
	u16 tile_lower_range = (rendered_scanline / 8) * 32;
	u16 tile_upper_range = tile_lower_range + 32;

	//Generate background pixel data for selected tiles
	for(int x = tile_lower_range; x < tile_upper_range; x++)
	{
		//Always read CHR data from Bank 0
		u8 old_vram_bank = mem->vram_bank;
		mem->vram_bank = 0;

		u8 map_entry = mem->read_u8(lcd_stat.window_map_addr + x);
		u8 tile_pixel = 0;

		//Read BG Map attributes from Bank 1
		mem->vram_bank = 1;
		u8 bg_map_attribute = mem->read_u8(lcd_stat.window_map_addr + x);
		u8 bg_palette = bg_map_attribute & 0x7;
		u8 bg_priority = (bg_map_attribute & 0x80) ? 1 : 0;
		mem->vram_bank = (bg_map_attribute & 0x8) ? 1 : 0;

		//Determine which line of the tiles to generate pixels for this scanline
		u8 tile_line = rendered_scanline % 8;
		if(bg_map_attribute & 0x40) { tile_line = lcd_stat.flip_8[tile_line]; }

		//Convert tile number to signed if necessary
		if(lcd_stat.bg_tile_addr == 0x8800) { map_entry = lcd_stat.signed_tile_lut[map_entry]; }

		//Calculate the address of the 8x1 pixel data based on map entry
		u16 tile_addr = (lcd_stat.bg_tile_addr + (map_entry << 4) + (tile_line << 1));

		//Grab bytes from VRAM representing 8x1 pixel data
		u16 tile_data = mem->read_u16(tile_addr);
		mem->vram_bank = old_vram_bank;

		for(int y = 7; y >= 0; y--)
		{
			//Calculate raw value of the tile's pixel
			if(bg_map_attribute & 0x20) 
			{
				tile_pixel = ((tile_data >> 8) & (1 << lcd_stat.flip_8[y])) ? 2 : 0;
				tile_pixel |= (tile_data & (1 << lcd_stat.flip_8[y])) ? 1 : 0;
			}

			else 
			{
				tile_pixel = ((tile_data >> 8) & (1 << y)) ? 2 : 0;
				tile_pixel |= (tile_data & (1 << y)) ? 1 : 0;
			}

			//Set the raw color of the BG
			scanline_raw[lcd_stat.scanline_pixel_counter] = tile_pixel;

			//Set the BG-to-OBJ priority
			scanline_priority[lcd_stat.scanline_pixel_counter] = bg_priority;

			//Set the final color of the BG
			scanline_buffer[lcd_stat.scanline_pixel_counter++] = lcd_stat.bg_colors_final[tile_pixel][bg_palette];

			//Abort rendering if next pixel is off-screen
			if(lcd_stat.scanline_pixel_counter == 160) { return; }
		}
	}
}

/****** Renders pixels for OBJs (per-scanline) - DMG version ******/
void DMG_LCD::render_dmg_obj_scanline()
{
	//If no sprites are rendered on this line, quit now
	if(obj_render_length < 0) { return; }

	//Cycle through all sprites that are rendering on this pixel, draw them according to their priority
	for(int x = obj_render_length; x >= 0; x--)
	{
		u8 sprite_id = obj_render_list[x];

		//Set the current pixel to start obj rendering
		lcd_stat.scanline_pixel_counter = obj[sprite_id].x;
		
		//Determine which line of the tiles to generate pixels for this scanline		
		u8 tile_line = (lcd_stat.current_scanline - obj[sprite_id].y);
		if(obj[sprite_id].v_flip) { tile_line = (lcd_stat.obj_size == 8) ? lcd_stat.flip_8[tile_line] : lcd_stat.flip_16[tile_line]; }

		u8 tile_pixel = 0;

		//Calculate the address of the 8x1 pixel data based on map entry
		u16 tile_addr = (0x8000 + (obj[sprite_id].tile_number << 4) + (tile_line << 1));

		//Grab bytes from VRAM representing 8x1 pixel data
		u16 tile_data = mem->read_u16(tile_addr);

		for(int y = 7; y >= 0; y--)
		{
			bool draw_obj_pixel = true;

			//Calculate raw value of the tile's pixel
			if(obj[sprite_id].h_flip) 
			{
				tile_pixel = ((tile_data >> 8) & (1 << lcd_stat.flip_8[y])) ? 2 : 0;
				tile_pixel |= (tile_data & (1 << lcd_stat.flip_8[y])) ? 1 : 0;
			}

			else 
			{
				tile_pixel = ((tile_data >> 8) & (1 << y)) ? 2 : 0;
				tile_pixel |= (tile_data & (1 << y)) ? 1 : 0;
			}

			//If raw color is zero, this is the sprite's transparency, abort rendering this pixel
			if(tile_pixel == 0) { draw_obj_pixel = false; }

			//If sprite is below BG and BG raw color is non-zero, abort rendering this pixel
			else if((obj[sprite_id].bg_priority == 1) && (scanline_raw[lcd_stat.scanline_pixel_counter] != 0)) { draw_obj_pixel = false; }
				
			//Render sprite pixel
			if(draw_obj_pixel)
			{
				switch(lcd_stat.obp[tile_pixel][obj[sprite_id].palette_number])
				{
					case 0: 
						scanline_buffer[lcd_stat.scanline_pixel_counter++] = config::DMG_OBJ_PAL[0][obj[sprite_id].palette_number];
						break;

					case 1: 
						scanline_buffer[lcd_stat.scanline_pixel_counter++] = config::DMG_OBJ_PAL[1][obj[sprite_id].palette_number];
						break;

					case 2: 
						scanline_buffer[lcd_stat.scanline_pixel_counter++] = config::DMG_OBJ_PAL[2][obj[sprite_id].palette_number];
						break;

					case 3: 
						scanline_buffer[lcd_stat.scanline_pixel_counter++] = config::DMG_OBJ_PAL[3][obj[sprite_id].palette_number];
						break;
				}
			}

			//Move onto next pixel in scanline to see if sprite rendering occurs
			else { lcd_stat.scanline_pixel_counter++; }
		}
	}
}

/****** Renders pixels for OBJs (per-scanline) - GBC version ******/
void DMG_LCD::render_gbc_obj_scanline()
{
	//If no sprites are rendered on this line, quit now
	if(obj_render_length < 0) { return; }

	//Cycle through all sprites that are rendering on this pixel, draw them according to their priority
	for(int x = obj_render_length; x >= 0; x--)
	{
		u8 sprite_id = obj_render_list[x];

		//Set the current pixel to start obj rendering
		lcd_stat.scanline_pixel_counter = obj[sprite_id].x;
		
		//Determine which line of the tiles to generate pixels for this scanline		
		u8 tile_line = (lcd_stat.current_scanline - obj[sprite_id].y);
		if(obj[sprite_id].v_flip) { tile_line = (lcd_stat.obj_size == 8) ? lcd_stat.flip_8[tile_line] : lcd_stat.flip_16[tile_line]; }

		u8 tile_pixel = 0;

		//Calculate the address of the 8x1 pixel data based on map entry
		u16 tile_addr = (0x8000 + (obj[sprite_id].tile_number << 4) + (tile_line << 1));

		//Grab bytes from VRAM representing 8x1 pixel data
		u8 old_vram_bank = mem->vram_bank;
		mem->vram_bank = obj[sprite_id].vram_bank;
		u16 tile_data = mem->read_u16(tile_addr);
		mem->vram_bank = old_vram_bank;

		for(int y = 7; y >= 0; y--)
		{
			bool draw_obj_pixel = true;

			//Calculate raw value of the tile's pixel
			if(obj[sprite_id].h_flip) 
			{
				tile_pixel = ((tile_data >> 8) & (1 << lcd_stat.flip_8[y])) ? 2 : 0;
				tile_pixel |= (tile_data & (1 << lcd_stat.flip_8[y])) ? 1 : 0;
			}

			else 
			{
				tile_pixel = ((tile_data >> 8) & (1 << y)) ? 2 : 0;
				tile_pixel |= (tile_data & (1 << y)) ? 1 : 0;
			}

			//If Bit 0 of LCDC is clear, always give sprites priority
			if(!lcd_stat.bg_enable) { scanline_priority[lcd_stat.scanline_pixel_counter] = 0; }

			//If raw color is zero, this is the sprite's transparency, abort rendering this pixel
			if(tile_pixel == 0) { draw_obj_pixel = false; }

			//If sprite is below BG and BG raw color is non-zero, abort rendering this pixel
			else if((obj[sprite_id].bg_priority == 1) && (scanline_raw[lcd_stat.scanline_pixel_counter] != 0)) { draw_obj_pixel = false; }

			//If sprite is above BG but BG has priority and BG raw color is non-zero, abort rendering this pixel
			else if((obj[sprite_id].bg_priority == 0) && (scanline_priority[lcd_stat.scanline_pixel_counter] == 1) && (scanline_raw[lcd_stat.scanline_pixel_counter] != 0)) { draw_obj_pixel = false; }
				
			//Render sprite pixel
			if(draw_obj_pixel)
			{
				scanline_buffer[lcd_stat.scanline_pixel_counter++] = lcd_stat.obj_colors_final[tile_pixel][obj[sprite_id].color_palette_number];
			}

			//Move onto next pixel in scanline to see if sprite rendering occurs
			else { lcd_stat.scanline_pixel_counter++; }
		}
	}
}

/****** Update background color palettes on the GBC ******/
void DMG_LCD::update_bg_colors()
{
	u8 hi_lo = (mem->memory_map[REG_BCPS] & 0x1);
	u8 color = (mem->memory_map[REG_BCPS] >> 1) & 0x3;
	u8 palette = (mem->memory_map[REG_BCPS] >> 3) & 0x7;
	u8 auto_increment = (mem->memory_map[REG_BCPS]) & 0x80;

	//Update lower-nibble of color
	if(hi_lo == 0) 
	{ 
		lcd_stat.bg_colors_raw[color][palette] &= 0xFF00;
		lcd_stat.bg_colors_raw[color][palette] |= mem->memory_map[REG_BCPD];
	}

	//Update upper-nibble of color
	else
	{
		lcd_stat.bg_colors_raw[color][palette] &= 0xFF;
		lcd_stat.bg_colors_raw[color][palette] |= (mem->memory_map[REG_BCPD] << 8);
	}

	//Auto update palette index
	if(mem->memory_map[REG_BCPS] & 0x80)
	{
		u8 new_index = mem->memory_map[REG_BCPS] & 0x3F;
		new_index = (new_index + 1) & 0x3F;
		mem->memory_map[REG_BCPS] = (0x80 | new_index);
	}

	//Convert RGB5 to 32-bit ARGB
	u16 color_bytes = lcd_stat.bg_colors_raw[color][palette];

	u8 red = ((color_bytes & 0x1F) * 8);
	color_bytes >>= 5;

	u8 green = ((color_bytes & 0x1F) * 8);
	color_bytes >>= 5;

	u8 blue = ((color_bytes & 0x1F) * 8);

	lcd_stat.bg_colors_final[color][palette] = 0xFF000000 | (red << 16) | (green << 8) | (blue);
	lcd_stat.bg_colors_raw[color][palette] = lcd_stat.bg_colors_raw[color][palette];

	//Update DMG BG palette when using GBC BIOS
	if(mem->in_bios)
	{
		config::DMG_BG_PAL[0] = lcd_stat.bg_colors_final[0][0];
		config::DMG_BG_PAL[1] = lcd_stat.bg_colors_final[1][0];
		config::DMG_BG_PAL[2] = lcd_stat.bg_colors_final[2][0];
		config::DMG_BG_PAL[3] = lcd_stat.bg_colors_final[3][0];
	}

	lcd_stat.update_bg_colors = false;
}

/****** Update sprite color palettes on the GBC ******/
void DMG_LCD::update_obj_colors()
{
	u8 hi_lo = (mem->memory_map[REG_OCPS] & 0x1);
	u8 color = (mem->memory_map[REG_OCPS] >> 1) & 0x3;
	u8 palette = (mem->memory_map[REG_OCPS] >> 3) & 0x7;
	u8 auto_increment = (mem->memory_map[REG_OCPS]) & 0x80;

	//Update lower-nibble of color
	if(hi_lo == 0) 
	{ 
		lcd_stat.obj_colors_raw[color][palette] &= 0xFF00;
		lcd_stat.obj_colors_raw[color][palette] |= mem->memory_map[REG_OCPD];
	}

	//Update upper-nibble of color
	else
	{
		lcd_stat.obj_colors_raw[color][palette] &= 0xFF;
		lcd_stat.obj_colors_raw[color][palette] |= (mem->memory_map[REG_OCPD] << 8);
	}

	//Auto update palette index
	if(mem->memory_map[REG_OCPS] & 0x80)
	{
		u8 new_index = mem->memory_map[REG_OCPS] & 0x3F;
		new_index = (new_index + 1) & 0x3F;
		mem->memory_map[REG_OCPS] = (0x80 | new_index);
	}

	//Convert RGB5 to 32-bit ARGB
	u16 color_bytes = lcd_stat.obj_colors_raw[color][palette];

	u8 red = ((color_bytes & 0x1F) * 8);
	color_bytes >>= 5;

	u8 green = ((color_bytes & 0x1F) * 8);
	color_bytes >>= 5;

	u8 blue = ((color_bytes & 0x1F) * 8);

	lcd_stat.obj_colors_final[color][palette] = 0xFF000000 | (red << 16) | (green << 8) | (blue);
	lcd_stat.obj_colors_raw[color][palette] = lcd_stat.obj_colors_raw[color][palette];

	//Update DMG OBJ palettes when using GBC BIOS
	if(mem->in_bios)
	{
		config::DMG_OBJ_PAL[0][0] = lcd_stat.obj_colors_final[0][0];
		config::DMG_OBJ_PAL[1][0] = lcd_stat.obj_colors_final[1][0];
		config::DMG_OBJ_PAL[2][0] = lcd_stat.obj_colors_final[2][0];
		config::DMG_OBJ_PAL[3][0] = lcd_stat.obj_colors_final[3][0];

		config::DMG_OBJ_PAL[0][1] = lcd_stat.obj_colors_final[0][1];
		config::DMG_OBJ_PAL[1][1] = lcd_stat.obj_colors_final[1][1];
		config::DMG_OBJ_PAL[2][1] = lcd_stat.obj_colors_final[2][1];
		config::DMG_OBJ_PAL[3][1] = lcd_stat.obj_colors_final[3][1];
	}

	lcd_stat.update_obj_colors = false;
}

/****** GBC General Purpose DMA ******/
void DMG_LCD::gdma()
{
	u16 start_addr = (mem->memory_map[REG_HDMA1] << 8) | mem->memory_map[REG_HDMA2];
	u16 dest_addr = (mem->memory_map[REG_HDMA3] << 8) | mem->memory_map[REG_HDMA4];

	//Ignore bottom 4 bits of start address
	start_addr &= 0xFFF0;

	//Ignore top 3 bits and bottom 4 bits of destination address
	dest_addr &= 0x1FF0;

	//Destination is ALWAYS in VRAM
	dest_addr |= 0x8000;

	u8 transfer_byte_count = (mem->memory_map[REG_HDMA5] & 0x7F) + 1;

	for(u16 x = 0; x < (transfer_byte_count * 16); x++)
	{
		mem->write_u8(dest_addr++, mem->read_u8(start_addr++));
	}

	lcd_stat.hdma_in_progress = false;
	mem->memory_map[REG_HDMA5] = 0xFF;
}

/****** GBC Horizontal DMA ******/
void DMG_LCD::hdma()
{
	u16 start_addr = (mem->memory_map[REG_HDMA1] << 8) | mem->memory_map[REG_HDMA2];
	u16 dest_addr = (mem->memory_map[REG_HDMA3] << 8) | mem->memory_map[REG_HDMA4];
	u8 line_transfer_count = (mem->memory_map[REG_HDMA5] & 0x7F);

	start_addr += (lcd_stat.hdma_current_line * 16);
	dest_addr += (lcd_stat.hdma_current_line * 16);

	//Ignore bottom 4 bits of start address
	start_addr &= 0xFFF0;

	//Ignore top 3 bits and bottom 4 bits of destination address
	dest_addr &= 0x1FF0;

	//Destination is ALWAYS in VRAM
	dest_addr |= 0x8000;

	for(u16 x = 0; x < 16; x++)
	{
		mem->write_u8(dest_addr++, mem->read_u8(start_addr++));
	}
							
	lcd_stat.hdma_current_line++;

	if(line_transfer_count == 0) 
	{ 
		lcd_stat.hdma_in_progress = false;
		lcd_stat.hdma_current_line = 0;
		mem->memory_map[REG_HDMA5] = 0xFF;
	}

	else { line_transfer_count--; mem->memory_map[REG_HDMA5] = line_transfer_count; }
}

/****** Execute LCD operations ******/
void DMG_LCD::step(int cpu_clock) 
{
        //Enable the LCD
	if((lcd_stat.on_off) && (lcd_stat.lcd_enable)) 
	{
		lcd_stat.on_off = false;
		lcd_stat.lcd_mode = 2;
	}

	//Disable the LCD (VBlank only?)
	else if((lcd_stat.on_off) && (!lcd_stat.lcd_enable))
	{
		lcd_stat.on_off = false;
	
		//This should only happen in VBlank, but it's possible to do it in other modes
		//On real DMG HW, it creates a black line on the scanline the LCD turns off
		//Nintendo did NOT like this, as it could damage the LCD over repeated uses
		//Note: this is the same effect you see when hitting the power switch OFF
		if(lcd_stat.lcd_mode != 1) { std::cout<<"LCD::Warning - Disabling LCD outside of VBlank\n"; }

		lcd_stat.current_scanline = mem->memory_map[REG_LY] = 0;
		scanline_compare();
		lcd_stat.lcd_clock = 0;
		lcd_stat.lcd_mode = 0;
	}

	//Update background color palettes on the GBC
	if((lcd_stat.update_bg_colors) && (config::gb_type == 2)) { update_bg_colors(); }

	//Update sprite color palettes on the GBC
	if((lcd_stat.update_obj_colors) && (config::gb_type == 2)) { update_obj_colors(); }

	//General Purpose DMA
	if((lcd_stat.hdma_in_progress) && (lcd_stat.hdma_type == 0) && (config::gb_type == 2)) { gdma(); }

	//Perform LCD operations if LCD is enabled
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

					//Horizontal blanking DMA
					if((lcd_stat.hdma_in_progress) && (lcd_stat.hdma_type == 1) && (config::gb_type == 2)) { hdma(); }

					//Update OAM
					if(lcd_stat.oam_update) { update_oam(); }
					else { update_obj_render_list(); }

					//Render scanline when first entering Mode 0
					if(config::gb_type != 2 ) { render_dmg_scanline(); }
					else { render_gbc_scanline(); }

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
	}

	mem->memory_map[REG_STAT] = (mem->memory_map[REG_STAT] & ~0x3) | lcd_stat.lcd_mode;
}
