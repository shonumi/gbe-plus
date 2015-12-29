// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mmu.h
// Date : September 14, 2015
// Description : NDS memory manager unit
//
// Handles reading and writing bytes to memory locations

#ifndef NDS_MMU
#define NDS_MMU

#include <fstream>
#include <string>
#include <vector>
#include <iostream>

#include "common.h"
#include "common/config.h"
#include "lcd_data.h"

class NTR_MMU
{
	public:

	std::vector <u8> memory_map;
	std::vector <u8> cart_data;

	//Memory access timings (Nonsequential and Sequential)
	u8 n_clock;
	u8 s_clock;

	struct cart_header
	{
		std::string title;
		std::string game_code;
		std::string maker_code;
		u8 unit_code;

		u32 arm9_rom_offset;
		u32 arm9_entry_addr;
		u32 arm9_ram_addr;
		u32 arm9_size;

		u32 arm7_rom_offset;
		u32 arm7_entry_addr;
		u32 arm7_ram_addr;
		u32 arm7_size;
	} header;

	NTR_MMU();
	~NTR_MMU();

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

	void set_lcd_data(ntr_lcd_data* ex_lcd_stat);

	void parse_header();

	private:

	//Only the MMU and LCD should communicate through this structure
	ntr_lcd_data* lcd_stat;
};

#endif // NDS_MMU


