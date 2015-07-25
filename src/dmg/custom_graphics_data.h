// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd_data.h
// Date : July 23, 2015
// Description : Custom graphics data
//
// Defines the data structure that hold info about custom DMG-GBC graphics
// Only the functions in the dmg_cgfx namespace should use this

#ifndef GB_CGFX_DATA
#define GB_CGFX_DATA

#include <vector>
#include <string>

#include "common/common.h"

struct dmg_cgfx_data
{ 
	std::vector <std::string> manifest;

	std::vector <std::string> current_obj_hash;
	std::vector <std::string> current_bg_hash;

	std::vector <std::string> obj_hash_list;
	std::vector <std::string> bg_hash_list;
};

#endif // GB_CGFX_DATA
