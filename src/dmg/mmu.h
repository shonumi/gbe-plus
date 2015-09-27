// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mmu.h
// Date : May 09, 2015
// Description : Game Boy (Color) memory manager unit
//
// Handles reading and writing bytes to memory locations

#ifndef GB_MMU
#define GB_MMU

#include <fstream>
#include <string>
#include <vector>
#include <iostream>

#include "common.h"
#include "common/config.h"
#include "gamepad.h"
#include "lcd_data.h"
#include "custom_graphics_data.h"
#include "apu_data.h"

class DMG_MMU
{
	public:

	//Memory bank controller-types enumeration
	enum mbc_types
	{ 
		ROM_ONLY, 
		MBC1, 
		MBC2, 
		MBC3, 
		MBC5 
	};

	std::vector <u8> memory_map;
	std::vector <u8> bios;

	//Memory Banks
	std::vector< std::vector<u8> > read_only_bank;
	std::vector< std::vector<u8> > random_access_bank;

	//Working RAM Banks - GBC only
	std::vector< std::vector<u8> > working_ram_bank;
	std::vector< std::vector<u8> > video_ram;

	//Bank controls
	u16 rom_bank;
	u8 ram_bank;
	u8 wram_bank;
	u8 vram_bank;
	u8 bank_bits;
	u8 bank_mode;
	bool ram_banking_enabled;

	//BIOS controls
	bool in_bios;
	u8 bios_type;
	u32 bios_size;

	//Cartridge data structure
	struct cart_data
	{
		u32 rom_size;
		u32 ram_size;
		mbc_types mbc_type;
		bool battery;
		bool ram;
		bool rtc;
		bool rtc_enabled;
		bool rtc_latched;
		u8 rtc_latch_1, rtc_latch_2, rtc_reg[5];
		u8 latch_reg[5];
	} cart;

	DMG_GamePad* g_pad;

	DMG_MMU();
	~DMG_MMU();

	void reset();
	void grab_time();

	u8 read_u8(u16 address);
	u16 read_u16(u16 address);
	s8 read_s8(u16 address);

	void write_u8(u16 address, u8 value);
	void write_u16(u16 address, u16 value);

	bool read_file(std::string filename);
	bool read_bios(std::string filename);
	bool save_backup(std::string filename);
	bool load_backup(std::string filename);

	//Memory Bank Controller dedicated read/write operations
	void mbc_write(u16 address, u8 value);
	u8 mbc_read(u16 address);

	void mbc1_write(u16 address, u8 value);
	u8 mbc1_read(u16 address);

	void mbc2_write(u16 address, u8 value);
	u8 mbc2_read(u16 address);

	void mbc3_write(u16 address, u8 value);
	u8 mbc3_read(u16 address);

	void mbc5_write(u16 address, u8 value);
	u8 mbc5_read(u16 address);

	void set_lcd_data(dmg_lcd_data* ex_lcd_stat);
	void set_cgfx_data(dmg_cgfx_data* ex_cgfx_stat);
	void set_apu_data(dmg_apu_data* ex_apu_stat);

	private:

	u8 previous_value;

	//Only the MMU and LCD should communicate through this structure
	dmg_lcd_data* lcd_stat;

	//Only the MMU and the LCD should communicate through this structure
	dmg_cgfx_data* cgfx_stat;

	//Only the MMU and APU should communicate through this structure
	dmg_apu_data* apu_stat;
};

#endif // GB_MMU
	


