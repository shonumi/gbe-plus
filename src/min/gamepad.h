// GB Enhanced+ Copyright Daniel Baxter 2021
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gamepad.h
// Date : April 03, 2021
// Description : Pokemon Mini keypad emulation and input handling
//
// Handles input from keyboard using SDL events

#ifndef PM_GAMEPAD
#define PM_GAMEPAD

#include "SDL2/SDL.h"
#include <iostream>

#include "common.h"
#include "common/config.h"

class MIN_GamePad
{
	public:

	int pad;
	u8 key_input;
	bool is_rumbling;

	bool joy_init;

	MIN_GamePad();
	~MIN_GamePad();

	void handle_input(SDL_Event &event);
	void init();

	void process_keyboard(int pad, bool pressed);
	void process_joystick(int pad, bool pressed);
	void start_rumble();
	void stop_rumble();

	private:

	//Shadow status for keyboard input
	bool up_shadow, down_shadow, left_shadow, right_shadow;

	SDL_Joystick* jstick;
    	SDL_Haptic* rumble;
};

#endif // PM_GAMEPAD 
