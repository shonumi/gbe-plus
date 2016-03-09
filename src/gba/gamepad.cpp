// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gamepad.h
// Date : October 12, 2014
// Description : Game Boy Advance joypad emulation and input handling
//
// Reads and writes to the KEYINPUT and KEYCNT registers
// Handles input from keyboard using SDL events

#include "gamepad.h"

/****** GamePad Constructor *******/
AGB_GamePad::AGB_GamePad()
{
	pad = 0;
	key_input = 0x3FF;
	jstick = NULL;
	up_shadow = down_shadow = left_shadow = right_shadow = false;
}

/****** Initialize GamePad ******/
void AGB_GamePad::init()
{
	jstick = NULL;
	jstick = SDL_JoystickOpen(0);

	if((jstick == NULL) && (SDL_NumJoysticks() >= 1)) { std::cout<<"JOY::Could not initialize joystick \n"; }
	else if((jstick == NULL) && (SDL_NumJoysticks() == 0)) { std::cout<<"JOY::No joysticks detected \n"; }
}

/****** GamePad Destructor ******/
AGB_GamePad::~AGB_GamePad() { } 

/****** Handle input from keyboard or joystick for processing ******/
void AGB_GamePad::handle_input(SDL_Event &event)
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
		pad = 100 + event.jbutton.button;
		process_joystick(pad, true);
	}

	//Joystick Button Releases
	else if(event.type == SDL_JOYBUTTONUP)
	{
		pad = 100 + event.jbutton.button;
		process_joystick(pad, false);
	}

	//Joystick axes
	else if(event.type == SDL_JOYAXISMOTION)
	{
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
void AGB_GamePad::process_keyboard(int pad, bool pressed)
{
	//Emulate A button press
	if((pad == config::agb_key_a) && (pressed)) { key_input &= ~0x1; }

	//Emulate A button release
	else if((pad == config::agb_key_a) && (!pressed)) { key_input |= 0x1; }

	//Emulate B button press
	else if((pad == config::agb_key_b) && (pressed)) { key_input &= ~0x2; }

	//Emulate B button release
	else if((pad == config::agb_key_b) && (!pressed)) { key_input |= 0x2; }

	//Emulate Select button press
	else if((pad == config::agb_key_select) && (pressed)) { key_input &= ~0x4; }

	//Emulate Select button release
	else if((pad == config::agb_key_select) && (!pressed)) { key_input |= 0x4; }

	//Emulate Start button press
	else if((pad == config::agb_key_start) && (pressed)) { key_input &= ~0x8; }

	//Emulate Start button release
	else if((pad == config::agb_key_start) && (!pressed)) { key_input |= 0x8; }

	//Emulate Right DPad press
	else if((pad == config::agb_key_right) && (pressed)) { key_input &= ~0x10; key_input |= 0x20; right_shadow = true; }

	//Emulate Right DPad release
	else if((pad == config::agb_key_right) && (!pressed)) 
	{
		right_shadow = false; 
		key_input |= 0x10;

		if(left_shadow) { key_input &= ~0x20; }
		else { key_input |= 0x20; }
	}

	//Emulate Left DPad press
	else if((pad == config::agb_key_left) && (pressed)) { key_input &= ~0x20; key_input |= 0x10; left_shadow = true; }

	//Emulate Left DPad release
	else if((pad == config::agb_key_left) && (!pressed)) 
	{
		left_shadow = false;
		key_input |= 0x20;
		
		if(right_shadow) { key_input &= ~0x10; }
		else { key_input |= 0x10; } 
	}

	//Emulate Up DPad press
	else if((pad == config::agb_key_up) && (pressed)) { key_input &= ~0x40; key_input |= 0x80; up_shadow = true; }

	//Emulate Up DPad release
	else if((pad == config::agb_key_up) && (!pressed)) 
	{
		up_shadow = false; 
		key_input |= 0x40;
		
		if(down_shadow) { key_input &= ~0x80; }
		else { key_input |= 0x80; }
	}

	//Emulate Down DPad press
	else if((pad == config::agb_key_down) && (pressed)) { key_input &= ~0x80; key_input |= 0x40; down_shadow = true; }

	//Emulate Down DPad release
	else if((pad == config::agb_key_down) && (!pressed)) 
	{
		down_shadow = false;
		key_input |= 0x80;

		if(up_shadow) { key_input &= ~0x40; }
		else { key_input |= 0x40; } 
	}

	//Emulate R Trigger press
	else if((pad == config::agb_key_r_trigger) && (pressed)) { key_input &= ~0x100; }

	//Emulate R Trigger release
	else if((pad == config::agb_key_r_trigger) && (!pressed)) { key_input |= 0x100; }

	//Emulate L Trigger press
	else if((pad == config::agb_key_l_trigger) && (pressed)) { key_input &= ~0x200; }

	//Emulate L Trigger release
	else if((pad == config::agb_key_l_trigger) && (!pressed)) { key_input |= 0x200; }
}

/****** Processes input based on unique pad # for joysticks ******/
void AGB_GamePad::process_joystick(int pad, bool pressed)
{
	//Emulate A button press
	if((pad == config::agb_joy_a) && (pressed)) { key_input &= ~0x1; }

	//Emulate A button release
	else if((pad == config::agb_joy_a) && (!pressed)) { key_input |= 0x1; }

	//Emulate B button press
	else if((pad == config::agb_joy_b) && (pressed)) { key_input &= ~0x2; }

	//Emulate B button release
	else if((pad == config::agb_joy_b) && (!pressed)) { key_input |= 0x2; }

	//Emulate Select button press
	else if((pad == config::agb_joy_select) && (pressed)) { key_input &= ~0x4; }

	//Emulate Select button release
	else if((pad == config::agb_joy_select) && (!pressed)) { key_input |= 0x4; }

	//Emulate Start button press
	else if((pad == config::agb_joy_start) && (pressed)) { key_input &= ~0x8; }

	//Emulate Start button release
	else if((pad == config::agb_joy_start) && (!pressed)) { key_input |= 0x8; }

	//Emulate Right DPad press
	else if((pad == config::agb_joy_right) && (pressed)) { key_input &= ~0x10; key_input |= 0x20; }

	//Emulate Right DPad release
	else if((pad == config::agb_joy_right) && (!pressed)) { key_input |= 0x10; key_input |= 0x20;}

	//Emulate Left DPad press
	else if((pad == config::agb_joy_left) && (pressed)) { key_input &= ~0x20; key_input |= 0x10; }

	//Emulate Left DPad release
	else if((pad == config::agb_joy_left) && (!pressed)) { key_input |= 0x20; key_input |= 0x10; }

	//Emulate Up DPad press
	else if((pad == config::agb_joy_up) && (pressed)) { key_input &= ~0x40; key_input |= 0x80; }

	//Emulate Up DPad release
	else if((pad == config::agb_joy_up) && (!pressed)) { key_input |= 0x40; key_input |= 0x80;}

	//Emulate Down DPad press
	else if((pad == config::agb_joy_down) && (pressed)) { key_input &= ~0x80; key_input |= 0x40;}

	//Emulate Down DPad release
	else if((pad == config::agb_joy_down) && (!pressed)) { key_input |= 0x80; key_input |= 0x40; }

	//Emulate R Trigger press
	else if((pad == config::agb_joy_r_trigger) && (pressed)) { key_input &= ~0x100; }

	//Emulate R Trigger release
	else if((pad == config::agb_joy_r_trigger) && (!pressed)) { key_input |= 0x100; }

	//Emulate L Trigger press
	else if((pad == config::agb_joy_l_trigger) && (pressed)) { key_input &= ~0x200; }

	//Emulate L Trigger release
	else if((pad == config::agb_joy_l_trigger) && (!pressed)) { key_input |= 0x200; }
}

/****** Clears any existing input - Primarily used for the SoftReset SWI ******/
void AGB_GamePad::clear_input()
{
	key_input = 0x3FF;
	up_shadow = down_shadow = left_shadow = right_shadow = false;
}
