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
#include "common/cgfx_common.h"
#include "common/util.h"

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

	screen_buffer.clear();
	scanline_buffer.clear();
	scanline_raw.clear();
	scanline_priority.clear();
	stretched_buffer.clear();

	screen_buffer.resize(0x5A00, 0);
	scanline_buffer.resize(0x100, 0);
	stretched_buffer.resize(0x100, 0);
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

	//CGFX setup
	cgfx_stat.current_obj_hash.clear();
	cgfx_stat.current_obj_hash.resize(40, "");

	cgfx_stat.current_bg_hash.clear();
	cgfx_stat.current_bg_hash.resize(384, "");

	cgfx_stat.current_gbc_bg_hash.clear();
	cgfx_stat.current_gbc_bg_hash.resize(2048, "");

	cgfx_stat.bg_update_list.clear();
	cgfx_stat.bg_update_list.resize(384, false);

	cgfx_stat.bg_tile_update_list.clear();
	cgfx_stat.bg_tile_update_list.resize(256, false);

	cgfx_stat.bg_map_update_list.clear();
	cgfx_stat.bg_map_update_list.resize(2048, false);

	cgfx_stat.update_bg = false;
	cgfx_stat.update_map = false;

	//Initialize system screen dimensions
	config::sys_width = 160;
	config::sys_height = 144;

	//Initialize DMG/GBC on GBA stretching to normal mode
	config::resize_mode = 0;
	config::request_resize = false;

	//Load CGFX manifest
	if(cgfx::load_cgfx) 
	{
		cgfx::load_cgfx = load_manifest(cgfx::manifest_file);
		
		//Initialize HD buffer for CGFX greater that 1:1
		if((cgfx::load_cgfx) && (cgfx::scaling_factor > 1))
		{
			config::sys_width *= cgfx::scaling_factor;
			config::sys_height *= cgfx::scaling_factor;
			cgfx::scale_squared = cgfx::scaling_factor * cgfx::scaling_factor;

			hd_screen_buffer.clear();
			hd_screen_buffer.resize((config::sys_width * config::sys_height), 0);
		}
	}
}

/****** Initialize LCD with SDL ******/
bool DMG_LCD::init()
{
	if(config::sdl_render)
	{
		if(SDL_Init(SDL_INIT_EVERYTHING) == -1)
		{
			std::cout<<"LCD::Error - Could not initialize SDL\n";
			return false;
		}

		if(config::use_opengl) {opengl_init(); }
		else { final_screen = SDL_SetVideoMode(config::sys_width, config::sys_height, 32, SDL_SWSURFACE); }

		if(final_screen == NULL) { return false; }
	}

	else if((!config::sdl_render) && (config::use_opengl))
	{
		final_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, config::sys_width, config::sys_height, 32, 0, 0, 0, 0);
	}

	std::cout<<"LCD::Initialized\n";

	return true;
}

/****** Read LCD data from save state ******/
bool DMG_LCD::lcd_read(u32 offset, std::string filename)
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

	file.close();
	return true;
}

/****** Read LCD data from save state ******/
bool DMG_LCD::lcd_write(std::string filename)
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

			//CGFX - Update OBJ hashes
			if((cgfx::load_cgfx) || (cgfx::auto_dump_obj)) 
			{
				if(config::gb_type == 2) { update_gbc_obj_hash(x); }
				else { update_dmg_obj_hash(x); }
			}
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

	//Ignore CGFX when greater than 1:1
	if((cgfx::load_cgfx) && (cgfx::scaling_factor > 1)) { return; }

	//Push scanline buffer to screen buffer - Normal version
	else if((config::resize_mode == 0) && (!config::request_resize))
	{
		for(int x = 0; x < 160; x++)
		{
			screen_buffer[(config::sys_width * lcd_stat.current_scanline) + x] = scanline_buffer[x];
			scanline_buffer[x] = 0xFFFFFFFF;
		}
	}

	//Push scanline buffer to screen buffer - DMG/GBC on GBA stretch
	else if((config::resize_mode == 1) && (!config::request_resize))
	{
		u16 offset = 1960 + (lcd_stat.current_scanline * 240);

		for(int x = 0; x < 160; x++)
		{
			screen_buffer[offset + x] = scanline_buffer[x];
			scanline_buffer[x] = 0xFFFFFFFF;
		}
	}

	//Push scanline buffer to screen buffer - DMG/GBC on GBA stretch
	else if((config::resize_mode == 2) && (!config::request_resize))
	{
		u16 offset = 1920 + (lcd_stat.current_scanline * 240);
		u8 pixel_counter = (0x100 - lcd_stat.bg_scroll_x);
		u16 stretched_pos = 0;
		u8 old_pos, blend_pos = 0;

		for(u8 x = 0; x < 160; x++)
		{
			old_pos = x;
			stretched_buffer[stretched_pos++] = scanline_buffer[x++];
			stretched_buffer[stretched_pos++] = util::rgb_blend(scanline_buffer[old_pos], scanline_buffer[x]);
			stretched_buffer[stretched_pos++] = scanline_buffer[x];
		}

		for(int x = 0; x < 240; x++)
		{
			screen_buffer[offset + x] = stretched_buffer[x];
			scanline_buffer[x] = 0xFFFFFFFF;
			stretched_buffer[x] = 0xFFFFFFFF;
		}
	}
}

/****** Render pixels for a given scanline (per-scanline) - GBC version ******/
void DMG_LCD::render_gbc_scanline() 
{
	//Draw background pixel data
	render_gbc_bg_scanline();

	//Draw window pixel data
	if(lcd_stat.window_enable) { render_gbc_win_scanline(); }

	//Draw sprite pixel data
	if(lcd_stat.obj_enable) { render_gbc_obj_scanline(); }

	//Ignore CGFX when greater than 1:1
	if((cgfx::load_cgfx) && (cgfx::scaling_factor > 1)) { return; }

	//Push scanline buffer to screen buffer - Normal version
	else if((config::resize_mode == 0) && (!config::request_resize))
	{
		for(int x = 0; x < 160; x++)
		{
			screen_buffer[(config::sys_width * lcd_stat.current_scanline) + x] = scanline_buffer[x];
			scanline_buffer[x] = 0xFFFFFFFF;
		}
	}

	//Push scanline buffer to screen buffer - DMG/GBC on GBA stretch
	else if((config::resize_mode == 1) && (!config::request_resize))
	{
		u16 offset = 1960 + (lcd_stat.current_scanline * 240);

		for(int x = 0; x < 160; x++)
		{
			screen_buffer[offset + x] = scanline_buffer[x];
			scanline_buffer[x] = 0xFFFFFFFF;
		}
	}

	//Push scanline buffer to screen buffer - DMG/GBC on GBA stretch
	else if((config::resize_mode == 2) && (!config::request_resize))
	{
		u16 offset = 1920 + (lcd_stat.current_scanline * 240);
		u8 pixel_counter = (0x100 - lcd_stat.bg_scroll_x);
		u16 stretched_pos = 0;
		u8 old_pos, blend_pos = 0;

		for(u8 x = 0; x < 160; x++)
		{
			old_pos = x;
			stretched_buffer[stretched_pos++] = scanline_buffer[x++];
			stretched_buffer[stretched_pos++] = util::rgb_blend(scanline_buffer[old_pos], scanline_buffer[x]);
			stretched_buffer[stretched_pos++] = scanline_buffer[x];
		}

		for(int x = 0; x < 240; x++)
		{
			screen_buffer[offset + x] = stretched_buffer[x];
			scanline_buffer[x] = 0xFFFFFFFF;
			stretched_buffer[x] = 0xFFFFFFFF;
		}
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

		u16 bg_id = (((lcd_stat.bg_tile_addr + (map_entry << 4)) & ~0x8000) >> 4);
		
		//Render CGFX
		u16 hash_addr = lcd_stat.bg_tile_addr + (map_entry << 4);
		if((cgfx::load_cgfx) && (has_hash(hash_addr, cgfx_stat.current_bg_hash[bg_id]))) { render_cgfx_dmg_bg_scanline(bg_id); }

		//Render original pixel data
		else 
		{
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

				u8 last_scanline_pixel = lcd_stat.scanline_pixel_counter - 1;

				//Render HD
				if((cgfx::load_cgfx) && (cgfx::scaling_factor > 1) && (last_scanline_pixel < 160))
				{
					u32 pos = (last_scanline_pixel * cgfx::scaling_factor) + (lcd_stat.current_scanline * cgfx::scaling_factor * config::sys_width);
			
					for(int a = 0; a < cgfx::scaling_factor; a++)
					{
						for(int b = 0; b < cgfx::scaling_factor; b++)
						{
							hd_screen_buffer[pos + b] = scanline_buffer[last_scanline_pixel];
						}
	
						pos += config::sys_width;
					}
				}
			}
		}
	}
}

/****** Renders pixels for the BG (per-scanline) - DMG CGFX version ******/
void DMG_LCD::render_cgfx_dmg_bg_scanline(u16 bg_id)
{
	//Determine where to start drawing
	u8 rendered_scanline = lcd_stat.current_scanline + lcd_stat.bg_scroll_y;

	//Determine which line of the tiles to generate pixels for this scanline
	u8 tile_line = rendered_scanline % 8;
	u8 tile_start = tile_line * 8;

	//Grab the ID of this hash to pull custom pixel data
	u16 bg_tile_id = cgfx_stat.m_id[cgfx_stat.last_id];

	//Grab bytes from VRAM representing 8x1 pixel data - Used for drawing the raw_scanline
	u16 tile_data = mem->read_u16(0x8000 + (bg_id << 4) + (tile_line << 1));
	u8 tile_pixel = 0;

	for(int x = tile_start, y = 7; x < (tile_start + 8); x++, y--)
	{
		//Calculate raw value of the tile's pixel
		tile_pixel = ((tile_data >> 8) & (1 << y)) ? 2 : 0;
		tile_pixel |= (tile_data & (1 << y)) ? 1 : 0;

		//Set the raw color of the BG
		scanline_raw[lcd_stat.scanline_pixel_counter] = tile_pixel;

		//Render 1:1
		if(cgfx::scaling_factor <= 1)
		{
			scanline_buffer[lcd_stat.scanline_pixel_counter++] = cgfx_stat.bg_pixel_data[bg_tile_id][x];
		}

		//Render HD
		else
		{
			u32 pos = (lcd_stat.scanline_pixel_counter * cgfx::scaling_factor) + (lcd_stat.current_scanline * cgfx::scaling_factor * config::sys_width);
			u32 bg_pos = ((x - tile_start) * cgfx::scaling_factor) + (tile_line * cgfx::scale_squared * 8);
			
			if(lcd_stat.scanline_pixel_counter < 160)
			{

				for(int a = 0; a < cgfx::scaling_factor; a++)
				{
					for(int b = 0; b < cgfx::scaling_factor; b++)
					{
						hd_screen_buffer[pos + b] = cgfx_stat.bg_pixel_data[bg_tile_id][bg_pos + b];
					}
				
					pos += config::sys_width;
					bg_pos += (8 * cgfx::scaling_factor);
				}
			}

			lcd_stat.scanline_pixel_counter++;
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

		//Render CGFX
		u16 map_id = (lcd_stat.bg_map_addr + x) - 0x9800;
		u16 hash_addr = lcd_stat.bg_tile_addr + (map_entry << 4);
		if((cgfx::load_cgfx) && (has_hash(hash_addr, cgfx_stat.current_gbc_bg_hash[map_id]))) { render_cgfx_gbc_bg_scanline(tile_data, bg_map_attribute); }

		//Render original pixel data
		else
		{
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

				u8 last_scanline_pixel = lcd_stat.scanline_pixel_counter - 1;

				//Render HD
				if((cgfx::load_cgfx) && (cgfx::scaling_factor > 1) && (last_scanline_pixel < 160))
				{
					u32 pos = (last_scanline_pixel * cgfx::scaling_factor) + (lcd_stat.current_scanline * cgfx::scaling_factor * config::sys_width);
			
					for(int a = 0; a < cgfx::scaling_factor; a++)
					{
						for(int b = 0; b < cgfx::scaling_factor; b++)
						{
							hd_screen_buffer[pos + b] = scanline_buffer[last_scanline_pixel];
						}
	
						pos += config::sys_width;
					}
				}
			}
		}
	}
}

/****** Renders pixels for the BG (per-scanline) - GBC CGFX version ******/
void DMG_LCD::render_cgfx_gbc_bg_scanline(u16 tile_data, u8 bg_map_attribute)
{
	//Determine where to start drawing
	u8 rendered_scanline = lcd_stat.current_scanline + lcd_stat.bg_scroll_y;

	//Determine which line of the tiles to generate pixels for this scanline
	u8 tile_line = rendered_scanline % 8;
	u8 tile_start = tile_line * 8;

	//Grab the ID of this hash to pull custom pixel data
	u16 bg_tile_id = cgfx_stat.m_id[cgfx_stat.last_id];

	//Grab the auto-bright property of this hash
	u8 auto_bright = cgfx_stat.m_auto_bright[cgfx_stat.last_id];

	u8 tile_pixel = 0;
	u8 bg_priority = (bg_map_attribute & 0x80) ? 1 : 0;

	for(int x = tile_start, y = 7; x < (tile_start + 8); x++, y--)
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

		//Render 1:1
		if(cgfx::scaling_factor <= 1)
		{
			//Adjust pixel brightness if EXT_AUTO_BRIGHT is set
			if(auto_bright) 
			{
				u32 custom_color = cgfx_stat.bg_pixel_data[bg_tile_id][x];
				scanline_buffer[lcd_stat.scanline_pixel_counter++] = adjust_pixel_brightness(custom_color, (bg_map_attribute & 0x7), 0);
			}

			else { scanline_buffer[lcd_stat.scanline_pixel_counter++] = cgfx_stat.bg_pixel_data[bg_tile_id][x]; }
		}

		//Render HD
		else
		{
			u32 pos = (lcd_stat.scanline_pixel_counter * cgfx::scaling_factor) + (lcd_stat.current_scanline * cgfx::scaling_factor * config::sys_width);
			u32 bg_pos = ((x - tile_start) * cgfx::scaling_factor) + (tile_line * cgfx::scale_squared * 8);
			
			if(lcd_stat.scanline_pixel_counter < 160)
			{

				for(int a = 0; a < cgfx::scaling_factor; a++)
				{
					for(int b = 0; b < cgfx::scaling_factor; b++)
					{

						//Adjust pixel brightness if EXT_AUTO_BRIGHT is set
						if(auto_bright)
						{
							u32 custom_color = cgfx_stat.bg_pixel_data[bg_tile_id][bg_pos + b];
							hd_screen_buffer[pos + b] = adjust_pixel_brightness(custom_color, (bg_map_attribute & 0x7), 0);
						}

						else { hd_screen_buffer[pos + b] = cgfx_stat.bg_pixel_data[bg_tile_id][bg_pos + b]; }
					}
				
					pos += config::sys_width;
					bg_pos += (8 * cgfx::scaling_factor);
				}
			}

			lcd_stat.scanline_pixel_counter++;
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

		u16 bg_id = (((lcd_stat.bg_tile_addr + (map_entry << 4)) & ~0x8000) >> 4);
		
		//Render CGFX
		u16 hash_addr = lcd_stat.bg_tile_addr + (map_entry << 4);
		if((cgfx::load_cgfx) && (has_hash(hash_addr, cgfx_stat.current_bg_hash[bg_id]))) { render_cgfx_dmg_bg_scanline(bg_id); }

		//Render original pixel data
		else
		{
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

				u8 last_scanline_pixel = lcd_stat.scanline_pixel_counter - 1;

				//Render HD
				if((cgfx::load_cgfx) && (cgfx::scaling_factor > 1) && (last_scanline_pixel < 160))
				{
					u32 pos = (last_scanline_pixel * cgfx::scaling_factor) + (lcd_stat.current_scanline * cgfx::scaling_factor * config::sys_width);
			
					for(int a = 0; a < cgfx::scaling_factor; a++)
					{
						for(int b = 0; b < cgfx::scaling_factor; b++)
						{
							hd_screen_buffer[pos + b] = scanline_buffer[last_scanline_pixel];
						}
	
						pos += config::sys_width;
					}
				}

				//Abort rendering if next pixel is off-screen
				if(lcd_stat.scanline_pixel_counter == 160) { return; }
			}
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

		//Render CGFX
		u16 map_id = (lcd_stat.window_map_addr + x) - 0x9800;
		u16 hash_addr = lcd_stat.bg_tile_addr + (map_entry << 4);
		if((cgfx::load_cgfx) && (has_hash(hash_addr, cgfx_stat.current_gbc_bg_hash[map_id]))) { render_cgfx_gbc_bg_scanline(tile_data, bg_map_attribute); }

		//Render original pixel data
		else
		{
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

				u8 last_scanline_pixel = lcd_stat.scanline_pixel_counter - 1;

				//Render HD
				if((cgfx::load_cgfx) && (cgfx::scaling_factor > 1) && (last_scanline_pixel < 160))
				{
					u32 pos = (last_scanline_pixel * cgfx::scaling_factor) + (lcd_stat.current_scanline * cgfx::scaling_factor * config::sys_width);
			
					for(int a = 0; a < cgfx::scaling_factor; a++)
					{
						for(int b = 0; b < cgfx::scaling_factor; b++)
						{
							hd_screen_buffer[pos + b] = scanline_buffer[last_scanline_pixel];
						}
	
						pos += config::sys_width;
					}
				}

				//Abort rendering if next pixel is off-screen
				if(lcd_stat.scanline_pixel_counter == 160) { return; }
			}
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

		//Render CGFX
		u16 hash_addr = 0x8000 + (obj[sprite_id].tile_number << 4);
		if((cgfx::load_cgfx) && (has_hash(hash_addr, cgfx_stat.current_obj_hash[sprite_id]))) { render_cgfx_dmg_obj_scanline(sprite_id); }

		//Render original pixel data
		else 
		{
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

					u8 last_scanline_pixel = lcd_stat.scanline_pixel_counter - 1;

					//Render HD
					if((cgfx::load_cgfx) && (cgfx::scaling_factor > 1) && (last_scanline_pixel < 160))
					{
						u32 pos = (last_scanline_pixel * cgfx::scaling_factor) + (lcd_stat.current_scanline * cgfx::scaling_factor * config::sys_width);
			
						for(int a = 0; a < cgfx::scaling_factor; a++)
						{
							for(int b = 0; b < cgfx::scaling_factor; b++)
							{
								hd_screen_buffer[pos + b] = scanline_buffer[last_scanline_pixel];
							}
	
							pos += config::sys_width;
						}
					}
				}

				//Move onto next pixel in scanline to see if sprite rendering occurs
				else { lcd_stat.scanline_pixel_counter++; }
			}
		}
	}
}

/****** Renders pixels for OBJs (per-scanline) - DMG CGFX version ******/
void DMG_LCD::render_cgfx_dmg_obj_scanline(u8 sprite_id)
{
	//Set the current pixel to start obj rendering
	lcd_stat.scanline_pixel_counter = obj[sprite_id].x;
		
	//Determine which line of the tiles to generate pixels for this scanline		
	u8 tile_line = (lcd_stat.current_scanline - obj[sprite_id].y);
	if(obj[sprite_id].v_flip) { tile_line = (lcd_stat.obj_size == 8) ? lcd_stat.flip_8[tile_line] : lcd_stat.flip_16[tile_line]; }

	//If sprite is below BG and BG raw color is non-zero, abort rendering this pixel
	if((obj[sprite_id].bg_priority == 1) && (scanline_raw[lcd_stat.scanline_pixel_counter] != 0)) { return; }

	//Grab the ID of this hash to pull custom pixel data
	u16 obj_id = cgfx_stat.m_id[cgfx_stat.last_id];

	u16 tile_pixel = (8 * tile_line);
	u32 custom_color = 0;

	//Account for horizontal flipping
	lcd_stat.scanline_pixel_counter = obj[sprite_id].h_flip ? (lcd_stat.scanline_pixel_counter + 7) : lcd_stat.scanline_pixel_counter;
	s16 counter = obj[sprite_id].h_flip ? -1 : 1;

	//Output 8x1 line of custom pixel data
	for(int x = tile_pixel; x < (tile_pixel + 8); x++)
	{
		//Render 1:1
		if(cgfx::scaling_factor <= 1)
		{
			custom_color = cgfx_stat.obj_pixel_data[obj_id][x];

			if(custom_color == cgfx::transparency_color) { }
			else if((obj[sprite_id].bg_priority == 1) && (scanline_raw[lcd_stat.scanline_pixel_counter] != 0)) { }
			else { scanline_buffer[lcd_stat.scanline_pixel_counter] = cgfx_stat.obj_pixel_data[obj_id][x]; }
		}

		//Render HD
		else
		{
			u32 pos = (lcd_stat.scanline_pixel_counter * cgfx::scaling_factor) + (lcd_stat.current_scanline * cgfx::scaling_factor * config::sys_width);
			u32 obj_pos = ((x - tile_pixel) * cgfx::scaling_factor) + (tile_line * cgfx::scale_squared * 8);
			u32 c = 0;

			if(lcd_stat.scanline_pixel_counter < 160)
			{
				for(int a = 0; a < cgfx::scaling_factor; a++)
				{
					for(int b = 0; b < cgfx::scaling_factor; b++)
					{
						c = obj[sprite_id].h_flip ? (cgfx::scaling_factor - b - 1) : b;
						custom_color = cgfx_stat.obj_pixel_data[obj_id][obj_pos + c];

						if(custom_color == cgfx::transparency_color) { }
						else if((obj[sprite_id].bg_priority == 1) && (scanline_raw[lcd_stat.scanline_pixel_counter] != 0)) { }
						else { hd_screen_buffer[pos + b] = cgfx_stat.obj_pixel_data[obj_id][obj_pos + c]; }
					}

					pos += config::sys_width;
					obj_pos += (8 * cgfx::scaling_factor);
				}
			}
		}

		lcd_stat.scanline_pixel_counter += counter;
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

		//Render CGFX
		u16 hash_addr = 0x8000 + (obj[sprite_id].tile_number << 4);
		if((cgfx::load_cgfx) && (has_hash(hash_addr, cgfx_stat.current_obj_hash[sprite_id]))) { render_cgfx_gbc_obj_scanline(sprite_id); }

		//Render original pixel data
		else
		{
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
				else if((obj[sprite_id].bg_priority == 0) && (scanline_priority[lcd_stat.scanline_pixel_counter] == 1) 
				&& (scanline_raw[lcd_stat.scanline_pixel_counter] != 0)) { draw_obj_pixel = false; }
				
				//Render sprite pixel
				if(draw_obj_pixel)
				{
					scanline_buffer[lcd_stat.scanline_pixel_counter++] = lcd_stat.obj_colors_final[tile_pixel][obj[sprite_id].color_palette_number];

					u8 last_scanline_pixel = lcd_stat.scanline_pixel_counter - 1;

					//Render HD
					if((cgfx::load_cgfx) && (cgfx::scaling_factor > 1) && (last_scanline_pixel < 160))
					{
						u32 pos = (last_scanline_pixel * cgfx::scaling_factor) + (lcd_stat.current_scanline * cgfx::scaling_factor * config::sys_width);
			
						for(int a = 0; a < cgfx::scaling_factor; a++)
						{
							for(int b = 0; b < cgfx::scaling_factor; b++)
							{
								hd_screen_buffer[pos + b] = scanline_buffer[last_scanline_pixel];
							}
	
							pos += config::sys_width;
						}
					}
				}

				//Move onto next pixel in scanline to see if sprite rendering occurs
				else { lcd_stat.scanline_pixel_counter++; }
			}
		}
	}
}

/****** Renders pixels for OBJs (per-scanline) - GBC CGFX version ******/
void DMG_LCD::render_cgfx_gbc_obj_scanline(u8 sprite_id)
{
	//Set the current pixel to start obj rendering
	lcd_stat.scanline_pixel_counter = obj[sprite_id].x;
		
	//Determine which line of the tiles to generate pixels for this scanline		
	u8 tile_line = (lcd_stat.current_scanline - obj[sprite_id].y);
	if(obj[sprite_id].v_flip) { tile_line = (lcd_stat.obj_size == 8) ? lcd_stat.flip_8[tile_line] : lcd_stat.flip_16[tile_line]; }

	//If sprite is below BG and BG raw color is non-zero, abort rendering this pixel
	if((obj[sprite_id].bg_priority == 1) && (scanline_raw[lcd_stat.scanline_pixel_counter] != 0)) { return; }

	//Grab the ID of this hash to pull custom pixel data
	u16 obj_id = cgfx_stat.m_id[cgfx_stat.last_id];

	//Grab the auto-bright property of this hash
	u8 auto_bright = cgfx_stat.m_auto_bright[cgfx_stat.last_id];

	u16 tile_pixel = (8 * tile_line);
	u32 custom_color = 0;

	//Account for horizontal flipping
	lcd_stat.scanline_pixel_counter = obj[sprite_id].h_flip ? (lcd_stat.scanline_pixel_counter + 7) : lcd_stat.scanline_pixel_counter;
	s16 counter = obj[sprite_id].h_flip ? -1 : 1;

	//Output 8x1 line of custom pixel data
	for(int x = tile_pixel; x < (tile_pixel + 8); x++)
	{
		if(!lcd_stat.bg_enable) { scanline_priority[lcd_stat.scanline_pixel_counter] = 0; }

		//Render 1:1
		if(cgfx::scaling_factor <= 1)
		{
			custom_color = cgfx_stat.obj_pixel_data[obj_id][x];

			//Adjust pixel brightness if EXT_AUTO_BRIGHT is set
			if((auto_bright) && (custom_color != cgfx::transparency_color)) 
			{
				custom_color = adjust_pixel_brightness(custom_color, obj[sprite_id].color_palette_number, 1);
			}

			if(custom_color == cgfx::transparency_color) { }
			else if((obj[sprite_id].bg_priority == 1) && (scanline_raw[lcd_stat.scanline_pixel_counter] != 0)) { }
			else if((obj[sprite_id].bg_priority == 0) && (scanline_priority[lcd_stat.scanline_pixel_counter] == 1) 
			&& (scanline_raw[lcd_stat.scanline_pixel_counter] != 0)) { }
			else { scanline_buffer[lcd_stat.scanline_pixel_counter] = custom_color; }
		}

		//Render HD
		else
		{
			u32 pos = (lcd_stat.scanline_pixel_counter * cgfx::scaling_factor) + (lcd_stat.current_scanline * cgfx::scaling_factor * config::sys_width);
			u32 obj_pos = ((x - tile_pixel) * cgfx::scaling_factor) + (tile_line * cgfx::scale_squared * 8);
			u32 c = 0;
			
			if(lcd_stat.scanline_pixel_counter < 160)
			{
				for(int a = 0; a < cgfx::scaling_factor; a++)
				{
					for(int b = 0; b < cgfx::scaling_factor; b++)
					{
						c = obj[sprite_id].h_flip ? (cgfx::scaling_factor - b - 1) : b;
						custom_color = cgfx_stat.obj_pixel_data[obj_id][obj_pos + c];

						//Adjust pixel brightness if EXT_AUTO_BRIGHT is set
						if((auto_bright) && (custom_color != cgfx::transparency_color)) 
						{
							custom_color = adjust_pixel_brightness(custom_color, obj[sprite_id].color_palette_number, 1);
						}

						if(custom_color == cgfx::transparency_color) { }
						else if((obj[sprite_id].bg_priority == 1) && (scanline_raw[lcd_stat.scanline_pixel_counter] != 0)) { }
						else if((obj[sprite_id].bg_priority == 0) && (scanline_priority[lcd_stat.scanline_pixel_counter] == 1) 
						&& (scanline_raw[lcd_stat.scanline_pixel_counter] != 0)) { }
						else { hd_screen_buffer[pos + b] = custom_color; }
					}

					pos += config::sys_width;
					obj_pos += (8 * cgfx::scaling_factor);
				}
			}
		}

		lcd_stat.scanline_pixel_counter += counter;
	}
}

/****** Update background color palettes on the GBC ******/
void DMG_LCD::update_bg_colors()
{
	u8 hi_lo = (mem->memory_map[REG_BCPS] & 0x1);
	u8 color = (mem->memory_map[REG_BCPS] >> 1) & 0x3;
	u8 palette = (mem->memory_map[REG_BCPS] >> 3) & 0x7;

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

	//CGFX - Update BG hashes
	if((cgfx::load_cgfx) || (cgfx::auto_dump_bg)) 
	{
		u8 temp_vram_bank = mem->vram_bank;
		mem->vram_bank = 1;

		for(u16 x = 0; x < 2048; x++)
		{
			u8 bg_pal = (mem->read_u8(0x9800 + x) & 0x7);

			if(bg_pal == palette)
			{
				cgfx_stat.update_map = true;
				cgfx_stat.bg_map_update_list[x] = true;
			}
		}

		mem->vram_bank = temp_vram_bank;
	}

	//CGFX - Calculate average palette brightness
	if(cgfx::load_cgfx)
	{
		u16 avg = 0;

		for(u8 x = 0; x < 4; x++) { avg += util::get_brightness_fast(lcd_stat.bg_colors_final[x][palette]); }

		avg >>= 2;
		cgfx_stat.bg_pal_brightness[palette] = avg;
	}
}

/****** Update sprite color palettes on the GBC ******/
void DMG_LCD::update_obj_colors()
{
	u8 hi_lo = (mem->memory_map[REG_OCPS] & 0x1);
	u8 color = (mem->memory_map[REG_OCPS] >> 1) & 0x3;
	u8 palette = (mem->memory_map[REG_OCPS] >> 3) & 0x7;

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

	//CGFX - Update OBJ hashes
	if((cgfx::load_cgfx) || (cgfx::auto_dump_obj)) 
	{
		for(int x = 0; x < 40; x++)
		{
			if(obj[x].color_palette_number == palette) { update_gbc_obj_hash(x); }
		}
	}

	//CGFX - Calculate average palette brightness
	if(cgfx::load_cgfx)
	{
		u16 avg = 0;

		for(u8 x = 0; x < 4; x++) { avg += util::get_brightness_fast(lcd_stat.obj_colors_final[x][palette]); }

		avg >>= 2;
		cgfx_stat.obj_pal_brightness[palette] = avg;
	}
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

					//CGFX - Update BG Hashes
					if(cgfx_stat.update_bg)
					{
						if(config::gb_type == 1)
						{
							for(int x = 0; x < 384; x++)
							{
								if(cgfx_stat.bg_update_list[x]) { update_dmg_bg_hash(x); }
							}
						}

						else
						{
							//Check for updates based on tile data
							for(int tile_number = 0; tile_number < 256; tile_number++)
							{
								//Cycle through all tile numbers that were updated
								if(cgfx_stat.bg_tile_update_list[tile_number])
								{
									//Scan BG map for all tiles that use this tile number
									for(int x = 0; x < 2048; x++)
									{
										if(mem->video_ram[0][x] == tile_number) { update_gbc_bg_hash(0x9800 + x); }
									}

									cgfx_stat.bg_tile_update_list[tile_number] = false;
								}
							}
						}

						cgfx_stat.update_bg = false;
					}

					if(cgfx_stat.update_map)
					{
						//Update specific map entries
						for(int x = 0; x < 2048; x++)
						{
							if(cgfx_stat.bg_map_update_list[x]) 
							{
								update_gbc_bg_hash(0x9800 + x);
								cgfx_stat.bg_map_update_list[x] = false;
							}
						}
						
						cgfx_stat.update_map = false;
					}
					
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

				//Check for screen resize - DMG/GBC stretch
				if((config::request_resize) && (config::resize_mode > 0))
				{
					config::sys_width = 240;
					config::sys_height = 160;
					screen_buffer.clear();
					screen_buffer.resize(0x9600, 0xFFFFFFFF);
					init();
					if(config::sdl_render) { config::request_resize = false; }
				}

				//Check for screen resize - Normal DMG/GBC screen
				else if(config::request_resize)
				{
					config::sys_width = 160;
					config::sys_height = 144;
					screen_buffer.clear();
					screen_buffer.resize(0x5A00, 0xFFFFFFFF);
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
						//Lock source surface
						if(SDL_MUSTLOCK(final_screen)){ SDL_LockSurface(final_screen); }
						u32* out_pixel_data = (u32*)final_screen->pixels;

						//Regular 1:1 framebuffer rendering
						if((!cgfx::load_cgfx) || (cgfx::scaling_factor <= 1))
						{
							for(int a = 0; a < screen_buffer.size(); a++) { out_pixel_data[a] = screen_buffer[a]; }
						}

						//HD CGFX framebuffer rendering
						else if((cgfx::load_cgfx) && (cgfx::scaling_factor > 1))
						{
							for(int a = 0; a < hd_screen_buffer.size(); a++) { out_pixel_data[a] = hd_screen_buffer[a]; }
						}

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

					//Use external rendering method (GUI)
					else 
					{
						if(!config::use_opengl)
						{
							//Regular 1:1 framebuffer rendering
							if((!cgfx::load_cgfx) || (cgfx::scaling_factor <= 1)) { config::render_external_sw(screen_buffer); }
						
							//HD CGFX framebuffer rendering
							else if((cgfx::load_cgfx) && (cgfx::scaling_factor > 1)) { config::render_external_sw(hd_screen_buffer); }
						}

						else
						{
							//Lock source surface
							if(SDL_MUSTLOCK(final_screen)){ SDL_LockSurface(final_screen); }
							u32* out_pixel_data = (u32*)final_screen->pixels;

							//Regular 1:1 framebuffer rendering
							if((!cgfx::load_cgfx) || (cgfx::scaling_factor <= 1))
							{
								for(int a = 0; a < screen_buffer.size(); a++) { out_pixel_data[a] = screen_buffer[a]; }
							}

							//HD CGFX framebuffer rendering
							else if((cgfx::load_cgfx) && (cgfx::scaling_factor > 1))
							{
								for(int a = 0; a < hd_screen_buffer.size(); a++) { out_pixel_data[a] = hd_screen_buffer[a]; }
							}

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
					SDL_WM_SetCaption(config::title.str().c_str(), NULL);
					fps_count = 0; 
				}

				//Process gyroscope
				if(mem->cart.mbc_type == DMG_MMU::MBC7) { mem->g_pad->process_gyroscope(); }
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
					//It will sit at Line 0 for 456 before entering Mode 2 properly
					//Line 0 STAT-LYC IRQs should be triggered here
					if(lcd_stat.current_scanline == 153)
					{
						lcd_stat.current_scanline = 0;
						mem->memory_map[REG_LY] = lcd_stat.current_scanline;
						scanline_compare();
					}

					//After sitting on Line 0 for 456 cycles, reset LCD clock, scanline count
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
