// GB Enhanced Copyright Daniel Baxter 2019
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : huc3.cpp
// Date : January 16, 2019
// Description : Game Boy HuC-3 I/O handling
//
// Handles reading and writing bytes to memory locations for HuC-3
// Used to switch ROM and RAM banks in HuC-3

#include "mmu.h"

/****** Performs write operations specific to the HuC-3 ******/
void DMG_MMU::huc3_write(u16 address, u8 value)
{
	//Write to External RAM or control IR signal
	if((address >= 0xA000) && (address <= 0xBFFF))
	{
		//Handle IR signals via networking
		if(ir_stat.trigger)
		{
			//IR Type must be specified as a HuC IR cart!
			if(config::cart_type == DMG_HUC_IR)
			{
				ir_stat.signal = (value & 0x1);
				ir_stat.send = true;

				//Start cycle counting if using GB KISS LINK
				if((ir_stat.signal) && (sio_stat->ir_type == GB_KISS_LINK) && (!kiss_link.is_locked))
				{
					if(kiss_link.cycles)
					{
						kiss_link.input_signals.push_back(kiss_link.cycles);
						bool restart = false;

						if((kiss_link.stage == GKL_INIT_RECEIVER) || (kiss_link.stage == GKL_ACK_NEW_SESSION))
						{
							restart = true;
						}

						if(((kiss_link.cycles / 100) == 98) && (restart))
						{
							kiss_link.state = GKL_RECV_HANDSHAKE_AA;
							kiss_link.input_signals.clear();
						}
					}

					kiss_link.cycles = 0;

					//Set generic RECV state if necessary
					if((kiss_link.state != GKL_RECV_HANDSHAKE_55) && (kiss_link.state != GKL_RECV_HANDSHAKE_3C)
					&& (kiss_link.state != GKL_RECV_HANDSHAKE_AA) && (kiss_link.state != GKL_RECV_HANDSHAKE_C3)
					&& (kiss_link.state != GKL_RECV_PING) && (kiss_link.state != GKL_RECV_DATA)
					&& (kiss_link.state != GKL_RECV_CMD))
					{
						kiss_link.state = GKL_RECV;
					}

					sio_stat->shifts_left = 1;
					sio_stat->shift_counter = 0;
					sio_stat->shift_clock = 0;
				}
			}
		}

		//Set RTC command
		else if(cart.huc_reg_map == 0x0B)
		{
			cart.huc_rtc_cmd = value;
		}

		//Clear RTC semaphore to execute command
		else if(cart.huc_reg_map == 0x0D)
		{
			//GBE+ executes commands instantly, so semaphore should remain unchanged (Bit 0 high)
			if((value & 0x01) == 0)
			{
				huc3_process_command();
			}
		}

		//Otherwise write to RAM if enabled
		else if(ram_banking_enabled) { random_access_bank[bank_bits][address - 0xA000] = value; }
	}

	//Register Map - Maps in RAM, IR, or RTC 
	if(address <= 0x1FFF)
	{
		cart.huc_reg_map = (value & 0xF);

		//Set RAM to read-write or read-only
		if(cart.huc_reg_map == 0x0A) { ram_banking_enabled = true; }
		else { ram_banking_enabled = false; }

		//Enable or disable IR communications
		if(cart.huc_reg_map == 0x0E) { ir_stat.trigger = 1; }
		else { ir_stat.trigger = 0; }
	}

	//MBC register - Select ROM bank
	else if((address >= 0x2000) && (address <= 0x3FFF))
	{
		rom_bank = (value & 0x7F);
		
		//Lowest switchable ROM bank is 1
		if(rom_bank == 0) { rom_bank++; }
	}

	//MBC register - Select RAM bank
	else if((address >= 0x4000) && (address <= 0x5FFF)) { bank_bits = (value & 0x3); }
} 

/****** Performs read operations specific to the HuC-3 ******/
u8 DMG_MMU::huc3_read(u16 address)
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
		else { return memory_map[address]; }
	}

	//Read using RAM Banking
	else if((address >= 0xA000) && (address <= 0xBFFF))
	{
		//Read from IR
		if(ir_stat.trigger) { return 0xC0 | (cart.huc_ir_input); }

		//Read RTC Command/Response
		else if(cart.huc_reg_map == 0x0C) { return cart.huc_rtc_out; }

		//Read RTC Semaphore
		else if(cart.huc_reg_map == 0x0D) { return cart.huc_semaphore; }

		//Otherwise read from RAM - Note, SRAM is always enabled on HuC-3 when selected by register map
		else if((cart.huc_reg_map == 0x00) || (cart.huc_reg_map == 0x0A))
		{
			return random_access_bank[bank_bits][address - 0xA000];
		}
	}

	//For all unhandled reads, attempt to return the value from the memory map
	return memory_map[address];
}

/****** Processes HuC-3 commands for RTC ******/
void DMG_MMU::huc3_process_command()
{
	switch(cart.huc_rtc_cmd >> 4)
	{
		//Read Value + Increment Address
		case 0x01:
			break;

		//Write Value + Increment Address
		case 0x03:
			break;

		//Set addr LO
		case 0x04:
			break;

		//Set addr Hi
		case 0x05:
			break;

		//Extended ops
		case 0x06:
			switch(cart.huc_rtc_cmd & 0x0F)
			{
				//Copy current time to 0x00 - 0x06
				case 0x00:
					break;

				//Copy 0x00 - 0x06 to current time
				case 0x01:
					break;

				//Return RTC status - Always 0x01 because GBE+ is always good to go
				case 0x02:
					cart.huc_rtc_out = 0x01;
					break;

				//Trigger tone generator
				case 0x0E:
					break;
			}

			break;
	}
}
