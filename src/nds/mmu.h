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
#include <queue>
#include <iostream>

#include "common.h"
#include "gamepad.h"
#include "common/config.h"
#include "lcd_data.h"

class NTR_MMU
{
	public:

	std::vector <u8> memory_map;
	std::vector <u8> cart_data;
	std::vector <u8> nds7_bios;
	std::vector <u8> nds9_bios;
	std::vector <u8> firmware;
	
	//NDS7 IPC FIFO
	struct nds7_interprocess
	{
		//MMIO registers
		u16 sync;
		u16 cnt;

		//IPC FIFO data
		std::queue <u32> fifo;
		u32 fifo_latest;
		u32 fifo_incoming;
	} nds7_ipc;

	//NDS9 IPC FIFO
	struct nds9_interprocess
	{
		//MMIO registers
		u16 sync;
		u16 cnt;

		//IPC FIFO data
		std::queue <u32> fifo;
		u32 fifo_latest;
		u32 fifo_incoming;
	} nds9_ipc;

	//NDS7 SPI Bus
	struct nds7_spi_bus
	{
		//MMIO registers
		u16 cnt;
		u16 data;

		//SPI data
		u32 baud_rate;
		s32 transfer_clock;
		bool active_transfer;
	} nds7_spi;

	//Memory access timings (Nonsequential and Sequential)
	u8 n_clock;
	u8 s_clock;

	u32 nds9_bios_vector;
	u32 nds9_irq_handler;

	u32 nds7_bios_vector;
	u32 nds7_irq_handler;

	//Determines whether memory access comes from NDS9/NDS7
	u8 access_mode;

	//Structure for handling DS cart headers
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

	//Structure to handle DMA transfers
	//0-3 are for NDS9
	//4-7 are for NDS7
	struct dma_controllers
	{
		bool enable;
		bool started;
		u32 start_address;
		u32 original_start_address;
		u32 destination_address;
		u32 current_dma_position;
		u32 word_count;
		u8 word_type;
		u32 control;
		u8 dest_addr_ctrl;
		u8 src_addr_ctrl;
		u8 delay;
	} dma[8];

	//NDS9 and NDS7 have separate IE and IF registers (accessed at the same address)
	u32 nds9_ie;
	u32 nds9_if;
	u32 nds9_old_ie;
	u32 nds9_old_if;

	u32 nds7_ie;
	u32 nds7_if;
	u32 nds7_old_ie;
	u32 nds7_old_if;

	u16 firmware_state;
	u32 firmware_index;

	NTR_MMU();
	~NTR_MMU();

	void reset();

	u8 read_u8(u32 address);
	u16 read_u16(u32 address);
	u32 read_u32(u32 address);

	u16 read_u16_fast(u32 address) const;
	u32 read_u32_fast(u32 address) const;

	void write_u8(u32 address, u8 value);
	void write_u16(u32 address, u16 value);
	void write_u32(u32 address, u32 value);

	void write_u16_fast(u32 address, u16 value);
	void write_u32_fast(u32 address, u32 value);

	bool read_file(std::string filename);
	bool read_bios_nds7(std::string filename);
	bool read_bios_nds9(std::string filename);
	bool save_backup(std::string filename);
	bool load_backup(std::string filename);

	void process_spi_bus();
	void process_firmware();
	void setup_default_firmware();

	void set_lcd_data(ntr_lcd_data* ex_lcd_stat);

	void parse_header();

	NTR_GamePad* g_pad;

	private:

	//Only the MMU and LCD should communicate through this structure
	ntr_lcd_data* lcd_stat;
};

#endif // NDS_MMU
