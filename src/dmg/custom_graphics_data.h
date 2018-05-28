// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : custom_graphics_data.h
// Date : July 23, 2015
// Description : Custom graphics data
//
// Defines the data structure that hold info about custom DMG-GBC graphics
// Only the functions in the dmg_cgfx namespace should use this

#ifndef GB_CGFX_DATA
#define GB_CGFX_DATA

#include <vector>
#include <string>
#include <map>

#include "common/common.h"

struct dmg_cgfx_data
{ 
	std::vector <std::string> manifest;
	std::vector <u8> manifest_entry_size;
	
	//Data pulled from manifest file - Regular entries
	//Hashes - Actual hash data. Duplicated abd sorted into separate OBJ + BG lists
	//Hashes Raw - So called raw hash data (no prepended palette data). May match multiple tiles.
	//Files - Location of the image file used for this hash
	//Types - Determines what system a hash belongs to (DMG, GBC, GBA) and if it's an OBJ or BG
	//ID - Keeps track of which manifest entry corresponds to which OBJ or BG entry
	//VRAM Address - If not zero, this value is computed with the hash
	//Auto Bright - Boolean to auto adjust CGFX assets to the original GBC or GBA palette brightness
	std::map <std::string, u32> m_hashes;
	std::map <std::string, u32> m_hashes_raw;
	std::vector <std::string> m_files;
	std::vector <u8> m_types;
	std::vector <u16> m_id;
	std::vector < std::map <std::string, u32> > m_vram_addr;
	std::vector <u16> m_auto_bright;

	//Data pulled from manifest file - Metatile entries
	//Files - Location of the metatile image file
	//Names - Base name for parsing entries
	//Forms - 1 byte number to identify base type and size
	//ID - Keeps track of where to look for pixel data when generating corresponding normal entries
	std::vector <std::string> m_meta_files;
	std::vector <std::string> m_meta_names;
	std::vector <u8> m_meta_forms;
	std::vector <u32> m_meta_width;
	std::vector <u32> m_meta_height;

	u32 last_id;

	//Working hash list of graphics in VRAM
	std::vector <std::string> current_obj_hash;
	std::vector <std::string> current_bg_hash;
	std::vector <std::string> current_gbc_bg_hash;

	//List of all computed hashes
	std::vector <std::string> obj_hash_list;
	std::vector <std::string> bg_hash_list;

	//Pixel data for all computed hashes (when loading CGFX)
	std::vector< std::vector<u32> > obj_pixel_data;
	std::vector< std::vector<u32> > bg_pixel_data;
	std::vector< std::vector<u32> > meta_pixel_data;

	//List of all tiles that have been updated
	//NOTE - OBJs don't need a list, since the LCD keeps track of OAM updates
	//The LCD does not keep track of BG updates, however.
	std::vector <bool> bg_update_list;
	bool update_bg;
	bool update_map;

	std::vector <bool> bg_tile_update_list;
	std::vector <bool> bg_map_update_list;

	//Palette brightness max and min
	u8 bg_pal_max[8];
	u8 bg_pal_min[8];
	u8 obj_pal_max[8];
	u8 obj_pal_min[8];
};

#endif // GB_CGFX_DATA
