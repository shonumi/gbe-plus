// GB Enhanced+ Copyright Daniel Baxter 2017
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd.h
// Date : June 17, 2017
// Description : Super Game Boy "LCD" i.e. TV-OUT emulation
//
// Draws background, window, and sprites to screen
// Responsible for blitting pixel data and limiting frame rate

#include "lcd.h"
#include "common/cgfx_common.h"
#include "common/util.h"

/****** LCD Constructor ******/
SGB_LCD::SGB_LCD()
{
	window = NULL;
	reset();
}

/****** LCD Destructor ******/
SGB_LCD::~SGB_LCD()
{
	SDL_DestroyWindow(window);
	std::cout<<"LCD::Shutdown\n";
}

/****** Reset LCD ******/
void SGB_LCD::reset()
{
	final_screen = NULL;
	mem = NULL;

	if((window != NULL) && (config::sdl_render)) { SDL_DestroyWindow(window); }
	window = NULL;

	screen_buffer.clear();
	scanline_buffer.clear();
	scanline_raw.clear();
	scanline_priority.clear();

	screen_buffer.resize(0x5A00, 0);
	scanline_buffer.resize(0x100, 0);
	border_buffer.resize(0xE000, 0);
	scanline_raw.resize(0x100, 0);
	scanline_priority.resize(0x100, 0);

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
	lcd_stat.last_y = 0;

	lcd_stat.oam_update = true;
	for(int x = 0; x < 40; x++) { lcd_stat.oam_update_list[x] = true; }

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

	//Signed-to-unsigned tile lookup generation
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

	//Unsigned-to-signed tile lookup generation
	for(int x = 0; x < 256; x++)
	{
		u8 tile_number = x;

		if(tile_number >= 127)
		{
			tile_number -= 128;
			lcd_stat.unsigned_tile_lut[x] = tile_number;
		}

		else 
		{ 
			tile_number += 128;
			lcd_stat.unsigned_tile_lut[x] = tile_number;
		}
	}

	//8 pixel (horizontal+vertical) flipping lookup generation
	for(int x = 0; x < 8; x++) { lcd_stat.flip_8[x] = (7 - x); }

	//16 pixel (vertical) flipping lookup generation
        for(int x = 0; x < 16; x++) { lcd_stat.flip_16[x] = (15 - x); }

	//Initialize SGB stuff
	sgb_mask_mode = 0;
	manual_pal = false;
	render_border = false;

	for(int x = 0; x < 2064; x += 4)
	{
		sgb_pal[x] = config::DMG_BG_PAL[0];
		sgb_pal[x + 1] = config::DMG_BG_PAL[1];
		sgb_pal[x + 2] = config::DMG_BG_PAL[2];
		sgb_pal[x + 3] = config::DMG_BG_PAL[3];
	}

	for(int x = 0; x < 4050; x++) { atf_data[x] = 0; }
	for(int x = 0; x < 4; x++) { sgb_system_pal[x] = 0; }

	for(int x = 0; x < 1024; x++) { border_tile_map[x] = 0; }
	for(int x = 0; x < 64; x++) { border_pal[x] = 0; }
	for(int x = 0; x < 8192; x++) { border_chr[x] = 0; }

	//Initialize SGB border off
	config::resize_mode = 0;
	config::request_resize = false;

	//Initialize system screen dimensions
	config::sys_width = 160;
	config::sys_height = 144;

	max_fullscreen_ratio = 2;
}

/****** Initialize LCD with SDL ******/
bool SGB_LCD::init()
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
		if(config::use_opengl) { opengl_init(); }

		//Set up software rendering
		else
		{
			window = SDL_CreateWindow("GBE+", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, config::sys_width, config::sys_height, config::flags);
			SDL_GetWindowSize(window, &config::win_width, &config::win_height);
			config::scaling_factor = 1;

			final_screen = SDL_GetWindowSurface(window);
			original_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, config::sys_width, config::sys_height, 32, 0, 0, 0, 0);
		}

		if(final_screen == NULL) { return false; }
	}

	//Initialize with only a buffer for OpenGL (for external rendering)
	else if((!config::sdl_render) && (config::use_opengl))
	{
		final_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, config::sys_width, config::sys_height, 32, 0, 0, 0, 0);
	}

	std::cout<<"LCD::Initialized\n";

	return true;
}

/****** Read LCD data from save state ******/
bool SGB_LCD::lcd_read(u32 offset, std::string filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary);
	
	if(!file.is_open()) { return false; }

	//Go to offset
	file.seekg(offset);

	//Serialize LCD data from file stream
	file.read((char*)&lcd_stat, sizeof(lcd_stat));

	//Serialize OBJ data from file stream
	for(int x = 0; x < 40; x++)
	{
		file.read((char*)&obj[x], sizeof(obj[x]));
	}

	//Sanitize LCD data
	if(lcd_stat.current_scanline > 153) { lcd_stat.current_scanline = 0;  }
	if(lcd_stat.last_y > 153) { lcd_stat.last_y = 0; }
	if(lcd_stat.lcd_clock > 70224) { lcd_stat.lcd_clock = 0; }

	lcd_stat.lcd_mode &= 0x3;
	lcd_stat.hdma_type &= 0x1;
	
	file.close();
	return true;
}

/****** Read LCD data from save state ******/
bool SGB_LCD::lcd_write(std::string filename)
{
	std::ofstream file(filename.c_str(), std::ios::binary | std::ios::app);
	
	if(!file.is_open()) { return false; }

	//Serialize LCD data to file stream
	file.write((char*)&lcd_stat, sizeof(lcd_stat));

	//Serialize OBJ data to file stream
	for(int x = 0; x < 40; x++)
	{
		file.write((char*)&obj[x], sizeof(obj[x]));
	}

	file.close();
	return true;
}

/****** Compares LY and LYC - Generates STAT interrupt ******/
void SGB_LCD::scanline_compare()
{
	if(mem->memory_map[REG_LY] == mem->memory_map[REG_LYC]) 
	{ 
		mem->memory_map[REG_STAT] |= 0x4; 
		if(mem->memory_map[REG_STAT] & 0x40) { mem->memory_map[IF_FLAG] |= 2; }
	}
	else { mem->memory_map[REG_STAT] &= ~0x4; }
}

/****** Updates OAM entries when values in memory change ******/
void SGB_LCD::update_oam()
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
void SGB_LCD::update_obj_render_list()
{
	obj_render_length = -1;

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
void SGB_LCD::render_sgb_scanline() 
{
	//Handle SGB Mask Mode
	switch(sgb_mask_mode)
	{
		//Freeze current image
		case 0x1: return;

		//Black screen
		case 0x2:
			for(int x = 0; x < 160; x++) { scanline_buffer[x] = 0; }
			return;

		//Color 0
		case 0x3:
			for(int x = 0; x < 160; x++) { scanline_buffer[x] = config::DMG_BG_PAL[0]; }
			return;
	}

	//Draw background pixel data
	if(lcd_stat.bg_enable) { render_sgb_bg_scanline(); }

	//Draw window pixel data
	if(lcd_stat.window_enable) { render_sgb_win_scanline(); }
				
	//Draw sprite pixel data
	if(lcd_stat.obj_enable) { render_sgb_obj_scanline(); }

	//Push scanline buffer to screen buffer - Normal version
	if((config::resize_mode == 0) && (!config::request_resize))
	{
		for(int x = 0; x < 160; x++)
		{
			screen_buffer[(config::sys_width * lcd_stat.current_scanline) + x] = scanline_buffer[x];
			scanline_buffer[x] = 0xFFFFFFFF;
		}
	}

	//Push scanline buffer to screen buffer - SGB border version
	else if((config::resize_mode == 1) && (!config::request_resize))
	{
		u16 offset = 10288 + (lcd_stat.current_scanline * 256);

		for(int x = 0; x < 160; x++)
		{
			screen_buffer[offset + x] = scanline_buffer[x];
			scanline_buffer[x] = 0xFFFFFFFF;
		}
	}
}

/****** Renders pixels for the BG (per-scanline) ******/
void SGB_LCD::render_sgb_bg_scanline()
{
	//Determine current ATF for SGB system colors
	u8 system_colors = 0;
	u16 pal_id = 0;
	s8 color_shift = 6;
	u16 atf_index = current_atf * 90;
	atf_index += (lcd_stat.current_scanline / 8) * 5;

	//Grab Color 0
	u32 color_0 = (manual_pal) ? sgb_pal[2048] : sgb_pal[(sgb_system_pal[0] * 4)];

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
		//Lookup SGB system colors from ATF
		if(((lcd_stat.scanline_pixel_counter + 8) & 0xFF) < 160)
		{
			system_colors = (atf_data[atf_index] >> color_shift) & 0x3;
			
			if(manual_pal) { pal_id = (system_colors * 4) + 2048; }
			else { pal_id = sgb_system_pal[system_colors] * 4; }
		}
		
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
					scanline_buffer[lcd_stat.scanline_pixel_counter++] = color_0;
					break;

				case 1: 
					scanline_buffer[lcd_stat.scanline_pixel_counter++] = sgb_pal[pal_id + 1];
					break;

				case 2: 
					scanline_buffer[lcd_stat.scanline_pixel_counter++] = sgb_pal[pal_id + 2];
					break;

				case 3: 
					scanline_buffer[lcd_stat.scanline_pixel_counter++] = sgb_pal[pal_id + 3];
					break;
			}

			u8 last_scanline_pixel = lcd_stat.scanline_pixel_counter - 1;
		}

		//Increment ATF index
		if(((lcd_stat.scanline_pixel_counter + 8) & 0xFF) < 160)
		{
			color_shift -= 2;

			if(color_shift < 0)
			{
				color_shift = 6;
				atf_index++;
			}
		}
	}
}

/****** Renders pixels for the Window (per-scanline) ******/
void SGB_LCD::render_sgb_win_scanline()
{
	//Determine if scanline is within window, if not abort rendering
	if((lcd_stat.current_scanline < lcd_stat.window_y) || (lcd_stat.window_x >= 160)) { return; }

	//Determine current ATF for SGB system colors
	u8 system_colors = 0;
	u16 pal_id = 0;
	s8 color_shift = 6;
	u16 atf_index = current_atf * 90;
	atf_index += (lcd_stat.current_scanline / 8) * 5;

	//Grab Color 0
	u32 color_0 = (manual_pal) ? sgb_pal[2048] : sgb_pal[(sgb_system_pal[0] * 4)];

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
		//Lookup SGB system colors from ATF
		if(((lcd_stat.scanline_pixel_counter + 8) & 0xFF) < 160)
		{
			system_colors = (atf_data[atf_index] >> color_shift) & 0x3;
			
			if(manual_pal) { pal_id = (system_colors * 4) + 2048; }
			else { pal_id = sgb_system_pal[system_colors] * 4; }
		}

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
					scanline_buffer[lcd_stat.scanline_pixel_counter++] = color_0;
					break;

				case 1: 
					scanline_buffer[lcd_stat.scanline_pixel_counter++] = sgb_pal[pal_id + 1];
					break;

				case 2: 
					scanline_buffer[lcd_stat.scanline_pixel_counter++] = sgb_pal[pal_id + 2];
					break;

				case 3: 
					scanline_buffer[lcd_stat.scanline_pixel_counter++] = sgb_pal[pal_id + 3];
					break;
			}

			u8 last_scanline_pixel = lcd_stat.scanline_pixel_counter - 1;

			//Abort rendering if next pixel is off-screen
			if(lcd_stat.scanline_pixel_counter == 160) { return; }
		}

		//Increment ATF index
		if(((lcd_stat.scanline_pixel_counter + 8) & 0xFF) < 160)
		{
			color_shift -= 2;

			if(color_shift < 0)
			{
				color_shift = 6;
				atf_index++;
			}
		}
	}
}

/****** Renders pixels for OBJs (per-scanline) ******/
void SGB_LCD::render_sgb_obj_scanline()
{
	//If no sprites are rendered on this line, quit now
	if(obj_render_length < 0) { return; }

	//Determine current ATF for SGB system colors
	u8 system_colors = 0;
	u16 pal_id = 0;
	s8 color_shift = 0;
	u16 atf_index = current_atf * 90;
	atf_index += (lcd_stat.current_scanline / 8) * 5;

	//Grab Color 0
	u32 color_0 = (manual_pal) ? sgb_pal[2048] : sgb_pal[(sgb_system_pal[0] * 4)];

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
				//Lookup SGB system colors from ATF
				if(lcd_stat.scanline_pixel_counter < 160)
				{
					u8 shift_index = (lcd_stat.scanline_pixel_counter / 32);
					color_shift = 6 - ((lcd_stat.scanline_pixel_counter % 8) & ~0x1);
					system_colors = (atf_data[atf_index + shift_index] >> color_shift) & 0x3;
			
					if(manual_pal) { pal_id = (system_colors * 4) + 2048; }
					else { pal_id = sgb_system_pal[system_colors] * 4; }
				}

				switch(lcd_stat.obp[tile_pixel][obj[sprite_id].palette_number])
				{
					case 0: 
						scanline_buffer[lcd_stat.scanline_pixel_counter++] = color_0;
						break;

					case 1: 
						scanline_buffer[lcd_stat.scanline_pixel_counter++] = sgb_pal[pal_id + 1];
						break;

					case 2: 
						scanline_buffer[lcd_stat.scanline_pixel_counter++] = sgb_pal[pal_id + 2];
						break;

					case 3: 
						scanline_buffer[lcd_stat.scanline_pixel_counter++] = sgb_pal[pal_id + 3];
						break;
				}

				u8 last_scanline_pixel = lcd_stat.scanline_pixel_counter - 1;
			}

			//Move onto next pixel in scanline to see if sprite rendering occurs
			else { lcd_stat.scanline_pixel_counter++; }
		}
	}
}

/****** Execute LCD operations ******/
void SGB_LCD::step(int cpu_clock) 
{
	//Process SGB commands
	if(mem->g_pad->get_pad_data(0)) { process_sgb_command(); }

        //Enable the LCD
	if((lcd_stat.on_off) && (lcd_stat.lcd_enable)) 
	{
		lcd_stat.on_off = false;
		lcd_stat.lcd_mode = 2;

		scanline_compare();
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

		//Set LY to zero here, but DO NOT do a LYC test until LCD is turned back on
		//Mr. Do! requires the test to occur only when the LCD is turned on
		lcd_stat.current_scanline = mem->memory_map[REG_LY] = 0;
		lcd_stat.lcd_clock = 0;
		lcd_stat.lcd_mode = 0;
	}

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

					//Update OAM
					if(lcd_stat.oam_update) { update_oam(); }
					else { update_obj_render_list(); }
					
					//Render scanline when first entering Mode 0
					render_sgb_scanline();

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

				//Update border
				if(render_border) { render_sgb_border(); }

				//Restore Window parameters
				lcd_stat.last_y = 0;
				lcd_stat.window_y = mem->memory_map[REG_WY];

				//Check for screen resize - SGB border on
				if((config::request_resize) && (config::resize_mode > 0))
				{
					config::sys_width = 256;
					config::sys_height = 224;
					screen_buffer.clear();
					screen_buffer.resize(0xE000, 0xFFFFFFFF);
					
					if((window != NULL) && (config::sdl_render)) { SDL_DestroyWindow(window); }
					init();
					
					if(config::sdl_render) { config::request_resize = false; }
				}

				//Check for screen resize - SGB border off
				else if(config::request_resize)
				{
					config::sys_width = 160;
					config::sys_height = 144;
					screen_buffer.clear();
					screen_buffer.resize(0x5A00, 0xFFFFFFFF);

					if((window != NULL) && (config::sdl_render)) { SDL_DestroyWindow(window); }
					init();
					
					if(config::sdl_render) { config::request_resize = false; }
				}

				//Increment scanline count
				lcd_stat.current_scanline++;
				mem->memory_map[REG_LY] = lcd_stat.current_scanline;
				scanline_compare();
				
				//Setup the VBlank clock to count 10 scanlines
				lcd_stat.vblank_clock = lcd_stat.lcd_clock - 65664;
					
				//VBlank STAT INT
				if(mem->memory_map[REG_STAT] & 0x10) { mem->memory_map[IF_FLAG] |= 2; }

				//Raise other STAT INTs on this line
				if(((mem->memory_map[IE_FLAG] & 0x1) == 0) && ((mem->memory_map[REG_STAT] & 0x20))) { mem->memory_map[IF_FLAG] |= 2; }

				//VBlank INT
				mem->memory_map[IF_FLAG] |= 1;

				//Render final screen buffer
				if(lcd_stat.lcd_enable)
				{
					//Use SDL
					if(config::sdl_render)
					{
						//If using SDL and no OpenGL, manually stretch for fullscreen via SDL
						if((config::flags & SDL_WINDOW_FULLSCREEN_DESKTOP) && (!config::use_opengl))
						{
							//Lock source surface
							if(SDL_MUSTLOCK(original_screen)){ SDL_LockSurface(original_screen); }
							u32* out_pixel_data = (u32*)original_screen->pixels;

							for(int a = 0; a < screen_buffer.size(); a++) { out_pixel_data[a] = screen_buffer[a]; }

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

							for(int a = 0; a < screen_buffer.size(); a++) { out_pixel_data[a] = screen_buffer[a]; }

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
						if(!config::use_opengl) { config::render_external_sw(screen_buffer); }

						else
						{
							//Lock source surface
							if(SDL_MUSTLOCK(final_screen)){ SDL_LockSurface(final_screen); }
							u32* out_pixel_data = (u32*)final_screen->pixels;

							for(int a = 0; a < screen_buffer.size(); a++) { out_pixel_data[a] = screen_buffer[a]; }

							//Unlock source surface
							if(SDL_MUSTLOCK(final_screen)){ SDL_UnlockSurface(final_screen); }

							config::render_external_hw(final_screen);
						}
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
				if(((SDL_GetTicks() - fps_time) >= 1000) && (config::sdl_render)) 
				{ 
					fps_time = SDL_GetTicks();
					config::title.str("");
					config::title << "GBE+ " << fps_count << "FPS";
					SDL_SetWindowTitle(window, config::title.str().c_str());
					fps_count = 0; 
				}

				//Process gyroscope
				if(mem->cart.mbc_type == DMG_MMU::MBC7) { mem->g_pad->process_gyroscope(); }

				//Process Gameshark cheats
				if(config::use_cheats) { mem->set_gs_cheats(); }

				if(config::resize_mode == 1)
				{
					for(u32 x = 0; x < 0xE000; x++) { screen_buffer[x] = border_buffer[x]; }
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

					//By line 153, LCD has actually reached the top of the screen again
					//LY will read 153 for only a few cycles, then go to 0 for the rest of the scanline
					//Line 153 and Line 0 STAT-LYC IRQs should be triggered here
					if(lcd_stat.current_scanline == 153)
					{
						//Do a scanline compare for Line 153 now
						mem->memory_map[REG_LY] = lcd_stat.current_scanline;
						scanline_compare();

						//Set LY to 0, also trigger Line 0 STAT-LYC IRQ if necessary
						//Technically this should fire 8 cycles into the scanline
						lcd_stat.current_scanline = 0;
						mem->memory_map[REG_LY] = lcd_stat.current_scanline;
						scanline_compare();
					}

					//After Line 153 reset LCD clock, scanline count
					else if(lcd_stat.current_scanline == 1) 
					{
						lcd_stat.lcd_clock -= 70224;
						lcd_stat.current_scanline = 0;
						mem->memory_map[REG_LY] = lcd_stat.current_scanline;
					}

					//Process Lines 144-152 normally
					else
					{
						mem->memory_map[REG_LY] = lcd_stat.current_scanline;
						scanline_compare();
					}
				}
			}
		}
	}

	mem->memory_map[REG_STAT] = (mem->memory_map[REG_STAT] & ~0x3) | lcd_stat.lcd_mode;
}

/****** Processes video related SGB commands ******/
void SGB_LCD::process_sgb_command()
{
	switch(mem->g_pad->get_pad_data(2))
	{
		//PAL01
		case 0x0:
				mem->g_pad->set_pad_data(0, 0);

				manual_pal = true;

				//Grab Colors 0-3 - Palette 0
				sgb_pal[2048] = get_color(mem->g_pad->get_pad_data(3));
				sgb_pal[2049] = get_color(mem->g_pad->get_pad_data(4));
				sgb_pal[2050] = get_color(mem->g_pad->get_pad_data(5));
				sgb_pal[2051] = get_color(mem->g_pad->get_pad_data(6));

				//Grab Colors 1-3 - Palette 1
				sgb_pal[2053] = get_color(mem->g_pad->get_pad_data(7));
				sgb_pal[2054] = get_color(mem->g_pad->get_pad_data(8));
				sgb_pal[2055] = get_color(mem->g_pad->get_pad_data(9));

				break;

		//PAL23
		case 0x1:
				mem->g_pad->set_pad_data(0, 0);

				manual_pal = true;

				//Grab Colors 0-3 - Palette 2
				sgb_pal[2056] = get_color(mem->g_pad->get_pad_data(3));
				sgb_pal[2057] = get_color(mem->g_pad->get_pad_data(4));
				sgb_pal[2058] = get_color(mem->g_pad->get_pad_data(5));
				sgb_pal[2059] = get_color(mem->g_pad->get_pad_data(6));

				//Grab Colors 1-3 - Palette 3
				sgb_pal[2061] = get_color(mem->g_pad->get_pad_data(7));
				sgb_pal[2062] = get_color(mem->g_pad->get_pad_data(8));
				sgb_pal[2063] = get_color(mem->g_pad->get_pad_data(9));

				break;

		//PAL03
		case 0x2:
				mem->g_pad->set_pad_data(0, 0);

				manual_pal = true;

				//Grab Colors 0-3 - Palette 0
				sgb_pal[2048] = get_color(mem->g_pad->get_pad_data(3));
				sgb_pal[2049] = get_color(mem->g_pad->get_pad_data(4));
				sgb_pal[2050] = get_color(mem->g_pad->get_pad_data(5));
				sgb_pal[2051] = get_color(mem->g_pad->get_pad_data(6));

				//Grab Colors 1-3 - Palette 3
				sgb_pal[2061] = get_color(mem->g_pad->get_pad_data(7));
				sgb_pal[2062] = get_color(mem->g_pad->get_pad_data(8));
				sgb_pal[2063] = get_color(mem->g_pad->get_pad_data(9));

				break;

		//PAL12
		case 0x3:
				mem->g_pad->set_pad_data(0, 0);

				manual_pal = true;

				//Grab Colors 0-3 - Palette 1
				sgb_pal[2052] = get_color(mem->g_pad->get_pad_data(3));
				sgb_pal[2053] = get_color(mem->g_pad->get_pad_data(4));
				sgb_pal[2054] = get_color(mem->g_pad->get_pad_data(5));
				sgb_pal[2055] = get_color(mem->g_pad->get_pad_data(6));

				//Grab Colors 1-3 - Palette 2
				sgb_pal[2057] = get_color(mem->g_pad->get_pad_data(7));
				sgb_pal[2058] = get_color(mem->g_pad->get_pad_data(8));
				sgb_pal[2059] = get_color(mem->g_pad->get_pad_data(9));

				break;		

		//PAL_SET
		case 0xA:
			{
				mem->g_pad->set_pad_data(0, 0);

				//Grab system palette numbers
				sgb_system_pal[0] = mem->g_pad->get_pad_data(3);
				sgb_system_pal[1] = mem->g_pad->get_pad_data(4);
				sgb_system_pal[2] = mem->g_pad->get_pad_data(5);
				sgb_system_pal[3] = mem->g_pad->get_pad_data(6);

				u8 atf_byte = mem->g_pad->get_pad_data(10);

				//Grab ATF if necessary
				if(atf_byte & 0x80)
				{
					current_atf = atf_byte & 0x3F;
					if(current_atf > 0x2C) { current_atf = 0x2C; }
				}

				//Disable mask if necessary
				if(atf_byte & 0x40)
				{
					sgb_mask_mode = 0;
					mem->g_pad->set_pad_data(1, 0);
				}

				manual_pal = false;
			}

			break;

		//PAL_TRN
		case 0xB:
			mem->g_pad->set_pad_data(0, 0);

			//1K VRAM transfer -> 2048 SGB colors, 512 palettes, 4 colors per palette
			for(u32 x = 0; x < 0x1000; x += 2)
			{
				u16 color = mem->read_u16(lcd_stat.bg_tile_addr + x);
				sgb_pal[x >> 1] = get_color(color);
			}

			break;

		//CHR_TRN
		case 0x13:
			{
				mem->g_pad->set_pad_data(0, 0);

				u8 tile_options = (mem->g_pad->get_pad_data(3) & 0x3);

				//SNES OBJ not supported yet
				if((tile_options & 0x2) == 0)
				{
					u16 offset = (tile_options & 0x1) ? 0x1000 : 0; 

					//Grab border CHR data
					for(u32 x = 0; x < 0x1000; x++)
					{
						border_chr[x + offset] = mem->read_u8(lcd_stat.bg_tile_addr + x);
					}

					render_border = true;
				}
			}
				
			break;

		//PIC_TRN
		case 0x14:
			mem->g_pad->set_pad_data(0, 0);

			//Grab border tile map data
			for(u32 x = 0; x < 0x800; x+= 2)
			{
				border_tile_map[x >> 1] = mem->read_u16(lcd_stat.bg_tile_addr + x);
			}

			//Grab border palette data
			for(u32 x = 0; x < 0x80; x += 2)
			{
				u16 color = mem->read_u16(lcd_stat.bg_tile_addr + 0x800 + x);
				border_pal[x >> 1] = get_color(color);
			}

			render_border = true;

			break;

		//ATTR_TRN
		case 0x15:
			mem->g_pad->set_pad_data(0, 0);

			//4050 byte VRAM transfer -> 20x18 ATR map
			for(u32 x = 0; x < 4050; x++) { atf_data[x] = mem->read_u8(lcd_stat.bg_tile_addr + x); }

			break;

		//MASK_EN
		case 0x17:
			mem->g_pad->set_pad_data(0, 0);
			sgb_mask_mode = mem->g_pad->get_pad_data(1);
			break;

		//Unknown command or unhandled command
		default:
			mem->g_pad->set_pad_data(0, 0);
			std::cout<<"LCD::Unhandled SGB command 0x" << mem->g_pad->get_pad_data(2) << "\n"; 
			break;
	}
}

/****** Converts 15-bit color to 32-bit ******/
u32 SGB_LCD::get_color(u16 input_color)
{
	u8 red = ((input_color & 0x1F) << 3);
	input_color >>= 5;

	u8 green = ((input_color & 0x1F) << 3);
	input_color >>= 5;

	u8 blue = ((input_color & 0x1F) << 3);

	return 0xFF000000 | (red << 16) | (green << 8) | (blue);
}

/****** Draws the SGB border to a buffer ******/
void SGB_LCD::render_sgb_border()
{
	render_border = false;

	u16 border_pixel_counter = 0;

	//Cycle through each tile, rendering border scanline-by-scanline
	for(u16 line = 0; line < 224; line++)
	{
		//Determine which tiles we should generate to get the scanline data - integer division ftw :p
		u16 tile_lower_range = (line / 8) * 32;
		u16 tile_upper_range = tile_lower_range + 32;

		//Determine which line of the tiles to generate pixels for this scanline
		u8 tile_line = (line % 8);

		//Generate border pixel data for selected tiles
		for(int x = tile_lower_range; x < tile_upper_range; x++)
		{		
			u16 map_entry = border_tile_map[x];
			u8 tile_pixel = 0;
			u8 ext_pal = 0;

			u8 pal_id = ((map_entry >> 10) & 0x7) - 4;
			pal_id *= 16;

			bool h_flip = (map_entry & 0x4000) ? true : false;
			bool v_flip = (map_entry & 0x8000) ? true : false;

			u8 real_line = (v_flip) ? lcd_stat.flip_8[tile_line] : (tile_line % 8);

			map_entry &= 0xFF;

			//Calculate the address of the 8x1 pixel data based on map entry
			u16 tile_addr = (map_entry << 5) + (real_line << 1);

			//Calculate the address of the extended palette data
			u16 pal_addr = tile_addr + 16;

			//Grab bytes from CHR data representing 8x1 pixel data
			u16 tile_data = (border_chr[tile_addr + 1] << 8) | border_chr[tile_addr];

			//Grab bytes from CHR data representing extended palette data
			u16 ext_color_data = (border_chr[pal_addr + 1] << 8) | border_chr[pal_addr];

			for(int y = 7; y >= 0; y--)
			{
				if(h_flip)
				{
					//Calculate raw value of the tile's pixel
					tile_pixel = ((tile_data >> 8) & (1 << lcd_stat.flip_8[y])) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << lcd_stat.flip_8[y])) ? 1 : 0;

					//Calculate extended color data
					ext_pal = ((ext_color_data >> 8) & (1 << lcd_stat.flip_8[y])) ? 2 : 0;
					ext_pal |= (ext_color_data & (1 << lcd_stat.flip_8[y])) ? 1 : 0;
				}

				else
				{
					//Calculate raw value of the tile's pixel
					tile_pixel = ((tile_data >> 8) & (1 << y)) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << y)) ? 1 : 0;

					//Calculate extended color data
					ext_pal = ((ext_color_data >> 8) & (1 << y)) ? 2 : 0;
					ext_pal |= (ext_color_data & (1 << y)) ? 1 : 0;
				}				

				border_buffer[border_pixel_counter++] = border_pal[pal_id + (ext_pal * 4) + tile_pixel];
			}
		}
	}
}
