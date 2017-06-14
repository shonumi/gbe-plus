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
#include "sio_data.h"

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
		MBC5,
		MBC7,
		HUC1,
		MMM01,
		GB_CAMERA,
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
		//General MBC attributes
		u32 rom_size;
		u32 ram_size;
		mbc_types mbc_type;
		bool battery;
		bool ram;
		bool multicart;
		bool rumble;

		//MBC3 RTC
		bool rtc;
		bool rtc_enabled;
		bool rtc_latched;
		u8 rtc_latch_1, rtc_latch_2, rtc_reg[5];
		u8 latch_reg[5];

		//MBC7
		bool idle;
		u8 internal_value;
		u8 internal_state;
		u8 cs;
		u8 sk;
		u8 buffer_length;
		u8 command_code;
		u16 addr;
		u16 buffer;

		//Camera
		u8 cam_reg[54];
		std::vector <u8> cam_buffer;
		bool cam_lock;
	} cart;

	u8 ir_signal;
	bool ir_send;

	dmg_core_pad* g_pad;

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

	bool patch_ips(std::string filename);
	bool patch_ups(std::string filename);

	//Memory Bank Controller dedicated read/write operations
	void mbc_write(u16 address, u8 value);
	u8 mbc_read(u16 address);

	void mbc1_write(u16 address, u8 value);
	u8 mbc1_read(u16 address);

	void mbc1_multicart_write(u16 address, u8 value);
	u8 mbc1_multicart_read(u16 address);

	void mbc2_write(u16 address, u8 value);
	u8 mbc2_read(u16 address);

	void mbc3_write(u16 address, u8 value);
	u8 mbc3_read(u16 address);

	void mbc5_write(u16 address, u8 value);
	u8 mbc5_read(u16 address);

	void mbc7_write(u16 address, u8 value);
	void mbc7_write_ram(u8 value);
	u8 mbc7_read(u16 address);

	void huc1_write(u16 address, u8 value);
	u8 huc1_read(u16 address);

	void mmm01_write(u16 address, u8 value);
	u8 mmm01_read(u16 address);

	void cam_write(u16 address, u8 value);
	u8 cam_read(u16 address);
	bool cam_load_snapshot(std::string filename);

	void set_gs_cheats();
	void set_gg_cheats();

	void set_lcd_data(dmg_lcd_data* ex_lcd_stat);
	void set_cgfx_data(dmg_cgfx_data* ex_cgfx_stat);
	void set_apu_data(dmg_apu_data* ex_apu_stat);
	void set_sio_data(dmg_sio_data* ex_sio_stat);

	//Serialize data for save state loading/saving
	bool mmu_read(u32 offset, std::string filename);
	bool mmu_write(std::string filename);
	u32 size();

	private:

	u8 previous_value;

	//Only the MMU and LCD should communicate through this structure
	dmg_lcd_data* lcd_stat;

	//Only the MMU and the LCD should communicate through this structure
	dmg_cgfx_data* cgfx_stat;

	//Only the MMU and APU should communicate through this structure
	dmg_apu_data* apu_stat;

	//Only the MMU and SIO should communicate through this structure
	dmg_sio_data* sio_stat;
};

#endif // GB_MMU
