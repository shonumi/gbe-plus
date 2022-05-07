// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : config.cpp
// Date : September 20, 2014
// Description : GBE+ configuration options
//
// Parses command-line arguments to configure GBE options

#include <iostream>
#include <fstream>

#include <cstdlib>

#include "config.h"
#include "cgfx_common.h"
#include "util.h"

namespace config
{
	std::string rom_file = "";
	std::string bios_file = "";
	std::string save_file = "";
	std::string dmg_bios_path = "";
	std::string gbc_bios_path = "";
	std::string agb_bios_path = "";
	std::string nds7_bios_path = "";
	std::string nds9_bios_path = "";
	std::string nds_firmware_path = "";
	std::string min_bios_path = "";
	std::string save_path = "";
	std::string ss_path = "";
	std::string cfg_path = "";
	std::string data_path = "";
	std::string cheats_path = "";
	std::string external_camera_file = "";
	std::string external_card_file = "";
	std::string external_image_file = "";
	std::string external_data_file = "";
	std::vector <std::string> recent_files;
	std::vector <std::string> cli_args;
	bool use_debugger = false;

	//Default keyboard bindings
	//Arrow Z = A button, X = B button, START = Return, Select = Space
	//UP, LEFT, DOWN, RIGHT = Arrow keys
	//A key = Left Shoulder, S key = Right Shoulder
	u32 gbe_key_a = SDLK_z; u32 gbe_key_b = SDLK_x; u32 gbe_key_x = SDLK_d; u32 gbe_key_y = SDLK_c; u32 gbe_key_start = SDLK_RETURN; u32 gbe_key_select = SDLK_SPACE;
	u32 gbe_key_l_trigger = SDLK_a; u32 gbe_key_r_trigger = SDLK_s;
	u32 gbe_key_left = SDLK_LEFT; u32 gbe_key_right = SDLK_RIGHT; u32 gbe_key_down = SDLK_DOWN; u32 gbe_key_up = SDLK_UP;

	//Default joystick bindings
	u32 gbe_joy_a = 100; u32 gbe_joy_b = 101; u32 gbe_joy_x = 102; u32 gbe_joy_y = 103; u32 gbe_joy_start = 107; u32 gbe_joy_select = 106;
	u32 gbe_joy_r_trigger = 105; u32 gbe_joy_l_trigger = 104;
	u32 gbe_joy_left = 200; u32 gbe_joy_right = 201; u32 gbe_joy_up = 202; u32 gbe_joy_down = 203;

	//Default keyboard bindings - Context
	//Left = 4 (numpad), Right = 6 (numpad), Up = 8 (numpad), Down = 2 (numpad)
	//Con1 = 7 (numpad), Con2 = 9 (numpad)
	u32 con_key_left = 260; u32 con_key_right = 262; u32 con_key_up = 264; u32 con_key_down = 258; u32 con_key_1 = 263; u32 con_key_2 = 265;

	//Default joystick bindings - Context
	u32 con_joy_left = 204; u32 con_joy_right = 205; u32 con_joy_up = 206; u32 con_joy_down = 207; u32 con_joy_1 = 109; u32 con_joy_2 = 110;

	//Default NDS touch zone mappings
	int touch_zone_x[10] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
	int touch_zone_y[10] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
	int touch_zone_pad[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	//Default NDS touch mode (light pressure)
	u8 touch_mode = 0;

	//Hotkey bindings
	//Turbo = TAB
	u32 hotkey_turbo = SDLK_TAB;
	u32 hotkey_mute = SDLK_m;
	u32 hotkey_camera = SDLK_p;
	u32 hotkey_swap_screen = SDLK_F4;
	u32 hotkey_shift_screen = SDLK_F3;

	//Default joystick dead-zone
	int dead_zone = 16000;

	//Default joystick ID
	int joy_id = 0;
	int joy_sdl_id = 0;

	//Default Haptic setting
	bool use_haptics = false;

	u32 flags = 0x4;
	bool pause_emu = false;
	bool use_bios = false;
	bool use_firmware = false;
	bool no_cart = false;
	bool ignore_illegal_opcodes = true;

	special_cart_types cart_type = NORMAL_CART;
	gba_save_types agb_save_type = AGB_AUTO_DETECT;

	u32 sio_device = 0;
	u32 ir_device = 0;
	u16 mpos_id = 0;
	u32 utp_steps = 0;
	u32 magic_reader_id = 0x500000;
	bool use_opengl = false;
	bool turbo = false;

	std::string vertex_shader = "vertex.vs";
	std::string fragment_shader = "fragment.fs";

	u8 scaling_factor = 1;
	u8 old_scaling_factor = 1;

	std::stringstream title;

	//Cheats - Gameshark and Game Genie (DMG-GBC), Gameshark - GBA
	bool use_cheats = false;
	std::vector <u32> gs_cheats;
	std::vector <std::string> gg_cheats;
	std::vector <std::string> gsa_cheats;
	std::vector <std::string> cheats_info;

	//Patches
	bool use_patches = false;

	//Netplay settings
	bool use_netplay = true;
	bool netplay_hard_sync = true;
	bool use_net_gate = false;
	u32 netplay_sync_threshold = 32;
	u16 netplay_server_port = 2000;
	u16 netplay_client_port = 2001;
	u8 netplay_id = 0;
	std::string netplay_client_ip = "127.0.0.1";

	bool use_real_gbma_server = false;
	std::string gbma_server = "127.0.0.1";
	u16 gbma_server_http_port = 8000;

	u8 dmg_gbc_pal = 0;

	//Emulated Gameboy type
	//TODO - Make this an enum
	//0 - DMG, 1 - DMG on GBC, 2 - GBC, 3 - GBA, 4 - NDS????
	u8 gb_type = 0;

	//Boolean dictating whether this is a DMG/GBC game on a GBA
	bool gba_enhance = false;

	//Variables dictating whether or not to stretch DMG/GBC games when playing on a GBA
	bool request_resize = false;
	s8 resize_mode = 0;

	//Aspect ratio
	bool maintain_aspect_ratio = false;

	//LCD configuration (NDS primarily)
	u8 lcd_config = 0;

	//Max FPS
	u16 max_fps = 0;

	//Sound parameters
	u8 volume = 128;
	u8 old_volume = 0;
	u32 sample_size = 0;
	double sample_rate = 44100.0;
	bool mute = false;
	bool use_stereo = false;

	//Virtual Cursor parameters for NDS
	bool vc_enable = false;
	std::string vc_file = "";
	std::vector <u32> vc_data;
	u32 vc_wait = 1;
	u32 vc_timeout = 180;
	u8 vc_opacity = 31;

	//System screen sizes
	u32 sys_width = 0;
	u32 sys_height = 0;

	//Window screen sizes
	s32 win_width = 0;
	s32 win_height = 0;

	bool sdl_render = true;

	bool use_external_interfaces = false;

	void (*render_external_sw)(std::vector<u32>&);
	void (*render_external_hw)(SDL_Surface*);
	void (*debug_external)();

	//Default Gameboy BG palettes
	u32 DMG_BG_PAL[4] = { 0xFFFFFFFF, 0xFFC0C0C0, 0xFF606060, 0xFF000000 };

	u32 DMG_OBJ_PAL[4][2] = 
	{ 
		{ 0xFFFFFFFF, 0xFFFFFFFF },
		{ 0xFFC0C0C0, 0xFFC0C0C0 },
		{ 0xFF606060, 0xFF606060 },
		{ 0xFF000000, 0xFF000000 }
	};

	//NDS Slot-2 device and file
	u8 nds_slot1_device = 0;
	u8 nds_slot2_device = 0;
	std::string nds_slot2_file = "";

	//Pokemon Mini flags
	u8 min_config = 0x7;

	//Real-time clock offsets
	u16 rtc_offset[6] = { 0, 0, 0, 0, 0, 0 };

	//CPU overclocking flags
	u32 oc_flags = 0;

	//IR database index
	u32 ir_db_index = 0;

	//Battle Chip ID for Megaman Battle Network games + Chip Gates
	u16 battle_chip_id = 0;

	//Default Battle Chip IDs
	u16 chip_list[6] = { 0, 0, 0, 0, 0, 0 };

	//Turbo File options flags
	u8 turbo_file_options = 0;

	//Magical Watch Data
	u8 mw_data[6] = { 0, 0, 0, 0, 0, 0 };

	//AM3 DES Key Auto Generate Flag
	bool auto_gen_am3_key = false;

	//On-screen display settings
	bool use_osd = false;
	std::vector <u32> osd_font;
	std::string osd_message = "";
	u32 osd_count = 0;
}

/****** Reset DMG default colors ******/
void reset_dmg_colors()
{
	config::DMG_BG_PAL[0] = 0xFFFFFFFF;
	config::DMG_BG_PAL[1] = 0xFFC0C0C0;
	config::DMG_BG_PAL[2] = 0xFF606060;
	config::DMG_BG_PAL[3] = 0xFF000000;

	config::DMG_OBJ_PAL[0][0] = 0xFFFFFFFF;
	config::DMG_OBJ_PAL[1][0] = 0xFFC0C0C0;
	config::DMG_OBJ_PAL[2][0] = 0xFF606060;
	config::DMG_OBJ_PAL[3][0] = 0xFF000000;

	config::DMG_OBJ_PAL[0][1] = 0xFFFFFFFF;
	config::DMG_OBJ_PAL[1][1] = 0xFFC0C0C0;
	config::DMG_OBJ_PAL[2][1] = 0xFF606060;
	config::DMG_OBJ_PAL[3][1] = 0xFF000000;
}

/****** Set DMG colors based on GBC BIOS ******/
void set_dmg_colors(u8 color_type)
{
	switch(color_type)
	{
		//Default palette
		case 0:
			reset_dmg_colors();
			break;

		//No input
		case 1:
			config::DMG_BG_PAL[0] = 0xFFF8F8F8;
			config::DMG_BG_PAL[1] = 0xFF78F830;
			config::DMG_BG_PAL[2] = 0xFF0060C0;
			config::DMG_BG_PAL[3] = 0xFF000000;

			config::DMG_OBJ_PAL[0][0] = 0xFFF8F8F8;
			config::DMG_OBJ_PAL[1][0] = 0xFFF88080;
			config::DMG_OBJ_PAL[2][0] = 0xFF903838;
			config::DMG_OBJ_PAL[3][0] = 0xFF000000;

			config::DMG_OBJ_PAL[0][1] = 0xFFF8F8F8;
			config::DMG_OBJ_PAL[1][1] = 0xFFF88080;
			config::DMG_OBJ_PAL[2][1] = 0xFF903838;
			config::DMG_OBJ_PAL[3][1] = 0xFF000000;

			break;

		//UP
		case 2:
			config::DMG_BG_PAL[0] = 0xFFF8F8F8;
			config::DMG_BG_PAL[1] = 0xFFF8A860;
			config::DMG_BG_PAL[2] = 0xFF803000;
			config::DMG_BG_PAL[3] = 0xFF000000;

			config::DMG_OBJ_PAL[0][0] = 0xFFF8F8F8;
			config::DMG_OBJ_PAL[1][0] = 0xFFF8A860;
			config::DMG_OBJ_PAL[2][0] = 0xFF803000;
			config::DMG_OBJ_PAL[3][0] = 0xFF000000;

			config::DMG_OBJ_PAL[0][1] = 0xFFF8F8F8;
			config::DMG_OBJ_PAL[1][1] = 0xFFF8A860;
			config::DMG_OBJ_PAL[2][1] = 0xFF803000;
			config::DMG_OBJ_PAL[3][1] = 0xFF000000;

			break;

		//DOWN
		case 3:
			config::DMG_BG_PAL[0] = 0xFFF8F8A0;
			config::DMG_BG_PAL[1] = 0xFFF89090;
			config::DMG_BG_PAL[2] = 0xFF9090F8;
			config::DMG_BG_PAL[3] = 0xFF000000;

			config::DMG_OBJ_PAL[0][0] = 0xFFF8F8A0;
			config::DMG_OBJ_PAL[1][0] = 0xFFF89090;
			config::DMG_OBJ_PAL[2][0] = 0xFF9090F8;
			config::DMG_OBJ_PAL[3][0] = 0xFF000000;

			config::DMG_OBJ_PAL[0][1] = 0xFFF8F8A0;
			config::DMG_OBJ_PAL[1][1] = 0xFFF89090;
			config::DMG_OBJ_PAL[2][1] = 0xFF9090F8;
			config::DMG_OBJ_PAL[3][1] = 0xFF000000;

			break;

		//LEFT
		case 4:
			config::DMG_BG_PAL[0] = 0xFFF8F8F8;
			config::DMG_BG_PAL[1] = 0xFF60A0F8;
			config::DMG_BG_PAL[2] = 0xFF0000F8;
			config::DMG_BG_PAL[3] = 0xFF000000;

			config::DMG_OBJ_PAL[0][0] = 0xFFF8F8F8;
			config::DMG_OBJ_PAL[1][0] = 0xFFF88080;
			config::DMG_OBJ_PAL[2][0] = 0xFF903838;
			config::DMG_OBJ_PAL[3][0] = 0xFF000000;

			config::DMG_OBJ_PAL[0][1] = 0xFFF8F8F8;
			config::DMG_OBJ_PAL[1][1] = 0xFF78F830;
			config::DMG_OBJ_PAL[2][1] = 0xFF008000;
			config::DMG_OBJ_PAL[3][1] = 0xFF000000;

			break;

		//RIGHT
		case 5:
			config::DMG_BG_PAL[0] = 0xFFF8F8F8;
			config::DMG_BG_PAL[1] = 0xFF50F800;
			config::DMG_BG_PAL[2] = 0xFFF84000;
			config::DMG_BG_PAL[3] = 0xFF000000;

			config::DMG_OBJ_PAL[0][0] = 0xFFF8F8F8;
			config::DMG_OBJ_PAL[1][0] = 0xFF50F800;
			config::DMG_OBJ_PAL[2][0] = 0xFFF84000;
			config::DMG_OBJ_PAL[3][0] = 0xFF000000;

			config::DMG_OBJ_PAL[0][1] = 0xFFF8F8F8;
			config::DMG_OBJ_PAL[1][1] = 0xFF50F800;
			config::DMG_OBJ_PAL[2][1] = 0xFFF84000;
			config::DMG_OBJ_PAL[3][1] = 0xFF000000;

			break;

		//UP + A
		case 6:
                        config::DMG_BG_PAL[0] = 0xFFF8F8F8;
                        config::DMG_BG_PAL[1] = 0xFFF88080;
                        config::DMG_BG_PAL[2] = 0xFF903838;
                        config::DMG_BG_PAL[3] = 0xFF000000;

                        config::DMG_OBJ_PAL[0][0] = 0xFFF8F8F8;
                        config::DMG_OBJ_PAL[1][0] = 0xFF78F830;
                        config::DMG_OBJ_PAL[2][0] = 0xFF008000;
                        config::DMG_OBJ_PAL[3][0] = 0xFF000000;

                        config::DMG_OBJ_PAL[0][1] = 0xFFF8F8F8;
                        config::DMG_OBJ_PAL[1][1] = 0xFF60A0F8;
                        config::DMG_OBJ_PAL[2][1] = 0xFF0000F8;
                        config::DMG_OBJ_PAL[3][1] = 0xFF000000;

			break;

		//DOWN + A
		case 7:
                        config::DMG_BG_PAL[0] = 0xFFF8F8F8;
                        config::DMG_BG_PAL[1] = 0xFFF8F800;
                        config::DMG_BG_PAL[2] = 0xFFF80000;
                        config::DMG_BG_PAL[3] = 0xFF000000;

                        config::DMG_OBJ_PAL[0][0] = 0xFFF8F8F8;
                        config::DMG_OBJ_PAL[1][0] = 0xFFF8F800;
                        config::DMG_OBJ_PAL[2][0] = 0xFFF80000;
                        config::DMG_OBJ_PAL[3][0] = 0xFF000000;

                        config::DMG_OBJ_PAL[0][1] = 0xFFF8F8F8;
                        config::DMG_OBJ_PAL[1][1] = 0xFFF8F800;
                        config::DMG_OBJ_PAL[2][1] = 0xFFF80000;
                        config::DMG_OBJ_PAL[3][1] = 0xFF000000;

			break;

		//LEFT + A
		case 8:
                        config::DMG_BG_PAL[0] = 0xFFF8F8F8;
                        config::DMG_BG_PAL[1] = 0xFF8888D8;
                        config::DMG_BG_PAL[2] = 0xFF505088;
                        config::DMG_BG_PAL[3] = 0xFF000000;

                        config::DMG_OBJ_PAL[0][0] = 0xFFF8F8F8;
                        config::DMG_OBJ_PAL[1][0] = 0xFFF88080;
                        config::DMG_OBJ_PAL[2][0] = 0xFF903838;
                        config::DMG_OBJ_PAL[3][0] = 0xFF000000;

                        config::DMG_OBJ_PAL[0][1] = 0xFFF8F8F8;
                        config::DMG_OBJ_PAL[1][1] = 0xFFF8A860;
                        config::DMG_OBJ_PAL[2][1] = 0xFF803000;
                        config::DMG_OBJ_PAL[3][1] = 0xFF000000;

			break;

		//RIGHT + A
		case 9:
                        config::DMG_BG_PAL[0] = 0xFFF8F8F8;
                        config::DMG_BG_PAL[1] = 0xFF78F830;
                        config::DMG_BG_PAL[2] = 0xFF0060C0;
                        config::DMG_BG_PAL[3] = 0xFF000000;

                        config::DMG_OBJ_PAL[0][0] = 0xFFF8F8F8;
                        config::DMG_OBJ_PAL[1][0] = 0xFFF88080;
                        config::DMG_OBJ_PAL[2][0] = 0xFF903838;
                        config::DMG_OBJ_PAL[3][0] = 0xFF000000;

                        config::DMG_OBJ_PAL[0][1] = 0xFFF8F8F8;
                        config::DMG_OBJ_PAL[1][1] = 0xFFF88080;
                        config::DMG_OBJ_PAL[2][1] = 0xFF903838;
                        config::DMG_OBJ_PAL[3][1] = 0xFF000000;

			break;

		//UP + B
		case 10:
                        config::DMG_BG_PAL[0] = 0xFFF8E0C0;
                        config::DMG_BG_PAL[1] = 0xFFC89880;
                        config::DMG_BG_PAL[2] = 0xFF806828;
                        config::DMG_BG_PAL[3] = 0xFF583008;

                        config::DMG_OBJ_PAL[0][0] = 0xFFF8F8F8;
                        config::DMG_OBJ_PAL[1][0] = 0xFFF8A860;
                        config::DMG_OBJ_PAL[2][0] = 0xFF803000;
                        config::DMG_OBJ_PAL[3][0] = 0xFF000000;

                        config::DMG_OBJ_PAL[0][1] = 0xFFF8F8F8;
                        config::DMG_OBJ_PAL[1][1] = 0xFFF8A860;
                        config::DMG_OBJ_PAL[2][1] = 0xFF803000;
                        config::DMG_OBJ_PAL[3][1] = 0xFF000000;

			break;

		//DOWN + B
		case 11:
                        config::DMG_BG_PAL[0] = 0xFFF8F8F8;
                        config::DMG_BG_PAL[1] = 0xFFF8F800;
                        config::DMG_BG_PAL[2] = 0xFF784800;
                        config::DMG_BG_PAL[3] = 0xFF000000;

                        config::DMG_OBJ_PAL[0][0] = 0xFFF8F8F8;
                        config::DMG_OBJ_PAL[1][0] = 0xFF60A0F8;
                        config::DMG_OBJ_PAL[2][0] = 0xFF0000F8;
                        config::DMG_OBJ_PAL[3][0] = 0xFF000000;

                        config::DMG_OBJ_PAL[0][1] = 0xFFF8F8F8;
                        config::DMG_OBJ_PAL[1][1] = 0xFF78F830;
                        config::DMG_OBJ_PAL[2][1] = 0xFF008000;
                        config::DMG_OBJ_PAL[3][1] = 0xFF000000;

			break;

		//LEFT + B
		case 12:
                        config::DMG_BG_PAL[0] = 0xFFF8F8F8;
                        config::DMG_BG_PAL[1] = 0xFFA0A0A0;
                        config::DMG_BG_PAL[2] = 0xFF505050;
                        config::DMG_BG_PAL[3] = 0xFF000000;

                        config::DMG_OBJ_PAL[0][0] = 0xFFF8F8F8;
                        config::DMG_OBJ_PAL[1][0] = 0xFFA0A0A0;
                        config::DMG_OBJ_PAL[2][0] = 0xFF505050;
                        config::DMG_OBJ_PAL[3][0] = 0xFF000000;

                        config::DMG_OBJ_PAL[0][1] = 0xFFF8F8F8;
                        config::DMG_OBJ_PAL[1][1] = 0xFFA0A0A0;
                        config::DMG_OBJ_PAL[2][1] = 0xFF505050;
                        config::DMG_OBJ_PAL[3][1] = 0xFF000000;

			break;

		//RIGHT + B
		case 13:
                        config::DMG_BG_PAL[0] = 0xFF000000;
                        config::DMG_BG_PAL[1] = 0xFF008080;
                        config::DMG_BG_PAL[2] = 0xFFF8D800;
                        config::DMG_BG_PAL[3] = 0xFFF8F8F8;

                        config::DMG_OBJ_PAL[0][0] = 0xFF000000;
                        config::DMG_OBJ_PAL[1][0] = 0xFF008080;
                        config::DMG_OBJ_PAL[2][0] = 0xFFF8D800;
                        config::DMG_OBJ_PAL[3][0] = 0xFFF8F8F8;

                        config::DMG_OBJ_PAL[0][1] = 0xFF000000;
                        config::DMG_OBJ_PAL[1][1] = 0xFF008080;
                        config::DMG_OBJ_PAL[2][1] = 0xFFF8D800;
                        config::DMG_OBJ_PAL[3][1] = 0xFFF8F8F8;

			break;

		//Classic Green
		case 14:
                        config::DMG_BG_PAL[3] = 0xFF091921;
                        config::DMG_BG_PAL[2] = 0xFF316B52;
                        config::DMG_BG_PAL[1] = 0xFF8AC573;
                        config::DMG_BG_PAL[0] = 0xFFE6FFD6;

                        config::DMG_OBJ_PAL[3][0] = 0xFF091921;
                        config::DMG_OBJ_PAL[2][0] = 0xFF316B52;
                        config::DMG_OBJ_PAL[1][0] = 0xFF8AC573;
                        config::DMG_OBJ_PAL[0][0] = 0xFFE6FFD6;

                        config::DMG_OBJ_PAL[3][1] = 0xFF091921;
                        config::DMG_OBJ_PAL[2][1] = 0xFF316B52;
                        config::DMG_OBJ_PAL[1][1] = 0xFF8AC573;
                        config::DMG_OBJ_PAL[0][1] = 0xFFE6FFD6;

			break;

		//Game Boy Light
		case 15:
			config::DMG_BG_PAL[0] = 0xFF38D5AA;
			config::DMG_BG_PAL[1] = 0xFF40B38C;
			config::DMG_BG_PAL[2] = 0xFF39A37D;
			config::DMG_BG_PAL[3] = 0xFF008E63;

			config::DMG_OBJ_PAL[0][0] = 0xFF38D5AA;
			config::DMG_OBJ_PAL[1][0] = 0xFF40B38C;
			config::DMG_OBJ_PAL[2][0] = 0xFF39A37D;
			config::DMG_OBJ_PAL[3][0] = 0xFF008E63;

			config::DMG_OBJ_PAL[0][1] = 0xFF38D5AA;
			config::DMG_OBJ_PAL[1][1] = 0xFF40B38C;
			config::DMG_OBJ_PAL[2][1] = 0xFF39A37D;
			config::DMG_OBJ_PAL[3][1] = 0xFF008E63;
	}
}	

/****** Validates emulated system type ******/
void validate_system_type()
{
	if(config::rom_file.empty()) { return; }
	if((config::rom_file == "NOCART") || config::no_cart) { return; }
	if((config::rom_file == "-h") || (config::rom_file == "--help")) { config::cli_args.push_back(config::rom_file); return; }

	//When loading AM3 files, force system type to GBA
	if(config::cart_type == AGB_AM3)
	{
		config::gb_type = 3;
		return;
	}

	//Determine Gameboy type based on file name
	//Note, DMG and GBC games are automatically detected in the Gameboy MMU, so only check for GBA types here
	std::size_t dot = config::rom_file.find_last_of(".");
	
	if(dot == std::string::npos)
	{
		std::cout<<"GBE::Warning - Could not determine the emulated system type for file " << config::rom_file << "\n";
		return;
	}

	std::string ext = config::rom_file.substr(dot);

	//Convert extension to lower-case ASCII
	for(u32 x = 0; x < ext.length(); x++)
	{
		if((ext[x] >= 0x41) && (ext[x] <= 0x5A)) { ext[x] += 0x20; }
	}

	if(ext == ".gba") { config::gb_type = 3; }
	else if(ext == ".nds") { config::gb_type = 4; }
	else if(ext == ".min") { config::gb_type = 7; }

	//Force GBC mode if system type is set to GBA, but a GB/GBC game is loaded
	else if((ext != ".gba") && (config::gb_type == 3)) 
	{
		config::gb_type = 2;
		config::gba_enhance = true;
	}
}

/****** Returns the emulated system type from a given filename ******/
u8 get_system_type_from_file(std::string filename)
{
	//Determine if no cart is inserted
	if(filename == "NOCART")
	{
		config::no_cart = true;
		return config::gb_type;
	}

	//When loading AM3 files, force system type to GBA
	if(config::cart_type == AGB_AM3)
	{
		config::gb_type = 3;
		return config::gb_type;
	}

	//Determine Gameboy type based on file name
	std::size_t dot = filename.find_last_of(".");

	if(dot == std::string::npos) { return 0; }

	std::string ext = filename.substr(dot);
	
	//Convert extension to lower-case ASCII
	for(u32 x = 0; x < ext.length(); x++)
	{
		if((ext[x] >= 0x41) && (ext[x] <= 0x5A)) { ext[x] += 0x20; }
	}

	u8 gb_type = config::gb_type;

	if(ext == ".gba") { gb_type = 3; }
	else if(ext == ".nds") { gb_type = 4; }
	else if(ext == ".min") { gb_type = 7; }
	else if((ext != ".gba") && (gb_type == 3)) { gb_type = 2; }

	//For Auto or GBC mode, determine what the CGB Flag is
	if((gb_type == 0) || (gb_type == 2) || (gb_type == 5) || (gb_type == 6))
	{
		std::ifstream test_stream(filename.c_str(), std::ios::binary);
		
		if(test_stream.is_open())
		{
			u8 color_byte;
			u8 sgb_byte;

			test_stream.seekg(0x143);
			test_stream.read((char*)&color_byte, 1);

			//If GBC compatible, use GBC mode. Otherwise, use DMG mode
			if((color_byte == 0xC0) || (color_byte == 0x80)) { gb_type = 2; }
			else { gb_type = 1; }

			test_stream.seekg(0x146);
			test_stream.read((char*)&sgb_byte, 1);

			//If SGB compatible, use it if SGB set as the system
			if((sgb_byte == 0x3) && (config::gb_type == 5)) { gb_type = 5; }
			else if((sgb_byte == 0x3) && (config::gb_type == 6)) { gb_type = 6; }

			test_stream.close();
		}
	}

	return gb_type;
}

/****** Parse arguments passed from the command-line ******/
bool parse_cli_args()
{
	//If no arguments were passed, cannot run without ROM file
	//If using external interfaces (e.g. the GUI), a ROM file is not necessary
	if((config::cli_args.size() < 1) && (!config::use_external_interfaces))
	{
		std::cout<<"GBE::Error - No ROM file in arguments \n";
		return false;
	}

	else 
	{
		//Parse the rest of the arguments if any		
		for(int x = 1; x < config::cli_args.size(); x++)
		{	
			//Run GBE+ in debug mode
			if((config::cli_args[x] == "-d") || (config::cli_args[x] == "--debug")) { config::use_debugger = true; }

			//Load BIOS
			else if((config::cli_args[x] == "-b") || (config::cli_args[x] == "--bios")) 
			{
				if((++x) == config::cli_args.size()) { std::cout<<"GBE::Error - No BIOS file in arguments\n"; }

				else 
				{
					config::use_bios = true; 
					config::bios_file = config::cli_args[x];

					//For the NDS, read 1st argument as NDS7 BIOS, 2nd as NDS9 BIOS
					if(config::gb_type == 4)
					{
						if((++x) == config::cli_args.size())
						{
							std::cout<<"GBE::Error - No NDS9 BIOS file in arguments\n";
							config::use_bios = false;
						}

						else
						{
							config::nds7_bios_path = config::bios_file;
							config::nds9_bios_path = config::cli_args[x];
						} 
					}
				}
			}

			//Load NDS Slot-2 device
			else if(config::cli_args[x] == "--slot2-gba")
			{
				if((++x) == config::cli_args.size()) { std::cout<<"GBE::Error - No GBA ROM file in arguments\n"; }

				else 
				{
					config::nds_slot2_device = 4;
					config::nds_slot2_file = config::cli_args[x];
				}
			}

			//Load Firmware (NDS)
			else if((config::cli_args[x] == "-fw") || (config::cli_args[x] == "--firmware"))
			{
				if((++x) == config::cli_args.size()) { std::cout<<"GBE::Error - No firmware file in arguments\n"; }

				else
				{
					config::use_firmware = true;
					config::nds_firmware_path = config::cli_args[x];
				}
			}

			//Set maximum FPS
			else if((config::cli_args[x] == "-mf") || (config::cli_args[x] == "--max-fps"))
			{
				if((++x) == config::cli_args.size()) { std::cout<<"GBE::Error - No maximum framerate set\n"; }

				else
				{
					u32 output = 0;
					util::from_str(config::cli_args[x], output);
					config::max_fps = output;
				}
			}

			//Enable fullscreen mode
			else if((config::cli_args[x] == "-f") || (config::cli_args[x] == "--fullscreen")) { config::flags |= SDL_WINDOW_FULLSCREEN_DESKTOP; } 

			//Use MBC1M multicart mode if applicable for a given ROM
			else if(config::cli_args[x] == "--mbc1m") { config::cart_type = DMG_MBC1M; }

			//Use MBC1S if applicable for Pocket Sonar
			else if(config::cli_args[x] == "--mbc1s") { config::cart_type = DMG_MBC1S; }

			//Use MMM01 multicart mode if applicable for a given ROM
			else if(config::cli_args[x] == "--mmm01") { config::cart_type = DMG_MMM01; }

			//Use MBC30 (double SRAM) for Pocket Monsters Crystal
			else if(config::cli_args[x] == "--mbc30") { config::cart_type = DMG_MBC30; }

			//Use GB Memory Cartridge mapper
			else if(config::cli_args[x] == "--gbmem") { config::cart_type = DMG_GBMEM; }

			//Use GBA RTC for a given ROM
			else if(config::cli_args[x] == "--agb-rtc") { config::cart_type = AGB_RTC; }
			
			//Use GBA solar sensor for a given ROM
			else if(config::cli_args[x] == "--agb-solar-sensor") { config::cart_type = AGB_SOLAR_SENSOR; }

			//Use GBA rumble for Drill Dozer
			else if(config::cli_args[x] == "--agb-rumble") { config::cart_type = AGB_RUMBLE; }

			//Use GBA gyro sensor for WarioWare: Twisted
			else if(config::cli_args[x] == "--agb-gyro-sensor") { config::cart_type = AGB_GYRO_SENSOR; }

			//Use GBA tilt sensor for Yoshi Topsy Turvy aka Universal Gravitation
			else if(config::cli_args[x] == "--agb-tilt-sensor") { config::cart_type = AGB_TILT_SENSOR; }

			//Use GBA 8M DACS for Hikaru no Go 3
			else if(config::cli_args[x] == "--agb-dacs") { config::cart_type = AGB_8M_DACS; }

			//Use GBA AM3 mapper
			else if(config::cli_args[x] == "--agb-am3") { config::cart_type = AGB_AM3; }

			//Use GBA Music Recorder mapper
			else if(config::cli_args[x] == "--agb-jukebox") { config::cart_type = AGB_JUKEBOX; }

			//Use Auto-Detect for GBA saves
			else if(config::cli_args[x] == "--save-auto") { config::agb_save_type = AGB_AUTO_DETECT; }

			//Disable all GBA saves
			else if(config::cli_args[x] == "--save-none") { config::agb_save_type = AGB_NO_SAVE; }

			//Force SRAM GBA saves
			else if(config::cli_args[x] == "--save-sram") { config::agb_save_type = AGB_SRAM; }

			//Force EEPROM GBA saves
			else if(config::cli_args[x] == "--save-eeprom") { config::agb_save_type = AGB_EEPROM; }

			//Force FLASH 64KB GBA saves
			else if(config::cli_args[x] == "--save-flash64") { config::agb_save_type = AGB_FLASH64; }

			//Force FLASH 128KB GBA saves
			else if(config::cli_args[x] == "--save-auto") { config::agb_save_type = AGB_FLASH128; }

			//Disable Pokemon Mini 3-color Mode
			else if(config::cli_args[x] == "--min-disable-colors") { config::min_config &= ~0x1; }

			//Disable Pokemon Mini RTC
			else if(config::cli_args[x] == "--min-disable-rtc") { config::min_config &= ~0x2; }

			//Enable shared EEPROM
			else if(config::cli_args[x] == "--min-shared-eeprom") { config::min_config &= ~0x4; }
			
			//Use OpenGL for screen drawing
			else if(config::cli_args[x] == "--opengl") { config::use_opengl = true; }

			//Use Gameshark or Game Genie cheats
			else if(config::cli_args[x] == "--cheats") { config::use_cheats = true; }

			else if(config::cli_args[x] == "--patch") { config::use_patches = true; }

			//Scale screen by 2x
			else if(config::cli_args[x] == "--2x") { config::scaling_factor = config::old_scaling_factor = 2; }

			//Scale screen by 3x
			else if(config::cli_args[x] == "--3x") { config::scaling_factor = config::old_scaling_factor = 3; }

			//Scale screen by 4x
			else if(config::cli_args[x] == "--4x") { config::scaling_factor = config::old_scaling_factor = 4; }

			//Scale screen by 5x
			else if(config::cli_args[x] == "--5x") { config::scaling_factor = config::old_scaling_factor = 5; }

			//Scale screen by 6x
			else if(config::cli_args[x] == "--6x") { config::scaling_factor = config::old_scaling_factor = 6; }

			//Set system type - Auto
			else if(config::cli_args[x] == "--sys-auto") { config::gb_type = 0; }

			//Set system type - DMG
			else if(config::cli_args[x] == "--sys-dmg") { config::gb_type = 1; }

			//Set system type - GBC
			else if(config::cli_args[x] == "--sys-gbc") { config::gb_type = 2; }

			//Set system type - GBA
			else if(config::cli_args[x] == "--sys-gba") { config::gb_type = 3; }

			//Set system type - NDS
			else if(config::cli_args[x] == "--sys-nds") { config::gb_type = 4; }

			//Set system type - SGB1
			else if(config::cli_args[x] == "--sys-sgb") { config::gb_type = 5; }

			//Set system type - SGB2
			else if(config::cli_args[x] == "--sys-sgb2") { config::gb_type = 6; }

			//Set system type - MIN
			else if(config::cli_args[x] == "--sys-min") { config::gb_type = 7; }

			//Enable Turbo File memory card
			else if(config::cli_args[x] == "--turbo-file-memcard") { config::turbo_file_options |= 0x1; }

			//Enable Turbo File write protection
			else if(config::cli_args[x] == "--turbo-file-protect") { config::turbo_file_options |= 0x2; }

			//Ignore Illegal Opcodes
			else if(config::cli_args[x] == "--ignore-illegal-opcodes") { config::ignore_illegal_opcodes = true; }

			//Auto Generate AM3 DES Key
			else if(config::cli_args[x] == "--auto-gen-key") { config::auto_gen_am3_key = true; }

			//Print Help
			else if((config::cli_args[x] == "-h") || (config::cli_args[x] == "--help")) 
			{
				if(!config::use_external_interfaces) { std::cout<<"\ngbe_plus file [options ...]\n\n"; }
				else { std::cout<<"\ngbe_plus_qt file [options ...]\n\n"; }

				std::cout<<"GBE+ Command Line Options:\n";
				std::cout<<"-b [FILE], --bios [FILE] \t\t Load and use BIOS file\n";
				std::cout<<"-fw [FILE], --firmware [FILE] \t\t Load and use firmware file (NDS)\n";
				std::cout<<"-d, --debug \t\t\t\t Start the command-line debugger\n";
				std::cout<<"--mbc1m \t\t\t\t Use MBC1M multicart mode if applicable\n";
				std::cout<<"--mmm01 \t\t\t\t Use MMM01 multicart mode if applicable\n";
				std::cout<<"--mbc1s \t\t\t\t Use MBC1S sonar cart\n";
				std::cout<<"--mbc30 \t\t\t\t Use MBC30 for Pocket Monsters Crystal\n";
				std::cout<<"--gbmem \t\t\t\t Use GB Memory Cartridge mapper\n";
				std::cout<<"--agb-rtc \t\t\t\t Use GBA RTC cart\n";
				std::cout<<"--agb-solar-sensor \t\t\t Use GBA Solar Sensor cart\n";
				std::cout<<"--agb-rumble \t\t\t\t Use GBA Rumble cart\n";
				std::cout<<"--agb-gyro-sensor \t\t\t Use GBA Gyro Sensor cart\n";
				std::cout<<"--agb-tilt-sensor \t\t\t Use GBA Tilt Sensor cart\n";
				std::cout<<"--agb-dacs \t\t\t Use GBA 8M DACS Flash cart\n";
				std::cout<<"--agb-am3 \t\t\t Use GBA AM3 SmartCard adapter cart\n";
				std::cout<<"--agb-jukebox \t\t\t Use GBA Music Recorder/Jukebox cart\n";
				std::cout<<"--opengl \t\t\t\t Use OpenGL for screen drawing and scaling\n";
				std::cout<<"--cheats \t\t\t\t Use Gameshark or Game Genie cheats\n";
				std::cout<<"--patch \t\t\t\t Use a patch file for the ROM\n";
				std::cout<<"--2x, --3x, --4x, --5x, --6x \t\t Scale screen by a given factor (OpenGL only)\n";
				std::cout<<"--sys-auto \t\t\t\t Set the emulated system type to AUTO\n";
				std::cout<<"--sys-dmg \t\t\t\t Set the emulated system type to DMG (old Gameboy)\n";
				std::cout<<"--sys-gbc \t\t\t\t Set the emulated system type to GBC\n";
				std::cout<<"--sys-gba \t\t\t\t Set the emulated system type to GBA\n";
				std::cout<<"--sys-nds \t\t\t\t Set the emulated system type to NDS\n";
				std::cout<<"--sys-sgb \t\t\t\t Set the emulated system type to SGB\n";
				std::cout<<"--sys-sgb2 \t\t\t\t Set the emulated system type to SGB2\n";
				std::cout<<"--sys-min \t\t\t\t Set the emulated system type to MIN\n";
				std::cout<<"--save-auto \t\t\t\t Set the GBA save type to Auto Detect\n";
				std::cout<<"--save-none \t\t\t\t Disables all GBA saves\n";
				std::cout<<"--save-sram \t\t\t\t Force the GBA save type to SRAM\n";
				std::cout<<"--save-eeprom \t\t\t\t Force the GBA save type to EEPROM\n";
				std::cout<<"--save-flash64 \t\t\t\t Force the GBA save type to FLASH 64KB\n";
				std::cout<<"--save-flash128 \t\t\t Force the GBA save type to FLASH 128KB\n";
				std::cout<<"--turbo-file-memcard \t\t\t Enable memory card for Turbo File\n";
				std::cout<<"--turbo-file-protect \t\t\t Enable write-proection for Turbo File\n";
				std::cout<<"--ignore-illegal-opcodes \t\t\t Ignore Illegal CPU instructions when running\n";
				std::cout<<"--auto-gen-key \t\t\t\t Automatically generate 16-byte DES key for AM3\n";
				std::cout<<"-h, --help \t\t\t\t Print these help messages\n";
				return false;
			}

			else
			{
				std::cout<<"GBE::Error - Unknown argument - " << config::cli_args[x] << "\n";
				return false;
			}
		}

		return true;
	}
}

/****** Parse ROM filename and save file ******/
void parse_filenames()
{
	//ROM file is always first argument
	config::rom_file = config::cli_args[0];
	config::save_file = config::rom_file + ".sav";

	validate_system_type();
}

/****** Parse options from the .ini file ******/
bool parse_ini_file()
{
	//Test for Windows or Portable version first
	//Always give preference to portable .ini settings on every OS
	std::ifstream file("gbe.ini", std::ios::in);
	config::data_path = "./data/";

	std::string input_line = "";
	std::string line_char = "";

	//Clear recent files
	config::recent_files.clear();

	//Clear existing .ini parameters
	std::vector <std::string> ini_opts;
	ini_opts.clear();

	if(!file.is_open())
	{
		const char* unix_chr = getenv("HOME");
		const char* win_chr = getenv("LOCALAPPDATA");

		std::string unix_str = "";
		std::string win_str = "";
		std::string last_chr = "";

		if(unix_chr != NULL) { unix_str = unix_chr; }
		if(win_chr != NULL) { win_str = win_chr; }
		
		if((win_chr == NULL) && (unix_chr == NULL))
		{
			std::cout<<"GBE::Error - Could not open gbe.ini configuration file. Check file path or permissions. \n";
			return false;
		}

		bool config_result = false;

		//Test for Linux or Unix install location next
		if(win_chr == NULL)
		{
			last_chr = unix_str[unix_str.length() - 1];
			config::cfg_path = (last_chr == "/") ? unix_str + ".gbe_plus/" : unix_str + "/.gbe_plus/";
			config::data_path = config::cfg_path + "data/";
			unix_str += (last_chr == "/") ? ".gbe_plus/gbe.ini" : "/.gbe_plus/gbe.ini";

			file.open(unix_str.c_str(), std::ios::in);

			if(!file.is_open())
			{
				std::cout<<"GBE::Error - Could not open gbe.ini configuration file. Check file path or permissions. \n";
				return false;
			}
		}

		//Test for Windows install location next
		else
		{
			last_chr = win_str[win_str.length() - 1];
			config::cfg_path = (last_chr == "\\") ? win_str + "gbe_plus/" : win_str + "/gbe_plus/";
			config::data_path = config::cfg_path + "data/";
			win_str += (last_chr == "\\") ? "gbe_plus/gbe.ini" : "/gbe_plus/gbe.ini";

			file.open(win_str.c_str(), std::ios::in);

			if(!file.is_open())
			{
				std::cout<<"GBE::Error - Could not open gbe.ini configuration file. Check file path or permissions. \n";
				return false;
			}
		}
	}

	int touch_zone_counter = 0;

	//Cycle through whole file, line-by-line
	while(getline(file, input_line))
	{
		line_char = input_line[0];
		bool ignore = false;
	
		//Check if line starts with [ - if not, skip line
		if(line_char == "[")
		{
			std::string line_item = "";

			//Cycle through line, character-by-character
			for(int x = 0; ++x < input_line.length();)
			{
				line_char = input_line[x];

				//Check for single-quotes, don't parse ":" or "]" within them
				if((line_char == "'") && (!ignore)) { ignore = true; }
				else if((line_char == "'") && (ignore)) { ignore = false; }

				//Check the character for item limiter : or ] - Push to Vector
				else if(((line_char == ":") || (line_char == "]")) && (!ignore)) 
				{
					//Find and replace sequence for single quotes
					bool parse_quotes = true;

					while(parse_quotes)
					{
						size_t seq = line_item.find("^^^^");
						if(seq == std::string::npos) { parse_quotes = false; }
						else { line_item.replace(seq, std::string("^^^^").length(), "'"); }
					}

					ini_opts.push_back(line_item);
					line_item = ""; 
				}

				else { line_item += line_char; }
			}
		}
	}
	
	file.close();

	//Cycle through all items in the .ini file
	//Set options as appropiate
	int size = ini_opts.size();
	u32 output = 0;
	std::string ini_item = "";

	for(int x = 0; x < size; x++)
	{
		ini_item = ini_opts[x];

		//Use BIOS
		if(ini_item == "#use_bios")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if(output == 1) { config::use_bios = true; }
				else { config::use_bios = false; }
			}

			else 
			{ 
				std::cout<<"GBE::Error - Could not parse gbe.ini (#use_bios) \n";
				return false;
			}
		}

		//Use firmware
		if(ini_item == "#use_firmware")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if(output == 1) { config::use_firmware = true; }
				else { config::use_firmware = false; }
			}

			else 
			{ 
				std::cout<<"GBE::Error - Could not parse gbe.ini (#use_firmware) \n";
				return false;
			}
		}

		//Emulated SIO device
		if(ini_item == "#sio_device")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if((output >= 0) && (output <= 20)) { config::sio_device = output; }
			}

			else 
			{ 
				std::cout<<"GBE::Error - Could not parse gbe.ini (#sio_device) \n";
				return false;
			}
		}

		//Emulated IR device
		if(ini_item == "#ir_device")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if((output >= 0) && (output <= 7)) { config::ir_device = output; }
			}

			else 
			{ 
				std::cout<<"GBE::Error - Could not parse gbe.ini (#ir_device) \n";
				return false;
			}
		}

		//Emulated Slot1 device
		if(ini_item == "#slot1_device")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if((output >= 0) && (output <= 1)) { config::nds_slot1_device = output; }
			}

			else 
			{ 
				std::cout<<"GBE::Error - Could not parse gbe.ini (#slot1_device) \n";
				return false;
			}
		}

		//Emulated Slot2 device
		if(ini_item == "#slot2_device")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if((output >= 0) && (output <= 7)) { config::nds_slot2_device = output; }
			}

			else 
			{ 
				std::cout<<"GBE::Error - Could not parse gbe.ini (#slot2_device) \n";
				return false;
			}
		}

		//Set emulated system type
		else if(ini_item == "#system_type")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if((output >= 0) && (output <= 7)) 
				{
					config::gb_type = output;
					validate_system_type();
				}
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#system_type) \n";
				return false;
			}
		}

		//Use cheats
		if(ini_item == "#use_cheats")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if(output == 1) { config::use_cheats = true; }
				else { config::use_cheats = false; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#use_cheats) \n";
				return false;
			}
		}

		//Use patches
		if(ini_item == "#use_patches")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if(output == 1) { config::use_patches = true; }
				else { config::use_patches = false; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#use_patches) \n";
				return false;
			}
		}

		//Use OSD
		if(ini_item == "#use_osd")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if(output == 1) { config::use_osd = true; }
				else { config::use_osd = false; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#use_osd) \n";
				return false;
			}
		}

		//DMG BIOS path
		else if(ini_item == "#dmg_bios_path")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { config::dmg_bios_path = ini_item; }
				else { config::dmg_bios_path = ""; x--;}
 
			}

			else { config::dmg_bios_path = ""; }
		}

		//GBC BIOS path
		else if(ini_item == "#gbc_bios_path")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { config::gbc_bios_path = ini_item; }
				else { config::gbc_bios_path = ""; x--;}
 
			}

			else { config::gbc_bios_path = ""; }
		}

		//GBA BIOS path
		else if(ini_item == "#agb_bios_path")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { config::agb_bios_path = ini_item; }
				else { config::agb_bios_path = ""; x--;}
 
			}

			else { config::agb_bios_path = ""; }
		}

		//NDS9 BIOS path
		else if(ini_item == "#nds9_bios_path")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { config::nds9_bios_path = ini_item; }
				else { config::nds9_bios_path = ""; x--;}
 
			}

			else { config::nds9_bios_path = ""; }
		}

		//NDS7 BIOS path
		else if(ini_item == "#nds7_bios_path")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { config::nds7_bios_path = ini_item; }
				else { config::nds7_bios_path = ""; x--;}
 
			}

			else { config::nds7_bios_path = ""; }
		}

		//NDS firmware path
		else if(ini_item == "#nds_firmware_path")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { config::nds_firmware_path = ini_item; }
				else { config::nds_firmware_path = ""; x--;}
 
			}

			else { config::nds_firmware_path = ""; }
		}

		//MIN BIOS path
		else if(ini_item == "#min_bios_path")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { config::min_bios_path = ini_item; }
				else { config::min_bios_path = ""; x--;}
 
			}

			else { config::min_bios_path = ""; }
		}

		//Game save path
		else if(ini_item == "#save_path")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { config::save_path = ini_item; }
				else { config::save_path = ""; x--;}
 
			}

			else { config::save_path = ""; }
		}

		//Screenshots path
		else if(ini_item == "#screenshot_path")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { config::ss_path = ini_item; }
				else { config::ss_path = ""; x--;}
 
			}

			else { config::ss_path = ""; }
		}

		//Cheats path
		else if(ini_item == "#cheats_path")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { config::cheats_path = ini_item; }
				else { config::cheats_path = ""; x--;}
 
			}

			else { config::cheats_path = ""; }
		}

		//External camera file
		else if(ini_item == "#camera_file")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { config::external_camera_file = ini_item; }
				else { config::external_camera_file = ""; x--;}
 
			}

			else { config::external_camera_file = ""; }
		}

		//External card file
		else if(ini_item == "#card_file")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { config::external_card_file = ini_item; }
				else { config::external_card_file = ""; x--;}
 
			}

			else { config::external_card_file = ""; }
		}

		//External image file
		else if(ini_item == "#image_file")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { config::external_image_file = ini_item; }
				else { config::external_image_file = ""; x--;}
 
			}

			else { config::external_image_file = ""; }
		}

		//External data file
		else if(ini_item == "#data_file")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { config::external_data_file = ini_item; }
				else { config::external_data_file = ""; x--;}
 
			}

			else { config::external_data_file = ""; }
		}

		//Use OpenGL
		else if(ini_item == "#use_opengl")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if(output == 1) { config::use_opengl = true; }
				else { config::use_opengl = false; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#use_opengl) \n";
				return false;
			}
		}

		//Fragment shader
		else if(ini_item == "#fragment_shader")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { config::fragment_shader = config::data_path + "shaders/" + ini_item; }
				else { config::fragment_shader = config::data_path + "shaders/fragment.fs"; x--;}
 
			}

			else { config::fragment_shader = config::data_path + "shaders/fragment.fs"; }
		}

		//Vertex shader
		else if(ini_item == "#vertex_shader")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { config::vertex_shader = config::data_path + "shaders/" + ini_item; }
				else { config::vertex_shader = config::data_path + "shaders/vertex.vs"; x--;}
 
			}

			else { config::vertex_shader = config::data_path + "shaders/vertex.vs"; }
		}

		//Max FPS
		else if(ini_item == "#max_fps")
		{
			if((x + 1) < size)
			{
				util::from_str(ini_opts[++x], output);

				if(output <= 65535) { config::max_fps = output; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#max_fps) \n";
				return false;
			}
		}

		//Use gamepad dead zone
		else if(ini_item == "#dead_zone")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if((output >= 0) && (output <= 32767)) { config::dead_zone = output; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#dead_zone) \n";
				return false;
			}
		}

		//Use haptics
		else if(ini_item == "#use_haptics")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if(output == 1) { config::use_haptics = true; }
				else { config::use_haptics = false; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#use_haptics) \n";
				return false;
			}
		}

		//Volume settings
		else if(ini_item == "#volume")
		{
			if((x + 1) < size)
			{
				util::from_str(ini_opts[++x], output);

				if((output >= 0) && (output <= 128)) { config::volume = output; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#volume) \n";
				return false;
			}
		}

		//Mute settings
		else if(ini_item == "#mute")
		{
			if((x + 1) < size)
			{
				util::from_str(ini_opts[++x], output);

				if((output >= 0) && (output <= 1)) { config::mute = output; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#mute) \n";
				return false;
			}
		}

		//Stereo settings
		else if(ini_item == "#use_stereo")
		{
			if((x + 1) < size)
			{
				util::from_str(ini_opts[++x], output);

				if((output >= 0) && (output <= 1)) { config::use_stereo = output; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#use_stereo) \n";
				return false;
			}
		}

		//Sample rate
		else if(ini_item == "#sample_rate")
		{
			if((x + 1) < size)
			{
				util::from_str(ini_opts[++x], output);

				if((output >= 1) && (output <= 48000)) { config::sample_rate = (double)output; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#sample_rate) \n";
				return false;
			}
		}

		//Sample size
		else if(ini_item == "#sample_size")
		{
			if((x + 1) < size)
			{
				util::from_str(ini_opts[++x], output);

				if(output <= 4096) { config::sample_size = output; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#sample_size) \n";
				return false;
			}
		}

		//Scaling factor
		else if(ini_item == "#scaling_factor")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if((output >= 1) && (output <= 10)) { config::scaling_factor = config::old_scaling_factor = output; }
				else { config::scaling_factor = 1; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#scaling_factor) \n";
				return false;
			}
		}

		//Maintain aspect ratio
		else if(ini_item == "#maintain_aspect_ratio")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if(output == 1) { config::maintain_aspect_ratio = true; }
				else { config::maintain_aspect_ratio = false; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#maintain_aspect_ratio) \n";
				return false;
			}
		}

		//Real-time clock offsets
		else if(ini_item == "#rtc_offset")
		{
			if((x + 6) < size)
			{
				//Seconds offset
				util::from_str(ini_opts[++x], (u32&)config::rtc_offset[0]);
				if(config::rtc_offset[0] > 59) { config::rtc_offset[0] = 59; }

				//Minutes offset
				util::from_str(ini_opts[++x], (u32&)config::rtc_offset[1]);
				if(config::rtc_offset[1] > 59) { config::rtc_offset[1] = 59; }

				//Hours offset
				util::from_str(ini_opts[++x], (u32&)config::rtc_offset[2]);
				if(config::rtc_offset[2] > 23) { config::rtc_offset[2] = 23; }

				//Days offset
				util::from_str(ini_opts[++x], (u32&)config::rtc_offset[3]);
				if(config::rtc_offset[3] > 365) { config::rtc_offset[3] = 365; }

				//Months offset
				util::from_str(ini_opts[++x], (u32&)config::rtc_offset[4]);
				if(config::rtc_offset[4] > 11) { config::rtc_offset[4] = 11; }

				//Years offset
				util::from_str(ini_opts[++x], (u32&)config::rtc_offset[5]);
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#rtc_offset) \n";
				return false;
			}
		}

		//CPU overclocking flags
		else if(ini_item == "#oc_flags")
		{
			if((x + 1) < size)
			{
				util::from_str(ini_opts[++x], output);
				
				if((output >= 0) && (output <= 3)) { config::oc_flags = output; }
			}

			else
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#oc_flags) \n";
				return false;
			}
		}
			
		//Emulated DMG-on-GBC palette
		else if(ini_item == "#dmg_on_gbc_pal")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if((output >= 1) && (output <= 15)) 
				{
					config::dmg_gbc_pal = output;
					set_dmg_colors(config::dmg_gbc_pal);
				}
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#dmg_on_gbc_pal) \n";
				return false;
			}
		}

		//Keyboard controls
		else if(ini_item == "#gbe_key_controls")
		{
			if((x + 12) < size)
			{
				//A
				util::from_str(ini_opts[++x], config::gbe_key_a);

				//B
				util::from_str(ini_opts[++x], config::gbe_key_b);

				//X
				util::from_str(ini_opts[++x], config::gbe_key_x);

				//Y
				util::from_str(ini_opts[++x], config::gbe_key_y);

				//START
				util::from_str(ini_opts[++x], config::gbe_key_start);

				//SELECT
				util::from_str(ini_opts[++x], config::gbe_key_select);

				//LEFT
				util::from_str(ini_opts[++x], config::gbe_key_left);

				//RIGHT
				util::from_str(ini_opts[++x], config::gbe_key_right);

				//UP
				util::from_str(ini_opts[++x], config::gbe_key_up);

				//DOWN
				util::from_str(ini_opts[++x], config::gbe_key_down);

				//LEFT TRIGGER
				util::from_str(ini_opts[++x], config::gbe_key_l_trigger);

				//RIGHT TRIGGER
				util::from_str(ini_opts[++x], config::gbe_key_r_trigger);
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#gbe_key_controls) \n";
				return false;
			}
		}

		//Gamepad controls
		else if(ini_item == "#gbe_joy_controls")
		{
			if((x + 12) < size)
			{
				//A
				util::from_str(ini_opts[++x], config::gbe_joy_a);

				//B
				util::from_str(ini_opts[++x], config::gbe_joy_b);

				//X
				util::from_str(ini_opts[++x], config::gbe_joy_x);

				//Y
				util::from_str(ini_opts[++x], config::gbe_joy_y);

				//START
				util::from_str(ini_opts[++x], config::gbe_joy_start);

				//SELECT
				util::from_str(ini_opts[++x], config::gbe_joy_select);

				//LEFT
				util::from_str(ini_opts[++x], config::gbe_joy_left);

				//RIGHT
				util::from_str(ini_opts[++x], config::gbe_joy_right);

				//UP
				util::from_str(ini_opts[++x], config::gbe_joy_up);

				//DOWN
				util::from_str(ini_opts[++x], config::gbe_joy_down);

				//LEFT TRIGGER
				util::from_str(ini_opts[++x], config::gbe_joy_l_trigger);

				//RIGHT TRIGGER
				util::from_str(ini_opts[++x], config::gbe_joy_r_trigger);
			}


			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#gbe_joy_controls) \n";
				return false;
			}
		}

		//Context keyboard controls
		else if(ini_item == "#con_key_controls")
		{
			if((x + 6) < size)
			{
				//LEFT
				util::from_str(ini_opts[++x], config::con_key_left);

				//RIGHT
				util::from_str(ini_opts[++x], config::con_key_right);

				//UP
				util::from_str(ini_opts[++x], config::con_key_up);

				//DOWN
				util::from_str(ini_opts[++x], config::con_key_down);

				//CON1
				util::from_str(ini_opts[++x], config::con_key_1);

				//CON2
				util::from_str(ini_opts[++x], config::con_key_2);
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#con_key_controls) \n";
				return false;
			}
		}

		//Context joystick controls
		else if(ini_item == "#con_joy_controls")
		{
			if((x + 6) < size)
			{
				//LEFT
				util::from_str(ini_opts[++x], config::con_joy_left);

				//RIGHT
				util::from_str(ini_opts[++x], config::con_joy_right);

				//UP
				util::from_str(ini_opts[++x], config::con_joy_up);

				//DOWN
				util::from_str(ini_opts[++x], config::con_joy_down);

				//CON1
				util::from_str(ini_opts[++x], config::con_joy_1);

				//CON2
				util::from_str(ini_opts[++x], config::con_joy_2);
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#con_joy_controls) \n";
				return false;
			}
		}

		//Battle Chip ID list
		else if(ini_item == "#chip_list")
		{
			if((x + 6) < size)
			{
				for(u32 y = 0; y < 6; y++)
				{
					u32 val = 0;
					util::from_str(ini_opts[++x], val);
					config::chip_list[y] = val;
				}
			}
		}

		//Magical Watch Data
		else if(ini_item == "#mw_data")
		{
			if((x + 6) < size)
			{
				for(u32 y = 0; y < 6; y++)
				{
					u32 val = 0;
					util::from_str(ini_opts[++x], val);
					config::mw_data[y] = val;
				}
			}
		}
			
		//Hotkeys
		else if(ini_item == "#hotkeys")
		{
			if((x + 5) < size)
			{
				//Turbo
				util::from_str(ini_opts[++x], config::hotkey_turbo);

				//Mute
				util::from_str(ini_opts[++x], config::hotkey_mute);

				//GB Camera
				util::from_str(ini_opts[++x], config::hotkey_camera);

				//NDS swap screens
				util::from_str(ini_opts[++x], config::hotkey_swap_screen);

				//NDS vertical and landscape mode
				util::from_str(ini_opts[++x], config::hotkey_shift_screen);
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#hotkeys) \n";
				return false;
			}
		}

		//NDS touch zone mappings
		else if(ini_item == "#nds_touch_zone")
		{
			if(touch_zone_counter < 10)
			{
				if((x + 3) < size)
				{

					std::stringstream temp_stream;

					//Pad mapping
					temp_stream << ini_opts[++x];
					temp_stream >> config::touch_zone_pad[touch_zone_counter];
					temp_stream.clear();
					temp_stream.str(std::string());

					//X coordinate
					temp_stream << ini_opts[++x];
					temp_stream >> config::touch_zone_x[touch_zone_counter];
					temp_stream.clear();
					temp_stream.str(std::string());

					//Y coordinate
					temp_stream << ini_opts[++x];
					temp_stream >> config::touch_zone_y[touch_zone_counter];
					temp_stream.clear();
					temp_stream.str(std::string());

					touch_zone_counter++;
				}

				else 
				{
					std::cout<<"GBE::Error - Could not parse gbe.ini (#nds_touch_zone) \n";
					return false;
				}
			}
		}

		//NDS touch mode
		else if(ini_item == "#nds_touch_mode")
		{
			if((x + 1) < size)
			{
				util::from_str(ini_opts[++x], output);

				config::touch_mode = output;
			}

			else
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#touch_mode) \n";
				return false;
			}
		}

		//NDS virtual cursor enable
		else if(ini_item == "#virtual_cursor_enable")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if(output == 1) { config::vc_enable = true; }
				else { config::vc_enable = false; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#virtual_cursor_enable) \n";
				return false;
			}
		}

		//NDS virtual cursor file
		else if(ini_item == "#virtual_cursor_file")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { config::vc_file = ini_item; }
				else { config::vc_file = ""; x--;}
 
			}

			else { config::vc_file = ""; }
		}

		//NDS virtual cursor opacity
		else if(ini_item == "#virtual_cursor_opacity")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if((output >= 0) && (output <= 31)) { config::vc_opacity = output; }
				else { config::vc_opacity = output; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#virtual_cursor_opacity) \n";
				return false;
			}
		}

		//NDS virtual cursor timeout
		else if(ini_item == "#virtual_cursor_timeout")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);
				config::vc_timeout = output;

			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#virtual_cursor_timeout) \n";
				return false;
			}
		}

		//Use CGFX
		else if(ini_item == "#use_cgfx")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if(output == 1) { cgfx::load_cgfx = true; }
				else { cgfx::load_cgfx = false; }
			}

			else 
			{ 
				std::cout<<"GBE::Error - Could not parse gbe.ini (#use_cgfx) \n";
				return false;
			}
		}

		//CGFX manifest path
		else if(ini_item == "#manifest_path")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { cgfx::manifest_file = ini_item; }
				else { cgfx::manifest_file = ""; x--;}
 
			}

			else { cgfx::manifest_file = ""; }
		}

		//CGFX BG Tile dump folder
		else if(ini_item == "#dump_bg_path")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { cgfx::dump_bg_path = ini_item; }
				else { x--; }
			}
		}

		//CGFX OBJ Tile dump folder
		else if(ini_item == "#dump_obj_path")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#") { cgfx::dump_obj_path = ini_item; }
				else { x--; }
			}
		}

		//CGFX Scaling factor
		else if(ini_item == "#cgfx_scaling_factor")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if((output >= 1) && (output <= 10)) { cgfx::scaling_factor = output; }
				else { cgfx::scaling_factor = 1; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#cgfx_scaling_factor) \n";
				return false;
			}
		}

		//CGFX Transparency color
		else if(ini_item == "#cgfx_transparency")
		{
			if((x + 1) < size)
			{
				ini_item = ini_opts[++x];
				std::size_t found = ini_item.find("0x");
				std::string format = ini_item.substr(0, 2);

				//Value must be in hex format with "0x"
				if(format != "0x")
				{
					std::cout<<"GBE::Error - Could not parse gbe.ini (#cgfx_transparency) \n";
					return false;
				}

				std::string hex_color = ini_item.substr(found + 2);

				//Value must not be more than 8 characters long for AARRGGBB
				if(hex_color.size() > 8)
				{
					std::cout<<"GBE::Error - Could not parse gbe.ini (#cgfx_transparency) \n";
					return false;
				}

				u32 transparency = 0;

				//Parse the string into hex
				if(!util::from_hex_str(hex_color, transparency))
				{
					std::cout<<"GBE::Error - Could not parse gbe.ini (#cgfx_transparency) \n";
					return false;
				}
			}

			else
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#cgfx_transparency) \n";
				return false;
			}
		}

		//Use netplay
		else if(ini_item == "#use_netplay")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if(output == 1) { config::use_netplay = true; }
				else { config::use_netplay = false; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#use_netplay) \n";
				return false;
			}
		}

		//Use netplay hard sync
		if(ini_item == "#use_netplay_hard_sync")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if(output == 1) { config::netplay_hard_sync = true; }
				else { config::netplay_hard_sync = false; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#use_netplay_hard_sync) \n";
				return false;
			}
		}

		//Use Net Gate
		else if(ini_item == "#use_net_gate")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if(output == 1) { config::use_net_gate = true; }
				else { config::use_net_gate = false; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#use_net_gate) \n";
				return false;
			}
		}

		//Use real server for Mobile Adapter GB
		else if(ini_item == "#use_real_gbma_server")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if(output == 1) { config::use_real_gbma_server = true; }
				else { config::use_real_gbma_server = false; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#use_real_gbma_server) \n";
				return false;
			}
		}

		//Real server Mobile Adapter GB HTTP port
		else if(ini_item == "#gbma_server_http_port")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if(output <= 65535) { config::gbma_server_http_port = output; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#netplay_server_port) \n";
				return false;
			}
		}

		//Netplay sync threshold
		if(ini_item == "#netplay_sync_threshold")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				config::netplay_sync_threshold = output;
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#netplay_sync_threshold) \n";
				return false;
			}
		}


		//Netplay server port
		else if(ini_item == "#netplay_server_port")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if(output <= 65535) { config::netplay_server_port = output; }
				else { config::netplay_server_port = 2000; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#netplay_server_port) \n";
				return false;
			}
		}

		//Netplay client port
		else if(ini_item == "#netplay_client_port")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);

				if(output <= 65535) { config::netplay_client_port = output; }
				else { config::netplay_client_port = 2001; }
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#netplay_client_port) \n";
				return false;
			}
		}

		//Netplay client IP address
		else if(ini_item == "#netplay_client_ip")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				config::netplay_client_ip = ini_item;
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#netplay_client_ip) \n";
				return false;
			}
		}


		//Real Mobile Adapter GB IP address
		else if(ini_item == "#gbma_server_ip")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				config::gbma_server = ini_item;
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#gbma_server) \n";
				return false;
			}
		}

		//Netplay Player ID
		else if(ini_item == "#netplay_id")
		{
			if((x + 1) < size) 
			{
				util::from_str(ini_opts[++x], output);
				config::netplay_id = output;
			}

			else 
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#netplay_id) \n";
				return false;
			}
		}

		//IR database index
		else if(ini_item == "#ir_db_index")
		{
			if((x + 1) < size)
			{
				util::from_str(ini_opts[++x], output);
				
				if(output >= 0) { config::ir_db_index = output; }
			}

			else
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#ir_db_index) \n";
				return false;
			}
		}

		//Multi Plust On System ID
		else if(ini_item == "#mpos_id")
		{
			if((x + 1) < size)
			{
				ini_item = ini_opts[++x];
				std::size_t found = ini_item.find("0x");
				std::string format = ini_item.substr(0, 2);

				//Value must be in hex format with "0x"
				if(format != "0x")
				{
					std::cout<<"GBE::Error - Could not parse gbe.ini (#mpos_id) \n";
					return false;
				}

				std::string id = ini_item.substr(found + 2);

				//Value must not be more than 4 characters long for 16-bit
				if(id.size() > 4)
				{
					std::cout<<"GBE::Error - Could not parse gbe.ini (#mpos_id) \n";
					return false;
				}

				u32 final_id = 0;

				//Parse the string into hex
				if(!util::from_hex_str(id, final_id))
				{
					std::cout<<"GBE::Error - Could not parse gbe.ini (#mpos_id) \n";
					return false;
				}

				config::mpos_id = final_id;
			}

			else
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#mpos_id) \n";
				return false;
			}
		}

		//Ubisoft Thrustmaster Pedometer steps
		else if(ini_item == "#utp_steps")
		{
			if((x + 1) < size)
			{
				ini_item = ini_opts[++x];

				//Make sure only 5 characters max are used
				if(ini_item.size() > 5) { ini_item = ini_item.substr(0, 5); }

				u32 steps = 0;

				//Parse the string into hex
				if(!util::from_hex_str(ini_item, steps))
				{
					std::cout<<"GBE::Error - Could not parse gbe.ini (#utp_steps) \n";
					return false;
				}

				config::utp_steps = steps;
			}

			else
			{
				std::cout<<"GBE::Error - Could not parse gbe.ini (#utp_steps) \n";
				return false;
			}
		}

		//Recent files
		else if(ini_item == "#recent_files")
		{
			if((x + 1) < size) 
			{
				ini_item = ini_opts[++x];
				std::string first_char = "";
				first_char = ini_item[0];
				
				//When left blank, don't parse the next line item
				if(first_char != "#")
				{
					//Only take at most the 1st 10 entries
					if(config::recent_files.size() < 10) { config::recent_files.push_back(ini_item); }
				}

				else { x--; }
			}
		}
	}

	return true;
}

/****** Save options to the .ini file ******/
bool save_ini_file()
{
	//Test for Windows or Portable version first
	//Always give preference to portable .ini settings on every OS
	std::string ini_path = config::cfg_path + "gbe.ini";
	std::ifstream in_file(ini_path.c_str(), std::ios::in);

	std::string input_line = "";
	std::string line_char = "";

	std::vector <std::string> output_lines;
	std::vector <u32> output_count;
	int line_counter = 0;
	int recent_count = config::recent_files.size();

	//Clear existing .ini parameters
	std::vector <std::string> ini_opts;
	ini_opts.clear();

	if(!in_file.is_open())
	{
		std::cout<<"GBE::Error - Could not open gbe.ini configuration file. Check file path or permissions. \n";
		return false; 
	}

	//Cycle through whole file, line-by-line
	while(getline(in_file, input_line))
	{
		line_char = input_line[0];
		bool ignore = false;	

		//Push line to output text for later manipulation
		output_lines.push_back(input_line);
	
		//Check if line starts with [ - if not, skip line
		if(line_char == "[")
		{
			std::string line_item = "";

			//Cycle through line, character-by-character
			for(int x = 0; ++x < input_line.length();)
			{
				line_char = input_line[x];

				//Check for single-quotes, don't parse ":" or "]" within them
				if((line_char == "'") && (!ignore)) { ignore = true; }
				else if((line_char == "'") && (ignore)) { ignore = false; }

				//Check the character for item limiter : or ] - Push to Vector
				else if(((line_char == ":") || (line_char == "]")) && (!ignore)) 
				{
					ini_opts.push_back(line_item);
					output_count.push_back(line_counter);
					line_item = "";
				}

				else { line_item += line_char; }
			}
		}

		line_counter++;
	}
	
	in_file.close();

	//Cycle through all items in the .ini file
	//Save options as appropiate
	int size = ini_opts.size();
	int line_pos = 0;
	std::string ini_item = "";

	for(int x = 0; x < size; x++)
	{
		ini_item = ini_opts[x];

		//Use BIOS
		if(ini_item == "#use_bios")
		{
			line_pos = output_count[x];
			std::string val = (config::use_bios) ? "1" : "0";

			output_lines[line_pos] = "[#use_bios:" + val + "]";
		}

		//Use firmware
		if(ini_item == "#use_firmware")
		{
			line_pos = output_count[x];
			std::string val = (config::use_firmware) ? "1" : "0";

			output_lines[line_pos] = "[#use_firmware:" + val + "]";
		}

		//Emulated SIO device
		if(ini_item == "#sio_device")
		{
			line_pos = output_count[x];

			output_lines[line_pos] = "[#sio_device:" + util::to_str(config::sio_device) + "]";
		}

		//Emulated IR device
		if(ini_item == "#ir_device")
		{
			line_pos = output_count[x];

			output_lines[line_pos] = "[#ir_device:" + util::to_str(config::ir_device) + "]";
		}

		//Emulated Slot1 device
		if(ini_item == "#slot1_device")
		{
			line_pos = output_count[x];

			output_lines[line_pos] = "[#slot1_device:" + util::to_str(config::nds_slot1_device) + "]";
		}

		//Emulated Slot2 device
		if(ini_item == "#slot2_device")
		{
			line_pos = output_count[x];

			output_lines[line_pos] = "[#slot2_device:" + util::to_str(config::nds_slot2_device) + "]";
		}

		//Ubisoft Thrustmaster Steps
		if(ini_item == "#utp_steps")
		{
			line_pos = output_count[x];

			u32 temp_val = config::utp_steps;
			std::string val = util::to_hex_str(temp_val);
			val = val.substr(3);

			output_lines[line_pos] = "[#utp_steps:" + val + "]";
		}

		//Set emulated system type
		else if(ini_item == "#system_type")
		{
			line_pos = output_count[x];

			output_lines[line_pos] = "[#system_type:" + util::to_str(config::gb_type) + "]";
		}

		//Use cheats
		else if(ini_item == "#use_cheats")
		{
			line_pos = output_count[x];
			std::string val = (config::use_cheats) ? "1" : "0";

			output_lines[line_pos] = "[#use_cheats:" + val + "]";
		}

		//Use patches
		else if(ini_item == "#use_patches")
		{
			line_pos = output_count[x];
			std::string val = (config::use_patches) ? "1" : "0";

			output_lines[line_pos] = "[#use_patches:" + val + "]";
		}

		//Use OSD
		else if(ini_item == "#use_osd")
		{
			line_pos = output_count[x];
			std::string val = (config::use_osd) ? "1" : "0";

			output_lines[line_pos] = "[#use_osd:" + val + "]";
		}

		//DMG BIOS path
		else if(ini_item == "#dmg_bios_path")
		{
			line_pos = output_count[x];
			std::string val = (config::dmg_bios_path == "") ? "" : (":'" + config::dmg_bios_path + "'");

			output_lines[line_pos] = "[#dmg_bios_path" + val + "]";
		}

		//GBC BIOS path
		else if(ini_item == "#gbc_bios_path")
		{
			line_pos = output_count[x];
			std::string val = (config::gbc_bios_path == "") ? "" : (":'" + config::gbc_bios_path + "'");

			output_lines[line_pos] = "[#gbc_bios_path" + val + "]";
		}

		//GBA BIOS path
		else if(ini_item == "#agb_bios_path")
		{
			line_pos = output_count[x];
			std::string val = (config::agb_bios_path == "") ? "" : (":'" + config::agb_bios_path + "'");

			output_lines[line_pos] = "[#agb_bios_path" + val + "]";
		}

		//NDS9 BIOS path
		else if(ini_item == "#nds9_bios_path")
		{
			line_pos = output_count[x];
			std::string val = (config::nds9_bios_path == "") ? "" : (":'" + config::nds9_bios_path + "'");

			output_lines[line_pos] = "[#nds9_bios_path" + val + "]";
		}

		//NDS7 BIOS path
		else if(ini_item == "#nds7_bios_path")
		{
			line_pos = output_count[x];
			std::string val = (config::nds7_bios_path == "") ? "" : (":'" + config::nds7_bios_path + "'");

			output_lines[line_pos] = "[#nds7_bios_path" + val + "]";
		}

		//NDS9 firmware path
		else if(ini_item == "#nds_firmware_path")
		{
			line_pos = output_count[x];
			std::string val = (config::nds_firmware_path == "") ? "" : (":'" + config::nds_firmware_path + "'");

			output_lines[line_pos] = "[#nds_firmware_path" + val + "]";
		}

		//MIN BIOS path
		else if(ini_item == "#min_bios_path")
		{
			line_pos = output_count[x];
			std::string val = (config::min_bios_path == "") ? "" : (":'" + config::min_bios_path + "'");

			output_lines[line_pos] = "[#min_bios_path" + val + "]";
		}

		//Game save path
		else if(ini_item == "#save_path")
		{
			line_pos = output_count[x];
			std::string val = (config::save_path == "") ? "" : (":'" + config::save_path + "'");

			output_lines[line_pos] = "[#save_path" + val + "]";
		}

		//Screenshots path
		else if(ini_item == "#screenshot_path")
		{
			line_pos = output_count[x];
			std::string val = (config::ss_path == "") ? "" : (":'" + config::ss_path + "'");

			output_lines[line_pos] = "[#screenshot_path" + val + "]";
		}

		//Cheats path
		else if(ini_item == "#cheats_path")
		{
			line_pos = output_count[x];
			std::string val = (config::cheats_path == "") ? "" : (":'" + config::cheats_path + "'");

			output_lines[line_pos] = "[#cheats_path" + val + "]";
		}

		//External camera file
		else if(ini_item == "#camera_file")
		{
			line_pos = output_count[x];
			std::string val = (config::external_camera_file == "") ? "" : (":'" + config::external_camera_file + "'");

			output_lines[line_pos] = "[#camera_file" + val + "]";
		}

		//External card file
		else if(ini_item == "#card_file")
		{
			line_pos = output_count[x];
			std::string val = (config::external_card_file == "") ? "" : (":'" + config::external_card_file + "'");

			output_lines[line_pos] = "[#card_file" + val + "]";
		}

		//External image file
		else if(ini_item == "#image_file")
		{
			line_pos = output_count[x];
			std::string val = (config::external_image_file == "") ? "" : (":'" + config::external_image_file + "'");

			output_lines[line_pos] = "[#image_file" + val + "]";
		}

		//External image file
		else if(ini_item == "#data_file")
		{
			line_pos = output_count[x];
			std::string val = (config::external_data_file == "") ? "" : (":'" + config::external_data_file + "'");

			output_lines[line_pos] = "[#data_file" + val + "]";
		}

		//Use OpenGL
		else if(ini_item == "#use_opengl")
		{
			line_pos = output_count[x];
			std::string val = (config::use_opengl) ? "1" : "0";

			output_lines[line_pos] = "[#use_opengl:" + val + "]";
		}

		//Use gamepad dead zone
		else if(ini_item == "#dead_zone")
		{
			line_pos = output_count[x];

			output_lines[line_pos] = "[#dead_zone:" + util::to_str(config::dead_zone) + "]";
		}

		//Use haptics
		else if(ini_item == "#use_haptics")
		{
			line_pos = output_count[x];
			std::string val = (config::use_haptics) ? "1" : "0";

			output_lines[line_pos] = "[#use_haptics:" + val + "]";
		}

		//Volume settings
		else if(ini_item == "#volume")
		{
			line_pos = output_count[x];

			output_lines[line_pos] = "[#volume:" + util::to_str(config::volume) + "]";
		}

		//Mute settings
		else if(ini_item == "#mute")
		{
			line_pos = output_count[x];
			std::string val = (config::mute) ? "1" : "0";

			output_lines[line_pos] = "[#mute:" + val + "]";
		}

		//Stereo settings
		else if(ini_item == "#use_stereo")
		{
			line_pos = output_count[x];
			std::string val = (config::use_stereo) ? "1" : "0";

			output_lines[line_pos] = "[#use_stereo:" + val + "]";
		}

		//Sample rate
		else if(ini_item == "#sample_rate")
		{
			line_pos = output_count[x];

			output_lines[line_pos] = "[#sample_rate:" + util::to_str(config::sample_rate) + "]";
		}

		//Sample size
		else if(ini_item == "#sample_size")
		{
			line_pos = output_count[x];

			output_lines[line_pos] = "[#sample_size:" + util::to_str(config::sample_size) + "]";
		}

		//Scaling factor
		else if(ini_item == "#scaling_factor")
		{
			line_pos = output_count[x];

			output_lines[line_pos] = "[#scaling_factor:" + util::to_str(config::scaling_factor) + "]";
		}

		//Maintain aspect ratio
		else if(ini_item == "#maintain_aspect_ratio")
		{
			line_pos = output_count[x];
			std::string val = (config::maintain_aspect_ratio) ? "1" : "0";

			output_lines[line_pos] = "[#maintain_aspect_ratio:" + val + "]";
		}

		//Real-time clock offsets
		else if(ini_item == "#rtc_offset")
		{
			line_pos = output_count[x];
			std::string val = util::to_str(config::rtc_offset[0]) + ":";
			val += util::to_str(config::rtc_offset[1]) + ":";
			val += util::to_str(config::rtc_offset[2]) + ":";
			val += util::to_str(config::rtc_offset[3]) + ":";
			val += util::to_str(config::rtc_offset[4]) + ":";
			val += util::to_str(config::rtc_offset[5]);

			output_lines[line_pos] = "[#rtc_offset:" + val + "]";
		}

		//CPU overclocking flags
		else if(ini_item == "#oc_flags")
		{
			line_pos = output_count[x];
			std::string val = util::to_str(config::oc_flags);

			output_lines[line_pos] = "[#oc_flags:" + val + "]";
		}

		//Emulated DMG-on-GBC palette
		else if(ini_item == "#dmg_on_gbc_pal")
		{
			line_pos = output_count[x];

			output_lines[line_pos] = "[#dmg_on_gbc_pal:" + util::to_str(config::dmg_gbc_pal) + "]";
		}

		//OpenGL Fragment Shader
		else if(ini_item == "#fragment_shader")
		{
			line_pos = output_count[x];

			if(config::fragment_shader == (config::data_path + "shaders/fragment.fs")) { config::fragment_shader = "fragment.fs"; }
			else if(config::fragment_shader == (config::data_path + "shaders/2xBR.fs")) { config::fragment_shader = "2xBR.fs"; }
			else if(config::fragment_shader == (config::data_path + "shaders/4xBR.fs")) { config::fragment_shader = "4xBR.fs"; }
			else if(config::fragment_shader == (config::data_path + "shaders/bad_bloom.fs")) { config::fragment_shader = "bad_bloom.fs"; }
			else if(config::fragment_shader == (config::data_path + "shaders/badder_bloom.fs")) { config::fragment_shader = "badder_bloom.fs"; }
			else if(config::fragment_shader == (config::data_path + "shaders/chrono.fs")) { config::fragment_shader = "chrono.fs"; }
			else if(config::fragment_shader == (config::data_path + "shaders/dmg_mode.fs")) { config::fragment_shader = "dmg_mode.fs"; }
			else if(config::fragment_shader == (config::data_path + "shaders/gba_gamma.fs")) { config::fragment_shader = "gba_gamma.fs"; }
			else if(config::fragment_shader == (config::data_path + "shaders/gbc_gamma.fs")) { config::fragment_shader = "gbc_gamma.fs"; }
			else if(config::fragment_shader == (config::data_path + "shaders/grayscale.fs")) { config::fragment_shader = "grayscale.fs"; }
			else if(config::fragment_shader == (config::data_path + "shaders/lcd_mode.fs")) { config::fragment_shader = "lcd_mode.fs"; }
			else if(config::fragment_shader == (config::data_path + "shaders/pastel.fs")) { config::fragment_shader = "pastel.fs"; }
			else if(config::fragment_shader == (config::data_path + "shaders/scale2x.fs")) { config::fragment_shader = "scale2x.fs"; }
			else if(config::fragment_shader == (config::data_path + "shaders/scale3x.fs")) { config::fragment_shader = "scale3x.fs"; }
			else if(config::fragment_shader == (config::data_path + "shaders/sepia.fs")) { config::fragment_shader = "sepia.fs"; }
			else if(config::fragment_shader == (config::data_path + "shaders/spotlight.fs")) { config::fragment_shader = "spotlight.fs"; }
			else if(config::fragment_shader == (config::data_path + "shaders/tv_mode.fs")) { config::fragment_shader = "tv_mode.fs"; }
			else if(config::fragment_shader == (config::data_path + "shaders/washout.fs")) { config::fragment_shader = "washout.fs"; }

			output_lines[line_pos] = "[#fragment_shader:'" + config::fragment_shader + "']";
		}

		//OpenGL Vertex Shader
		else if(ini_item == "#vertex_shader")
		{
			line_pos = output_count[x];

			if(config::vertex_shader == (config::data_path + "shaders/vertex.vs")) { config::vertex_shader = "vertex.vs"; }
			else if(config::vertex_shader == (config::data_path + "shaders/invert_x.vs")) { config::vertex_shader = "invert_x.vs"; }

			output_lines[line_pos] = "[#vertex_shader:'" + config::vertex_shader + "']";
		}

		//Max FPS
		else if(ini_item == "#max_fps")
		{
			line_pos = output_count[x];

			output_lines[line_pos] = "[#max_fps:" + util::to_str(config::max_fps) + "]";
		}

		//Keyboard controls
		else if(ini_item == "#gbe_key_controls")
		{
			line_pos = output_count[x];
			std::string val = util::to_str(config::gbe_key_a) + ":";
			val += util::to_str(config::gbe_key_b) + ":";
			val += util::to_str(config::gbe_key_x) + ":";
			val += util::to_str(config::gbe_key_y) + ":";
			val += util::to_str(config::gbe_key_start) + ":";
			val += util::to_str(config::gbe_key_select) + ":";
			val += util::to_str(config::gbe_key_left) + ":";
			val += util::to_str(config::gbe_key_right) + ":";
			val += util::to_str(config::gbe_key_up) + ":";
			val += util::to_str(config::gbe_key_down) + ":";
			val += util::to_str(config::gbe_key_l_trigger) + ":";
			val += util::to_str(config::gbe_key_r_trigger);

			output_lines[line_pos] = "[#gbe_key_controls:" + val + "]";
		}

		//Gamepad controls
		else if(ini_item == "#gbe_joy_controls")
		{
			line_pos = output_count[x];
			std::string val = util::to_str(config::gbe_joy_a) + ":";
			val += util::to_str(config::gbe_joy_b) + ":";
			val += util::to_str(config::gbe_joy_x) + ":";
			val += util::to_str(config::gbe_joy_y) + ":";
			val += util::to_str(config::gbe_joy_start) + ":";
			val += util::to_str(config::gbe_joy_select) + ":";
			val += util::to_str(config::gbe_joy_left) + ":";
			val += util::to_str(config::gbe_joy_right) + ":";
			val += util::to_str(config::gbe_joy_up) + ":";
			val += util::to_str(config::gbe_joy_down) + ":";
			val += util::to_str(config::gbe_joy_l_trigger) + ":";
			val += util::to_str(config::gbe_joy_r_trigger);

			output_lines[line_pos] = "[#gbe_joy_controls:" + val + "]";
		}

		//Context keyboard controls
		else if(ini_item == "#con_key_controls")
		{
			line_pos = output_count[x];
			std::string val = util::to_str(config::con_key_left) + ":";
			val += util::to_str(config::con_key_right) + ":";
			val += util::to_str(config::con_key_up) + ":";
			val += util::to_str(config::con_key_down) + ":";
			val += util::to_str(config::con_key_1) + ":";
			val += util::to_str(config::con_key_2);

			output_lines[line_pos] = "[#con_key_controls:" + val + "]";
		}

		//Context joystick controls
		else if(ini_item == "#con_joy_controls")
		{
			line_pos = output_count[x];
			std::string val = util::to_str(config::con_joy_left) + ":";
			val += util::to_str(config::con_joy_right) + ":";
			val += util::to_str(config::con_joy_up) + ":";
			val += util::to_str(config::con_joy_down) + ":";
			val += util::to_str(config::con_joy_1) + ":";
			val += util::to_str(config::con_joy_2);

			output_lines[line_pos] = "[#con_joy_controls:" + val + "]";
		}

		//Battle Chip List
		else if(ini_item == "#chip_list")
		{
			line_pos = output_count[x];
			std::string val = util::to_str(config::chip_list[0]) + ":";
			val += util::to_str(config::chip_list[1]) + ":";
			val += util::to_str(config::chip_list[2]) + ":";
			val += util::to_str(config::chip_list[3]) + ":";
			val += util::to_str(config::chip_list[4]) + ":";
			val += util::to_str(config::chip_list[5]);

			output_lines[line_pos] = "[#chip_list:" + val + "]";
		}

		//Magical Watch Data
		else if(ini_item == "#mw_data")
		{
			line_pos = output_count[x];
			std::string val = util::to_str(config::mw_data[0]) + ":";
			val += util::to_str(config::mw_data[1]) + ":";
			val += util::to_str(config::mw_data[2]) + ":";
			val += util::to_str(config::mw_data[3]) + ":";
			val += util::to_str(config::mw_data[4]) + ":";
			val += util::to_str(config::mw_data[5]);

			output_lines[line_pos] = "[#mw_data:" + val + "]";
		}

		//Hotkeys
		else if(ini_item == "#hotkeys")
		{
			line_pos = output_count[x];
			std::string val_1 = util::to_str(config::hotkey_turbo);
			std::string val_2 = util::to_str(config::hotkey_mute);
			std::string val_3 = util::to_str(config::hotkey_camera);
			std::string val_4 = util::to_str(config::hotkey_swap_screen);
			std::string val_5 = util::to_str(config::hotkey_shift_screen);

			output_lines[line_pos] = "[#hotkeys:" + val_1 + ":" + val_2 + ":" + val_3 + ":" + val_4 + ":" + val_5 + "]";
		}

		//Use CGFX
		else if(ini_item == "#use_cgfx")
		{
			line_pos = output_count[x];
			std::string val = (cgfx::load_cgfx) ? "1" : "0";

			output_lines[line_pos] = "[#use_cgfx:" + val + "]";
		}

		//CGFX manifest path
		else if(ini_item == "#manifest_path")
		{
			line_pos = output_count[x];
			std::string val = (cgfx::manifest_file == "") ? "" : (":'" + cgfx::manifest_file + "'");

			output_lines[line_pos] = "[#manifest_path" + val + "]";
		}

		//CGFX BG Tile dump folder
		else if(ini_item == "#dump_bg_path")
		{
			line_pos = output_count[x];
			std::string val = (cgfx::dump_bg_path == "") ? "" : (":'" + cgfx::dump_bg_path + "'");

			output_lines[line_pos] = "[#dump_bg_path" + val + "]";
		}

		//CGFX OBJ Tile dump folder
		else if(ini_item == "#dump_obj_path")
		{
			line_pos = output_count[x];
			std::string val = (cgfx::dump_obj_path == "") ? "" : (":'" + cgfx::dump_obj_path + "'");

			output_lines[line_pos] = "[#dump_obj_path" + val + "]";
		}

		//CGFX Scaling factor
		else if(ini_item == "#cgfx_scaling_factor")
		{
			line_pos = output_count[x];
			std::string val = util::to_str(cgfx::scaling_factor);

			output_lines[line_pos] = "[#cgfx_scaling_factor:" + val + "]";
		}

		//CGFX Transparency color
		else if(ini_item == "#cgfx_transparency")
		{
			line_pos = output_count[x];
			std::string val = util::to_hex_str(cgfx::transparency_color);

			output_lines[line_pos] = "[#cgfx_transparency:" + val + "]";
		}

		//Use netplay
		else if(ini_item == "#use_netplay")
		{
			line_pos = output_count[x];
			std::string val = (config::use_netplay) ? "1" : "0";

			output_lines[line_pos] = "[#use_netplay:" + val + "]";
		}

		//Use Net Gate
		else if(ini_item == "#use_net_gate")
		{
			line_pos = output_count[x];
			std::string val = (config::use_net_gate) ? "1" : "0";

			output_lines[line_pos] = "[#use_net_gate:" + val + "]";
		}

		//Use netplay hard sync
		else if(ini_item == "#use_netplay_hard_sync")
		{
			line_pos = output_count[x];
			std::string val = (config::netplay_hard_sync) ? "1" : "0";

			output_lines[line_pos] = "[#use_netplay_hard_sync:" + val + "]";
		}

		//Use netplay sync threshold
		else if(ini_item == "#netplay_sync_threshold")
		{
			line_pos = output_count[x];
			std::string val = util::to_str(config::netplay_sync_threshold);

			output_lines[line_pos] = "[#netplay_sync_threshold:" + val + "]";
		}

		//Netplay server port
		else if(ini_item == "#netplay_server_port")
		{
			line_pos = output_count[x];
			std::string val = util::to_str(config::netplay_server_port);

			output_lines[line_pos] = "[#netplay_server_port:" + val + "]";
		}

		//Netplay client port
		else if(ini_item == "#netplay_client_port")
		{
			line_pos = output_count[x];
			std::string val = util::to_str(config::netplay_client_port);

			output_lines[line_pos] = "[#netplay_client_port:" + val + "]";
		}

		//Netplay client IP address
		else if(ini_item == "#netplay_client_ip")
		{
			line_pos = output_count[x];
			output_lines[line_pos] = "[#netplay_client_ip:" + config::netplay_client_ip + "]";
		}

		//Netplay Player ID
		else if(ini_item == "#netplay_id")
		{
			line_pos = output_count[x];
			std::string val = util::to_str(config::netplay_id);

			output_lines[line_pos] = "[#netplay_id:" + val + "]";
		}

		//Use real GBMA server
		else if(ini_item == "#use_real_gbma_server")
		{
			line_pos = output_count[x];
			std::string val = (config::use_real_gbma_server) ? "1" : "0";

			output_lines[line_pos] = "[#use_real_gbma_server:" + val + "]";
		}

		//GBMA server IP or hostname
		else if(ini_item == "#gbma_server_ip")
		{
			line_pos = output_count[x];
			output_lines[line_pos] = "[#gbma_server_ip:" + config::gbma_server + "]";
		}

		//GBMA server HTTP port
		else if(ini_item == "#gbma_server_http_port")
		{
			line_pos = output_count[x];
			std::string val = util::to_str(config::gbma_server_http_port);

			output_lines[line_pos] = "[#gbma_server_http_port:" + val + "]";
		}

		//IR database index
		else if(ini_item == "#ir_db_index")
		{
			line_pos = output_count[x];
			std::string val = util::to_str(config::ir_db_index);

			output_lines[line_pos] = "[#ir_db_index:" + val + "]";
		}

		//NDS touch mode
		else if(ini_item == "#nds_touch_mode")
		{
			line_pos = output_count[x];
			std::string val = util::to_str(config::touch_mode);

			output_lines[line_pos] = "[#nds_touch_mode:" + val + "]";
		}

		//NDS virtual cursor enable
		else if(ini_item == "#virtual_cursor_enable")
		{
			line_pos = output_count[x];
			std::string val = (config::vc_enable) ? "1" : "0";

			output_lines[line_pos] = "[#virtual_cursor_enable:" + val + "]";
		}

		//NDS virtual cursor file
		else if(ini_item == "#virtual_cursor_file")
		{
			line_pos = output_count[x];
			std::string val = (config::vc_file == "") ? "" : (":'" + config::vc_file + "'");

			output_lines[line_pos] = "[#virtual_cursor_file" + val + "]";
		}

		//NDS virtual cursor opacity
		else if(ini_item == "#virtual_cursor_opacity")
		{
			line_pos = output_count[x];
			std::string val = util::to_str(config::vc_opacity);

			output_lines[line_pos] = "[#virtual_cursor_opacity:" + val + "]";
		}

		//NDS virtual cursor timeout
		else if(ini_item == "#virtual_cursor_timeout")
		{
			line_pos = output_count[x];
			std::string val = util::to_str(config::vc_timeout);

			output_lines[line_pos] = "[#virtual_cursor_timeout:" + val + "]";
		}

		else if(ini_item == "#recent_files")
		{
			line_pos = output_count[x];
			output_lines[line_pos] = "[#ignore#]";
		}
	}

	//Write contents to .ini file
	std::ofstream out_file(ini_path.c_str(), std::ios::out);

	if(!out_file.is_open())
	{
		std::cout<<"GBE::Error - Could not save gbe.ini configuration file. Check file path or permissions. \n";
		return false; 
	}

	for(int x = 0; x < output_lines.size(); x++)
	{
		if(output_lines[x] != "[#ignore#]")
		{
			output_lines[x] += "\n";
			out_file << output_lines[x];
		}
	}

	for(int x = 0; x < recent_count; x++)
	{

		//Find and replace sequence for single quotes
		bool parse_quotes = true;

		while(parse_quotes)
		{
			size_t seq = config::recent_files[x].find("'");
			if(seq == std::string::npos) { parse_quotes = false; }
			else { config::recent_files[x].replace(seq, std::string("'").length(), "^^^^"); }
		}

		std::string val = "'" + config::recent_files[x] + "'";
		val = "[#recent_files:" + val + "]";
		
		if(x == 0) { out_file << val; }
		else { out_file << "\n" << val; }
	}

	out_file.close();
	return true;
}

/****** Parse the cheats file ******/
bool parse_cheats_file(bool add_cheats)
{
	if(config::cheats_path.empty()) { return false; }

	std::ifstream file(config::cheats_path.c_str(), std::ios::in); 
	std::string input_line = "";
	std::string line_char = "";

	std::vector<std::string> cheat_entry;
	std::string code_type;
	std::string cheat_code;
	std::string info;

	//Clear cheat codes
	if(!add_cheats)
	{
		config::gs_cheats.clear();
		config::gg_cheats.clear();
		config::gsa_cheats.clear();
		config::cheats_info.clear();
	}

	if(!file.is_open())
	{
		std::cout<<"GBE::Could not open cheats file " << config::cheats_path << ". Check file path or permissions. \n";
		return false; 
	}

	//Cycle through whole file, line-by-line
	while(getline(file, input_line))
	{
		line_char = input_line[0];
		bool ignore = false;	
		u8 item_count = 0;	

		//Check if line starts with [ - if not, skip line
		if(line_char == "[")
		{
			std::string line_item = "";

			//Cycle through line, character-by-character
			for(int x = 0; ++x < input_line.length();)
			{
				line_char = input_line[x];

				//Check for single-quotes, don't parse ":" or "]" within them
				if((line_char == "'") && (!ignore)) { ignore = true; }
				else if((line_char == "'") && (ignore)) { ignore = false; }

				//Check the character for item limiter : or ] - Push to Vector
				else if(((line_char == ":") || (line_char == "]")) && (!ignore)) 
				{
					cheat_entry.push_back(line_item);
					line_item = "";
					item_count++;
				}

				else { line_item += line_char; }
			}
		}

		if((item_count != 3) && (item_count != 0))
		{
			std::cout<<"GBE::Cheat file has incorrect entry: " << input_line << "\n";
			file.close();
			return false;
		}
	}
	
	file.close();

	//Parse entries
	for(int x = 0; x < cheat_entry.size();)
	{
		code_type = cheat_entry[x++];
		cheat_code = cheat_entry[x++];
		info = cheat_entry[x++];

		//Add Gameshark codes 
		if(code_type == "GS")
		{
			//Verify length
			if(cheat_code.length() == 8)
			{
				//Convert code into u32
				u32 converted_cheat = 0;
				util::from_hex_str(cheat_code, converted_cheat);
				config::gs_cheats.push_back(converted_cheat);
				
				info += "*";
				config::cheats_info.push_back(info);
			}

			else
			{
				std::cout<<"GBE::Error - Could not parse Gameshark (DMG-GBC) cheat code " << cheat_code << "\n";

				config::gs_cheats.clear();
				config::gg_cheats.clear();
				config::gsa_cheats.clear();
				config::cheats_info.clear();

				return false;
			}
		}

		//Add Game Genie codes
		else if(code_type == "GG")
		{
			//Verify length
			if(cheat_code.length() == 9)
			{
				config::gg_cheats.push_back(cheat_code);

				info += "^";
				config::cheats_info.push_back(info);
			}

			else
			{
				std::cout<<"GBE::Error - Could not parse Game Genie cheat code " << cheat_code << "\n";

				config::gs_cheats.clear();
				config::gg_cheats.clear();
				config::gsa_cheats.clear();
				config::cheats_info.clear();

				return false;
			}
		}

		//Add Gameshark GBA codes
		else if(code_type == "GSA1")
		{
			//Verify length
			if(cheat_code.length() == 16)
			{
				config::gsa_cheats.push_back(cheat_code);

				info += "#";
				config::cheats_info.push_back(info);
			}

			else
			{
				std::cout<<"GBE::Error - Could not parse Gameshark (GBA) cheat code " << cheat_code << "\n";

				config::gs_cheats.clear();
				config::gg_cheats.clear();
				config::gsa_cheats.clear();
				config::cheats_info.clear();

				return false;
			}
		}	
	}

	if(add_cheats) { save_cheats_file(); }

	return true;
}

/****** Saves the cheat file ******/
bool save_cheats_file()
{
	if(config::cheats_path.empty()) { return false; }

	std::ofstream file(config::cheats_path.c_str(), std::ios::out);

	if(!file.is_open())
	{
		std::cout<<"GBE::Could not open cheats file " << config::cheats_path << ". Check file path or permissions. \n";
		return false; 
	}

	int gs_count = 0;
	int gg_count = 0;
	int gsa_count = 0;

	//Cycle through cheats
	for(u32 x = 0; x < config::cheats_info.size(); x++)
	{
		std::string info_str = config::cheats_info[x];
		std::string code_str = "";
		std::string data_str = "";

		//Determine code type based on info string
		std::string last_char = "";
		last_char += info_str[info_str.size() - 1];

		//GS code
		if(last_char == "*")
		{
			info_str.resize(info_str.size() - 1);
			code_str = "GS";
			data_str = util::to_hex_str(config::gs_cheats[gs_count++]).substr(2);

			//Make sure code data length is 8
			while(data_str.size() != 8)
			{
				data_str = "0" + data_str;
			}

			std::string output_line = "[" + code_str + ":" + data_str + ":" + info_str + "]\n";
			file << output_line;
		}

		//GG code
		else if(last_char == "^")
		{
			info_str.resize(info_str.size() - 1);
			code_str = "GG";
			data_str = config::gg_cheats[gg_count++];

			//Make sure code data length is 9
			while(data_str.size() != 9)
			{
				data_str = "0" + data_str;
			}

			std::string output_line = "[" + code_str + ":" + data_str + ":" + info_str + "]\n";
			file << output_line;
		}

		//GSA1 code
		else if(last_char == "#")
		{
			info_str.resize(info_str.size() - 1);
			code_str = "GSA1";
			data_str = config::gsa_cheats[gsa_count++];

			//Make sure code data length is 16
			while(data_str.size() != 16)
			{
				data_str = "0" + data_str;
			}

			std::string output_line = "[" + code_str + ":" + data_str + ":" + info_str + "]\n";
			file << output_line;
		}
	}

	file.close();
	return true;
}
