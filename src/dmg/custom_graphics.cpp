// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : custom_gfx.cpp
// Date : July 23, 2015
// Description : Game Boy Enhanced custom graphics (DMG/GBC version)
//
// Handles dumping original BG and Sprite tiles
// Handles loading custom pixel data

#include "custom_graphics.h"
#include "custom_graphics_data.h"

namespace dmg_cgfx
{
	dmg_cgfx_data cgfx_stat;

	/****** Loads the manifest file ******/
	bool load_manifest(std::string filename) { }

	/****** Loads 24-bit data from source and converts it to 32-bit ARGB ******/
	void load_image_data(int size, SDL_Surface* custom_source, u32 custom_dest[]) { }

	/****** Dumps DMG OBJ tile from selected memory address ******/
	void dump_dmg_obj(u16 addr) { }

	/****** Dumps DMG BG tile from selected memory address ******/
	void dump_dmg_bg(u16 addr) { }

	/****** Dumps GBC OBJ tile from selected memory address ******/
	void dump_gbc_obj(u16 addr) { }

	/****** Dumps GBC BG tile from selected memory address ******/
	void dump_gbc_bg(u16 addr) { }
}
