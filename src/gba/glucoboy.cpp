// GB Enhanced+ Copyright Daniel Baxter 2023
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : glucoboy.cpp
// Date : June 5, 2023
// Description : Campho Advance
//
// Handles I/O for the Glucoboy

#include "mmu.h"
#include "common/util.h"

/****** Resets Glucoboy data structure ******/
void AGB_MMU::glucoboy_reset()
{
	glucoboy.io_index = 0;
	glucoboy.io_regs.clear();
	glucoboy.io_regs.resize(0x100, 0x00);
	glucoboy.index_shift = 0;
	glucoboy.request_interrupt = false;
	glucoboy.reset_shift = false;

	//Setup GRP data
	glucoboy.io_regs[0x21] = config::glucoboy_daily_grps;
	glucoboy.io_regs[0x22] = config::glucoboy_bonus_grps;
	glucoboy.io_regs[0x23] = config::glucoboy_good_days;
	glucoboy.io_regs[0x24] = config::glucoboy_days_until_bonus;
}

/****** Writes data to Glucoboy I/O ******/
void AGB_MMU::write_glucoboy(u32 address, u8 value)
{
	glucoboy.io_index = value;
	glucoboy.request_interrupt = true;
	glucoboy.reset_shift = true;
}

/****** Reads data from Glucoboy I/O ******/
u8 AGB_MMU::read_glucoboy(u32 address)
{
	//Read data from I/O index, read MSB-first
	u8 result = (glucoboy.io_regs[glucoboy.io_index] >> glucoboy.index_shift);

	//If additional data needs to be read, trigger another IRQ
	if(glucoboy.index_shift)
	{
		glucoboy.request_interrupt = true;
		glucoboy.index_shift -= 8;
	}

	//std::cout<<"GLUCOBOY READ -> 0x" << address << " :: 0x" << (u32)result << "\n";

	return result;
}

/****** Handles Glucoboy interrupt requests and what data to respond with ******/
void AGB_MMU::process_glucoboy_irq()
{
	//Trigger Game Pak IRQ
	memory_map[REG_IF+1] |= 0x20;
	glucoboy.request_interrupt = false;

	//Set data size of each index (8-bit or 32-bit)
	if(glucoboy.reset_shift)
	{
		switch(glucoboy.io_index)
		{
			case 0x31:
			case 0x32:
			case 0xE0:
			case 0xE1:
			case 0xE2:
				glucoboy.index_shift = 0;
				break;

			default:
				glucoboy.index_shift = 24;
		}

		glucoboy.reset_shift = false;
	}
}
