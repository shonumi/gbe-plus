// GB Enhanced+ Copyright Daniel Baxter 2017
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gamecard.cpp
// Date : August 22, 2017
// Description : NDS gamecard I/O handling
//
// Reads and writes to cartridge backup
// Reads from the ROM and handles encryption and decryption

#include "mmu.h"

/****** Handles various cartridge bus interactions ******/
void NTR_MMU::process_card_bus()
{
	//Process various card bus state
	switch(nds_card.state)
	{
		//Grab command
		case 0:
			//Read Header -> 0x00000000000000000000000000000000
			if(!nds_card.cmd_hi && !nds_card.cmd_lo)
			{
				nds_card.state = 0x10;

				if(nds_card.last_state != 0x10)
				{
					nds_card.transfer_src = 0;
					nds_card.last_state = 0x10;
				}

				else
				{
					nds_card.transfer_src += 0x200;
					nds_card.transfer_src &= 0xFFF;
				}

				nds_card.transfer_size = 0x200;
			}

			//Dummy
			else if((!nds_card.cmd_hi) && (nds_card.cmd_lo == 0x9F000000))
			{
				nds_card.state = 0x20;
				nds_card.transfer_size = 0x2000;
			}

			//Get ROM chip ID 1
			else if((!nds_card.cmd_hi) && (nds_card.cmd_lo == 0x90000000))
			{
				nds_card.state = 0x30;
				nds_card.transfer_size = 0x4;
			}

			//Activate Key 1 Encryption
			else if((nds_card.cmd_lo >> 24) == 0x3C)
			{
				nds_card.state = 0x40;
				nds_card.transfer_size = 0x4;
			}

			//Activate Key 2 Encryption
			else if((nds_card.cmd_lo >> 28) == 0x4)
			{
				std::cout<<"MMU::Game Card Bus Activate Key 2 Encryption Command (STUBBED)\n";
			}

			//Get ROM chip ID 2
			else if((nds_card.cmd_lo >> 28) == 0x1)
			{
				std::cout<<"MMU::Game Card Bus Chip ID 2 Command (STUBBED)\n";
			}


			//Get Secure Area Block
			else if((nds_card.cmd_lo >> 28) == 0x2)
			{
				std::cout<<"MMU::Game Card Bus Get Secure Area Block Command (STUBBED)\n";
			}


			//Disable Key 2
			else if((nds_card.cmd_lo >> 28) == 0x6)
			{
				std::cout<<"MMU::Game Card Bus Disable Key 2 Command (STUBBED)\n";
			}

			//Enter Main Data Mode
			else if((nds_card.cmd_lo >> 28) == 0xA)
			{
				std::cout<<"MMU::Game Card Bus Enter Main Data Mode Command (STUBBED)\n";
			}

			//Get Data
			else if((nds_card.cmd_lo >> 24) == 0xB7)
			{
				std::cout<<"MMU::Game Card Bus Get Data Command (STUBBED)\n";
			}

			//Get ROM ID 3
			else if((nds_card.cmd_lo >> 24) == 0xB8)
			{
				std::cout<<"MMU::Game Card Bus Get Chip ID 3 (STUBBED)\n";
			}

			else
			{
				std::cout<<"MMU::Warning - Unexpected command to game card bus -> 0x" << nds_card.cmd_lo << nds_card.cmd_hi << "\n";
			}

			break;

	}

	//Perform transfer
	for(u32 x = 0; x < 4; x++)
	{
		switch(nds_card.state)
		{
			//Dummy
			//Activate Key 1 Encryption
			case 0x20:
			case 0x40:
				memory_map[NDS_CARD_DATA + x] = 0xFF;
				break;

			//1st ROM Chip ID
			case 0x30:
				memory_map[NDS_CARD_DATA + x] = (nds_card.chip_id >> (x * 8)) & 0xFF;
				break;

			//Normal Transfer
			default:
				memory_map[NDS_CARD_DATA + x] = cart_data[nds_card.transfer_src++];
		}
	}

	//Prepare for next transfer, if any
	nds_card.transfer_size -= 4;

	//Finish card transfer, raise IRQ if necessary
	if(!nds_card.transfer_size)
	{
		nds_card.active_transfer = false;
		nds_card.cnt |= 0x800000;
		nds_card.cnt &= ~0x80000000;

		if((nds9_exmem & 0x800) == 0) { nds9_if |= 0x80000; }
		else if(nds7_exmem & 0x800) { nds7_if |= 0x80000; }
	}
	
	else { nds_card.transfer_clock = nds_card.baud_rate; }
}

/****** Decrypts 64-bit game card command ******/
void NTR_MMU::decrypt_card_command()
{

}

/****** Initializes keycode ******/
void NTR_MMU::init_key_code(u32 id, u8 level, u32 mod)
{

}