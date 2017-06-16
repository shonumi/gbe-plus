// GB Enhanced+ Copyright Daniel Baxter 2017
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gamepad.h
// Date : June 13, 2017
// Description : Super Game Boy joypad emulation and input handling
//
// Reads and writes to the P1 register
// Handles input from keyboard using SDL events
// Handles SGB commands

#ifndef SGB_GAMEPAD
#define SGB_GAMEPAD

#include "SDL2/SDL.h"
#include <string>
#include <iostream>

#include "dmg/common.h"
#include "common/config.h"
#include "common/dmg_core_pad.h"

class SGB_GamePad : virtual public dmg_core_pad
{
	public:

	SGB_GamePad();
	~SGB_GamePad();

	void handle_input(SDL_Event &event);
	void init();

	void process_keyboard(int pad, bool pressed);
	void process_joystick(int pad, bool pressed);
	void process_gyroscope();
	void start_rumble();
	void stop_rumble();
	u8 read();
	void write(u8 value);

	struct sgb_packets
	{
		u8 state;
		u8 command;
		u8 ptr;
		u8 length;
		u8 bit_count;
		u8 data_count;
		std::vector<u8> data;
		bool lcd_command;
	} packet;
};

#endif // SGB_GAMEPAD 
