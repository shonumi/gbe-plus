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
	}

	//Validate new index
	else
	{
		switch(value)
		{
			//On this index, update Glucoboy with system date
			case 0x20:
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
					
			case 0x21:
			case 0x22:
			case 0x23:
			case 0x24:
			case 0x25:
			case 0x26:
			case 0x27:
			case 0x31:
			case 0x32:
			case 0x60:
			case 0x61:
			case 0x62:
			case 0x63:
			case 0x64:
			case 0x65:
			case 0x66:
			case 0x67:
			case 0xE0:
			case 0xE1:
			case 0xE2:
				glucoboy.io_index = value;
				glucoboy.request_interrupt = true;
				glucoboy.reset_shift = true;
				std::cout<<"INDEX -> 0x" << (u32)glucoboy.io_index << "\n";
				break;

			default:
				glucoboy.io_index = 0;
				glucoboy.request_interrupt = false;
				glucoboy.reset_shift = false;
				std::cout<<"MMU::Unknown Glucoboy Index: 0x" << (u32)value << "\n";
		}

		//Set input length for write indices
		switch(glucoboy.io_index)
		{
			case 0x60:
			case 0x61:
			case 0x62:
			case 0x63:
			case 0x64:
			case 0x65:
			case 0x66:
			case 0x67:
				glucoboy.parameters.clear();
				glucoboy.parameter_length = 4;
				break;

			case 0xE1:
				glucoboy.parameters.clear();
				glucoboy.parameter_length = 6;
				break;
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
		switch(glucoboy.io_index)
		{
			case 0x20:
			case 0x21:
			case 0x22:
			case 0x23:
			case 0x24:
			case 0x25:
			case 0x26:
			case 0x27:
				glucoboy.index_shift = 24;
				break;

			default:
				glucoboy.index_shift = 0;
		}

		glucoboy.reset_shift = false;
	}
}
