// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gamepad.h
// Date : November 13, 2016
// Description : NDS joypad emulation and input handling
//
// Reads and writes to the KEYINPUT, EXTKEYIN, and KEYCNT registers
// Handles input from keyboard using SDL events

#ifndef NDS_GAMEPAD
#define NDS_GAMEPAD

#include "SDL.h"
#include <string>
#include <iostream>

#include "common.h"
#include "common/config.h"

class NTR_GamePad
{
	public:

	NTR_GamePad();
	~NTR_GamePad();

	void handle_input(SDL_Event &event);
	void init();
	void clear_input();

	void process_keyboard(int pad, bool pressed);
	void process_joystick(int pad, bool pressed);
	void process_mouse(int pad, bool pressed);
	void process_virtual_cursor();
	void process_turbo_buttons();

	void start_rumble(s32 len);
	void stop_rumble();

	int pad;
	u8 sdl_fs_ratio;
	u16 key_input;
	u16 ext_key_input;
	u32 mouse_x, mouse_y;
	bool touch_hold;
	bool touch_by_mouse;
	bool is_rumbling;

	u32 vc_x;
	u32 vc_y;
	u32 vc_counter;
	u32 vc_pause;
	s32 vc_delta_x;
	s32 vc_delta_y;

	u32* nds7_input_irq;
	u32* nds9_input_irq;

	bool joypad_irq;
	bool joy_init;
	u16 key_cnt;
	u16 con_flags;

	u32 turbo_button_enabled;
	bool turbo_button_stat[12];
	bool turbo_button_end[12];
	u32 turbo_button_val[12];

	private:

	//Shadow status for keyboard input
	bool up_shadow, down_shadow, left_shadow, right_shadow;

	SDL_Joystick* jstick;
    	SDL_Haptic* rumble;
};

#endif // NDS_GAMEPAD