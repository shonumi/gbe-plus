// GB Enhanced+ Copyright Daniel Baxter 2019
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : slot2.cpp
// Date : December 26, 2019
// Description : Slot-2 Device Emulation
//
// Handles emulation of various Slot-2 add-ons and accessories

#include "mmu.h"

u8 NTR_MMU::read_slot2_device(u32 address)
{
	u8 slot_byte = 0xFF;

	switch(current_slot2_device)
	{
		case SLOT2_PASSME:
			if((address & 0x7FFFFFF) < cart_data.size()) { slot_byte = cart_data[address & 0x7FFFFFF]; }
			break;

		case SLOT2_RUMBLE_PAK:
			//Reading is used for detection
			slot_byte = (address & 0x1) ? 0xFF : 0xFD;
			break;
	}

	return slot_byte;
}
