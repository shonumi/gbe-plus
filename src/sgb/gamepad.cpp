// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gamepad.cpp
// Date : June 13, 2017
// Description : Game Boy joypad emulation and input handling
//
// Reads and writes to the P1 register
// Handles input from keyboard using SDL events
// Handles SGB commands

#include "gamepad.h"

/****** GamePad Constructor *******/
SGB_GamePad::SGB_GamePad()
{
	p14 = 0xDF;
	p15 = 0xEF;
	column_id = 0;
	pad = 0;
	up_shadow = down_shadow = left_shadow = right_shadow = false;

	packet.state = 0;
	packet.command = 0;
	packet.length = 0;
	packet.ptr = 0;
	packet.bit_count = 0;
	packet.data.resize(0x80, 0);
	packet.lcd_command = false;
}

/****** Initialize GamePad ******/
void SGB_GamePad::init()
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
SGB_GamePad::~SGB_GamePad() { }

/****** Handle Input From Keyboard ******/
void SGB_GamePad::handle_input(SDL_Event &event)
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
void SGB_GamePad::process_keyboard(int pad, bool pressed)
{
	//Emulate A button press
	if((pad == config::dmg_key_a) && (pressed)) { p14 &= ~0x1; }

	//Emulate A button release
	else if((pad == config::dmg_key_a) && (!pressed)) { p14 |= 0x1; }

	//Emulate B button press
	else if((pad == config::dmg_key_b) && (pressed)) { p14 &= ~0x2; }

	//Emulate B button release
	else if((pad == config::dmg_key_b) && (!pressed)) { p14 |= 0x2; }

	//Emulate Select button press
	else if((pad == config::dmg_key_select) && (pressed)) { p14 &= ~0x4; }

	//Emulate Select button release
	else if((pad == config::dmg_key_select) && (!pressed)) { p14 |= 0x4; }

	//Emulate Start button press
	else if((pad == config::dmg_key_start) && (pressed)) { p14 &= ~0x8; }

	//Emulate Start button release
	else if((pad == config::dmg_key_start) && (!pressed)) { p14 |= 0x8; }

	//Emulate Right DPad press
	else if((pad == config::dmg_key_right) && (pressed)) { p15 &= ~0x1; p15 |= 0x2; right_shadow = true; }

	//Emulate Right DPad release
	else if((pad == config::dmg_key_right) && (!pressed)) 
	{
		right_shadow = false; 
		p15 |= 0x1;

		if(left_shadow) { p15 &= ~0x2; }
		else { p15 |= 0x2; }
	}

	//Emulate Left DPad press
	else if((pad == config::dmg_key_left) && (pressed)) { p15 &= ~0x2; p15 |= 0x1; left_shadow = true; }

	//Emulate Left DPad release
	else if((pad == config::dmg_key_left) && (!pressed)) 
	{
		left_shadow = false;
		p15 |= 0x2;
		
		if(right_shadow) { p15 &= ~0x1; }
		else { p15 |= 0x1; } 
	}

	//Emulate Up DPad press
	else if((pad == config::dmg_key_up) && (pressed)) { p15 &= ~0x4; p15 |= 0x8; up_shadow = true; }

	//Emulate Up DPad release
	else if((pad == config::dmg_key_up) && (!pressed)) 
	{
		up_shadow = false; 
		p15 |= 0x4;
		
		if(down_shadow) { p15 &= ~0x8; }
		else { p15 |= 0x8; }
	}

	//Emulate Down DPad press
	else if((pad == config::dmg_key_down) && (pressed)) { p15 &= ~0x8; p15 |= 0x4; down_shadow = true; }

	//Emulate Down DPad release
	else if((pad == config::dmg_key_down) && (!pressed)) 
	{
		down_shadow = false;
		p15 |= 0x8;

		if(up_shadow) { p15 &= ~0x4; }
		else { p15 |= 0x4; } 
	}
}

/****** Processes input based on unique pad # for joysticks ******/
void SGB_GamePad::process_joystick(int pad, bool pressed)
{
	//Emulate A button press
	if((pad == config::dmg_joy_a) && (pressed)) { p14 &= ~0x1; }

	//Emulate A button release
	else if((pad == config::dmg_joy_a) && (!pressed)) { p14 |= 0x1; }

	//Emulate B button press
	else if((pad == config::dmg_joy_b) && (pressed)) { p14 &= ~0x2; }

	//Emulate B button release
	else if((pad == config::dmg_joy_b) && (!pressed)) { p14 |= 0x2; }

	//Emulate Select button press
	else if((pad == config::dmg_joy_select) && (pressed)) { p14 &= ~0x4; }

	//Emulate Select button release
	else if((pad == config::dmg_joy_select) && (!pressed)) { p14 |= 0x4; }

	//Emulate Start button press
	else if((pad == config::dmg_joy_start) && (pressed)) { p14 &= ~0x8; }

	//Emulate Start button release
	else if((pad == config::dmg_joy_start) && (!pressed)) { p14 |= 0x8; }

	//Emulate Right DPad press
	else if((pad == config::dmg_joy_right) && (pressed)) { p15 &= ~0x1; p15 |= 0x2; }

	//Emulate Right DPad release
	else if((pad == config::dmg_joy_right) && (!pressed)) { p15 |= 0x1; p15 |= 0x2;}

	//Emulate Left DPad press
	else if((pad == config::dmg_joy_left) && (pressed)) { p15 &= ~0x2; p15 |= 0x1; }

	//Emulate Left DPad release
	else if((pad == config::dmg_joy_left) && (!pressed)) { p15 |= 0x2; p15 |= 0x1; }

	//Emulate Up DPad press
	else if((pad == config::dmg_joy_up) && (pressed)) { p15 &= ~0x4; p15 |= 0x8; }

	//Emulate Up DPad release
	else if((pad == config::dmg_joy_up) && (!pressed)) { p15 |= 0x4; p15 |= 0x8;}

	//Emulate Down DPad press
	else if((pad == config::dmg_joy_down) && (pressed)) { p15 &= ~0x8; p15 |= 0x4;}

	//Emulate Down DPad release
	else if((pad == config::dmg_joy_down) && (!pressed)) { p15 |= 0x8; p15 |= 0x4; }
}

/****** Process gyroscope sensors - Only used for MBC7 ******/
void SGB_GamePad::process_gyroscope() { }

/****** Start haptic force-feedback on joypad ******/
void SGB_GamePad::start_rumble()
{
	if((jstick != NULL) && (rumble != NULL) && (is_rumbling == false))
	{
		SDL_HapticRumblePlay(rumble, 1, -1);
		is_rumbling = true;
	}
}

/****** Stop haptic force-feedback on joypad ******/
void SGB_GamePad::stop_rumble()
{
	if((jstick != NULL) && (rumble != NULL) && (is_rumbling == true))
	{
		SDL_HapticRumbleStop(rumble);
       		is_rumbling = false;
	}
}

/****** Update P1 ******/
u8 SGB_GamePad::read()
{
	switch(column_id)
	{
		case 0x30:
			return (p14 | p15);

		case 0x20:
			return p15;
		
		case 0x10:
			return p14;

		default:
			return 0xFF;
	}
} 

/****** Write to P1 ******/
void SGB_GamePad::write(u8 value)
{
	column_id = (value & 0x30);

	//Grab bit from SGB packet transmission
	u8 sgb_bit = 0xFF;
	
	if((value & 0x30) == 0x20) { sgb_bit = 0; }
	else if((value & 0x30) == 0x10) { sgb_bit = 1; }

	//Process incoming SGB packets
	switch(packet.state)
	{
		//RESET pulse
		case 0x0:
			if((value & 0x30) == 0)
			{
				packet.bit_count = 0;

				//Start new packet transmission
				if(packet.length == 0)
				{
					packet.state = 1;
					packet.data.resize(0x80, 0);
					packet.ptr = 0;
					packet.command = 0;
					packet.length = 0;
				}

				//Continue current transmission
				else
				{
					packet.state = 2;
					packet.data_count = 0;
				}
			}

			break;

		//Command Code + Length
		case 0x1:
			if(packet.bit_count < 4) { packet.length |= (sgb_bit << packet.bit_count); }
			else { packet.command |= (sgb_bit << (4 - packet.bit_count)); }

			packet.bit_count++;
			
			//Move to data
			if(packet.bit_count == 8)
			{
				packet.state = 2;
				packet.bit_count = 0;
				packet.data_count = 1;
			}

			break;

		//Parameter Data
		case 0x2:
			packet.data[packet.ptr] |= (sgb_bit << packet.bit_count);

			packet.bit_count++;

			if(packet.bit_count == 8)
			{
				packet.bit_count = 0;
				packet.ptr++;
				packet.data_count++;

				//Move to Stop bit
				if(packet.data_count == 16) { packet.state = 3; }
			}

			break;

		//Stop Bit
		case 0x3:
			if(sgb_bit == 0)
			{
				packet.state = 0;
				packet.length--;

				//Process SGB command
				if(packet.length == 0) { packet.lcd_command = true; }
			}

			break;
	}
}
