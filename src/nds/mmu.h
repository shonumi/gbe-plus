// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mmu.h
// Date : September 14, 2015
// Description : NDS memory manager unit
//
// Handles reading and writing bytes to memory locations

#ifndef DS_MMU
#define DS_MMU

#include <fstream>
#include <string>
#include <vector>
#include <iostream>

#include "common.h"

class NDS_MMU
{
	public:

	std::vector <u8> memory_map;
	std::vector <u8> cart_data;

	//Memory access timings (Nonsequential and Sequential)
	u8 n_clock;
	u8 s_clock;

	NDS_MMU();
	~NDS_MMU();

	void reset();

	u8 read_u8(u32 address) const;
	u16 read_u16(u32 address) const;
	u32 read_u32(u32 address) const;

	u16 read_u16_fast(u32 address) const;
	u32 read_u32_fast(u32 address) const;

	void write_u8(u32 address, u8 value);
	void write_u16(u32 address, u16 value);
	void write_u32(u32 address, u32 value);

	void write_u16_fast(u32 address, u16 value);
	void write_u32_fast(u32 address, u32 value);

	bool read_file(std::string filename);
	bool read_bios(std::string filename);
	bool save_backup(std::string filename);
	bool load_backup(std::string filename);
};

#endif // DS_MMU


