// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gamepad.h
// Date : October 12, 2014
// Description : Game Boy Advance joypad emulation and input handling
//
// Reads and writes to the KEYINPUT and KEYCNT registers
// Handles input from keyboard using SDL events

#ifndef GBA_GAMEPAD
#define GBA_GAMEPAD

#include "SDL2/SDL.h"
#include <string>
#include <iostream>

#include "common.h"
#include "common/config.h"

class AGB_GamePad
{
	public:

	AGB_GamePad();
	~AGB_GamePad();

	void handle_input(SDL_Event &event);
	void init();
	void clear_input();

	void process_keyboard(int pad, bool pressed);
	void process_joystick(int pad, bool pressed);
	void process_gyroscope();
	void process_gyroscope(float x, float y);
	void process_turbo_buttons();

	void start_rumble();
	void stop_rumble();

	int pad;
	u16 key_input;
	bool is_rumbling;
	bool is_gb_player;
	bool disable_input;

	u16 con_flags;

	u16 gyro_value;
	u8 gyro_flags;

	u8 solar_value;

	u16 sensor_x;
	u16 sensor_y;

	bool joypad_irq;
	bool joy_init;
	u16 key_cnt;

	u32 turbo_button_enabled;
	bool turbo_button_stat[12];
	bool turbo_button_end[12];
	u32 turbo_button_val[12];

	private:

	//Shadow status for keyboard input
	bool up_shadow, down_shadow, left_shadow, right_shadow;

	SDL_Joystick* jstick;
    	SDL_Haptic* rumble;

	SDL_GameController* gc_sensor;
	bool sensor_init;
};

#endif // GBA_GAMEPAD