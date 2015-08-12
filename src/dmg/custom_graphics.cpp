// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : custom_gfx.cpp
// Date : July 23, 2015
// Description : Game Boy Enhanced custom graphics (DMG/GBC version)
//
// Handles dumping original BG and Sprite tiles
// Handles loading custom pixel data

#include "common/hash.h"
#include "common/util.h"
#include "common/cgfx_common.h"
#include "lcd.h"

/****** Loads the manifest file ******/
bool DMG_LCD::load_manifest(std::string filename) { }

/****** Loads 24-bit data from source and converts it to 32-bit ARGB ******/
void DMG_LCD::load_image_data(int size, SDL_Surface* custom_source, u32 custom_dest[]) { }

/****** Dumps DMG OBJ tile from selected memory address ******/
void DMG_LCD::dump_dmg_obj(u8 obj_index) 
{
	SDL_Surface* obj_dump = NULL;
	u8 obj_height = 0;
	bool add_hash = true;

	cgfx_stat.current_obj_hash[obj_index] = "";
	std::string final_hash = "";

	//Generate salt for hash - Use OBJ palettes
	u16 hash_salt = ((mem->memory_map[REG_OBP0] << 8) | mem->memory_map[REG_OBP1]);

	//Determine if in 8x8 or 8x16 mode
	obj_height = (mem->memory_map[REG_LCDC] & 0x04) ? 16 : 8;

	//Grab OBJ tile addr from index
	u16 obj_tile_addr = 0x8000 + (obj[obj_index].tile_number << 4);

	//Create a hash for this OBJ tile
	for(int x = 0; x < obj_height/2; x++)
	{
		u16 temp_hash = mem->read_u8((x * 4) + obj_tile_addr);
		temp_hash << 8;
		temp_hash += mem->read_u8((x * 4) + obj_tile_addr + 1);
		temp_hash = temp_hash ^ hash_salt;
		cgfx_stat.current_obj_hash[obj_index] += hash::raw_to_64(temp_hash);

		temp_hash = mem->read_u8((x * 4) + obj_tile_addr + 2);
		temp_hash << 8;
		temp_hash += mem->read_u8((x * 4) + obj_tile_addr + 3);
		temp_hash = temp_hash ^ hash_salt;
		cgfx_stat.current_obj_hash[obj_index] += hash::raw_to_64(temp_hash);
	}

	final_hash = cgfx_stat.current_obj_hash[obj_index];

	//Update the OBJ hash list
	for(int x = 0; x < cgfx_stat.obj_hash_list.size(); x++)
	{
		if(final_hash == cgfx_stat.obj_hash_list[x]) { add_hash = false; return; }
	}

	//For new OBJs, dump BMP file
	if(add_hash) 
	{ 
		cgfx_stat.obj_hash_list.push_back(final_hash);

		obj_dump = SDL_CreateRGBSurface(SDL_SWSURFACE, 8, obj_height, 32, 0, 0, 0, 0);
		std::string dump_file = "Dump/Sprites/" + final_hash + ".bmp";

		if(SDL_MUSTLOCK(obj_dump)) { SDL_LockSurface(obj_dump); }

		u32* dump_pixel_data = (u32*)obj_dump->pixels;
		u8 pixel_counter = 0;

		//Generate RGBA values of the sprite for the dump file
		for(int x = 0; x < obj_height; x++)
		{
			//Grab bytes from VRAM representing 8x1 pixel data
			u16 raw_data = mem->read_u16(obj_tile_addr);

			//Grab individual pixels
			for(int y = 7; y >= 0; y--)
			{
				u8 raw_pixel = ((raw_data >> 8) & (1 << y)) ? 2 : 0;
				raw_pixel |= (raw_data & (1 << y)) ? 1 : 0;

				switch(lcd_stat.obp[raw_pixel][obj[obj_index].palette_number])
				{
					case 0: 
						dump_pixel_data[pixel_counter++] = 0xFFFFFFFF;
						break;

					case 1: 
						dump_pixel_data[pixel_counter++] = 0xFFC0C0C0;
						break;

					case 2: 
						dump_pixel_data[pixel_counter++] = 0xFF606060;
						break;

					case 3: 
						dump_pixel_data[pixel_counter++] = 0xFF000000;
						break;
				}
			}

			obj_tile_addr += 2;
		}

		if(SDL_MUSTLOCK(obj_dump)) { SDL_UnlockSurface(obj_dump); }

		//Save to BMP
		std::cout<<"LCD::Saving Sprite - " << dump_file << "\n";
		SDL_SaveBMP(obj_dump, dump_file.c_str());
	}
}

/****** Dumps GBC OBJ tile from selected memory address ******/
void DMG_LCD::dump_gbc_obj(u8 obj_index) 
{
	SDL_Surface* obj_dump = NULL;
	u8 obj_height = 0;
	bool add_hash = true;

	cgfx_stat.current_obj_hash[obj_index] = "";
	std::string final_hash = "";

	//Determine if in 8x8 or 8x16 mode
	obj_height = (mem->memory_map[REG_LCDC] & 0x04) ? 16 : 8;

	//Grab OBJ tile addr from index
	u16 obj_tile_addr = 0x8000 + (obj[obj_index].tile_number << 4);

	//Grab VRAM bank
	u8 old_vram_bank = mem->vram_bank;
	mem->vram_bank = obj[obj_index].vram_bank;

	//Create a hash for this OBJ tile
	for(int x = 0; x < obj_height/2; x++)
	{
		u16 temp_hash = mem->read_u8((x * 4) + obj_tile_addr);
		temp_hash << 8;
		temp_hash += mem->read_u8((x * 4) + obj_tile_addr + 1);
		cgfx_stat.current_obj_hash[obj_index] += hash::raw_to_64(temp_hash);

		temp_hash = mem->read_u8((x * 4) + obj_tile_addr + 2);
		temp_hash << 8;
		temp_hash += mem->read_u8((x * 4) + obj_tile_addr + 3);
		cgfx_stat.current_obj_hash[obj_index] += hash::raw_to_64(temp_hash);
	}

	//Prepend the hues to each hash
	std::string hue_data = "";
	
	for(int x = 0; x < 4; x++)
	{
		util::hsv color = util::rgb_to_hsv(lcd_stat.obj_colors_final[x][obj[obj_index].palette_number]);
		u8 hue = (color.hue / 10);
		hue_data += hash::base_64_index[hue];
	}

	cgfx_stat.current_obj_hash[obj_index] = hue_data + "_" + cgfx_stat.current_obj_hash[obj_index];

	final_hash = cgfx_stat.current_obj_hash[obj_index];

	//Update the OBJ hash list
	for(int x = 0; x < cgfx_stat.obj_hash_list.size(); x++)
	{
		if(final_hash == cgfx_stat.obj_hash_list[x]) { add_hash = false; return; }
	}

	//For new OBJs, dump BMP file
	if(add_hash) 
	{ 
		cgfx_stat.obj_hash_list.push_back(final_hash);

		obj_dump = SDL_CreateRGBSurface(SDL_SWSURFACE, 8, obj_height, 32, 0, 0, 0, 0);
		std::string dump_file = "Dump/Sprites/" + final_hash + ".bmp";

		if(SDL_MUSTLOCK(obj_dump)) { SDL_LockSurface(obj_dump); }

		u32* dump_pixel_data = (u32*)obj_dump->pixels;
		u8 pixel_counter = 0;

		//Generate RGBA values of the sprite for the dump file
		for(int x = 0; x < obj_height; x++)
		{
			//Grab bytes from VRAM representing 8x1 pixel data
			u16 raw_data = mem->read_u16(obj_tile_addr);

			//Grab individual pixels
			for(int y = 7; y >= 0; y--)
			{
				u8 raw_pixel = ((raw_data >> 8) & (1 << y)) ? 2 : 0;
				raw_pixel |= (raw_data & (1 << y)) ? 1 : 0;

				dump_pixel_data[pixel_counter++] = lcd_stat.obj_colors_final[raw_pixel][obj[obj_index].color_palette_number];
			}

			obj_tile_addr += 2;
		}

		if(SDL_MUSTLOCK(obj_dump)) { SDL_UnlockSurface(obj_dump); }

		//Save to BMP
		std::cout<<"LCD::Saving Sprite - " << dump_file << "\n";
		SDL_SaveBMP(obj_dump, dump_file.c_str());
	}

	//Reset VRAM bank
	mem->vram_bank = old_vram_bank;
}

/****** Dumps DMG BG tile from selected memory address ******/
void DMG_LCD::dump_dmg_bg(u16 bg_index) 
{
	SDL_Surface* bg_dump = NULL;
	bool add_hash = true;

	cgfx_stat.current_bg_hash[bg_index] = "";
	std::string final_hash = "";

	//Generate salt for hash - Use BG palette (mirrored to 16-bits)
	u16 hash_salt = ((mem->memory_map[REG_BGP] << 8) | mem->memory_map[REG_BGP]);

	//Grab OBJ tile addr from index
	u16 bg_tile_addr = (bg_index * 16) + 0x8000;

	//Create a hash for this BG tile
	for(int x = 0; x < 4; x++)
	{
		u16 temp_hash = mem->read_u8((x * 4) + bg_tile_addr);
		temp_hash << 8;
		temp_hash += mem->read_u8((x * 4) + bg_tile_addr + 1);
		temp_hash = temp_hash ^ hash_salt;
		cgfx_stat.current_bg_hash[bg_index] += hash::raw_to_64(temp_hash);

		temp_hash = mem->read_u8((x * 4) + bg_tile_addr + 2);
		temp_hash << 8;
		temp_hash += mem->read_u8((x * 4) + bg_tile_addr + 3);
		temp_hash = temp_hash ^ hash_salt;
		cgfx_stat.current_bg_hash[bg_index] += hash::raw_to_64(temp_hash);
	}

	final_hash = cgfx_stat.current_bg_hash[bg_index];

	//Update the OBJ hash list
	for(int x = 0; x < cgfx_stat.bg_hash_list.size(); x++)
	{
		if(final_hash == cgfx_stat.bg_hash_list[x]) { add_hash = false; return; }
	}

	//For new OBJs, dump BMP file
	if(add_hash) 
	{ 
		cgfx_stat.bg_hash_list.push_back(final_hash);

		bg_dump = SDL_CreateRGBSurface(SDL_SWSURFACE, 8, 8, 32, 0, 0, 0, 0);
		std::string dump_file = "Dump/BG/" + final_hash + ".bmp";

		if(SDL_MUSTLOCK(bg_dump)) { SDL_LockSurface(bg_dump); }

		u32* dump_pixel_data = (u32*)bg_dump->pixels;
		u8 pixel_counter = 0;

		//Generate RGBA values of the sprite for the dump file
		for(int x = 0; x < 8; x++)
		{
			//Grab bytes from VRAM representing 8x1 pixel data
			u16 raw_data = mem->read_u16(bg_tile_addr);

			//Grab individual pixels
			for(int y = 7; y >= 0; y--)
			{
				u8 raw_pixel = ((raw_data >> 8) & (1 << y)) ? 2 : 0;
				raw_pixel |= (raw_data & (1 << y)) ? 1 : 0;

				switch(lcd_stat.bgp[raw_pixel])
				{
					case 0: 
						dump_pixel_data[pixel_counter++] = 0xFFFFFFFF;
						break;

					case 1: 
						dump_pixel_data[pixel_counter++] = 0xFFC0C0C0;
						break;

					case 2: 
						dump_pixel_data[pixel_counter++] = 0xFF606060;
						break;

					case 3: 
						dump_pixel_data[pixel_counter++] = 0xFF000000;
						break;
				}
			}

			bg_tile_addr += 2;
		}

		if(SDL_MUSTLOCK(bg_dump)) { SDL_UnlockSurface(bg_dump); }

		//Save to BMP
		std::cout<<"LCD::Saving Background Tile - " << dump_file << "\n";
		SDL_SaveBMP(bg_dump, dump_file.c_str());
	}
}

/****** Dumps GBC BG tile from selected memory address ******/
void DMG_LCD::dump_gbc_bg(u16 bg_index) 
{
	SDL_Surface* bg_dump = NULL;
	bool add_hash = true;

	cgfx_stat.current_bg_hash[bg_index] = "";
	std::string final_hash = "";

	//Grab OBJ tile addr from index
	u16 bg_tile_addr = 0x8000 + (bg_index << 4);

	//Set VRAM bank
	u8 old_vram_bank = mem->vram_bank;
	mem->vram_bank = cgfx::gbc_bg_vram_bank;

	//Create a hash for this BG tile
	for(int x = 0; x < 4; x++)
	{
		u16 temp_hash = mem->read_u8((x * 4) + bg_tile_addr);
		temp_hash << 8;
		temp_hash += mem->read_u8((x * 4) + bg_tile_addr + 1);
		cgfx_stat.current_bg_hash[bg_index] += hash::raw_to_64(temp_hash);

		temp_hash = mem->read_u8((x * 4) + bg_tile_addr + 2);
		temp_hash << 8;
		temp_hash += mem->read_u8((x * 4) + bg_tile_addr + 3);
		cgfx_stat.current_bg_hash[bg_index] += hash::raw_to_64(temp_hash);
	}

	//Prepend the hues to each hash
	std::string hue_data = "";
	
	for(int x = 0; x < 4; x++)
	{
		util::hsv color = util::rgb_to_hsv(lcd_stat.bg_colors_final[x][cgfx::gbc_bg_color_pal]);
		u8 hue = (color.hue / 10);
		hue_data += hash::base_64_index[hue];
	}

	cgfx_stat.current_bg_hash[bg_index] = hue_data + "_" + cgfx_stat.current_bg_hash[bg_index];

	final_hash = cgfx_stat.current_bg_hash[bg_index];

	//Update the BG hash list
	for(int x = 0; x < cgfx_stat.bg_hash_list.size(); x++)
	{
		if(final_hash == cgfx_stat.bg_hash_list[x]) { add_hash = false; return; }
	}

	//For new BGs, dump BMP file
	if(add_hash) 
	{ 
		cgfx_stat.bg_hash_list.push_back(final_hash);

		bg_dump = SDL_CreateRGBSurface(SDL_SWSURFACE, 8, 8, 32, 0, 0, 0, 0);
		std::string dump_file = "Dump/Sprites/" + final_hash + ".bmp";

		if(SDL_MUSTLOCK(bg_dump)) { SDL_LockSurface(bg_dump); }

		u32* dump_pixel_data = (u32*)bg_dump->pixels;
		u8 pixel_counter = 0;

		//Generate RGBA values of the sprite for the dump file
		for(int x = 0; x < 8; x++)
		{
			//Grab bytes from VRAM representing 8x1 pixel data
			u16 raw_data = mem->read_u16(bg_tile_addr);

			//Grab individual pixels
			for(int y = 7; y >= 0; y--)
			{
				u8 raw_pixel = ((raw_data >> 8) & (1 << y)) ? 2 : 0;
				raw_pixel |= (raw_data & (1 << y)) ? 1 : 0;

				dump_pixel_data[pixel_counter++] = lcd_stat.bg_colors_final[raw_pixel][cgfx::gbc_bg_color_pal];
			}

			bg_tile_addr += 2;
		}

		if(SDL_MUSTLOCK(bg_dump)) { SDL_UnlockSurface(bg_dump); }

		//Save to BMP
		std::cout<<"LCD::Saving Sprite - " << dump_file << "\n";
		SDL_SaveBMP(bg_dump, dump_file.c_str());
	}

	//Reset VRAM bank
	mem->vram_bank = old_vram_bank;
}
