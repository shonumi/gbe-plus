// GB Enhanced+ Copyright Daniel Baxter 2021
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gamepad.cpp
// Date : April 03, 2021
// Description : Pokemon Mini keypad emulation and input handling
//
// Handles input from keyboard using SDL events

#include "gamepad.h"

/****** GamePad Constructor *******/
MIN_GamePad::MIN_GamePad()
{
	pad = 0;
	key_input = 0xFF;
	jstick = NULL;
	up_shadow = down_shadow = left_shadow = right_shadow = false;
	is_rumbling = false;

	joy_init = false;
}

/****** Initialize GamePad ******/
void MIN_GamePad::init()
{
	//Initialize joystick subsystem
	if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1)
	{
		std::cout<<"JOY::Could not initialize SDL joysticks\n";
		return;
	}

	jstick = NULL;
	jstick = SDL_JoystickOpen(config::joy_id);
	config::joy_sdl_id = SDL_JoystickInstanceID(jstick);

	joy_init = (jstick == NULL) ? true : false;

	if((jstick == NULL) && (SDL_NumJoysticks() >= 1)) { std::cout<<"JOY::Could not initialize joystick \n"; }
	else if((jstick == NULL) && (SDL_NumJoysticks() == 0)) { std::cout<<"JOY::No joysticks detected \n"; return; }

	rumble = NULL;

	//Open haptics for rumbling
	if(config::use_haptics)
	{
		if(SDL_InitSubSystem(SDL_INIT_HAPTIC) == -1)
		{
			std::cout<<"JOY::Could not initialize SDL haptics\n";
			return;
		}

		rumble = SDL_HapticOpenFromJoystick(jstick);

		if(rumble == NULL) { std::cout<<"JOY::Could not init rumble \n"; }
	
		else
		{
			SDL_HapticRumbleInit(rumble);
			std::cout<<"JOY::Rumble initialized\n";
		}
	}
}

/****** GamePad Destructor *******/
MIN_GamePad::~MIN_GamePad() { }

/****** Handle Input From Keyboard ******/
void MIN_GamePad::handle_input(SDL_Event &event)
{
	//Key Presses
	if(event.type == SDL_KEYDOWN)
	{
		pad = event.key.keysym.sym;
		process_keyboard(pad, true);
	}

	//Key Releases
	else if(event.type == SDL_KEYUP)
	{
		pad = event.key.keysym.sym;
		process_keyboard(pad, false);
	}

	//Joystick Button Presses
	else if(event.type == SDL_JOYBUTTONDOWN)
	{
		if(event.jbutton.which != config::joy_sdl_id) { return; }

		pad = 100 + event.jbutton.button;
		process_joystick(pad, true);
	}

	//Joystick Button Releases
	else if(event.type == SDL_JOYBUTTONUP)
	{
		if(event.jbutton.which != config::joy_sdl_id) { return; }

		pad = 100 + event.jbutton.button;
		process_joystick(pad, false);
	}

	//Joystick axes
	else if(event.type == SDL_JOYAXISMOTION)
	{
		if(event.jaxis.which != config::joy_sdl_id) { return; }

		pad = 200 + (event.jaxis.axis * 2);
		int axis_pos = event.jaxis.value;
		if(axis_pos > 0) { pad++; }
		else { axis_pos *= -1; }

		if(axis_pos > config::dead_zone) { process_joystick(pad, true); }
		else { process_joystick(pad, false); }
	}

	//Joystick hats
        else if(event.type == SDL_JOYHATMOTION)
	{
		if(event.jhat.which != config::joy_sdl_id) { return; }

		pad = 300;
		pad += event.jhat.hat * 4;

		switch(event.jhat.value)
		{
			case SDL_HAT_LEFT:
				process_joystick(pad, true);
				process_joystick(pad+2, false);
				break;

			case SDL_HAT_LEFTUP:
				process_joystick(pad, true);
				process_joystick(pad+2, true);
				break;

			case SDL_HAT_LEFTDOWN:
				process_joystick(pad, true);
				process_joystick(pad+3, true);
				break;

			case SDL_HAT_RIGHT:
				process_joystick(pad+1, true);
				process_joystick(pad+2, false);
				break;

			case SDL_HAT_RIGHTUP:
				process_joystick(pad+1, true);
				process_joystick(pad+2, true);
				break;

			case SDL_HAT_RIGHTDOWN:
				process_joystick(pad+1, true);
				process_joystick(pad+3, true);
				break;

			case SDL_HAT_UP:
				process_joystick(pad+2, true);
				process_joystick(pad, false);
				break;

			case SDL_HAT_DOWN:
				process_joystick(pad+3, true);
				process_joystick(pad, false);
				break;

			case SDL_HAT_CENTERED:
				process_joystick(pad, false);
				process_joystick(pad+2, false);
				break;
		}
	}
}

/****** Processes input based on unique pad # for keyboards ******/
void MIN_GamePad::process_keyboard(int pad, bool pressed)
{
	//Emulate A button press
	if((pad == config::gbe_key_a) && (pressed)) { key_input &= ~0x1; }

	//Emulate A button release
	else if((pad == config::gbe_key_a) && (!pressed)) { key_input |= 0x1; }

	//Emulate B button press
	else if((pad == config::gbe_key_b) && (pressed)) { key_input &= ~0x2; }

	//Emulate B button release
	else if((pad == config::gbe_key_b) && (!pressed)) { key_input |= 0x2; }

	//Emulate Power button press
	else if((pad == config::gbe_key_select) && (pressed)) { key_input &= ~0x80; }

	//Emulate Power button release
	else if((pad == config::gbe_key_select) && (!pressed)) { key_input |= 0x80; }

	//Emulate C button press
	else if((pad == config::gbe_key_start) && (pressed)) { key_input &= ~0x4; }

	//Emulate C button release
	else if((pad == config::gbe_key_start) && (!pressed)) { key_input |= 0x4; }

	//Emulate Right DPad press
	else if((pad == config::gbe_key_right) && (pressed)) { key_input &= ~0x40; key_input |= 0x20; right_shadow = true; }

	//Emulate Right DPad release
	else if((pad == config::gbe_key_right) && (!pressed)) 
	{
		right_shadow = false; 
		key_input |= 0x40;

		if(left_shadow) { key_input &= ~0x20; }
		else { key_input |= 0x20; }
	}

	//Emulate Left DPad press
	else if((pad == config::gbe_key_left) && (pressed)) { key_input &= ~0x20; key_input |= 0x40; left_shadow = true; }

	//Emulate Left DPad release
	else if((pad == config::gbe_key_left) && (!pressed)) 
	{
		left_shadow = false;
		key_input |= 0x20;
		
		if(right_shadow) { key_input &= ~0x40; }
		else { key_input |= 0x40; } 
	}

	//Emulate Up DPad press
	else if((pad == config::gbe_key_up) && (pressed)) { key_input &= ~0x8; key_input |= 0x10; up_shadow = true; }

	//Emulate Up DPad release
	else if((pad == config::gbe_key_up) && (!pressed)) 
	{
		up_shadow = false; 
		key_input |= 0x8;
		
		if(down_shadow) { key_input &= ~0x10; }
		else { key_input |= 0x10; }
	}

	//Emulate Down DPad press
	else if((pad == config::gbe_key_down) && (pressed)) { key_input &= ~0x10; key_input |= 0x8; down_shadow = true; }

	//Emulate Down DPad release
	else if((pad == config::gbe_key_down) && (!pressed)) 
	{
		down_shadow = false;
		key_input |= 0x10;

		if(up_shadow) { key_input &= ~0x8; }
		else { key_input |= 0x8; } 
	}
}

/****** Processes input based on unique pad # for joysticks ******/
void MIN_GamePad::process_joystick(int pad, bool pressed)
{
	//Emulate A button press
	if((pad == config::gbe_joy_a) && (pressed)) { key_input &= ~0x1; }

	//Emulate A button release
	else if((pad == config::gbe_joy_a) && (!pressed)) { key_input |= 0x1; }

	//Emulate B button press
	else if((pad == config::gbe_joy_b) && (pressed)) { key_input &= ~0x2; }

	//Emulate B button release
	else if((pad == config::gbe_joy_b) && (!pressed)) { key_input |= 0x2; }

	//Emulate Power button press
	else if((pad == config::gbe_joy_select) && (pressed)) { key_input &= ~0x80; }

	//Emulate Power button release
	else if((pad == config::gbe_joy_select) && (!pressed)) { key_input |= 0x80; }

	//Emulate C button press
	else if((pad == config::gbe_joy_start) && (pressed)) { key_input &= ~0x4; }

	//Emulate C button release
	else if((pad == config::gbe_joy_start) && (!pressed)) { key_input |= 0x4; }

	//Emulate Right DPad press
	else if((pad == config::gbe_joy_right) && (pressed)) { key_input &= ~0x40; key_input |= 0x20; }

	//Emulate Right DPad release
	else if((pad == config::gbe_joy_right) && (!pressed)) { key_input |= 0x40; key_input |= 0x20;}

	//Emulate Left DPad press
	else if((pad == config::gbe_joy_left) && (pressed)) { key_input &= ~0x20; key_input |= 0x40; }

	//Emulate Left DPad release
	else if((pad == config::gbe_joy_left) && (!pressed)) { key_input |= 0x20; key_input |= 0x40; }

	//Emulate Up DPad press
	else if((pad == config::gbe_joy_up) && (pressed)) { key_input &= ~0x8; key_input |= 0x10; }

	//Emulate Up DPad release
	else if((pad == config::gbe_joy_up) && (!pressed)) { key_input |= 0x8; key_input |= 0x10;}

	//Emulate Down DPad press
	else if((pad == config::gbe_joy_down) && (pressed)) { key_input &= ~0x10; key_input |= 0x8;}

	//Emulate Down DPad release
	else if((pad == config::gbe_joy_down) && (!pressed)) { key_input |= 0x10; key_input |= 0x8; }
}

/****** Start haptic force-feedback on joypad ******/
void MIN_GamePad::start_rumble()
{
	if((jstick != NULL) && (rumble != NULL) && (is_rumbling == false))
	{
		SDL_HapticRumblePlay(rumble, 1, -1);
		is_rumbling = true;
	}
}

/****** Stop haptic force-feedback on joypad ******/
void MIN_GamePad::stop_rumble()
{
	if((jstick != NULL) && (rumble != NULL) && (is_rumbling == true))
	{
		SDL_HapticRumbleStop(rumble);
       		is_rumbling = false;
	}
}
