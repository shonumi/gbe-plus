// GB Enhanced+ Copyright Daniel Baxter 2019
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : slot2.cpp
// Date : December 26, 2019
// Description : Slot-2 Device Emulation
//
// Handles emulation of various Slot-2 add-ons and accessories

#include <filesystem>

#include "mmu.h"

#include "common/util.h"

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

		case SLOT2_MEMORY_EXPANSION:
			slot_byte = 0xFF;

			//Reading these addresses is for detection
			if((address >= 0x80000B0) && (address <= 0x80000BF))
			{
				u8 detect_map[16] =
				{
					0xFF, 0xFF, 0x96, 0x00, 0x00, 0x24, 0x24, 0x24,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F,
				};

				slot_byte = detect_map[address & 0x0F];
			}

			//Get lock status
			else if((address >= 0x8240000) && (address <= 0x8240003))
			{
				if(address & 0x3) { slot_byte = 0x00; }
				else { slot_byte = (mem_pak.is_locked) ? 0x01 : 0x00; }
			}

			//8MB RAM located at 0x9000000
			else if((address >> 24) == 9)
			{
				if(mem_pak.is_locked) { slot_byte = 0xFF; }
				else { slot_byte = mem_pak.data[address & 0x7FFFFF]; }
			}

			break;

		case SLOT2_MOTION_PACK:
			//Reading these addresses is for detection
			if(address < 0x8020000)
			{
				u8 data = 0xF0 | ((address & 0x1F) >> 1);
				slot_byte = (address & 0x1) ? 0xFC : data;
			}

			break;

		case SLOT2_FACENING_SCAN:
			//Reading these cart addresses is for detection
			if(address < 0x8020000)
			{
				switch(address)
				{
					case 0x80000B5: slot_byte = 0x25; break;
					case 0x80000BE: slot_byte = 0xFF; break;
					case 0x80000BF: slot_byte = 0x7F; break;
					case 0x801FFFE: slot_byte = 0xFF; break;
					case 0x801FFFF: slot_byte = 0x7F; break;
					default: slot_byte = 0;
				}
			}

			else
			{
				switch(address)
				{
					//I2C Shift Register - Forced to zero here to indicate end of transfer
					case NEON_I2C_SR:
					case NEON_I2C_SR+1:
						slot_byte = 0;
						break;

					default:
						std::cout<<"NTR-014 READ -> 0x" << std::hex << address << "\n";
						slot_byte = 0;
				}
			}

			break;

		case SLOT2_BAYER_DIDGET:
			//Reading these cart addresses is for detection
			if(address < 0x8020000)
			{
				slot_byte = (address & 0x1) ? 0xF3 : 0x00;
			}

			else if(address == DIDGET_CNT)
			{
				//Inital read-out = 0xBD, 0xDA
				//Seems to be the default when no index is specified OR after an index has been fully read
				if(bayer_didget.is_idle)
				{
					slot_byte = (bayer_didget.idle_value & 0xFF);
					bayer_didget.idle_value >>= 8;
				}

				//Read data normally from indices
				else
				{
					std::cout<<"INDEX -> 0x" << u32(bayer_didget.io_index) << "\n";

					//Read messages data
					if((bayer_didget.io_index >= 0x10) && (bayer_didget.io_index <= 0x1F))
					{
						u8 msg_box = bayer_didget.io_index - 0x10;
						slot_byte = bayer_didget.messages[msg_box][bayer_didget.msg_index];

						//Signal end of index reading
						if((bayer_didget.msg_index + 1) >= bayer_didget.messages[msg_box].size())
						{
							process_bayer_didget_irq();
							bayer_didget.is_idle = true;
							bayer_didget.idle_value = 0xDABD;
						}

						//If additional data needs to be read, trigger another IRQ
						else
						{
							process_bayer_didget_irq();
							bayer_didget.msg_index++;
						}
					}

					//Read regular I/O data
					else
					{
						slot_byte = (bayer_didget.io_regs[bayer_didget.io_index] >> bayer_didget.index_shift);

						//Signal end of index reading
						if(!bayer_didget.index_shift)
						{
							bayer_didget.is_idle = true;
							bayer_didget.idle_value = 0xDABD;
						}

						//If additional data needs to be read, trigger another IRQ
						else
						{
							process_bayer_didget_irq();
							bayer_didget.index_shift -= 8;
						}
					}
				}

				std::cout<<"DIDGET READ -> 0x" << std::hex << address << " :: 0x" << u32(slot_byte) << "\n";
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

		case SLOT2_MEMORY_EXPANSION:
			//Set lock status
			if(address == 0x8240000)
			{
				mem_pak.is_locked = (value) ? false : true;
			}

			//8MB RAM located at 0x9000000
			else if(((address >> 24) == 9) && (!mem_pak.is_locked))
			{
				mem_pak.data[address & 0x7FFFFF] = value;
			}

			break;

		case SLOT2_MOTION_PACK:
			break;

		case SLOT2_FACENING_SCAN:
			if(address >= 0x8020000)
			{
				switch(address)
				{
					case NEON_I2C_TX:
						neon.i2c_data = value;
						break;

					case NEON_I2C_TX+1:
						break;

					case NEON_I2C_CNT:
						neon.i2c_cnt = value;

						//Start Transfer
						if((neon.i2c_cnt & 0xF0) == 0x90)
						{
							neon.i2c_transfer.clear();
							neon.i2c_transfer.push_back(neon.i2c_data);
						}

						//Continue Transfer
						else if((neon.i2c_cnt & 0xF0) == 0x10)
						{
							neon.i2c_transfer.push_back(neon.i2c_data);
						}

						//End Transfer
						else if((neon.i2c_cnt & 0xF0) == 0x50)
						{
							neon.i2c_transfer.push_back(neon.i2c_data);

							//Ensure minimum data for transfer was received (4 Bytes)
							if((neon.i2c_transfer.size() >= 4) && (neon.mmap.size() >= 0x10000))
							{
								neon.index = (neon.i2c_transfer[1] << 8 | neon.i2c_transfer[2]);

								std::cout<<"I2C Transfer Complete\n";
								std::cout<<"Transfer Size: " << std::dec << (neon.i2c_transfer.size() - 3) << std::hex << "\n";
								if(neon.i2c_transfer.size() == 4) { std::cout<<"Transfer Data -> 0x" << u32(neon.i2c_transfer[3]) << "\n"; }
								std::cout<<"Transfer Index: 0x" << neon.index << "\n\n";

								for(u32 x = 3; x < neon.i2c_transfer.size(); x++)
								{
									neon_set_stm_register(neon.index, neon.i2c_transfer[x]);
									neon.index++;
								}
							}
						}

						break;

					case NEON_I2C_CNT+1:
						break;

					default:
						std::cout<<"NTR-014 WRITE -> 0x" << std::hex << (u32)address << " :: 0x" << (u32)value << "\n";

				}
			}

			break;

		case SLOT2_BAYER_DIDGET:
			if(address == DIDGET_CNT)
			{
				bayer_didget.is_idle = false;

				//Grab new parameters
				if(bayer_didget.parameter_length)
				{
					bayer_didget.parameters.push_back(value);
					bayer_didget.parameter_length--;
					bayer_didget.request_interrupt = true;
					bayer_didget.reset_shift = true;

					//Write new data to index if necessary
					if(!bayer_didget.parameter_length) { process_bayer_didget_index(); }
				}

				//Validate new index
				else
				{
					//On this read index, update Bayer Didget with system date
					if(value == 0x20)
					{
						time_t system_time = time(0);
						tm* current_time = localtime(&system_time);

						u8 min = current_time->tm_min;
						u8 hour = current_time->tm_hour;
						u8 day = (current_time->tm_mday - 1);
						u8 month = current_time->tm_mon;
						u8 year = (current_time->tm_year % 100);
					
						if(year == 0) { year = 100; }

						bayer_didget.io_regs[0x20] = min;
						bayer_didget.io_regs[0x20] |= ((hour & 0x3F) << 6);
						bayer_didget.io_regs[0x20] |= ((day & 0x1F) << 12);
						bayer_didget.io_regs[0x20] |= ((month & 0xF) << 20);
						bayer_didget.io_regs[0x20] |= ((year & 0x7F) << 24);
					}


					//Validate known indices
					if(((value >= 0x10) && (value <= 0x2D))
					|| ((value >= 0x31) && (value <= 0x32))
					|| (value == 0x6C) || (value == 0xE3))
					{
						bayer_didget.io_index = value;
						bayer_didget.request_interrupt = true;
						bayer_didget.reset_shift = true;
					}

					//Unknown index - May be valid but needs to be debugged
					else
					{
						bayer_didget.io_index = 0;
						bayer_didget.request_interrupt = false;
						bayer_didget.reset_shift = false;
						std::cout<<"MMU::Unknown Bayer Didget Index: 0x" << (u32)value << "\n";
					}

					//Set up parameters length when using write indices
					if(value == 0x6C)
					{
						bayer_didget.parameters.clear();
						bayer_didget.parameter_length = 4;
					}

					else if(value == 0xE3)
					{
						bayer_didget.parameters.clear();
						bayer_didget.parameter_length = 6;
					}
				}

				if(bayer_didget.request_interrupt)
				{
					process_bayer_didget_irq();
				}
			}

			std::cout<<"DIDGET WRITE -> 0x" << std::hex << (u32)address << " :: 0x" << (u32)value << "\n";

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
	u32 barcode_size = std::filesystem::file_size(filename);

	hcv.data.clear();
	hcv.data.resize(barcode_size, 0x0);

	u8* ex_data = &hcv.data[0];

	barcode.read((char*)ex_data, barcode_size); 
	barcode.close();

	//Fill empty data with 0x5F
	while(hcv.data.size() < 16) { hcv.data.push_back(0x5F); }

	std::cout<<"MMU::Loaded HCV-1000 barcode data.\n";
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

/****** Sets the register for STM VL6524 camera ******/
void NTR_MMU::neon_set_stm_register(u16 index, u8 value)
{
	neon.mmap[index] = value;

	switch(index)
	{
		case STM_MICRO_ENABLE:
			if(value == 0x06)
			{
				std::cout<<"MMU::STM VL6524 Clocks Enabled\n";
			}

			break;

		case STM_IO_ENABLE:
			if(value & 0x01)
			{
				std::cout<<"MMU::STM VL6524 IO Enabled\n";
			}

			break;

		case STM_USER_CMD:
			std::cout<<"MMU::STM VL6524 Mode - " << u32(value) << "\n";
			break;
	}
}

/****** Resets Bayer Didget data structure ******/
void NTR_MMU::bayer_didget_reset()
{
	bayer_didget.io_index = 0;
	bayer_didget.io_regs.clear();
	bayer_didget.io_regs.resize(0x100, 0x00);
	bayer_didget.index_shift = 0;
	bayer_didget.request_interrupt = false;
	bayer_didget.reset_shift = false;
	bayer_didget.parameter_length = 0;

	bayer_didget.daily_grps = config::glucoboy_daily_grps;
	bayer_didget.bonus_grps = config::glucoboy_bonus_grps;
	bayer_didget.good_days = config::glucoboy_good_days;
	bayer_didget.days_until_bonus = config::glucoboy_days_until_bonus;
	bayer_didget.total = config::glucoboy_total;
	bayer_didget.hardware_flags = 0;
	bayer_didget.ld_threshold = 0;
	bayer_didget.serial_number = 0;

	//Setup index data
	bayer_didget.io_regs[0x21] = bayer_didget.daily_grps;
	bayer_didget.io_regs[0x22] = bayer_didget.bonus_grps;
	bayer_didget.io_regs[0x23] = bayer_didget.good_days;
	bayer_didget.io_regs[0x24] = bayer_didget.days_until_bonus;
	bayer_didget.io_regs[0x25] = bayer_didget.hardware_flags;
	bayer_didget.io_regs[0x26] = bayer_didget.ld_threshold;
	bayer_didget.io_regs[0x27] = bayer_didget.serial_number;
	bayer_didget.io_regs[0x2C] = bayer_didget.total;

	bayer_didget.is_idle = true;
	bayer_didget.idle_value = 0xDABD;

	bayer_didget.messages.clear();
	bayer_didget.messages.resize(0x10);

	for(int x = 0; x < 0x10; x++)
	{
		bayer_didget.messages[x].resize(0x40, 0x00);

		std::string filename = config::data_path + "didget_messages/msg_" + util::to_str(x) + ".txt";
		std::ifstream file(filename.c_str(), std::ios::binary);

		if(file.is_open()) 
		{
			//Get the file size - Limit to 64 bytes
			u32 file_size = std::filesystem::file_size(filename);

			if(file_size > 0x40)
			{
				file_size = 0x40;
			}

			u8* ex_mem = &bayer_didget.messages[x][0];
			file.read((char*)ex_mem, file_size);
			file.close();
		}
	}
}

/****** Handles Bayer Didget interrupt requests and what data to respond with ******/
void NTR_MMU::process_bayer_didget_irq()
{
	//Trigger Game Pak IRQ
	nds9_if |= 0x2000;
	bayer_didget.request_interrupt = false;

	//Set data size of each index (8-bit or 32-bit)
	if(bayer_didget.reset_shift)
	{
		//4-byte read indices
		if((bayer_didget.io_index >= 0x20) && (bayer_didget.io_index <= 0x2D))
		{
			bayer_didget.index_shift = 24;
		}

		//64-byte page indices
		else if((bayer_didget.io_index >= 0x10) && (bayer_didget.io_index <= 0x1F))
		{
			bayer_didget.msg_index = 0;
		}

		//Unknown index - Do not process
		else
		{
			bayer_didget.index_shift = 0;
		}

		bayer_didget.reset_shift = false;
	}
}

/****** Handles writing input streams to Bayer Didget indices ******/
void NTR_MMU::process_bayer_didget_index()
{
	u32 input_stream = 0;

	if(!bayer_didget.parameters.empty())
	{
		input_stream = (bayer_didget.parameters[0] << 24) | (bayer_didget.parameters[1] << 16) | (bayer_didget.parameters[2] << 8) | (bayer_didget.parameters[3]);
	}	

	switch(bayer_didget.io_index)
	{
		case 0x6C:
			bayer_didget.io_regs[bayer_didget.io_index - 0x40] = input_stream;
			config::glucoboy_total = input_stream;
			break;

		case 0xE3:
			break;
	}
}
