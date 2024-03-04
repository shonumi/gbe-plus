// GB Enhanced+ Copyright Daniel Baxter 2023
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : nmp.cpp
// Date : October 27, 2023
// Description : Nintendo MP3 Player
//
// Handles I/O for the Nintendo MP3 Player
// Manages IRQs and firmware reads/writes
// Play-Yan and Play-Yan Micro handled separately (see play_yan.cpp)

#include "mmu.h"
#include "common/util.h" 

/****** Writes to Nintendo MP3 Player I/O ******/
void AGB_MMU::write_nmp(u32 address, u8 value)
{
	//std::cout<<"PLAY-YAN WRITE -> 0x" << address << " :: 0x" << (u32)value << "\n";

	switch(address)
	{
		//Device Control
		case PY_NMP_CNT:
			play_yan.access_mode &= ~0xFF00;
			play_yan.access_mode |= (value << 8);
			break;

		//Device Control
		case PY_NMP_CNT+1:
			play_yan.access_mode &= ~0xFF;
			play_yan.access_mode |= value;

			//After firmware is loaded, the Nintendo MP3 Player generates a Game Pak IRQ.
			//Confirm firmware is finished with this write after booting.
			if((play_yan.access_mode == 0x0808) && (play_yan.op_state == PLAY_YAN_INIT))
			{
				play_yan.irq_delay = 30;
				play_yan.op_state = PLAY_YAN_BOOT_SEQUENCE;
			}

			//Terminate command input now. Actual execution happens later.
			//Command is always first 16-bits of stream
			else if((play_yan.access_mode == 0x0404) && (play_yan.op_state == PLAY_YAN_PROCESS_CMD))
			{
				if(play_yan.command_stream.size() >= 2)
				{
					play_yan.cmd = (play_yan.command_stream[0] << 8) | play_yan.command_stream[1];
					process_nmp_cmd();
				}
			}

			break;

		//Device Parameter
		case PY_NMP_PARAMETER:
			play_yan.access_param &= ~0xFF00;
			play_yan.access_param |= (value << 8);
			break;

		//Device Parameter
		case PY_NMP_PARAMETER+1:
			play_yan.access_param &= ~0xFF;
			play_yan.access_param |= value;

			//Set high 16-bits of the param or begin processing commands now
			if(play_yan.access_mode == 0x1010) { play_yan.access_param <<= 16; }
			else if(play_yan.access_mode == 0) { access_nmp_io(); }

			break;

		//Device Data Input (firmware, commands, etc?)
		case PY_NMP_DATA_IN:
		case PY_NMP_DATA_IN+1:
			if(play_yan.firmware_addr)
			{
				play_yan.firmware[play_yan.firmware_addr++] = value;
			}

			else if(play_yan.op_state == PLAY_YAN_PROCESS_CMD)
			{
				play_yan.command_stream.push_back(value);
			}

			break;
	}	
}

/****** Reads from Nintendo MP3 Player I/O ******/
u8 AGB_MMU::read_nmp(u32 address)
{
	u8 result = 0;

	switch(address)
	{
		case PY_NMP_DATA_OUT:
		case PY_NMP_DATA_OUT+1:

			//Return SD card data
			if(play_yan.op_state == PLAY_YAN_GET_SD_DATA)
			{
				if(play_yan.nmp_data_index < play_yan.card_data.size())
				{
					result = play_yan.card_data[play_yan.nmp_data_index++];
				}
			}

			//Some kind of status data read after each Game Pak IRQ
			else if(play_yan.nmp_data_index < 16)
			{
				result = play_yan.nmp_status_data[play_yan.nmp_data_index++];
			}

			break;
	}

	//std::cout<<"PLAY-YAN READ -> 0x" << address << " :: 0x" << (u32)result << "\n";

	return result;
}

/****** Handles Nintendo MP3 Player command processing ******/
void AGB_MMU::process_nmp_cmd()
{
	std::cout<<"CMD -> 0x" << play_yan.cmd << "\n";

	switch(play_yan.cmd)
	{
		//Undocumented command
		case 0x10:
			play_yan.nmp_cmd_status = 0x4010;
			play_yan.nmp_valid_command = true;

			break;

		//Undocumented command
		case 0x11:
			play_yan.nmp_cmd_status = 0x4011;
			play_yan.nmp_valid_command = true;

			break;

		//Change directory
		case 0x20:
			play_yan.nmp_cmd_status = 0x4020;
			play_yan.nmp_valid_command = true;
			play_yan.current_dir = "";

			//Grab directory
			for(u32 x = 2; x < play_yan.command_stream.size(); x++)
			{
				u8 chr = play_yan.command_stream[x];
				if(!chr) { break; }

				play_yan.current_dir += chr;
			}
				
			break;

		//Generate Sound (for menus) - No IRQ generated
		case 0x200:
			break;

		//Check for firmware update file
		case 0x300:
			play_yan.nmp_cmd_status = 0x4300;
			play_yan.nmp_valid_command = true;
			
			break;

		//Undocumented command (firmware update related?)
		case 0x301:
			play_yan.nmp_cmd_status = 0x4301;
			play_yan.nmp_valid_command = true;
			
			break;

		//Undocumented command (firmware update related?)
		case 0x303:
			play_yan.nmp_cmd_status = 0x4303;
			play_yan.nmp_valid_command = true;
			play_yan.cmd = 0;
			
			break;

		default:
			play_yan.nmp_valid_command = false;
			play_yan.nmp_cmd_status = 0;
			std::cout<<"Unknown Nintendo MP3 Player Command -> 0x" << play_yan.cmd << "\n";
	}
}

/****** Handles prep work for accessing Nintendo MP3 Player I/O such as writing commands, cart status, busy signal etc ******/
void AGB_MMU::access_nmp_io()
{
	play_yan.firmware_addr = 0;

	//Determine which kinds of data to access (e.g. cart status, hardware busy flag, command stuff, etc)
	if(play_yan.access_param)
	{
		//std::cout<<"ACCESS -> 0x" << play_yan.access_param << "\n";
		play_yan.firmware_addr = (play_yan.access_param << 1);

		u16 stat_data = 0;

		//Cartridge Status
		if(play_yan.access_param == 0x100)
		{
			//Cartridge status during initial boot phase (e.g. Health and Safety screen)
			if(play_yan.nmp_init_stage < 4)
			{
				stat_data = play_yan.nmp_boot_data[play_yan.nmp_init_stage >> 1];
				play_yan.nmp_init_stage++;

				if(play_yan.nmp_init_stage == 2) { memory_map[REG_IF+1] |= 0x20; }
			}

			//Status after running a command
			else if(play_yan.nmp_cmd_status)
			{
				stat_data = play_yan.nmp_cmd_status;
				play_yan.op_state = PLAY_YAN_STATUS;
			}
		}

		//Write command or wait for command to finish
		else if(play_yan.access_param == 0x10F)
		{
			play_yan.op_state = PLAY_YAN_PROCESS_CMD;
			play_yan.firmware_addr = 0;
			play_yan.command_stream.clear();

			//Finish command
			if(play_yan.nmp_valid_command)
			{
				memory_map[REG_IF+1] |= 0x20;
				play_yan.nmp_valid_command = false;
			}

			//Increment internal ticks
			//Value here is 6 ticks, a rough average of how often a real NMP updates at ~60Hz
			play_yan.nmp_ticks += 6;
			stat_data = play_yan.nmp_ticks;
		}

		//I/O Busy Flag
		else if(play_yan.access_param == 0x110)
		{
			//1 = I/O Busy, 0 = I/O Ready. For now, we are never busy
			play_yan.op_state = PLAY_YAN_WAIT;
		}

		play_yan.nmp_status_data[0] = (stat_data >> 8);
		play_yan.nmp_status_data[1] = (stat_data & 0xFF);
		play_yan.nmp_data_index = 0;
		play_yan.access_param = 0;
	}

	//Access SD card data
	else
	{
		play_yan.card_data.clear();
		play_yan.op_state = PLAY_YAN_GET_SD_DATA;
		play_yan.nmp_data_index = 0;
	}
}
