// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : huc1.cpp
// Date : September 05, 2016
// Description : Game Boy HuC-1 I/O handling
//
// Handles reading and writing bytes to memory locations for HuC-1
// Used to switch ROM and RAM banks in HuC-1

#include "mmu.h"

/****** Performs write operations specific to the HuC-1 ******/
void DMG_MMU::huc1_write(u16 address, u8 value)
{
	//Write to External RAM
	if((address >= 0xA000) && (address <= 0xBFFF))
	{
		//Only write to RAM if writing has been enabled
		if((bank_mode == 0) && (ram_banking_enabled)) { random_access_bank[0][address - 0xA000] = value; }
		else if((bank_mode == 1) && (ram_banking_enabled)) { random_access_bank[bank_bits][address - 0xA000] = value; }
	}

	//MBC register - Enable or Disable RAM Write
	if(address <= 0x1FFF)
	{
		if((value & 0xF) == 0xA) { ram_banking_enabled = true; }
		else { ram_banking_enabled = false; }
	}

	//MBC register - Select ROM bank
	else if((address >= 0x2000) && (address <= 0x3FFF))
	{
		rom_bank = (value & 0x3F);
		
		//Lowest switchable ROM bank is 1
		if(rom_bank == 0) { rom_bank++; }
	}

	//MBC register - Select RAM bank
	else if((address >= 0x4000) && (address <= 0x5FFF)) { bank_bits = (value & 0x3); }

	//MBC register - Switch between 8KB and 32KB
	else if((address >= 0x6000) && (address <= 0x7FFF)) { bank_mode = (value & 0x1); }
}

/****** Performs read operations specific to the MBC1 ******/
u8 DMG_MMU::huc1_read(u16 address)
{
	//Read using ROM Banking - Bank 0
	if(address <= 0x3FFF) { return memory_map[address]; }

	//Read using ROM Banking
	if((address >= 0x4000) && (address <= 0x7FFF))
	{
		//Read from Banks 2 and above
		if(rom_bank >= 2) 
		{ 
			return read_only_bank[rom_bank - 2][address - 0x4000];
		}

		//When reading from Bank 1, just use the memory map
		else { return memory_map[address]; }
	}

	//Read using RAM Banking
	//RAM is always enabled of the HuC-1
	else if((address >= 0xA000) && (address <= 0xBFFF))
	{
		if(bank_mode == 0) { return random_access_bank[0][address - 0xA000]; }
		else { return random_access_bank[bank_bits][address - 0xA000]; }
	}
}
