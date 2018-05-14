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
	cgfx_stat.manifest_entry_size.clear();
	cgfx_stat.m_hashes.clear();
	cgfx_stat.m_hashes_raw.clear();
	cgfx_stat.m_files.clear();
	cgfx_stat.m_types.clear();
	cgfx_stat.m_vram_addr.clear();
	cgfx_stat.m_auto_bright.clear();

	cgfx_stat.m_meta_files.clear();
	cgfx_stat.m_meta_names.clear();
	cgfx_stat.m_meta_forms.clear();

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
		u8 item_count = 0;	

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
					item_count++;
				}

				else { line_item += line_char; }
			}
		}

		//Determine if manifest is properly formed (roughly)
		//Each manifest normal entry should have 5 parameters
		//Each metatile entry should have 3 parameters
		if((item_count != 5) && (item_count != 3) && (item_count != 0))
		{
			std::cout<<"CGFX::Manifest file " << filename << " has some missing parameters for some entries. \n";
			std::cout<<"CGFX::Entry -> " << input_line << "\n";
			file.close();
			return false;
		}

		else if(item_count != 0) { cgfx_stat.manifest_entry_size.push_back(item_count); }
	}
	
	file.close();

	u32 hash_id = 0;

	//Parse entries
	for(int x = 0, y = 0; y < cgfx_stat.manifest_entry_size.size(); y++)
	{
		//Parse regular entries
		if(cgfx_stat.manifest_entry_size[y] == 5)
		{
			//Grab hash
			std::string hash = cgfx_stat.manifest[x++];
			std::string sub_hash = "";

			//Only add hash from manifest if it is new. Otherwise, ignore duplicate entries
			if(cgfx_stat.m_hashes.find(hash) == cgfx_stat.m_hashes.end())
			{
				//Grab file associated with hash
				cgfx_stat.m_files.push_back(cgfx_stat.manifest[x++]);

				//Grab the type
				u32 type_byte = 0;
				util::from_str(cgfx_stat.manifest[x++], type_byte);
				cgfx_stat.m_types.push_back(type_byte);

				switch(type_byte)
				{
					//DMG, GBC, or GBA OBJ
					case 1:
						sub_hash = hash.substr(4);
						cgfx_stat.m_id.push_back(cgfx_stat.obj_hash_list.size());
						cgfx_stat.obj_hash_list.push_back(hash);
						cgfx_stat.m_hashes.insert(std::make_pair(hash, hash_id));
						cgfx_stat.m_hashes_raw.insert(std::make_pair(sub_hash, hash_id));
						break;

					case 2:
						sub_hash = hash.substr(5);
						cgfx_stat.m_id.push_back(cgfx_stat.obj_hash_list.size());
						cgfx_stat.obj_hash_list.push_back(hash);
						cgfx_stat.m_hashes.insert(std::make_pair(hash, hash_id));
						cgfx_stat.m_hashes_raw.insert(std::make_pair(sub_hash, hash_id));
						break;

					case 3:
						break;

					//DMG, GBC, or GBA BG
					case 10:
						sub_hash = hash.substr(4);
						cgfx_stat.m_id.push_back(cgfx_stat.bg_hash_list.size());
						cgfx_stat.bg_hash_list.push_back(hash);
						cgfx_stat.m_hashes.insert(std::make_pair(hash, hash_id));
						cgfx_stat.m_hashes_raw.insert(std::make_pair(sub_hash, hash_id));
						break;
						
					case 20:
						sub_hash = hash.substr(5);
						cgfx_stat.m_id.push_back(cgfx_stat.bg_hash_list.size());
						cgfx_stat.bg_hash_list.push_back(hash);
						cgfx_stat.m_hashes.insert(std::make_pair(hash, hash_id));
						cgfx_stat.m_hashes_raw.insert(std::make_pair(sub_hash, hash_id));
						break;

					case 30:
						break;
		
					//Undefined type
					default:
						std::cout<<"CGFX::Undefined hash type " << (int)type_byte << "\n";
						return false;
				}

				//Load image based on filename and hash type
				if(!load_image_data()) { return false; }

				//EXT_VRAM_ADDR
				u32 vram_address = 0;
				util::from_hex_str(cgfx_stat.manifest[x++], vram_address);
				cgfx_stat.m_vram_addr.push_back(vram_address);	

				//EXT_AUTO_BRIGHT
				u32 bright_value = 0;
				util::from_str(cgfx_stat.manifest[x++], bright_value);
				cgfx_stat.m_auto_bright.push_back(bright_value);

				//Enable EXT_AUTO_BRIGHT for all matching raw hashes if necessary
				if((bright_value) && (cgfx_stat.m_hashes_raw.find(sub_hash) != cgfx_stat.m_hashes_raw.end()))
				{
					u32 bright_id = cgfx_stat.m_hashes_raw[sub_hash];
					cgfx_stat.m_auto_bright[bright_id] = 1;
				} 

				hash_id++;
			}

			else
			{
				std::cout<<"CGFX::Warning - Duplicate hash detected. Only the 1st entry that uses this hash will properly render \n";
				x += 4;
			}
		}

		//Parse metatile entries
		else
		{
			//Grab metafile
			cgfx_stat.m_meta_files.push_back(cgfx_stat.manifest[x++]);

			//Load image based on filename and hash type
			if(!load_meta_data()) { return false; }

			//Grab base pattern name
			cgfx_stat.m_meta_names.push_back(cgfx_stat.manifest[x++]);

			//Grab metatile form
			u32 form_value = 0;
			util::from_str(cgfx_stat.manifest[x++], form_value);
			cgfx_stat.m_meta_forms.push_back(form_value);
		}
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
		//Attempt to find the meta tile data instead
		if(find_meta_data()) { return true; }

		std::cout<<"GBE::CGFX - Could not load " << filename << "\n";
		return false;
	}

	int custom_bpp = source->format->BitsPerPixel;

	if(custom_bpp != 24)
	{
		std::cout<<"GBE::CGFX - " << filename << " has " << custom_bpp << "bpp instead of 24bpp \n";
		return false;
	}

	//Verify source width
	if((source->w % 8) != 0)
	{
		std::cout<<"GBE::CGFX - " << filename << " has irregular width -> " << source->w << "\n";
		return false;
	}

	//Verify source height
	if((source->h % 8) != 0)
	{
		std::cout<<"GBE::CGFX - " << filename << " has irregular height -> " << source->h << "\n";
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

	return true;
}

/****** Loads 24-bit data from source and converts it to 32-bit ARGB - For metatiles ******/
bool DMG_LCD::load_meta_data() 
{
	//TODO - PNG loading via SDL_image

	//NOTE - Only call this function during manifest loading, immediately after the filename is parsed
	std::string filename = config::data_path + cgfx_stat.m_meta_files.back();
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

	//Verify source width
	if((source->w % 8) != 0)
	{
		std::cout<<"GBE::CGFX - " << filename << " has irregular width -> " << source->w << "\n";
		return false;
	}

	//Verify source height
	if((source->h % 8) != 0)
	{
		std::cout<<"GBE::CGFX - " << filename << " has irregular height -> " << source->h << "\n";
		return false;
	}
		
	std::vector<u32> cgfx_pixels;

	//Load 24-bit data
	u8* pixel_data = (u8*)source->pixels;

	for(int a = 0, b = 0; a < (source->w * source->h); a++, b+=3)
	{
		cgfx_pixels.push_back(0xFF000000 | (pixel_data[b+2] << 16) | (pixel_data[b+1] << 8) | (pixel_data[b]));
	}

	//Store meta pixel data
	cgfx_stat.meta_pixel_data.push_back(cgfx_pixels);
	
	//Save width and height for reference
	cgfx_stat.m_meta_width.push_back(source->w);
	cgfx_stat.m_meta_height.push_back(source->h);

	return true;
}

/****** Finds meta data from the meta tile for regular manifest entries ******/
bool DMG_LCD::find_meta_data()
{
	//Find the base name for the entry
	std::string base_name = "";
	std::string base_number = "";
	std::string original_name = cgfx_stat.m_files.back();
	bool is_meta_tile = false;
	u32 meta_tile_number = 0;

	//Parse the entry in reverse.
	//1st underscore indicates metatile number, any others are part of the base name
	for(int x = original_name.size() - 1; x >= 0; x--)
	{
		std::string temp = "";
		temp += original_name[x];

		if(temp == "_")
		{
			is_meta_tile = true;
		}

		if(is_meta_tile) { base_name = temp + base_name; }
		else { base_number = temp + base_number; }
	}

	//Chop off last underscore separating base name and meta tile number
	base_name = base_name.substr(0, base_name.size() - 1);

	//Return false if original name does not contain a base name for a metatile
	if(!is_meta_tile) { return false; }

	//Parse the meta tile number from the original name
	util::from_str(base_number, meta_tile_number);

	//Search metatile name entries for a match and grab id
	u32 meta_id = 0;
	bool found_match = false;

	for(int x = 0; x < cgfx_stat.m_meta_names.size(); x++)
	{
		if(cgfx_stat.m_meta_names[x] == base_name)
		{
			found_match = true;
			meta_id = x;
			break;
		}
	}

	//Return false if no meta tile base name exists
	if(!found_match) { return false; }

	//Abort if invalid metatile form
	if(cgfx_stat.m_meta_forms[meta_id] > 3)
	{
		std::cout<<"GBE::CGFX - Invalid metatile form : " << (u32)cgfx_stat.m_meta_forms[meta_id] << "\n";
		return false;
	}

	//Grab meta pixel data
	std::vector<u32> cgfx_pixels;

	//Find start position inside meta pixel data
	u32 width = cgfx_stat.m_meta_width[meta_id];
	u32 height = cgfx_stat.m_meta_height[meta_id];
	
	u32 tile_w = width / (8 * cgfx::scaling_factor);
	u32 tile_h = (cgfx_stat.m_meta_forms[meta_id] != 2) ? height / (8 * cgfx::scaling_factor) : height / (16 * cgfx::scaling_factor);

	u32 pixel_w = width / tile_w;
	u32 pixel_h = height / tile_h;

	u32 pos = (width * pixel_h) * (meta_tile_number / tile_w);
	pos += (meta_tile_number % tile_w) * pixel_w;

	for(int y = 0; y < pixel_h; y++)
	{
		for(int x = 0; x < pixel_w; x++)
		{
			u32 meta_pixel = cgfx_stat.meta_pixel_data[meta_id][pos++];
			cgfx_pixels.push_back(meta_pixel);
		}

		pos -= pixel_w;
		pos += width;
	}

	//Try to push the meta pixel data to the BG pixel data set
	if(cgfx_stat.m_meta_forms[meta_id] == 0) { cgfx_stat.bg_pixel_data.push_back(cgfx_pixels); }

	//Try to push the meta pixel data to the OBJ pixel data set
	else { cgfx_stat.obj_pixel_data.push_back(cgfx_pixels); }

	return true;
} 
	
/****** Dumps DMG OBJ tile from selected memory address ******/
void DMG_LCD::dump_dmg_obj(u8 obj_index) 
{
	SDL_Surface* obj_dump = NULL;
	u8 obj_height = 0;

	cgfx_stat.current_obj_hash[obj_index] = "";
	std::string final_hash = "";

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
		cgfx_stat.current_obj_hash[obj_index] += hash::raw_to_64(temp_hash);

		temp_hash = mem->read_u8((x * 4) + obj_tile_addr + 2);
		temp_hash <<= 8;
		temp_hash += mem->read_u8((x * 4) + obj_tile_addr + 3);
		cgfx_stat.current_obj_hash[obj_index] += hash::raw_to_64(temp_hash);
	}

	//Prepend palette data
	u8 pal_data = (obj[obj_index].palette_number == 1) ? mem->memory_map[REG_OBP1] : mem->memory_map[REG_OBP0];
	cgfx_stat.current_obj_hash[obj_index] = hash::raw_to_64(pal_data) + "_" + cgfx_stat.current_obj_hash[obj_index];

	final_hash = cgfx_stat.current_obj_hash[obj_index];

	//Update the OBJ hash list
	if(!cgfx::ignore_existing_hash)
	{
		for(int x = 0; x < cgfx_stat.obj_hash_list.size(); x++)
		{
			if(final_hash == cgfx_stat.obj_hash_list[x])
			{
				cgfx::last_added = false;
				return;
			}
		}
	}

	//For new OBJs, dump BMP file
	cgfx_stat.obj_hash_list.push_back(final_hash);

	obj_dump = SDL_CreateRGBSurface(SDL_SWSURFACE, 8, obj_height, 32, 0, 0, 0, 0);
		
	std::string dump_file =  "";
	if(cgfx::dump_name.empty()) { dump_file = config::data_path + cgfx::dump_obj_path + final_hash + ".bmp"; }
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

		if(blank)
		{
			cgfx::last_added = false;
			return;
		}
	}

	//Save to BMP
	if(SDL_SaveBMP(obj_dump, dump_file.c_str()) == 0)
	{
		cgfx::last_saved = true;
		std::cout<<"LCD::Saving Sprite - " << dump_file << "\n";
	}

	else
	{
		cgfx::last_saved = false;
		std::cout<<"LCD::Error - Could not save sprite to " << dump_file << ". Please check file path and permissions \n";
	}

	//Save CGFX data
	cgfx::last_hash = final_hash;
	cgfx::last_vram_addr = 0x8000 + (obj[obj_index].tile_number << 4);
	cgfx::last_type = 1;
	cgfx::last_palette = 0;
	cgfx::last_added = true;
}

/****** Dumps GBC OBJ tile from selected memory address ******/
void DMG_LCD::dump_gbc_obj(u8 obj_index) 
{
	SDL_Surface* obj_dump = NULL;
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
		util::hsv color = util::rgb_to_hsv(lcd_stat.obj_colors_final[x][obj[obj_index].color_palette_number]);
		u8 hue = (color.hue / 10);
		hue_data += hash::base_64_index[hue];
	}

	cgfx_stat.current_obj_hash[obj_index] = hue_data + "_" + cgfx_stat.current_obj_hash[obj_index];

	final_hash = cgfx_stat.current_obj_hash[obj_index];

	//Update the OBJ hash list
	if(!cgfx::ignore_existing_hash)
	{
		for(int x = 0; x < cgfx_stat.obj_hash_list.size(); x++)
		{
			if(final_hash == cgfx_stat.obj_hash_list[x])
			{
				mem->vram_bank = old_vram_bank;
				cgfx::last_added = false;
				return;
			}
		}
	}

	//For new OBJs, dump BMP file
	cgfx_stat.obj_hash_list.push_back(final_hash);

	obj_dump = SDL_CreateRGBSurface(SDL_SWSURFACE, 8, obj_height, 32, 0, 0, 0, 0);

	std::string dump_file =  "";
	if(cgfx::dump_name.empty()) { dump_file = config::data_path + cgfx::dump_obj_path + final_hash + ".bmp"; }
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

		if(blank)
		{
			cgfx::last_added = false;
			return;
		}
	}

	//Save to BMP
	if(SDL_SaveBMP(obj_dump, dump_file.c_str()) == 0)
	{
		cgfx::last_saved = true;
		std::cout<<"LCD::Saving Sprite - " << dump_file << "\n";
	}

	else
	{
		cgfx::last_saved = false;
		std::cout<<"LCD::Error - Could not save sprite to " << dump_file << ". Please check file path and permissions \n";
	}

	//Reset VRAM bank
	mem->vram_bank = old_vram_bank;

	//Save CGFX data
	cgfx::last_hash = final_hash;
	cgfx::last_vram_addr = 0x8000 + (obj[obj_index].tile_number << 4);
	cgfx::last_type = 2;
	cgfx::last_palette = obj[obj_index].color_palette_number + 1;
	cgfx::last_added = true;
}

/****** Dumps DMG BG tile from selected memory address ******/
void DMG_LCD::dump_dmg_bg(u16 bg_index) 
{
	SDL_Surface* bg_dump = NULL;

	cgfx_stat.current_bg_hash[bg_index] = "";
	std::string final_hash = "";

	//Grab OBJ tile addr from index
	u16 bg_tile_addr = (bg_index * 16) + 0x8000;

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

	//Prepend palette data
	u8 pal_data = mem->memory_map[REG_BGP];
	cgfx_stat.current_bg_hash[bg_index] = hash::raw_to_64(pal_data) + "_" + cgfx_stat.current_bg_hash[bg_index];

	final_hash = cgfx_stat.current_bg_hash[bg_index];

	//Update the BG hash list
	if(!cgfx::ignore_existing_hash)
	{
		for(int x = 0; x < cgfx_stat.bg_hash_list.size(); x++)
		{
			if(final_hash == cgfx_stat.bg_hash_list[x])
			{
				cgfx::last_added = false;
				return;
			}
		}
	}

	//For new BG graphics, dump BMP file
	cgfx_stat.bg_hash_list.push_back(final_hash);

	bg_dump = SDL_CreateRGBSurface(SDL_SWSURFACE, 8, 8, 32, 0, 0, 0, 0);

	std::string dump_file =  "";
	if(cgfx::dump_name.empty()) { dump_file = config::data_path + cgfx::dump_bg_path + final_hash + ".bmp"; }
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

		if(blank)
		{
			cgfx::last_added = false;
			return;
		}
	}

	//Save to BMP
	if(SDL_SaveBMP(bg_dump, dump_file.c_str()) == 0)
	{
		cgfx::last_saved = true;
		std::cout<<"LCD::Background Tile - " << dump_file << "\n";
	}

	else
	{
		cgfx::last_saved = false;
		std::cout<<"LCD::Error - Could not save background tile to " << dump_file << ". Please check file path and permissions \n";
	}

	//Save CGFX data
	cgfx::last_hash = final_hash;
	cgfx::last_vram_addr = (bg_index << 4) + 0x8000;
	cgfx::last_type = 10;
	cgfx::last_palette = 0;
	cgfx::last_added = true;
}

/****** Dumps GBC BG tile from selected memory address (GUI version) ******/
void DMG_LCD::dump_gbc_bg(u16 bg_index) 
{
	SDL_Surface* bg_dump = NULL;

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
	if(!cgfx::ignore_existing_hash)
	{
		for(int x = 0; x < cgfx_stat.bg_hash_list.size(); x++)
		{
			if(final_hash == cgfx_stat.bg_hash_list[x])
			{
				mem->vram_bank = old_vram_bank;
				cgfx::last_added = false;
				return;
			}
		}
	}

	//For new BGs, dump BMP file
	cgfx_stat.bg_hash_list.push_back(final_hash);

	bg_dump = SDL_CreateRGBSurface(SDL_SWSURFACE, 8, 8, 32, 0, 0, 0, 0);

	std::string dump_file =  "";
	if(cgfx::dump_name.empty()) { dump_file = config::data_path + cgfx::dump_bg_path + final_hash + ".bmp"; }
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

		if(blank)
		{
			cgfx::last_added = false;
			return;
		}
	}

	//Save to BMP
	if(SDL_SaveBMP(bg_dump, dump_file.c_str()) == 0)
	{
		cgfx::last_saved = true;
		std::cout<<"LCD::Background Tile - " << dump_file << "\n";
	}

	else
	{
		cgfx::last_saved = false;
		std::cout<<"LCD::Error - Could not save background tile to " << dump_file << ". Please check file path and permissions \n";
	}

	//Reset VRAM bank
	mem->vram_bank = old_vram_bank;

	//Save CGFX data
	cgfx::last_hash = final_hash;
	cgfx::last_vram_addr = (bg_index << 4) + 0x8000;
	cgfx::last_type = 20;
	cgfx::last_palette = cgfx::gbc_bg_color_pal + 1;
	cgfx::last_added = true;
}

/****** Dumps GBC BG tile from selected memory address (Auto-dump version) ******/
void DMG_LCD::dump_gbc_bg(std::string final_hash, u16 bg_tile_addr, u8 palette) 
{
	SDL_Surface* bg_dump = SDL_CreateRGBSurface(SDL_SWSURFACE, 8, 8, 32, 0, 0, 0, 0);

	std::string dump_file =  "";
	if(cgfx::dump_name.empty()) { dump_file = config::data_path + cgfx::dump_bg_path + final_hash + ".bmp"; }
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

		if(blank)
		{
			cgfx::last_added = false;
			return;
		}
	}

	//Save to BMP
	if(SDL_SaveBMP(bg_dump, dump_file.c_str()) == 0)
	{
		cgfx::last_saved = true;
		std::cout<<"LCD::Background Tile - " << dump_file << "\n";
	}

	else
	{
		cgfx::last_saved = false;
		std::cout<<"LCD::Error - Could not save background tile to " << dump_file << ". Please check file path and permissions \n";
	}

	//Save CGFX data
	cgfx::last_hash = final_hash;
	cgfx::last_vram_addr = bg_tile_addr - 16;
	cgfx::last_type = 20;
	cgfx::last_palette = palette + 1;
	cgfx::last_added = true;
}

/****** Updates the current hash for the selected DMG OBJ ******/
void DMG_LCD::update_dmg_obj_hash(u8 obj_index)
{
	u8 obj_height = 0;

	cgfx_stat.current_obj_hash[obj_index] = "";
	std::string final_hash = "";

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
		cgfx_stat.current_obj_hash[obj_index] += hash::raw_to_64(temp_hash);

		temp_hash = mem->read_u8((x * 4) + obj_tile_addr + 2);
		temp_hash <<= 8;
		temp_hash += mem->read_u8((x * 4) + obj_tile_addr + 3);
		cgfx_stat.current_obj_hash[obj_index] += hash::raw_to_64(temp_hash);
	}

	//Prepend palette data
	u8 pal_data = (obj[obj_index].palette_number == 1) ? mem->memory_map[REG_OBP1] : mem->memory_map[REG_OBP0];
	cgfx_stat.current_obj_hash[obj_index] = hash::raw_to_64(pal_data) + "_" + cgfx_stat.current_obj_hash[obj_index];

	final_hash = cgfx_stat.current_obj_hash[obj_index];

	//Optionally auto-dump DMG OBJ
	if(cgfx::auto_dump_obj) { dump_dmg_obj(obj_index); }

	//Update the OBJ hash list
	for(int x = 0; x < cgfx_stat.obj_hash_list.size(); x++)
	{
		if(final_hash == cgfx_stat.obj_hash_list[x]) { return; }
	}

	cgfx_stat.obj_hash_list.push_back(final_hash);

	//Limit OBJ hash list size, delete oldest entry
	if(cgfx_stat.obj_hash_list.size() > 128) { cgfx_stat.obj_hash_list.erase(cgfx_stat.obj_hash_list.begin()); }
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
		util::hsv color = util::rgb_to_hsv(lcd_stat.obj_colors_final[x][obj[obj_index].color_palette_number]);
		u8 hue = (color.hue / 10);
		hue_data += hash::base_64_index[hue];
	}

	cgfx_stat.current_obj_hash[obj_index] = hue_data + "_" + cgfx_stat.current_obj_hash[obj_index];

	final_hash = cgfx_stat.current_obj_hash[obj_index];

	//Optionally auto-dump GBC OBJ
	if(cgfx::auto_dump_obj) { dump_gbc_obj(obj_index); }

	//Limit OBJ hash list size, delete oldest entry
	if(cgfx_stat.obj_hash_list.size() > 128) { cgfx_stat.obj_hash_list.erase(cgfx_stat.obj_hash_list.begin()); }

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

	//Grab BG tile addr from index
	u16 bg_tile_addr = (bg_index * 16) + 0x8000;

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

	//Prepend palette data
	u8 pal_data = mem->memory_map[REG_BGP];
	cgfx_stat.current_bg_hash[bg_index] = hash::raw_to_64(pal_data) + "_" + cgfx_stat.current_bg_hash[bg_index];

	final_hash = cgfx_stat.current_bg_hash[bg_index];

	//Optionally auto-dump DMG BG
	if(cgfx::auto_dump_bg) { dump_dmg_bg(bg_index); }

	//Update the BG hash list
	for(int x = 0; x < cgfx_stat.bg_hash_list.size(); x++)
	{
		if(final_hash == cgfx_stat.bg_hash_list[x]) { return; }
	}

	cgfx_stat.bg_hash_list.push_back(final_hash);

	//Limit BG hash list size, delete oldest entry
	if(cgfx_stat.bg_hash_list.size() > 512) { cgfx_stat.bg_hash_list.erase(cgfx_stat.bg_hash_list.begin()); }
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

	//Limit BG hash list size, delete oldest entry
	//To be on the safe side, allow 2x as much as the DMG
	if(cgfx_stat.bg_hash_list.size() > 1024) { cgfx_stat.bg_hash_list.erase(cgfx_stat.bg_hash_list.begin()); }

	//Reset VRAM bank
	mem->vram_bank = old_vram_bank;
}	

/****** Search for an existing hash from the manifest ******/
bool DMG_LCD::has_hash(u16 addr, std::string hash)
{
	//Sanity check on hash lengths
	if(hash.length() < 5) { return false; }

	bool match = false;
	std::string sub_hash = (config::gb_type == 0) ? hash.substr(4) : hash.substr(5);
	
	//Check for 100% match
	if(cgfx_stat.m_hashes.find(hash) != cgfx_stat.m_hashes.end())
	{
		u32 id = cgfx_stat.m_hashes[hash];
	
		if(cgfx_stat.m_vram_addr[id] == 0)
		{
			cgfx_stat.last_id = id;
			return true;
		}
			
		//Check VRAM addr requirement, if applicable
		else if(cgfx_stat.m_vram_addr[id] == addr)
		{
			cgfx_stat.last_id = id;
			return true;
		}
	}

	//Check for pure afterwards for EXT_AUTO_BRIGHT
	else if(cgfx_stat.m_hashes_raw.find(sub_hash) != cgfx_stat.m_hashes_raw.end())
	{
		u32 id = cgfx_stat.m_hashes_raw[sub_hash];

		if(cgfx_stat.m_auto_bright[id] == 1)
		{
			cgfx_stat.last_id = id;
			return true;
		}
	}

	return match;
}

/****** Adjusts pixel brightness according to a given GBC palette ******/
u32 DMG_LCD::adjust_pixel_brightness(u32 color, u8 palette_id, u8 gfx_type)
{
	u8 pal_max = (gfx_type) ? cgfx_stat.obj_pal_max[palette_id] : cgfx_stat.bg_pal_max[palette_id];
	u8 pal_min = (gfx_type) ? cgfx_stat.obj_pal_min[palette_id] : cgfx_stat.bg_pal_min[palette_id];
	u8 current_brightness = util::get_brightness_fast(color);

	if(current_brightness > pal_max)
	{
		util::hsl temp_color = util::rgb_to_hsl(color);
		temp_color.lightness = pal_max / 255.0;

		u32 final_color = util::hsl_to_rgb(temp_color);
		return final_color;
	}

	else if(current_brightness < pal_min)
	{
		util::hsl temp_color = util::rgb_to_hsl(color);
		temp_color.lightness = pal_min / 255.0;

		u32 final_color = util::hsl_to_rgb(temp_color);
		return final_color;
	}

	return color;
}

/****** Returns the hash for a specific tile ******/
std::string DMG_LCD::get_hash(u16 addr, u8 gfx_type)
{
	std::string final_hash = "";

	//Get DMG OBJ hash
	if(gfx_type == 1)
	{
		//0-7 index, 8-15 tile number from OAM
		u8 obj_index = addr & 0xFF;
		
		addr >>= 8;
		addr = 0x8000 + (addr << 4);

		//Determine if in 8x8 or 8x16 mode
		u8 obj_height = (mem->memory_map[REG_LCDC] & 0x04) ? 16 : 8;

		//Create a hash for this OBJ tile
		for(int x = 0; x < obj_height/2; x++)
		{
			u16 temp_hash = mem->read_u8((x * 4) + addr);
			temp_hash <<= 8;
			temp_hash += mem->read_u8((x * 4) + addr + 1);
			final_hash += hash::raw_to_64(temp_hash);

			temp_hash = mem->read_u8((x * 4) + addr + 2);
			temp_hash <<= 8;
			temp_hash += mem->read_u8((x * 4) + addr + 3);
			final_hash += hash::raw_to_64(temp_hash);
		}

		//Prepend palette data
		u8 pal_data = (obj[obj_index].palette_number == 1) ? mem->memory_map[REG_OBP1] : mem->memory_map[REG_OBP0];
		final_hash = hash::raw_to_64(pal_data) + "_" + final_hash;
	}

	//Get GBC OBJ hash
	else if(gfx_type == 2)
	{
		//0-7 index, 8-15 tile number from OAM
		u8 obj_index = addr & 0xFF;
		
		addr >>= 8;
		addr = 0x8000 + (addr << 4);

		//Determine if in 8x8 or 8x16 mode
		u8 obj_height = (mem->memory_map[REG_LCDC] & 0x04) ? 16 : 8;

		//Grab VRAM bank
		u8 old_vram_bank = mem->vram_bank;
		mem->vram_bank = obj[obj_index].vram_bank;

		//Get color palette from OAM
		u8 color_pal = obj[obj_index].color_palette_number;

		//Create a hash for this OBJ tile
		for(int x = 0; x < obj_height/2; x++)
		{
			u16 temp_hash = mem->read_u8((x * 4) + addr);
			temp_hash <<= 8;
			temp_hash += mem->read_u8((x * 4) + addr + 1);
			final_hash += hash::raw_to_64(temp_hash);

			temp_hash = mem->read_u8((x * 4) + addr + 2);
			temp_hash <<= 8;
			temp_hash += mem->read_u8((x * 4) + addr + 3);
			final_hash += hash::raw_to_64(temp_hash);
		}

		//Prepend the hues to each hash
		std::string hue_data = "";
	
		for(int x = 0; x < 4; x++)
		{
			util::hsv color = util::rgb_to_hsv(lcd_stat.obj_colors_final[x][color_pal]);
			u8 hue = (color.hue / 10);
			hue_data += hash::base_64_index[hue];
		}

		final_hash = hue_data + "_" + final_hash;
		mem->vram_bank = old_vram_bank;
	}

	//Get DMG BG hash
	else if(gfx_type == 10)
	{
		//Create a hash for this BG tile
		for(int x = 0; x < 4; x++)
		{
			u16 temp_hash = mem->read_u8((x * 4) + addr);
			temp_hash <<= 8;
			temp_hash += mem->read_u8((x * 4) + addr + 1);
			final_hash += hash::raw_to_64(temp_hash);

			temp_hash = mem->read_u8((x * 4) + addr + 2);
			temp_hash <<= 8;
			temp_hash += mem->read_u8((x * 4) + addr + 3);
			final_hash += hash::raw_to_64(temp_hash);
		}

		//Prepend palette data
		u8 pal_data = mem->memory_map[REG_BGP];
		final_hash = hash::raw_to_64(pal_data) + "_" + final_hash;
	}

	//Get GBC BG hash
	else if(gfx_type == 20)
	{
		//Set VRAM bank
		u8 old_vram_bank = mem->vram_bank;
		mem->vram_bank = cgfx::gbc_bg_vram_bank;

		//Create a hash for this BG tile
		for(int x = 0; x < 4; x++)
		{
			u16 temp_hash = mem->read_u8((x * 4) + addr);
			temp_hash <<= 8;
			temp_hash += mem->read_u8((x * 4) + addr + 1);
			final_hash += hash::raw_to_64(temp_hash);

			temp_hash = mem->read_u8((x * 4) + addr + 2);
			temp_hash <<= 8;
			temp_hash += mem->read_u8((x * 4) + addr + 3);
			final_hash += hash::raw_to_64(temp_hash);
		}

		//Prepend the hues to each hash
		std::string hue_data = "";
	
		for(int x = 0; x < 4; x++)
		{
			util::hsv color = util::rgb_to_hsv(lcd_stat.bg_colors_final[x][cgfx::gbc_bg_color_pal]);
			u8 hue = (color.hue / 10);
			hue_data += hash::base_64_index[hue];
		}

		final_hash = hue_data + "_" + final_hash;
		mem->vram_bank = old_vram_bank;
	}


	return final_hash;
}

/****** Invalidates all current hashes and forces updates for all of VRAM ******/
void DMG_LCD::invalidate_cgfx()
{
	cgfx_stat.current_obj_hash.clear();
	cgfx_stat.current_obj_hash.resize(40, "");

	cgfx_stat.current_bg_hash.clear();
	cgfx_stat.current_bg_hash.resize(384, "");

	cgfx_stat.current_gbc_bg_hash.clear();
	cgfx_stat.current_gbc_bg_hash.resize(2048, "");

	cgfx_stat.bg_update_list.clear();
	cgfx_stat.bg_update_list.resize(384, true);

	cgfx_stat.bg_tile_update_list.clear();
	cgfx_stat.bg_tile_update_list.resize(256, true);

	cgfx_stat.bg_map_update_list.clear();
	cgfx_stat.bg_map_update_list.resize(2048, true);

	cgfx_stat.update_bg = true;
	cgfx_stat.update_map = true;
}
