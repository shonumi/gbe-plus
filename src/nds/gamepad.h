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

#include "SDL2/SDL.h"
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

	int pad;
	u16 key_input;
	u16 ext_key_input;

	private:

	//Shadow status for keyboard input
	bool up_shadow, down_shadow, left_shadow, right_shadow;

	SDL_Joystick* jstick;
};

#endif // NDS_GAMEPAD