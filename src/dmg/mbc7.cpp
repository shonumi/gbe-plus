// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mbc7.cpp
// Date : December 26, 2015
// Description : Game Boy Memory Bank Controller 7 I/O handling
//
// Handles reading and writing bytes to memory locations for MBC7
// Used to switch ROM and RAM banks in MBC7
// Also used for Gyroscope functionality

#include "mmu.h" 

/****** Performs write operations specific to the MBC7 ******/
void DMG_MMU::mbc7_write(u16 address, u8 value)
{
	//MBC register - Select ROM bank
	if((address >= 0x2000) && (address <= 0x3FFF)) 
	{ 
		rom_bank = (value & 0x7F);
	}

	//MBC register - Select RAM bank
	else if((address >= 0x4000) && (address <= 0x5FFF)) 
	{ 
		bank_bits = value;
	}

	//Write to External RAM
	else if((address >= 0xA000) && (address <= 0xBFFF))
	{
		if(address == 0xA080) { mbc7_write_ram(value); }
	}
}

/****** Performs write operations specific to the MBC7 RAM ******/
void DMG_MMU::mbc7_write_ram(u8 value)
{
	//MBC7 State 0 - Idle state?
	//MBC7 State 1 - Receive MBC7 command
	//MBC7 State 2 - Receive MBC7 address
	//MBC7 State 3 - Process MBC7 command
	// * Command 0 - Write data buffer to MBC7 RAM or enable MBC7 RAM
	// * Command 1 - Reset internal MBC7 register and set to State 5
	// * Command 2 - Read data buffer from MBC7 RAM
	// * Command 3 - Erase data buffer???
	//MBC7 State 4 - Read out data buffer to MBC7 internal value (1 bit at a time)
	//MBC7 State 5 - Write data buffer to MBC7 RAM (when CS flips from 0 to 1)

	//Save the previous Chip Select and SK value
	u8 old_cs = cart.cs;
	u8 old_sk = cart.sk;

	//Update current Chip Select and SK values
	//CS = Bit 7 of the written value
	//SK = Bit 6 of the written value
	cart.cs = value >> 7;
    	cart.sk = (value >> 6) & 1;

	//Check to see if the Chip Select flips from 0 to 1
	if((old_cs == 0) && (cart.cs == 1))
	{
		//If the MBC7 state is 5, reset, and possibly write buffer to RAM
		if(cart.internal_state == 5)
		{
			if(cart.ram)
			{
				//Save the 16-bit buffer to the MBC7's specified memory addresses
				memory_map[0xA000 + (cart.addr * 2)] = cart.buffer >> 8;
				memory_map[0xA000 + (cart.addr * 2) + 1]= cart.buffer & 0xFF;
			}

			//Reset state, set MBC7 internal value to 1
			cart.internal_state = 0;
			cart.internal_value = 1;
		}
		
		//If MBC7 state is something else, reset MBC7 state, idle MBC7 state
		else
		{
			cart.idle = true;
			cart.internal_state = 0;
		}
	}

	//Check to see if the SK flips from 0 to 1
	if((old_sk == 0) && (cart.sk == 1))
	{
		//If MBC7 is idling and it receives a value with Bit 1 set
		//Stop MBC7 idling, set the buffer length to zero, and set the MBC7 state to 1 (to receive commands)
		if((cart.idle) && (value & 0x02))
		{
			cart.idle = false;
			cart.buffer_length = 0;
			cart.internal_state = 1;
		}

		//If MBC7 is not idling, process commands!
		else 
		{
			//Command processing
        		switch(cart.internal_state)
			{
				//Receive command code
        			case 1:
					//Command code is two bits, stored in the buffer.
					//Bit stored is the input's Bit 1
					cart.buffer <<= 1;
					cart.buffer |= (value & 0x02) ? 1 : 0;
					cart.buffer_length++;
					
					//Finish command
					//Once both bits have been received, reset buffer length, set MBC7 state to 2, grab 2 bits as the command code
					if(cart.buffer_length == 2)
					{
						cart.internal_state = 2;
						cart.buffer_length = 0;
						cart.command_code = (cart.buffer & 0x3);
          				}
					
					break;

				//Receive MBC7 specified address
				case 2:
					//Address is 8-bits long
					//Bit stored in the input's Bit 1
					cart.buffer <<= 1;
					cart.buffer |= (value & 0x02) ? 1 : 0;
					cart.buffer_length++;

					//Finish address
					if(cart.buffer_length == 8)
					{
						//Set MBC7 state to 3, reset buffer length, grab address from buffer
						cart.internal_state = 3;
						cart.buffer_length = 0;
						cart.addr = (cart.buffer & 0xFF);

						//If the MBC7 command code is 0, enable or disable RAM writes
						if(cart.command_code == 0)
						{
							//If Bits 6 and 7 of the MBC7 address are empty
							//Disable MBC7 RAM writes, reset MBC7 state to 0
							if((cart.addr >> 6) == 0)
							{
								cart.ram = false;
								cart.internal_state = 0;
							}
							
							//If Bits 6 and 7 of the MBC7 address are set
							//Enable MBC7 writes, reset MBC7 state to 0
							else if((cart.addr >> 6) == 3)
							{
								cart.ram = false;
								cart.internal_state = 0;
							}
						}
					}

					break;

				//Process additional commands
				case 3:
					//Read one more bit into the buffer
					//Take Bit 1 of the input
					cart.buffer <<= 1;
          				cart.buffer |= (value & 0x02) ? 1:0;
					cart.buffer_length++;

          				switch(cart.command_code)
					{
						//Process Command 0
						//Write data to RAM or enable writes
						case 0:
						
							//Wait until buffer is full at 16-bits
            						if(cart.buffer_length == 16)
							{
								//If Bits 6 and 7 of the MBC7 address are empty
								//Disable MBC7 RAM writes, reset MBC7 state to 0
								if((cart.addr >> 6) == 0)
								{
                							cart.ram = false;
                							cart.internal_state = 0;
								}

								//Check to see if Bit 6 of the MBC7 address is set, and Bit 7 is empty...
              							else if((cart.addr >> 6) == 1)
								{
									//If MBC7 RAM writes are enabled, write buffer data to memory
									if(cart.ram)
									{
										for(u16 x = 0; x < 256; x++)
										{
                    									memory_map[0xA000 + (x * 2)] = cart.buffer >> 8;
                    									memory_map[0xA000 + (x * 2) + 1] = cart.buffer & 0xFF;
                  								}
                							}

									//Set MBC7 state to 5
									cart.internal_state = 5;
								}

								//If Bit 7 of the MBC7 address is set, and Bit 6 is empty...
              							else if((cart.addr >> 6) == 2)
								{
									//If MBC7 RAM writes are enabled, fill memory with 0xFF
                							if(cart.ram)
									{
										for(u16 x = 0; x < 256; x++)
										{
                    									memory_map[0xA000 + (x * 2)] = 0xFF;
                    									memory_map[0xA000 + (x * 2) + 1] = 0xFF;
                  								}
                							}
									
									//Set MBC7 state to 5
									cart.internal_state = 5;
								}
	
								//If Bit 6 and 7 of the MBC7 address is set, enable MBC7 RAM writes, reset MBC7 state to 0
              							else if((cart.addr >> 6) == 3)
								{
									cart.ram = true;
									cart.internal_state = 0;
              							}

							//Reset buffer length 
							cart.buffer_length = 0;
            						}
							
							break;

						//Process Command 1
						//Reset MBC7 internal value and set to MBC7 state 5
						//Do this after filling a 16-bit buffer!
          					case 1:
							if(cart.buffer_length == 16) 
							{
              							cart.buffer_length = 0;
              							cart.internal_state = 5;
								cart.internal_value = 0;
            						}

            						break;

						//Process Command 2
						//Load current MBC7 RAM data @ address into buffer
						case 2:
            						if(cart.buffer_length == 1)
							{
								//Set to state 4 and reset buffer length as well
              							cart.internal_state = 4;
              							cart.buffer_length = 0;
              							cart.buffer = (memory_map[0xA000 + (cart.addr * 2)] << 8) | (memory_map[0xA000 + (cart.addr * 2) + 1]);
            						}
     							
							break;

          					//Process Command 3
						//Set all bits in the buffer
						case 3:
							if(cart.buffer_length == 16)
							{
              							cart.buffer_length = 0;
              							cart.buffer = 0xFFFF;
              							cart.internal_state = 5;
              							cart.internal_value = 0;
            						}
            
							break;
          				}
          					
					break;
			}
      		}
    	}

	//Check to see if the SK flips from 1 to 0
	if((old_sk == 1) && (cart.sk == 0))
	{
		//If MBC7 state is 4 (from Command Code 2)
		if(cart.internal_state == 4)
		{
			//Set the MBC7 internal value to Bit 15 of the buffer
			cart.internal_value = (cart.buffer & 0x8000) ? 1 : 0;
			cart.buffer <<= 1;
			cart.buffer_length++;
			
			//Once all values of the buffer have been shifted out to the MBC7 internal value
			//Reset buffer length, reset MBC7 state to 0
			if(cart.buffer_length == 16)
			{
				cart.buffer_length = 0;
				cart.internal_state = 0;
			}
		}
	}
}

/****** Performs write operations specific to the MBC7 ******/
u8 DMG_MMU::mbc7_read(u16 address)
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

	//Read from RAM or gyroscope
	else if((address >= 0xA000) && (address <= 0xBFFF))
	{
		switch(address & 0xA0F0)
		{
			//Return 0 for invalid reads
			case 0xA000:
			case 0xA010:
			case 0xA060:
			case 0xA070:
				return 0x0;

			//Gyroscope X - Low Byte
			case 0xA020:
				return 0x0;

			//Gyroscope X - High Byte
			case 0xA030:
				return 0x0;

			//Gyroscope Y - Low Byte
			case 0xA040:
				return 0x0;

			//Gyroscope Y - High Byte
			case 0xA050:
				return 0x0;

			//Byte from RAM
			case 0xA080:
				return cart.internal_value;
		
			default:
				return 0x0;
		}
	}
}