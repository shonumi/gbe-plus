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

		case SLOT2_UBISOFT_PEDOMETER:
			//Reading these cart addresses is for detection
			if(address < 0x8020000)
			{
				u8 data = 0xF0 | ((address & 0x1F) >> 1);
				slot_byte = (address & 0x1) ? 0xF7 : data;
			}

			//0xA000000 through 0xA000004 = step values
			//0xA00000C resets step count
			switch(address)
			{
				case 0xA000000: slot_byte = config::utp_steps & 0xF; break;
				case 0xA000001: slot_byte = (config::utp_steps >> 4) & 0xF; break;
				case 0xA000002: slot_byte = (config::utp_steps >> 8) & 0xF; break;
				case 0xA000003: slot_byte = (config::utp_steps >> 12) & 0xF; break;
				case 0xA000004: slot_byte = (config::utp_steps >> 16) & 0xF; break;
				case 0xA00000C: slot_byte = 0; config::utp_steps = 0; break;
			}

			break;
	}

	return slot_byte;
}

void NTR_MMU::write_slot2_device(u32 address, u8 value)
{
	switch(current_slot2_device)
	{
		case SLOT2_RUMBLE_PAK:
			//Turn on rumble for 1 frame (16ms) if rumble state changes
			if(((address == 0x8000000) || (address == 0x8001000)) && (rumble_state != value))
			{
				g_pad->stop_rumble();
				rumble_state = value;
				g_pad->start_rumble(16);
			}

			break;
	}
}
