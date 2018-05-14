// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : cgfx.h
// Date : August 12, 2015
// Description : GBE+ Global Custom Graphics
//
// Emulator-wide custom graphics assets

#ifndef GBE_CGFX
#define GBE_CGFX

#include <string>

#include "common.h"

namespace cgfx
{ 
	extern u8 gbc_bg_color_pal;
	extern u8 gbc_obj_color_pal;
	extern u8 gbc_bg_vram_bank;
	extern u8 gbc_obj_vram_bank;

	extern bool load_cgfx;
	extern bool loaded;
	extern bool auto_dump_obj;
	extern bool auto_dump_bg;
	extern bool ignore_blank_dumps;

	extern u32 transparency_color;
	extern u8 scaling_factor;
	extern u8 scale_squared;

	extern std::string manifest_file;
	extern std::string dump_bg_path;
	extern std::string dump_obj_path;
	extern std::string dump_name;
	extern std::string meta_dump_name;

	extern std::string last_hash;
	extern u32 last_vram_addr;
	extern u8 last_type;
	extern u8 last_palette;
	extern bool last_added;
	extern bool last_saved;
	extern bool ignore_existing_hash;
}

#endif // GBE_CGFX 
