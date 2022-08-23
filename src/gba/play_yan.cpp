// GB Enhanced+ Copyright Daniel Baxter 2022
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : play_yan.cpp
// Date : August 19, 2022
// Description : Nintendo Play-Yan
//
// Handles I/O for the Nintendo Play-Yan
// Manages IRQs and firmware reads/writes

#include "mmu.h"
#include "common/util.h"

/****** Resets Play-Yan data structure ******/
void AGB_MMU::play_yan_reset()
{
	play_yan.firmware.clear();
	play_yan.firmware.resize(0x100000, 0x00);

	play_yan.firmware_addr = 0;
	play_yan.firmware_status = 0x10;
	play_yan.firmware_addr_count = 0;

	play_yan.status = 0x80;

	play_yan.access_mode = 0;
	play_yan.access_param = 0;

	play_yan.irq_count = 0;
	play_yan.irq_delay = 240;

	for(u32 x = 0; x < 12; x++) { play_yan.cnt_data[x] = 0; }

	for(u32 x = 0; x < 5; x++)
	{
		for(u32 y = 0; y < 8; y++)
		{
			play_yan.sd_check_data[x][y] = 0x55555555;
		}
	}

	for(u32 x = 0; x < 8; x++)
	{
		play_yan.music_check_data[x] = 0x0;
	}

	for(u32 x = 0; x < 3; x++)
	{
		for(u32 y = 0; y < 8; y++)
		{
			play_yan.video_check_data[x][y] = 0x0;
		}
	}

	//Set 32-bit flags for SD Check interrupts
	play_yan.sd_check_data[0][0] = 0x80000100; play_yan.sd_check_data[0][1] = 0x00000000;
	play_yan.sd_check_data[1][0] = 0x40008000; play_yan.sd_check_data[1][1] = 0x00000005; play_yan.sd_check_data[1][2] = 0x00000000;
	play_yan.sd_check_data[2][0] = 0x40800000; play_yan.sd_check_data[2][1] = 0x00000005; play_yan.sd_check_data[2][2] = 0x00000000;
	play_yan.sd_check_data[3][0] = 0x80000100; play_yan.sd_check_data[3][1] = 0x00000000;
	play_yan.sd_check_data[4][0] = 0x40000200; play_yan.sd_check_data[4][1] = 0x00042C78;

	//Set 32-bit flags for entering/exiting music menu
	play_yan.music_check_data[0] = 0x80001000;

	//Set 32-bit flags for entering/exiting video menu
	play_yan.video_check_data[0][0] = 0x40800000;
	play_yan.video_check_data[1][0] = 0x80000100;
	play_yan.video_check_data[2][0] = 0x40000200;

	for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
}

/****** Writes to Play-Yan I/O ******/
void AGB_MMU::write_play_yan(u32 address, u8 value)
{
	std::cout<<"PLAY-YAN WRITE -> 0x" << address << " :: 0x" << (u32)value << "\n";

	switch(address)
	{
		//Unknown I/O
		case PY_UNK_00:
		case PY_UNK_00+1:
		case PY_UNK_02:
		case PY_UNK_02+1:
			break;

		//Access Mode (determines firmware read/write)
		case PY_ACCESS_MODE:
			play_yan.access_mode = value;
			break;

		//Firmware address
		case PY_FIRM_ADDR:
			if(play_yan.firmware_addr_count < 2)
			{
				play_yan.firmware_addr &= ~0xFF;
				play_yan.firmware_addr |= value;
			}

			else
			{
				play_yan.firmware_addr &= ~0xFF0000;
				play_yan.firmware_addr |= (value << 16);
			}

			play_yan.firmware_addr_count++;
			play_yan.firmware_addr_count &= 0x3;

			break;

		//Firmware address
		case PY_FIRM_ADDR+1:
			if(play_yan.firmware_addr_count < 2)
			{
				play_yan.firmware_addr &= ~0xFF00;
				play_yan.firmware_addr |= (value << 8);
			}

			else
			{
				play_yan.firmware_addr &= ~0xFF000000;
				play_yan.firmware_addr |= (value << 24);
			}

			play_yan.firmware_addr_count++;
			play_yan.firmware_addr_count &= 0x3;

			break;

		//Parameter for reads and writes
		case PY_ACCESS_PARAM:
			play_yan.access_param &= ~0xFF;
			play_yan.access_param |= value;
			break;

		//Parameter for reads and writes
		case PY_ACCESS_PARAM+1:
			play_yan.access_param &= ~0xFF00;
			play_yan.access_param |= (value << 8);
			break;
	}

	//Write to firmware area
	if((address >= 0xB000100) && (address < 0xB000300) && ((play_yan.access_param == 0x09) || (play_yan.access_param == 0x0A)))
	{
		u32 offset = address - 0xB000100;
		
		if((play_yan.firmware_addr + offset) < 0xFF020)
		{
			play_yan.firmware[play_yan.firmware_addr + offset] = value;
		}
	}

	//Write to ... something else (control structure?)
	if((address >= 0xB000100) && (address <= 0xB00010B) && (play_yan.access_param == 0x08) && (play_yan.firmware_addr >= 0xFF020))
	{
		u32 offset = address - 0xB000100;

		if(offset <= 0x0B) { play_yan.cnt_data[offset] = value; }

		//Check for control command
		if(address == 0xB000103)
		{
			u32 control_cmd = ((play_yan.cnt_data[3] << 24) | (play_yan.cnt_data[2] << 16) | (play_yan.cnt_data[1] << 8) | (play_yan.cnt_data[offset]));

			//Trigger Game Pak IRQ for entering/exiting music menu
			if((control_cmd == 0x00000400) && (play_yan.op_state == 0xFF))
			{
				play_yan.op_state = 2;
				play_yan.irq_delay = 1;
			}

			//Trigger Game Pak IRQ for entering/exiting video menu
			else if((control_cmd == 0x00800000) && (play_yan.op_state == 0xFF))
			{
				play_yan.op_state = 3;
				play_yan.irq_delay = 1;
			}
		}
	}

}

/****** Reads from Play-Yan I/O ******/
u8 AGB_MMU::read_play_yan(u32 address)
{
	u8 result = memory_map[address];

	switch(address)
	{
		//Some kind of data stream
		case PY_INIT_DATA:
			result = 0x00;
			break;

		//Play-Yan Status
		case PY_STAT:
			result = play_yan.status;
			break;

		//Parameter for reads and writes
		case PY_ACCESS_PARAM:
			result = (play_yan.access_param & 0xFF);
			break;

		//Parameter for reads and writes
		case PY_ACCESS_PARAM+1:
			result = ((play_yan.access_param >> 8) & 0xFF);
			break;

		//Firmware Status
		case PY_FIRM_STAT:
			result = (play_yan.firmware_status & 0xFF);
			break;

		//Firmware Status
		case PY_FIRM_STAT+1:
			result = ((play_yan.firmware_status >> 8) & 0xFF);
			break;
	}

	//Read IRQ data
	if((play_yan.irq_data_in_use) && (address >= 0xB000300) && (address < 0xB000320))
	{
		u32 offset = (address - 0xB000300) >> 2;
		u8 shift = (address & 0x3);

		//Switch back to reading firmware after all IRQ data is read
		if(address == 0xB00031C) { play_yan.irq_data_in_use = false; }

		result = (play_yan.irq_data[offset] >> (shift << 3));
	}

	//Read from firmware area
	else if((address >= 0xB000300) && (address < 0xB000500) && ((play_yan.access_param == 0x09) || (play_yan.access_param == 0x0A)))
	{
		u32 offset = address - 0xB000300;
		
		if((play_yan.firmware_addr + offset) < 0xFF020)
		{
			result = play_yan.firmware[play_yan.firmware_addr + offset];

			//Update Play-Yan firmware address if necessary
			if(offset == 0x1FE) { play_yan.firmware_addr += 0x200; }
		}
	}

	std::cout<<"PLAY-YAN READ -> 0x" << address << " :: 0x" << (u32)result << "\n";

	return result;
}

/****** Handles Play-Yan interrupt requests including delays and what data to respond with ******/
void AGB_MMU::process_play_yan_irq()
{
	//Wait for a certain amount of frames to pass to simulate delays in Game Pak IRQs firing
	if(play_yan.irq_delay)
	{
		play_yan.irq_delay--;
		if(play_yan.irq_delay) { return; }
	}

	//Process SD card check first and foremost after booting
	if(!play_yan.op_state)
	{
		play_yan.op_state = 1;
		play_yan.irq_count = 0;
	}

	switch(play_yan.op_state)
	{
		//SD card check
		case 0x1:
			//Trigger Game Pak IRQ
			memory_map[REG_IF+1] |= 0x20;

			//Wait for next IRQ condition after sending all flags
			if(play_yan.irq_count == 5)
			{
				play_yan.op_state = 0xFF;
				play_yan.irq_delay = 0;
				play_yan.irq_count = 0;
			}

			//Send data for IRQ
			else
			{
				for(u32 x = 0; x < 8; x++)
				{
					play_yan.irq_data[x] = play_yan.sd_check_data[play_yan.irq_count][x];
				}

				play_yan.irq_count++;
				play_yan.irq_delay = 120;
				play_yan.irq_data_in_use = true;
			}

			break;

		//Enter/exit music menu
		case 0x2:
			//Trigger Game Pak IRQ
			memory_map[REG_IF+1] |= 0x20;

			//Wait for next IRQ condition after sending all flags
			if(play_yan.irq_count == 1)
			{
				play_yan.op_state = 0xFF;
				play_yan.irq_delay = 0;
				play_yan.irq_count = 0;
			}

			//Send data for IRQ
			else
			{
				for(u32 x = 0; x < 8; x++)
				{
					play_yan.irq_data[x] = play_yan.music_check_data[play_yan.irq_count];
				}

				play_yan.irq_count++;
				play_yan.irq_delay = 10;
				play_yan.irq_data_in_use = true;
			}

			break;

		//Enter/exit vieo menu
		case 0x3:
			//Trigger Game Pak IRQ
			memory_map[REG_IF+1] |= 0x20;

			//Wait for next IRQ condition after sending all flags
			if(play_yan.irq_count == 3)
			{
				play_yan.op_state = 0xFF;
				play_yan.irq_delay = 0;
				play_yan.irq_count = 0;
			}

			//Send data for IRQ
			else
			{
				for(u32 x = 0; x < 8; x++)
				{
					play_yan.irq_data[x] = play_yan.video_check_data[play_yan.irq_count][x];
				}

				play_yan.irq_count++;
				play_yan.irq_delay = 10;
				play_yan.irq_data_in_use = true;
			}

			break;
	}
}
