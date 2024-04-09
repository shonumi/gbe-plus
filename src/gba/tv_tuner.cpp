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

	u8 reset_mask_a = 0xC0;
	u8 reset_mask_b = 0x40;
	u8 reset_mask;

	u8 start_mask_a = 0xC3;
	u8 start_mask_b = 0x43;
	u8 start_mask;

	u8 stop_mask_a = 0xC2;
	u8 stop_mask_b = 0x42;
	u8 stop_mask;

	bool is_tv_a;

	switch(address)
	{
		case TV_CNT_A:
		case TV_CNT_B:
			is_tv_a = (address == TV_CNT_A);
			reset_mask = (is_tv_a) ? reset_mask_a : reset_mask_b;
			start_mask = (is_tv_a) ? start_mask_a : start_mask_b;
			stop_mask = (is_tv_a) ? stop_mask_a : stop_mask_b;

			tv_tuner.cnt_a = value;
			tv_tuner.data_stream.push_back(value);
			tv_tuner.transfer_count++;

			//Reset transfer count
			if((value & 0xF0) != reset_mask)
			{
				tv_tuner.transfer_count = 0;
				tv_tuner.data_stream.clear();
			}

			//Start data transfer
			else if((tv_tuner.state == TV_TUNER_STOP_DATA) && (tv_tuner.transfer_count == 3))
			{
				if((tv_tuner.data_stream[1] & 0xF3) == start_mask)
				{
					std::cout<<"DATA START\n";

					tv_tuner.state = TV_TUNER_START_DATA;
					tv_tuner.data_stream.clear();
					tv_tuner.transfer_count = 0;
				}
			}

			//Stop data transfer
			else if((tv_tuner.state == TV_TUNER_NEXT_DATA) && (tv_tuner.transfer_count == 3))
			{
				if((tv_tuner.data_stream[1] & 0xF3) == stop_mask)
				{
					std::cout<<"DATA STOP\n";

					tv_tuner.state = TV_TUNER_STOP_DATA;
					tv_tuner.data_stream.clear();
					tv_tuner.transfer_count = 0;
				}
			}

			//Grab 8-bit data from stream;
			else if(((tv_tuner.state == TV_TUNER_START_DATA) || (tv_tuner.state == TV_TUNER_NEXT_DATA))
			&& (tv_tuner.transfer_count == 32))
			{
				tv_tuner.state = TV_TUNER_ACK_DATA;
				tv_tuner.data_stream.clear();
				tv_tuner.transfer_count = 0;
				
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

			//Wait for acknowledgement
			else if((tv_tuner.state == TV_TUNER_ACK_DATA) && (tv_tuner.transfer_count == 4))
			{
				std::cout<<"DATA ACK\n";

				tv_tuner.state = TV_TUNER_NEXT_DATA;
				tv_tuner.data_stream.clear();
				tv_tuner.transfer_count = 0;
			}
			
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
