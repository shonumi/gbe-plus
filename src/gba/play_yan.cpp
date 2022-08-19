// GB Enhanced+ Copyright Daniel Baxter 2022
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : play_yan.cpp
// Date : August 19, 2022
// Description : Nintendo Play-Yan
//
// Handles I/O for the Nintendo Play-Yan
// Manages IRQs and firmware reads/writes

#include "mmu.h"
#include "common/util.h"

/****** Writes to Play-Yan I/O ******/
void AGB_MMU::write_play_yan(u32 address, u8 value)
{

}

/****** Reads from Play-Yan I/O ******/
u8 read_play_yan(u32 address)
{
	return 0;
}
