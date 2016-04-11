// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : cgfx_common.h
// Date : August 12, 2015
// Description : GBE+ Global Custom Graphics
//
// Emulator-wide custom graphics assets

#include "cgfx_common.h"

namespace cgfx
{ 
	u8 gbc_bg_color_pal = 0;
	u8 gbc_bg_vram_bank = 0;
	u8 gbc_obj_color_pal = 0;
	u8 gbc_obj_vram_bank = 0;

	bool load_cgfx = false;
	bool auto_dump_obj = false;
	bool auto_dump_bg = false;
	bool ignore_blank_dumps = false;

	u8 scaling_factor = 1;
	u8 scale_squared = 1;
	u32 transparency_color = 0xFF00FF00;

	std::string manifest_file = "";
	std::string dump_bg_path = "Dump/BG/";
	std::string dump_obj_path = "Dump/OBJ/";
	std::string dump_name = "";

	std::string last_hash = "";
	u32 last_vram_addr = 0;
	u8 last_type = 0;
	u8 last_palette = 0;
}
