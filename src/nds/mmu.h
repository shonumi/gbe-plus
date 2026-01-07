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
#include "timer.h"
#include "common/config.h"
#include "lcd_data.h"
#include "apu_data.h"

class NTR_MMU
{
	public:

	//NDS cartridge save-type enumerations
	enum backup_types
	{
		AUTO,
		NONE,
		EEPROM_512,
		EEPROM,
		FLASH,
		FRAM
	};

	//GBA cartridge save-type enumerations
	enum gba_backup_types
	{
		GBA_NONE,
		GBA_EEPROM,
		GBA_FLASH_64,
		GBA_FLASH_128,
		GBA_SRAM
	};

	//Slot-1 enumerations
	enum slot1_types
	{
		SLOT1_NORMAL,
		SLOT1_NTR_031,
	};

	//Slot-2 enumerations
	enum slot2_types
	{
		SLOT2_AUTO,
		SLOT2_NONE,
		SLOT2_PASSME,
		SLOT2_RUMBLE_PAK,
		SLOT2_GBA_CART,
		SLOT2_UBISOFT_PEDOMETER,
		SLOT2_HCV_1000,
		SLOT2_MAGIC_READER,
		SLOT2_MEMORY_EXPANSION,
		SLOT2_MOTION_PACK,
		SLOT2_FACENING_SCAN,
		SLOT2_BAYER_DIGIT,
	};

	backup_types current_save_type;
	gba_backup_types gba_save_type;

	slot1_types current_slot1_device;
	slot2_types current_slot2_device;

	std::vector <u8> memory_map;
	std::vector <u8> cart_data;
	std::vector <u8> nds7_bios;
	std::vector <u8> nds9_bios;
	std::vector <u8> firmware;
 	std::vector <u8> dtcm;
	std::vector <u8> save_data;
	std::vector <u8> nds7_vwram;

	std::vector <u32> capture_buffer;

	u8 gx_fifo_mem[4];
	
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
		u32 transfer_count;
		u32 access_index;
		u32 access_addr;
		u8 state;
		u8 last_state;
		u8 hold_state;
		bool cmd_wait;

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
		u8 last_state;
		u32 transfer_src;
		u8 transfer_count;
		u32 chip_id;

		u32 seed_0_lo[2];
		u16 seed_0_hi[2];
		u32 seed_1_lo[2];
		u16 seed_1_hi[2];

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
		u8 read_stat;
		
		//0 = STAT1, 1 = STAT2, 2,3,4 = Alarm1-INT1, 5,6,7 = Alarm2-INT2, 8 = Clock adjust, 9 = Free
		u8 regs[10];
		u8 reg_index;

		//INT1 and INT2 data
		s32 int1_clock;
		u32 int1_freq;
		bool int1_enable;
		bool int2_enable;
	} nds7_rtc;

	//NDS9 Maths
	struct nds9_hardware_math
	{
		u64 div_numer;
		u64 div_denom;
		u64 div_result;
		u64 div_remainder;
		u64 sqrt_param;
		u32 sqrt_result;
	} nds9_math;

	//Touchscreen controller
	struct nds_touchscreen_controller
	{
		u16 adc_x1, adc_x2;
		u16 adc_y1, adc_y2;
		u8 scr_x1, scr_x2;
		u8 scr_y1, scr_y2;
	} touchscreen;

	//HCV-1000
	struct hcv_1000
	{
		u8 cnt;
		std::vector <u8> data;
	} hcv;

	//Magic Reader
	struct konami_magic_reader
	{
		u8 command;
		u8 in_data;
		u8 counter;
		u8 out_byte;
		u8 state;
		u8 sck;
		u32 out_data;
		u32 index;
		u32 oid_status;
		bool oid_reset;
	} magic_reader;

	//NTR-014 aka "NEON" aka Facening Scan/DS Scanner
	struct ntr_014
	{
		std::vector <u8> mmap;
		std::vector <u8> i2c_transfer;
		u16 index;
		u8 i2c_data;
		u8 i2c_cnt;
	} neon;

	//Structure to handle Glucoboy
	struct gluco
	{
		u8 io_index;
		u8 index_shift;
		u32 parameter_length;
		u32 msg_index;
		std::vector<u32> io_regs;
		std::vector<u8> parameters;
		std::vector< std::vector<u8> > messages;
		bool request_interrupt;
		bool reset_shift;
		bool is_idle;

		u32 daily_grps;
		u32 bonus_grps;
		u32 good_days;
		u32 days_until_bonus;
		u32 hardware_flags;
		u32 ld_threshold;
		u32 serial_number;
		u16 idle_value;
	} bayer_digit;

	//Wantame Card Scanner
	struct wantame_card_scanner
	{
		u32 index;
		std::string barcode;
		std::vector <u8> data;
	} wcs;

	//Wave Scanner
	struct starforce_wave_scanner
	{
		u32 index;
		u8 level;
		u8 type;
		bool is_data_barcode;
		std::string barcode;
		std::vector <u8> data;
	} wave_scanner;
	
	//NTR-027
	struct activity_meter
	{
		u8 command;
		u8 state;
		u8 packet_parameter;
		u16 mem_addr;
		u32 ir_counter;
		bool connected;
		bool start_comms;
		std::vector <u8> data;
		std::vector <u8> ir_stream;
	} ntr_027;

	//Memory Expansion Pak
	struct memory_expansion_pak
	{
		bool is_locked;
		std::vector <u8> data;
	} mem_pak;

	//NDS9 3D GX FIFO
	std::queue <u32> nds9_gx_fifo;
	u32 gx_fifo_entry;
	u32 gx_fifo_param_length;

	//Memory access timings (Nonsequential and Sequential)
	u8 n_clock;
	u8 s_clock;

	u32 nds9_bios_vector;
	u32 nds9_irq_handler;

	u32 nds7_bios_vector;
	u32 nds7_irq_handler;

	//Determines whether memory access comes from NDS9/NDS7
	u8 access_mode;
	u8 wram_mode;
	u8 rumble_state;

	bool do_save;
	bool fetch_request;
	bool gx_command;

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

	//Structure to handle microphone sound capture
	struct sound_capture
	{
		u8 cnt;
		u32 destination_address;
		u16 length;
	} sound_cap[2];

	//KEY1 key table
	std::vector<u8> key1_table;

	//Keycode
	std::vector<u32> key_code;

	//Encryption key level and ID
	u8 key_level;
	u32 key_id;

	//KEY2
	u64 key_2_x;
	u64 key_2_y;

	//NDS9 and NDS7 have separate IE, IF, and other registers (accessed at the same address)
	u32 nds9_ie;
	u32 nds9_if;
	u8 gx_if;
	u32 nds9_temp_if;
	u32 nds9_ime;
	u16 power_cnt1;
	u16 nds9_exmem;

	u32 nds7_ie;
	u32 nds7_if;
	u32 nds7_temp_if;
	u32 nds7_ime;
	u16 power_cnt2;
	u16 nds7_exmem;

	u8 firmware_status;
	u16 firmware_state;
	u16 firmware_count;
	u32 firmware_index;
	bool in_firmware;

	u16 touchscreen_state;
	u8 apu_io_id;

	u32 dtcm_addr;
	u32 dtcm_end;
	bool dtcm_load_mode;
	
	u32 itcm_addr;
	bool itcm_load_mode;

	u32 pal_a_bg_slot[4];
	u32 pal_a_obj_slot[4];

	u32 pal_b_bg_slot[4];
	u32 pal_b_obj_slot[4];

	u32 vram_tex_slot[4];
	u32 vram_bank_log[9][5];

	bool bg_vram_bank_enable_a;
	bool bg_vram_bank_enable_b;

	//Advanced debugging
	#ifdef GBE_DEBUG
	bool debug_write;
	bool debug_read;
	u32 debug_addr[8];
	u8 debug_access;
	#endif

	NTR_MMU();
	~NTR_MMU();

	void reset();

	void start_hblank_dma();
	void start_vblank_dma();
	void start_gxfifo_dma();
	void start_dma(u8 dma_bits);

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
	void write_u64_fast(u32 address, u64 value);

	u16 read_cart_u16(u32 address) const;
	u32 read_cart_u32(u32 address) const;

	bool read_file(std::string filename);
	bool read_slot2_file(std::string filename);
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
	void process_microphone();
	void write_rtc();
	u8 read_rtc();
	void setup_default_firmware();

	u8 read_slot2_device(u32 address);
	void write_slot2_device(u32 address, u8 value);

	void key1_encrypt(u32 &lo, u32 &hi);
	void key1_decrypt(u32 &lo, u32 &hi);
	void init_key_code(u8 level, u32 mod);
	void apply_key_code(u32 mod);
	u32 key1_read_u32(u32 index);
	u32 key_code_read_u32(u32 index);

	void get_gx_fifo_param_length();
	void copy_capture_buffer(u32 capture_addr);
	void deallocate_vram(u8 bank_id, u8 mst);

	void set_lcd_data(ntr_lcd_data* ex_lcd_stat);
	void set_lcd_3D_data(ntr_lcd_3D_data* ex_lcd_3D_stat);
	void set_apu_data(ntr_apu_data* ex_apu_stat);
	void set_nds7_pc(u32* ex_pc);
	void set_nds9_pc(u32* ex_pc);

	//Slot-1 device functions
	u16 get_checksum();
	void setup_ntr_027();
	void ntr_027_process();

	//Slot-2 device functions
	bool slot2_hcv_load_barcode(std::string filename);
	void magic_reader_process();

	void bayer_digit_reset();
	void process_bayer_digit_index();
	void process_bayer_digit_irq();

	//Microphone device functions
	void wantame_scanner_process();
	void wantame_scanner_set_barcode();
	void wantame_scanner_set_pulse(u32 lo, u32 hi);
	bool wantame_scanner_load_barcode(std::string filename);

	void wave_scanner_process();
	void wave_scanner_set_pulse(u32 hi, u32 lo);
	void wave_scanner_set_data();

	void neon_set_stm_register(u16 index, u8 value);

	void parse_header();

	NTR_GamePad* g_pad;
	std::vector<nds_timer>* nds7_timer;
	std::vector<nds_timer>* nds9_timer;

	//Serialize data for save state loading/saving
	bool mmu_read(u32 offset, std::string filename);
	bool mmu_write(std::string filename);
	u32 size();

	private:

	//Only the MMU and LCD should communicate through this structure
	ntr_lcd_data* lcd_stat;

	//Only the MMU and LCD should communicate through this structure
	ntr_lcd_3D_data* lcd_3D_stat;

	//Only the MMU and APU should communicate through this structure
	ntr_apu_data* apu_stat;

	u32* nds7_pc;
	u32* nds9_pc;
};

#endif // NDS_MMU
