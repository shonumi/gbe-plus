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
#include <map>

#ifdef GBE_NETPLAY
#include <SDL2/SDL_net.h>
#endif

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
		TV_TUNER_FLASH,
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
		PLAY_YAN_IRQ_UPDATE,
		PLAY_YAN_WAKE,
		PLAY_YAN_PROCESS_VIDEO_THUMBNAILS,
		PLAY_YAN_START_AUDIO,
		PLAY_YAN_PROCESS_AUDIO,
		PLAY_YAN_START_VIDEO,
		PLAY_YAN_PROCESS_VIDEO,
		PLAY_YAN_WAIT,
		PLAY_YAN_STATUS,
		PLAY_YAN_INIT,
		PLAY_YAN_BOOT_SEQUENCE,
		PLAY_YAN_GET_SD_DATA,
	};

	//Play-Yan command enumerations
	enum play_yan_commands
	{
		PLAY_YAN_GET_FILESYS_INFO = 0x200,
		PLAY_YAN_SET_DIR = 0x201,
		PLAY_YAN_SET_BRIGHTNESS = 0x300,
		PLAY_YAN_GET_THUMBNAIL = 0x500,
		PLAY_YAN_GET_ID3_DATA = 0x600,
		PLAY_YAN_PLAY_VIDEO = 0x700,
		PLAY_YAN_STOP_VIDEO = 0x701,
		PLAY_YAN_PLAY_MUSIC = 0x800,
		PLAY_YAN_STOP_MUSIC = 0x801,
		PLAY_YAN_PAUSE = 0x902,
		PLAY_YAN_RESUME = 0x904,
		PLAY_YAN_ADVANCE_FRAME = 0x903,
		PLAY_YAN_SEEK = 0x905,
		PLAY_YAN_SET_VOLUME = 0xB00,
		PLAY_YAN_SET_BASS_BOOST = 0xD00,
		PLAY_YAN_ENABLE_BASS_BOOST = 0xD01,
		PLAY_YAN_UPDATE = 0x1000,
		PLAY_YAN_PLAY_SFX = 0x2000,
		PLAY_YAN_CHECK_KEY_FILE = 0x3000,
		PLAY_YAN_READ_KEY_FILE = 0x3001,
		PLAY_YAN_CLOSE_KEY_FILE = 0x3003,
		PLAY_YAN_CHECK_FOR_FIRMWARE = 0x8000,
		PLAY_YAN_GET_STATUS = 0x8001,
		PLAY_YAN_SLEEP = 0x10000,
		PLAY_YAN_UNSLEEP = 0x10001,
		PLAY_YAN_FIRMWARE = 0x800000,

		NMP_START_FILE_LIST = 0x10,
		NMP_CONTINUE_FILE_LIST = 0x11,
		NMP_SET_DIR = 0x20,
		NMP_GET_ID3_DATA = 0x40,
		NMP_PLAY_MUSIC = 0x50,
		NMP_STOP_MUSIC = 0x51,
		NMP_PAUSE = 0x52,
		NMP_RESUME = 0x53,
		NMP_SEEK = 0x60,
		NMP_SET_VOLUME = 0x80,
		NMP_PLAY_SFX = 0x200,
		NMP_CHECK_FIRMWARE_FILE = 0x300,
		NMP_READ_FIRMWARE_FILE = 0x301,
		NMP_CLOSE_FIRMWARE_FILE = 0x303,
		NMP_SLEEP = 0x500,
		NMP_WAKE = 0x501,
		NMP_INIT = 0x8001,
		NMP_UPDATE_AUDIO = 0x8100,
		NMP_HEADPHONE_STATUS = 0x8600,
	};

	//Campho command enumerations
	enum campho_commands
	{
		CAMPHO_END_PHONE_CALL = 0x1741,
		CAMPHO_DIAL_PHONE_NUMBER = 0x3740,
		CAMPHO_ANSWER_PHONE_CALL = 0x5740,
		CAMPHO_CANCEL_PHONE_CALL = 0x9740,
		CAMPHO_GET_CAMERA_FRAME_SMALL = 0xB740,
		CAMPHO_GET_CAMERA_FRAME_LARGE = 0xD740,
		CAMPHO_STOP_CAMERA = 0xF740,
		CAMPHO_SET_MIC_VOLUME = 0x1742,
		CAMPHO_SET_SPEAKER_VOLUME = 0x3742,
		CAMPHO_SET_VIDEO_BRIGHTNESS = 0x5742,
		CAMPHO_SET_VIDEO_CONTRAST = 0x7742,
		CAMPHO_SET_VIDEO_FLIP = 0x9742,
		CAMPHO_READ_GRAPHICS_ROM = 0xB742,
		CAMPHO_PULSE_DIALING = 0x3744,
		CAMPHO_WRITE_CONFIG_DATA = 0x3778,
		CAMPHO_READ_CONFIG_DATA = 0xB778,
		CAMPHO_GET_CONFIG_ENTRY_COUNT = 0xD778,
		CAMPHO_ERASE_CONTACT_DATA = 0x1779,
		CAMPHO_FINISH_CAMERA_FRAME = 0xFF9F,
		CAMPHO_SEND_AT_COMMAND = 0x9FF0,
	};

	//TV Tuner Operational States
	enum tv_tuner_states
	{
		TV_TUNER_STOP_DATA,
		TV_TUNER_START_DATA,
		TV_TUNER_DELAY_DATA,
		TV_TUNER_ACK_DATA,
		TV_TUNER_NEXT_DATA,
		TV_TUNER_READ_DATA,
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
		u16 firmware_cnt;
		u8 firmware_addr_count;
		bool update_firmware;

		u8 status;
		play_yan_states op_state;

		u16 access_mode;
		u16 access_param;

		u32 irq_delay;
		u32 last_delay;
		u32 cycles;
		u32 cycle_limit;
		bool irq_update;

		//Nintendo MP3 data
		u8 nmp_status_data[16];
		u16 nmp_boot_data[2];
		u16 nmp_audio_index;
		u32 nmp_init_stage;
		u16 nmp_ticks;
		u16 nmp_manual_cmd;
		u32 nmp_data_index;
		u32 nmp_cmd_status;
		u32 nmp_entry_count;
		bool nmp_valid_command;
		bool nmp_manual_irq;
		std::string nmp_title;
		std::string nmp_artist;
		u8 nmp_seek_pos;
		u8 nmp_seek_dir;
		u32 nmp_seek_count;

		u32 irq_data[8];
		bool irq_data_in_use;
		bool is_video_playing;
		bool is_music_playing;
		bool is_media_playing;
		bool is_sfx_playing;
		bool is_end_of_samples;

		u8 cnt_data[12];
		u32 cmd;
		u32 update_cmd;
		u32 serial_cmd_index;
		std::vector <u8> command_stream;
		bool capture_command_stream;

		std::vector<u8> card_data;
		u32 card_addr;

		std::vector<std::string> music_files;
		std::vector<std::string> video_files;
		std::vector<std::string> folders;

		std::vector<u32> video_fps;

		std::vector<u8> video_thumbnail;
		std::vector<u8> video_data;
		std::vector<u8> video_bytes;
		std::vector<u32> video_frames;
		u16 thumbnail_addr;
		u16 thumbnail_index;
		u32 video_data_addr;

		u32 music_length;
		u32 video_progress;
		u32 video_length;
		u32 audio_buffer_size;
		u32 audio_frame_count;
		u32 audio_sample_index;
		s32 current_frame;
		float video_current_fps;
		float video_frame_count;
		bool update_video_frame;
		bool update_audio_stream;
		bool update_trackbar_timestamp;

		bool audio_irq_active;
		bool video_irq_active;

		std::vector<u8> sfx_data;

		u32 tracker_progress;
		u32 tracker_update_size;

		u32 video_index;
		std::string current_dir;
		std::string base_dir;
		std::string current_music_file;
		std::string current_video_file;

		u8 volume;
		u8 bass_boost;
		u32 video_brightness;
		bool use_bass_boost;
		bool use_headphones;

		u8 audio_channels;
		u32 audio_sample_rate;
		s16 l_audio_dither_error;
		s16 r_audio_dither_error;

		play_yan_types type;
	} play_yan;

	//Structure to handle Campho Advance
	struct cam_001
	{
		std::vector<u8> data;
		std::vector<u8> g_stream;
		std::vector<u8> video_frame;
		std::vector<u8> local_capture_buffer;
		std::vector<u8> remote_capture_buffer;
		std::vector<u8> tele_data;
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
		bool update_local_camera;
		bool update_remote_camera;
		u8 camera_mode;

		u32 tele_data_index;
		u8 call_state;
		bool is_call_incoming;
		bool is_call_active;
		bool is_call_video_enabled;
		bool send_video_data;
		bool send_audio_data;

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

		std::string at_command;
		std::string dialed_number;
		std::map<std::string, std::string> phone_number_to_ip_addr;
		std::map<std::string, std::string> phone_number_to_port;

		std::vector<u32> mapped_bank_id;
		std::vector<u32> mapped_bank_index;
		std::vector<u32> mapped_bank_len;
		std::vector<u32> mapped_bank_pos;

		#ifdef GBE_NETPLAY

		//"RINGER" - An always on socket that receives calls
		struct campho_ringer
		{
			TCPsocket host_socket, remote_socket;
			IPaddress host_ip;
			IPaddress remote_ip;
			bool connected;
			bool host_init;
			bool remote_init;
			u16 port;
		} ringer;

		//"LINE" - Input/Output sockets for sending/receiving data to remote Campho
		struct campho_line
		{
			TCPsocket host_socket, remote_socket;
			IPaddress host_ip;
			bool connected;
			bool host_init;
			bool remote_init;
		} line;

		SDLNet_SocketSet phone_sockets;

		#endif

		u16 ring_port;
		u16 phone_in_port;
		u16 phone_out_port;
		std::vector <u8> net_buffer;
		u32 network_state;
		bool network_init;
	} campho;

	//Structure to handle Glucoboy
	struct gluco
	{
		u8 io_index;
		u8 index_shift;
		u32 parameter_length;
		std::vector<u32> io_regs;
		std::vector<u8> parameters;
		bool request_interrupt;
		bool reset_shift;

		u32 daily_grps;
		u32 bonus_grps;
		u32 good_days;
		u32 days_until_bonus;
		u32 hardware_flags;
		u32 ld_threshold;
		u32 serial_number;
	} glucoboy;

	//Structure to handle the Agatsuma TV Tuner
	struct tuner
	{
		u8 index;
		u8 data;
		u8 transfer_count;
		u8 current_channel;
		u8 video_brightness;
		u8 video_contrast;
		u8 video_hue;
		u16 channel_id_list[62];
		float channel_freq;
		tv_tuner_states state;
		std::vector<u8> data_stream;
		std::vector<u8> cmd_stream;
		std::vector<u8> video_stream;

		u8 flash_cmd;
		u8 flash_cmd_status;
		std::vector<u8> flash_data;

		bool read_request;
		bool is_channel_on[62];
		bool is_av_input_on;
		bool is_av_connected;

		u8 cnt_a;
		u8 cnt_b;
		u8 read_data;
	} tv_tuner;

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
	void play_yan_update();
	void play_yan_set_music_file();
	void play_yan_set_video_file();
	void play_yan_set_folder();
	void play_yan_get_id3_data(std::string filename);
	void play_yan_set_ini_file();
	void play_yan_set_sound_samples();
	void play_yan_set_video_pixels();
	void play_yan_wake();
	bool play_yan_load_audio(std::string filename);
	bool play_yan_load_video(std::string filename);
	bool play_yan_load_sfx(std::string filename);
	void play_yan_grab_frame_data(u32 frame);
	void play_yan_check_video_header(std::string filename);
	void play_yan_check_audio_from_video(std::vector <u8> &data);
	bool play_yan_get_headphone_status();
	void play_yan_set_headphone_status();

	void write_nmp(u32 address, u8 value);
	u8 read_nmp(u32 address);
	void process_nmp_cmd();
	void access_nmp_io();

	void campho_reset();
	u8 read_campho(u32 address);
	u8 read_campho_seq(u32 address);
	void write_campho(u32 address, u8 value);
	void campho_set_rom_bank(u32 bank, u32 address, bool set_hi_bank);
	void campho_map_rom_banks();
	u32 campho_get_bank_by_id(u32 id);
	u32 campho_get_bank_by_id(u32 id, u32 index);
	void campho_set_local_video_data();
	void campho_set_remote_video_data();
	void campho_get_image_data(u8* img_data, std::vector <u8> &out_buffer, u32 width, u32 height, bool is_net_video);
	u16 campho_convert_settings_val(u8 input);
	std::string campho_convert_contact_name();
	u8 campho_find_settings_val(u16 input);
	void campho_make_settings_stream(u32 input);
	void campho_process_input_stream();
	void campho_process_call();
	void process_campho();
	bool campho_read_contact_list();
	void campho_process_networking();
	void campho_close_network();
	void campho_reset_network();

	void glucoboy_reset();
	u8 read_glucoboy(u32 address);
	void write_glucoboy(u32 address, u8 value);
	void process_glucoboy_irq();
	void process_glucoboy_index();

	void tv_tuner_reset();
	u8 read_tv_tuner(u32 address);
	void write_tv_tuner(u32 address, u8 value);
	void tv_tuner_render_frame();
	void process_tv_tuner_cmd();

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
