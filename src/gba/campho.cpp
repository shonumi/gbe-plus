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

	campho.bank_index = 0;
	campho.bank_id = 0;
	campho.rom_stat = 0xA00A;
	campho.rom_cnt = 0;
	campho.block_len = 0;
	campho.block_stat = 0;
	campho.bank_state = 0;
}

/****** Writes data to Campho I/O ******/
void AGB_MMU::write_campho(u32 address, u8 value)
{
	switch(address)
	{
		//Campho ROM Status
		case CAM_ROM_STAT:
			campho.rom_stat &= 0xFF00;
			campho.rom_stat |= value;
			break;

		case CAM_ROM_STAT+1:
			campho.rom_stat &= 0xFF;
			campho.rom_stat |= (value << 8);
			break;

		//Campho ROM Control
		case CAM_ROM_CNT:
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
					campho.bank_index = 0;
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
						campho.bank_state = 1;
					}
				}
			}

			break;
	}
}

/****** Reads data from Campho I/O ******/
u8 AGB_MMU::read_campho(u32 address)
{
	u8 result = 0;

	return result;
}

/****** Reads sequential ROM data from Campho ******/
u8 AGB_MMU::read_campho_seq(u32 address)
{
	u8 result = 0;

	return result;
}
