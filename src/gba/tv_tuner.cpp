// GB Enhanced+ Copyright Daniel Baxter 2024
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : tv_tuner.cpp
// Date : April 3, 2024
// Description : Agatsuma TV Tuner
//
// Handles I/O for the Agatsuma TV Tuner (ATVT)

#include "mmu.h"
#include "common/util.h"

/****** Resets ATVT data structure ******/
void AGB_MMU::tv_tuner_reset()
{
	tv_tuner.index = 0;
	tv_tuner.data = 0;
	tv_tuner.transfer_count = 0;
	tv_tuner.state = TV_TUNER_NEXT_DATA;

	tv_tuner.cnt_a = 0;
	tv_tuner.cnt_b = 0;
}

/****** Writes to ATVT I/O ******/
void AGB_MMU::write_tv_tuner(u32 address, u8 value)
{
	std::cout<<"TV TUNER WRITE -> 0x" << address << " :: 0x" << (u32)value << "\n";

	switch(address)
	{
		case TV_CNT_A:
			tv_tuner.cnt_a = value;
			break;

		case TV_CNT_B:
			tv_tuner.cnt_b = value;
			break;	
	}
}

/****** Reads from ATVT I/O ******/
u8 AGB_MMU::read_tv_tuner(u32 address)
{
	u8 result = 0;

	std::cout<<"TV TUNER READ -> 0x" << address << " :: 0x" << (u32)result << "\n";
	
	return result;
}
