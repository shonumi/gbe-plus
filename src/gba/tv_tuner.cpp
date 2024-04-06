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
	tv_tuner.state = TV_TUNER_STOP_DATA;
	tv_tuner.data_stream.clear();

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
			tv_tuner.data_stream.push_back(value);
			tv_tuner.transfer_count++;

			//Reset transfer count
			if((value & 0xF0) != 0xC0)
			{
				tv_tuner.transfer_count = 0;
				tv_tuner.data_stream.clear();
			}

			//Start data transfer
			else if((tv_tuner.state == TV_TUNER_STOP_DATA) && (tv_tuner.transfer_count == 3))
			{
				if((tv_tuner.data_stream[1] & 0xF3) == 0xC3)
				{
					std::cout<<"DATA START\n";

					tv_tuner.state = TV_TUNER_START_DATA;
					tv_tuner.data_stream.clear();
					tv_tuner.transfer_count = 0;
				}
			}

			//Grab 8-bit data from stream;
			else if((tv_tuner.state == TV_TUNER_START_DATA) && (tv_tuner.transfer_count == 32))
			{
				tv_tuner.state == TV_TUNER_ACK_DATA;
				tv_tuner.data = 0;
				u8 mask = 0x80;

				for(u32 x = 0; x < 8; x++)
				{
					if(tv_tuner.data_stream[(x * 4) + 1] & 0x01)
					{
						tv_tuner.data |= mask;
					}

					mask >>= 1;
				}

				std::cout<<"DATA -> 0x" << (u32)tv_tuner.data << "\n";
				
			}
			
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
