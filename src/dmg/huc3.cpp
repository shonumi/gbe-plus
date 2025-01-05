// GB Enhanced Copyright Daniel Baxter 2019
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : huc3.cpp
// Date : January 16, 2019
// Description : Game Boy HuC-3 I/O handling
//
// Handles reading and writing bytes to memory locations for HuC-3
// Used to switch ROM and RAM banks in HuC-3

#include "mmu.h"

/****** Performs write operations specific to the HuC-3 ******/
void DMG_MMU::huc3_write(u16 address, u8 value)
{
	//Write to External RAM
	if((address >= 0xA000) && (address <= 0xBFFF))
	{
		//Handle IR signals via networking
		if(ir_trigger)
		{
			//IR Type must be specified as a HuC IR cart!
			if(sio_stat->ir_type == HUC_IR_CART)
			{
				ir_signal = (value & 0x1);
				ir_send = true;
			}
		}

		//Otherwise write to RAM if enabled
		else if(ram_banking_enabled) { random_access_bank[bank_bits][address - 0xA000] = value; }
	}

	//MBC register - Enable or Disable RAM Write or IR
	if(address <= 0x1FFF)
	{
		if((value & 0xF) == 0xA) { ram_banking_enabled = true; }
		else { ram_banking_enabled = false; }

		if((value & 0xF) == 0xE) { ir_trigger = 1; }
		else { ir_trigger = 0; }
	}

	//MBC register - Select ROM bank
	else if((address >= 0x2000) && (address <= 0x3FFF))
	{
		rom_bank = (value & 0x7F);
		
		//Lowest switchable ROM bank is 1
		if(rom_bank == 0) { rom_bank++; }
	}

	//MBC register - Select RAM bank
	else if((address >= 0x4000) && (address <= 0x5FFF)) { bank_bits = (value & 0x3); }
} 

/****** Performs read operations specific to the HuC-3 ******/
u8 DMG_MMU::huc3_read(u16 address)
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
	else if((address >= 0xA000) && (address <= 0xBFFF))
	{
		//Prioritize IR reading if applicable
		if(ir_trigger) { return 0xC0 | (cart.huc_ir_input); }

		//Otherwise read from RAM
		else if(ram_banking_enabled) { return random_access_bank[bank_bits][address - 0xA000]; }
		else { return 1; }
	}

	//For all unhandled reads, attempt to return the value from the memory map
	return memory_map[address];
}
