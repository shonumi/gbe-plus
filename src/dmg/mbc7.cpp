// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mbc7.cpp
// Date : December 26, 2015
// Description : Game Boy Memory Bank Controller 7 I/O handling
//
// Handles reading and writing bytes to memory locations for MBC7
// Used to switch ROM and RAM banks in MBC7
// Also used for Gyroscope functionality

#include "mmu.h" 

/****** Performs write operations specific to the MBC7 ******/
void DMG_MMU::mbc7_write(u16 address, u8 value)
{
	//MBC register - Select ROM bank
	if((address >= 0x2000) && (address <= 0x3FFF)) 
	{ 
		rom_bank = (value & 0x7F);
	}

	//MBC register - Select RAM bank
	if((address >= 0x4000) && (address <= 0x5FFF)) 
	{ 
		bank_bits = value;
	}
}

/****** Performs write operations specific to the MBC7 ******/
u8 DMG_MMU::mbc7_read(u16 address)
{
	//Read using ROM Banking
	if((address >= 0x4000) && (address <= 0x7FFF))
	{
		if(rom_bank >= 2) 
		{ 
			return read_only_bank[rom_bank - 2][address - 0x4000];
		}

		//When reading from Banks 0-1, just use the memory map
		else { return memory_map[address]; }
	}

	//Read from RAM or gyroscope
	else if((address >= 0xA000) && (address <= 0xBFFF))
	{
		switch(address & 0xA0F0)
		{
			//Return 0 for invalid reads
			case 0xA000:
			case 0xA010:
			case 0xA060:
			case 0xA070:
				return 0x0;

			//Gyroscope X - Low Byte
			case 0xA020:
				return 0x0;

			//Gyroscope X - High Byte
			case 0xA030:
				return 0x0;

			//Gyroscope Y - Low Byte
			case 0xA040:
				return 0x0;

			//Gyroscope Y - High Byte
			case 0xA050:
				return 0x0;

			//Byte from RAM
			case 0xA080:
				return cart.internal_value;
		
			default:
				return 0x0;
		}
	}
}