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
				if(gpio.data & 0x2) { gpio.serial_byte |= (1 << gpio.serial_counter); }
				gpio.serial_counter++;

				//Stop after 8 bits are received
				if(gpio.serial_counter == 8)
				{
					//Validate command
					if((gpio.serial_byte & 0xF) == 6)
					{
						u8 param = ((gpio.serial_byte >> 4) & 0x7);
						bool read_data = gpio.serial_byte & 0x80;
						
						//Change state based on the received parameter
						switch(param)
						{
							//Force Reset
							case 0x0:
								gpio.state = 0x100;
								break;

							//Alarm Time 1 - does nothing on the GBA
							case 0x1:
								//Reply with 0xFF
								if(read_data) { }
								break;

							//Date+Time
							case 0x2:
								//Reply with 7 bytes for date+time
								if(read_data) { }

								//Receive 7 bytes for date+time
								else { }
								break;

							//Clock Adjust Register-Force IRQ
							case 0x3: break;

							//Control Register
							case 0x4: 
								//Reply with 1 byte for control register
								if(read_data) { }

								//Write 1 byte for control register
								else { }
								break;

							//Alarm Time 2 - does nothing on the GBA
							case 0x5:
								//Reply with 0xFF
								if(read_data) { }
								break;

							//Time
							case 0x6: 
								//Reply with 3 bytes for time
								if(read_data) { }

								//Write 3 bytes for time
								else { }
								break;

							//Free register - does nothing on the GBA
							case 0x7:
								//Reply with 0xFF
								if(read_data) { }
								break;
						}
					}
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
