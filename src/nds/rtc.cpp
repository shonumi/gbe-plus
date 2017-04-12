// GB Enhanced+ Copyright Daniel Baxter 2017
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : rtc.cpp
// Date : April 10, 2017
// Description : NDS Real Time Clock processing
//
// Handles reading and writing to the NDS RTC

#include <ctime>

#include "mmu.h"
#include "common/util.h"

/****** Handles various RTC interactions ******/
void NTR_MMU::process_rtc()
{
	//Trigger command transfer even if in the middle of another operation
	if(((nds7_rtc.data & 0x6) == 0x2) && (nds7_rtc.io_direction & 0x1) && (nds7_rtc.state != 0x100))
	{
		nds7_rtc.state = 0x101;

		//Write data to RTC if necessary
		switch(nds7_rtc.serial_byte)
		{
			//Status 1 Register
			case 0x6: nds7_rtc.regs[0] = nds7_rtc.serial_data[0]; break;

			//Status 2 Register
			case 0x46: nds7_rtc.regs[1] = nds7_rtc.serial_data[1]; break;

			//Alarm Time 1 or Frequency Duty Setting
			case 0x16: break;
		}

		return;
	}

	//Perform actions based on current internal state of serial communications with RTC
	switch(nds7_rtc.state)
	{
		//0x100 - Init serial transfer Stage 1 (CS=lo, SCK=hi)
		case 0x100:
			if(((nds7_rtc.data & 0x6) == 0x2) && (nds7_rtc.io_direction & 0x1)) { nds7_rtc.state++; }
			break;

		//0x101 - Init serial transfer Stage 2 (CS=hi, SCK=hi)
		case 0x101:
			if((nds7_rtc.data & 0x6) == 0x6)
			{
				nds7_rtc.state++;
				nds7_rtc.serial_counter = 0;
				nds7_rtc.serial_byte = 0;
			}

			//Return to Stage 1 if CS is set to low
			else { nds7_rtc.state = 0x100; }

			break;

		//0x102 - Send command byte (SCK=lo)
		case 0x102:
			if(nds7_rtc.data & 0x2)
			{
				//Use Data IO
				if(nds7_rtc.data & 0x1) { nds7_rtc.serial_byte |= (1 << nds7_rtc.serial_counter); }
				else { nds7_rtc.serial_byte &= ~(1 << nds7_rtc.serial_counter); }

				nds7_rtc.serial_counter++;

				//Stop after 8 bits are received
				if(nds7_rtc.serial_counter == 8)
				{
					//Validate command
					if((nds7_rtc.serial_byte & 0xF) == 0x6)
					{
						std::cout<<"COMMAND -> 0x" << (u16)nds7_rtc.serial_byte << "\n";

						u8 param = ((nds7_rtc.serial_byte >> 4) & 0x7);
						bool read_data = (nds7_rtc.serial_byte & 0x80) ? true : false;
						
						if(read_data) { std::cout<<"READ COMMAND 0x" << std::hex << u16(param) << "\n"; }
						else { std::cout<<"WRITE COMMAND 0x" << std::hex << u16(param) << "\n"; }

						//Change state based on the received parameter
						switch(param)
						{
							//Status Register 1
							case 0x0:
								//Reply with 1 byte for register
								if(read_data)
								{
									nds7_rtc.serial_data[0] = nds7_rtc.regs[1];
									nds7_rtc.serial_len = 1;
									nds7_rtc.serial_counter = 0;
									nds7_rtc.data_index = 0;
									nds7_rtc.state = 0x103;
								}

								//Write 1 byte for control register
								else
								{
									nds7_rtc.serial_len = 1;
									nds7_rtc.serial_counter = 0;
									nds7_rtc.data_index = 0;
									nds7_rtc.state = 0x104;
								}
						
								break;

							//Status Register 2
							case 0x4: 
								//Reply with 1 byte for register
								if(read_data)
								{
									nds7_rtc.serial_data[0] = nds7_rtc.regs[1];
									nds7_rtc.serial_len = 1;
									nds7_rtc.serial_counter = 0;
									nds7_rtc.data_index = 0;
									nds7_rtc.state = 0x103;
								}

								//Write 1 byte for control register
								else
								{
									nds7_rtc.serial_len = 1;
									nds7_rtc.serial_counter = 0;
									nds7_rtc.data_index = 0;
									nds7_rtc.state = 0x104;
								}

								break;

							//Date+Time
							case 0x2:
								//Reply with 7 bytes for date+time
								if(read_data)
								{
									nds7_rtc.serial_len = 7;
									nds7_rtc.serial_counter = 0;
									nds7_rtc.data_index = 0;
									nds7_rtc.state = 0x103;
									u8 raw_hours = 0;

									//Grab local time
									time_t system_time = time(0);
									tm* current_time = localtime(&system_time);

									//Year
									nds7_rtc.serial_data[0] = (current_time->tm_year % 100);
									nds7_rtc.serial_data[0] = util::get_bcd(nds7_rtc.serial_data[0]);

									//Month
									nds7_rtc.serial_data[1] = (current_time->tm_mon + 1);
									nds7_rtc.serial_data[1] = util::get_bcd(nds7_rtc.serial_data[1]);

									//Day of month
									nds7_rtc.serial_data[2] = current_time->tm_mday;
									nds7_rtc.serial_data[2] = util::get_bcd(nds7_rtc.serial_data[2]);
									
									//Day of week
									nds7_rtc.serial_data[3] = current_time->tm_wday;
									nds7_rtc.serial_data[3] = util::get_bcd(nds7_rtc.serial_data[3]);

									//Hours
									nds7_rtc.serial_data[4] = current_time->tm_hour;
									nds7_rtc.serial_data[4] += config::rtc_offset[2];
									nds7_rtc.serial_data[4] = (nds7_rtc.regs[0] & 0x2) ? (nds7_rtc.serial_data[4] % 24) : (nds7_rtc.serial_data[4] % 12);
									raw_hours = nds7_rtc.serial_data[4];
									nds7_rtc.serial_data[4] = util::get_bcd(nds7_rtc.serial_data[4]);

									//AM-PM flag
									if((nds7_rtc.regs[0] & 0x40) && (raw_hours >= 12)) { nds7_rtc.serial_data[4] |= 0x40; }
									
									//Minutes
									nds7_rtc.serial_data[5] = current_time->tm_min;
									nds7_rtc.serial_data[5] += config::rtc_offset[1];
									nds7_rtc.serial_data[5] = (nds7_rtc.serial_data[5] % 60);
									nds7_rtc.serial_data[5] = util::get_bcd(nds7_rtc.serial_data[5]);

									//Seconds - Disregard tm_sec's 60 or 61 seconds
									nds7_rtc.serial_data[6] = current_time->tm_sec;
									if(nds7_rtc.serial_data[6] > 59) { nds7_rtc.serial_data[6] = 59; }

									nds7_rtc.serial_data[6] += config::rtc_offset[0];
									nds7_rtc.serial_data[6] = (nds7_rtc.serial_data[6] % 60);
									nds7_rtc.serial_data[6] = util::get_bcd(nds7_rtc.serial_data[6]);
								}

								//Receive 7 bytes for date+time
								else { }
							
								break;

							//Time
							case 0x6: 
								//Reply with 3 bytes for time
								if(read_data)
								{
									nds7_rtc.serial_len = 3;
									nds7_rtc.serial_counter = 0;
									nds7_rtc.data_index = 0;
									nds7_rtc.state = 0x103;
									u8 raw_hours = 0;

									//Grab local time
									time_t system_time = time(0);
									tm* current_time = localtime(&system_time);

									//Hours
									nds7_rtc.serial_data[0] = current_time->tm_hour;
									nds7_rtc.serial_data[0] += config::rtc_offset[2];
									nds7_rtc.serial_data[0] = (nds7_rtc.regs[0] & 0x40) ? (nds7_rtc.serial_data[0] % 24) : (nds7_rtc.serial_data[0] % 12);
									raw_hours = nds7_rtc.serial_data[0];
									nds7_rtc.serial_data[0] = util::get_bcd(nds7_rtc.serial_data[0]);

									//AM-PM flag
									if((nds7_rtc.regs[0] & 0x40) && (raw_hours >= 12)) { nds7_rtc.serial_data[0] |= 0x40; }
	
									//Minutes
									nds7_rtc.serial_data[1] = current_time->tm_min;
									nds7_rtc.serial_data[1] += config::rtc_offset[1];
									nds7_rtc.serial_data[1] = (nds7_rtc.serial_data[1] % 60);
									nds7_rtc.serial_data[1] = util::get_bcd(nds7_rtc.serial_data[1]);

									//Seconds - Disregard tm_sec's 60 or 61 seconds
									nds7_rtc.serial_data[2] = current_time->tm_sec;
									if(nds7_rtc.serial_data[2] > 59) { nds7_rtc.serial_data[2] = 59; }

									nds7_rtc.serial_data[2] += config::rtc_offset[0];
									nds7_rtc.serial_data[2] = (nds7_rtc.serial_data[2] % 60);
									nds7_rtc.serial_data[2] = util::get_bcd(nds7_rtc.serial_data[2]);
								}

								//Write 3 bytes for time
								else { }
							
								break;

							//Alarm Time 1
							case 0x1:
								if(read_data)
								{
									nds7_rtc.serial_data[0] = 0x0;
									nds7_rtc.serial_len = 1;
									nds7_rtc.serial_counter = 0;
									nds7_rtc.data_index = 0;
									nds7_rtc.state = 0x103;
								}

								else
								{
									nds7_rtc.serial_len = 3;
									nds7_rtc.serial_counter = 0;
									nds7_rtc.data_index = 0;
									nds7_rtc.state = 0x104;
								}

								break;

							//Alarm Time 2
							case 0x5:
								if(read_data)
								{
									nds7_rtc.serial_data[0] = 0x0;
									nds7_rtc.serial_len = 1;
									nds7_rtc.serial_counter = 0;
									nds7_rtc.data_index = 0;
									nds7_rtc.state = 0x103;
								}

								else
								{
									nds7_rtc.serial_len = 3;
									nds7_rtc.serial_counter = 0;
									nds7_rtc.data_index = 0;
									nds7_rtc.state = 0x104;
								}

								break;

							//Clock Adjust Register
							case 0x3:
								if(read_data)
								{
									nds7_rtc.serial_data[0] = 0x0;
									nds7_rtc.serial_len = 1;
									nds7_rtc.serial_counter = 0;
									nds7_rtc.data_index = 0;
									nds7_rtc.state = 0x103;
								}

								else
								{
									nds7_rtc.serial_len = 1;
									nds7_rtc.serial_counter = 0;
									nds7_rtc.data_index = 0;
									nds7_rtc.state = 0x104;
								}

								break;

							//Free register
							case 0x7:
								if(read_data)
								{
									nds7_rtc.serial_data[0] = nds7_rtc.regs[3];
									nds7_rtc.serial_len = 1;
									nds7_rtc.serial_counter = 0;
									nds7_rtc.data_index = 0;
									nds7_rtc.state = 0x103;
								}

								else
								{
									nds7_rtc.serial_len = 1;
									nds7_rtc.serial_counter = 0;
									nds7_rtc.data_index = 0;
									nds7_rtc.state = 0x104;
								}

								break;
						}
					}
				}
			}

			break;

		//Reply to NDS with data (SCK=hi)
		case 0x103:
			if(nds7_rtc.data & 0x1)
			{
				//Set or unset Bit 1 of SIO data depending on the serial data being sent back
				if(nds7_rtc.serial_data[nds7_rtc.data_index] & 0x1) { memory_map[NDS_RTC] |= 0x1; }
				else { memory_map[NDS_RTC] &= ~0x1; }

				nds7_rtc.serial_data[nds7_rtc.data_index] >>= 1;
				nds7_rtc.serial_counter++;

				//Once 8-bits have been sent, move onto the next byte in serial data
				if(nds7_rtc.serial_counter == 8)
				{
					nds7_rtc.serial_counter = 0;
					nds7_rtc.data_index++;

					//Once all bytes from serial data have been sent, return to idle mode
					if(nds7_rtc.data_index == nds7_rtc.serial_len) { nds7_rtc.state = 0x100; }
				}
			}

			break;

		//Write data from NDS (SCK=lo)
		case 0x104:
			if((nds7_rtc.data & 0x2) == 0)
			{
				//Set serial data from Bit 1 of SIO
				if(nds7_rtc.data & 0x2) { nds7_rtc.serial_data[nds7_rtc.data_index] |= (1 << nds7_rtc.serial_counter); }
				else { nds7_rtc.serial_data[nds7_rtc.data_index] &= ~(1 << nds7_rtc.serial_counter); }

				nds7_rtc.serial_counter++;

				//Once 8-bits have been received, move onto next byte in serial data
				if(nds7_rtc.serial_counter == 8)
				{
					nds7_rtc.serial_counter = 0;
					nds7_rtc.data_index++;

					//Once all bytes for serial data have been received, return to idle mode
					if(nds7_rtc.data_index == nds7_rtc.serial_len)
					{
						nds7_rtc.state = 0x100;

						//Write data to RTC
						switch(nds7_rtc.serial_byte)
						{
							//Status 1 Register
							case 0x6: nds7_rtc.regs[0] = nds7_rtc.serial_data[0]; break;

							//Status 2 Register
							case 0x46: nds7_rtc.regs[1] = nds7_rtc.serial_data[1]; break;

							//Alarm Time 1 or Frequency Duty Setting
							case 0x16: break;
						}
					}
				}
			}

			break;
	}
} 
