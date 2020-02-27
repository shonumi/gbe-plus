// GB Enhanced+ Copyright Daniel Baxter 2019
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : slot2.cpp
// Date : December 26, 2019
// Description : Slot-2 Device Emulation
//
// Handles emulation of various Slot-2 add-ons and accessories

#include "mmu.h"

/****** Reads from Slot-2 device ******/
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

		case SLOT2_HCV_1000:
			//Reading these cart addresses is for detection
			if(address < 0x8020000)
			{
				u8 data = 0xF0 | ((address & 0x1F) >> 1);
				slot_byte = (address & 0x1) ? 0xFD : data;
			}

			//HCV_CNT
			else if(address == 0xA000000) { slot_byte = hcv.cnt; }

			//HCV_DATA
			else if((address >= 0xA000010) && (address <= 0xA00001F))
			{
				slot_byte = hcv.data[address & 0xF];
				std::cout<<"READING 0x" << std::hex << address << " :: 0x" << (u32)slot_byte << "\n";
			}

			break;
	}

	return slot_byte;
}

/****** Writes to Slot-2 device ******/
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

		case SLOT2_HCV_1000:
			//HCV_CNT
			if(address == 0xA000000) { hcv.cnt = (value & 0x83); }

			break;
	}
}

/****** Reads external barcode file for HCV-1000 ******/
bool NTR_MMU::slot2_hcv_load_barcode(std::string filename)
{
	std::ifstream barcode(filename.c_str(), std::ios::binary);

	if(!barcode.is_open()) 
	{ 
		std::cout<<"MMU::HCV-1000 barcode data could not be read. Check file path or permissions. \n";
		return false;
	}

	//Get file size
	barcode.seekg(0, barcode.end);
	u32 barcode_size = barcode.tellg();
	barcode.seekg(0, barcode.beg);

	hcv.data.clear();
	hcv.data.resize(barcode_size, 0x0);

	u8* ex_data = &hcv.data[0];

	barcode.read((char*)ex_data, barcode_size); 
	barcode.close();

	//Fill empty data with 0x5F
	while(hcv.data.size() < 16) { hcv.data.push_back(0x5F); }

	std::cout<<"SIO::Loaded HCV-1000 barcode data.\n";
	return true;
}
