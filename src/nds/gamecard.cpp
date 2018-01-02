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

#include "common/util.h"

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

				init_key_code(1, 8);
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
				nds_card.state = 0x10;
				nds_card.transfer_size = 0x200;

				nds_card.transfer_src = (nds_card.cmd_lo << 8);
				nds_card.transfer_src |= (nds_card.cmd_hi >> 24);
				nds_card.transfer_src &= (cart_data.size() - 1);
			}

			//Get ROM ID 3
			else if((nds_card.cmd_lo >> 24) == 0xB8)
			{
				nds_card.state = 0x30;
				nds_card.transfer_size = 0x4;
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
		nds_card.cnt &= ~0x800000;
		nds_card.cnt &= ~0x80000000;

		if((nds9_exmem & 0x800) == 0) { nds9_if |= 0x80000; }
		else if(nds7_exmem & 0x800) { nds7_if |= 0x80000; }

		nds_card.state = 0;
	}
	
	else { nds_card.transfer_clock = nds_card.baud_rate; }
}

/****** Encrypts 64-bit value via KEY1 ******/
void NTR_MMU::key1_encrypt(u32 &lo, u32 &hi)
{
	u32 x = hi;
	u32 y = lo;
	u32 z = 0;

	for(u32 index = 0; index < 16; index++)
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

	for(u32 index = 17; index > 2; index--)
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
	if(level == 1)
	{
		key_level = 1;
		apply_key_code(mod);
	}

	//TODO - Everything else
}

/****** Applies keycode ******/
void NTR_MMU::apply_key_code(u32 mod)
{
	key1_encrypt(key_code[1], key_code[2]);
	key1_encrypt(key_code[0], key_code[1]);

	u32 temp_lo = 0;
	u32 temp_hi = 0;
}

/****** Reads 4 bytes from the KEY1 table ******/
u32 NTR_MMU::key1_read_u32(u32 index)
{
	return ((key1_table[index + 3] << 24) | (key1_table[index + 2] << 16) | (key1_table[index + 1] << 8) | key1_table[index]);
}

/****** Performs shifted 32-bit reads from keycode ******/
u32 NTR_MMU::key_code_read_u32(u32 index)
{
	u32 result = 0;
	u8 shift = 24;
	index += 3;

	for(u32 x = 0; x < 4; x++)
	{
		switch(index)
		{
			//Byte 1
			case 0x0: result |= ((key_code[0] & 0xFF) << shift); break;
			case 0x1: result |= (((key_code[0] >> 8) & 0xFF) << shift); break;
			case 0x2: result |= (((key_code[0] >> 16) & 0xFF) << shift); break;
			case 0x3: result |= (((key_code[0] >> 24) & 0xFF) << shift); break;

			//Byte 2
			case 0x4: result |= ((key_code[1] & 0xFF) << shift); break;
			case 0x5: result |= (((key_code[1] >> 8) & 0xFF) << shift); break;
			case 0x6: result |= (((key_code[1] >> 16) & 0xFF) << shift); break;
			case 0x7: result |= (((key_code[1] >> 24) & 0xFF) << shift); break;

			//Byte 3
			case 0x8: result |= ((key_code[2] & 0xFF) << shift); break;
			case 0x9: result |= (((key_code[2] >> 8) & 0xFF) << shift); break;
			case 0xA: result |= (((key_code[2] >> 16) & 0xFF) << shift); break;
			case 0xB: result |= (((key_code[2] >> 24) & 0xFF) << shift); break;
		}

		shift -= 8;
		index--;
	}

	return result;
}
