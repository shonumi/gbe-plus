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

	//Cartridge save-type enumerations
	enum backup_types
	{
		AUTO,
		NONE,
		EEPROM_512,
		EEPROM,
		FLASH,
		FRAM
	};

	backup_types current_save_type;

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

	//NDS Auxillary SPI Bus
	struct nds_aux_spi_bus
	{
		//MMIO registers
		u16 cnt;
		u16 data;

		//SPI data
		u32 baud_rate;
		s32 transfer_clock;
		bool active_transfer;

		//EEPROM
		u8 eeprom_stat;
		u8 backup_cmd;
		bool backup_cmd_ready;
	} nds_aux_spi;

	//NDS Cart Bus
	struct nds_card_bus
	{
		//MMIO registers
		u32 cnt;
	
		u32 baud_rate;
		s32 transfer_clock;
		u16 transfer_size;
		bool active_transfer;
		u8 state;
		u32 transfer_src;

		//ROM data
		u32 cmd_lo;
		u32 cmd_hi;
		u32 data;
	} nds_card;

	//NDS7 RTC
	struct nds7_real_time_clock
	{
		//MMIO registers
		u8 cnt;
		u8 data;
		u8 io_direction;

		//RTC data and status
		u16 state;
		u8 serial_counter;
		u8 serial_byte;
		u8 serial_data[8];
		u8 data_index;
		u8 serial_len;
		
		//0 = STAT1, 1 = STAT2, 2,3,4 = Alarm1-INT1, 5,6,7 = Alarm2-INT2, 8 = Clock adjust, 9 = Free
		u8 regs[10];
		u8 reg_index;

		//INT1 and INT2 data
		s32 int1_clock;
		u32 int1_freq;
		bool int1_enable;
		bool int2_enable;
	} nds7_rtc;

	//Touchscreen controller
	struct nds_touchscreen_controller
	{
		u16 adc_x1, adc_x2;
		u16 adc_y1, adc_y2;
		u8 scr_x1, scr_x2;
		u8 scr_y1, scr_y2;
	} touchscreen;

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

		u8 raw_sad[4];
		u8 raw_dad[4];
		u8 raw_cnt[4];
	} dma[8];

	//NDS9 and NDS7 have separate IE, IF, and other registers (accessed at the same address)
	u32 nds9_ie;
	u32 nds9_if;
	u32 nds9_old_ie;
	u32 nds9_old_if;
	u32 nds9_ime;
	u16 power_cnt1;
	u16 nds9_exmem;

	u32 nds7_ie;
	u32 nds7_if;
	u32 nds7_old_ie;
	u32 nds7_old_if;
	u32 nds7_ime;
	u16 power_cnt2;
	u16 nds7_exmem;

	u16 firmware_state;
	u32 firmware_index;
	bool in_firmware;

	u16 touchscreen_state;

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
	bool read_firmware(std::string filename);
	bool save_backup(std::string filename);
	bool load_backup(std::string filename);

	void process_spi_bus();
	void process_aux_spi_bus();
	void process_card_bus();
	void process_firmware();
	void process_touchscreen();
	void process_rtc();
	void setup_default_firmware();

	void set_lcd_data(ntr_lcd_data* ex_lcd_stat);

	void parse_header();

	NTR_GamePad* g_pad;

	private:

	//Only the MMU and LCD should communicate through this structure
	ntr_lcd_data* lcd_stat;
};

#endif // NDS_MMU
