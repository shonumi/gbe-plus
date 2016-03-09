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

#include "SDL/SDL.h"
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

	int pad;
	u16 key_input;

	private:

	//Shadow status for keyboard input
	bool up_shadow, down_shadow, left_shadow, right_shadow;

	SDL_Joystick* jstick;
};

#endif // GBA_GAMEPAD