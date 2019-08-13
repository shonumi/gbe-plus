// GB Enhanced Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mmm01.cpp
// Date : December 14, 2016
// Description : Game Boy MMM01 I/O handling
//
// Handles reading and writing bytes to memory locations for MMM01
// Used to switch ROM and RAM banks in MMM01

#include "mmu.h"

/****** Performs write operations specific to the MMM01 ******/
void DMG_MMU::mmm01_write(u16 address, u8 value)
{
	//Write to External RAM
	if((address >= 0xA000) && (address <= 0xBFFF) && (cart.ram))
	{
		if(ram_banking_enabled) { random_access_bank[bank_bits][address - 0xA000] = value; }
	}

	//MBC register - Enable or Disable RAM banking or Set ROM mode
	if(address <= 0x1FFF)
	{
		//Set new ROM mode
		if(bank_mode == 0) { bank_mode = 1; }

		//Enable or Disable RAM banking
		else if((cart.ram) && ((value & 0xF) == 0xA)) { ram_banking_enabled = true; }
		else { ram_banking_enabled = false; }
	}

	//MBC register - Select ROM base or ROM bank
	else if((address >= 0x2000) && (address <= 0x3FFF)) 
	{ 
		//For convenience (save state compatibility) top 8-bits is ROM base
		if(bank_mode == 0)
		{
			rom_bank &= 0xFF;
			rom_bank |= ((value & 0x1F) << 8);
		}

		//For convenience (save state compatibility) lower 8-bits is ROM bank
		else
		{
			rom_bank &= 0xFF00;
			rom_bank |= (value & 0x1F);
		}
	}

	//MBC register - Set RAM bank
	else if((address >= 0x4000) && (address <= 0x5FFF))
	{
		if(bank_mode == 1) { bank_bits = (value & 0x3); }
	}
}

/****** Performs read operations specific to the MMM01 ******/
u8 DMG_MMU::mmm01_read(u16 address)
{
	//Read using ROM Banking - Bank 0
	if(address <= 0x3FFF)
	{
		//Read normally if ROM mode has not been set
		if(bank_mode == 0) { return memory_map[address]; }

		//Read from another base bank
		else
		{
			u8 base_bank = (rom_bank >> 8) & 0xFF;
			return read_only_bank[base_bank][address];
		}
	}

	//Read using ROM Banking - Bank 1
	else if((address >= 0x4000) && (address <= 0x7FFF))
	{
		//Read normally if ROM mode has not been set
		if(bank_mode == 0) { return memory_map[address]; }

		//Read from another base bank + ROM bank
		else
		{
			u8 base_bank = (rom_bank >> 8) & 0xFF;
			u8 ext_bank = rom_bank & 0xFF;

			return read_only_bank[base_bank + ext_bank][address - 0x4000];
		}
	}

	//Read using RAM Banking
	else if((address >= 0xA000) && (address <= 0xBFFF))
	{
		if(ram_banking_enabled) { return random_access_bank[bank_bits][address - 0xA000]; }
		else { return 0x00; }
	}

	//For all unhandled reads, attempt to return the value from the memory map
	return memory_map[address];
}
