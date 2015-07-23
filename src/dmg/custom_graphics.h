// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : custom_gfx.cpp
// Date : July 23, 2015
// Description : Game Boy Enhanced custom graphics (DMG/GBC version)
//
// Handles dumping original BG and Sprite tiles
// Handles loading custom pixel data

#ifndef GB_CGFX
#define GB_CGFX

#include <SDL/SDL.h>
#include <string>

#include "common/common.h"

namespace dmg_cgfx
{
	bool load_manifest(std::string filename);
	void load_image_data(int size, SDL_Surface* custom_source, u32 custom_dest[]);

	void dump_dmg_obj(u16 addr);
	void dump_dmg_bg(u16 addr);

	void dump_gbc_obj(u16 addr);
	void dump_gbc_bg(u16 addr);

	void load_dmg_obj(u16 obj_index);
	void load_dmg_bg(u16 bg_index);

	void load_gbc_obj(u16 obj_index);
	void load_gbc_bg(u16 bg_index);
}

#endif // GB_CGFX
