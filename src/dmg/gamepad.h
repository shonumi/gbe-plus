// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gamepad.h
// Date : May 12, 2015
// Description : Game Boy joypad emulation and input handling
//
// Reads and writes to the P1 register
// Handles input from keyboard using SDL events

#ifndef GB_GAMEPAD
#define GB_GAMEPAD

#include "SDL2/SDL.h"
#include <string>
#include <iostream>

#include "common.h"
#include "common/config.h"
#include "common/dmg_core_pad.h"

class DMG_GamePad : virtual public dmg_core_pad
{
	public:

	DMG_GamePad();
	~DMG_GamePad();

	void handle_input(SDL_Event &event);
	void init();

	void process_keyboard(int pad, bool pressed);
	void process_joystick(int pad, bool pressed);
	void process_gyroscope();
	void process_gyroscope(float x, float y);
	void process_turbo_buttons();
	void start_rumble();
	void stop_rumble();
	u8 read();
	void write(u8 value);
	u32 get_pad_data(u32 index);
	void set_pad_data(u32 index, u32 value);

	u32 turbo_button_enabled;
	bool turbo_button_stat[12];
	bool turbo_button_end[12];
	u32 turbo_button_val[12];

	private:
	
	SDL_GameController* gc_sensor;
	bool sensor_init;

	u32 ddr_key_mapping[3];
	u32 ddr_joy_mapping[3];
	bool ddr_was_mapped;
};

#endif // GB_GAMEPAD 
