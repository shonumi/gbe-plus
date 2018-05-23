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
void draw_osd_msg(std::vector <u32> &osd_surface);

enum special_cart_types
{
	NORMAL_CART,
	DMG_MBC1M,
	DMG_MMM01,
	AGB_RTC,
	AGB_SOLAR_SENSOR,
	AGB_RUMBLE,
	AGB_GYRO_SENSOR,
	AGB_TILT_SENSOR,
};

enum gba_save_types
{
	AGB_AUTO_DETECT,
	AGB_NO_SAVE,
	AGB_SRAM,
	AGB_EEPROM,
	AGB_FLASH64,
	AGB_FLASH128,
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
	extern std::string save_path;
	extern std::string ss_path;
	extern std::string cfg_path;
	extern std::string data_path;
	extern std::string cheats_path;
	extern std::string external_camera_file;
	extern std::string external_card_file;
	extern std::vector <std::string> recent_files;
	extern std::vector <std::string> cli_args;

	extern int ntr_key_a, ntr_key_b, ntr_key_x, ntr_key_y, ntr_key_start, ntr_key_select, ntr_key_up, ntr_key_down, ntr_key_left, ntr_key_right, ntr_key_r_trigger, ntr_key_l_trigger;
	extern int ntr_joy_a, ntr_joy_b, ntr_joy_x, ntr_joy_y, ntr_joy_start, ntr_joy_select, ntr_joy_up, ntr_joy_down, ntr_joy_left, ntr_joy_right, ntr_joy_r_trigger, ntr_joy_l_trigger;

	extern int agb_key_a, agb_key_b, agb_key_start, agb_key_select, agb_key_up, agb_key_down, agb_key_left, agb_key_right, agb_key_r_trigger, agb_key_l_trigger;
	extern int agb_joy_a, agb_joy_b, agb_joy_start, agb_joy_select, agb_joy_up, agb_joy_down, agb_joy_left, agb_joy_right, agb_joy_r_trigger, agb_joy_l_trigger;

	extern int dmg_key_a, dmg_key_b, dmg_key_start, dmg_key_select, dmg_key_up, dmg_key_down, dmg_key_left, dmg_key_right;
	extern int dmg_joy_a, dmg_joy_b, dmg_joy_start, dmg_joy_select, dmg_joy_up, dmg_joy_down, dmg_joy_left, dmg_joy_right;

	extern int con_key_up, con_key_down, con_key_left, con_key_right;
	extern int con_joy_up, con_joy_down, con_joy_left, con_joy_right;

	extern int touch_zone_x[10];
	extern int touch_zone_y[10];
	extern int touch_zone_pad[10];

	extern int hotkey_turbo;
	extern int hotkey_mute;
	extern int hotkey_camera;
	extern int dead_zone;
	extern int joy_id;
	extern int joy_sdl_id;
	extern bool use_haptics;

	extern u32 flags;
	extern bool pause_emu;
	extern bool use_bios;
	extern bool use_firmware;

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

	extern bool use_cheats;
	extern std::vector <u32> gs_cheats;
	extern std::vector <std::string> gg_cheats;
	extern std::vector <std::string> gsa_cheats;
	extern std::vector <std::string> cheats_info;
	extern bool use_patches;

	extern bool use_netplay;
	extern bool netplay_hard_sync;
	extern u32 netplay_sync_threshold;
	extern u16 netplay_server_port;
	extern u16 netplay_client_port;
	extern std::string netplay_client_ip;

	extern u8 volume;
	extern u8 old_volume;
	extern double sample_rate;
	extern bool mute;
	extern bool use_stereo;
	
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

	extern u32 DMG_BG_PAL[4];
	extern u32 DMG_OBJ_PAL[4][2];

	extern u16 rtc_offset[6];
	extern u32 oc_flags;
	extern u32 ir_db_index;

	extern bool use_osd;
	extern std::vector <u32> osd_font;
	extern std::string osd_message;
	extern u32 osd_count;

	extern bool use_external_interfaces;

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
