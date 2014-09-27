// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mmu.h
// Date : April 22, 2014
// Description : Game Boy Advance memory manager unit
//
// Handles reading and writing bytes to memory locations

#ifndef GBA_MMU
#define GBA_MMU

#include <fstream>
#include <string>
#include <vector>
#include <iostream>

#include "common.h"

class MMU
{
	public:

	std::vector <u8> memory_map;

	struct dma_controllers
	{
		bool enable;
		bool started;
		u32 start_address;
		u32 destination_address;
		u16 word_count;
		u16 control;
		u8 delay;
	} dma[4];

	MMU();
	~MMU();

	u8 read_u8(u32 address) const;
	u16 read_u16(u32 address) const;
	u32 read_u32(u32 address) const;

	void write_u8(u32 address, u8 value);
	void write_u16(u32 address, u16 value);
	void write_u32(u32 address, u32 value);

	bool read_file(std::string filename);
};

#endif // GBA_MMU


