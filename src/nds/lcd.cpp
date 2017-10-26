// GB Enhanced Copyright Daniel Baxter 2013
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd.cpp
// Date : August 16, 2014
// Description : NDS LCD emulation
//
// Draws background, window, and sprites to screen
// Responsible for blitting pixel data and limiting frame rate

#include <cmath>

#include "lcd.h"
#include "common/util.h"

/****** LCD Constructor ******/
NTR_LCD::NTR_LCD()
{
	window = NULL;
	reset();
}

/****** LCD Destructor ******/
NTR_LCD::~NTR_LCD()
{
	screen_buffer.clear();

	scanline_buffer_a.clear();
	scanline_buffer_b.clear();

	render_buffer_a.clear();
	render_buffer_b.clear();

	SDL_DestroyWindow(window);

	std::cout<<"LCD::Shutdown\n";
}

/****** Reset LCD ******/
void NTR_LCD::reset()
{
	final_screen = NULL;
	original_screen = NULL;
	mem = NULL;

	if((window != NULL) && (config::sdl_render)) { SDL_DestroyWindow(window); }
	window = NULL;

	screen_buffer.clear();

	scanline_buffer_a.clear();
	scanline_buffer_b.clear();

	render_buffer_a.clear();
	render_buffer_b.clear();

	lcd_stat.lcd_clock = 0;
	lcd_stat.lcd_mode = 0;

	frame_start_time = 0;
	frame_current_time = 0;
	fps_count = 0;
	fps_time = 0;

	lcd_stat.current_scanline = 0;
	scanline_pixel_counter = 0;

	lcd_stat.lyc_a = 0;
	lcd_stat.lyc_b = 0;

	//Screen + render buffer initialization
	screen_buffer.resize(0x18000, 0);

	scanline_buffer_a.resize(0x100, 0);
	scanline_buffer_b.resize(0x100, 0);

	render_buffer_a.resize(0x100, 0);
	render_buffer_b.resize(0x100, 0);

	full_scanline_render_a = false;
	full_scanline_render_b = false;

	//BG palette initialization
	lcd_stat.bg_pal_update_a = true;
	lcd_stat.bg_pal_update_list_a.resize(0x100, true);

	lcd_stat.bg_pal_update_b = true;
	lcd_stat.bg_pal_update_list_b.resize(0x100, true);

	lcd_stat.bg_ext_pal_update_a = true;
	lcd_stat.bg_ext_pal_update_list_a.resize(0x4000, true);

	lcd_stat.bg_ext_pal_update_b = true;
	lcd_stat.bg_ext_pal_update_list_b.resize(0x4000, true);

	//OBJ palette initialization
	lcd_stat.obj_pal_update_a = true;
	lcd_stat.obj_pal_update_list_a.resize(0x100, true);

	lcd_stat.obj_pal_update_b = true;
	lcd_stat.obj_pal_update_list_b.resize(0x100, true);

	lcd_stat.obj_ext_pal_update_a = true;
	lcd_stat.obj_ext_pal_update_list_a.resize(0x1000, true);

	lcd_stat.obj_ext_pal_update_b = true;
	lcd_stat.obj_ext_pal_update_list_b.resize(0x1000, true);

	//OAM initialization
	lcd_stat.oam_update = true;
	lcd_stat.oam_update_list.resize(0x100, true);

	lcd_stat.update_bg_control_a = false;
	lcd_stat.update_bg_control_b = false;

	lcd_stat.display_mode_a = 0;
	lcd_stat.display_mode_b = 0;

	lcd_stat.obj_boundary_a = 32;
	lcd_stat.obj_boundary_b = 32;

	lcd_stat.ext_pal_a = 0;
	lcd_stat.ext_pal_b = 0;

	lcd_stat.display_stat_a = 0;
	lcd_stat.display_stat_b = 0;

	lcd_stat.bg_mode_a = 0;
	lcd_stat.bg_mode_b = 0;
	lcd_stat.hblank_interval_free = false;

	lcd_stat.forced_blank_a = false;
	lcd_stat.forced_blank_b = false;

	lcd_stat.vblank_irq_enable_a = false;
	lcd_stat.hblank_irq_enable_a = false;
	lcd_stat.vcount_irq_enable_a = false;

	lcd_stat.vblank_irq_enable_b = false;
	lcd_stat.hblank_irq_enable_b = false;
	lcd_stat.vcount_irq_enable_b = false;

	//Misc BG initialization
	for(int x = 0; x < 4; x++)
	{
		lcd_stat.bg_control_a[x] = 0;
		lcd_stat.bg_control_b[x] = 0;

		lcd_stat.bg_priority_a[x] = 0;
		lcd_stat.bg_priority_b[x] = 0;

		lcd_stat.bg_offset_x_a[x] = 0;
		lcd_stat.bg_offset_x_b[x] = 0;

		lcd_stat.bg_offset_y_a[x] = 0;
		lcd_stat.bg_offset_y_b[x] = 0;

		lcd_stat.bg_depth_a[x] = 4;
		lcd_stat.bg_depth_b[x] = 4;

		lcd_stat.bg_size_a[x] = 0;
		lcd_stat.bg_size_b[x] = 0;

		lcd_stat.text_width_a[x] = 0;
		lcd_stat.text_width_b[x] = 0;
		lcd_stat.text_height_a[x] = 0;
		lcd_stat.text_height_b[x] = 0;

		lcd_stat.bg_base_tile_addr_a[x] = 0x6000000;
		lcd_stat.bg_base_tile_addr_b[x] = 0x6000000;

		lcd_stat.bg_base_map_addr_a[x] = 0x6000000;
		lcd_stat.bg_base_map_addr_b[x] = 0x6000000;

		lcd_stat.bg_enable_a[x] = false;
		lcd_stat.bg_enable_b[x] = false;
	}

	//BG2/3 affine parameters + bitmap base addrs
	for(int x = 0; x < 2; x++)
	{
		lcd_stat.bg_affine_a[x].overflow = false;
		lcd_stat.bg_affine_a[x].dmx = lcd_stat.bg_affine_a[x].dy = 0.0;
		lcd_stat.bg_affine_a[x].dx = lcd_stat.bg_affine_a[x].dmy = 1.0;
		lcd_stat.bg_affine_a[x].x_ref = lcd_stat.bg_affine_a[x].y_ref = 0.0;
		lcd_stat.bg_affine_a[x].x_pos = lcd_stat.bg_affine_a[x].y_pos = 0.0;

		lcd_stat.bg_affine_b[x].overflow = false;
		lcd_stat.bg_affine_b[x].dmx = lcd_stat.bg_affine_b[x].dy = 0.0;
		lcd_stat.bg_affine_b[x].dx = lcd_stat.bg_affine_b[x].dmy = 1.0;
		lcd_stat.bg_affine_a[x].x_ref = lcd_stat.bg_affine_b[x].y_ref = 0.0;
		lcd_stat.bg_affine_b[x].x_pos = lcd_stat.bg_affine_b[x].y_pos = 0.0;

		lcd_stat.bg_bitmap_base_addr_a[x] = 0x6000000;
		lcd_stat.bg_bitmap_base_addr_b[x] = 0x6000000;
	}

	//VRAM blocks
	lcd_stat.vram_bank_addr[0] = 0x6800000;
	lcd_stat.vram_bank_addr[1] = 0x6820000;
	lcd_stat.vram_bank_addr[2] = 0x6840000;
	lcd_stat.vram_bank_addr[3] = 0x6860000;
	lcd_stat.vram_bank_addr[4] = 0x6880000;
	lcd_stat.vram_bank_addr[5] = 0x6890000;
	lcd_stat.vram_bank_addr[6] = 0x6894000;
	lcd_stat.vram_bank_addr[7] = 0x6898000;
	lcd_stat.vram_bank_addr[8] = 0x68A0000;

	lcd_stat.vram_bank_enable[0] = false;
	lcd_stat.vram_bank_enable[1] = false;
	lcd_stat.vram_bank_enable[2] = false;
	lcd_stat.vram_bank_enable[3] = false;
	lcd_stat.vram_bank_enable[4] = false;
	lcd_stat.vram_bank_enable[5] = false;
	lcd_stat.vram_bank_enable[6] = false;
	lcd_stat.vram_bank_enable[7] = false;
	lcd_stat.vram_bank_enable[8] = false;

	//Inverse LUT
	for(int x = 0, y = 7; x < 8; x++, y--) { inv_lut[x] = y; }

	//Screen offset LUT
	for(int x = 0; x < 512; x++)
	{
		screen_offset_lut[x] = (x > 255) ? 0x800 : 0x0;
	}

	//Initialize system screen dimensions
	config::sys_width = 256;
	config::sys_height = 384;
}

/****** Initialize LCD with SDL ******/
bool NTR_LCD::init()
{
	//Initialize with SDL rendering software or hardware
	if(config::sdl_render)
	{
		//Initialize all of SDL
		if(SDL_Init(SDL_INIT_VIDEO) == -1)
		{
			std::cout<<"LCD::Error - Could not initialize SDL video\n";
			return false;
		}

		//Setup OpenGL rendering
		if(config::use_opengl) { opengl_init(); }

		//Set up software rendering
		else
		{
			window = SDL_CreateWindow("GBE+", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, config::sys_width, config::sys_height, config::flags);
			SDL_GetWindowSize(window, &config::win_width, &config::win_height);
			final_screen = SDL_GetWindowSurface(window);
			original_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, config::sys_width, config::sys_height, 32, 0, 0, 0, 0);
			config::scaling_factor = 1;
		}

		if(final_screen == NULL) { return false; }

		SDL_SetWindowIcon(window, util::load_icon(config::data_path + "icons/gbe_plus.bmp"));
	}

	//Initialize with only a buffer for OpenGL (for external rendering)
	else if((!config::sdl_render) && (config::use_opengl))
	{
		final_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, config::sys_width, config::sys_height, 32, 0, 0, 0, 0);
	}

	std::cout<<"LCD::Initialized\n";

	return true;
}

/****** Updates OAM entries when values in memory change ******/
void NTR_LCD::update_oam()
{
	lcd_stat.oam_update = false;
	
	u32 oam_ptr = 0x7000000;
	u16 attribute = 0;

	for(int x = 0; x < 256; x++)
	{
		//Update if OAM entry has changed
		if(lcd_stat.oam_update_list[x])
		{
			lcd_stat.oam_update_list[x] = false;

			//Read and parse Attribute 0
			attribute = mem->read_u16_fast(oam_ptr);
			oam_ptr += 2;

			obj[x].y = (attribute & 0xFF);
			obj[x].affine_enable = (attribute & 0x100) ? 1 : 0;
			obj[x].type = (attribute & 0x200) ? 1 : 0;
			obj[x].mode = (attribute >> 10) & 0x3;
			obj[x].mosiac = (attribute >> 12) & 0x1;
			obj[x].bit_depth = (attribute & 0x2000) ? 8 : 4;
			obj[x].shape = (attribute >> 14);

			if((obj[x].affine_enable == 0) && (obj[x].type == 1)) { obj[x].visible = false; }
			else { obj[x].visible = true; }

			//Read and parse Attribute 1
			attribute = mem->read_u16_fast(oam_ptr);
			oam_ptr += 2;

			obj[x].x = (attribute & 0x1FF);
			obj[x].h_flip = (attribute & 0x1000) ? true : false;
			obj[x].v_flip = (attribute & 0x2000) ? true : false;
			obj[x].size = (attribute >> 14);

			if(obj[x].affine_enable) { obj[x].affine_group = (attribute >> 9) & 0x1F; }

			//Read and parse Attribute 2
			attribute = mem->read_u16_fast(oam_ptr);
			oam_ptr += 4;

			obj[x].tile_number = (attribute & 0x3FF);
			obj[x].bg_priority = ((attribute >> 10) & 0x3);
			obj[x].palette_number = ((attribute >> 12) & 0xF); 

			//Determine dimensions of the sprite
			switch(obj[x].size)
			{
				//Size 0 - 8x8, 16x8, 8x16
				case 0x0:
					if(obj[x].shape == 0) { obj[x].width = 8; obj[x].height = 8; }
					else if(obj[x].shape == 1) { obj[x].width = 16; obj[x].height = 8; }
					else if(obj[x].shape == 2) { obj[x].width = 8; obj[x].height = 16; }
					break;

				//Size 1 - 16x16, 32x8, 8x32
				case 0x1:
					if(obj[x].shape == 0) { obj[x].width = 16; obj[x].height = 16; }
					else if(obj[x].shape == 1) { obj[x].width = 32; obj[x].height = 8; }
					else if(obj[x].shape == 2) { obj[x].width = 8; obj[x].height = 32; }
					break;

				//Size 2 - 32x32, 32x16, 16x32
				case 0x2:
					if(obj[x].shape == 0) { obj[x].width = 32; obj[x].height = 32; }
					else if(obj[x].shape == 1) { obj[x].width = 32; obj[x].height = 16; }
					else if(obj[x].shape == 2) { obj[x].width = 16; obj[x].height = 32; }
					break;

				//Size 3 - 64x64, 64x32, 32x64
				case 0x3:
					if(obj[x].shape == 0) { obj[x].width = 64; obj[x].height = 64; }
					else if(obj[x].shape == 1) { obj[x].width = 64; obj[x].height = 32; }
					else if(obj[x].shape == 2) { obj[x].width = 32; obj[x].height = 64; }
					break;
			}

			//Precalulate OBJ boundaries
			obj[x].left = obj[x].x;
			obj[x].right = (obj[x].x + obj[x].width - 1) & 0x1FF;

			obj[x].top = obj[x].y;
			obj[x].bottom = (obj[x].y + obj[x].height - 1) & 0xFF;

			//Precalculate OBJ wrapping
			if(obj[x].left > obj[x].right) 
			{
				obj[x].x_wrap = true;
				obj[x].x_wrap_val = (obj[x].width - obj[x].right - 1);
			}

			else { obj[x].x_wrap = false; }

			if(obj[x].top > obj[x].bottom)
			{
				obj[x].y_wrap = true;
				obj[x].y_wrap_val = (obj[x].height - obj[x].bottom - 1);
			}

			else { obj[x].y_wrap = false; }

			//Precalculate OBJ base address
			u8 boundary = (x < 128) ? lcd_stat.obj_boundary_a : lcd_stat.obj_boundary_b;
			u32 base = (x < 128) ? 0x6400000 : 0x6600000;
			obj[x].addr = (obj[x].tile_number * boundary) + base;

			//Read and parse OAM affine attribute
			attribute = mem->read_u16_fast(oam_ptr - 2);
			
			//Only update if this attribute is non-zero
			if(attribute)
			{	
				if(attribute & 0x8000) 
				{ 
					u16 temp = ((attribute >> 8) - 1);
					temp = (~temp & 0xFF);
					lcd_stat.obj_affine[x] = -1.0 * temp;
				}

				else { lcd_stat.obj_affine[x] = (attribute >> 8); }

				if((attribute & 0xFF) != 0) { lcd_stat.obj_affine[x] += (attribute & 0xFF) / 256.0; }
			}

			else { lcd_stat.obj_affine[x] = 0.0; }
		}

		else { oam_ptr += 8; }
	}

	//Update OBJ for affine transformations
	//update_obj_affine_transformation();

	//Update render list for the current scanline
	update_obj_render_list();
}

/****** Updates a list of OBJs to render on the current scanline ******/
void NTR_LCD::update_obj_render_list()
{
	obj_render_length_a = 0;
	obj_render_length_b = 0;

	//Sort them based on BG priorities
	for(int bg = 0; bg < 4; bg++)
	{
		//Cycle through all of the OBJs
		for(int x = 0; x < 256; x++)
		{	
			//Check to see if sprite is rendered on the current scanline
			if(!obj[x].visible) { continue; }
			else if((!obj[x].y_wrap) && ((lcd_stat.current_scanline < obj[x].top) || (lcd_stat.current_scanline > obj[x].bottom))) { continue; }
			else if((obj[x].y_wrap) && ((lcd_stat.current_scanline > obj[x].bottom) && (lcd_stat.current_scanline < obj[x].top))) { continue; }

			else if(obj[x].bg_priority == bg)
			{
				if(x < 128) { obj_render_list_a[obj_render_length_a++] = x; }
				else { obj_render_list_b[obj_render_length_b++] = x; } 
			}
		}
	}
}

/****** Updates palette entries when values in memory change ******/
void NTR_LCD::update_palettes()
{
	//Update BG palettes - Engine A
	if(lcd_stat.bg_pal_update_a)
	{
		lcd_stat.bg_pal_update_a = false;

		//Cycle through all updates to BG palettes
		for(int x = 0; x < 256; x++)
		{
			//If this palette has been updated, convert to ARGB
			if(lcd_stat.bg_pal_update_list_a[x])
			{
				lcd_stat.bg_pal_update_list_a[x] = false;

				u16 color_bytes = mem->read_u16_fast(0x5000000 + (x << 1));
				lcd_stat.raw_bg_pal_a[x] = color_bytes;

				u8 red = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 green = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 blue = ((color_bytes & 0x1F) << 3);

				lcd_stat.bg_pal_a[x] =  0xFF000000 | (red << 16) | (green << 8) | (blue);
			}
		}
	}

	//Update Extended BG palettes - Engine A
	if(lcd_stat.bg_ext_pal_update_a)
	{
		lcd_stat.bg_ext_pal_update_a = false;

		//Cycle through all updates to Extended BG palettes
		for(int x = 0; x < 0x4000; x++)
		{
			//If this palette has been updated, convert to ARGB
			if(lcd_stat.bg_ext_pal_update_list_a[x])
			{
				lcd_stat.bg_ext_pal_update_list_a[x] = false;

				u16 color_bytes = mem->read_u16_fast(0x6880000 + (x << 1));
				lcd_stat.raw_bg_ext_pal_a[x] = color_bytes;

				u8 red = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 green = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 blue = ((color_bytes & 0x1F) << 3);

				lcd_stat.bg_ext_pal_a[x] =  0xFF000000 | (red << 16) | (green << 8) | (blue);
			}
		}
	}

	//Update Extended BG palettes - Engine B
	if(lcd_stat.bg_ext_pal_update_b)
	{
		lcd_stat.bg_ext_pal_update_b = false;

		//Cycle through all updates to Extended BG palettes
		for(int x = 0; x < 0x4000; x++)
		{
			//If this palette has been updated, convert to ARGB
			if(lcd_stat.bg_ext_pal_update_list_b[x])
			{
				lcd_stat.bg_ext_pal_update_list_b[x] = false;

				u16 color_bytes = mem->read_u16_fast(0x6898000 + (x << 1));
				lcd_stat.raw_bg_ext_pal_b[x] = color_bytes;

				u8 red = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 green = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 blue = ((color_bytes & 0x1F) << 3);

				lcd_stat.bg_ext_pal_b[x] =  0xFF000000 | (red << 16) | (green << 8) | (blue);
			}
		}
	}

	//Update BG palettes - Engine B
	if(lcd_stat.bg_pal_update_b)
	{
		lcd_stat.bg_pal_update_b = false;

		//Cycle through all updates to BG palettes
		for(int x = 0; x < 256; x++)
		{
			//If this palette has been updated, convert to ARGB
			if(lcd_stat.bg_pal_update_list_b[x])
			{
				lcd_stat.bg_pal_update_list_b[x] = false;

				u16 color_bytes = mem->read_u16_fast(0x5000400 + (x << 1));
				lcd_stat.raw_bg_pal_b[x] = color_bytes;

				u8 red = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 green = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 blue = ((color_bytes & 0x1F) << 3);

				lcd_stat.bg_pal_b[x] =  0xFF000000 | (red << 16) | (green << 8) | (blue);
			}
		}
	}

	//Update OBJ palettes - Engine A
	if(lcd_stat.obj_pal_update_a)
	{
		lcd_stat.obj_pal_update_a = false;

		//Cycle through all updates to BG palettes
		for(int x = 0; x < 256; x++)
		{
			//If this palette has been updated, convert to ARGB
			if(lcd_stat.obj_pal_update_list_a[x])
			{
				lcd_stat.obj_pal_update_list_a[x] = false;

				u16 color_bytes = mem->read_u16_fast(0x5000200 + (x << 1));
				lcd_stat.raw_obj_pal_a[x] = color_bytes;

				u8 red = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 green = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 blue = ((color_bytes & 0x1F) << 3);

				lcd_stat.obj_pal_a[x] =  0xFF000000 | (red << 16) | (green << 8) | (blue);
			}
		}
	}

	//Update Extended OBJ palettes - Engine A
	if(lcd_stat.obj_ext_pal_update_a)
	{
		lcd_stat.obj_ext_pal_update_a = false;

		//Cycle through all updates to Extended OBJ palettes
		for(u16 x = 0; x < 4096; x++)
		{
			//If this palette has been updated, convert to ARGB
			if(lcd_stat.obj_ext_pal_update_list_a[x])
			{
				lcd_stat.obj_ext_pal_update_list_a[x] = false;

				u16 color_bytes = (lcd_stat.vram_bank_enable[5]) ? mem->read_u16_fast(0x6890000 + (x << 1)) : mem->read_u16_fast(0x6894000 + (x << 1));
				lcd_stat.raw_obj_ext_pal_a[x] = color_bytes;

				u8 red = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 green = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 blue = ((color_bytes & 0x1F) << 3);

				lcd_stat.obj_ext_pal_a[x] =  0xFF000000 | (red << 16) | (green << 8) | (blue);
			}
		}
	}

	//Update OBJ palettes - Engine B
	if(lcd_stat.obj_pal_update_b)
	{
		lcd_stat.obj_pal_update_b = false;

		//Cycle through all updates to BG palettes
		for(int x = 0; x < 256; x++)
		{
			//If this palette has been updated, convert to ARGB
			if(lcd_stat.obj_pal_update_list_b[x])
			{
				lcd_stat.obj_pal_update_list_b[x] = false;

				u16 color_bytes = mem->read_u16_fast(0x5000600 + (x << 1));
				lcd_stat.raw_obj_pal_b[x] = color_bytes;

				u8 red = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 green = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 blue = ((color_bytes & 0x1F) << 3);

				lcd_stat.obj_pal_b[x] =  0xFF000000 | (red << 16) | (green << 8) | (blue);
			}
		}
	}

	//Update Extended OBJ palettes - Engine B
	if(lcd_stat.obj_ext_pal_update_b)
	{
		lcd_stat.obj_ext_pal_update_b = false;

		//Cycle through all updates to Extended OBJ palettes
		for(u16 x = 0; x < 4096; x++)
		{
			//If this palette has been updated, convert to ARGB
			if(lcd_stat.obj_ext_pal_update_list_b[x])
			{
				lcd_stat.obj_ext_pal_update_list_b[x] = false;

				u16 color_bytes = mem->read_u16_fast(0x68A0000 + (x << 1));
				lcd_stat.raw_obj_ext_pal_b[x] = color_bytes;

				u8 red = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 green = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 blue = ((color_bytes & 0x1F) << 3);

				lcd_stat.obj_ext_pal_b[x] =  0xFF000000 | (red << 16) | (green << 8) | (blue);
			}
		}
	}
}

/****** Render the line for a BG ******/
void NTR_LCD::render_bg_scanline(u32 bg_control)
{
	u8 bg_mode = (bg_control & 0x1000) ? lcd_stat.bg_mode_b : lcd_stat.bg_mode_a;
	u8 bg_render_list[4];
	u8 bg_id = 0;

	//Render Engine A
	if((bg_control & 0x1000) == 0)
	{
		//Reset render buffer
		full_scanline_render_a = false;
		render_buffer_a.assign(0x100, 0);

		//Clear scanline with backdrop
		for(u16 x = 0; x < 256; x++) { scanline_buffer_a[x] = lcd_stat.bg_pal_a[0]; }

		//Render OBJs if possible
		if(lcd_stat.display_control_a & 0x1000) { render_obj_scanline(bg_control); }

		//Determine BG priority
		for(int x = 0, list_length = 0; x < 4; x++)
		{
			if(lcd_stat.bg_priority_a[0] == x) { bg_render_list[list_length++] = 0; }
			if(lcd_stat.bg_priority_a[1] == x) { bg_render_list[list_length++] = 1; }
			if(lcd_stat.bg_priority_a[2] == x) { bg_render_list[list_length++] = 2; }
			if(lcd_stat.bg_priority_a[3] == x) { bg_render_list[list_length++] = 3; }
		}

		//Render BGs based on priority (3 is the 'lowest', 0 is the 'highest')
		for(int x = 0; x < 4; x++)
		{
			bg_id = bg_render_list[x];
			bg_control = NDS_BG0CNT_A + (bg_id << 1);

			switch(lcd_stat.bg_mode_a)
			{
				//BG Mode 0
				case 0x0:
					render_bg_mode_text(bg_control);
					break;

				//BG Mode 1
				case 0x1:
					
					//BG0-2 Text
					if(bg_id != 3) { render_bg_mode_text(bg_control); }
					
					//BG3 Affine
					else { render_bg_mode_affine(bg_control); }

					break;

				//BG Mode 2
				case 0x2:
					//BG0-1 Text
					if(bg_id < 2) { render_bg_mode_text(bg_control); }

					//BG2-3 Affine
					else { render_bg_mode_affine(bg_control); }

					break;

				//BG Mode 3
				case 0x3:
					//BG0-2 Text
					if(bg_id != 3) { render_bg_mode_text(bg_control); }

					//BG3 Extended
					else
					{
						u8 ext_mode = (lcd_stat.bg_control_a[bg_id] & 0x80) | (lcd_stat.bg_control_a[bg_id] & 0x4);
						switch(ext_mode)
						{
							//16-bit Affine
							case 0x0:
							case 0x4:
								render_bg_mode_affine_ext(bg_control);
								break;

							//256 color Affine
							case 0x80:
								render_bg_mode_bitmap(bg_control);
								break;

							//Direct color Affine
							case 0x84:
								render_bg_mode_direct(bg_control);
								break;
						}
					}

					break;

				//BG Mode 4
				case 0x4:
					//BG0-1 Text
					if(bg_id < 2) { render_bg_mode_text(bg_control); }

					//BG2 Affine
					else if(bg_id == 2) { render_bg_mode_affine(bg_control); }

					//BG3 Extended
					else
					{
						u8 ext_mode = (lcd_stat.bg_control_a[bg_id] & 0x80) | (lcd_stat.bg_control_a[bg_id] & 0x4);
						switch(ext_mode)
						{
							//16-bit Affine
							case 0x0:
							case 0x4:
								render_bg_mode_affine_ext(bg_control);
								break;

							//256 color Affine
							case 0x80:
								render_bg_mode_bitmap(bg_control);
								break;

							//Direct color Affine
							case 0x84:
								render_bg_mode_direct(bg_control);
								break;
						}
					}

					break;

				//BG Mode 5
				case 0x5:
					//BG0-1 Text
					if(bg_id < 2) { render_bg_mode_text(bg_control); }

					//BG2-3 Extended
					else
					{
						u16 ext_mode = (lcd_stat.bg_control_a[bg_id] & 0x80) | (lcd_stat.bg_control_a[bg_id] & 0x4);
						switch(ext_mode)
						{
							//16-bit Affine
							case 0x0:
							case 0x4:
								render_bg_mode_affine_ext(bg_control);
								break;

							//256 color Affine
							case 0x80:
								render_bg_mode_bitmap(bg_control);
								break;

							//Direct color Affine
							case 0x84:
								render_bg_mode_direct(bg_control);
								break;
						}
					}

					break;

				default:
					std::cout<<"LCD::Engine A - invalid or unsupported BG Mode : " << std::dec << (u16)lcd_stat.bg_mode_a << "\n";
			}
		}
	}

	//Render Engine B
	else
	{
		//Reset render buffer
		full_scanline_render_b = false;
		render_buffer_b.assign(0x100, 0);

		//Clear scanline with backdrop
		for(u16 x = 0; x < 256; x++) { scanline_buffer_b[x] = lcd_stat.bg_pal_b[0]; }

		//Render OBJs if possible
		if(lcd_stat.display_control_b & 0x1000) { render_obj_scanline(bg_control); }

		//Determine BG priority
		for(int x = 0, list_length = 0; x < 4; x++)
		{
			if(lcd_stat.bg_priority_b[0] == x) { bg_render_list[list_length++] = 0; }
			if(lcd_stat.bg_priority_b[1] == x) { bg_render_list[list_length++] = 1; }
			if(lcd_stat.bg_priority_b[2] == x) { bg_render_list[list_length++] = 2; }
			if(lcd_stat.bg_priority_b[3] == x) { bg_render_list[list_length++] = 3; }
		}

		//Render BGs based on priority (3 is the 'lowest', 0 is the 'highest')
		for(int x = 0; x < 4; x++)
		{
			bg_id = bg_render_list[x];
			bg_control = NDS_BG0CNT_B + (bg_id << 1);

			switch(lcd_stat.bg_mode_b)
			{
				//BG Mode 0
				case 0x0:
					render_bg_mode_text(bg_control);
					break;

				//BG Mode 1
				case 0x1:
					//BG0-2 Text
					if(bg_id != 3) { render_bg_mode_text(bg_control); }
					
					//BG3 Affine
					else { render_bg_mode_affine(bg_control); }

					break;

				//BG Mode 2
				case 0x2:
					//BG0-1 Text
					if(bg_id < 2) { render_bg_mode_text(bg_control); }

					//BG2-3 Affine
					else { render_bg_mode_affine(bg_control); }

					break;

				//BG Mode 3
				case 0x3:
					//BG0-2 Text
					if(bg_id != 3) { render_bg_mode_text(bg_control); }

					//BG3 Extended
					else
					{
						u8 ext_mode = (lcd_stat.bg_control_b[bg_id] & 0x80) | (lcd_stat.bg_control_b[bg_id] & 0x4);
						switch(ext_mode)
						{
							//16-bit Affine
							case 0x0:
							case 0x4:
								render_bg_mode_affine_ext(bg_control);
								break;

							//256 color Affine
							case 0x80:
								render_bg_mode_bitmap(bg_control);
								break;

							//Direct color Affine
							case 0x84:
								render_bg_mode_direct(bg_control);
								break;
						}
					}

					break;

				//BG Mode 4
				case 0x4:
					//BG0-1 Text
					if(bg_id < 2) { render_bg_mode_text(bg_control); }

					//BG2 Affine
					else if(bg_id == 2) { render_bg_mode_affine(bg_control); }

					//BG3 Extended
					else
					{
						u8 ext_mode = (lcd_stat.bg_control_b[bg_id] & 0x80) | (lcd_stat.bg_control_b[bg_id] & 0x4);
						switch(ext_mode)
						{
							//16-bit Affine
							case 0x0:
							case 0x4:
								render_bg_mode_affine_ext(bg_control);
								break;

							//256 color Affine
							case 0x80:
								render_bg_mode_bitmap(bg_control);
								break;

							//Direct color Affine
							case 0x84:
								render_bg_mode_direct(bg_control);
								break;
						}
					}

					break;

				//BG Mode 5
				case 0x5:
					//BG0-1 Text
					if(bg_id < 2) { render_bg_mode_text(bg_control); }

					//BG2-3 Extended
					else
					{
						u8 ext_mode = (lcd_stat.bg_control_b[bg_id] & 0x80) | (lcd_stat.bg_control_b[bg_id] & 0x4);
						switch(ext_mode)
						{
							//16-bit Affine
							case 0x0:
							case 0x4:
								render_bg_mode_affine_ext(bg_control);
								break;

							//256 color Affine
							case 0x80:
								render_bg_mode_bitmap(bg_control);
								break;

							//Direct color Affine
							case 0x84:
								render_bg_mode_direct(bg_control);
								break;
						}
					}

					break;

				default:
					std::cout<<"LCD::Engine B - invalid or unsupported BG Mode : " << std::dec << (u16)lcd_stat.bg_mode_b << "\n";
			}
		}
	}
}

/****** Renders a scanline for OBJs ******/
void NTR_LCD::render_obj_scanline(u32 bg_control)
{
	//Detemine if Engine A or B
	u8 engine_id = (bg_control & 0x1000) ? 1 : 0;

	//Abort if no OBJs are rendered on this line
	if(!engine_id && !obj_render_length_a) { return; }
	else if(engine_id && !obj_render_length_b) { return; }

	u8 obj_id = 0;
	u8 pal_id = 0;
	u8 obj_render_length = engine_id ? obj_render_length_b : obj_render_length_a;
	u16 scanline_pixel_counter = 0;
	u8 render_width = 0;
	u8 raw_color = 0;
	bool ext_pal = false;
	s16 h_flip, v_flip = 0;

	if((!engine_id) && (lcd_stat.ext_pal_a & 0x2)) { ext_pal = true; }
	else if((engine_id) && (lcd_stat.ext_pal_b & 0x2)) { ext_pal = true; }

	//Cycle through all current OBJ and render them based on their priority
	for(int x = 0; x < obj_render_length; x++)
	{
		obj_id = engine_id ? obj_render_list_b[x] : obj_render_list_a[x];
		pal_id = obj[obj_id].palette_number;
		
		//Check to see if OBJ is even onscreen
		if((obj[obj_id].left < 256) || (obj[obj_id].right < 256))
		{
			//Start rendering from X coordinate
			scanline_pixel_counter = obj[obj_id].left;
			render_width = 0;

			u8 meta_width = obj[obj_id].width / 8;
			u8 bit_depth = obj[obj_id].bit_depth * 8;
			u8 pixel_shift = (bit_depth == 32) ? 1 : 0;

			while(render_width < obj[obj_id].width)
			{
				if(scanline_pixel_counter < 256)
				{
					//Determine X and Y meta-tiles
					u16 obj_x = render_width;
					u16 obj_y = obj[obj_id].y_wrap ? (lcd_stat.current_scanline + obj[obj_id].y_wrap_val) : (lcd_stat.current_scanline - obj[obj_id].y);

					//Horizontal flip the internal X coordinate
					if(obj[obj_id].h_flip)
					{
						h_flip = obj_x;
						h_flip -= (obj[obj_id].width - 1);

						if(h_flip < 0) { h_flip *= -1; }

						obj_x = h_flip;
					}

					//Vertical flip the internal Y coordinate
					if(obj[obj_id].v_flip)
					{
						v_flip = obj_y;
						v_flip -= (obj[obj_id].height - 1);

						if(v_flip < 0) { v_flip *= -1; }

						obj_y = v_flip;
					}

					u8 meta_x = obj_x / 8;
					u8 meta_y = obj_y / 8;

					//Determine address of this pixel
					u32 obj_addr = obj[obj_id].addr + (((meta_y * meta_width) + meta_x) * bit_depth);
					obj_addr += (((obj_y % 8) * 8) + (obj_x % 8)) >> pixel_shift;

					raw_color = mem->read_u8(obj_addr);

					//Process 4-bit depth if necessary
					if((bit_depth == 32) && (!ext_pal))
					{
						raw_color = (obj_x & 0x1) ? (raw_color >> 4) : (raw_color & 0xF);
						raw_color += (pal_id * 16);
					}

					//Draw for Engine A
					if(!engine_id && raw_color && !render_buffer_a[scanline_pixel_counter])
					{
						scanline_buffer_a[scanline_pixel_counter] = (ext_pal) ? lcd_stat.obj_ext_pal_a[(pal_id * 256) + raw_color] : lcd_stat.obj_pal_a[raw_color];
						render_buffer_a[scanline_pixel_counter] = (obj[obj_id].bg_priority + 1);
					}

					//Draw for Engine B
					else if(engine_id && raw_color && !render_buffer_b[scanline_pixel_counter])
					{
						scanline_buffer_b[scanline_pixel_counter] = (ext_pal) ? lcd_stat.obj_ext_pal_b[(pal_id * 256) + raw_color] : lcd_stat.obj_pal_b[raw_color];
						render_buffer_b[scanline_pixel_counter] = (obj[obj_id].bg_priority + 1);
					}
						
				}

				scanline_pixel_counter++;
				scanline_pixel_counter &= 0x1FF;

				render_width++;
			}
		}
	}
}			

/****** Render BG Mode Text scanline ******/
void NTR_LCD::render_bg_mode_text(u32 bg_control)
{
	//Render Engine A
	if((bg_control & 0x1000) == 0)
	{
		//Grab BG ID
		u8 bg_id = (bg_control - 0x4000008) >> 1;
		u8 bg_priority = lcd_stat.bg_priority_a[bg_id] + 1;

		//Abort rendering if this BG is disabled
		if(!lcd_stat.bg_enable_a[bg_id]) { return; }

		//Abort rendering if BGs with high priority have already completely rendered a scanline
		if(full_scanline_render_a) { return; }

		bool full_render = true;

		//Grab tile offsets
		u8 tile_offset_x = lcd_stat.bg_offset_x_a[bg_id] & 0x7;

		u16 tile_id;
		u16 ext_pal_id;
		u8 pal_id;
		u8 flip;

		u16 scanline_pixel_counter = 0;
		u8 current_screen_line = (lcd_stat.current_scanline + lcd_stat.bg_offset_y_a[bg_id]);

		u8 current_screen_pixel = lcd_stat.bg_offset_x_a[bg_id];
		u16 current_tile_line = (lcd_stat.current_scanline + lcd_stat.bg_offset_y_a[bg_id]) % 8;

		//Grab BG bit-depth and offset for the current tile line
		u8 bit_depth = lcd_stat.bg_depth_a[bg_id] ? 64 : 32;
		u8 line_offset;

		//Get tile and map addresses
		u32 tile_addr = 0x6000000 + lcd_stat.bg_base_tile_addr_a[bg_id];
		u32 map_addr_base = 0x6000000 + lcd_stat.bg_base_map_addr_a[bg_id];
		u32 map_addr = 0;
 
		//Cycle through all tiles on this scanline
		for(u32 x = 0; x < 256;)
		{
			//Determine meta x-coordinate of rendered BG pixel
			u16 meta_x = ((scanline_pixel_counter + lcd_stat.bg_offset_x_a[bg_id]) & lcd_stat.text_width_a[bg_id]);

			//Determine meta Y-coordinate of rendered BG pixel
			u16 meta_y = ((lcd_stat.current_scanline + lcd_stat.bg_offset_y_a[bg_id]) & lcd_stat.text_height_a[bg_id]);

			//Determine the address offset for the screen
			switch(lcd_stat.bg_size_a[bg_id])
			{
				//Size 0 - 256x256
				case 0x0:
					map_addr = map_addr_base;
					break;

				//Size 1 - 512x256
				case 0x1: 
					map_addr = map_addr_base + screen_offset_lut[meta_x];
					break;

				//Size 2 - 256x512
				case 0x2:
					map_addr = map_addr_base + screen_offset_lut[meta_y];
					break;

				//Size 3 - 512x512
				case 0x3:
					map_addr = screen_offset_lut[meta_x];
					if(meta_y > 255) { map_addr |= 0x1000; }
					map_addr += map_addr_base;
					break;
			}
				
			//Determine which map entry to start looking up tiles
			u16 map_entry = ((current_screen_line >> 3) << 5);
			map_entry += (current_screen_pixel >> 3);

			//Pull map data from current map entry
			u16 map_data = mem->read_u16(map_addr + (map_entry << 1));

			//Get tile, palette number, and flipping parameters
			tile_id = (map_data & 0x3FF);
			pal_id = (map_data >> 12) & 0xF;
			ext_pal_id = (bg_id << 12) + (pal_id << 8);
			flip = (map_data >> 10) & 0x3;

			//Calculate VRAM address to start pulling up tile data
			line_offset = (flip & 0x2) ? ((bit_depth >> 3) * inv_lut[current_tile_line]) : ((bit_depth >> 3) * current_tile_line);
			u32 tile_data_addr = tile_addr + (tile_id * bit_depth) + line_offset;
			if(flip & 0x1) { tile_data_addr += ((bit_depth >> 3) - 1); }

			//Read 8 pixels from VRAM and put them in the scanline buffer
			for(u32 y = tile_offset_x; y < 8; y++, x++)
			{
				//Process 8-bit depth
				if(bit_depth == 64)
				{
					//Grab dot-data, account for horizontal flipping 
					u8 raw_color = (flip & 0x1) ? mem->read_u8(tile_data_addr--) : mem->read_u8(tile_data_addr++);

					//Only draw if no previous pixel was rendered
					if(!render_buffer_a[scanline_pixel_counter] || (bg_priority < render_buffer_a[scanline_pixel_counter]))
					{
						//Only draw colors if not transparent
						if(raw_color)
						{
							scanline_buffer_a[scanline_pixel_counter] = (lcd_stat.ext_pal_a) ? lcd_stat.bg_ext_pal_a[ext_pal_id + raw_color]  : lcd_stat.bg_pal_a[raw_color];
							render_buffer_a[scanline_pixel_counter] = bg_priority;
						}

						else { full_render = false; }
					}

					//Draw 256 pixels max
					scanline_pixel_counter++;
					current_screen_pixel++;
					if(scanline_pixel_counter & 0x100) { return; }
				}

				//Process 4-bit depth
				else
				{
					//Grab dot-data, account for horizontal flipping 
					u8 raw_color = (flip & 0x1) ? mem->read_u8(tile_data_addr--) : mem->read_u8(tile_data_addr++);

					u8 pal_1 = (pal_id * 16) + (raw_color & 0xF);
					u8 pal_2 = (pal_id * 16) + (raw_color >> 4);

					//Only draw if no previous pixel was rendered
					if(!render_buffer_a[scanline_pixel_counter] || (bg_priority < render_buffer_a[scanline_pixel_counter]))
					{
						//Only draw colors if not transparent
						if(raw_color & 0xF)
						{
							scanline_buffer_a[scanline_pixel_counter] = lcd_stat.bg_pal_a[pal_1];
							render_buffer_a[scanline_pixel_counter] = bg_priority;
						}

						else { full_render = false; }
					}

					//Draw 256 pixels max
					scanline_pixel_counter++;
					if(scanline_pixel_counter & 0x100) { return; }

					//Only draw if no previous pixel was rendered
					if(!render_buffer_a[scanline_pixel_counter] || (bg_priority < render_buffer_a[scanline_pixel_counter]))
					{
						//Only draw colors if not transparent
						if(raw_color >> 4)
						{
							scanline_buffer_a[scanline_pixel_counter] = lcd_stat.bg_pal_a[pal_2];
							render_buffer_a[scanline_pixel_counter] = bg_priority;
						}

						else { full_render = false; }
					}

					//Draw 256 pixels max
					scanline_pixel_counter++;
					if(scanline_pixel_counter & 0x100) { return; }

					current_screen_pixel += 2;
					y++;
				}
			}

			tile_offset_x = 0;
		}

		full_scanline_render_a = full_render;
	}

	//Render Engine B
	else
	{
		//Grab BG ID
		u8 bg_id = (bg_control - 0x4001008) >> 1;
		u8 bg_priority = lcd_stat.bg_priority_b[bg_id] + 1;

		//Abort rendering if this bg is disabled
		if(!lcd_stat.bg_enable_b[bg_id]) { return; }

		//Abort rendering if BGs with high priority have already completely rendered a scanline
		if(full_scanline_render_b) { return; }

		bool full_render = true;

		//Grab tile offsets
		u8 tile_offset_x = lcd_stat.bg_offset_x_b[bg_id] & 0x7;

		u16 tile_id;
		u16 ext_pal_id;
		u8 pal_id;
		u8 flip;

		u16 scanline_pixel_counter = 0;
		u8 current_screen_line = (lcd_stat.current_scanline + lcd_stat.bg_offset_y_b[bg_id]);

		u8 current_screen_pixel = lcd_stat.bg_offset_x_b[bg_id];
		u16 current_tile_line = (lcd_stat.current_scanline + lcd_stat.bg_offset_y_b[bg_id]) % 8;

		//Grab BG bit-depth and offset for the current tile line
		u8 bit_depth = lcd_stat.bg_depth_b[bg_id] ? 64 : 32;
		u8 line_offset;

		//Get tile and map addresses
		u32 tile_addr = 0x6200000 + lcd_stat.bg_base_tile_addr_b[bg_id];
		u32 map_addr_base = 0x6200000 + lcd_stat.bg_base_map_addr_b[bg_id];
		u32 map_addr = 0;

		//Cycle through all tiles on this scanline
		for(u32 x = 0; x < 256;)
		{
			//Determine meta x-coordinate of rendered BG pixel
			u16 meta_x = ((scanline_pixel_counter + lcd_stat.bg_offset_x_b[bg_id]) & lcd_stat.text_width_b[bg_id]);

			//Determine meta Y-coordinate of rendered BG pixel
			u16 meta_y = ((lcd_stat.current_scanline + lcd_stat.bg_offset_y_b[bg_id]) & lcd_stat.text_height_b[bg_id]);

			//Determine the address offset for the screen
			switch(lcd_stat.bg_size_b[bg_id])
			{
				//Size 0 - 256x256
				case 0x0:
					map_addr = map_addr_base;
					break;

				//Size 1 - 512x256
				case 0x1: 
					map_addr = map_addr_base + screen_offset_lut[meta_x];
					break;

				//Size 2 - 256x512
				case 0x2:
					map_addr = map_addr_base + screen_offset_lut[meta_y];
					break;

				//Size 3 - 512x512
				case 0x3:
					map_addr = screen_offset_lut[meta_x];
					if(meta_y > 255) { map_addr |= 0x1000; }
					map_addr += map_addr_base;
					break;
			}

			//Determine which map entry to start looking up tiles
			u16 map_entry = ((current_screen_line >> 3) << 5);
			map_entry += (current_screen_pixel >> 3);

			//Pull map data from current map entry
			u16 map_data = mem->read_u16(map_addr + (map_entry << 1));

			//Get tile, palette number, and flipping parameters
			tile_id = (map_data & 0x3FF);
			pal_id = (map_data >> 12) & 0xF;
			ext_pal_id = (bg_id << 12) + (pal_id << 8);
			flip = (map_data >> 10) & 0x3;

			//Calculate VRAM address to start pulling up tile data
			line_offset = (flip & 0x2) ? ((bit_depth >> 3) * inv_lut[current_tile_line]) : ((bit_depth >> 3) * current_tile_line);
			u32 tile_data_addr = tile_addr + (tile_id * bit_depth) + line_offset;
			if(flip & 0x1) { tile_data_addr += ((bit_depth >> 3) - 1); }

			//Read 8 pixels from VRAM and put them in the scanline buffer
			for(u32 y = tile_offset_x; y < 8; y++, x++)
			{
				//Process 8-bit depth
				if(bit_depth == 64)
				{
					//Grab dot-data, account for horizontal flipping 
					u8 raw_color = (flip & 0x1) ? mem->read_u8(tile_data_addr--) : mem->read_u8(tile_data_addr++);

					//Only draw if no previous pixel was rendered
					if(!render_buffer_b[scanline_pixel_counter] || (bg_priority < render_buffer_b[scanline_pixel_counter]))
					{
						//Only draw colors if not transparent
						if(raw_color)
						{
							scanline_buffer_b[scanline_pixel_counter] = (lcd_stat.ext_pal_b) ? lcd_stat.bg_ext_pal_b[ext_pal_id + raw_color]  : lcd_stat.bg_pal_b[raw_color];
							render_buffer_b[scanline_pixel_counter] = bg_priority;
						}

						else { full_render = false; }
					}

					//Draw 256 pixels max
					scanline_pixel_counter++;
					current_screen_pixel++;
					if(scanline_pixel_counter & 0x100) { return; }
				}

				//Process 4-bit depth
				else
				{
					//Grab dot-data, account for horizontal flipping 
					u8 raw_color = (flip & 0x1) ? mem->read_u8(tile_data_addr--) : mem->read_u8(tile_data_addr++);

					u8 pal_1 = (pal_id * 16) + (raw_color & 0xF);
					u8 pal_2 = (pal_id * 16) + (raw_color >> 4);

					//Only draw if no previous pixel was rendered
					if(!render_buffer_b[scanline_pixel_counter] || (bg_priority < render_buffer_b[scanline_pixel_counter]))
					{
						//Only draw colors if not transparent
						if(raw_color & 0xF)
						{
							scanline_buffer_b[scanline_pixel_counter] = lcd_stat.bg_pal_b[pal_1];
							render_buffer_b[scanline_pixel_counter] = bg_priority;
						}

						else { full_render = false; }
					}

					//Draw 256 pixels max
					scanline_pixel_counter++;
					if(scanline_pixel_counter & 0x100) { return; }

					//Only draw if no previous pixel was rendered
					if(!render_buffer_b[scanline_pixel_counter] || (bg_priority < render_buffer_b[scanline_pixel_counter]))
					{
						//Only draw colors if not transparent
						if(raw_color >> 4)
						{
							scanline_buffer_b[scanline_pixel_counter] = lcd_stat.bg_pal_b[pal_2];
							render_buffer_b[scanline_pixel_counter] = bg_priority;
						}

						else { full_render = false; }
					}

					//Draw 256 pixels max
					scanline_pixel_counter++;
					if(scanline_pixel_counter & 0x100) { return; }

					current_screen_pixel += 2;
					y++;
				}
			}

			tile_offset_x = 0;
		}		
	}
}

/****** Render BG Mode Affine scanline ******/
void NTR_LCD::render_bg_mode_affine(u32 bg_control)
{
	//Render Engine A
	if((bg_control & 0x1000) == 0)
	{
		//Grab BG ID and affine ID
		u8 bg_id = (bg_control - 0x4000008) >> 1;
		u8 affine_id = (bg_id & 0x1);

		//Reload X-Y references at start of frame
		if(lcd_stat.current_scanline == 0) { reload_affine_references(bg_control); }

		//Abort rendering if this bg is disabled
		if(!lcd_stat.bg_enable_a[bg_id]) { return; }

		//Abort rendering if BGs with high priority have already completely rendered a scanline
		if(full_scanline_render_a) { return; }

		//Get BG size in tiles, pixels
		//0 - 128x128, 1 - 256x256, 2 - 512x512, 3 - 1024x1024
		u16 bg_tile_size = (16 << (lcd_stat.bg_control_a[bg_id] >> 14));
		u16 bg_pixel_size = bg_tile_size << 3;

		u16 scanline_pixel_counter = 0;
		u16 src_x, src_y = 0;
		double new_x, new_y = 0.0;

		//Set current texture position at X and Y references
		lcd_stat.bg_affine_a[affine_id].x_pos = lcd_stat.bg_affine_a[affine_id].x_ref - lcd_stat.bg_affine_a[affine_id].dx;
		lcd_stat.bg_affine_a[affine_id].y_pos = lcd_stat.bg_affine_a[affine_id].y_ref - lcd_stat.bg_affine_a[affine_id].dy;

		//Get tile and map addresses
		u32 tile_base = 0x6000000 + lcd_stat.bg_base_tile_addr_a[bg_id];
		u32 map_base = 0x6000000 + lcd_stat.bg_base_map_addr_a[bg_id];
 
		//Cycle through all tiles on this scanline
		for(u32 x = 0; x < 256; x++, scanline_pixel_counter++)
		{
			bool render_pixel = true;

			//Update texture position with DX and DY
			lcd_stat.bg_affine_a[affine_id].x_pos += lcd_stat.bg_affine_a[affine_id].dx;
			lcd_stat.bg_affine_a[affine_id].y_pos += lcd_stat.bg_affine_a[affine_id].dy;

			new_x = lcd_stat.bg_affine_a[affine_id].x_pos;
			new_y = lcd_stat.bg_affine_a[affine_id].y_pos;

			//Clip BG if coordinates overflow and overflow flag is not set
			if(!lcd_stat.bg_affine_a[affine_id].overflow)
			{
				if((new_x >= bg_pixel_size) || (new_x < 0)) { render_pixel = false; }
				if((new_y >= bg_pixel_size) || (new_y < 0)) { render_pixel = false; }
			}

			//Wrap BG if coordinates overflow and overflow flag is set
			else 
			{
				while(new_x >= bg_pixel_size) { new_x -= bg_pixel_size; }
				while(new_y >= bg_pixel_size) { new_y -= bg_pixel_size; }
				while(new_x < 0) { new_x += bg_pixel_size; }
				while(new_y < 0) { new_y += bg_pixel_size; } 
			}

			if(render_pixel)
			{
				//Determine source pixel X-Y coordinates
				src_x = new_x; 
				src_y = new_y;

				//Get current map entry for rendered pixel
				u16 tile_number = ((src_y / 8) * bg_tile_size) + (src_x / 8);

				//Look at the Tile Map #(tile_number), see what Tile # it points to
				u8 map_entry = mem->memory_map[map_base + tile_number];

				//Get address of Tile #(map_entry)
				u32 tile_addr = tile_base + (map_entry * 64);

				u8 current_tile_pixel = ((src_y % 8) * 8) + (src_x % 8);

				//Grab the byte corresponding to (current_tile_pixel), render it as ARGB - 8-bit version
				tile_addr += current_tile_pixel;
				u8 raw_color = mem->memory_map[tile_addr];

				//Only draw BG color if not transparent
				if(raw_color != 0) { scanline_buffer_a[scanline_pixel_counter] = lcd_stat.bg_pal_a[raw_color]; }
			}
		}

		//Update XREF and YREF for next line
		lcd_stat.bg_affine_a[affine_id].x_ref += lcd_stat.bg_affine_a[affine_id].dmx;
		lcd_stat.bg_affine_a[affine_id].y_ref += lcd_stat.bg_affine_a[affine_id].dmy;
	}

	//Render Engine B
	else
	{
		//Grab BG ID and affine ID
		u8 bg_id = (bg_control - 0x4001008) >> 1;
		u8 affine_id = (bg_id & 0x1);

		//Reload X-Y references at start of frame
		if(lcd_stat.current_scanline == 0) { reload_affine_references(bg_control); }

		//Abort rendering if this bg is disabled
		if(!lcd_stat.bg_enable_b[bg_id]) { return; }

		//Abort rendering if BGs with high priority have already completely rendered a scanline
		if(full_scanline_render_b) { return; }

		//Get BG size in tiles, pixels
		//0 - 128x128, 1 - 256x256, 2 - 512x512, 3 - 1024x1024
		u16 bg_tile_size = (16 << (lcd_stat.bg_control_b[bg_id] >> 14));
		u16 bg_pixel_size = bg_tile_size << 3;

		u16 scanline_pixel_counter = 0;
		u16 src_x, src_y = 0;
		double new_x, new_y = 0.0;

		//Set current texture position at X and Y references
		lcd_stat.bg_affine_b[affine_id].x_pos = lcd_stat.bg_affine_b[affine_id].x_ref - lcd_stat.bg_affine_b[affine_id].dx;
		lcd_stat.bg_affine_b[affine_id].y_pos = lcd_stat.bg_affine_b[affine_id].y_ref - lcd_stat.bg_affine_b[affine_id].dy;

		//Get tile and map addresses
		u32 tile_base = 0x6200000 + lcd_stat.bg_base_tile_addr_b[bg_id];
		u32 map_base = 0x6200000 + lcd_stat.bg_base_map_addr_b[bg_id];
 
		//Cycle through all tiles on this scanline
		for(u32 x = 0; x < 256; x++, scanline_pixel_counter++)
		{
			bool render_pixel = true;

			//Update texture position with DX and DY
			lcd_stat.bg_affine_b[affine_id].x_pos += lcd_stat.bg_affine_b[affine_id].dx;
			lcd_stat.bg_affine_b[affine_id].y_pos += lcd_stat.bg_affine_b[affine_id].dy;

			new_x = lcd_stat.bg_affine_b[affine_id].x_pos;
			new_y = lcd_stat.bg_affine_b[affine_id].y_pos;

			//Clip BG if coordinates overflow and overflow flag is not set
			if(!lcd_stat.bg_affine_b[affine_id].overflow)
			{
				if((new_x >= bg_pixel_size) || (new_x < 0)) { render_pixel = false; }
				if((new_y >= bg_pixel_size) || (new_y < 0)) { render_pixel = false; }
			}

			//Wrap BG if coordinates overflow and overflow flag is set
			else 
			{
				while(new_x >= bg_pixel_size) { new_x -= bg_pixel_size; }
				while(new_y >= bg_pixel_size) { new_y -= bg_pixel_size; }
				while(new_x < 0) { new_x += bg_pixel_size; }
				while(new_y < 0) { new_y += bg_pixel_size; } 
			}

			if(render_pixel)
			{
				//Determine source pixel X-Y coordinates
				src_x = new_x; 
				src_y = new_y;

				//Get current map entry for rendered pixel
				u16 tile_number = ((src_y / 8) * bg_tile_size) + (src_x / 8);

				//Look at the Tile Map #(tile_number), see what Tile # it points to
				u8 map_entry = mem->memory_map[map_base + tile_number];

				//Get address of Tile #(map_entry)
				u32 tile_addr = tile_base + (map_entry * 64);

				u8 current_tile_pixel = ((src_y % 8) * 8) + (src_x % 8);

				//Grab the byte corresponding to (current_tile_pixel), render it as ARGB - 8-bit version
				tile_addr += current_tile_pixel;
				u8 raw_color = mem->memory_map[tile_addr];

				//Only draw BG color if not transparent
				if(raw_color != 0) { scanline_buffer_b[scanline_pixel_counter] = lcd_stat.bg_pal_b[raw_color]; }
			}
		}

		//Update XREF and YREF for next line
		lcd_stat.bg_affine_b[affine_id].x_ref += lcd_stat.bg_affine_b[affine_id].dmx;
		lcd_stat.bg_affine_b[affine_id].y_ref += lcd_stat.bg_affine_b[affine_id].dmy;
	}
}

/****** Render BG Mode Affine-Extended scanline ******/
void NTR_LCD::render_bg_mode_affine_ext(u32 bg_control)
{
	//Render Engine A
	if((bg_control & 0x1000) == 0)
	{
		//Grab BG ID and affine ID
		u8 bg_id = (bg_control - 0x4000008) >> 1;
		u8 affine_id = (bg_id & 0x1);

		//Reload X-Y references at start of frame
		if(lcd_stat.current_scanline == 0) { reload_affine_references(bg_control); }

		//Abort rendering if this bg is disabled
		if(!lcd_stat.bg_enable_a[bg_id]) { return; }

		//Abort rendering if BGs with high priority have already completely rendered a scanline
		if(full_scanline_render_a) { return; }

		bool full_render = true;

		//Get BG size in tiles, pixels
		//0 - 128x128, 1 - 256x256, 2 - 512x512, 3 - 1024x1024
		u16 bg_tile_size = (16 << (lcd_stat.bg_control_a[bg_id] >> 14));
		u16 bg_pixel_size = bg_tile_size << 3;

		u8 scanline_pixel_counter = 0;
		u16 src_x, src_y = 0;
		double new_x, new_y = 0.0;
		u8 flip = 0;

		//Set current texture position at X and Y references
		lcd_stat.bg_affine_a[affine_id].x_pos = lcd_stat.bg_affine_a[affine_id].x_ref - lcd_stat.bg_affine_a[affine_id].dx;
		lcd_stat.bg_affine_a[affine_id].y_pos = lcd_stat.bg_affine_a[affine_id].y_ref - lcd_stat.bg_affine_a[affine_id].dy;

		//Get tile and map addresses
		u32 tile_base = 0x6000000 + lcd_stat.bg_base_tile_addr_a[bg_id];
		u32 map_base = 0x6000000 + lcd_stat.bg_base_map_addr_a[bg_id];

		//Cycle through all tiles on this scanline
		for(u32 x = 0; x < 256; x++, scanline_pixel_counter++)
		{
			bool render_pixel = true;

			//Update texture position with DX and DY
			lcd_stat.bg_affine_a[affine_id].x_pos += lcd_stat.bg_affine_a[affine_id].dx;
			lcd_stat.bg_affine_a[affine_id].y_pos += lcd_stat.bg_affine_a[affine_id].dy;

			new_x = lcd_stat.bg_affine_a[affine_id].x_pos;
			new_y = lcd_stat.bg_affine_a[affine_id].y_pos;

			//Clip BG if coordinates overflow and overflow flag is not set
			if(!lcd_stat.bg_affine_a[affine_id].overflow)
			{
				if((new_x >= bg_pixel_size) || (new_x < 0)) { render_pixel = false; }
				if((new_y >= bg_pixel_size) || (new_y < 0)) { render_pixel = false; }
			}

			//Wrap BG if coordinates overflow and overflow flag is set
			else 
			{
				while(new_x >= bg_pixel_size) { new_x -= bg_pixel_size; }
				while(new_y >= bg_pixel_size) { new_y -= bg_pixel_size; }
				while(new_x < 0) { new_x += bg_pixel_size; }
				while(new_y < 0) { new_y += bg_pixel_size; } 
			}

			if(render_pixel)
			{
				//Determine source pixel X-Y coordinates
				src_x = new_x; 
				src_y = new_y;

				//Get current map entry for rendered pixel
				u16 tile_number = ((src_y / 8) * bg_tile_size) + (src_x / 8);

				//Look at the Tile Map #(tile_number), see what Tile # it points to
				u16 map_entry = mem->read_u16(map_base + (tile_number << 1));

				//Grab flipping attributes
				flip = (map_entry >> 10) & 0x3;
				src_x = (flip & 0x1) ? inv_lut[src_x % 8] : (src_x % 8);
				src_y = (flip & 0x2) ? inv_lut[src_y % 8] : (src_y % 8);

				//Get address of Tile #(map_entry)
				u32 tile_addr = tile_base + ((map_entry & 0x3FF) * 64);

				u8 current_tile_pixel = (src_y * 8) + src_x;

				//Grab the byte corresponding to (current_tile_pixel), render it as ARGB - 8-bit version
				tile_addr += current_tile_pixel;
				u8 raw_color = mem->memory_map[tile_addr];

				//Only draw if no previous pixel was rendered
				if(!render_buffer_a[scanline_pixel_counter])
				{
					//Only draw BG color if not transparent
					if(raw_color != 0)
					{
						scanline_buffer_a[scanline_pixel_counter] = lcd_stat.bg_pal_a[raw_color];
						render_buffer_a[scanline_pixel_counter] = (bg_id + 1);
					}

					else { full_render = false; }
				}	
			}
		}

		//Update XREF and YREF for next line
		lcd_stat.bg_affine_a[affine_id].x_ref += lcd_stat.bg_affine_a[affine_id].dmx;
		lcd_stat.bg_affine_a[affine_id].y_ref += lcd_stat.bg_affine_a[affine_id].dmy;

		full_scanline_render_a = full_render;
	}

	//Render Engine B
	else
	{
		//Grab BG ID and affine ID
		u8 bg_id = (bg_control - 0x4001008) >> 1;
		u8 affine_id = (bg_id & 0x1);

		//Reload X-Y references at start of frame
		if(lcd_stat.current_scanline == 0) { reload_affine_references(bg_control); }

		//Abort rendering if this bg is disabled
		if(!lcd_stat.bg_enable_b[bg_id]) { return; }

		//Abort rendering if BGs with high priority have already completely rendered a scanline
		if(full_scanline_render_b) { return; }

		bool full_render = true;

		//Get BG size in tiles, pixels
		//0 - 128x128, 1 - 256x256, 2 - 512x512, 3 - 1024x1024
		u16 bg_tile_size = (16 << (lcd_stat.bg_control_b[bg_id] >> 14));
		u16 bg_pixel_size = bg_tile_size << 3;

		u8 scanline_pixel_counter = 0;
		u16 src_x, src_y = 0;
		double new_x, new_y = 0.0;
		u8 flip = 0;

		//Set current texture position at X and Y references
		lcd_stat.bg_affine_b[affine_id].x_pos = lcd_stat.bg_affine_b[affine_id].x_ref - lcd_stat.bg_affine_b[affine_id].dx;
		lcd_stat.bg_affine_b[affine_id].y_pos = lcd_stat.bg_affine_b[affine_id].y_ref - lcd_stat.bg_affine_b[affine_id].dy;

		//Get tile and map addresses
		u32 tile_base = 0x6200000 + lcd_stat.bg_base_tile_addr_b[bg_id];
		u32 map_base = 0x6200000 + lcd_stat.bg_base_map_addr_b[bg_id];

		//Cycle through all tiles on this scanline
		for(u32 x = 0; x < 256; x++, scanline_pixel_counter++)
		{
			bool render_pixel = true;

			//Update texture position with DX and DY
			lcd_stat.bg_affine_b[affine_id].x_pos += lcd_stat.bg_affine_b[affine_id].dx;
			lcd_stat.bg_affine_b[affine_id].y_pos += lcd_stat.bg_affine_b[affine_id].dy;

			new_x = lcd_stat.bg_affine_b[affine_id].x_pos;
			new_y = lcd_stat.bg_affine_b[affine_id].y_pos;

			//Clip BG if coordinates overflow and overflow flag is not set
			if(!lcd_stat.bg_affine_b[affine_id].overflow)
			{
				if((new_x >= bg_pixel_size) || (new_x < 0)) { render_pixel = false; }
				if((new_y >= bg_pixel_size) || (new_y < 0)) { render_pixel = false; }
			}

			//Wrap BG if coordinates overflow and overflow flag is set
			else 
			{
				while(new_x >= bg_pixel_size) { new_x -= bg_pixel_size; }
				while(new_y >= bg_pixel_size) { new_y -= bg_pixel_size; }
				while(new_x < 0) { new_x += bg_pixel_size; }
				while(new_y < 0) { new_y += bg_pixel_size; } 
			}

			if(render_pixel)
			{
				//Determine source pixel X-Y coordinates
				src_x = new_x; 
				src_y = new_y;

				//Get current map entry for rendered pixel
				u16 tile_number = ((src_y / 8) * bg_tile_size) + (src_x / 8);

				//Look at the Tile Map #(tile_number), see what Tile # it points to
				u16 map_entry = mem->read_u16(map_base + (tile_number << 1));

				//Grab flipping attributes
				flip = (map_entry >> 10) & 0x3;
				src_x = (flip & 0x1) ? inv_lut[src_x % 8] : (src_x % 8);
				src_y = (flip & 0x2) ? inv_lut[src_y % 8] : (src_y % 8);

				//Get address of Tile #(map_entry)
				u32 tile_addr = tile_base + ((map_entry & 0x3FF) * 64);

				u8 current_tile_pixel = (src_y * 8) + src_x;

				//Grab the byte corresponding to (current_tile_pixel), render it as ARGB - 8-bit version
				tile_addr += current_tile_pixel;
				u8 raw_color = mem->memory_map[tile_addr];

				//Only draw if no previous pixel was rendered
				if(!render_buffer_b[scanline_pixel_counter])
				{
					//Only draw BG color if not transparent
					if(raw_color != 0)
					{
						scanline_buffer_b[scanline_pixel_counter] = lcd_stat.bg_pal_b[raw_color];
						render_buffer_b[scanline_pixel_counter] = (bg_id + 1);
					}

					else { full_render = false; }
				}	
			}
		}

		//Update XREF and YREF for next line
		lcd_stat.bg_affine_b[affine_id].x_ref += lcd_stat.bg_affine_b[affine_id].dmx;
		lcd_stat.bg_affine_b[affine_id].y_ref += lcd_stat.bg_affine_b[affine_id].dmy;

		full_scanline_render_b = full_render;
	}
}

/****** Render BG Mode 256-color scanline ******/
void NTR_LCD::render_bg_mode_bitmap(u32 bg_control)
{
	//Render Engine A
	if((bg_control & 0x1000) == 0)
	{
		//Grab BG ID
		u8 bg_id = (bg_control - 0x4000008) >> 1;
		u8 affine_id = (bg_id & 0x1);

		//Reload X-Y references at start of frame
		if(lcd_stat.current_scanline == 0) { reload_affine_references(bg_control); }

		//Abort rendering if this bg is disabled
		if(!lcd_stat.bg_enable_a[bg_id]) { return; }

		//Abort rendering if BGs with high priority have already completely rendered a scanline
		if(full_scanline_render_a) { return; }

		bool full_render = true;

		u8 raw_color = 0;
		u8 scanline_pixel_counter = 0;

		u16 src_x, src_y = 0;
		double new_x, new_y = 0.0;
		u16 bg_pixel_width, bg_pixel_height = 0;

		u32 bitmap_addr = lcd_stat.bg_bitmap_base_addr_a[bg_id & 0x1];

		//Determine bitmap dimensions
		switch(lcd_stat.bg_size_a[bg_id])
		{
			case 0x0:
				bg_pixel_width = 128;
				bg_pixel_height = 128;
				break;

			case 0x1:
				bg_pixel_width = 256;
				bg_pixel_height = 256;
				break;

			case 0x2:
				bg_pixel_width = 512;
				bg_pixel_height = 256;
				break;

			case 0x3:
				bg_pixel_width = 512;
				bg_pixel_height = 512;
				break;
		}

		//Set current texture position at X and Y references
		lcd_stat.bg_affine_a[affine_id].x_pos = lcd_stat.bg_affine_a[affine_id].x_ref;
		lcd_stat.bg_affine_a[affine_id].y_pos = lcd_stat.bg_affine_a[affine_id].y_ref;
		
		for(int x = 0; x < 256; x++)
		{
			bool render_pixel = true;

			new_x = lcd_stat.bg_affine_a[affine_id].x_pos;
			new_y = lcd_stat.bg_affine_a[affine_id].y_pos;

			//Clip BG if coordinates overflow and overflow flag is not set
			if(!lcd_stat.bg_affine_a[affine_id].overflow)
			{
				if((new_x >= bg_pixel_width) || (new_x < 0)) { render_pixel = false; }
				if((new_y >= bg_pixel_height) || (new_y < 0)) { render_pixel = false; }
			}

			//Wrap BG if coordinates overflow and overflow flag is set
			else 
			{
				while(new_x >= bg_pixel_width) { new_x -= bg_pixel_width; }
				while(new_y >= bg_pixel_height) { new_y -= bg_pixel_height; }
				while(new_x < 0) { new_x += bg_pixel_width; }
				while(new_y < 0) { new_y += bg_pixel_height; } 
			}

			//Only draw if no previous pixel was rendered
			if(!render_buffer_a[scanline_pixel_counter])
			{
				if(render_pixel)
				{
					//Determine source pixel X-Y coordinates
					src_x = new_x;
					src_y = new_y;

					raw_color = mem->memory_map[bitmap_addr + (src_y * bg_pixel_width) + src_x];
			
					if(raw_color)
					{
						scanline_buffer_a[scanline_pixel_counter] = lcd_stat.bg_pal_a[raw_color];
						render_buffer_a[scanline_pixel_counter] = (bg_id + 1);
					}

					else { full_render = false; }
				}

				else { full_render = false; }
			}

			scanline_pixel_counter++;

			//Update texture position with DX and DY
			lcd_stat.bg_affine_a[affine_id].x_pos += lcd_stat.bg_affine_a[affine_id].dx;
			lcd_stat.bg_affine_a[affine_id].y_pos += lcd_stat.bg_affine_a[affine_id].dy;
		}

		full_scanline_render_a = full_render;

		//Update XREF and YREF for next line
		lcd_stat.bg_affine_a[affine_id].x_ref += lcd_stat.bg_affine_a[affine_id].dmx;
		lcd_stat.bg_affine_a[affine_id].y_ref += lcd_stat.bg_affine_a[affine_id].dmy;
	}

	//Render Engine B
	else
	{
		//Grab BG ID
		u8 bg_id = (bg_control - 0x4001008) >> 1;
		u8 affine_id = (bg_id & 0x1);

		//Reload X-Y references at start of frame
		if(lcd_stat.current_scanline == 0) { reload_affine_references(bg_control); }

		//Abort rendering if this bg is disabled
		if(!lcd_stat.bg_enable_b[bg_id]) { return; }

		//Abort rendering if BGs with high priority have already completely rendered a scanline
		if(full_scanline_render_b) { return; }

		bool full_render = true;

		u8 raw_color = 0;
		u8 scanline_pixel_counter = 0;

		u16 src_x, src_y = 0;
		double new_x, new_y = 0.0;
		u16 bg_pixel_width, bg_pixel_height = 0;

		u32 bitmap_addr = lcd_stat.bg_bitmap_base_addr_b[bg_id & 0x1];

		//Determine bitmap dimensions
		switch(lcd_stat.bg_size_b[bg_id])
		{
			case 0x0:
				bg_pixel_width = 128;
				bg_pixel_height = 128;
				break;

			case 0x1:
				bg_pixel_width = 256;
				bg_pixel_height = 256;
				break;

			case 0x2:
				bg_pixel_width = 512;
				bg_pixel_height = 256;
				break;

			case 0x3:
				bg_pixel_width = 512;
				bg_pixel_height = 512;
				break;
		}

		//Set current texture position at X and Y references
		lcd_stat.bg_affine_b[affine_id].x_pos = lcd_stat.bg_affine_b[affine_id].x_ref;
		lcd_stat.bg_affine_b[affine_id].y_pos = lcd_stat.bg_affine_b[affine_id].y_ref;
		
		for(int x = 0; x < 256; x++)
		{
			bool render_pixel = true;

			new_x = lcd_stat.bg_affine_b[affine_id].x_pos;
			new_y = lcd_stat.bg_affine_b[affine_id].y_pos;

			//Clip BG if coordinates overflow and overflow flag is not set
			if(!lcd_stat.bg_affine_b[affine_id].overflow)
			{
				if((new_x >= bg_pixel_width) || (new_x < 0)) { render_pixel = false; }
				if((new_y >= bg_pixel_height) || (new_y < 0)) { render_pixel = false; }
			}

			//Wrap BG if coordinates overflow and overflow flag is set
			else 
			{
				while(new_x >= bg_pixel_width) { new_x -= bg_pixel_width; }
				while(new_y >= bg_pixel_height) { new_y -= bg_pixel_height; }
				while(new_x < 0) { new_x += bg_pixel_width; }
				while(new_y < 0) { new_y += bg_pixel_height; } 
			}

			//Only draw if no previous pixel was rendered
			if(!render_buffer_b[scanline_pixel_counter])
			{
				if(render_pixel)
				{
					//Determine source pixel X-Y coordinates
					src_x = new_x;
					src_y = new_y;

					raw_color = mem->memory_map[bitmap_addr + (src_y * bg_pixel_width) + src_x];
			
					if(raw_color)
					{
						scanline_buffer_b[scanline_pixel_counter] = lcd_stat.bg_pal_b[raw_color];
						render_buffer_b[scanline_pixel_counter] = (bg_id + 1);
					}

					else { full_render = false; }
				}

				else { full_render = false; }
			}
				
			scanline_pixel_counter++;

			//Update texture position with DX and DY
			lcd_stat.bg_affine_b[affine_id].x_pos += lcd_stat.bg_affine_b[affine_id].dx;
			lcd_stat.bg_affine_b[affine_id].y_pos += lcd_stat.bg_affine_b[affine_id].dy;
		}

		full_scanline_render_b = full_render;

		//Update XREF and YREF for next line
		lcd_stat.bg_affine_b[affine_id].x_ref += lcd_stat.bg_affine_b[affine_id].dmx;
		lcd_stat.bg_affine_b[affine_id].y_ref += lcd_stat.bg_affine_b[affine_id].dmy;
	}
}

/****** Render BG Mode direct color scanline ******/
void NTR_LCD::render_bg_mode_direct(u32 bg_control)
{
	//Render Engine A
	if((bg_control & 0x1000) == 0)
	{
		//Grab BG ID
		u8 bg_id = (bg_control - 0x4000008) >> 1;
		u8 affine_id = (bg_id & 0x1);

		//Reload X-Y references at start of frame
		if(lcd_stat.current_scanline == 0) { reload_affine_references(bg_control); }

		//Abort rendering if this bg is disabled
		if(!lcd_stat.bg_enable_a[bg_id]) { return; }

		//Abort rendering if BGs with high priority have already completely rendered a scanline
		if(full_scanline_render_a) { return; }

		u16 raw_color = 0;
		u8 scanline_pixel_counter = 0;

		u16 src_x, src_y = 0;
		double new_x, new_y = 0.0;
		u16 bg_pixel_width, bg_pixel_height = 0;

		//Determine bitmap dimensions
		switch(lcd_stat.bg_size_a[bg_id])
		{
			case 0x0:
				bg_pixel_width = 128;
				bg_pixel_height = 128;
				break;

			case 0x1:
				bg_pixel_width = 256;
				bg_pixel_height = 256;
				break;

			case 0x2:
				bg_pixel_width = 512;
				bg_pixel_height = 256;
				break;

			case 0x3:
				bg_pixel_width = 512;
				bg_pixel_height = 512;
				break;
		}

		//Set current texture position at X and Y references
		lcd_stat.bg_affine_a[affine_id].x_pos = lcd_stat.bg_affine_a[affine_id].x_ref;
		lcd_stat.bg_affine_a[affine_id].y_pos = lcd_stat.bg_affine_a[affine_id].y_ref;
		
		for(int x = 0; x < 256; x++)
		{
			bool render_pixel = true;

			new_x = lcd_stat.bg_affine_a[affine_id].x_pos;
			new_y = lcd_stat.bg_affine_a[affine_id].y_pos;

			//Clip BG if coordinates overflow and overflow flag is not set
			if(!lcd_stat.bg_affine_a[affine_id].overflow)
			{
				if((new_x >= bg_pixel_width) || (new_x < 0)) { render_pixel = false; }
				if((new_y >= bg_pixel_height) || (new_y < 0)) { render_pixel = false; }
			}

			//Wrap BG if coordinates overflow and overflow flag is set
			else 
			{
				while(new_x >= bg_pixel_width) { new_x -= bg_pixel_width; }
				while(new_y >= bg_pixel_height) { new_y -= bg_pixel_height; }
				while(new_x < 0) { new_x += bg_pixel_width; }
				while(new_y < 0) { new_y += bg_pixel_height; } 
			}

			if(render_pixel)
			{
				//Determine source pixel X-Y coordinates
				src_x = new_x;
				src_y = new_y;

				raw_color = mem->read_u16(0x6000000 + (((src_y * bg_pixel_width) + src_x) * 2));
			
				//Convert 16-bit ARGB to 32-bit ARGB - Bit 15 is alpha transparency
				if(raw_color & 0x8000)
				{
					u8 red = ((raw_color & 0x1F) << 3);
					raw_color >>= 5;

					u8 green = ((raw_color & 0x1F) << 3);
					raw_color >>= 5;

					u8 blue = ((raw_color & 0x1F) << 3);

					scanline_buffer_a[scanline_pixel_counter] = 0xFF000000 | (red << 16) | (green << 8) | (blue);
				}
			}

			scanline_pixel_counter++;

			//Update texture position with DX and DY
			lcd_stat.bg_affine_a[affine_id].x_pos += lcd_stat.bg_affine_a[affine_id].dx;
			lcd_stat.bg_affine_a[affine_id].y_pos += lcd_stat.bg_affine_a[affine_id].dy;
		}

		//Update XREF and YREF for next line
		lcd_stat.bg_affine_a[affine_id].x_ref += lcd_stat.bg_affine_a[affine_id].dmx;
		lcd_stat.bg_affine_a[affine_id].y_ref += lcd_stat.bg_affine_a[affine_id].dmy;
	}

	//Render Engine B
	else
	{
		//Grab BG ID
		u8 bg_id = (bg_control - 0x4001008) >> 1;
		u8 affine_id = (bg_id & 0x1);

		//Reload X-Y references at start of frame
		if(lcd_stat.current_scanline == 0) { reload_affine_references(bg_control); }

		//Abort rendering if this bg is disabled
		if(!lcd_stat.bg_enable_b[bg_id]) { return; }

		//Abort rendering if BGs with high priority have already completely rendered a scanline
		if(full_scanline_render_b) { return; }

		u16 raw_color = 0;
		u8 scanline_pixel_counter = 0;

		u16 src_x, src_y = 0;
		double new_x, new_y = 0.0;
		u16 bg_pixel_width, bg_pixel_height = 0;

		//Determine bitmap dimensions
		switch(lcd_stat.bg_size_b[bg_id])
		{
			case 0x0:
				bg_pixel_width = 128;
				bg_pixel_height = 128;
				break;

			case 0x1:
				bg_pixel_width = 256;
				bg_pixel_height = 256;
				break;

			case 0x2:
				bg_pixel_width = 512;
				bg_pixel_height = 256;
				break;

			case 0x3:
				bg_pixel_width = 512;
				bg_pixel_height = 512;
				break;
		}

		//Set current texture position at X and Y references
		lcd_stat.bg_affine_b[affine_id].x_pos = lcd_stat.bg_affine_b[affine_id].x_ref;
		lcd_stat.bg_affine_b[affine_id].y_pos = lcd_stat.bg_affine_b[affine_id].y_ref;
		
		for(int x = 0; x < 256; x++)
		{
			bool render_pixel = true;

			new_x = lcd_stat.bg_affine_b[affine_id].x_pos;
			new_y = lcd_stat.bg_affine_b[affine_id].y_pos;

			//Clip BG if coordinates overflow and overflow flag is not set
			if(!lcd_stat.bg_affine_b[affine_id].overflow)
			{
				if((new_x >= bg_pixel_width) || (new_x < 0)) { render_pixel = false; }
				if((new_y >= bg_pixel_height) || (new_y < 0)) { render_pixel = false; }
			}

			//Wrap BG if coordinates overflow and overflow flag is set
			else 
			{
				while(new_x >= bg_pixel_width) { new_x -= bg_pixel_width; }
				while(new_y >= bg_pixel_height) { new_y -= bg_pixel_height; }
				while(new_x < 0) { new_x += bg_pixel_width; }
				while(new_y < 0) { new_y += bg_pixel_height; } 
			}

			if(render_pixel)
			{
				//Determine source pixel X-Y coordinates
				src_x = new_x;
				src_y = new_y;

				raw_color = mem->read_u16(0x6200000 + (((src_y * bg_pixel_width) + src_x) * 2));
			
				//Convert 16-bit ARGB to 32-bit ARGB - Bit 15 is alpha transparency
				if(raw_color & 0x8000)
				{
					u8 red = ((raw_color & 0x1F) << 3);
					raw_color >>= 5;

					u8 green = ((raw_color & 0x1F) << 3);
					raw_color >>= 5;

					u8 blue = ((raw_color & 0x1F) << 3);

					scanline_buffer_b[scanline_pixel_counter] = 0xFF000000 | (red << 16) | (green << 8) | (blue);
				}
			}

			scanline_pixel_counter++;

			//Update texture position with DX and DY
			lcd_stat.bg_affine_b[affine_id].x_pos += lcd_stat.bg_affine_b[affine_id].dx;
			lcd_stat.bg_affine_b[affine_id].y_pos += lcd_stat.bg_affine_b[affine_id].dy;
		}

		//Update XREF and YREF for next line
		lcd_stat.bg_affine_b[affine_id].x_ref += lcd_stat.bg_affine_b[affine_id].dmx;
		lcd_stat.bg_affine_b[affine_id].y_ref += lcd_stat.bg_affine_b[affine_id].dmy;
	}
}

/****** Render pixels for a given scanline (per-pixel) ******/
void NTR_LCD::render_scanline()
{
	//Engine A - Render based on display modes
	switch(lcd_stat.display_mode_a)
	{
		//Display Mode 0 - Blank screen
		case 0x0:

		//Forced Blank
		case 0x80:
		case 0x81:
		case 0x82:
			for(u16 x = 0; x < 256; x++) { scanline_buffer_a[x] = 0xFFFFFFFF; }
			break;

		//Display Mode 1 - Tiled BG and OBJ
		case 0x1:
			render_bg_scanline(NDS_DISPCNT_A);
			break;

		//Display Mode 2 - VRAM
		case 0x2:
			{
				u8 vram_block = ((lcd_stat.display_control_a >> 18) & 0x3);
				u32 vram_addr = lcd_stat.vram_bank_addr[vram_block] + (lcd_stat.current_scanline * 256 * 2);

				for(u16 x = 0; x < 256; x++)
				{
					u16 color_bytes = mem->read_u16_fast(vram_addr + (x << 1));

					u8 red = ((color_bytes & 0x1F) << 3);
					color_bytes >>= 5;

					u8 green = ((color_bytes & 0x1F) << 3);
					color_bytes >>= 5;

					u8 blue = ((color_bytes & 0x1F) << 3);

					scanline_buffer_a[x] = 0xFF000000 | (red << 16) | (green << 8) | (blue);
				}
			}

			break;

		//Display Mode 3 - Main Memory
		default:
			std::cout<<"LCD::Warning - Engine A - Unsupported Display Mode 3 \n";
			break;
	}

	//Engine B - Render based on display modes
	switch(lcd_stat.display_mode_b)
	{
		//Display Mode 0 - Blank screen
		case 0x0:

		//Forced Blank
		case 0x80:
		case 0x81:
			for(u16 x = 0; x < 256; x++) { scanline_buffer_b[x] = 0xFFFFFFFF; }
			break;

		//Display Mode 1 - Tiled BG and OBJ
		case 0x1:
			render_bg_scanline(NDS_DISPCNT_B);
			break;

		//Modes 2 and 3 unsupported by Engine B
		default:
			std::cout<<"LCD::Warning - Engine B - Unsupported Display Mode " << std::dec << (int)lcd_stat.display_mode_b << "\n";
			break;
	}		
}

/****** Immediately draw current buffer to the screen ******/
void NTR_LCD::update()
{
	//Use SDL
	if(config::sdl_render)
	{
		//Lock source surface
		if(SDL_MUSTLOCK(final_screen)){ SDL_LockSurface(final_screen); }
		u32* out_pixel_data = (u32*)final_screen->pixels;

		for(int a = 0; a < 0x18000; a++) { out_pixel_data[a] = screen_buffer[a]; }

		//Unlock source surface
		if(SDL_MUSTLOCK(final_screen)){ SDL_UnlockSurface(final_screen); }
		
		//Display final screen buffer - OpenGL
		if(config::use_opengl) { opengl_blit(); }

		//Display final screen buffer - SDL
		else
		{
			if(SDL_UpdateWindowSurface(window) != 0) { std::cout<<"LCD::Error - Could not blit\n"; }
		}
	}

	//Use external rendering method (GUI)
	else
	{
		if(!config::use_opengl) { config::render_external_sw(screen_buffer); }

		else
		{
			//Lock source surface
			if(SDL_MUSTLOCK(final_screen)){ SDL_LockSurface(final_screen); }
			u32* out_pixel_data = (u32*)final_screen->pixels;

			for(int a = 0; a < 0x18000; a++) { out_pixel_data[a] = screen_buffer[a]; }

			//Unlock source surface
			if(SDL_MUSTLOCK(final_screen)){ SDL_UnlockSurface(final_screen); }

			config::render_external_hw(final_screen);
		}
	}
}


/****** Run LCD for one cycle ******/
void NTR_LCD::step()
{
	lcd_stat.lcd_clock++;

	//Mode 0 - Scanline rendering
	if(((lcd_stat.lcd_clock % 2130) <= 1536) && (lcd_stat.lcd_clock < 408960)) 
	{
		//Change mode
		if(lcd_stat.lcd_mode != 0) 
		{
			lcd_stat.lcd_mode = 0;

			//Reset HBlank flag in DISPSTAT
			lcd_stat.display_stat_a &= ~0x2;
			lcd_stat.display_stat_b &= ~0x2;
			
			lcd_stat.current_scanline++;

			//Update VCOUNT
			mem->write_u16_fast(NDS_VCOUNT, lcd_stat.current_scanline);

			scanline_compare();
		}
	}

	//Mode 1 - H-Blank
	else if(((lcd_stat.lcd_clock % 2130) > 1536) && (lcd_stat.lcd_clock < 408960))
	{
		//Change mode
		if(lcd_stat.lcd_mode != 1) 
		{
			lcd_stat.lcd_mode = 1;

			//Set HBlank flag in DISPSTAT
			lcd_stat.display_stat_a |= 0x2;
			lcd_stat.display_stat_b |= 0x2;

			//Trigger HBlank IRQ
			if(lcd_stat.hblank_irq_enable_a) { mem->nds9_if |= 0x2; }
			if(lcd_stat.hblank_irq_enable_b) { mem->nds7_if |= 0x2; }

			//Update 2D engine OAM
			if(lcd_stat.oam_update) { update_oam(); }

			//Update OBJ render list
			update_obj_render_list();

			//Update 2D engine palettes
			if(lcd_stat.bg_pal_update_a || lcd_stat.bg_pal_update_b || lcd_stat.bg_ext_pal_update_a || lcd_stat.bg_ext_pal_update_b
			|| lcd_stat.obj_pal_update_a || lcd_stat.obj_pal_update_b || lcd_stat.obj_ext_pal_update_a || lcd_stat.obj_ext_pal_update_b) { update_palettes(); }

			//Update BG control registers - Engine A
			if(lcd_stat.update_bg_control_a)
			{
				mem->write_u8(NDS_BG0CNT_A, mem->memory_map[NDS_BG0CNT_A]);
				mem->write_u8(NDS_BG1CNT_A, mem->memory_map[NDS_BG1CNT_A]);
				mem->write_u8(NDS_BG2CNT_A, mem->memory_map[NDS_BG2CNT_A]);
				mem->write_u8(NDS_BG3CNT_A, mem->memory_map[NDS_BG3CNT_A]);
				lcd_stat.update_bg_control_a = false;
			}

			//Update BG control registers - Engine B
			if(lcd_stat.update_bg_control_b)
			{
				mem->write_u8(NDS_BG0CNT_B, mem->memory_map[NDS_BG0CNT_B]);
				mem->write_u8(NDS_BG1CNT_B, mem->memory_map[NDS_BG1CNT_B]);
				mem->write_u8(NDS_BG2CNT_B, mem->memory_map[NDS_BG2CNT_B]);
				mem->write_u8(NDS_BG3CNT_B, mem->memory_map[NDS_BG3CNT_B]);
				lcd_stat.update_bg_control_b = false;
			}

			//Render scanline data
			render_scanline();

			u32 render_position = (lcd_stat.current_scanline * 256);

			//Swap top and bottom if POWERCNT1 Bit 15 is not set, otherwise A is top, B is bottom
			u16 disp_a_offset = (mem->power_cnt1 & 0x8000) ? 0 : 0xC000;
			u16 disp_b_offset = (mem->power_cnt1 & 0x8000) ? 0xC000 : 0;

			//Swap top and bottom if LCD configuration calls for it
			if(config::lcd_config & 0x1)
			{
				disp_a_offset = (disp_a_offset) ? 0 : 0xC000;
				disp_b_offset = (disp_b_offset) ? 0 : 0xC000;
			}
			
			//Push scanline pixel data to screen buffer
			for(u16 x = 0; x < 256; x++)
			{
				screen_buffer[render_position + x + disp_a_offset] = scanline_buffer_a[x];
				screen_buffer[render_position + x + disp_b_offset] = scanline_buffer_b[x];
			}
		}
	}

	//Mode 2 - VBlank
	else
	{
		//Change mode
		if(lcd_stat.lcd_mode != 2) 
		{
			lcd_stat.lcd_mode = 2;

			//Set VBlank flag in DISPSTAT
			lcd_stat.display_stat_a |= 0x1;
			lcd_stat.display_stat_b |= 0x1;

			//Reset HBlank flag in DISPSTAT
			lcd_stat.display_stat_a &= ~0x2;
			lcd_stat.display_stat_b &= ~0x2;

			//Trigger VBlank IRQ
			if(lcd_stat.vblank_irq_enable_a) { mem->nds9_if |= 0x1; }
			if(lcd_stat.vblank_irq_enable_b) { mem->nds7_if |= 0x1; }

			//Increment scanline count
			lcd_stat.current_scanline++;
			scanline_compare();

			//Update VCOUNT
			mem->write_u16_fast(NDS_VCOUNT, lcd_stat.current_scanline);

			//Use SDL
			if(config::sdl_render)
			{
				//Lock source surface
				if(SDL_MUSTLOCK(final_screen)){ SDL_LockSurface(final_screen); }
				u32* out_pixel_data = (u32*)final_screen->pixels;

				for(int a = 0; a < 0x18000; a++) { out_pixel_data[a] = screen_buffer[a]; }

				//Unlock source surface
				if(SDL_MUSTLOCK(final_screen)){ SDL_UnlockSurface(final_screen); }
				
				//Display final screen buffer - OpenGL
				if(config::use_opengl) { opengl_blit(); }
				
				//Display final screen buffer - SDL
				else 
				{
					if(SDL_UpdateWindowSurface(window) != 0) { std::cout<<"LCD::Error - Could not blit\n"; }
				}
			}

			//Use external rendering method (GUI)
			else
			{
				if(!config::use_opengl) { config::render_external_sw(screen_buffer); }

				else
				{
					//Lock source surface
					if(SDL_MUSTLOCK(final_screen)){ SDL_LockSurface(final_screen); }
					u32* out_pixel_data = (u32*)final_screen->pixels;

					for(int a = 0; a < 0x18000; a++) { out_pixel_data[a] = screen_buffer[a]; }

					//Unlock source surface
					if(SDL_MUSTLOCK(final_screen)){ SDL_UnlockSurface(final_screen); }

					config::render_external_hw(final_screen);
				}
			}

			//Limit framerate
			if(!config::turbo)
			{
				frame_current_time = SDL_GetTicks();
				if((frame_current_time - frame_start_time) < 16) { SDL_Delay(16 - (frame_current_time - frame_start_time));}
				frame_start_time = SDL_GetTicks();
			}

			//Update FPS counter + title
			fps_count++;
			if(((SDL_GetTicks() - fps_time) >= 1000) && (config::sdl_render))
			{ 
				fps_time = SDL_GetTicks(); 
				config::title.str("");
				config::title << "GBE+ " << fps_count << "FPS";
				SDL_SetWindowTitle(window, config::title.str().c_str());
				fps_count = 0; 
			}
		}

		//Increment scanline after HBlank starts
		else if((lcd_stat.lcd_clock % 2130) == 1536)
		{
			lcd_stat.current_scanline++;
			scanline_compare();

			//Update VCOUNT
			mem->write_u16_fast(NDS_VCOUNT, lcd_stat.current_scanline);

			//Reset VBlank flag in DISPSTAT on line 261
			if(lcd_stat.current_scanline == 261)
			{
				lcd_stat.display_stat_a &= ~0x1;
				lcd_stat.display_stat_b &= ~0x1;
			}

			//Set HBlank flag in DISPSTAT
			lcd_stat.display_stat_a |= 0x2;
			lcd_stat.display_stat_b |= 0x2;

			//Trigger HBlank IRQ
			if(lcd_stat.hblank_irq_enable_a) { mem->nds9_if |= 0x2; }
			if(lcd_stat.hblank_irq_enable_b) { mem->nds7_if |= 0x2; }
		}

		//Turn off HBlank flag at the start of a scanline
		else if((lcd_stat.lcd_clock % 2130) == 0)
		{
			//Reset HBlank flag in DISPSTAT
			lcd_stat.display_stat_a &= ~0x2;
			lcd_stat.display_stat_b &= ~0x2;

			//Reset LCD clock
			if(lcd_stat.current_scanline == 263)
			{
				lcd_stat.lcd_clock -= 560190;
				lcd_stat.current_scanline = 0xFFFF;
			}
		}
	}
}

/****** Compare VCOUNT to LYC ******/
void NTR_LCD::scanline_compare()
{
	//Raise VCOUNT interrupt - Engine A
	if(lcd_stat.current_scanline == lcd_stat.lyc_a)
	{
		//Check to see if the VCOUNT IRQ is enabled in DISPSTAT
		if(lcd_stat.vcount_irq_enable_a) { mem->nds9_if |= 0x4; }

		//Toggle VCOUNT flag ON
		lcd_stat.display_stat_a |= 0x4;
	}

	else
	{
		//Toggle VCOUNT flag OFF
		lcd_stat.display_stat_a &= ~0x4;
	}

	//Raise VCOUNT interrupt - Engine B
	if(lcd_stat.current_scanline == lcd_stat.lyc_b)
	{
		//Check to see if the VCOUNT IRQ is enabled in DISPSTAT
		if(lcd_stat.vcount_irq_enable_b) { mem->nds7_if |= 0x4; }

		//Toggle VCOUNT flag ON
		lcd_stat.display_stat_b |= 0x4;
	}

	else
	{
		//Toggle VCOUNT flag OFF
		lcd_stat.display_stat_b &= ~0x4;
	}
}

/****** Grabs the most current X-Y references for affine backgrounds ******/
void NTR_LCD::reload_affine_references(u32 bg_control)
{
	u32 x_raw, y_raw;
	bool engine_a;
	u8 aff_id;

	//Read X-Y references from memory, determine which engine it belongs to
	switch(bg_control)
	{
		case NDS_BG2CNT_A:
			x_raw = mem->read_u32_fast(NDS_BG2X_A);
			y_raw = mem->read_u32_fast(NDS_BG2Y_A);
			engine_a = true;
			aff_id = 0;
			break;

		case NDS_BG3CNT_A:
			x_raw = mem->read_u32_fast(NDS_BG3X_A);
			y_raw = mem->read_u32_fast(NDS_BG3Y_A);
			engine_a = true;
			aff_id = 1;
			break;

		case NDS_BG2CNT_B:
			x_raw = mem->read_u32_fast(NDS_BG2X_B);
			y_raw = mem->read_u32_fast(NDS_BG2Y_B);
			engine_a = false;
			aff_id = 0;
			break;

		case NDS_BG3CNT_B:
			x_raw = mem->read_u32_fast(NDS_BG3X_B);
			y_raw = mem->read_u32_fast(NDS_BG3Y_B);
			aff_id = 1;
			break;

		default: return;
	}
			
	//Get X reference point
	if(x_raw & 0x8000000) 
	{ 
		u32 x = ((x_raw >> 8) - 1);
		x = (~x & 0x7FFFF);
		
		if(engine_a) { lcd_stat.bg_affine_a[aff_id].x_ref = -1.0 * x; }
		else { lcd_stat.bg_affine_b[aff_id].x_ref = -1.0 * x; }
	}
	
	else
	{
		if(engine_a) { lcd_stat.bg_affine_a[aff_id].x_ref = (x_raw >> 8) & 0x7FFFF; }
		else { lcd_stat.bg_affine_b[aff_id].x_ref = (x_raw >> 8) & 0x7FFFF; }
	}
	
	if((x_raw & 0xFF) != 0)
	{
		if(engine_a) { lcd_stat.bg_affine_a[aff_id].x_ref += (x_raw & 0xFF) / 256.0; }
		else { lcd_stat.bg_affine_b[aff_id].x_ref += (x_raw & 0xFF) / 256.0; }
	}

	//Set current X position as the new reference point
	if(engine_a) { lcd_stat.bg_affine_a[aff_id].x_pos = lcd_stat.bg_affine_a[aff_id].x_ref; }
	else { lcd_stat.bg_affine_b[aff_id].x_pos = lcd_stat.bg_affine_b[aff_id].x_ref; }

	//Get Y reference point
	if(y_raw & 0x8000000) 
	{ 
		u32 y = ((y_raw >> 8) - 1);
		y = (~y & 0x7FFFF);
		
		if(engine_a) { lcd_stat.bg_affine_a[aff_id].y_ref = -1.0 * y; }
		else { lcd_stat.bg_affine_b[aff_id].y_ref = -1.0 * y; }
	}
	
	else
	{
		if(engine_a) { lcd_stat.bg_affine_a[aff_id].y_ref = (y_raw >> 8) & 0x7FFFF; }
		else { lcd_stat.bg_affine_b[aff_id].y_ref = (y_raw >> 8) & 0x7FFFF; }
	}
	
	if((y_raw & 0xFF) != 0)
	{
		if(engine_a) { lcd_stat.bg_affine_a[aff_id].y_ref += (y_raw & 0xFF) / 256.0; }
		else { lcd_stat.bg_affine_b[aff_id].y_ref += (y_raw & 0xFF) / 256.0; }
	}

	//Set current Y position as the new reference point
	if(engine_a) { lcd_stat.bg_affine_a[aff_id].y_pos = lcd_stat.bg_affine_a[aff_id].y_ref; }
	else { lcd_stat.bg_affine_b[aff_id].y_pos = lcd_stat.bg_affine_b[aff_id].y_ref; }
}

/****** Generates the game icon (non-animated) from NDS cart header ******/
bool NTR_LCD::get_cart_icon(SDL_Surface* nds_icon)
{
	u32 icon_base = mem->read_cart_u32(0x68);
	
	if(!icon_base) { return false; }

	//Create SDL_Surface for icon
	nds_icon = SDL_CreateRGBSurface(SDL_SWSURFACE, 32, 32, 32, 0, 0, 0, 0);

	//Lock source surface
	if(SDL_MUSTLOCK(nds_icon)){ SDL_LockSurface(nds_icon); }
	u32* icon_data = (u32*)nds_icon->pixels;

	//Generate palettes
	u32 pals[16];

	for(u32 x = 0; x < 16; x++)
	{
		u16 raw_pal = mem->read_cart_u16(icon_base + 0x220 + (x << 1));
		
		u8 red = ((raw_pal & 0x1F) << 3);
		raw_pal >>= 5;

		u8 green = ((raw_pal & 0x1F) << 3);
		raw_pal >>= 5;

		u8 blue = ((raw_pal & 0x1F) << 3);

		pals[x] =  0xFF000000 | (red << 16) | (green << 8) | (blue);
	}

	u16 data_pos = 0;

	//Cycle through all tiles to generate icon data
	for(u32 x = 0; x < 16; x++)
	{
		u16 pixel_pos = ((x / 4) * 256) + ((x % 4) * 8);

		for(u32 y = 0; y < 32; y++)
		{
			u8 icon_char = mem->cart_data[icon_base + 0x20 + data_pos];
			u8 char_r = (icon_char >> 4);
			u8 char_l = (icon_char & 0xF);
			data_pos++;

			icon_data[pixel_pos++] = pals[char_l];
			icon_data[pixel_pos++] = pals[char_r];

			if((pixel_pos % 8) == 0) { pixel_pos += 24; }
		}
	}

	//Unlock source surface
	if(SDL_MUSTLOCK(nds_icon)){ SDL_UnlockSurface(nds_icon); }

	SDL_SaveBMP(nds_icon, "icon.bmp");

	return true;
}
