// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : custom_gfx.cpp
// Date : July 23, 2015
// Description : Game Boy Enhanced custom graphics (DMG/GBC version)
//
// Handles dumping original BG and Sprite tiles
// Handles loading custom pixel data

#include <fstream>
#include <sstream>

#include "common/hash.h"
#include "common/util.h"
#include "common/cgfx_common.h"
#include "lcd.h"

/****** Loads the manifest file ******/
bool DMG_LCD::load_manifest(std::string filename) 
{
	std::ifstream file(filename.c_str(), std::ios::in); 
	std::string input_line = "";
	std::string line_char = "";

	//Clear existing hash data
	cgfx_stat.obj_hash_list.clear();
	cgfx_stat.bg_hash_list.clear();

	//Clear existing manifest data
	cgfx_stat.manifest.clear();
	cgfx_stat.m_hashes.clear();
	cgfx_stat.m_files.clear();
	cgfx_stat.m_types.clear();
	cgfx_stat.m_vram_addr.clear();
	cgfx_stat.m_auto_bright.clear();

	if(!file.is_open())
	{
		std::cout<<"CGFX::Could not open manifest file " << filename << ". Check file path or permissions. \n";
		return false; 
	}

	//Cycle through whole file, line-by-line
	while(getline(file, input_line))
	{
		line_char = input_line[0];
		bool ignore = false;	
	
		//Check if line starts with [ - if not, skip line
		if(line_char == "[")
		{
			std::string line_item = "";

			//Cycle through line, character-by-character
			for(int x = 0; ++x < input_line.length();)
			{
				line_char = input_line[x];

				//Check for single-quotes, don't parse ":" or "]" within them
				if((line_char == "'") && (!ignore)) { ignore = true; }
				else if((line_char == "'") && (ignore)) { ignore = false; }

				//Check the character for item limiter : or ] - Push to Vector
				else if(((line_char == ":") || (line_char == "]")) && (!ignore)) 
				{
					cgfx_stat.manifest.push_back(line_item);
					line_item = ""; 
				}

				else { line_item += line_char; }
			}
		}
	}
	
	file.close();

	//Determine if manifest is properly formed (roughly)
	//Each manifest entry should have 5 parameters
	if((cgfx_stat.manifest.size() % 5) != 0)
	{
		std::cout<<"CGFX::Manifest file " << filename << " has some missing parameters for some entries. \n";
		return false;
	}

	//Parse entries
	for(int x = 0; x < cgfx_stat.manifest.size();)
	{
		//Grab hash
		std::string hash = cgfx_stat.manifest[x++];

		//Grab file associated with hash
		cgfx_stat.m_files.push_back(cgfx_stat.manifest[x++]);

		//Grab the type
		std::stringstream type_stream(cgfx_stat.manifest[x++]);
		int type_byte = 0;
		type_stream >> type_byte;
		cgfx_stat.m_types.push_back(type_byte);

		switch(type_byte)
		{
			//DMG, GBC, or GBA OBJ
			case 1:
			case 2:
			case 3:
				cgfx_stat.m_id.push_back(cgfx_stat.obj_hash_list.size());
				cgfx_stat.obj_hash_list.push_back(hash);
				cgfx_stat.m_hashes.push_back(hash);
				break;

			//DMG, GBC, or GBA BG
			case 10:
			case 20:
			case 30:
				cgfx_stat.m_id.push_back(cgfx_stat.bg_hash_list.size());
				cgfx_stat.bg_hash_list.push_back(hash);
				cgfx_stat.m_hashes.push_back(hash);
				break;
		
			//Undefined type
			default:
				std::cout<<"CGFX::Undefined hash type " << (int)type_byte << "\n";
				return false;
		}

		//Load image based on filename and hash type
		if(!load_image_data()) { return false; }

		//EXT_VRAM_ADDR
		std::stringstream vram_stream(cgfx_stat.manifest[x++]);
		u32 vram_address = 0;
		util::from_hex_str(vram_stream.str(), vram_address);
		cgfx_stat.m_vram_addr.push_back(vram_address);	

		//EXT_AUTO_BRIGHT
		std::stringstream bright_stream(cgfx_stat.manifest[x++]);
		u32 bright_value = 0;
		bright_stream >> bright_value;
		cgfx_stat.m_auto_bright.push_back(bright_value);
	}

	std::cout<<"CGFX::" << filename << " loaded successfully\n"; 

	return true;
}

/****** Loads 24-bit data from source and converts it to 32-bit ARGB ******/
bool DMG_LCD::load_image_data() 
{
	//TODO - PNG loading via SDL_image

	//NOTE - Only call this function during manifest loading, immediately after the filename AND type are parsed
	std::string filename = config::data_path + cgfx_stat.m_files.back();
	SDL_Surface* source = SDL_LoadBMP(filename.c_str());

	if(source == NULL)
	{
		std::cout<<"GBE::CGFX - Could not load " << filename << "\n";
		return false;
	}

	int custom_bpp = source->format->BitsPerPixel;

	if(custom_bpp != 24)
	{
		std::cout<<"GBE::CGFX - " << filename << " has " << custom_bpp << "bpp instead of 24bpp \n";
		return false;
	}
		
	std::vector<u32> cgfx_pixels;

	//Load 24-bit data
	u8* pixel_data = (u8*)source->pixels;

	for(int a = 0, b = 0; a < (source->w * source->h); a++, b+=3)
	{
		cgfx_pixels.push_back(0xFF000000 | (pixel_data[b+2] << 16) | (pixel_data[b+1] << 8) | (pixel_data[b]));
	}

	//Store OBJ pixel data
	if(cgfx_stat.m_types.back() < 10) { cgfx_stat.obj_pixel_data.push_back(cgfx_pixels); }

	//Store BG pixel data
	else { cgfx_stat.bg_pixel_data.push_back(cgfx_pixels); }
}

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
		temp_hash <<= 8;
		temp_hash += mem->read_u8((x * 4) + obj_tile_addr + 1);
		temp_hash = temp_hash ^ hash_salt;
		cgfx_stat.current_obj_hash[obj_index] += hash::raw_to_64(temp_hash);

		temp_hash = mem->read_u8((x * 4) + obj_tile_addr + 2);
		temp_hash <<= 8;
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
		
		std::string dump_file =  "";
		if(cgfx::dump_name == "") { dump_file = config::data_path + cgfx::dump_obj_path + final_hash + ".bmp"; }
		else { dump_file = config::data_path + cgfx::dump_obj_path + cgfx::dump_name; }

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

		//Ignore blank or empty dumps
		if(cgfx::ignore_blank_dumps)
		{
			bool blank = true;

			for(int x = 1; x < pixel_counter; x++)
			{
				if(dump_pixel_data[0] != dump_pixel_data[x]) { blank = false; break; }
			}

			if(blank) { return; }
		}

		//Save to BMP
		std::cout<<"LCD::Saving Sprite - " << dump_file << "\n";
		SDL_SaveBMP(obj_dump, dump_file.c_str());
	}

	//Save CGFX data
	cgfx::last_hash = final_hash;
	cgfx::last_vram_addr = 0x8000 + (obj[obj_index].tile_number << 4);
	cgfx::last_type = 1;
	cgfx::last_palette = 0;
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
		temp_hash <<= 8;
		temp_hash += mem->read_u8((x * 4) + obj_tile_addr + 1);
		cgfx_stat.current_obj_hash[obj_index] += hash::raw_to_64(temp_hash);

		temp_hash = mem->read_u8((x * 4) + obj_tile_addr + 2);
		temp_hash <<= 8;
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
		if(final_hash == cgfx_stat.obj_hash_list[x]) { mem->vram_bank = old_vram_bank; return; }
	}

	//For new OBJs, dump BMP file
	if(add_hash) 
	{ 
		cgfx_stat.obj_hash_list.push_back(final_hash);

		obj_dump = SDL_CreateRGBSurface(SDL_SWSURFACE, 8, obj_height, 32, 0, 0, 0, 0);

		std::string dump_file =  "";
		if(cgfx::dump_name == "") { dump_file = config::data_path + cgfx::dump_obj_path + final_hash + ".bmp"; }
		else { dump_file = config::data_path + cgfx::dump_obj_path + cgfx::dump_name; }

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

		//Ignore blank or empty dumps
		if(cgfx::ignore_blank_dumps)
		{
			bool blank = true;

			for(int x = 1; x < pixel_counter; x++)
			{
				if(dump_pixel_data[0] != dump_pixel_data[x]) { blank = false; break; }
			}

			if(blank) { return; }
		}

		//Save to BMP
		std::cout<<"LCD::Saving Sprite - " << dump_file << "\n";
		SDL_SaveBMP(obj_dump, dump_file.c_str());
	}

	//Reset VRAM bank
	mem->vram_bank = old_vram_bank;

	//Save CGFX data
	cgfx::last_hash = final_hash;
	cgfx::last_vram_addr = 0x8000 + (obj[obj_index].tile_number << 4);
	cgfx::last_type = 2;
	cgfx::last_palette = obj[obj_index].color_palette_number + 1;
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
		temp_hash <<= 8;
		temp_hash += mem->read_u8((x * 4) + bg_tile_addr + 1);
		temp_hash = temp_hash ^ hash_salt;
		cgfx_stat.current_bg_hash[bg_index] += hash::raw_to_64(temp_hash);

		temp_hash = mem->read_u8((x * 4) + bg_tile_addr + 2);
		temp_hash <<= 8;
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

		std::string dump_file =  "";
		if(cgfx::dump_name == "") { dump_file = config::data_path + cgfx::dump_bg_path + final_hash + ".bmp"; }
		else { dump_file = config::data_path + cgfx::dump_bg_path + cgfx::dump_name; }

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

		//Ignore blank or empty dumps
		if(cgfx::ignore_blank_dumps)
		{
			bool blank = true;

			for(int x = 1; x < pixel_counter; x++)
			{
				if(dump_pixel_data[0] != dump_pixel_data[x]) { blank = false; break; }
			}

			if(blank) { return; }
		}

		//Save to BMP
		std::cout<<"LCD::Saving Background Tile - " << dump_file << "\n";
		SDL_SaveBMP(bg_dump, dump_file.c_str());
	}

	//Save CGFX data
	cgfx::last_hash = final_hash;
	cgfx::last_vram_addr = (bg_index << 4) + 0x8000;
	cgfx::last_type = 10;
	cgfx::last_palette = 0;
}

/****** Dumps GBC BG tile from selected memory address (GUI version) ******/
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
		temp_hash <<= 8;
		temp_hash += mem->read_u8((x * 4) + bg_tile_addr + 1);
		cgfx_stat.current_bg_hash[bg_index] += hash::raw_to_64(temp_hash);

		temp_hash = mem->read_u8((x * 4) + bg_tile_addr + 2);
		temp_hash <<= 8;
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
		if(final_hash == cgfx_stat.bg_hash_list[x]) { mem->vram_bank = old_vram_bank; return; }
	}

	//For new BGs, dump BMP file
	if(add_hash) 
	{ 
		cgfx_stat.bg_hash_list.push_back(final_hash);

		bg_dump = SDL_CreateRGBSurface(SDL_SWSURFACE, 8, 8, 32, 0, 0, 0, 0);

		std::string dump_file =  "";
		if(cgfx::dump_name == "") { dump_file = config::data_path + cgfx::dump_bg_path + final_hash + ".bmp"; }
		else { dump_file = config::data_path + cgfx::dump_bg_path + cgfx::dump_name; }

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

		//Ignore blank or empty dumps
		if(cgfx::ignore_blank_dumps)
		{
			bool blank = true;

			for(int x = 1; x < pixel_counter; x++)
			{
				if(dump_pixel_data[0] != dump_pixel_data[x]) { blank = false; break; }
			}

			if(blank) { return; }
		}

		//Save to BMP
		std::cout<<"LCD::Saving Background Tile - " << dump_file << "\n";
		SDL_SaveBMP(bg_dump, dump_file.c_str());
	}

	//Reset VRAM bank
	mem->vram_bank = old_vram_bank;

	//Save CGFX data
	cgfx::last_hash = final_hash;
	cgfx::last_vram_addr = (bg_index << 4) + 0x8000;
	cgfx::last_type = 20;
	cgfx::last_palette = cgfx::gbc_bg_color_pal + 1;
}

/****** Dumps GBC BG tile from selected memory address (Auto-dump version) ******/
void DMG_LCD::dump_gbc_bg(std::string final_hash, u16 bg_tile_addr, u8 palette) 
{
	SDL_Surface* bg_dump = SDL_CreateRGBSurface(SDL_SWSURFACE, 8, 8, 32, 0, 0, 0, 0);

	std::string dump_file =  "";
	if(cgfx::dump_name == "") { dump_file = config::data_path + cgfx::dump_bg_path + final_hash + ".bmp"; }
	else { dump_file = config::data_path + cgfx::dump_bg_path + cgfx::dump_name; }

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

			dump_pixel_data[pixel_counter++] = lcd_stat.bg_colors_final[raw_pixel][palette];
		}

		bg_tile_addr += 2;
	}

	if(SDL_MUSTLOCK(bg_dump)) { SDL_UnlockSurface(bg_dump); }

	//Ignore blank or empty dumps
	if(cgfx::ignore_blank_dumps)
	{
		bool blank = true;

		for(int x = 1; x < pixel_counter; x++)
		{
			if(dump_pixel_data[0] != dump_pixel_data[x]) { blank = false; break; }
		}

		if(blank) { return; }
	}

	//Save to BMP
	std::cout<<"LCD::Saving Background Tile - " << dump_file << "\n";
	SDL_SaveBMP(bg_dump, dump_file.c_str());

	//Save CGFX data
	cgfx::last_hash = final_hash;
	cgfx::last_vram_addr = bg_tile_addr - 16;
	cgfx::last_type = 20;
	cgfx::last_palette = palette + 1;
}

/****** Updates the current hash for the selected DMG OBJ ******/
void DMG_LCD::update_dmg_obj_hash(u8 obj_index)
{
	u8 obj_height = 0;

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
		temp_hash <<= 8;
		temp_hash += mem->read_u8((x * 4) + obj_tile_addr + 1);
		temp_hash = temp_hash ^ hash_salt;
		cgfx_stat.current_obj_hash[obj_index] += hash::raw_to_64(temp_hash);

		temp_hash = mem->read_u8((x * 4) + obj_tile_addr + 2);
		temp_hash <<= 8;
		temp_hash += mem->read_u8((x * 4) + obj_tile_addr + 3);
		temp_hash = temp_hash ^ hash_salt;
		cgfx_stat.current_obj_hash[obj_index] += hash::raw_to_64(temp_hash);
	}

	final_hash = cgfx_stat.current_obj_hash[obj_index];

	//Optionally auto-dump DMG OBJ
	if(cgfx::auto_dump_obj) { dump_dmg_obj(obj_index); }

	//Update the OBJ hash list
	for(int x = 0; x < cgfx_stat.obj_hash_list.size(); x++)
	{
		if(final_hash == cgfx_stat.obj_hash_list[x]) { return; }
	}

	cgfx_stat.obj_hash_list.push_back(final_hash);
}

/****** Updates the current hash for the selected GBC OBJ ******/
void DMG_LCD::update_gbc_obj_hash(u8 obj_index) 
{
	u8 obj_height = 0;

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
		temp_hash <<= 8;
		temp_hash += mem->read_u8((x * 4) + obj_tile_addr + 1);
		cgfx_stat.current_obj_hash[obj_index] += hash::raw_to_64(temp_hash);

		temp_hash = mem->read_u8((x * 4) + obj_tile_addr + 2);
		temp_hash <<= 8;
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

	//Optionally auto-dump GBC OBJ
	if(cgfx::auto_dump_obj) { dump_gbc_obj(obj_index); }

	//Reset VRAM bank
	mem->vram_bank = old_vram_bank;

	//Update the OBJ hash list
	for(int x = 0; x < cgfx_stat.obj_hash_list.size(); x++)
	{
		if(final_hash == cgfx_stat.obj_hash_list[x]) { return; }
	}
}

/****** Updates the current hash for the selected DMG BG tile ******/
void DMG_LCD::update_dmg_bg_hash(u16 bg_index)
{
	cgfx_stat.current_bg_hash[bg_index] = "";
	std::string final_hash = "";

	//Generate salt for hash - Use BG palette (mirrored to 16-bits)
	u16 hash_salt = ((mem->memory_map[REG_BGP] << 8) | mem->memory_map[REG_BGP]);

	//Grab BG tile addr from index
	u16 bg_tile_addr = (bg_index * 16) + 0x8000;

	//Create a hash for this BG tile
	for(int x = 0; x < 4; x++)
	{
		u16 temp_hash = mem->read_u8((x * 4) + bg_tile_addr);
		temp_hash <<= 8;
		temp_hash += mem->read_u8((x * 4) + bg_tile_addr + 1);
		temp_hash = temp_hash ^ hash_salt;
		cgfx_stat.current_bg_hash[bg_index] += hash::raw_to_64(temp_hash);

		temp_hash = mem->read_u8((x * 4) + bg_tile_addr + 2);
		temp_hash <<= 8;
		temp_hash += mem->read_u8((x * 4) + bg_tile_addr + 3);
		temp_hash = temp_hash ^ hash_salt;
		cgfx_stat.current_bg_hash[bg_index] += hash::raw_to_64(temp_hash);
	}

	final_hash = cgfx_stat.current_bg_hash[bg_index];

	//Optionally auto-dump DMG BG
	if(cgfx::auto_dump_bg) { dump_dmg_bg(bg_index); }

	//Update the BG hash list
	for(int x = 0; x < cgfx_stat.bg_hash_list.size(); x++)
	{
		if(final_hash == cgfx_stat.bg_hash_list[x]) { return; }
	}

	cgfx_stat.bg_hash_list.push_back(final_hash);
}

/****** Updates the current hash for the selected GBC BG tile ******/
void DMG_LCD::update_gbc_bg_hash(u16 map_addr)
{
	//Grab VRAM bank
	u8 old_vram_bank = mem->vram_bank;
	mem->vram_bank = 1;

	//Parse BG attributes
	u8 bg_attribute = mem->read_u8(map_addr);
	u8 palette = bg_attribute & 0x7;
	u8 vram_bank = (bg_attribute & 0x8) ? 1 : 0;

	mem->vram_bank = 0;
	u8 tile_number = mem->read_u8(map_addr);
	mem->vram_bank = vram_bank;

	//Convert tile number to signed if necessary
	if(lcd_stat.bg_tile_addr == 0x8800) { tile_number = lcd_stat.signed_tile_lut[tile_number]; }
	
	u16 bg_tile_addr = lcd_stat.bg_tile_addr + (tile_number << 4);
	u16 bg_index = map_addr - 0x9800;

	cgfx_stat.current_gbc_bg_hash[bg_index] = "";
	std::string final_hash = "";

	//Create a hash for this BG tile
	for(int x = 0; x < 4; x++)
	{
		u16 temp_hash = mem->read_u8((x * 4) + bg_tile_addr);
		temp_hash <<= 8;
		temp_hash += mem->read_u8((x * 4) + bg_tile_addr + 1);
		cgfx_stat.current_gbc_bg_hash[bg_index] += hash::raw_to_64(temp_hash);

		temp_hash = mem->read_u8((x * 4) + bg_tile_addr + 2);
		temp_hash <<= 8;
		temp_hash += mem->read_u8((x * 4) + bg_tile_addr + 3);
		cgfx_stat.current_gbc_bg_hash[bg_index] += hash::raw_to_64(temp_hash);
	}

	//Prepend the hues to each hash
	std::string hue_data = "";
	
	for(int x = 0; x < 4; x++)
	{
		util::hsv color = util::rgb_to_hsv(lcd_stat.bg_colors_final[x][palette]);
		u8 hue = (color.hue / 10);
		hue_data += hash::base_64_index[hue];
	}

	cgfx_stat.current_gbc_bg_hash[bg_index] = hue_data + "_" + cgfx_stat.current_gbc_bg_hash[bg_index];

	final_hash = cgfx_stat.current_gbc_bg_hash[bg_index];

	//Update the BG hash list
	for(int x = 0; x < cgfx_stat.bg_hash_list.size(); x++)
	{
		if(final_hash == cgfx_stat.bg_hash_list[x]) { mem->vram_bank = old_vram_bank; return; }
	}

	cgfx_stat.bg_hash_list.push_back(final_hash);

	//Optionally auto-dump GBC BG
	if(cgfx::auto_dump_bg) { dump_gbc_bg(final_hash, bg_tile_addr, palette); }

	//Reset VRAM bank
	mem->vram_bank = old_vram_bank;
}	

/****** Search for an existing hash from the manifest ******/
bool DMG_LCD::has_hash(u16 addr, std::string hash)
{
	bool match = false;

	for(int x = 0; x < cgfx_stat.m_hashes.size(); x++)
	{
		if(hash == cgfx_stat.m_hashes[x])
		{
			if(cgfx_stat.m_vram_addr[x] == 0)
			{
				cgfx_stat.last_id = x;
				match = true;
			}
			
			//Check VRAM addr requirement, if applicable
			else if(cgfx_stat.m_vram_addr[x] == addr)
			{
				cgfx_stat.last_id = x;
				return true;
			}
		}
	}

	return match;
}

/****** Adjusts pixel brightness according to a given GBC palette ******/
u32 DMG_LCD::adjust_pixel_brightness(u32 color, u8 palette_id, u8 gfx_type)
{
	//Compare average palette brightness with input brightness
	u8 palette_brightness = (gfx_type) ? cgfx_stat.obj_pal_brightness[palette_id] : cgfx_stat.bg_pal_brightness[palette_id];

	util::hsl temp_color = util::rgb_to_hsl(color);
	temp_color.lightness = palette_brightness / 255.0;

	u32 final_color = util::hsl_to_rgb(temp_color);
	return final_color;
}
