// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gamepad.h
// Date : November 13, 2016
// Description : NDS joypad emulation and input handling
//
// Reads and writes to the KEYINPUT, EXTKEYIN and KEYCNT registers
// Handles input from keyboard using SDL events

#include "gamepad.h"

/****** GamePad Constructor *******/
NTR_GamePad::NTR_GamePad()
{
	pad = 0;
	key_input = 0x3FF;
	ext_key_input = 0x7F;
	jstick = NULL;
	mouse_x = 0;
	mouse_y = 0;
	up_shadow = down_shadow = left_shadow = right_shadow = false;
}

/****** Initialize GamePad ******/
void NTR_GamePad::init()
{
	//Initialize joystick subsystem
	if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1)
	{
		std::cout<<"JOY::Could not initialize SDL joysticks\n";
		return;
	}

	jstick = NULL;
	jstick = SDL_JoystickOpen(config::joy_id);

	if((jstick == NULL) && (SDL_NumJoysticks() >= 1)) { std::cout<<"JOY::Could not initialize joystick \n"; }
	else if((jstick == NULL) && (SDL_NumJoysticks() == 0)) { std::cout<<"JOY::No joysticks detected \n"; }
}

/****** GamePad Destructor ******/
NTR_GamePad::~NTR_GamePad() { } 

/****** Handle input from keyboard or joystick for processing ******/
void NTR_GamePad::handle_input(SDL_Event &event)
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

	//Mouse button press
	else if(event.type == SDL_MOUSEBUTTONDOWN)
	{
		pad = 400;
		mouse_x = event.button.x;
		mouse_y = event.button.y;

		switch(event.button.button)
		{
			case SDL_BUTTON_LEFT:
				process_mouse(pad, true);
				break;

			case SDL_BUTTON_MIDDLE:
				process_mouse(pad+1, true);
				break;

			case SDL_BUTTON_RIGHT:
				process_mouse(pad+2, true);
				break;
		}
	}

	//Mouse button release
	else if(event.type == SDL_MOUSEBUTTONUP)
	{
		pad = 400;
		mouse_x = event.button.x;
		mouse_y = event.button.y;

		switch(event.button.button)
		{
			case SDL_BUTTON_LEFT:
				process_mouse(pad, false);
				break;

			case SDL_BUTTON_MIDDLE:
				process_mouse(pad+1, false);
				break;

			case SDL_BUTTON_RIGHT:
				process_mouse(pad+2, false);
				break;
		}
	}
}

/****** Processes input based on unique pad # for keyboards ******/
void NTR_GamePad::process_keyboard(int pad, bool pressed)
{
	//Emulate A button press
	if((pad == config::ntr_key_a) && (pressed)) { key_input &= ~0x1; }

	//Emulate A button release
	else if((pad == config::ntr_key_a) && (!pressed)) { key_input |= 0x1; }

	//Emulate B button press
	else if((pad == config::ntr_key_b) && (pressed)) { key_input &= ~0x2; }

	//Emulate B button release
	else if((pad == config::ntr_key_b) && (!pressed)) { key_input |= 0x2; }

	//Emulate Select button press
	else if((pad == config::ntr_key_select) && (pressed)) { key_input &= ~0x4; }

	//Emulate Select button release
	else if((pad == config::ntr_key_select) && (!pressed)) { key_input |= 0x4; }

	//Emulate Start button press
	else if((pad == config::ntr_key_start) && (pressed)) { key_input &= ~0x8; }

	//Emulate Start button release
	else if((pad == config::ntr_key_start) && (!pressed)) { key_input |= 0x8; }

	//Emulate Right DPad press
	else if((pad == config::ntr_key_right) && (pressed)) { key_input &= ~0x10; key_input |= 0x20; right_shadow = true; }

	//Emulate Right DPad release
	else if((pad == config::ntr_key_right) && (!pressed)) 
	{
		right_shadow = false; 
		key_input |= 0x10;

		if(left_shadow) { key_input &= ~0x20; }
		else { key_input |= 0x20; }
	}

	//Emulate Left DPad press
	else if((pad == config::ntr_key_left) && (pressed)) { key_input &= ~0x20; key_input |= 0x10; left_shadow = true; }

	//Emulate Left DPad release
	else if((pad == config::ntr_key_left) && (!pressed)) 
	{
		left_shadow = false;
		key_input |= 0x20;
		
		if(right_shadow) { key_input &= ~0x10; }
		else { key_input |= 0x10; } 
	}

	//Emulate Up DPad press
	else if((pad == config::ntr_key_up) && (pressed)) { key_input &= ~0x40; key_input |= 0x80; up_shadow = true; }

	//Emulate Up DPad release
	else if((pad == config::ntr_key_up) && (!pressed)) 
	{
		up_shadow = false; 
		key_input |= 0x40;
		
		if(down_shadow) { key_input &= ~0x80; }
		else { key_input |= 0x80; }
	}

	//Emulate Down DPad press
	else if((pad == config::ntr_key_down) && (pressed)) { key_input &= ~0x80; key_input |= 0x40; down_shadow = true; }

	//Emulate Down DPad release
	else if((pad == config::ntr_key_down) && (!pressed)) 
	{
		down_shadow = false;
		key_input |= 0x80;

		if(up_shadow) { key_input &= ~0x40; }
		else { key_input |= 0x40; } 
	}

	//Emulate R Trigger press
	else if((pad == config::ntr_key_r_trigger) && (pressed)) { key_input &= ~0x100; }

	//Emulate R Trigger release
	else if((pad == config::ntr_key_r_trigger) && (!pressed)) { key_input |= 0x100; }

	//Emulate L Trigger press
	else if((pad == config::ntr_key_l_trigger) && (pressed)) { key_input &= ~0x200; }

	//Emulate L Trigger release
	else if((pad == config::ntr_key_l_trigger) && (!pressed)) { key_input |= 0x200; }
}

/****** Processes input based on unique pad # for joysticks ******/
void NTR_GamePad::process_joystick(int pad, bool pressed)
{
	//Emulate A button press
	if((pad == config::ntr_joy_a) && (pressed)) { key_input &= ~0x1; }

	//Emulate A button release
	else if((pad == config::ntr_joy_a) && (!pressed)) { key_input |= 0x1; }

	//Emulate B button press
	else if((pad == config::ntr_joy_b) && (pressed)) { key_input &= ~0x2; }

	//Emulate B button release
	else if((pad == config::ntr_joy_b) && (!pressed)) { key_input |= 0x2; }

	//Emulate Select button press
	else if((pad == config::ntr_joy_select) && (pressed)) { key_input &= ~0x4; }

	//Emulate Select button release
	else if((pad == config::ntr_joy_select) && (!pressed)) { key_input |= 0x4; }

	//Emulate Start button press
	else if((pad == config::ntr_joy_start) && (pressed)) { key_input &= ~0x8; }

	//Emulate Start button release
	else if((pad == config::ntr_joy_start) && (!pressed)) { key_input |= 0x8; }

	//Emulate Right DPad press
	else if((pad == config::ntr_joy_right) && (pressed)) { key_input &= ~0x10; key_input |= 0x20; }

	//Emulate Right DPad release
	else if((pad == config::ntr_joy_right) && (!pressed)) { key_input |= 0x10; key_input |= 0x20;}

	//Emulate Left DPad press
	else if((pad == config::ntr_joy_left) && (pressed)) { key_input &= ~0x20; key_input |= 0x10; }

	//Emulate Left DPad release
	else if((pad == config::ntr_joy_left) && (!pressed)) { key_input |= 0x20; key_input |= 0x10; }

	//Emulate Up DPad press
	else if((pad == config::ntr_joy_up) && (pressed)) { key_input &= ~0x40; key_input |= 0x80; }

	//Emulate Up DPad release
	else if((pad == config::ntr_joy_up) && (!pressed)) { key_input |= 0x40; key_input |= 0x80;}

	//Emulate Down DPad press
	else if((pad == config::ntr_joy_down) && (pressed)) { key_input &= ~0x80; key_input |= 0x40;}

	//Emulate Down DPad release
	else if((pad == config::ntr_joy_down) && (!pressed)) { key_input |= 0x80; key_input |= 0x40; }

	//Emulate R Trigger press
	else if((pad == config::ntr_joy_r_trigger) && (pressed)) { key_input &= ~0x100; }

	//Emulate R Trigger release
	else if((pad == config::ntr_joy_r_trigger) && (!pressed)) { key_input |= 0x100; }

	//Emulate L Trigger press
	else if((pad == config::ntr_joy_l_trigger) && (pressed)) { key_input &= ~0x200; }

	//Emulate L Trigger release
	else if((pad == config::ntr_joy_l_trigger) && (!pressed)) { key_input |= 0x200; }
}

/****** Processes input based on the mouse ******/
void NTR_GamePad::process_mouse(int pad, bool pressed)
{
	//Emulate touchscreen press (NDS mode only, DSi does not use this)
	if((pad == 400) && (pressed))
	{
		ext_key_input &= ~0x60;
		std::cout<<"DOWN\n";
	}

	//Emulate touchscreen release (NDS mode only, DSi does not use this)
	else if((pad == 400) && (!pressed))
	{
		ext_key_input |= 0x60;
		std::cout<<"UP\n";
	}
}

/****** Clears any existing input - Primarily used for the SoftReset SWI ******/
void NTR_GamePad::clear_input()
{
	key_input = 0x3FF;
	ext_key_input = 0x7F;
	up_shadow = down_shadow = left_shadow = right_shadow = false;
}
