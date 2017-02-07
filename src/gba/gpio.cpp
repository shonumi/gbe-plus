// GB Enhanced+ Copyright Daniel Baxter 2017
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gpio.cpp
// Date : February 03, 2017
// Description : Game Boy Advance General Purpose I/O (cart add-ons)
//
// Handles reading and writing GPIO for various specialty carts
// Deals with the RTC (Pokemon games), solar sensor, rumble, gyro sensor, etc

#include "mmu.h"
#include "common/util.h"

//Handles GPIO for the Real-Time Clock
void AGB_MMU::process_rtc()
{
	//Perform actions based on current internal state of serial communications with RTC
	switch(gpio.state)
	{
		//0x100 - Init serial transfer Stage 1 (CS=lo, SCK=hi)
		case 0x100:
			if((gpio.data & 0x5) == 0x1) { gpio.state++; }
			break;

		//0x101 - Init serial transfer Stage 2 (CS=hi, SCK=hi)
		case 0x101:
			if((gpio.data & 0x5) == 0x5)
			{
				gpio.state++;
				gpio.serial_counter = 0;
				gpio.serial_byte = 0;
			}

			break;

		//0x102 - Send command byte (SCK=hi)
		case 0x102:
			if(gpio.data & 0x1)
			{
				//Use SIO (GPIO data Bit 1)
				if(gpio.data & 0x2) { gpio.serial_byte |= (1 << (7 - gpio.serial_counter)); }
				else { gpio.serial_byte &= ~(1 << (7 - gpio.serial_counter)); }

				gpio.serial_counter++;

				//Stop after 8 bits are received
				if(gpio.serial_counter == 8)
				{
					//Validate command
					if((gpio.serial_byte & 0xF0) == 0x60)
					{
						u8 param = ((gpio.serial_byte >> 1) & 0x7);
						bool read_data = gpio.serial_byte & 0x1;
						
						if(read_data) { std::cout<<"READ COMMAND 0x" << std::hex << u16(param) << "\n"; }
						else { std::cout<<"WRITE COMMAND 0x" << std::hex << u16(param) << "\n"; }

						//Change state based on the received parameter
						switch(param)
						{
							//Force Reset
							case 0x0:
								gpio.state = 0x100;
								break;

							//Control Register
							case 0x1: 
								//Reply with 1 byte for control register
								if(read_data)
								{
									gpio.serial_data[0] = gpio.rtc_control;
									gpio.serial_len = 1;
									gpio.serial_counter = 0;
									gpio.data_index = 0;
									gpio.state = 0x103;
								}

								//Write 1 byte for control register
								else
								{
									gpio.serial_len = 1;
									gpio.serial_counter = 0;
									gpio.data_index = 0;
									gpio.state = 0x104;
								}

								break;

							//Date+Time
							case 0x2:
								//Reply with 7 bytes for date+time
								if(read_data)
								{
									gpio.serial_len = 7;
									gpio.serial_counter = 0;
									gpio.data_index = 0;
									gpio.state = 0x103;

									//Grab local time
									time_t system_time = time(0);
									tm* current_time = localtime(&system_time);

									//Year
									gpio.serial_data[0] = (current_time->tm_year % 100);
									gpio.serial_data[0] = util::get_bcd(gpio.serial_data[0]);

									//Month
									gpio.serial_data[1] = (current_time->tm_mon + 1);
									gpio.serial_data[1] = util::get_bcd(gpio.serial_data[1]);

									//Day of month
									gpio.serial_data[2] = current_time->tm_mday;
									gpio.serial_data[2] = util::get_bcd(gpio.serial_data[2]);
									
									//Day of week
									gpio.serial_data[3] = current_time->tm_wday;
									gpio.serial_data[3] = util::get_bcd(gpio.serial_data[3]);

									//Hours
									gpio.serial_data[4] = current_time->tm_hour;
									gpio.serial_data[4] += config::rtc_offset[2];
									gpio.serial_data[4] = (gpio.rtc_control & 0x20) ? (gpio.serial_data[4] % 24) : (gpio.serial_data[4] % 12);
									gpio.serial_data[4] = util::get_bcd(gpio.serial_data[4]);
									
									//Minutes
									gpio.serial_data[5] = current_time->tm_min;
									gpio.serial_data[5] += config::rtc_offset[1];
									gpio.serial_data[5] = (gpio.serial_data[5] % 60);
									gpio.serial_data[5] = util::get_bcd(gpio.serial_data[5]);

									//Seconds - Disregard tm_sec's 60 or 61 seconds
									gpio.serial_data[6] = current_time->tm_sec;
									if(gpio.serial_data[6] > 59) { gpio.serial_data[6] = 59; }

									gpio.serial_data[6] += config::rtc_offset[0];
									gpio.serial_data[6] = (gpio.serial_data[6] % 60);
									gpio.serial_data[6] = util::get_bcd(gpio.serial_data[6]);
								}

								//Receive 7 bytes for date+time
								else { }
								break;

							//Time
							case 0x3: 
								//Reply with 3 bytes for time
								if(read_data)
								{
									gpio.serial_len = 3;
									gpio.serial_counter = 0;
									gpio.data_index = 0;
									gpio.state = 0x103;

									//Grab local time
									time_t system_time = time(0);
									tm* current_time = localtime(&system_time);

									//Hours
									gpio.serial_data[0] = current_time->tm_hour;
									gpio.serial_data[0] += config::rtc_offset[2];
									gpio.serial_data[0] = (gpio.rtc_control & 0x20) ? (gpio.serial_data[0] % 24) : (gpio.serial_data[0] % 12);
									gpio.serial_data[0] = util::get_bcd(gpio.serial_data[0]);
									
									//Minutes
									gpio.serial_data[1] = current_time->tm_min;
									gpio.serial_data[1] += config::rtc_offset[1];
									gpio.serial_data[1] = (gpio.serial_data[1] % 60);
									gpio.serial_data[1] = util::get_bcd(gpio.serial_data[1]);

									//Seconds - Disregard tm_sec's 60 or 61 seconds
									gpio.serial_data[2] = current_time->tm_sec;
									if(gpio.serial_data[2] > 59) { gpio.serial_data[2] = 59; }

									gpio.serial_data[2] += config::rtc_offset[0];
									gpio.serial_data[2] = (gpio.serial_data[2] % 60);
									gpio.serial_data[2] = util::get_bcd(gpio.serial_data[2]);
								}

								//Write 3 bytes for time
								else { }
								break;

							//Alarm Time 1 - does nothing on the GBA
							case 0x4:
								//Reply with 0xFF
								if(read_data)
								{
									gpio.serial_data[0] = 0xFF;
									gpio.serial_len = 1;
									gpio.serial_counter = 0;
									gpio.data_index = 0;
									gpio.state = 0x103;
								}

								break;

							//Alarm Time 2 - does nothing on the GBA
							case 0x5:
								//Reply with 0xFF
								if(read_data)
								{
									gpio.serial_data[0] = 0xFF;
									gpio.serial_len = 1;
									gpio.serial_counter = 0;
									gpio.data_index = 0;
									gpio.state = 0x103;
								}

								break;

							//Clock Adjust Register-Force IRQ
							case 0x6: break;

							//Free register - does nothing on the GBA
							case 0x7:
								//Reply with 0xFF
								if(read_data)
								{
									gpio.serial_data[0] = 0xFF;
									gpio.serial_len = 1;
									gpio.serial_counter = 0;
									gpio.data_index = 0;
									gpio.state = 0x103;
								}

								break;
						}
					}
				}
			}

			break;

		//Reply to GBA with data (SCK=hi)
		case 0x103:
			if(gpio.data & 0x1)
			{
				//Set or unset Bit 1 of GPIO data depending on the serial data being sent back
				if(gpio.serial_data[gpio.data_index] & 0x1) { gpio.data |= 0x2; }
				else { gpio.data &= ~0x2; }

				gpio.serial_data[gpio.data_index] >>= 1;
				gpio.serial_counter++;

				//Once 8-bits have been sent, move onto the next byte in serial data
				if(gpio.serial_counter == 8)
				{
					gpio.serial_counter = 0;
					gpio.data_index++;

					//Once all bytes from serial data have been sent, return to idle mode
					if(gpio.data_index == gpio.serial_len) { gpio.state = 0x100; }
				}
			}

			break;

		//Write data from GBA (SCK=lo)
		case 0x104:
			if((gpio.data & 0x1) == 0)
			{
				//Set serial data from Bit 1 of GPIO
				if(gpio.data & 0x2) { gpio.serial_data[gpio.data_index] |= (1 << gpio.serial_counter); }
				else { gpio.serial_data[gpio.data_index] &= ~(1 << gpio.serial_counter); }

				gpio.serial_counter++;

				//Once 8-bits have been received, move onto next byte in serial data
				if(gpio.serial_counter == 8)
				{
					gpio.serial_counter = 0;
					gpio.data_index++;

					//Once all bytes for serial data have been received, return to idle mode
					if(gpio.data_index == gpio.serial_len) { gpio.state = 0x100; }
				}
			}

			break;
	}
}

//Handles GPIO for the Solar Sensor
void AGB_MMU::process_solar_sensor() { }

//Handles GPIO for Rumble
void AGB_MMU::process_rumble() { }

//Handles GPIO for the Gyro Sensor
void AGB_MMU::process_gyro_sensor() { }
