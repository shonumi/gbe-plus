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
	tv_tuner.current_channel = 1;
	tv_tuner.state = TV_TUNER_STOP_DATA;
	tv_tuner.data_stream.clear();
	tv_tuner.cmd_stream.clear();
	tv_tuner.video_stream.clear();
	tv_tuner.read_request = false;
	tv_tuner.is_av_input_on = false;

	tv_tuner.video_brightness = 0;
	tv_tuner.video_contrast = 0;
	tv_tuner.video_hue = 0;

	u16 temp_channel_list[62] =
	{
		0x0890, 0x08F0, 0x0950, 0x0D90, 0x0DF0, 0x0E50, 0x0EB0, 0x0EF0, 0x0F50, 0x0FB0,
		0x1010, 0x1070, 0x2050, 0x20B0, 0x2110, 0x2170, 0x21D0, 0x2230, 0x2290, 0x22F0,
		0x2350, 0x23B0, 0x2410, 0x2470, 0x24D0, 0x2530, 0x2590, 0x25F0, 0x2650, 0x26B0,
		0x2710, 0x2770, 0x27D0, 0x2830, 0x2890, 0x28F0, 0x2950, 0x29B0, 0x2A10, 0x2A70,
		0x2AD0, 0x2B30, 0x2B90, 0x2BF0, 0x2C50, 0x2CB0, 0x2D10, 0x2D70, 0x2DD0, 0x2E30,
		0x2E90, 0x2EF0, 0x2F50, 0x2FB0, 0x3010, 0x3070, 0x30D0, 0x3130, 0x3190, 0x31F0,
		0x3250, 0x32B0
	};

	for(u32 x = 0; x < 62; x++)
	{
		tv_tuner.is_channel_on[x] = false;
		tv_tuner.channel_id_list[x] = temp_channel_list[x];
	}

	tv_tuner.cnt_a = 0;
	tv_tuner.cnt_b = 0;
}

/****** Writes to ATVT I/O ******/
void AGB_MMU::write_tv_tuner(u32 address, u8 value)
{
	//std::cout<<"TV TUNER WRITE -> 0x" << address << " :: 0x" << (u32)value << " :: " << (u32)tv_tuner.state <<"\n";

	switch(address)
	{
		case TV_CNT_A:
		case TV_CNT_B:
			tv_tuner.data_stream.push_back(value);
			tv_tuner.transfer_count++;

			//Reset transfer count
			if((value == 0x67) || (value == 0xE7))
			{
				tv_tuner.transfer_count = 0;
				tv_tuner.data_stream.clear();
			}

			//Start data transfer
			else if((tv_tuner.state == TV_TUNER_STOP_DATA) && (tv_tuner.transfer_count == 3))
			{
				if((tv_tuner.data_stream[1] & 0x03) == 0x03)
				{
					if(tv_tuner.data_stream[2] & 0x01)
					{
						tv_tuner.state = TV_TUNER_DELAY_DATA;
					}

					else
					{
						//std::cout<<"START\n";
						tv_tuner.state = TV_TUNER_START_DATA;
					}

					tv_tuner.data_stream.clear();
					tv_tuner.transfer_count = 0;
				}
			}

			//Delayed start of data transfer
			else if((tv_tuner.state == TV_TUNER_DELAY_DATA) && (tv_tuner.transfer_count == 1))
			{
				if((value & 0x1) == 0)
				{
					//std::cout<<"START\n";
					tv_tuner.state = TV_TUNER_START_DATA;
				}

				tv_tuner.data_stream.clear();
				tv_tuner.transfer_count = 0;
			}

			//Stop data transfer - After Write Request
			else if((tv_tuner.state == TV_TUNER_NEXT_DATA) && (tv_tuner.transfer_count == 3) && (!tv_tuner.read_request))
			{
				if((tv_tuner.data_stream[1] & 0x03) == 0x02)
				{
					//std::cout<<"STOP\n";
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
					//std::cout<<"STOP\n";
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
				//std::cout<<"ACK\n";
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
	
	else
	{
		//std::cout<<"TV TUNER READ -> 0x" << address << " :: 0x" << (u32)result << "\n";
	}

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

		//std::cout<<"CMD 0xD8 -> 0x" << (u32)param_1 << " :: 0x" << (u32)param_2 << "\n";

		//Render video frame
		if((param_1 == 0x0D) && (param_2 == 0x00))
		{
			tv_tuner_render_frame();
		}

		//Switch between TV channels and AV input
		else if(param_1 == 0x02)
		{
			tv_tuner.is_av_input_on = (param_2 == 0xE1) ? true : false;

			if(tv_tuner.is_av_input_on) { std::cout<<"AV INPUT ON\n"; }
			else { std::cout<<"TV INPUT ON\n"; }
		}

		//Set video contrast level
		else if(param_1 == 0x10)
		{
			tv_tuner.video_contrast = param_2;

			std::cout<<"VIDEO CONTRAST -> 0x" << (u32)param_2 << "\n";
		}

		//Set video brightness level
		else if(param_1 == 0x11)
		{
			tv_tuner.video_brightness = param_2;

			std::cout<<"VIDEO BRIGHTNESS -> 0x" << (u32)param_2 << "\n";
		}
	}

	//D8 command -> Writes 8-bit values
	else if((tv_tuner.cmd_stream.size() == 2) && (tv_tuner.cmd_stream[0] == 0xD8) && (tv_tuner.state == TV_TUNER_STOP_DATA))
	{
		u8 param_1 = tv_tuner.cmd_stream[1];

		//std::cout<<"CMD 0xD8 -> 0x" << (u32)param_1 << "\n";
	}

	//D9 command -> Reads a single 8-bit value
	else if((tv_tuner.cmd_stream.size() == 1) && (tv_tuner.cmd_stream[0] == 0xD9) && (tv_tuner.state == TV_TUNER_NEXT_DATA))
	{
		tv_tuner.state = TV_TUNER_READ_DATA;
		tv_tuner.read_request = true;

		//std::cout<<"CMD 0xD9\n";
	}

	//C0 Command -> Reads a 16-bit value
	else if((tv_tuner.cmd_stream.size() == 3) && (tv_tuner.cmd_stream[0] == 0xC0))
	{
		u8 param_1 = tv_tuner.cmd_stream[1];
		u8 param_2 = tv_tuner.cmd_stream[2];

		//Get 16-bit channel ID - Used to change channel
		if(param_1 != 0x86)
		{
			u16 channel_id = ((param_1 << 8) | param_2);

			for(u32 x = 0; x < 62; x++)
			{
				if(channel_id == tv_tuner.channel_id_list[x])
				{
					tv_tuner.current_channel = x;
					break;
				}
			}

			std::cout<<"CHANNEL CHANGE -> " << std::dec << ((u32)tv_tuner.current_channel + 1) << std::hex << "\n";
		}
	}

	//Unknown command
	else if(tv_tuner.state == TV_TUNER_STOP_DATA)
	{
		std::cout<<"UNKOWN CMD -> ";
		
		for(u32 x = 0; x < tv_tuner.cmd_stream.size(); x++)
		{
			std::cout<<"0x" << (u32)tv_tuner.cmd_stream[x] << " :: ";
		}

		std::cout<<"\n";
	}
}

/****** Renders video data for a frame for the AVTV ******/
void AGB_MMU::tv_tuner_render_frame()
{
	tv_tuner.video_stream.clear();

	//Render static noise (grayscale) if channel has no signal
	if(!tv_tuner.is_channel_on[tv_tuner.current_channel])
	{
		srand(SDL_GetTicks());
		u8 num = 0;
		u16 color = 0;
		u8 bit = 0;

		for(u32 x = 0; x < 0x12C00; x += 2)
		{
			num = (rand() % 0x1F); 

			u16 color = (num << 10) | (num << 5) | num;
			
			tv_tuner.video_stream.push_back(color & 0xFF);
			tv_tuner.video_stream.push_back(color >> 8);
		}
	}
}
