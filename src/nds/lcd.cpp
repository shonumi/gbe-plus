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

	scanline_buffer_a.clear();
	scanline_buffer_b.clear();
	screen_buffer.clear();

	lcd_clock = 0;
	lcd_mode = 0;

	frame_start_time = 0;
	frame_current_time = 0;
	fps_count = 0;
	fps_time = 0;

	lcd_stat.current_scanline = 0;
	lcd_stat.lyc = 0;
	scanline_pixel_counter = 0;

	screen_buffer.resize(0x18000, 0);
	scanline_buffer_a.resize(0x100, 0);
	scanline_buffer_b.resize(0x100, 0);

	lcd_stat.bg_pal_update_a = true;
	lcd_stat.bg_pal_update_list_a.resize(0x100, 0);

	lcd_stat.bg_pal_update_b = true;
	lcd_stat.bg_pal_update_list_b.resize(0x100, 0);

	lcd_stat.bg_mode_a = 0;
	lcd_stat.bg_mode_b = 0;
	lcd_stat.hblank_interval_free = false;

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

		lcd_stat.bg_base_tile_addr_a[x] = 0x6000000;
		lcd_stat.bg_base_tile_addr_b[x] = 0x6000000;

		lcd_stat.bg_base_map_addr_a[x] = 0x6000000;
		lcd_stat.bg_base_map_addr_b[x] = 0x6000000;

		lcd_stat.bg_enable_a[x] = false;
		lcd_stat.bg_enable_b[x] = false;
	}

	for(int x = 0; x < 9; x++) { lcd_stat.vram_bank_addr[x] = 0x0; }

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
		//if(config::use_opengl) { opengl_init(); }

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
	}

	std::cout<<"LCD::Initialized\n";

	return true;
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
				case 0:
					return render_bg_mode_0(bg_control); break;

				default:
					std::cout<<"LCD::Engine A - invalid or unsupported BG Mode : " << std::dec << lcd_stat.bg_mode_a;
			}
		}
	}

	//Render Engine B
	else
	{
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
				case 0:
					return render_bg_mode_0(bg_control); break;

				default:
					std::cout<<"LCD::Engine B - invalid or unsupported BG Mode : " << std::dec << lcd_stat.bg_mode_b;
			}
		}
	}
}

/****** Render BG Mode 0 scanline ******/
void NTR_LCD::render_bg_mode_0(u32 bg_control)
{
	//Render Engine A
	if((bg_control & 0x1000) == 0)
	{

	}

	//Render Engine B
	else
	{
		//Grab BG ID
		u8 bg_id = (bg_control - 0x4001008) >> 1;

		//Abort rendering if this bg is disabled
		if(!lcd_stat.bg_enable_b[bg_id]) { return; }

		u16 tile_id;
		u8 pal_id;

		u8 scanline_pixel_counter = 0;
		u8 current_tile_line = (lcd_stat.current_scanline % 8);

		//Grab BG bit-depth and offset for the current tile line
		u8 bit_depth = lcd_stat.bg_depth_b[bg_id] ? 64 : 32;
		u8 line_offset = (bit_depth >> 3) * current_tile_line;

		//Get VRAM bank + tile and map addresses
		u8 bank_id = (lcd_stat.bg_control_b[bg_id] >> 18) & 0x3;
		u32 base_addr = lcd_stat.vram_bank_addr[bank_id] + 0x200000;

		u32 tile_addr = base_addr + lcd_stat.bg_base_tile_addr_b[bg_id];
		u32 map_addr = base_addr + lcd_stat.bg_base_map_addr_b[bg_id];

		//Cycle through all tiles on this scanline
		for(u32 x = 0; x < 32; x++)
		{
			//Determine which map entry to start looking up tiles
			u16 map_entry = ((lcd_stat.current_scanline >> 3) << 5);
			map_entry += (scanline_pixel_counter >> 3);

			//Pull map data from current map entry
			u16 map_data = mem->read_u16(map_addr + (map_entry << 1));

			//Get tile and palette number
			tile_id = (map_data & 0x1FF);
			pal_id = (map_data >> 12) & 0xF;

			//Calculate VRAM address to start pulling up tile data
			u32 tile_data_addr = tile_addr + (tile_id * bit_depth) + line_offset;

			//Read 8 pixels from VRAM and put them in the scanline buffer
			for(u32 y = 0; y < 8; y++)
			{
				//Process 8-bit depth
				if(bit_depth == 64)
				{
					u8 raw_color = mem->read_u8(tile_data_addr++);
					scanline_buffer_b[scanline_pixel_counter++] = lcd_stat.bg_pal_b[raw_color];
				}

				//Process 4-bit depth
				else
				{
					u8 raw_color = mem->read_u8(tile_data_addr++);
					u8 pal_1 = (pal_id * 16) + (raw_color & 0xF);
					u8 pal_2 = (pal_id * 16) + (raw_color >> 4);

					scanline_buffer_b[scanline_pixel_counter++] = lcd_stat.bg_pal_b[pal_1];
					scanline_buffer_b[scanline_pixel_counter++] = lcd_stat.bg_pal_b[pal_2];

					y++;
				}
			}
		}		
	}
}

/****** Render BG Mode 1 scanline ******/
void NTR_LCD::render_bg_mode_1(u32 bg_control) { }

/****** Render BG Mode 3 scanline ******/
void NTR_LCD::render_bg_mode_3(u32 bg_control) { }

/****** Render BG Mode 4 scanline ******/
void NTR_LCD::render_bg_mode_4(u32 bg_control) { }

/****** Render BG Mode 5 scanline ******/
void NTR_LCD::render_bg_mode_5(u32 bg_control) { }

/****** Render BG Mode 6 ******/
void NTR_LCD::render_bg_mode_6(u32 bg_control) { }

/****** Render pixels for a given scanline (per-pixel) ******/
void NTR_LCD::render_scanline()
{
	//Engine A - Render based on display modes
	switch(lcd_stat.display_mode_a)
	{
		//Display Mode 0 - Blank screen
		case 0x0:
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
			for(u16 x = 0; x < 256; x++) { scanline_buffer_b[x] = 0xFFFFFFFF; }
			break;

		//Display Mode 1 - Tiled BG and OBJ
		case 0x1:
			render_bg_scanline(NDS_DISPCNT_B);
			break;

		//Modes 2 and 3 unsupported by Engine B
		default:
			std::cout<<"LCD::Warning - Engine B - Unsupported Display Mode " << std::dec << lcd_stat.display_mode_b << "\n";
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
		
		//Display final screen buffer - SDL
		if(SDL_UpdateWindowSurface(window) != 0) { std::cout<<"LCD::Error - Could not blit\n"; }
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
	//TODO - Not sure if disabling the display acts like the DMG/GBC where everything shuts down (nothing is clocked, no IRQs)
	//For now, just going to assume that is the case.
	//TODO - Test this on real HW.
	//if((lcd_stat.display_mode_a == 0) && (lcd_stat.display_mode_b == 0)) { return; }

	lcd_clock++;

	//Mode 0 - Scanline rendering
	if(((lcd_clock % 2130) <= 1536) && (lcd_clock < 408960)) 
	{
		//Change mode
		if(lcd_mode != 0) 
		{
			lcd_mode = 0;

			//Reset VBlank flag in DISPSTAT
			mem->memory_map[NDS_DISPSTAT] &= ~0x1;
			
			lcd_stat.current_scanline++;
			if(lcd_stat.current_scanline == 263) { lcd_stat.current_scanline = 0; }

			//Update VCOUNT
			mem->write_u16_fast(NDS_VCOUNT, lcd_stat.current_scanline);

			scanline_compare();
		}
	}

	//Mode 1 - H-Blank
	else if(((lcd_clock % 2130) > 1536) && (lcd_clock < 408960))
	{
		//Change mode
		if(lcd_mode != 1) 
		{
			//Update 2D engine palettes
			if((lcd_stat.bg_pal_update_a || lcd_stat.bg_pal_update_b)) { update_palettes(); }

			//Render scanline data
			render_scanline();

			u32 render_position = (lcd_stat.current_scanline * 256);

			//Push scanline pixel data to screen buffer
			for(u16 x = 0; x < 256; x++)
			{
				screen_buffer[render_position + x] = scanline_buffer_a[x];
				screen_buffer[render_position + x + 0xC000] = scanline_buffer_b[x];
			}

			lcd_mode = 1;
		}
	}

	//Mode 2 - VBlank
	else
	{
		//Change mode
		if(lcd_mode != 2) 
		{
			lcd_mode = 2;

			//Set VBlank flag in DISPSTAT
			mem->memory_map[NDS_DISPSTAT] |= 0x1;

			//Trigger VBlank IRQ
			if(lcd_stat.vblank_irq_enable)
			{
				mem->nds9_if |= 0x1;
				mem->nds7_if |= 0x1;
			}

			//Increment scanline count
			lcd_stat.current_scanline++;

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
				
				if(SDL_UpdateWindowSurface(window) != 0) { std::cout<<"LCD::Error - Could not blit\n"; }
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

		//Reset LCD clock
		else if(lcd_clock >= 558060) 
		{
			lcd_clock -= 558060;
			lcd_stat.current_scanline = 0xFFFF;
		}

		//Increment Scanline after HBlank
		else if((lcd_clock % 2130) == 1536)
		{
			lcd_stat.current_scanline++;
			scanline_compare();

			//Update VCOUNT
			mem->write_u16_fast(NDS_VCOUNT, lcd_stat.current_scanline);
		}
	}
}

/****** Compare VCOUNT to LYC ******/
void NTR_LCD::scanline_compare()
{
	//Raise VCOUNT interrupt
	if(lcd_stat.current_scanline == lcd_stat.lyc)
	{
		//Check to see if the VCOUNT IRQ is enabled in DISPSTAT
		if(lcd_stat.vcount_irq_enable) 
		{
			mem->nds9_if |= 0x4;
			mem->nds7_if |= 0x4;
		}

		//Toggle VCOUNT flag ON
		mem->memory_map[NDS_DISPSTAT] |= 0x4;
	}

	else
	{
		//Toggle VCOUNT flag OFF
		mem->memory_map[NDS_DISPSTAT] &= ~0x4;
	}
}