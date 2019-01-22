// GB Enhanced Copyright Daniel Baxter 2019
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : tama5.cpp
// Date : January 21, 2019
// Description : Game Boy TAMA5 I/O handling
//
// Handles reading and writing bytes to memory locations for TAMA5
// Used to switch ROM, access RAM and RTC

#include "mmu.h" 

/****** Performs write operations specific to the TAMA5 ******/
void DMG_MMU::tama5_write(u16 address, u8 value)
{
	//Access TAMA5 registers
	if((address >= 0xA000) && (address <= 0xBFFF) && (cart.ram))
	{
		//Unlock MBC
		if(((bank_mode & 0x80) == 0) && (address == 0xA001) && (value == 0xA))
		{
			//Bit 7 = Unlock flag. Bit 6 = Read from 0xA000 flag
			bank_mode |= 0x80;
			bank_mode &= ~0x40;
		}

		//Select MBC register
		else if(((bank_mode & 0xC0) == 0xC0) && (address == 0xA001))
		{
			//Store TAMA5 MBC register as single byte in lower halfbyte
			bank_mode |= (value & 0xF);
		}

		//Write register data
		else if(((bank_mode & 0xC0) == 0xC0) && (address == 0xA000))
		{
			switch(bank_mode & 0xF)
			{
				//ROM Bank Low
				case 0x0:
					rom_bank &= ~0xF;
					rom_bank |= (value & 0xF);
					break;

				//ROM Bank High
				case 0x1:
					rom_bank &= ~0xF0;
					rom_bank |= ((value & 0xF) << 4);
					break;
			}
		}
	}
}


/****** Performs read operations specific to the TAMA5 ******/
u8 DMG_MMU::tama5_read(u16 address)
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

	//Access TAMA5 registers
	else if((address >= 0xA000) && (address <= 0xBFFF))
	{
		//Unlock MBC
		if((bank_mode & 0xC0) == 0x80)
		{
			bank_mode |= 0x40;
			return 0xF1;
		}
	}
}