// GB Enhanced+ Copyright Daniel Baxter 2022
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : campho.cpp
// Date : December 28, 2022
// Description : Campho Advance
//
// Handles I/O for the Campho Advance (CAM-001)

#include "mmu.h"
#include "common/util.h"

/****** Resets Campho data structure ******/
void AGB_MMU::campho_reset()
{
	campho.data.clear();
	campho.g_stream.clear();

	campho.bank_index_lo = 0;
	campho.bank_index_hi = 0;
	campho.bank_id = 0;
	campho.rom_stat = 0xA00A;
	campho.rom_cnt = 0;
	campho.block_len = 0;
	campho.block_stat = 0;
	campho.bank_state = 0;
	campho.stream_started = false;
	campho.last_id = 0xFFFFFFFF;

	campho.video_capture_counter = 0;
	campho.video_frame_index = 0;
	campho.video_frame_size = 0;
	campho.capture_video = false;
}

/****** Writes data to Campho I/O ******/
void AGB_MMU::write_campho(u32 address, u8 value)
{
	switch(address)
	{
		//Campho ROM Status
		case CAM_ROM_STAT:
		case CAM_ROM_STAT_B:
			campho.rom_stat &= 0xFF00;
			campho.rom_stat |= value;
			break;

		case CAM_ROM_STAT+1:
		case CAM_ROM_STAT_B+1:
			campho.rom_stat &= 0xFF;
			campho.rom_stat |= (value << 8);

			//Process commands to read Graphics ROM
			if(campho.rom_stat == 0x4015) { campho.stream_started = true; }

			break;

		//Campho ROM Control
		case CAM_ROM_CNT:
		case CAM_ROM_CNT_B:
			campho.rom_cnt &= 0xFF00;
			campho.rom_cnt |= value;
			break;

		case CAM_ROM_CNT+1:
			campho.rom_cnt &= 0xFF;
			campho.rom_cnt |= (value << 8);

			//Detect access to main Program ROM
			if(campho.rom_cnt == 0xA00A)
			{
				//Detect reading of first Program ROM bank
				if(!campho.block_stat)
				{
					campho.block_stat = 0xCC00;
					campho.bank_state = 1;

					std::cout<<"MMU::Campho Reading PROM Bank 0x" << campho.block_stat << "\n";
					u32 prom_bank_id = campho_get_bank_by_id(campho.block_stat);
					campho_set_rom_bank(campho.mapped_bank_id[prom_bank_id], campho.mapped_bank_index[prom_bank_id], false);
				}

				//Increment Program ROM bank
				else
				{
					campho.block_stat++;
					campho.bank_state = 1;

					//Signal end of Program ROM banks
					if(campho.block_stat == 0xCC10)
					{
						campho.block_stat = 0xCD00;
					}

					else if(campho.block_stat < 0xCC10)
					{
						std::cout<<"MMU::Campho Reading PROM Bank 0x" << campho.block_stat << "\n";
						u32 prom_bank_id = campho_get_bank_by_id(campho.block_stat);
						campho_set_rom_bank(campho.mapped_bank_id[prom_bank_id], campho.mapped_bank_index[prom_bank_id], false);
					}
				}
			}

			break;

		//Process Graphics ROM read requests
		case CAM_ROM_CNT_B+1:
			campho.rom_cnt &= 0xFF;
			campho.rom_cnt |= (value << 8);

			if(campho.rom_cnt == 0x4015)
			{
				campho.stream_started = false; 

				//Grab Graphics ROM data based on input from stream
				if((!campho.g_stream.empty()) && (campho.g_stream.size() >= 4))
				{
					u32 pos = campho.g_stream.size() - 4;
					u32 index = (campho.g_stream[pos] | (campho.g_stream[pos+1] << 8) | (campho.g_stream[pos+2] << 16) | (campho.g_stream[pos+3] << 24));
					u16 param_1 = (campho.g_stream[0] | (campho.g_stream[1] << 8));

					if(campho.g_stream.size() == 12)
					{
						u32 param_2 = (campho.g_stream[4] | (campho.g_stream[5] << 8) | (campho.g_stream[6] << 16) | (campho.g_stream[7] << 24));
						campho.last_id = param_2;
						std::cout<<"Graphics ROM ID -> 0x" << param_2 << "\n";

						//Set new Graphics ROM bank
						u32 g_bank_id = campho_get_bank_by_id(param_2, index);

						if(g_bank_id != 0xFFFFFFFF)
						{
							campho_set_rom_bank(campho.mapped_bank_id[g_bank_id], campho.mapped_bank_index[g_bank_id], true);
						}
					}

					else if(campho.g_stream.size() == 6)
					{
						//Set new Graphics ROM bank
						u32 g_bank_id = campho_get_bank_by_id(campho.last_id, index);

						if(g_bank_id != 0xFFFFFFFF)
						{
							campho_set_rom_bank(campho.mapped_bank_id[g_bank_id], campho.mapped_bank_index[g_bank_id], true);
						}
					}

					std::cout<<"Graphics ROM Index -> 0x" << index << "\n";
				}

				campho.g_stream.clear();
			}

			break;

		//Graphics ROM Stream
		case CAM_ROM_DATA_HI_B:
		case CAM_ROM_DATA_HI_B+1:
			if(campho.stream_started) { campho.g_stream.push_back(value); }
			break;
	}

	//std::cout<<"CAMPHO WRITE 0x" << address << " :: 0x" << (u32)value << "\n";
}

/****** Reads data from Campho I/O ******/
u8 AGB_MMU::read_campho(u32 address)
{
	u8 result = 0;

	switch(address)
	{
		//ROM Data Stream
		case CAM_ROM_DATA_LO:
			//Read Program ROM
			if(campho.bank_state)
			{
				//Return STAT LOW on first read
				if(campho.bank_state == 1)
				{
					result = (campho.block_stat & 0xFF);
				}

				//Return LEN LOW on second read
				//These 16-bit values should be fixed (0xFFA), except for last block (zero-length)
				else if(campho.bank_state == 2)
				{
					result = (campho.block_stat == 0xCD00) ? 0x00 : 0xFA;
				}
			}

			//Sequential ROM read
			else { result = read_campho_seq(address); }

			break;

		case CAM_ROM_DATA_LO+1:
			//Read Program ROM
			if(campho.bank_state)
			{
				//Return STAT HIGH on first read
				if(campho.bank_state == 1)
				{
					result = ((campho.block_stat >> 8) & 0xFF);
					campho.bank_state++;
				}

				//Return LEN HIGH on second read
				//These 16-bit values should be fixed (0xFFA), except for last block (zero-length)
				else if(campho.bank_state == 2)
				{
					result = (campho.block_stat == 0xCD00) ? 0x00 : 0x0F;
					campho.bank_state = (campho.block_stat == 0xCD00) ? 0x03 : 0x00;
				}
			}

			//Sequential ROM read
			else { result = read_campho_seq(address); }

			break;
		
		//Campho ROM Status
		case CAM_ROM_STAT:
		case CAM_ROM_STAT_B:
			result = (campho.rom_stat & 0xFF);
			break;

		case CAM_ROM_STAT+1:
		case CAM_ROM_STAT_B+1:
			result = ((campho.rom_stat >> 8) & 0xFF);
			break;

		//Campho ROM Control
		case CAM_ROM_CNT:
		case CAM_ROM_CNT_B:
			result = (campho.rom_cnt & 0xFF);
			break;

		case CAM_ROM_CNT+1:
		case CAM_ROM_CNT_B+1:
			result = ((campho.rom_cnt >> 8) & 0xFF);
			break;

		//Sequential ROM read
		default:
			result = read_campho_seq(address);
	}

	//std::cout<<"CAMPHO READ 0x" << address << " :: 0x" << (u32)result << "\n";
	return result;
}

/****** Reads sequential ROM data from Campho ******/
u8 AGB_MMU::read_campho_seq(u32 address)
{
	u8 result = 0;

	//Read Low ROM Data Stream
	if(address < 0x8008000)
	{
		if((campho.bank_index_lo + 1) < campho.data.size())
		{
			result = campho.data[campho.bank_index_lo++];
		}
	}

	//Read High ROM Data Stream
	else
	{
		if((campho.bank_index_hi + 1) < campho.data.size())
		{
			result = campho.data[campho.bank_index_hi++];
		}
	}

	return result;
}

/****** Sets the absolute position within the Campho ROM for a bank's base address ******/
void AGB_MMU::campho_set_rom_bank(u32 bank, u32 address, bool set_hi_bank)
{
	//Abort if invalid bank is set
	if(bank == 0xFFFFFFFF) { return; }

	//Search all known banks for a specific ID
	for(u32 x = 0; x < campho.mapped_bank_id.size(); x++)
	{
		//Match bank ID and base address
		if((campho.mapped_bank_id[x] == bank) && (campho.mapped_bank_index[x] == address))
		{
			//Set High ROM bank
			if(set_hi_bank) { campho.bank_index_hi = campho.mapped_bank_pos[x]; }

			//Set Low ROM bank
			else { campho.bank_index_lo = campho.mapped_bank_pos[x]; }

			campho.block_len = campho.mapped_bank_len[x];
			return;
		}
	}

	std::cout<<"MMU::Warning - Campho Advance ROM position for Bank 0x" << bank << " @ 0x" << address << " was not found\n";
}

/****** Maps various ROM banks for the Campho ******/
void AGB_MMU::campho_map_rom_banks()
{
	//Currently it is unknown how much ROM data needs to be dumped from the Campho Advance
	//Also, the way the hardware maps data is not entirely understood and it's all over the place
	//Until more research is done, a basic PROVISIONAL mapper is implemented here
	//GBE+ will accept a ROM with a header that with the following data (MSB first!):

	//TOTAL HEADER LENGTH		1st 4 bytes
	//BANK ENTRIES			... rest of the header, see below for Bank Entry format

	//BANK ID			4 bytes
	//BANK INDEX			4 bytes
	//BANK LENGTH IN BYTES		4 bytes

	//Grab ROM header length
	if(campho.data.size() < 4) { return; }

	u32 header_len = (campho.data[0] << 24) | (campho.data[1] << 16) | (campho.data[2] << 8) | campho.data[3];

	u32 rom_pos = 0;
	u32 bank_pos = header_len;

	campho.mapped_bank_id.clear();
	campho.mapped_bank_index.clear();
	campho.mapped_bank_len.clear();
	campho.mapped_bank_pos.clear();

	//Grab bank entries and parse them accordingly
	for(u32 header_index = 4; header_index < header_len;)
	{
		rom_pos = header_index;
		if((rom_pos + 12) >= campho.data.size()) { break; }

		u32 bank_id = (campho.data[rom_pos] << 24) | (campho.data[rom_pos+1] << 16) | (campho.data[rom_pos+2] << 8) | campho.data[rom_pos+3];
		u32 bank_addr = (campho.data[rom_pos+4] << 24) | (campho.data[rom_pos+5] << 16) | (campho.data[rom_pos+6] << 8) | campho.data[rom_pos+7];
		u32 bank_len = (campho.data[rom_pos+8] << 24) | (campho.data[rom_pos+9] << 16) | (campho.data[rom_pos+10] << 8) | campho.data[rom_pos+11];

		campho.mapped_bank_id.push_back(bank_id);
		campho.mapped_bank_index.push_back(bank_addr);
		campho.mapped_bank_len.push_back(bank_len);
		campho.mapped_bank_pos.push_back(bank_pos);

		bank_pos += bank_len;
		header_index += 12;
	}

	//Setup initial BS1 and BS2
	u32 bs1_bank = campho_get_bank_by_id(0x08000000);
	u32 bs2_bank = campho_get_bank_by_id(0x08008000);
	
	campho_set_rom_bank(campho.mapped_bank_id[bs1_bank], campho.mapped_bank_index[bs1_bank], false);
	campho_set_rom_bank(campho.mapped_bank_id[bs2_bank], campho.mapped_bank_index[bs2_bank], true);
}

/****** Returns the internal ROM bank GBE+ needs - Mapped to the Campho Advance's IDs ******/
u32 AGB_MMU::campho_get_bank_by_id(u32 id)
{
	for(u32 x = 0; x < campho.mapped_bank_id.size(); x++)
	{
		if(campho.mapped_bank_id[x] == id) { return x; }
	}

	return 0xFFFFFFFF;
}

/****** Returns the internal ROM bank GBE+ needs - Mapped to the Campho Advance's IDs ******/
u32 AGB_MMU::campho_get_bank_by_id(u32 id, u32 index)
{
	for(u32 x = 0; x < campho.mapped_bank_id.size(); x++)
	{
		if((campho.mapped_bank_id[x] == id) && (campho.mapped_bank_index[x] == index)) { return x; }
	}

	return 0xFFFFFFFF;
}

/****** Processes regular events such as audio/video capture for the Campho Advance ******/
void AGB_MMU::process_campho()
{

}
