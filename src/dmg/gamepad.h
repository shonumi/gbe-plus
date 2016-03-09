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

#include "SDL/SDL.h"
#include <string>
#include <iostream>

#include "common.h"
#include "common/config.h"

class DMG_GamePad
{
	public:

	u8 p14, p15;
	u8 column_id;

	u16 sensor_x;
	u16 sensor_y;
	u8 gyro_flags;

	int pad;

	//Shadow status for keyboard input
	bool up_shadow, down_shadow, left_shadow, right_shadow;

	SDL_Joystick* jstick;

	DMG_GamePad();
	~DMG_GamePad();

	void handle_input(SDL_Event &event);
	void init();

	void process_keyboard(int pad, bool pressed);
	void process_joystick(int pad, bool pressed);
	void process_gyroscope();
	u8 read();
};

#endif // GB_GAMEPAD 
