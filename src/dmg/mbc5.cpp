// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mbc5.cpp
// Date : May 11, 2015
// Description : Game Boy Memory Bank Controller 5 I/O handling
//
// Handles reading and writing bytes to memory locations for MBC5
// Used to switch ROM and RAM banks in MBC5
// Also used for Rumble functionality

#include "mmu.h"

/****** Performs write operations specific to the MBC5 ******/
void DMG_MMU::mbc5_write(u16 address, u8 value)
{
	//Write to External RAM or RTC register
	if((address >= 0xA000) && (address <= 0xBFFF))
	{
		if(ram_banking_enabled) { random_access_bank[bank_bits][address - 0xA000] = value; }
	}

	//MBC register - Enable or Disable RAM Banking
	if(address <= 0x1FFF)
	{
		//Normal MBC5 operations
		if(config::cart_type != DMG_GBMEM)
		{
			if((value & 0xF) == 0xA) 
			{ 
				if(cart.ram) { ram_banking_enabled = true; } 
			}
			else { ram_banking_enabled = false; }
		}

		//GB Memory Cartridge flash commands + parameters
		else if((address >= 0x120) && (address <= 0x13F))
		{
			switch(address)
			{
				//Grab flash command
				case 0x120:
					cart.flash_cmd = value;
					cart.flash_stat = 0;
					break;

				//Equivalent to 0x2AAA
				case 0x121:
					cart.flash_stat |= 0x01;
					break;

				//Equivalent to 0x5555
				case 0x122:
					cart.flash_stat |= 0x02;
					break;

				//Execute flash command
				case 0x13F:
					if(value == 0xA5)
					{

						//Map in ROM with index (GBC/GBA) with reset
						if((cart.flash_cmd >= 0x80) && (cart.flash_cmd <= 0xBF))
						{
							cart.flash_io_bank = (cart.flash_cmd - 0x80);
							cart.flash_stat = 0xF0;

							//Forces reset in core
							lcd_stat->current_scanline = 144;

							std::cout<<"MMU::GB Memory Cartridge - Map Index 0x" << u32(cart.flash_io_bank) << " - Reset\n";
							break;
						}

						//Map in ROM with index (DMG/SGB) without reset
						else if((cart.flash_cmd >= 0xC0) && (cart.flash_cmd <= 0xFF))
						{
							cart.flash_io_bank = (cart.flash_cmd - 0xC0);
							cart.flash_stat |= 0xF0;

							//Forces reset in core
							lcd_stat->current_scanline = 144;

							std::cout<<"MMU::GB Memory Cartridge - Map Index 0x" << u32(cart.flash_io_bank) << " - No Reset\n";
							break;
						}

						switch(cart.flash_cmd)
						{
							//Map Entire Cartridge
							case 0x04:
								std::cout<<"MMU::GB Memory Cartridge - Map Entire\n";
								break;

							//Map Menu
							case 0x05:
								std::cout<<"MMU::GB Memory Cartridge - Map Menu\n";
								break;

							//Undo Wakeup
							case 0x08:
								std::cout<<"MMU::GB Memory Cartridge - Undo Wakeup\n";
								break;

							//Wake Up
							case 0x09:
								std::cout<<"MMU::GB Memory Cartridge - Wakeup\n";
								break;

							default:
								std::cout<<"MMU::Warning - Unhandled flash command for GB Memory Cartridge :: 0x" << u32(cart.flash_cmd) << "\n";
						}
					}

					break;
			}
		}			
	}

	//MBC register - Select ROM bank - Lower 8 bits
	if((address >= 0x2000) && (address <= 0x2FFF)) 
	{
		rom_bank &= ~0xFF;
		rom_bank |= value;
	}

	//MBC register - Select ROM Bank - High 9th bit
	if((address >= 0x3000) && (address <= 0x3FFF))
	{
		//If the game does not use all 512 available banks, ignore high 9th bit (treat as 0)
		//Lets games like Lufia: The Legend Returns boot properly
		if(memory_map[ROM_ROMSIZE] > 0x7)
		{
			if((value & 0x1) == 1) { rom_bank |= 0x100; }
			else { rom_bank &= ~0x100; }
		}
	}

	//MBC register - Select RAM bank or Enable/Disable rumbling
	if((address >= 0x4000) && (address <= 0x5FFF)) 
	{
		//Rumble - RAM bank is bits 0-2, rumble is Bit 3
		if(cart.rumble)
		{
			bank_bits = (value & 0x7);
			
			if(value & 0x8) { g_pad->start_rumble(); }
			else { g_pad->stop_rumble(); }
		}

		//RAM Bank is bits 0-3 on non-rumble carts 
		else { bank_bits = (value & 0xF); }

		//Mirror RAM R-W to Bank 0 when using only 1 Bank
		//Most MBC5 games don't rely on mirroring, but DWI&II does
		//Nintendo specifically says to avoid it though (nice one Capcom...)
		if(memory_map[ROM_RAMSIZE] <= 2) { bank_bits = 0; }
	}
}

/****** Performs write operations specific to the MBC5 ******/
u8 DMG_MMU::mbc5_read(u16 address)
{
	//Read using ROM Banking
	if((address >= 0x4000) && (address <= 0x7FFF))
	{
		if(rom_bank >= 2) { return read_only_bank[rom_bank - 2][address - 0x4000]; }

		//When reading from Banks 0-1, just use the memory map
		else { return memory_map[address]; }
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
