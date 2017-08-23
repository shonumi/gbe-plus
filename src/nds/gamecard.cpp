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

				key_level = 1;
				init_key_code(key_level, 0);
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

/****** Encrypts 64-bit value via KEY1 ******/
void NTR_MMU::key1_encrypt(u32 &lo, u32 &hi)
{
	u32 x = hi;
	u32 y = lo;
	u32 z = 0;

	for(u32 index = 0; index <= 16; index++)
	{
		z = (key1_read_u32(index << 2) ^ x);
		x = key1_read_u32(0x48 + (((z >> 24) & 0xFF) * 4));
		x = key1_read_u32(0x448 + (((z >> 16) & 0xFF) * 4)) + x;
		x = key1_read_u32(0x848 + (((z >> 8) & 0xFF) * 4)) ^ x;
		x = key1_read_u32(0xC48 + ((z & 0xFF) * 4)) + x;
		x = (y ^ x);
		y = z;
	}

	lo = (x ^ key1_read_u32(0x40));
	hi = (y ^ key1_read_u32(0x44));
}

/****** Decrypts 64-bit value via KEY1 ******/
void NTR_MMU::key1_decrypt(u32 &lo, u32 &hi)
{
	u32 x = hi;
	u32 y = lo;
	u32 z = 0;

	for(u32 index = 17; index >= 2; index--)
	{
		z = (key1_read_u32(index << 2) ^ x);
		x = key1_read_u32(0x48 + (((z >> 24) & 0xFF) * 4));
		x = key1_read_u32(0x448 + (((z >> 16) & 0xFF) * 4)) + x;
		x = key1_read_u32(0x848 + (((z >> 8) & 0xFF) * 4)) ^ x;
		x = key1_read_u32(0xC48 + ((z & 0xFF) * 4)) + x;
		x = (y ^ x);
		y = z;
	}

	lo = (x ^ key1_read_u32(4));
	hi = (y ^ key1_read_u32(0));
}

/****** Initializes keycode ******/
void NTR_MMU::init_key_code(u8 level, u32 mod)
{
	//Copy KEY1 table
	key1_table.clear();
	for(u32 x = 0; x < 0x1048; x++) { key1_table.push_back(nds7_bios[0x30 + x]); }

	key_code.clear();
	key_code.push_back(key_id);
	key_code.push_back(key_id / 2);
	key_code.push_back(key_id * 2);

	//Init Level 1
	if(level == 1) { }

	//TODO - Everything else
}

/****** Reads 4 bytes from the KEY1 table ******/
u32 NTR_MMU::key1_read_u32(u32 index)
{
	return ((key1_table[index + 3] << 24) | (key1_table[index + 2] << 16) | (key1_table[index + 1] << 8) | key1_table[index]);
}
