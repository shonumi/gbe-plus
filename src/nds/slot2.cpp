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
			}

			break;

		case SLOT2_MAGIC_READER:
			switch(address)
			{
				//Two-wire interface to OID
				case 0xA000000:
					slot_byte = magic_reader.out_byte;
					break;

				//Reading these cart addresses is for detection
				default:
					slot_byte = (address & 0x1) ? 0xFB : 0xFF;
					break;
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

		case SLOT2_MAGIC_READER:
			//Konami Magic Reader
			if(address == 0xA000000)
			{
				magic_reader.in_data = value;
				magic_reader_process();
			}

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

/****** Processes data for the Magic Reader ******/
void NTR_MMU::magic_reader_process()
{
	//Reset state
	if(magic_reader.in_data == 0x42)
	{
		magic_reader.state = 0;
		magic_reader.oid_reset = true;
		magic_reader.out_byte = 0xFF;
	}

	//Grab Serial Clock aka SCK from Bit 0
	u8 new_sck = magic_reader.in_data & 0x1;

	//Grab data bit from SDIO aka Bit 1
	u8 sd = (magic_reader.in_data & 0x2) ? 1 : 0;

	//Process Magic Reader I/O
	switch(magic_reader.state)
	{
		//State 0 - Wait for LO to HI transition after reset
		case 0x00:
			//Test for SCK transition from LO to HI
			if((magic_reader.sck == 0) && (new_sck == 1)) { magic_reader.state = 1; }

			//Set SDIO low. Signal to NDS to read OIDCmd_PowerOn
			magic_reader.out_byte = 0xFB;
			break;

		//State 1 - Wait for Write or Read command
		case 0x01:
			if(g_pad->con_flags & 0x100) { magic_reader.out_byte = 0xFB; }

			//SDIO going LO signals new RW phase
			if(sd == 0) { magic_reader.state = 2; }
			break;

		//State 2 - Check RW bit
		case 0x02:
			if((magic_reader.sck == 0) && (new_sck == 1))
			{
				//RW high means write
				if(sd == 1)
				{
					magic_reader.state = 3;
					magic_reader.command = 0;
					magic_reader.counter = 0;
				}

				//RW low means read
				else
				{
					magic_reader.state = 4;
					magic_reader.counter = 0;

					//Return OIDCmd_PowerDown if necessary
					if(magic_reader.oid_reset)
					{
						magic_reader.oid_status = 0x60FFF8;
						magic_reader.oid_reset = false;
					}

					//Return null data, status, or valid index
					magic_reader.index = config::magic_reader_id;
					magic_reader.out_data = (g_pad->con_flags & 0x100) ? magic_reader.index : magic_reader.oid_status;
				}
			}

			break;

		//State 3 - Receive 8 bits from NDS via SD when SCK is HI
		case 0x03:
			//Test for SCK transition from LO to HI
			if((magic_reader.sck == 0) && (new_sck == 1) && (magic_reader.counter < 8))
			{
				//Build command byte MSB first
				if(sd)
				{
					magic_reader.command |= (1 << (7 - magic_reader.counter));
				}

				magic_reader.counter++;
			}

			//Wait for SCK to go low for a while to finish command
			else if((magic_reader.sck == 0) && (new_sck == 0) && (magic_reader.counter == 8))
			{
				magic_reader.state = 1;

				switch(magic_reader.command)
				{
					//Unknown command 0x24
					case 0x24:
						magic_reader.out_byte = 0xFF;
						magic_reader.oid_status = 0x53FFFB;
						break;

					//Disable Auto-Sleep
					case 0xA3:
						magic_reader.out_byte = 0xFF;
						break;

					//Request read status (CheckOIDStatus)
					case 0x30:
						magic_reader.out_byte = 0xFB;
						break;

					//Power Down (PowerDownOID)
					case 0x56:
						magic_reader.out_byte = 0xFB;
						magic_reader.oid_status = 0x60FFF7;
						break;

					default:
						magic_reader.out_byte = 0xFF;
						break;
				}
			}

			break;

		//State 4 - Send 23 bits to NDS via SD when SCK is HI
		case 0x04:
			//Test for SCK transition from LO to HI
			if((magic_reader.sck == 0) && (new_sck == 1) && (magic_reader.counter < 23))
			{
				//Send response MSB first
				u32 test_bit = magic_reader.out_data & (1 << (22 - magic_reader.counter));
				magic_reader.out_byte = (test_bit) ? 0xFF : 0xFB;
				magic_reader.counter++;
			}

			//Wait for SCK to go low for a while to finish command
			else if((magic_reader.sck == 0) && (new_sck == 0) && (magic_reader.counter == 23))
			{
				magic_reader.out_byte = 0xFF;
				magic_reader.state = 1;
			}

			break;
	}

	//Update new SCK in Magic Reader structure
	magic_reader.sck = new_sck;
}
