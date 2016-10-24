// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : camera.cpp
// Date : October 21, 2016
// Description : Game Boy Camera I/O handling
//
// Handles reading and writing bytes to memory locations for the GB Camera
// Used to switch ROM and RAM banks in the GB Camera
// Also used to access specific registers for the Camera sensor
// Loads external images into the GB Camera, converts them, and saves them

#include "mmu.h"
#include "common/util.h"

/****** Performs write operations specific to the GB Camera ******/
void DMG_MMU::cam_write(u16 address, u8 value)
{
	//MBC register - Enable or Disable RAM Banking (writing only, banks can still be read, camera registers too!)
	if(address <= 0x1FFF)
	{
		if((value & 0xF) == 0xA) { ram_banking_enabled = true; }
		else { ram_banking_enabled = false; }
	}

	//MBC register - Select ROM bank - Bits 0 to 5
	else if((address >= 0x2000) && (address <= 0x3FFF)) 
	{ 
		rom_bank = (value & 0x3F);
	}

	//MBC register - Select RAM bank or camera registers
	else if((address >= 0x4000) && (address <= 0x5FFF)) 
	{ 
		bank_bits = value;

		//If Bit 4 is set, access camera registers
		if(bank_bits & 0x10) { bank_bits = 0x10; }

		//Otherwise, access RAM banks 0x0 - 0xF;
		else { bank_bits &= 0xF; }
	}

	//Write to External RAM or camera register
	else if((address >= 0xA000) && (address <= 0xBFFF))
	{
		//Write to RAM bank normally
		if((ram_banking_enabled) && (bank_bits != 0x10)) { random_access_bank[bank_bits][address - 0xA000] = value; }

		//Write to camera registers
		else if((ram_banking_enabled) && (bank_bits == 0x10))
		{
			//Camera registers repeat every 0x80 bytes
			u8 reg_id = (address - 0xA000) % 0x80;

			if(reg_id <= 53) { cart.cam_reg[reg_id] = value; }
		}
	}
}

/****** Performs write operations specific to the GB Camera ******/
u8 DMG_MMU::cam_read(u16 address)
{
	//Read using ROM Banking
	if((address >= 0x4000) && (address <= 0x7FFF))
	{
		//Note, Bank 1 can map any of the 64 banks, including Bank 0
		if(rom_bank == 0) { return memory_map[address - 0x4000]; }

		//Read Bank 1 normally
		else if(rom_bank == 1) { return memory_map[address]; }

		//Read Banks 2 - 63
		else { return read_only_bank[rom_bank - 2][address - 0x4000]; }
	}

	//Read using RAM Banking or camera register
	else if((address >= 0xA000) && (address <= 0xBFFF))
	{
		//Read RAM bank if RAM enabled
		if((ram_banking_enabled) && (bank_bits != 0x10)) { return random_access_bank[bank_bits][address - 0xA000]; }

		//Read camera registers - Only 0xA000 can be read, all others are write-only
		else if((bank_bits == 0x10) && (address == 0xA000)) { return cart.cam_reg[0]; }

		else { return 0x00; }
	}
}

/****** Open external image and convert to GB colors in VRAM ******/
bool DMG_MMU::cam_load_snapshot(std::string filename)
{
	//Load source image
	SDL_Surface* src_img = NULL;
	src_img = SDL_LoadBMP(filename.c_str());

	if(src_img == NULL)
	{
		std::cout<<"MMU::Error - Could not load GB snapshot " << filename << "\n";
		return false;
	}

	//Grab image dimensions, calculate scaling - GB Camera images are 128x112
	double scale_x = src_img->w / 128.0;
	double scale_y = src_img->h / 112.0;

	std::vector<u32> src_buffer;

	//Lock surface
	if(SDL_MUSTLOCK(src_img)) { SDL_LockSurface(src_img); }

	//Grab pixel data from input image - Load 24-bit color data
	//Load 24-bit data
	u8* pixel_data = (u8*)src_img->pixels;

	for(int a = 0, b = 0; a < (src_img->w * src_img->h); a++, b+=3)
	{
		src_buffer.push_back(0xFF000000 | (pixel_data[b+2] << 16) | (pixel_data[b+1] << 8) | (pixel_data[b]));
	}

	//Unlock source surface
	if(SDL_MUSTLOCK(src_img)){ SDL_UnlockSurface(src_img); }

	//Scale buffer from input image to 128x112 pixel buffer
	std::vector<u32> pixel_buffer;

	for(u32 y = 0; y < 112; y++)
	{
		for(u32 x = 0; x < 128; x++)
		{
			u32 x_pos = x * scale_x;
			u32 y_pos = y * scale_y;
			u32 final_pos = (y_pos * src_img->w) + (x_pos);

			u32 final_color = src_buffer[final_pos];

			//Convert to GB grayscale
			u8 brightness = util::get_brightness_fast(final_color) / 64;

			switch(brightness)
			{
				//Darkest color
				case 0x0: pixel_buffer.push_back(config::DMG_BG_PAL[3]); break;
				
				//Semi-darkest color
				case 0x1: pixel_buffer.push_back(config::DMG_BG_PAL[2]); break;

				//Semi-lightest color
				case 0x2: pixel_buffer.push_back(config::DMG_BG_PAL[1]); break;

				//Lightest color
				case 0x3: pixel_buffer.push_back(config::DMG_BG_PAL[0]); break;
			}
		}
	}

	return true;
}
