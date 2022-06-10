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
#include "gamepad.h"
#include "timer.h"
#include "lcd_data.h"
#include "apu_data.h"
#include "sio_data.h"

class AGB_MMU
{
	public:

	//Cartridge save-type enumerations
	enum backup_types
	{
		NONE,
		EEPROM,
		FLASH_64,
		FLASH_128,
		SRAM,
		DACS,
		JUKEBOX_CONFIG,
	};

	//Cartridge GPIO-type enumerations
	enum gpio_types
	{
		GPIO_DISABLED,
		GPIO_RTC,
		GPIO_SOLAR_SENSOR,
		GPIO_RUMBLE,
		GPIO_GYRO_SENSOR,
	};

	backup_types current_save_type;

	std::vector <u8> memory_map;

	//Memory access timings (Nonsequential and Sequential)
	u8 n_clock;
	u8 s_clock;

	bool bios_lock;

	//Structure to handle DMA transfers
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
		u16 control;
		u8 dest_addr_ctrl;
		u8 src_addr_ctrl;
		u8 delay;
	} dma[4];

	//Structure to handle EEPROM reading and writing
	struct eeprom_controller
	{
		u8 bitstream_byte;
		u16 address;
		u32 dma_ptr;
		std::vector <u8> data;
		u16 size;
		bool size_lock;
	} eeprom;

	//Structure to handle FLASH RAM reading and writing
	struct flash_ram_controller
	{
		u8 current_command;
		u8 bank;
		std::vector< std::vector<u8> > data;
		bool write_single_byte;
		bool switch_bank;
		bool grab_ids;
		bool next_write;
	} flash_ram;

	//Structure to handle 8M DACS FLASH commands and writing
	struct dacs_flash_controller
	{
		u8 current_command;
		u8 status_register;
	} dacs_flash;

	//Structure to handle AM3 SmartMedia cards
	struct am3_smart_media
	{
		bool read_sm_card;
		bool read_key;

		u8 op_delay;
		u32 transfer_delay;
		u32 base_addr;

		u16 blk_stat;
		u16 blk_size;
		u32 blk_addr;

		u32 smc_offset;
		u32 last_offset;
		u16 smc_size;
		u16 smc_base;

		u16 file_index;
		u32 file_count;
		u32 file_size;
		std::vector<u32> file_size_list;
		std::vector<u32> file_addr_list;

		u16 remaining_size;
		std::vector<u8> smid;

		std::vector<u8> firmware_data;
		std::vector<u8> card_data;
	} am3;

	//Structure to handle GBA Jukebox/Music Recorder
	struct jukebox_cart
	{
		std::vector<u16> io_regs;
		u16 status;
		u16 config;
		u16 io_index;
		u8 current_category;
		u8 current_frame;
		u16 file_limit;
		u32 progress;
		u32 remaining_recording_time;
		u32 remaining_playback_time;
		u32 current_recording_time;
		bool format_compact_flash;
		bool is_recording;

		u8 out_hi;
		u8 out_lo;

		u16 current_file;
		u16 last_music_file;
		u16 last_voice_file;
		u16 last_karaoke_file;

		std::vector<std::string> music_files;
		std::vector<std::string> voice_files;
		std::vector<std::string> karaoke_files;

		std::vector<std::string> music_titles;
		std::vector<std::string> music_artists;

		std::vector<u16> music_times;
		std::vector<u16> voice_times;
		std::vector<u16> karaoke_times;

		u32 spectrum_values[9];
	} jukebox;

	//Structure to handle GPIO reading and writing
	struct gpio_controller
	{
		u8 data;
		u8 prev_data;
		u8 direction;
		u8 control;
		u16 state;
		u8 serial_counter;
		u8 serial_byte;
		u8 serial_data[8];
		u8 data_index;
		u8 serial_len;
		gpio_types type;

		u8 rtc_control;

		u8 solar_counter;
		u8 adc_clear;
	} gpio;

	std::vector<u32> cheat_bytes;
	u8 gsa_patch_count;

	bool sio_emu_device_ready;

	std::vector<u32> sub_screen_buffer;
	u32 sub_screen_update;
	bool sub_screen_lock;

	//Advanced debugging
	#ifdef GBE_DEBUG
	bool debug_write;
	bool debug_read;
	u32 debug_addr[4];
	#endif

	AGB_MMU();
	~AGB_MMU();

	void reset();

	void start_blank_dma();

	u8 read_u8(u32 address);
	u16 read_u16(u32 address);
	u32 read_u32(u32 address);

	u16 read_u16_fast(u32 address);
	u32 read_u32_fast(u32 address);

	void write_u8(u32 address, u8 value);
	void write_u16(u32 address, u16 value);
	void write_u32(u32 address, u32 value);

	void write_u16_fast(u32 address, u16 value);
	void write_u32_fast(u32 address, u32 value);

	bool read_file(std::string filename);
	bool read_bios(std::string filename);
	bool read_am3_firmware(std::string filename);
	bool read_smid(std::string filename);
	bool save_backup(std::string filename);
	bool load_backup(std::string filename);

	bool patch_ips(std::string filename);
	bool patch_ups(std::string filename);

	void eeprom_set_addr();
	void eeprom_read_data();
	void eeprom_write_data();

	void flash_erase_chip();
	void flash_erase_sector(u32 sector);
	void flash_switch_bank();

	u8 read_dacs(u32 address);
	void write_dacs(u32 address, u8 value);

	void write_am3(u32 address, u8 value);
	bool check_am3_fat(); 

	void write_jukebox(u32 address, u8 value);
	bool read_jukebox_file_list(std::string filename, u8 category);
	bool jukebox_delete_file();
	bool jukebox_save_recording();
	void jukebox_set_file_info();
	void jukebox_update_metadata();
	bool jukebox_load_audio(std::string filename);
	void process_jukebox();

	//GPIO handling functions
	void process_rtc();
	void process_rumble();
	void process_solar_sensor();
	void process_gyro_sensor();
	
	void process_motion();

	void process_sio();
	void process_player_rumble();

	void magic_watch_recv();

	//Cheat code functions
	void decrypt_gsa(u32 &addr, u32 &val, bool v1);
	void set_cheats();
	void process_cheats(u32 a, u32 v, u32& index);

	void set_lcd_data(agb_lcd_data* ex_lcd_stat);
	void set_apu_data(agb_apu_data* ex_apu_stat);
	void set_sio_data(agb_sio_data* ex_sio_stat);
	void set_mw_data(mag_watch* ex_mw_data);

	AGB_GamePad* g_pad;
	std::vector<gba_timer>* timer;

	//Serialize data for save state loading/saving
	bool mmu_read(u32 offset, std::string filename);
	bool mmu_write(std::string filename);
	u32 size();

	private:

	//Only the MMU and LCD should communicate through this structure
	agb_lcd_data* lcd_stat;

	//Only the MMU and APU should communicate through this structure
	agb_apu_data* apu_stat;

	//Only the MMU and SIO should communicate through this structure
	agb_sio_data* sio_stat;

	//Only the MMU and SIO should communicate through this structure
	mag_watch* mw;
};

#endif // GBA_MMU
