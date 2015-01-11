// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mmu.h
// Date : April 22, 2014
// Description : Game Boy Advance memory manager unit
//
// Handles reading and writing bytes to memory locations

#include "mmu.h"

/****** MMU Constructor ******/
MMU::MMU() 
{
	reset();
}

/****** MMU Deconstructor ******/
MMU::~MMU() 
{ 
	memory_map.clear();
	std::cout<<"MMU::Shutdown\n"; 
}

/****** MMU Reset ******/
void MMU::reset()
{
	memory_map.clear();
	memory_map.resize(0x10000000, 0);

	eeprom.data.clear();
	eeprom.data.resize(0x200, 0);
	eeprom.size = 0x200;
	eeprom.size_lock = false;

	//HLE stuff
	memory_map[DISPCNT] = 0x80;
	write_u16(0x4000134, 0x8000);

	bios_lock = false;

	write_u32(0x18, 0xEA000042);
	write_u32(0x128, 0xE92D500F);
	write_u32(0x12C, 0xE3A00301);
	write_u32(0x130, 0xE28FE000);
	write_u32(0x134, 0xE510F004);
	write_u32(0x138, 0xE8BD500F);
	write_u32(0x13C, 0xE25EF004);

	bios_lock = true;

	//Default memory access timings (4, 2)
	n_clock = 4;
	s_clock = 2;

	dma[0].enable = dma[1].enable = dma[2].enable = dma[3].enable = false;
	dma[0].started = dma[1].started = dma[2].started = dma[3].started = false;

	lcd_updates.oam_update = false;
	lcd_updates.oam_update_list.resize(128, false);

	lcd_updates.bg_pal_update = true;
	lcd_updates.bg_pal_update_list.resize(256, true);

	lcd_updates.obj_pal_update = true;
	lcd_updates.obj_pal_update_list.resize(256, true);

	lcd_updates.bg_offset_update = false;
	lcd_updates.bg_params_update = true;

	current_save_type = NONE;

	g_pad = NULL;
	timer = NULL;

	std::cout<<"MMU::Initialized\n";
}

/****** Read byte from memory ******/
u8 MMU::read_u8(u32 address) const
{
	//Check for unused memory first
	if(address >= 0x10000000) { std::cout<<"Out of bounds read : 0x" << std::hex << address << "\n"; return 0; }

	switch(address)
	{
		case TM0CNT_L:
			return (timer->at(0).counter & 0xFF);
			break;

		case TM0CNT_L+1:
			return (timer->at(0).counter >> 8);
			break;

		case TM1CNT_L:
			return (timer->at(1).counter & 0xFF);
			break;

		case TM1CNT_L+1:
			return (timer->at(1).counter >> 8);
			break;

		case TM2CNT_L:
			return (timer->at(2).counter & 0xFF);
			break;

		case TM2CNT_L+1:
			return (timer->at(2).counter >> 8);
			break;

		case TM3CNT_L:
			return (timer->at(3).counter & 0xFF);
			break;

		case TM3CNT_L+1:
			return (timer->at(3).counter >> 8);
			break;

		case KEYINPUT:
			return (g_pad->key_input & 0xFF);
			break;

		case KEYINPUT+1:
			return (g_pad->key_input >> 8);
			break;
		
		default:
			return memory_map[address];
	}
}

/****** Read 2 bytes from memory ******/
u16 MMU::read_u16(u32 address) const
{
	return ((read_u8(address+1) << 8) | read_u8(address)); 
}

/****** Read 4 bytes from memory ******/
u32 MMU::read_u32(u32 address) const
{
	return ((read_u8(address+3) << 24) | (read_u8(address+2) << 16) | (read_u8(address+1) << 8) | read_u8(address));
}

/****** Reads 2 bytes from memory - No checks done on the read, used for known memory locations such as registers ******/
u16 MMU::read_u16_fast(u32 address) const
{
	return ((memory_map[address+1] << 8) | memory_map[address]);
}

/****** Reads 4 bytes from memory - No checks done on the read, used for known memory locations such as registers ******/
u32 MMU::read_u32_fast(u32 address) const
{
	return ((memory_map[address+3] << 24) | (memory_map[address+2] << 16) | (memory_map[address+1] << 8) | memory_map[address]);
}

/****** Write byte into memory ******/
void MMU::write_u8(u32 address, u8 value)
{
	//Check for unused memory first
	if(address >= 0x10000000) { std::cout<<"Out of bounds write : 0x" << std::hex << address << "\n"; return; }

	//BIOS is read-only, prevent any attempted writes
	else if((address <= 0x3FF) && (bios_lock)) { return; }

	switch(address)
	{
		case REG_IF:
		case REG_IF+1:
			memory_map[address] &= ~value;
			break;

		case DMA0CNT_H:
		case DMA0CNT_H+1:
			dma[0].enable = true;
			dma[0].started = false;
			dma[0].delay = 2;

			memory_map[address] = value;
			break;

		case DMA1CNT_H:
			//Start DMA1 transfer if Bit 15 goes from 0 to 1
			if((value & 0x80) && ((memory_map[DMA1CNT_H] & 0x80) == 0))
			{
				dma[1].enable = true;
				dma[1].started = false;
				dma[1].delay = 2;
			}

			//Halt DMA1 transfer if Bit 15 goes from 1 to 0
			else if(((value & 0x80) == 0) && (memory_map[DMA1CNT_H] & 0x80)) { dma[1].enable = false; }

			memory_map[DMA1CNT_H] = value;
			break;

		case DMA2CNT_H:
			//Start DMA2 transfer if Bit 15 goes from 0 to 1
			if((value & 0x80) && ((memory_map[DMA2CNT_H] & 0x80) == 0))
			{
				dma[2].enable = true;
				dma[2].started = false;
				dma[2].delay = 2;
			}

			//Halt DMA2 transfer if Bit 15 goes from 1 to 0
			else if(((value & 0x80) == 0) && (memory_map[DMA2CNT_H] & 0x80)) { dma[2].enable = false; }

			memory_map[DMA2CNT_H] = value;
			break;

		case DMA3CNT_H:
		case DMA3CNT_H+1:
			dma[3].enable = true;
			dma[3].started = false;
			dma[3].delay = 2;

			memory_map[address] = value;
			break;

		case KEYINPUT: break;

		case TM0CNT_L:
		case TM0CNT_L+1:
			memory_map[address] = value;
			timer->at(0).reload_value = ((memory_map[TM0CNT_L+1] << 8) | memory_map[TM0CNT_L]);
			break;

		case TM1CNT_L:
		case TM1CNT_L+1:
			memory_map[address] = value;
			timer->at(1).reload_value = ((memory_map[TM1CNT_L+1] << 8) | memory_map[TM1CNT_L]);
			break;

		case TM2CNT_L:
		case TM2CNT_L+1:
			memory_map[address] = value;
			timer->at(2).reload_value = ((memory_map[TM2CNT_L+1] << 8) | memory_map[TM2CNT_L]);
			break;

		case TM3CNT_L:
		case TM3CNT_L+1:
			memory_map[address] = value;
			timer->at(3).reload_value = ((memory_map[TM3CNT_L+1] << 8) | memory_map[TM3CNT_L]);
			break;

		case TM0CNT_H:
		case TM0CNT_H+1:
			memory_map[address] = value;

			switch(memory_map[TM0CNT_H] & 0x3)
			{
				case 0x0: timer->at(0).prescalar = 1; break;
				case 0x1: timer->at(0).prescalar = 64; break;
				case 0x2: timer->at(0).prescalar = 256; break;
				case 0x3: timer->at(0).prescalar = 1024; break;
			}

			timer->at(0).enable = (memory_map[TM0CNT_H] & 0x80) ?  true : false;
			break;

		case TM1CNT_H:
		case TM1CNT_H+1:
			memory_map[address] = value;

			switch(memory_map[TM1CNT_H] & 0x3)
			{
				case 0x0: timer->at(1).prescalar = 1; break;
				case 0x1: timer->at(1).prescalar = 64; break;
				case 0x2: timer->at(1).prescalar = 256; break;
				case 0x3: timer->at(1).prescalar = 1024; break;
			}

			timer->at(1).enable = (memory_map[TM1CNT_H] & 0x80) ?  true : false;
			break;

		case TM2CNT_H:
		case TM2CNT_H+1:
			memory_map[address] = value;

			switch(memory_map[TM2CNT_H] & 0x3)
			{
				case 0x0: timer->at(2).prescalar = 1; break;
				case 0x1: timer->at(2).prescalar = 64; break;
				case 0x2: timer->at(2).prescalar = 256; break;
				case 0x3: timer->at(2).prescalar = 1024; break;
			}

			timer->at(2).enable = (memory_map[TM2CNT_H] & 0x80) ?  true : false;
			break;

		case TM3CNT_H:
		case TM3CNT_H+1:
			memory_map[address] = value;

			switch(memory_map[TM3CNT_H] & 0x3)
			{
				case 0x0: timer->at(3).prescalar = 1; break;
				case 0x1: timer->at(3).prescalar = 64; break;
				case 0x2: timer->at(3).prescalar = 256; break;
				case 0x3: timer->at(3).prescalar = 1024; break;
			}

			timer->at(3).enable = (memory_map[TM3CNT_H] & 0x80) ?  true : false;
			break;

		case WAITCNT:
		case WAITCNT+1:
			{
				memory_map[address] = value;
				u16 wait_control = read_u16(WAITCNT);

				//Determine first access cycles (Non-Sequential)
				switch((wait_control >> 2) & 0x3)
				{
					case 0x0: n_clock = 4; break;
					case 0x1: n_clock = 3; break;
					case 0x2: n_clock = 2; break;
					case 0x3: n_clock = 8; break;
				}

				//Determine second access cycles (Sequential)
				switch((wait_control >> 4) & 0x1)
				{
					case 0x0: s_clock = 2; break;
					case 0x1: s_clock = 1; break;
				}
			}

			break;

		default:
			memory_map[address] = value;
	}

	//Mirror memory from 0x3007FXX to 0x3FFFFXX
	if((address >= 0x3007F00) && (address <= 0x3007FFF)) 
	{
		u32 mirror_addr = 0x03FFFF00 + (address & 0xFF);
		memory_map[mirror_addr] = value;
	}

	//Trigger BG offset update in LCD
	else if((address >= 0x4000010) && (address <= 0x400001F))
	{
		lcd_updates.bg_offset_update = true;
	}

	//Trigger BG scaling+rotation parameter update in LCD
	else if((address >= 0x4000020) && (address <= 0x400003F))
	{
		lcd_updates.bg_params_update = true;
	}

	//Trigger BG palette update in LCD
	else if((address >= 0x5000000) && (address <= 0x50001FF))
	{
		lcd_updates.bg_pal_update = true;
		lcd_updates.bg_pal_update_list[(address & 0x1FF) >> 1] = true;
	}

	//Trigger OBJ palette update in LCD
	else if((address >= 0x5000200) && (address <= 0x50003FF))
	{
		lcd_updates.obj_pal_update = true;
		lcd_updates.obj_pal_update_list[(address & 0x1FF) >> 1] = true;
	}

	//Trigger OAM update in LCD
	else if((address >= 0x07000000) && (address <= 0x070003FF))
	{
		lcd_updates.oam_update = true;
		lcd_updates.oam_update_list[(address & 0x3FF) >> 3] = true;
	}
}

/****** Write 2 bytes into memory ******/
void MMU::write_u16(u32 address, u16 value)
{
	write_u8(address, (value & 0xFF));
	write_u8((address+1), ((value >> 8) & 0xFF));
}

/****** Write 4 bytes into memory ******/
void MMU::write_u32(u32 address, u32 value)
{
	write_u8(address, (value & 0xFF));
	write_u8((address+1), ((value >> 8) & 0xFF));
	write_u8((address+2), ((value >> 16) & 0xFF));
	write_u8((address+3), ((value >> 24) & 0xFF));
}

/****** Writes 2 bytes into memory - No checks done on the read, used for known memory locations such as registers ******/
void MMU::write_u16_fast(u32 address, u16 value)
{
	memory_map[address] = (value & 0xFF);
	memory_map[address+1] = ((value >> 8) & 0xFF);
}

/****** Writes 4 bytes into memory - No checks done on the read, used for known memory locations such as registers ******/
void MMU::write_u32_fast(u32 address, u32 value)
{
	memory_map[address] = (value & 0xFF);
	memory_map[address+1] = ((value >> 8) & 0xFF);
	memory_map[address+2] = ((value >> 16) & 0xFF);
	memory_map[address+3] = ((value >> 24) & 0xFF);
}	

/****** Read binary file to memory ******/
bool MMU::read_file(std::string filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary);

	if(!file.is_open()) 
	{
		std::cout<<"MMU::" << filename << " could not be opened. Check file path or permissions. \n";
		return false;
	}

	//Get the file size
	file.seekg(0, file.end);
	u32 file_size = file.tellg();
	file.seekg(0, file.beg);

	u8* ex_mem = &memory_map[0x8000000];

	//Read data from the ROM file
	file.read((char*)ex_mem, file_size);

	file.close();
	std::cout<<"MMU::" << filename << " loaded successfully. \n";

	std::string backup_file = filename + ".sav";

	//Try to auto-detect save-type, if any
	for(u32 x = 0x8000000; x < (0x8000000 + file_size); x+=4)
	{
		switch(memory_map[x])
		{
			//EEPROM
			case 0x45:
				if((memory_map[x+1] == 0x45) && (memory_map[x+2] == 0x50) && (memory_map[x+3] == 0x52) && (memory_map[x+4] == 0x4F) && (memory_map[x+5] == 0x4D))
				{
					std::cout<<"MMU::EEPROM save type detected\n";
					current_save_type = EEPROM;
					load_backup(backup_file);
					return true;
				}
				
				break;

			//FLASH RAM
			case 0x46:
				if((memory_map[x+1] == 0x4C) && (memory_map[x+2] == 0x41) && (memory_map[x+3] == 0x53) && (memory_map[x+4] == 0x48))
				{
					std::cout<<"MMU::FLASH RAM save type detected\n";
					current_save_type = FLASH;
					load_backup(backup_file);
					return true;
				}

				break;

			//SRAM
			case 0x53:
				if((memory_map[x+1] == 0x52) && (memory_map[x+2] == 0x41) && (memory_map[x+3] == 0x4D))
				{
					std::cout<<"MMU::SRAM save type detected\n";
					current_save_type = SRAM;
					load_backup(backup_file);
					return true;
				}

				break;
		}
	}
		
	return true;
}

/****** Read BIOS file into memory ******/
bool MMU::read_bios(std::string filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary);

	if(!file.is_open()) 
	{
		std::cout<<"MMU::BIOS file " << filename << " could not be opened. Check file path or permissions. \n";
		return false;
	}

	//Get the file size
	file.seekg(0, file.end);
	u32 file_size = file.tellg();
	file.seekg(0, file.beg);

	u8* ex_mem = &memory_map[0];

	//Read data from the ROM file
	file.read((char*)ex_mem, file_size);

	file.close();
	std::cout<<"MMU::BIOS file " << filename << " loaded successfully. \n";

	return true;
}

/****** Load backup save data ******/
bool MMU::load_backup(std::string filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary);
	std::vector<u8> save_data;

	if(!file.is_open()) 
	{
		std::cout<<"MMU::" << filename << " save data could not be opened. Check file path or permissions. \n";
		return false;
	}

	//Get the file size
	file.seekg(0, file.end);
	u32 file_size = file.tellg();
	file.seekg(0, file.beg);
	save_data.resize(file_size);

	//Load SRAM
	if(current_save_type == SRAM)
	{
		if(file_size > 0x7FFF) { std::cout<<"MMU::Warning - Irregular backup save size\n"; }

		//Read data from file
		file.read(reinterpret_cast<char*> (&save_data[0]), file_size);

		//Write that data into 0xE000000 to 0xE007FFF
		for(u32 x = 0; x < 0x7FFF; x++)
		{
			memory_map[0xE000000 + x] = save_data[x];
		}
	}

	//Load EEPROM
	else if(current_save_type == EEPROM)
	{
		if((file_size != 0x200) && (file_size != 0x2000)) { file_size = 0x200; std::cout<<"MMU::Warning - Irregular backup save size\n"; }

		//Read data from file
		file.read(reinterpret_cast<char*> (&save_data[0]), file_size);

		//Clear eeprom data and resize
		eeprom.size = file_size;
		eeprom.data.clear();
		eeprom.data.resize(file_size, 0);

		//Write that data into EEPROM
		for(u32 x = 0; x < file_size; x++)
		{
			eeprom.data[x] = save_data[x];
		}

		eeprom.size_lock = true;
	}

	file.close();

	std::cout<<"MMU::Loaded save data file " << filename <<  "\n";

	return true;
}

/****** Save backup save data ******/
bool MMU::save_backup(std::string filename)
{
	//Save SRAM
	if(current_save_type == SRAM)
	{
		std::ofstream file(filename.c_str(), std::ios::binary);
		std::vector<u8> save_data;

		if(!file.is_open()) 
		{
			std::cout<<"MMU::" << filename << " save data could not be written. Check file path or permissions. \n";
			return false;
		}


		//Grab data from 0xE000000 to 0xE007FFF
		for(u32 x = 0; x < 0x7FFF; x++)
		{
			save_data.push_back(memory_map[0xE000000 + x]);
		}

		//Write the data to a file
		file.write(reinterpret_cast<char*> (&save_data[0]), 0x7FFF);
		file.close();

		std::cout<<"MMU::Wrote save data file " << filename <<  "\n";
	}

	//Save EEPROM
	else if(current_save_type == EEPROM)
	{
		std::ofstream file(filename.c_str(), std::ios::binary);
		std::vector<u8> save_data;

		if(!file.is_open()) 
		{
			std::cout<<"MMU::" << filename << " save data could not be written. Check file path or permissions. \n";
			return false;
		}

		//Grab data from EEPROM
		for(u32 x = 0; x < eeprom.size; x++)
		{
			save_data.push_back(eeprom.data[x]);
		}

		//Write the data to a file
		file.write(reinterpret_cast<char*> (&save_data[0]), eeprom.size);
		file.close();

		std::cout<<"MMU::Wrote save data file " << filename <<  "\n";
	}

	return true;
}

/****** Start the DMA channels during blanking periods ******/
void MMU::start_blank_dma()
{
	//Repeat bits automatically enable DMAs
	if(dma[0].control & 0x200) { dma[0].enable = true; }
	if(dma[3].control & 0x200) { dma[3].enable = true; }

	//DMA0
	if(dma[0].enable)
	{
		u8 dma_type = ((dma[0].control >> 12) & 0x3);
		
		if(dma_type == 2) { dma[0].started = true; }
	}

	//DMA3
	if(dma[3].enable)
	{
		u8 dma_type = ((dma[3].control >> 12) & 0x3);
		
		if(dma_type == 2) { dma[3].started = true; }
	}
}

/****** Set EEPROM read-write address ******/
void MMU::eeprom_set_addr()
{
	//Clear EEPROM address
	eeprom.address = 0;

	//Skip 2 bits in the bitstream
	eeprom.dma_ptr += 4;

	//Read 6 or 14 bits from the bitstream, MSB 1st
	u16 bits = 0x20;
	u8 bit_length = 6;
	
	if(eeprom.size == 0x2000) { bits = 0x2000; bit_length = 14; }

	for(int x = 0; x < bit_length; x++)
	{
		u16 bitstream = read_u16(eeprom.dma_ptr);
		
		//If bit 0 of the halfword in the bitstream is set, flip the bit for the EEPROM address as well
		if(bitstream & 0x1) { eeprom.address |= bits; }

		eeprom.dma_ptr += 2;
		bits >>= 1;
	}
}

/****** Read EEPROM data ******/
void MMU::eeprom_read_data()
{
	//First 4 bits of the stream are ignored, send 0
	for(int x = 0; x < 4; x++) 
	{ 
		write_u16(eeprom.dma_ptr, 0x0);
		eeprom.dma_ptr += 2;
	}
	
	u16 temp_addr = (eeprom.address * 8);
	u8 bits = 0x80;

	//Get 64 bits from EEPROM, MSB 1st, write them to address pointed by the DMA (as halfwords)
	for(int x = 0; x < 64; x++)
	{
		u8 bitstream = (eeprom.data[temp_addr] & bits) ? 1 : 0;
		bits >>= 1;

		//Write stream to address provided by DMA
		write_u16(eeprom.dma_ptr, bitstream);
		eeprom.dma_ptr += 2;

		//On the 8th bit, move to next 8 bits in EEPROM, reload stuff
		if(bits == 0) 
		{
			temp_addr++;
			bits = 0x80;
		}
	}
}

/****** Write EEPROM data ******/
void MMU::eeprom_write_data()
{
	//Clear EEPROM address
	eeprom.address = 0;

	//Skip 2 bits in the bitstream
	eeprom.dma_ptr += 4;

	//Read 6 or 14 bits from the bitstream, MSB 1st
	u16 bits = 0x20;
	u8 bit_length = 6;
	
	if(eeprom.size == 0x2000) { bits = 0x2000; bit_length = 14; }

	for(int x = 0; x < bit_length; x++)
	{
		u16 bitstream = read_u16(eeprom.dma_ptr);
		
		//If bit 0 of the halfword in the bitstream is set, flip the bit for the EEPROM address as well
		if(bitstream & 0x1) { eeprom.address |= bits; }

		eeprom.dma_ptr += 2;
		bits >>= 1;
	}

	u8 temp_byte = 0;
	u16 temp_addr = (eeprom.address * 8);
	bits = 0x80;

	//Read 64 bits from the bitstream to store at EEPROM address, MSB 1st
	for(int x = 0; x < 64; x++)
	{
		u16 bitstream = read_u16(eeprom.dma_ptr);

		if(bitstream & 0x1) { temp_byte |= bits; }

		eeprom.dma_ptr += 2;
		bits >>= 1;

		//On the 8th bit, send the data to EEPROM, reload stuff
		if(bits == 0) 
		{
			eeprom.data[temp_addr] = temp_byte;
			temp_byte = 0;
			temp_addr++;
			bits = 0x80;
		}
	}

	memory_map[0xD000000] = 0x1;
}
