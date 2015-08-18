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

	bool load_cgfx = false;
	bool auto_dump_obj = false;
	bool auto_dump_bg = false;
}
