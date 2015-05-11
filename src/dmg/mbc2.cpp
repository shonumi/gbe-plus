// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mbc2.cpp
// Date : May 11, 2015
// Description : Game Boy Memory Bank Controller 2 I/O handling
//
// Handles reading and writing bytes to memory locations for MBC2
// Used to switch ROM banks in MBC2
// Used to read internal RAM in MBC2

#include "mmu.h"

/****** Performs write operations specific to the MBC2 ******/
void DMG_MMU::mbc2_write(u16 address, u8 value)
{
	//Write to Internal RAM
	if((address >= 0xA000) && (address <= 0xA1FF) && (ram_banking_enabled))
	{
		random_access_bank[0][address - 0xA000] = (value & 0xF);
	}

	//MBC register - Enable or Disable RAM
	//Note, MBC2 does not use RAM banking, so this variable is a bit of a misnomer
	else if(address <= 0x1FFF)
	{
		ram_banking_enabled = true;
		if((address & 0x100) == 0) { ram_banking_enabled = true; }
		else { ram_banking_enabled = false; }
	} 

	//MBC register - Select ROM bank - Bits 0 to 3
	else if((address >= 0x2000) && (address <= 0x3FFF)) 
	{ 	
		if(address & 0x100) { rom_bank = (value & 0xF); }
	}
}

/****** Performs read operations specific to the MBC2 ******/
u8 DMG_MMU::mbc2_read(u16 address)
{
	//Read using ROM Banking
	if((address >= 0x4000) && (address <= 0x7FFF))
	{
		if(rom_bank >= 2) { return read_only_bank[rom_bank - 2][address - 0x4000]; }

		//When reading from Banks 0-1, just use the memory map
		else { return memory_map[address]; }
	}

	//Read from Internal RAM
	else if((address >= 0xA000) && (address <= 0xA1FF))
	{
		if(ram_banking_enabled) { return (random_access_bank[0][address - 0xA000] & 0xF); }
		else { return 0x00; }
	}

	else { return memory_map[address]; }
} 
