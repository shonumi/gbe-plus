// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mmu.h
// Date : September 14, 2015
// Description : NDS memory manager unit
//
// Handles reading and writing bytes to memory locations

#include "mmu.h"

/****** MMU Constructor ******/
NDS_MMU::NDS_MMU() 
{
	reset();
}

/****** MMU Deconstructor ******/
NDS_MMU::~NDS_MMU() 
{ 
	memory_map.clear();
	std::cout<<"MMU::Shutdown\n"; 
}

/****** MMU Reset ******/
void NDS_MMU::reset()
{
	memory_map.clear();
	memory_map.resize(0xB000000, 0);

	//HLE stuff
	memory_map[NDS_DISPCNT] = 0x80;

	//Default memory access timings (4, 2)
	n_clock = 4;
	s_clock = 2;

	std::cout<<"MMU::Initialized\n";
}

/****** Read byte from memory ******/
u8 NDS_MMU::read_u8(u32 address) const
{
	//Check for unused memory first
	if(address >= 0xB000000) { std::cout<<"Out of bounds read : 0x" << std::hex << address << "\n"; return 0; }

	return memory_map[address];
}

/****** Read 2 bytes from memory ******/
u16 NDS_MMU::read_u16(u32 address) const
{
	return ((read_u8(address+1) << 8) | read_u8(address)); 
}

/****** Read 4 bytes from memory ******/
u32 NDS_MMU::read_u32(u32 address) const
{
	return ((read_u8(address+3) << 24) | (read_u8(address+2) << 16) | (read_u8(address+1) << 8) | read_u8(address));
}

/****** Reads 2 bytes from memory - No checks done on the read, used for known memory locations such as registers ******/
u16 NDS_MMU::read_u16_fast(u32 address) const
{
	return ((memory_map[address+1] << 8) | memory_map[address]);
}

/****** Reads 4 bytes from memory - No checks done on the read, used for known memory locations such as registers ******/
u32 NDS_MMU::read_u32_fast(u32 address) const
{
	return ((memory_map[address+3] << 24) | (memory_map[address+2] << 16) | (memory_map[address+1] << 8) | memory_map[address]);
}

/****** Write byte into memory ******/
void NDS_MMU::write_u8(u32 address, u8 value)
{
	//Check for unused memory first
	if(address >= 0x10000000) { std::cout<<"Out of bounds write : 0x" << std::hex << address << "\n"; return; }

	switch(address)
	{
		//Display Control
		case NDS_DISPCNT:
		case NDS_DISPCNT+1:
			memory_map[address] = value;
			break;

		//Display Status
		case NDS_DISPSTAT:
			{
				u8 read_only_bits = (memory_map[NDS_DISPSTAT] & 0x7);
				
				memory_map[address] = (value & ~0x7);
				memory_map[address] |= read_only_bits;
			}
 
			break;

		//BG0 Control
		case NDS_BG0CNT:
		case NDS_BG0CNT+1:
			memory_map[address] = value;
			break;

		//BG1 Control
		case NDS_BG1CNT:
		case NDS_BG1CNT+1:
			memory_map[address] = value;
			break;

		//BG2 Control
		case NDS_BG2CNT:
		case NDS_BG2CNT+1:
			memory_map[address] = value;
			break;

		//BG3 Control
		case NDS_BG3CNT:
		case NDS_BG3CNT+1:
			memory_map[address] = value;
			break;

		//BG0 Horizontal Offset
		case NDS_BG0HOFS:
		case NDS_BG0HOFS+1:
			memory_map[address] = value;
			break;

		//BG0 Vertical Offset
		case NDS_BG0VOFS:
		case NDS_BG0VOFS+1:
			memory_map[address] = value;
			break;

		//BG1 Horizontal Offset
		case NDS_BG1HOFS:
		case NDS_BG1HOFS+1:
			memory_map[address] = value;
			break;

		//BG1 Vertical Offset
		case NDS_BG1VOFS:
		case NDS_BG1VOFS+1:
			memory_map[address] = value;
			break;

		//BG2 Horizontal Offset
		case NDS_BG2HOFS:
		case NDS_BG2HOFS+1:
			memory_map[address] = value;
			break;

		//BG2 Vertical Offset
		case NDS_BG2VOFS:
		case NDS_BG2VOFS+1:
			memory_map[address] = value;
			break;

		//BG3 Horizontal Offset
		case NDS_BG3HOFS:
		case NDS_BG3HOFS+1:
			memory_map[address] = value;
			break;

		//BG3 Vertical Offset
		case NDS_BG3VOFS:
		case NDS_BG3VOFS+1:
			memory_map[address] = value;
			break;

		//BG2 Scale/Rotation Parameter A
		case NDS_BG2PA:
		case NDS_BG2PA+1:
			memory_map[address] = value;
			break;

		//BG2 Scale/Rotation Parameter B
		case NDS_BG2PB:
		case NDS_BG2PB+1:
			memory_map[address] = value;
			break;

		//BG2 Scale/Rotation Parameter C
		case NDS_BG2PC:
		case NDS_BG2PC+1:
			memory_map[address] = value;
			break;

		//BG2 Scale/Rotation Parameter D
		case NDS_BG2PD:
		case NDS_BG2PD+1:
			memory_map[address] = value;
			break;

		//BG2 Scale/Rotation X Reference
		case NDS_BG2X_L:
		case NDS_BG2X_L+1:
		case NDS_BG2X_L+2:
		case NDS_BG2X_L+3:
			memory_map[address] = value;
			break;

		//BG2 Scale/Rotation Y Reference
		case NDS_BG2Y_L:
		case NDS_BG2Y_L+1:
		case NDS_BG2Y_L+2:
		case NDS_BG2Y_L+3:
			memory_map[address] = value;
			break;

		//BG3 Scale/Rotation Parameter A
		case NDS_BG3PA:
		case NDS_BG3PA+1:
			memory_map[address] = value;
			break;

		//BG3 Scale/Rotation Parameter B
		case NDS_BG3PB:
		case NDS_BG3PB+1:
			memory_map[address] = value;
			break;

		//BG3 Scale/Rotation Parameter C
		case NDS_BG3PC:
		case NDS_BG3PC+1:
			memory_map[address] = value;
			break;

		//BG3 Scale/Rotation Parameter D
		case NDS_BG3PD:
		case NDS_BG3PD+1:
			memory_map[address] = value;
			break;

		//BG3 Scale/Rotation X Reference
		case NDS_BG3X_L:
		case NDS_BG3X_L+1:
		case NDS_BG3X_L+2:
		case NDS_BG3X_L+3:
			memory_map[address] = value;
			break;

		//BG3 Scale/Rotation Y Reference
		case NDS_BG3Y_L:
		case NDS_BG3Y_L+1:
		case NDS_BG3Y_L+2:
		case NDS_BG3Y_L+3:
			memory_map[address] = value;
			break;

		//Window 0 Horizontal Coordinates
		case NDS_WIN0H:
		case NDS_WIN0H+1:
			memory_map[address] = value;
			break;

		//Window 1 Horizontal Coordinates
		case NDS_WIN1H:
		case NDS_WIN1H+1:
			memory_map[address] = value;
			break;

		//Window 0 Vertical Coordinates
		case NDS_WIN0V:
		case NDS_WIN0V+1:
			memory_map[address] = value;
			break;

		//Window 1 Vertical Coordinates
		case NDS_WIN1V:
		case NDS_WIN1V+1:
			memory_map[address] = value;
			break;

		//Window 0 In Enable Flags
		case NDS_WININ:
			memory_map[address] = value;
			break;

		//Window 1 In Enable Flags
		case NDS_WININ+1:
			memory_map[address] = value;
			break;

		//Window 0 Out Enable Flags
		case NDS_WINOUT:
			memory_map[address] = value;
			break;

		//Window 1 Out Enable Flags
		case NDS_WINOUT+1:
			memory_map[address] = value;
			break;

		//SFX Control
		case NDS_BLDCNT:
			memory_map[address] = value;			
			break;

		case NDS_BLDCNT+1:
			memory_map[address] = value;
			break;

		//SFX Alpha Control
		case NDS_BLDALPHA:
			memory_map[address] = value;
			break;

		case NDS_BLDALPHA+1:
			memory_map[address] = value;
			break;

		//SFX Brightness Control
		case NDS_BLDY:
			memory_map[address] = value;
			break;
		
		default:
			memory_map[address] = value;
			break;
	}
}

/****** Write 2 bytes into memory ******/
void NDS_MMU::write_u16(u32 address, u16 value)
{
	write_u8(address, (value & 0xFF));
	write_u8((address+1), ((value >> 8) & 0xFF));
}

/****** Write 4 bytes into memory ******/
void NDS_MMU::write_u32(u32 address, u32 value)
{
	write_u8(address, (value & 0xFF));
	write_u8((address+1), ((value >> 8) & 0xFF));
	write_u8((address+2), ((value >> 16) & 0xFF));
	write_u8((address+3), ((value >> 24) & 0xFF));
}

/****** Writes 2 bytes into memory - No checks done on the read, used for known memory locations such as registers ******/
void NDS_MMU::write_u16_fast(u32 address, u16 value)
{
	memory_map[address] = (value & 0xFF);
	memory_map[address+1] = ((value >> 8) & 0xFF);
}

/****** Writes 4 bytes into memory - No checks done on the read, used for known memory locations such as registers ******/
void NDS_MMU::write_u32_fast(u32 address, u32 value)
{
	memory_map[address] = (value & 0xFF);
	memory_map[address+1] = ((value >> 8) & 0xFF);
	memory_map[address+2] = ((value >> 16) & 0xFF);
	memory_map[address+3] = ((value >> 24) & 0xFF);
}	

/****** Read binary file to memory ******/
bool NDS_MMU::read_file(std::string filename)
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

	//Read data from the ROM file
	file.read(reinterpret_cast<char*> (&cart_data[0]), file_size);

	//Copy 512 byte header to Main RAM on boot
	for(u32 x = 0; x < 0x200; x++) { memory_map[0x27FFE00 + x] = cart_data[x]; }

	file.close();
	std::cout<<"MMU::" << filename << " loaded successfully. \n";
		
	return true;
}

/****** Read BIOS file into memory ******/
bool NDS_MMU::read_bios(std::string filename) { return true; }

/****** Load backup save data ******/
bool NDS_MMU::load_backup(std::string filename) { return true; }

/****** Save backup save data ******/
bool NDS_MMU::save_backup(std::string filename) { return true; }
