// GB Enhanced Copyright Daniel Baxter 2019
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : tama5.cpp
// Date : January 21, 2019
// Description : Game Boy TAMA5 I/O handling
//
// Handles reading and writing bytes to memory locations for TAMA5
// Used to switch ROM, EEPROM and RTC

#include <ctime>

#include "mmu.h"
#include "common/util.h"

/****** Performs write operations specific to the TAMA5 ******/
void DMG_MMU::tama5_write(u16 address, u8 value)
{
	//Access TAMA5 registers
	if((address >= 0xA000) && (address <= 0xBFFF) && (cart.ram))
	{
		//Initial 0xA001 write
		if((address == 0xA001) && (value == 0xA) && (!bank_mode))
		{
			cart.tama_reg[0x0A] = 0xF1;
			cart.tama_out = 0xF1;
			bank_mode |= 0x80;
		}

		//Select MBC register
		else if((bank_mode & 0x80) && (address == 0xA001))
		{
			//Store TAMA5 MBC register as single byte in lower halfbyte
			cart.tama_cmd &= ~0xF;
			cart.tama_cmd |= (value & 0xF);

			//Return Data Out for TAMA RAM
			if(cart.tama_cmd == 0x0C)
			{
				u8 ram_addr = (cart.tama_reg[7] << 4) | cart.tama_reg[6];
				cart.tama_reg[0x0C] = (cart.tama_ram[ram_addr] & 0xF);
			}

			else if(cart.tama_cmd == 0x0D)
			{
				u8 ram_addr = (cart.tama_reg[7] << 4) | cart.tama_reg[6];
				cart.tama_reg[0x0D] = (cart.tama_ram[ram_addr] >> 4);
			}
		}

		//Write register data
		else if((bank_mode & 0x80) && (address == 0xA000))
		{
			cart.tama_reg[(cart.tama_cmd) & 0xF] = (value & 0xF);

			switch(cart.tama_cmd & 0xF)
			{
				//ROM Bank Low
				case 0x0:
					rom_bank &= ~0xF;
					rom_bank |= (value & 0xF);
					break;

				//ROM Bank High
				case 0x1:
					rom_bank &= ~0xF0;
					rom_bank |= ((value & 0xF) << 4);
					break;

				//RAM Write, save data and RTC data
				case 0x7:
					{
						u8 ram_val = (cart.tama_reg[5] << 4) | cart.tama_reg[4];
						u8 ram_addr = (cart.tama_reg[7] << 4) | cart.tama_reg[6];

						//Write last latched time to RTC data
						if((ram_addr == 0x08) || (ram_addr == 0x28)) { grab_tama5_time(ram_val); }

						//Latch time and write time to RTC data
						else if((ram_addr == 0x18) || (ram_addr == 0x54) || (ram_addr == 0x64)
						|| (ram_addr == 0x74) || (ram_addr == 0x84) || (ram_addr == 0x94))
						{
							grab_tama5_time(ram_val);
						}

						//Write to TAMA5 RAM
						else if(ram_addr <= 0x10)
						{
							cart.tama_ram[ram_addr] = ram_val;
						}
					}

					break;

				//Constant Value = 0xA1
				case 0xA:
					cart.tama_reg[0x0A] = 0x1;
					break;
			}
		}
	}
}


/****** Performs read operations specific to the TAMA5 ******/
u8 DMG_MMU::tama5_read(u16 address)
{
	//Read using ROM Banking - Bank 0
	if(address <= 0x3FFF) { return memory_map[address]; }

	//Read using ROM Banking
	if((address >= 0x4000) && (address <= 0x7FFF))
	{
		//Read from Banks 2 and above
		if(rom_bank >= 2) 
		{ 
			return read_only_bank[rom_bank - 2][address - 0x4000];
		}

		//When reading from Bank 1, just use the memory map
		return memory_map[address];
	}

	//Access TAMA5 registers
	else if((address >= 0xA000) && (address <= 0xBFFF))
	{
		//Read register data
		if((bank_mode & 0x80) && (address == 0xA000)) { return cart.tama_reg[cart.tama_cmd]; }
		return 0;
	}

	//For all unhandled reads, attempt to return the value from the memory map
	return memory_map[address];
}

/****** Grab current system time for Real-Time Clock ******/
void DMG_MMU::grab_tama5_time(u8 index)
{
	index &= 0xF;

	//Grab local time
	time_t system_time = time(0);
	tm* current_time = localtime(&system_time);

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

	current_time->tm_mon++;
	if((current_time->tm_mon + config::rtc_offset[4]) >= 12) { current_time->tm_year++; }
	current_time->tm_mon += config::rtc_offset[4];
	current_time->tm_mon = (current_time->tm_mon % 12);
	if(current_time->tm_mon == 0) { current_time->tm_mon++; }

	//Set time based on index
	switch(index)
	{
		//Set Seconds LO
		case 0x00: cart.tama_ram[0x18] = (util::get_bcd(current_time->tm_sec) & 0xF); break;
		
		//Set Seconds HI
		case 0x01: cart.tama_ram[0x18] = ((util::get_bcd(current_time->tm_sec) >> 4) & 0xF); break;

		//Set Minutes LO
		case 0x02: cart.tama_ram[0x18] = (util::get_bcd(current_time->tm_min) & 0xF); break;
		
		//Set Minutes HI
		case 0x03: cart.tama_ram[0x18] = ((util::get_bcd(current_time->tm_min) >> 4) & 0xF); break;

		//Set Hours LO
		case 0x04: cart.tama_ram[0x18] = (util::get_bcd(current_time->tm_hour) & 0xF); break;
		
		//Set Hours HI
		case 0x05: cart.tama_ram[0x18] = ((util::get_bcd(current_time->tm_hour) >> 4) & 0xF); break;

		//Set Days LO
		case 0x07: cart.tama_ram[0x18] = (util::get_bcd(current_time->tm_mday) & 0xF); break;
		
		//Set Days HI
		case 0x08: cart.tama_ram[0x18] = ((util::get_bcd(current_time->tm_mday) >> 4) & 0xF); break;

		//Set Months LO
		case 0x09: cart.tama_ram[0x18] = (util::get_bcd(current_time->tm_mon) & 0xF); break;
		
		//Set Months HI
		case 0x0A: cart.tama_ram[0x18] = ((util::get_bcd(current_time->tm_mon) >> 4) & 0xF); break;

		//Set Years - Digit 3
		case 0x0B: cart.tama_ram[0x18] = ((util::get_bcd(current_time->tm_year) >> 4) & 0xF); break;

		//Set Years - Digit 4
		case 0x0C: cart.tama_ram[0x18] = (util::get_bcd(current_time->tm_year) & 0xF); break;
	}

	cart.tama_ram[0x54] = util::get_bcd(current_time->tm_sec);
	cart.tama_ram[0x64] = util::get_bcd(current_time->tm_min);
	cart.tama_ram[0x74] = util::get_bcd(current_time->tm_hour);
	cart.tama_ram[0x84] = util::get_bcd(current_time->tm_mday);
	cart.tama_ram[0x94] = util::get_bcd(current_time->tm_mon);
}