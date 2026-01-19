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
	glucoboy.parameter_length = 0;

	glucoboy.daily_grps = config::glucoboy_daily_grps;
	glucoboy.bonus_grps = config::glucoboy_bonus_grps;
	glucoboy.good_days = config::glucoboy_good_days;
	glucoboy.days_until_bonus = config::glucoboy_days_until_bonus;
	glucoboy.hardware_flags = 0;
	glucoboy.ld_threshold = 0;
	glucoboy.serial_number = 0;

	//Setup index data
	glucoboy.io_regs[0x21] = glucoboy.daily_grps;
	glucoboy.io_regs[0x22] = glucoboy.bonus_grps;
	glucoboy.io_regs[0x23] = glucoboy.good_days;
	glucoboy.io_regs[0x24] = glucoboy.days_until_bonus;
	glucoboy.io_regs[0x25] = glucoboy.hardware_flags;
	glucoboy.io_regs[0x26] = glucoboy.ld_threshold;
	glucoboy.io_regs[0x27] = glucoboy.serial_number;
}

/****** Writes data to Glucoboy I/O ******/
void AGB_MMU::write_glucoboy(u32 address, u8 value)
{
	//Grab new parameters
	if(glucoboy.parameter_length)
	{
		glucoboy.parameters.push_back(value);
		glucoboy.parameter_length--;
		glucoboy.request_interrupt = true;
		glucoboy.reset_shift = true;

		//Write new data to index if necessary
		if(!glucoboy.parameter_length) { process_glucoboy_index(); }
	}

	//Validate new index
	else
	{
		//0x20 - 0x27 : Read glucose and device data
		//0x31 - 0x32 : Flags
		//0x60 - 0x67 : Write glucose and device data
		//0xE0 - 0xE2 : Flags + High Scores
		if(((value >= 0x20) && (value <= 0x27))
		|| ((value >= 0x31) && (value <= 0x32))
		|| ((value >= 0x60) && (value <= 0x67))
		|| ((value >= 0xE0) && (value <= 0xE2)))
		{
			glucoboy.io_index = value;
			glucoboy.request_interrupt = true;
			glucoboy.reset_shift = true;

			//On this index, update Glucoboy with system date
			if(glucoboy.io_index == 0x20)
			{
				time_t system_time = time(0);
				tm* current_time = localtime(&system_time);

				u8 min = current_time->tm_min;
				u8 hour = current_time->tm_hour;
				u8 day = (current_time->tm_mday - 1);
				u8 month = current_time->tm_mon;
				u8 year = (current_time->tm_year % 100);
					
				if(year == 0) { year = 100; }

				glucoboy.io_regs[0x20] = min;
				glucoboy.io_regs[0x20] |= ((hour & 0x3F) << 6);
				glucoboy.io_regs[0x20] |= ((day & 0x1F) << 12);
				glucoboy.io_regs[0x20] |= ((month & 0xF) << 20);
				glucoboy.io_regs[0x20] |= ((year & 0x7F) << 24);
			}

			//Set input length for write indices
			else if((glucoboy.io_index >= 0x60) && (glucoboy.io_index <= 0x67))
			{
				glucoboy.parameters.clear();
				glucoboy.parameter_length = 4;
			}

			else if(glucoboy.io_index == 0xE1)
			{
				glucoboy.parameters.clear();
				glucoboy.parameter_length = 6;
			}
		}

		//Unknown indices, potentially invalid
		else
		{
			glucoboy.io_index = 0;
			glucoboy.request_interrupt = false;
			glucoboy.reset_shift = false;
			std::cout<<"MMU::Unknown Glucoboy Index: 0x" << (u32)value << "\n";
		}
	}

	std::cout<<"GLUCOBOY WRITE -> 0x" << address << " :: 0x" << (u32)value << "\n";
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

	std::cout<<"GLUCOBOY READ -> 0x" << address << " :: 0x" << (u32)result << "\n";

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
		if((glucoboy.io_index >= 0x20) && (glucoboy.io_index <= 0x27))
		{
			glucoboy.index_shift = 24;
		}

		else
		{
			glucoboy.index_shift = 0;
		}

		glucoboy.reset_shift = false;
	}
}

/****** Handles writing input streams to Glucoboy indices ******/
void AGB_MMU::process_glucoboy_index()
{
	u32 input_stream = 0;

	if(!glucoboy.parameters.empty())
	{
		input_stream = (glucoboy.parameters[0] << 24) | (glucoboy.parameters[1] << 16) | (glucoboy.parameters[2] << 8) | (glucoboy.parameters[3]);
	}	

	if((glucoboy.io_index >= 0x60) && (glucoboy.io_index <= 0x67))
	{
		glucoboy.io_regs[glucoboy.io_index - 0x40] = input_stream;
	}
}
