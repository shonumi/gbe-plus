// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mbc3.cpp
// Date : May 11, 2015
// Description : Game Boy Memory Bank Controller 3 I/O handling
//
// Handles reading and writing bytes to memory locations for MBC3
// Used to switch ROM and RAM banks in MBC3
// Also used for RTC functionality if present

#include <ctime>

#include "mmu.h"

/****** Grab current system time for Real-Time Clock ******/
void DMG_MMU::grab_time()
{
	//Grab local time
	time_t system_time = time(0);
	tm* current_time = localtime(&system_time);
	tm last_time;

	bool first_time = true;

	for(int x = 0; x < 9; x++)
	{
		if(cart.rtc_last_time[x]) { first_time = false; }
	}

	if(first_time)
	{
		cart.rtc_last_time[0] = current_time->tm_sec;
		cart.rtc_last_time[1] = current_time->tm_min; 
		cart.rtc_last_time[2] = current_time->tm_hour; 
		cart.rtc_last_time[3] = current_time->tm_mday; 
		cart.rtc_last_time[4] = current_time->tm_mon; 
		cart.rtc_last_time[5] = current_time->tm_year; 
		cart.rtc_last_time[6] = current_time->tm_wday;
		cart.rtc_last_time[7] = current_time->tm_yday; 
		cart.rtc_last_time[8] = current_time->tm_isdst;
		return;
	}

	//Manually restore tm structure for previous time
	last_time.tm_sec = cart.rtc_last_time[0];
	last_time.tm_min = cart.rtc_last_time[1]; 
	last_time.tm_hour = cart.rtc_last_time[2]; 
	last_time.tm_mday = cart.rtc_last_time[3]; 
	last_time.tm_mon = cart.rtc_last_time[4]; 
	last_time.tm_year = cart.rtc_last_time[5]; 
	last_time.tm_wday = cart.rtc_last_time[6];
	last_time.tm_yday = cart.rtc_last_time[7]; 
	last_time.tm_isdst = cart.rtc_last_time[8];

	//Add offsets to system time
	if((current_time->tm_sec + config::rtc_offset[0]) >= 60) { current_time->tm_min++; }
	current_time->tm_sec += config::rtc_offset[0];
	current_time->tm_sec = (current_time->tm_sec % 60);

	if((current_time->tm_min + config::rtc_offset[1]) >= 60) { current_time->tm_hour++; }
	current_time->tm_min += config::rtc_offset[1];
	current_time->tm_min = (current_time->tm_min % 60);

	if((current_time->tm_hour + config::rtc_offset[2]) >= 24) { current_time->tm_mday++; }
	current_time->tm_hour += config::rtc_offset[2];
	current_time->tm_hour = (current_time->tm_hour % 24);

	current_time->tm_mday += config::rtc_offset[3];
	current_time->tm_mday = (current_time->tm_mday % 366);

	//Calculate difference in seconds since last time update
	double time_passed = difftime(mktime(current_time), mktime(&last_time));

	if(time_passed > 0)
	{
		for(int x = 0; x < time_passed; x++)
		{
			cart.rtc_reg[0]++;

			//Update seconds
			if(cart.rtc_reg[0] >= 60)
			{
				cart.rtc_reg[0] = 0;
				cart.rtc_reg[1]++;
			}

			//Update minutes
			if(cart.rtc_reg[1] >= 60)
			{
				cart.rtc_reg[1] = 0;
				cart.rtc_reg[2]++;
			}

			//Update hours
			if(cart.rtc_reg[2] >= 24)
			{
				cart.rtc_reg[2] = 0;

				if((cart.rtc_reg[3] == 0xFF) && (!cart.rtc_reg[4]))
				{
					cart.rtc_reg[3] = 0;
					cart.rtc_reg[4] = 1;
				}

				else if((cart.rtc_reg[3] == 0xFF) && (cart.rtc_reg[4]))
				{
					cart.rtc_reg[3] = 0;
					cart.rtc_reg[4] = 0;
				}

				else { cart.rtc_reg[3]++; }
			}
		}
	}

	else if(time_passed < 0)
	{
		time_passed *= -1;

		for(int x = 0; x < time_passed; x++)
		{
			cart.rtc_reg[0]--;

			//Update seconds
			if(cart.rtc_reg[0] == 0xFF)
			{
				cart.rtc_reg[0] = 59;
				cart.rtc_reg[1]--;
			}

			//Update minutes
			if(cart.rtc_reg[1] == 0xFF)
			{
				cart.rtc_reg[1] = 59;
				cart.rtc_reg[2]--;
			}

			//Update hours
			if(cart.rtc_reg[2] == 0xFF)
			{
				cart.rtc_reg[2] = 23;

				if((!cart.rtc_reg[3]) && (!cart.rtc_reg[4]))
				{
					cart.rtc_reg[3] = 0xFF;
					cart.rtc_reg[4] = 1;
				}

				else if((!cart.rtc_reg[3]) && (cart.rtc_reg[4]))
				{
					cart.rtc_reg[3] = 0xFF;
					cart.rtc_reg[4] = 0;
				}

				else { cart.rtc_reg[3]--; }
			}
		}
	}

	//Manually set new time
	cart.rtc_last_time[0] = current_time->tm_sec;
	cart.rtc_last_time[1] = current_time->tm_min; 
	cart.rtc_last_time[2] = current_time->tm_hour; 
	cart.rtc_last_time[3] = current_time->tm_mday; 
	cart.rtc_last_time[4] = current_time->tm_mon; 
	cart.rtc_last_time[5] = current_time->tm_year; 
	cart.rtc_last_time[6] = current_time->tm_wday;
	cart.rtc_last_time[7] = current_time->tm_yday; 
	cart.rtc_last_time[8] = current_time->tm_isdst;

	for(int x = 0; x < 5; x++) { cart.latch_reg[x] = cart.rtc_reg[x]; }
}

/****** Performs write operations specific to the MBC3 ******/
void DMG_MMU::mbc3_write(u16 address, u8 value)
{
	//Write to External RAM or RTC register
	if((address >= 0xA000) && (address <= 0xBFFF))
	{
		if((ram_banking_enabled) && (bank_bits <= 3) && (config::cart_type != DMG_MBC30)) { random_access_bank[bank_bits][address - 0xA000] = value; }
		else if((ram_banking_enabled) && (bank_bits < 8) && (config::cart_type == DMG_MBC30)) { random_access_bank[bank_bits][address - 0xA000] = value; }
		else if((cart.rtc_enabled) && (bank_bits >= 0x8) && (bank_bits <= 0xC)) { cart.rtc_reg[bank_bits - 8] = value; }
	}

	//MBC register - Enable or Disable RAM Banking - Enable or Disable RTC if present
	if(address <= 0x1FFF)
	{
		if((value & 0xF) == 0xA) 
		{ 
			if(cart.ram) { ram_banking_enabled = true; } 
			if(cart.rtc) { cart.rtc_enabled = true; }
		}
		else { ram_banking_enabled = false; cart.rtc_enabled = false; }
	}

	//MBC register - Select ROM bank - Bits 0 to 6
	if((address >= 0x2000) && (address <= 0x3FFF)) 
	{ 
		rom_bank = (value & 0x7F);
	}

	//MBC register - Select RAM bank or RTC register
	if((address >= 0x4000) && (address <= 0x5FFF)) 
	{ 
		bank_bits = value;
	}

	//Latch current time to RTC regs
	if((address >= 0x6000) && (address <= 0x7FFF))
	{
		if(cart.rtc_enabled)
		{
			//1st latch check
			if((cart.rtc_latch_1 == 0xFF) && (value == 0)) { cart.rtc_latch_1 = 0; }

			//After latch checks pass, grab system time to put into RTC regs
			else if((cart.rtc_latch_2 == 0xFF) && (value == 1)) 
			{
				grab_time();

				//Reset latches
				cart.rtc_latch_1 = cart.rtc_latch_2 = 0xFF;
			}
		}
	}	
}

/****** Performs write operations specific to the MBC3 ******/
u8 DMG_MMU::mbc3_read(u16 address)
{
	//Read using ROM Banking
	if((address >= 0x4000) && (address <= 0x7FFF))
	{
		if(rom_bank >= 2) 
		{ 
			return read_only_bank[rom_bank - 2][address - 0x4000];
		}

		//When reading from Banks 0-1, just use the memory map
		else { return memory_map[address]; }
	}

	//Read using RAM Banking or RTC regs
	else if((address >= 0xA000) && (address <= 0xBFFF))
	{
		if((ram_banking_enabled) && (bank_bits <= 3) && (config::cart_type != DMG_MBC30)) { return random_access_bank[bank_bits][address - 0xA000]; }
		else if((ram_banking_enabled) && (bank_bits < 8) && (config::cart_type == DMG_MBC30)) { return random_access_bank[bank_bits][address - 0xA000]; }
		else if((cart.rtc_enabled) && (bank_bits >= 0x8) && (bank_bits <= 0xC)) { return cart.latch_reg[bank_bits - 8]; }
		else { return 0x00; }
	}
}
