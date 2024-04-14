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
	tv_tuner.current_channel = 2;
	tv_tuner.state = TV_TUNER_STOP_DATA;
	tv_tuner.data_stream.clear();
	tv_tuner.cmd_stream.clear();
	tv_tuner.video_stream.clear();
	tv_tuner.read_request = false;

	for(u32 x = 0; x < 62; x++)
	{
		tv_tuner.is_channel_on[x] = false;
	}

	tv_tuner.cnt_a = 0;
	tv_tuner.cnt_b = 0;
}

/****** Writes to ATVT I/O ******/
void AGB_MMU::write_tv_tuner(u32 address, u8 value)
{
	//std::cout<<"TV TUNER WRITE -> 0x" << address << " :: 0x" << (u32)value << "\n";

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
					if(tv_tuner.data_stream[2] & 0x01)
					{
						tv_tuner.state = TV_TUNER_DELAY_DATA;
					}

					else
					{
						std::cout<<"START\n";
						tv_tuner.state = TV_TUNER_START_DATA;
					}

					tv_tuner.data_stream.clear();
					tv_tuner.transfer_count = 0;
				}
			}

			//Delayed start of data transfer
			else if((tv_tuner.state == TV_TUNER_DELAY_DATA) && (tv_tuner.transfer_count == 1))
			{
				std::cout<<"START\n";
				tv_tuner.state = TV_TUNER_START_DATA;
				tv_tuner.data_stream.clear();
				tv_tuner.transfer_count = 0;
			}

			//Stop data transfer - After Write Request
			else if((tv_tuner.state == TV_TUNER_NEXT_DATA) && (tv_tuner.transfer_count == 3) && (!tv_tuner.read_request))
			{
				if((tv_tuner.data_stream[1] & 0xF3) == stop_mask)
				{
					std::cout<<"STOP\n";
					tv_tuner.state = TV_TUNER_STOP_DATA;

					//Check for specific commands
					process_tv_tuner_cmd();

					tv_tuner.data_stream.clear();
					tv_tuner.cmd_stream.clear();
					tv_tuner.transfer_count = 0;
					tv_tuner.read_request = false;
				}
			}

			//Stop data transfer - After Read Request
			else if((tv_tuner.state == TV_TUNER_NEXT_DATA) && (tv_tuner.transfer_count == 4) && (tv_tuner.read_request))
			{
				if(((tv_tuner.data_stream[0] & 0x03) == 0x01) && ((tv_tuner.data_stream[1] & 0x03) == 0x00)
				&& ((tv_tuner.data_stream[2] & 0x03) == 0x02) && ((tv_tuner.data_stream[3] & 0x03) == 0x03))
				{
					std::cout<<"STOP\n";
					tv_tuner.state = TV_TUNER_STOP_DATA;
					tv_tuner.data_stream.clear();
					tv_tuner.cmd_stream.clear();
					tv_tuner.transfer_count = 0;
					tv_tuner.read_request = false;
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

				tv_tuner.cmd_stream.push_back(tv_tuner.data);
			}

			//Read data into stream
			if(tv_tuner.state == TV_TUNER_READ_DATA)
			{
				//TODO - Output bits every 3 transfers

				if(tv_tuner.transfer_count == 24)
				{
					tv_tuner.state = TV_TUNER_ACK_DATA;
					tv_tuner.data_stream.clear();
					tv_tuner.cmd_stream.clear();
					tv_tuner.transfer_count = 0;
					tv_tuner.cnt_a |= 0x40;
				}
			}

			//Wait for acknowledgement
			else if((tv_tuner.state == TV_TUNER_ACK_DATA) && (tv_tuner.transfer_count == 4))
			{
				std::cout<<"ACK\n";
				tv_tuner.state = TV_TUNER_NEXT_DATA;
				tv_tuner.data_stream.clear();
				tv_tuner.transfer_count = 0;

				//Check for specific commands
				process_tv_tuner_cmd();
			}
			
			break;

		case TV_CNT_C:
			tv_tuner.cnt_a = value;
			break;

		case TV_CNT_D:
			tv_tuner.cnt_b = value;
			break;	
	}
}

/****** Reads from ATVT I/O ******/
u8 AGB_MMU::read_tv_tuner(u32 address)
{
	u8 result = 0;

	switch(address)
	{
		case TV_CNT_A:
			result = tv_tuner.cnt_a;
			break;

		case TV_CNT_B:
			result = tv_tuner.cnt_b;
			break;
	}

	if((address >= 0xA800000) && (address < 0xA812C00))
	{
		u32 index = (address - 0xA800000);
		result = tv_tuner.video_stream[index];
	}
	
	//std::cout<<"TV TUNER READ -> 0x" << address << " :: 0x" << (u32)result << "\n";

	return result;
}

/****** Handles ATVT commands ******/
void AGB_MMU::process_tv_tuner_cmd()
{
	//D8 command -> Writes 16-bit values
	if((tv_tuner.cmd_stream.size() == 3) && (tv_tuner.cmd_stream[0] == 0xD8) && (tv_tuner.state == TV_TUNER_STOP_DATA))
	{
		u8 param_1 = tv_tuner.cmd_stream[1];
		u8 param_2 = tv_tuner.cmd_stream[2];

		std::cout<<"CMD 0xD8 -> 0x" << (u32)param_1 << " :: 0x" << (u32)param_2 << "\n";

		//Render video frame
		if((param_1 == 0x0D) && (param_2 == 0x00))
		{
			tv_tuner_render_frame();
		}
	}

	//D8 command -> Writes 8-bit values
	else if((tv_tuner.cmd_stream.size() == 2) && (tv_tuner.cmd_stream[0] == 0xD8) && (tv_tuner.state == TV_TUNER_STOP_DATA))
	{
		u8 param_1 = tv_tuner.cmd_stream[1];

		std::cout<<"CMD 0xD8 -> 0x" << (u32)param_1 << "\n";
	}

	//D9 command -> Reads a single 8-bit value
	else if((tv_tuner.cmd_stream.size() == 1) && (tv_tuner.cmd_stream[0] == 0xD9) && (tv_tuner.state == TV_TUNER_NEXT_DATA))
	{
		tv_tuner.state = TV_TUNER_READ_DATA;
		tv_tuner.read_request = true;

		std::cout<<"CMD 0xD9\n";
	}
}

/****** Renders video data for a frame for the AVTV ******/
void AGB_MMU::tv_tuner_render_frame()
{
	tv_tuner.video_stream.clear();

	for(u32 x = 0; x < 0x12C00; x++)
	{
		tv_tuner.video_stream.push_back(0x33);
	}
}
