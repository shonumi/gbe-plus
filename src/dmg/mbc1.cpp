// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mbc1.cpp
// Date : May 11, 2015
// Description : Game Boy Memory Bank Controller 1 I/O handling
//
// Handles reading and writing bytes to memory locations for MBC1
// Used to switch ROM and RAM banks in MBC1
// Also handles multicart (MBC1M) and sonar (MBC1S) variants

#include <cmath>

#include "mmu.h"
#include "common/util.h"

/****** Performs write operations specific to the MBC1 ******/
void DMG_MMU::mbc1_write(u16 address, u8 value)
{
	//Write to External RAM
	if((address >= 0xA000) && (address <= 0xBFFF) && (cart.ram))
	{
		if((bank_mode == 0) && (ram_banking_enabled)) { random_access_bank[0][address - 0xA000] = value; }
		else if((bank_mode == 1) && (ram_banking_enabled)) { random_access_bank[bank_bits][address - 0xA000] = value; }
	}

	//MBC register - Enable or Disable RAM Banking
	if((address <= 0x1FFF) && (cart.ram))
	{
		if((value & 0xF) == 0xA) { ram_banking_enabled = true; }
		else { ram_banking_enabled = false; }
	}

	//MBC register - Select ROM bank - Bits 0 to 4
	else if((address >= 0x2000) && (address <= 0x3FFF)) 
	{ 
		rom_bank = (value & 0x1F);
	}

	//MBC register - Select ROM bank bits 5 to 6 or Set or RAM bank
	else if((address >= 0x4000) && (address <= 0x5FFF)) { bank_bits = (value & 0x3); }

	//MBC register - ROM/RAM Select
	else if((address >= 0x6000) && (address <= 0x7FFF)) { bank_mode = (value & 0x1); }
}

/****** Performs read operations specific to the MBC1 ******/
u8 DMG_MMU::mbc1_read(u16 address)
{
	//Read using ROM Banking
	if((address >= 0x4000) && (address <= 0x7FFF))
	{
		u8 ext_rom_bank = ((bank_bits << 5) | rom_bank);
		
		//If bank is 0x20, 0x40, or 0x60, set to 0x21, 0x41, and 0x61 respectively
		if(ext_rom_bank == 0x20 || ext_rom_bank == 0x40 || ext_rom_bank == 0x60) { ext_rom_bank++; }
		
		//Ignore top 2 bits of MBC ROM select register if RAM Banking mode is enabled
		if(bank_mode == 1) { ext_rom_bank &= 0x1F; }

		//Ignore top 2 bits of MBC ROM select register if ROM size is 32 banks or less
		if(memory_map[ROM_ROMSIZE] < 0x5) { ext_rom_bank &= 0x1F; }

		//Read from Banks 2 and above
		if(ext_rom_bank >= 2) 
		{ 
			return read_only_bank[ext_rom_bank - 2][address - 0x4000];
		}

		//When reading from Banks 0-1, just use the memory map
		else { return memory_map[address]; }
	}

	//Read using RAM Banking
	else if((address >= 0xA000) && (address <= 0xBFFF))
	{
		if((bank_mode == 0) && (ram_banking_enabled)) { return random_access_bank[0][address - 0xA000]; }
		else if((bank_mode == 1) && (ram_banking_enabled)) { return random_access_bank[bank_bits][address - 0xA000]; }
		else { return 0x00; }
	}
}

/****** Performs write operations specific to the MBC1M ******/
void DMG_MMU::mbc1_multicart_write(u16 address, u8 value)
{
	//Write to External RAM
	if((address >= 0xA000) && (address <= 0xBFFF) && (cart.ram))
	{
		//MBC1M only accesses RAM Bank 0
		random_access_bank[0][address - 0xA000] = value;
	}

	//MBC register - Enable or Disable RAM
	if((address <= 0x1FFF) && (cart.ram))
	{
		if((value & 0xF) == 0xA) { ram_banking_enabled = true; }
		else { ram_banking_enabled = false; }
	}

	//MBC register - Select ROM bank - Bits 0 to 3
	if((address >= 0x2000) && (address <= 0x3FFF)) 
	{ 
		rom_bank = (value & 0xF);
	}

	//MBC register - Select ROM uppper 2 bank bits
	else if((address >= 0x4000) && (address <= 0x5FFF)) { bank_bits = (value & 0x3); }

	//MBC register - ROM/RAM Select
	//For the MBC1M, this selects whether or not to move another bank into ROM0
	else if((address >= 0x6000) && (address <= 0x7FFF)) { bank_mode = (value & 0x1); }
}

/****** Performs read operations specific to the MBC1M ******/
u8 DMG_MMU::mbc1_multicart_read(u16 address)
{
	//Read using ROM Banking - Bank 0
	if(address <= 0x3FFF)
	{
		//When the ROM/RAM select bit is unset, always read ROM0 normally
		if(bank_mode == 0) { return memory_map[address]; }

		//When the ROM/RAM select bit is set, read from Banks 0, 0x10, 0x20, or 0x30
		else
		{
			u8 ext_rom_bank = (bank_bits << 4);

			//Read from Banks 2 and above
			if(ext_rom_bank >= 2) 
			{
				return read_only_bank[ext_rom_bank - 2][address];
			}

			//When reading from Banks 0, just use the memory map
			else { return memory_map[address]; }
		}
	}

	//Read using ROM Banking - Banks 1 and above
	else if((address >= 0x4000) && (address <= 0x7FFF))
	{
		u8 ext_rom_bank = ((bank_bits << 4) | rom_bank);

		//If bank is 0x20, 0x40, or 0x60, set to 0x21, 0x41, and 0x61 respectively
		if(ext_rom_bank == 0x20 || ext_rom_bank == 0x40 || ext_rom_bank == 0x60) { ext_rom_bank++; }

		//Read from Banks 2 and above
		if(ext_rom_bank >= 2) 
		{ 
			return read_only_bank[ext_rom_bank - 2][address - 0x4000];
		}

		//When reading from Banks 0-1, just use the memory map
		else { return memory_map[address]; }
	}

	//Read cartridge RAM
	else if((address >= 0xA000) && (address <= 0xBFFF))
	{
		//MBC1M only accesses RAM Bank 0
		if(ram_banking_enabled) { return random_access_bank[0][address - 0xA000]; }
		else { return 0x00; }
	}
}

/****** Performs write operations specific to the MBC1S ******/
void DMG_MMU::mbc1s_write(u16 address, u8 value)
{
	//MBC register - Enable or Disable RAM Banking
	if((address <= 0x1FFF) && (cart.ram))
	{
		if((value & 0xF) == 0xA) { ram_banking_enabled = true; }
		else { ram_banking_enabled = false; }
	}

	//MBC register - Select ROM bank - Bits 0 to 4
	else if((address >= 0x2000) && (address <= 0x3FFF)) 
	{ 
		rom_bank = (value & 0x1F);
	}

	//MBC register - Sonar pulse
	else if((address >= 0x4000) && (address <= 0x5FFF))
	{
		if(bank_mode & 0x1)
		{
			//Reset counter when writing 1 then 0
			if((value == 0) && (bank_bits & 0x1))
			{
				cart.pulse_count = 0;
				cart.frame_count += 1;
				
				if(cart.frame_count >= 160) { cart.frame_count = 0; }
			}
		}

		bank_bits = (value & 0x1);
	}

	//MBC register - Activate sonar
	else if((address >= 0x6000) && (address <= 0x7FFF)) { bank_mode = (value & 0x1); }
}

/****** Performs read operations specific to the MBC1S ******/
u8 DMG_MMU::mbc1s_read(u16 address)
{
	//Read using ROM Banking
	if((address >= 0x4000) && (address <= 0x7FFF))
	{
		u8 ext_rom_bank = ((bank_bits << 5) | rom_bank);
		
		//If bank is 0x20, 0x40, or 0x60, set to 0x21, 0x41, and 0x61 respectively
		if(ext_rom_bank == 0x20 || ext_rom_bank == 0x40 || ext_rom_bank == 0x60) { ext_rom_bank++; }
		
		//Ignore top 2 bits of MBC ROM select register if RAM Banking mode is enabled
		if(bank_mode == 1) { ext_rom_bank &= 0x1F; }

		//Ignore top 2 bits of MBC ROM select register if ROM size is 32 banks or less
		if(memory_map[ROM_ROMSIZE] < 0x5) { ext_rom_bank &= 0x1F; }

		//Read from Banks 2 and above
		if(ext_rom_bank >= 2) 
		{ 
			return read_only_bank[ext_rom_bank - 2][address - 0x4000];
		}

		//When reading from Banks 0-1, just use the memory map
		else { return memory_map[address]; }
	}

	//Read sonar data
	else if((address >= 0xA000) && (address <= 0xBFFF))
	{
		//Grab frame slice size
		u32 slice_size = (cart.frame_data.size() == 15360) ? 96 : 192;

		//Find current index of frame data based on pulse count
		u32 index = cart.pulse_count;
		index += (slice_size * cart.frame_count);

		//Grab sonar data from frame
		if((index < cart.frame_data.size()) && (cart.pulse_count < slice_size)) {  cart.sonar_byte = cart.frame_data[index]; }

		cart.pulse_count++;

		//Return sonar byte
		return cart.sonar_byte;
	}
}

/****** Open external image and convert to sonar data ******/
bool DMG_MMU::mbc1s_load_sonar_data(std::string filename)
{
	if(filename.empty()) { return false; }

	//Load source image
	SDL_Surface* src_img = NULL;
	src_img = SDL_LoadBMP(filename.c_str());

	if(src_img == NULL)
	{
		std::cout<<"MMU::Error - Could not load sonar image " << filename << "\n";
		return false;
	}

	u32 output_w = 160;
	u32 output_h = (src_img->h > src_img->w) ? 192 : 96;

	//Grab image dimensions, calculate scaling - Ideal sonar images are 160x96
	double scale_x = src_img->w / output_w;
	double scale_y = src_img->h / output_h;

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

	//Scale buffer from input image to 160x96 or 160x192 pixel buffer
	std::vector<u8> pixel_buffer;

	for(u32 y = 0; y < output_h; y++)
	{
		for(u32 x = 0; x < output_w; x++)
		{
			u32 x_pos = x * scale_x;
			u32 y_pos = y * scale_y;
			u32 final_pos = (y_pos * src_img->w) + (x_pos);

			u32 final_color = 0xFF000000 | src_buffer[final_pos];

			//Darkest color
			if(final_color == 0xFF000000) { pixel_buffer.push_back(3); }
			
			//Semi-darkest color
			else if(final_color == 0xFF606060) { pixel_buffer.push_back(2); }

			//Semi-lightest color
			else if(final_color == 0xFFC0C0C0) { pixel_buffer.push_back(1); }

			//Lightest color
			else { pixel_buffer.push_back(0); }
		}
	}

	cart.frame_data.clear();
	u32 index = 0;
	u8 gt = 0;
	bool is_ground = false;

	//Convert pixel data frame data
	for(u32 x = 0; x < output_w; x++)
	{
		is_ground = false;

		for(u32 y = 0; y < output_h; y++)
		{
			index = (output_w * y) + x;
			u8 s_byte;			

			//Update ground detection
			if(!is_ground)
			{
				if(pixel_buffer[index] == 3)
				{
					gt++;	
					if(gt == 3) { is_ground = true; }
				}

				else { gt = 0; }
			}

			//Determine sonar byte
			switch(pixel_buffer[index])
			{
				//Open water : Not Applicable below ground (translate to Inner Floor 2)
				case 0: s_byte = 0x7; break;

				//Not Applicable above ground (translate to fish) : Inner Floor 1
				case 1: s_byte = (!is_ground) ? 0x1 : 0x7; break;

				//Inner Floor 2 : Not Applicable below ground (translate to Inner Floor 1)
				case 2: s_byte = (!is_ground) ? 0x3 : 0x7; break;

				//Outer floor : Outer floor
				case 3: s_byte = (!is_ground) ? 0x1 : 0x0; break;
			}

			cart.frame_data.push_back(s_byte);
		}
	}

	std::cout<<"MMU::Pocket Sonar image " << filename << " successfully loaded\n";

	return true;
}
