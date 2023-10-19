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
		CAMPHO_CONFIG,
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

	//Play-Yan type enumerations
	enum play_yan_types
	{
		PLAY_YAN_OG,
		PLAY_YAN_MICRO,
		NINTENDO_MP3,
	};

	//Play-Yan Operational States
	enum play_yan_states
	{
		PLAY_YAN_NOP,
		PLAY_YAN_PROCESS_CMD,
		PLAY_YAN_WAKE,
		PLAY_YAN_PROCESS_VIDEO_THUMBNAILS,
		PLAY_YAN_START_AUDIO,
		PLAY_YAN_PROCESS_AUDIO,
		PLAY_YAN_START_VIDEO,
		PLAY_YAN_PROCESS_VIDEO,
		PLAY_YAN_WAIT,
		PLAY_YAN_STATUS,
		PLAY_YAN_INIT,
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
		bool enable_karaoke;

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

		std::string recorded_file;
		std::string last_converted_file;
		std::string karaoke_track;

		u32 spectrum_values[9];
	} jukebox;

	//Structure to handle Play-Yan
	struct ags_006
	{
		std::vector<u8> firmware;
		u32 firmware_addr;
		u16 firmware_status;
		u8 firmware_addr_count;

		u8 status;
		play_yan_states op_state;

		u16 access_mode;
		u16 access_param;

		u32 irq_count;
		u32 irq_delay;
		u32 irq_repeat;
		u32 irq_repeat_id;
		u32 delay_reload;

		//Play-Yan (Micro) IRQ data
		u32 sd_check_data[4][8];
		u32 music_check_data[2][8];
		u32 music_play_data[3][8];
		u32 music_stop_data[2][8];
		u32 folder_ops_data[4][8];

		u32 video_check_data[4][8];
		u32 video_play_data[2][8];
		u32 video_stop_data[2][8];
		u32 wake_data[8];

		u32 micro[3][8];

		//Nintendo MP3 IRQ data
		u8 nmp_status_data[32];
		u16 nmp_boot_data[3];
		u32 nmp_data_index;

		u32 irq_data[8];
		bool irq_data_in_use;
		bool start_irqs;
		bool is_video_playing;
		bool is_music_playing;
		bool pause_media;

		u32* irq_data_ptr;
		u32 irq_len;

		u8 cnt_data[12];
		u32 cmd;
		u32 serial_cmd_index;
		std::vector <u8> command_stream;
		bool capture_command_stream;

		std::vector<u8> card_data;
		u32 card_addr;

		std::vector<std::string> music_files;
		std::vector<std::string> video_files;

		std::vector<u32> video_fps;

		std::vector<u8> video_thumbnail;
		std::vector<u8> video_data;
		u16 thumbnail_addr;
		u16 thumbnail_index;
		u32 video_data_addr; 

		u32 music_length;
		u32 video_progress;
		u32 video_length;
		float video_current_fps;
		float video_frame_count;
		bool update_video_frame;

		u32 tracker_progress;
		u32 tracker_update_size;

		u32 video_index;
		std::string current_dir;
		std::string base_dir;
		std::string current_music_file;
		std::string current_video_file;

		u8 volume;
		u8 bass_boost;
		bool use_bass_boost;

		play_yan_types type;
	} play_yan;

	//Structure to handle Campho Advance
	struct cam_001
	{
		std::vector<u8> data;
		std::vector<u8> g_stream;
		std::vector<u8> video_frame;
		std::vector<u8> capture_buffer;
		u32 last_id;
		u32 bank_index_lo;
		u32 bank_index_hi;
		u32 bank_id;
		u32 rom_data_1;
		u32 rom_data_2;
		u16 rom_stat;
		u16 rom_cnt;
		u16 block_len;
		u16 block_stat;
		bool stream_started;

		u8 video_capture_counter;
		u32 video_frame_index;
		u16 video_frame_size;
		u8 video_frame_slice;
		u8 last_slice;
		u8 repeated_slices;
		bool capture_video;
		bool new_frame;
		bool is_large_frame;

		u32 out_stream_index;
		s8 contact_index;
		std::vector <u8> config_data;
		std::vector <u8> contact_data;
		std::vector <u8> saved_contact_data;
		std::vector <u8> out_stream;
		bool read_out_stream;

		u8 speaker_volume;
		u8 mic_volume;
		u8 video_brightness;
		u8 video_contrast;
		bool image_flip;

		std::string dialed_number;

		std::vector<u32> mapped_bank_id;
		std::vector<u32> mapped_bank_index;
		std::vector<u32> mapped_bank_len;
		std::vector<u32> mapped_bank_pos;
	} campho;

	//Structure to handle Glucoboy
	struct gluco
	{
		u8 io_index;
		u8 index_shift;
		std::vector<u32> io_regs;
		bool request_interrupt;
		bool reset_shift;
	} glucoboy;

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

	void am3_reset();
	void write_am3(u32 address, u8 value);
	bool check_am3_fat();
	bool am3_load_folder(std::string folder);

	void jukebox_reset();
	void write_jukebox(u32 address, u8 value);
	bool read_jukebox_file_list(std::string filename, u8 category);
	bool jukebox_delete_file();
	bool jukebox_save_recording();
	void jukebox_set_file_info();
	void jukebox_update_metadata();
	bool jukebox_load_audio(std::string filename);
	bool jukebox_load_karaoke_audio();
	bool jukebox_update_music_file_list();
	void process_jukebox();

	void play_yan_reset();
	void write_play_yan(u32 address, u8 value);
	bool read_play_yan_thumbnail(std::string filename);
	u8 read_play_yan(u32 address);
	void process_play_yan_cmd();
	void process_play_yan_irq();
	void play_yan_set_music_file();
	void play_yan_set_video_file();
	void play_yan_set_folder();
	void play_yan_get_id3_data(std::string filename);
	void play_yan_set_ini_file();
	void play_yan_wake();
	bool play_yan_load_audio(std::string filename);

	void write_nmp(u32 address, u8 value);
	u8 read_nmp(u32 address);
	void process_nmp_cmd();

	void campho_reset();
	u8 read_campho(u32 address);
	u8 read_campho_seq(u32 address);
	void write_campho(u32 address, u8 value);
	void campho_set_rom_bank(u32 bank, u32 address, bool set_hi_bank);
	void campho_map_rom_banks();
	u32 campho_get_bank_by_id(u32 id);
	u32 campho_get_bank_by_id(u32 id, u32 index);
	void campho_set_video_data();
	void campho_get_image_data(u8* img_data, u32 width, u32 height);
	u16 campho_convert_settings_val(u8 input);
	std::string campho_convert_contact_name();
	u8 campho_find_settings_val(u16 input);
	void campho_make_settings_stream(u32 input);
	void campho_process_input_stream();
	void process_campho();

	void glucoboy_reset();
	u8 read_glucoboy(u32 address);
	void write_glucoboy(u32 address, u8 value);
	void process_glucoboy_irq();

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
