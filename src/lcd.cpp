// GB Enhanced Copyright Daniel Baxter 2013
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd.cpp
// Date : August 16, 2014
// Description : Game Boy Advance LCD emulation
//
// Draws background, window, and sprites to screen
// Responsible for blitting pixel data and limiting frame rate

#include "lcd.h"

/****** LCD Constructor ******/
LCD::LCD()
{
	reset();
}

/****** LCD Destructor ******/
LCD::~LCD()
{
	screen_buffer.clear();
	scanline_buffer.clear();
	std::cout<<"LCD::Shutdown\n";
}

/****** Reset LCD ******/
void LCD::reset()
{
	final_screen = internal_screen = NULL;

	scanline_buffer.clear();
	screen_buffer.clear();

	lcd_clock = 0;
	lcd_mode = 0;

	frame_start_time = 0;
	frame_current_time = 0;
	fps_count = 0;
	fps_time = 0;

	current_scanline = 0;
	scanline_pixel_counter = 0;

	screen_buffer.resize(0x9600, 0);
	scanline_buffer.resize(0x100, 0);
}

/****** Initialize LCD with SDL ******/
bool LCD::init()
{
	if(SDL_Init(SDL_INIT_EVERYTHING) == -1)
	{
		std::cout<<"LCD::Error - Could not initialize SDL\n";
		return false;
	}

	final_screen = SDL_SetVideoMode(240, 160, 32, SDL_SWSURFACE);
	internal_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, 240, 160, 32, 0, 0, 0, 0);

	if(final_screen == NULL) { return false; }

	std::cout<<"LCD::Initialized\n";

	return true;
}

/****** Updates all OAM when values in memory change ******/
void LCD::update_oam()
{
	mem->lcd_updates.oam_update = false;
	
	u32 oam_ptr = 0x7000000;
	u16 attribute = 0;

	for(int x = 0; x < 128; x++)
	{
		//Read and parse Attribute 0
		attribute = mem->read_u16(oam_ptr);
		oam_ptr += 2;

		obj[x].y = (attribute & 0xFF);
		obj[x].bit_depth = (attribute & 0x2000) ? 8 : 4;
		obj[x].shape = (attribute >> 14);

		//Read and parse Attribute 1
		attribute = mem->read_u16(oam_ptr);
		oam_ptr += 2;

		obj[x].x = (attribute & 0x1FF);
		obj[x].h_flip = (attribute & 0x1000) ? true : false;
		obj[x].v_flip = (attribute & 0x2000) ? true : false;
		obj[x].size = (attribute >> 14);

		//Read and parse Attribute 2
		attribute = mem->read_u16(oam_ptr);
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
	}
}

/****** Determines if a sprite pixel should be rendered, and if so draws it to the current scanline pixel ******/
bool LCD::render_sprite_pixel()
{
	//TODO - Account for BG priorities
	//TODO - Horizontal + Vertical flipping

	//If sprites are disabled, quit now
	if((mem->read_u16(DISPCNT) & 0x1000) == 0) { return false; }

	bool render_sprite = false;
	u8 sprite_id = 0;
	u32 sprite_tile_addr = 0;
	u32 meta_sprite_tile = 0;
	obj_render_length = 0;
	u8 raw_color = 0;

	//Cycle through all of the sprites
	for(int x = 0; x < 128; x++)
	{
		//Check to see if sprite is rendered on the current scanline
		if((current_scanline >= obj[x].y) && (current_scanline <= (obj[x].y + obj[x].height - 1)))
		{
			//Check to see if current_scanline_pixel is within sprite
			if((scanline_pixel_counter >= obj[x].x) && (scanline_pixel_counter <= (obj[x].x + obj[x].width - 1)))
			{
				obj_render_list[obj_render_length++] = x;
				render_sprite = true;
			}
		}
	}

	//If no sprites are rendered for this pixel, exit now, draw BG pixel instead
	if(!render_sprite) { return false; }

	//Cycle through all sprites that are rendering on this pixel, draw them according to their priority
	for(int x = obj_render_length; x > 0; x--)
	{
		sprite_id = obj_render_list[x-1];

		//Determine the internal X-Y coordinates of the sprite's pixel
		u16 sprite_tile_pixel_x = scanline_pixel_counter - obj[sprite_id].x;
		u16 sprite_tile_pixel_y = current_scanline - obj[sprite_id].y;

		//Horizontal flip the internal X coordinate
		if(obj[sprite_id].h_flip)
		{
			u16 h_flip = sprite_tile_pixel_x;
			u8 width = obj[sprite_id].width;
			u8 f_width = (width - 1);
			
	
			//Horizontal flipping
			if(h_flip < (width/2)) 
			{
				h_flip = ((f_width - h_flip) - h_flip);
				sprite_tile_pixel_x += h_flip;
			}

			else
			{
				h_flip = f_width - (2 * (f_width - h_flip));
				sprite_tile_pixel_x -= h_flip;
			}
		}

		//Vertical flip the internal Y coordinate
		if(obj[sprite_id].v_flip)
		{
			u16 v_flip = sprite_tile_pixel_y;
			u8 height = obj[sprite_id].height;
			u8 f_height = (height - 1);
			
	
			//Vertical flipping
			if(v_flip < (height/2)) 
			{
				v_flip = ((f_height - v_flip) - v_flip);
				sprite_tile_pixel_y += v_flip;
			}

			else
			{
				v_flip = f_height - (2 * (f_height - v_flip));
				sprite_tile_pixel_y -= v_flip;
			}
		}

		//Determine meta x-coordinate of rendered sprite pixel
		u8 meta_x = (sprite_tile_pixel_x / 8);

		//Determine meta Y-coordinate of rendered sprite pixel
		u8 meta_y = (sprite_tile_pixel_y / 8);

		//Determine which 8x8 section to draw pixel from, and what tile that actually represents in VRAM
		if(mem->read_u16(DISPCNT) & 0x40)
		{
			meta_sprite_tile = (meta_y * (obj[sprite_id].width/8)) + meta_x + obj[sprite_id].tile_number;	
		}

		else
		{
			meta_sprite_tile = obj[sprite_id].tile_number + (meta_y * 32) + meta_x;
		}

		//Determine sprite address
		sprite_tile_addr = 0x6010000 + (meta_sprite_tile * (obj[sprite_id].bit_depth << 3));

		meta_x = (sprite_tile_pixel_x % 8);
		meta_y = (sprite_tile_pixel_y % 8);

		u8 sprite_tile_pixel = (meta_y * 8) + meta_x;
		u16 color_bytes = 0;

		//Grab the byte corresponding to (sprite_tile_pixel), render it as ARGB - 4-bit version
		if(obj[sprite_id].bit_depth == 4)
		{
			sprite_tile_addr += (sprite_tile_pixel >> 1);
			raw_color = mem->read_u8(sprite_tile_addr);

			if((sprite_tile_pixel % 2) == 0) { raw_color &= 0xF; }
			else { raw_color >>= 4; }

			color_bytes = mem->read_u16(0x5000200 + (obj[sprite_id].palette_number * 32) + (raw_color * 2));
		}

		//Grab the byte corresponding to (sprite_tile_pixel), render it as ARGB - 8-bit version
		else
		{
			sprite_tile_addr += sprite_tile_pixel;
			raw_color = mem->read_u8(sprite_tile_addr);

			color_bytes = mem->read_u16(0x5000200 + (raw_color * 2));
		}

		//Only draw sprite pixel if it is not transparent, e.g. not Color #0 of a palette
		if(raw_color != 0)
		{
			//ARGB conversion
			u8 red = ((color_bytes & 0x1F) * 8);
			color_bytes >>= 5;

			u8 green = ((color_bytes & 0x1F) * 8);
			color_bytes >>= 5;

			u8 blue = ((color_bytes & 0x1F) * 8);

			u32 final_color =  0xFF000000 | (red << 16) | (green << 8) | (blue);

			scanline_buffer[scanline_pixel_counter] = final_color;
		}
	}

	return true;
}

/****** Determines if a background pixel should be rendered, and if so draws it to the current scanline pixel ******/
bool LCD::render_bg_pixel(u32 bg_control)
{
	u32 display_control = mem->read_u16(DISPCNT);

	if(((display_control & 0x100) == 0) && (bg_control == BG0CNT)) { return false; }
	else if(((display_control & 0x200) == 0) && (bg_control == BG1CNT)) { return false; }
	else if(((display_control & 0x400) == 0) && (bg_control == BG2CNT)) { return false; }
	else if(((display_control & 0x800) == 0) && (bg_control == BG3CNT)) { return false; }

	//Render BG pixel according to current BG Mode
	switch(display_control & 0x7)
	{
		//BG Mode 0
		case 0:
			render_bg_mode_0(bg_control); break;

		//BG Mode 1
		//case 1:
		//	render_bg_mode_0(bg_control); break;

		//BG Mode 4
		case 4:
			render_bg_mode_4(bg_control); break;

		default:
			std::cout<<"LCD::invalid or unsupported BG Mode : " << std::dec << (display_control & 0x7);
	}
}

/****** Render BG Mode 0 ******/
bool LCD::render_bg_mode_0(u32 bg_control)
{
	//Grab BG scrolling registers
	u16 x_scroll = 0;
	u16 y_scroll = 0;

	switch(bg_control)
	{
		//BG0 scroll values
		case BG0CNT: 
			x_scroll = mem->read_u16(BG0HOFS) & 0x1FF;
			y_scroll = mem->read_u16(BG0VOFS) & 0x1FF;
			break;

		//BG1 scroll values
		case BG1CNT: 
			x_scroll = mem->read_u16(BG1HOFS) & 0x1FF;
			y_scroll = mem->read_u16(BG1VOFS) & 0x1FF;
			break;

		//BG2 scroll values
		case BG2CNT: 
			x_scroll = mem->read_u16(BG2HOFS) & 0x1FF;
			y_scroll = mem->read_u16(BG2VOFS) & 0x1FF;
			break;

		//BG3 scroll values
		case BG3CNT: 
			x_scroll = mem->read_u16(BG3HOFS) & 0x1FF;
			y_scroll = mem->read_u16(BG3VOFS) & 0x1FF;
			break;
	}

	u16 bg_size = (mem->read_u16(bg_control) >> 14);
	u16 bg_width = 256;
	u16 bg_height = 256;
	u32 screen_offset = 0;

	//Determine BG screen size
	switch(bg_size)
	{
		//256x256
		case 0x0:
			bg_width = 256;
			bg_height = 256;
			break;

		//512x256
		case 0x1:
			bg_width = 512;
			bg_height = 256;
			break;

		//256x512
		case 0x2:
			bg_width = 256;
			bg_height = 512;
			break;

		//512x512
		case 0x3:
			bg_width = 512;
			bg_height = 512;
			break;
	}

	//Determine meta x-coordinate of rendered BG pixel
	u8 meta_x = ((scanline_pixel_counter + x_scroll) % bg_width) / 256;

	//Determine meta Y-coordinate of rendered BG pixel
	u8 meta_y = ((current_scanline + y_scroll) % bg_height) / 256;

	//Determine the address offset for the screen
	//SC1 - 512x256
	if((meta_x == 1) && (bg_size == 1)) { screen_offset = 0x800;  }

	//SC1 - 256x512
	else if((meta_x == 0) && (meta_y == 1) && (bg_size == 2)) { screen_offset = 0x800; }

	//SC1 - 512x512
	else if((meta_x == 1) && (meta_y == 0) && (bg_size == 3)) { screen_offset = 0x800; }

	//SC2 - 512x512
	else if((meta_x == 0) && (meta_y == 1) && (bg_size == 3)) { screen_offset = 0x1000; }

	//SC3 - 512x512
	else if((meta_x == 1) && (meta_y == 1) && (bg_size == 3)) { screen_offset = 0x1800; }

	//Determine whether color depth is 4-bit or 8-bit
	u8 bit_depth = (mem->read_u16(bg_control) & 0x80) ? 8 : 4;

	//Find out where the map base address is - Bits 2-3 of BG(X)CNT
	u32 map_base_addr = 0x6000000 + (0x800 * ((mem->read_u16(bg_control) >> 8) & 0x1F));
	map_base_addr += screen_offset;

	//Find out where the tile base address is - Bits 8-12 of BG(X)CNT
	u32 tile_base_addr = 0x6000000 + (0x4000 * ((mem->read_u16(bg_control) >> 2) & 0x3));

	//Determine the X-Y coordinates of the BG's tile on the tile map
	u16 current_tile_pixel_x = ((scanline_pixel_counter + x_scroll) % 256);
	u16 current_tile_pixel_y = ((current_scanline + y_scroll) % 256);

	//Get current map entry for rendered pixel
	u16 tile_number = ((current_tile_pixel_y / 8) * 32) + (current_tile_pixel_x / 8);

	//Look at the Tile Map #(tile_number), see what Tile # it points to
	u16 map_entry = mem->read_u16(map_base_addr + (tile_number * 2)) & 0x3FF;

	//Grab horizontal and vertical flipping options
	u8 flip_options = (mem->read_u16(map_base_addr + (tile_number * 2)) >> 10) & 0x3;

	//Grab the Palette number of the tiles
	u8 palette_number = (mem->read_u16(map_base_addr + (tile_number * 2)) >> 12);

	//Get address of Tile #(map_entry)
	u32 tile_addr = tile_base_addr + (map_entry * (bit_depth << 3));

	//Horizontal flip the internal X coordinate
	if(flip_options & 0x1) 
	{
		u8 h_flip = (current_tile_pixel_x % 8);
	
		//Horizontal flipping
		if(h_flip < 4) 
		{
			h_flip = ((7 - h_flip) - h_flip);
			current_tile_pixel_x += h_flip;
		}

		else
		{
			h_flip = 7 - (2 * (7 - h_flip));
			current_tile_pixel_x -= h_flip;
		}
	}

	//Vertical flip the internal Y coordinate
	if(flip_options & 0x2) 
	{
		u8 v_flip = (current_tile_pixel_y % 8);
	
		//Vertical flipping
		if(v_flip < 4) 
		{
			v_flip = ((7 - v_flip) - v_flip);
			current_tile_pixel_y += v_flip;
		}

		else
		{
			v_flip = 7 - (2 * (7 - v_flip));
			current_tile_pixel_y -= v_flip;
		}
	}

	u8 current_tile_pixel = ((current_tile_pixel_y % 8) * 8) + (current_tile_pixel_x % 8);
	u16 color_bytes = 0;

	//Grab the byte corresponding to (current_tile_pixel), render it as ARGB - 4-bit version
	if(bit_depth == 4)
	{
		tile_addr += (current_tile_pixel >> 1);
		u8 raw_color = mem->read_u8(tile_addr);

		if((current_tile_pixel % 2) == 0) { raw_color &= 0xF; }
		else { raw_color >>= 4; }

		//If the bg color is transparent, abort drawing
		if(raw_color == 0) { return false; }

		color_bytes = mem->read_u16(0x5000000 + (palette_number * 32) + (raw_color * 2));
	}

	//Grab the byte corresponding to (current_tile_pixel), render it as ARGB - 8-bit version
	else
	{
		tile_addr += current_tile_pixel;
		u8 raw_color = mem->read_u8(tile_addr);

		//If the bg color is transparent, abort drawing
		if(raw_color == 0) { return false; }

		color_bytes = mem->read_u16(0x5000000 + (raw_color * 2));
	}

	//ARGB conversion
	u8 red = ((color_bytes & 0x1F) * 8);
	color_bytes >>= 5;

	u8 green = ((color_bytes & 0x1F) * 8);
	color_bytes >>= 5;

	u8 blue = ((color_bytes & 0x1F) * 8);

	u32 final_color =  0xFF000000 | (red << 16) | (green << 8) | (blue);

	scanline_buffer[scanline_pixel_counter] = final_color;

	return true;
}

/****** Render BG Mode 4 ******/
bool LCD::render_bg_mode_4(u32 bg_control)
{
	//Determine which byte in VRAM to read for color data
	u32 bitmap_entry = (0x6000000 + (current_scanline * 240) + scanline_pixel_counter);

	u8 raw_color = mem->read_u8(bitmap_entry);
	if(raw_color == 0) { return false; }

	u16 color_bytes = mem->read_u16(0x5000000 + (raw_color * 2));

	//ARGB conversion
	u8 red = ((color_bytes & 0x1F) * 8);
	color_bytes >>= 5;

	u8 green = ((color_bytes & 0x1F) * 8);
	color_bytes >>= 5;

	u8 blue = ((color_bytes & 0x1F) * 8);

	u32 final_color =  0xFF000000 | (red << 16) | (green << 8) | (blue);

	scanline_buffer[scanline_pixel_counter] = final_color;

	return true;
}

/****** Render pixels for a given scanline (per-pixel) ******/
void LCD::render_scanline()
{
	//Use BG Palette #0, Color #0 as the backdrop
	u16 color_bytes = mem->read_u16(0x5000000);
	u8 red = ((color_bytes & 0x1F) * 8);
	color_bytes >>= 5;

	u8 green = ((color_bytes & 0x1F) * 8);
	color_bytes >>= 5;

	u8 blue = ((color_bytes & 0x1F) * 8);

	u32 final_color =  0xFF000000 | (red << 16) | (green << 8) | (blue);

	scanline_buffer[scanline_pixel_counter] = final_color;

	//Grab BG priorities 
	u8 priority_0 = mem->read_u16(BG0CNT) & 0x3;
	u8 priority_1 = mem->read_u16(BG1CNT) & 0x3;
	u8 priority_2 = mem->read_u16(BG2CNT) & 0x3;
	u8 priority_3 = mem->read_u16(BG3CNT) & 0x3;

	//Render BGs based on priority (3 is the 'lowest', 0 is the 'highest')
	for(int x = 3; x >= 0; x--)
	{
		if(priority_3 == x) { render_bg_pixel(BG3CNT); }
		if(priority_2 == x) { render_bg_pixel(BG2CNT); }
		if(priority_1 == x) { render_bg_pixel(BG1CNT); }
		if(priority_0 == x) { render_bg_pixel(BG0CNT); }
	}

	//Render sprites
	render_sprite_pixel();
}

/****** Run LCD for one cycle ******/
void LCD::step()
{
	lcd_clock++;

	//Update OAM
	if(mem->lcd_updates.oam_update) { update_oam(); }

	//Mode 0 - Scanline rendering
	if(((lcd_clock % 1232) <= 960) && (lcd_clock < 197120)) 
	{
		//Toggle HBlank flag OFF
		mem->memory_map[DISPSTAT] &= ~0x2;

		//Change mode
		if(lcd_mode != 0) { lcd_mode = 0; }

		//Render scanline data (per-pixel every 4 cycles)
		if((lcd_clock % 4) == 0) 
		{
			render_scanline();
			scanline_pixel_counter++;
		}
	}

	//Mode 1 - H-Blank
	else if(((lcd_clock % 1232) > 960) && (lcd_clock < 197120))
	{
		//Toggle HBlank flag ON
		mem->memory_map[DISPSTAT] |= 0x2;

		//Change mode
		if(lcd_mode != 1) 
		{ 
			lcd_mode = 1;
			scanline_pixel_counter = 0;

			//Raise HBlank interrupt
			if(mem->memory_map[DISPSTAT] & 0x10) { mem->memory_map[REG_IF] |= 0x2; }

			//Push scanline data to final buffer - Only if Forced Blank is disabled
			if((mem->memory_map[DISPCNT] & 0x80) == 0)
			{
				for(int x = 0, y = (240 * current_scanline); x < 240; x++, y++)
				{
					screen_buffer[y] = scanline_buffer[x];
				}
			}

			//Increment scanline count
			current_scanline++;
			mem->write_u16(VCOUNT, current_scanline);
			scanline_compare();
		}
	}

	//Mode 2 - VBlank
	else
	{
		//Toggle VBlank flag ON
		mem->memory_map[DISPSTAT] |= 0x1;

		//Toggle HBlank flag OFF
		mem->memory_map[DISPSTAT] &= ~0x2;

		//Change mode
		if(lcd_mode != 2) 
		{ 
			lcd_mode = 2;

			//Raise VBlank interrupt
			if(mem->memory_map[DISPSTAT] & 0x8) { mem->memory_map[REG_IF] |= 0x1; }

			//Render final buffer - Only if Forced Blank is disabled
			if((mem->memory_map[DISPCNT] & 0x80) == 0)
			{
				//Lock source surface
				if(SDL_MUSTLOCK(internal_screen)){ SDL_LockSurface(internal_screen); }
				u32* out_pixel_data = (u32*)internal_screen->pixels;

				for(int a = 0; a < 0x9600; a++) { out_pixel_data[a] = screen_buffer[a]; }

				//Unlock source surface
				if(SDL_MUSTLOCK(internal_screen)){ SDL_UnlockSurface(internal_screen); }
		
				//Blit
				SDL_BlitSurface(internal_screen, 0, final_screen, 0);
				if(SDL_Flip(final_screen) == -1) { std::cout<<"LCD::Error - Could not blit\n"; }
			}

			frame_current_time = SDL_GetTicks();
			if((frame_current_time - frame_start_time) < (1000/60)) { SDL_Delay((1000/60) - (frame_current_time - frame_start_time));}
				
			fps_count++;
			if((SDL_GetTicks() - fps_time) >= 1000) { fps_time = SDL_GetTicks(); std::cout<<"FPS : " <<  std::dec << (int)fps_count << "\n"; fps_count = 0; }
		}

		//Raise HBlank interrupt
		if((lcd_clock % 1232) == 960) 
		{
			//Toggle HBlank flag ON
			mem->memory_map[DISPSTAT] |= 0x2;

			current_scanline++;
			mem->write_u16(VCOUNT, current_scanline);
			scanline_compare();
				
			if(mem->memory_map[DISPSTAT] & 0x10) 
			{ 
				mem->memory_map[REG_IF] |= 0x2; 
			} 
		}

		//Reset LCD clock
		if(lcd_clock == 280896) 
		{
			//Toggle VBlank flag OFF
			mem->memory_map[DISPSTAT] &= ~0x1;

			//Toggle HBlank flag OFF
			mem->memory_map[DISPSTAT] &= ~0x2;

			lcd_clock = 0; 
			current_scanline = 0; 
			scanline_compare();
			scanline_pixel_counter = 0; 
			frame_start_time = SDL_GetTicks();
			mem->write_u16(VCOUNT, 0);
		}
	}
}

/****** Compare VCOUNT to LYC ******/
void LCD::scanline_compare()
{
	u16 disp_stat = mem->read_u16(DISPSTAT);
	u8 lyc = disp_stat >> 8;
	
	//Raise VCOUNT interrupt
	if(current_scanline == lyc)
	{
		if(mem->memory_map[DISPSTAT] & 0x20) 
		{ 
			mem->memory_map[REG_IF] |= 0x4; 
		}

		//Toggle VCOUNT flag ON
		disp_stat |= 0x4;
		mem->write_u16(DISPSTAT, disp_stat);
	}

	else
	{
		//Toggle VCOUNT flag OFF
		disp_stat &= ~0x4;
		mem->write_u16(DISPSTAT, disp_stat);
	}
}
		