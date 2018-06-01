// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mbc1.cpp
// Date : May 11, 2015
// Description : Game Boy Memory Bank Controller 1 I/O handling
//
// Handles reading and writing bytes to memory locations for MBC1
// Used to switch ROM and RAM banks in MBC1

#include "mmu.h"

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

	//Read using RAM Banking
	else if((address >= 0xA000) && (address <= 0xBFFF))
	{
		if((bank_mode == 0) && (ram_banking_enabled)) { return random_access_bank[0][address - 0xA000]; }
		else if((bank_mode == 1) && (ram_banking_enabled)) { return random_access_bank[bank_bits][address - 0xA000]; }
		else { return 0x00; }
	}
}