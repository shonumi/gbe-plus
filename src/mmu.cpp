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
	memory_map.clear();
	memory_map.resize(0x10000000, 0);

	//HLE stuff
	memory_map[DISPCNT] = 0x80;

	write_u32(0x18, 0xEA000042);
	write_u32(0x128, 0xE92D500F);
	write_u32(0x12C, 0xE3A00301);
	write_u32(0x130, 0xE28FE000);
	write_u32(0x134, 0xE510F004);
	write_u32(0x138, 0xE8BD500F);
	write_u32(0x13C, 0xE25EF004);

	dma[0].enable = dma[1].enable = dma[2].enable = dma[3].enable = false;

	std::cout<<"MMU::Initialized\n";
}

/****** MMU Deconstructor ******/
MMU::~MMU() 
{ 
	memory_map.clear();
	std::cout<<"MMU::Shutdown\n"; 
}

/****** Read byte from memory ******/
u8 MMU::read_u8(u32 address) const
{
	return memory_map[address];
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

/****** Write byte into memory ******/
void MMU::write_u8(u32 address, u8 value)
{
	switch(address)
	{
		case REG_IF:
			memory_map[REG_IF] &= ~value;
			break;

		case DMA0CNT_H:
			//Start DMA0 transfer if Bit 15 goes from 0 to 1
			if((value & 0x8000) && ((memory_map[DMA0CNT_H] & 0x8000) == 0))
			{
				dma[0].enable = true;
				dma[0].started = false;
				dma[0].delay = 2;
			}

			//Halt DMA0 transfer if Bit 15 goes from 1 to 0
			else if(((value & 0x8000) == 0) && (memory_map[DMA0CNT_H] & 0x8000)) { dma[0].enable = false; }

			memory_map[DMA0CNT_H] = value;
			break;

		case DMA1CNT_H:
			//Start DMA1 transfer if Bit 15 goes from 0 to 1
			if((value & 0x8000) && ((memory_map[DMA1CNT_H] & 0x8000) == 0))
			{
				dma[1].enable = true;
				dma[1].started = false;
				dma[1].delay = 2;
			}

			//Halt DMA1 transfer if Bit 15 goes from 1 to 0
			else if(((value & 0x8000) == 0) && (memory_map[DMA1CNT_H] & 0x8000)) { dma[1].enable = false; }

			memory_map[DMA1CNT_H] = value;
			break;

		case DMA2CNT_H:
			//Start DMA2 transfer if Bit 15 goes from 0 to 1
			if((value & 0x8000) && ((memory_map[DMA2CNT_H] & 0x8000) == 0))
			{
				dma[2].enable = true;
				dma[2].started = false;
				dma[2].delay = 2;
			}

			//Halt DMA2 transfer if Bit 15 goes from 1 to 0
			else if(((value & 0x8000) == 0) && (memory_map[DMA2CNT_H] & 0x8000)) { dma[2].enable = false; }

			memory_map[DMA2CNT_H] = value;
			break;

		case DMA3CNT_H:
			//Start DMA3 transfer if Bit 15 goes from 0 to 1
			if((value & 0x8000) && ((memory_map[DMA3CNT_H] & 0x8000) == 0))
			{
				dma[3].enable = true;
				dma[3].started = false;
				dma[3].delay = 2;
			}

			//Halt DMA3 transfer if Bit 15 goes from 1 to 0
			else if(((value & 0x8000) == 0) && (memory_map[DMA3CNT_H] & 0x8000)) { dma[3].enable = false; }
			
			memory_map[DMA3CNT_H] = value;
			break;

		default:
			memory_map[address] = value;
	}	
}

/****** Write 2 bytes into memory ******/
void MMU::write_u16(u32 address, u16 value)
{
	write_u8((address+1), ((value >> 8) & 0xFF));
	write_u8(address, (value & 0xFF));
}

/****** Write 4 bytes into memory ******/
void MMU::write_u32(u32 address, u32 value)
{
	write_u8((address+3), ((value >> 24) & 0xFF));
	write_u8((address+2), ((value >> 16) & 0xFF));
	write_u8((address+1), ((value >> 8) & 0xFF));
	write_u8(address, (value & 0xFF));
}

/****** Read binary file to memory ******/
bool MMU::read_file(std::string filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary);

	if(!file.is_open()) 
	{
		std::cout<<"MMU : " << filename << " could not be opened. Check file path or permissions. \n";
		return false;
	}

	//Get the file size
	file.seekg(0, file.end);
	u32 file_size = file.tellg();
	file.seekg(0, file.beg);

	u8* ex_mem = &memory_map[0x8000000];

	//Read 32KB worth of data from ROM file
	file.read((char*)ex_mem, file_size);

	file.close();
	std::cout<<"MMU : " << filename << " loaded successfully. \n";

	return true;
}


	
