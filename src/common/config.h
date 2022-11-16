// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : config.h
// Date : September 20, 2014
// Description : GBE+ configuration options
//
// Parses command-line arguments to configure GBE options

#ifndef EMU_CONFIG
#define EMU_CONFIG


#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include <sstream>

#include "common.h"

void reset_dmg_colors();
void set_dmg_colors(u8 color_type);
void validate_system_type();
u8 get_system_type_from_file(std::string filename);
bool parse_cli_args();
void parse_filenames();
bool parse_ini_file();
bool parse_cheats_file(bool add_cheats);
bool save_ini_file();
bool save_cheats_file();

bool load_osd_font();
void draw_osd_msg(std::string osd_text, std::vector <u32> &osd_surface, u8 x_offset, u8 y_offset);
void draw_osd_msg(std::string osd_text, std::vector <u32> &osd_surface, u8 x_offset, u8 y_offset, u32 width);

bool load_virtual_cursor();

enum special_cart_types
{
	NORMAL_CART,
	DMG_MBC1M,
	DMG_MBC1S,
	DMG_MMM01,
	DMG_MBC30,
	DMG_GBMEM,
	AGB_RTC,
	AGB_SOLAR_SENSOR,
	AGB_RUMBLE,
	AGB_GYRO_SENSOR,
	AGB_TILT_SENSOR,
	AGB_8M_DACS,
	AGB_AM3,
	AGB_JUKEBOX,
	AGB_PLAY_YAN,
	NDS_IR_CART,
};

enum gba_save_types
{
	AGB_AUTO_DETECT,
	AGB_NO_SAVE,
	AGB_SRAM,
	AGB_EEPROM,
	AGB_FLASH64,
	AGB_FLASH128,
	AGB_DACS_FLASH,
	AGB_JUKEBOX_CONFIG,
};

namespace config
{ 
	extern std::string rom_file;
	extern std::string bios_file;
	extern std::string save_file;
	extern std::string dmg_bios_path;
	extern std::string gbc_bios_path;
	extern std::string agb_bios_path;
	extern std::string nds7_bios_path;
	extern std::string nds9_bios_path;
	extern std::string nds_firmware_path;
	extern std::string min_bios_path;
	extern std::string save_path;
	extern std::string ss_path;
	extern std::string cfg_path;
	extern std::string data_path;
	extern std::string cheats_path;
	extern std::string external_camera_file;
	extern std::string external_card_file;
	extern std::string external_image_file;
	extern std::string external_data_file;
	extern std::vector <std::string> recent_files;
	extern std::vector <std::string> cli_args;

	extern u32 gbe_key_a, gbe_key_b, gbe_key_x, gbe_key_y, gbe_key_start, gbe_key_select, gbe_key_up, gbe_key_down, gbe_key_left, gbe_key_right, gbe_key_r_trigger, gbe_key_l_trigger;
	extern u32 gbe_joy_a, gbe_joy_b, gbe_joy_x, gbe_joy_y, gbe_joy_start, gbe_joy_select, gbe_joy_up, gbe_joy_down, gbe_joy_left, gbe_joy_right, gbe_joy_r_trigger, gbe_joy_l_trigger;

	extern u32 con_key_up, con_key_down, con_key_left, con_key_right, con_key_1, con_key_2;
	extern u32 con_joy_up, con_joy_down, con_joy_left, con_joy_right, con_joy_1, con_joy_2;

	extern int touch_zone_x[10];
	extern int touch_zone_y[10];
	extern int touch_zone_pad[10];
	extern u8 touch_mode;

	extern u32 hotkey_turbo;
	extern u32 hotkey_mute;
	extern u32 hotkey_camera;
	extern u32 hotkey_swap_screen;
	extern u32 hotkey_shift_screen;
	extern int dead_zone;
	extern int joy_id;
	extern int joy_sdl_id;
	extern bool use_haptics;

	extern bool use_motion;
	extern float motion_dead_zone;
	extern float motion_scaler;

	extern u32 flags;
	extern bool pause_emu;
	extern bool use_bios;
	extern bool use_firmware;
	extern bool no_cart;
	extern bool ignore_illegal_opcodes;

	extern special_cart_types cart_type;
	extern gba_save_types agb_save_type;

	extern u32 sio_device;
	extern u32 ir_device;	
	extern bool use_opengl;
	extern bool use_debugger;
	extern bool turbo;
	extern u8 scaling_factor;
	extern u8 old_scaling_factor;
	extern std::stringstream title;
	extern u8 gb_type;
	extern bool gba_enhance;
	extern bool sdl_render;
	extern u8 dmg_gbc_pal;
	extern u16 mpos_id;
	extern u32 utp_steps;
	extern u32 magic_reader_id;

	extern u8 nds_slot1_device;
	extern u8 nds_slot2_device;
	extern std::string nds_slot2_file;

	extern u8 min_config;

	extern bool use_cheats;
	extern std::vector <u32> gs_cheats;
	extern std::vector <std::string> gg_cheats;
	extern std::vector <std::string> gsa_cheats;
	extern std::vector <std::string> cheats_info;
	extern bool use_patches;

	extern bool use_netplay;
	extern bool netplay_hard_sync;
	extern bool use_net_gate;
	extern bool use_real_gbma_server;
	extern u32 netplay_sync_threshold;
	extern u16 netplay_server_port;
	extern u16 netplay_client_port;
	extern u8 netplay_id;
	extern std::string netplay_client_ip;

	extern std::string gbma_server;
	extern bool use_real_gbma_server;
	extern u16 gbma_server_http_port;

	extern u8 volume;
	extern u8 old_volume;
	extern u32 sample_size;
	extern double sample_rate;
	extern bool mute;
	extern bool use_stereo;
	extern bool use_microphone;
	extern std::string override_audio_driver;
	
	extern u32 sys_width;
	extern u32 sys_height;
	extern s32 win_width;
	extern s32 win_height;

	extern std::string vertex_shader;
	extern std::string fragment_shader;

	extern bool request_resize;
	extern s8 resize_mode;
	extern bool maintain_aspect_ratio;
	extern u8 lcd_config;
	extern u16 max_fps;

	extern u32 DMG_BG_PAL[4];
	extern u32 DMG_OBJ_PAL[4][2];

	extern u16 rtc_offset[6];
	extern u32 oc_flags;
	extern u32 ir_db_index;

	extern u16 battle_chip_id;
	extern u16 chip_list[6];

	extern u8 turbo_file_options;

	extern u8 mw_data[6];

	extern bool auto_gen_am3_id;
	extern bool use_am3_folder;

	extern u32 jukebox_total_time;
	extern std::string temp_media_file;
	extern std::string temp_karaoke_file;
	extern std::string audio_conversion_cmd;
	extern std::string remove_vocals_cmd;

	extern bool use_osd;
	extern std::vector <u32> osd_font;
	extern std::string osd_message;
	extern u32 osd_count;

	extern bool use_external_interfaces;

	extern bool vc_enable;
	extern std::string vc_file;
	extern std::vector <u32> vc_data;
	extern u32 vc_wait;
	extern u32 vc_timeout;
	extern u8 vc_opacity;

	//Function pointer for external software rendering
	//This function is provided by frontends that will not rely on SDL
	extern void (*render_external_sw)(std::vector<u32>&);

	//Function pointer for external rendering
	//This function is provided by frontends that will not rely on SDL+OGL
	extern void (*render_external_hw)(SDL_Surface*);

	//Function pointer for external debugging
	//This function is provided by frontends that will not rely on the CLI
	extern void (*debug_external)();
}

#endif // EMU_CONFIG
