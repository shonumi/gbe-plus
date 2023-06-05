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
	glucoboy.index = 0;
	glucoboy.io_regs.clear();
	glucoboy.io_regs.resize(0x100, 0x00);
}

/****** Writes data to Glucoboy I/O ******/
void AGB_MMU::write_glucoboy(u32 address, u8 value)
{

}

/****** Reads data from Glucoboy I/O ******/
u8 AGB_MMU::read_glucoboy(u32 address)
{
	u8 result = 0;

	return result;
}
