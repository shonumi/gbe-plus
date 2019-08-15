// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gamepad.cpp
// Date : May 12, 2015
// Description : Game Boy joypad emulation and input handling
//
// Reads and writes to the P1 register
// Handles input from keyboard using SDL events

#include "gamepad.h"

/****** GamePad Constructor *******/
DMG_GamePad::DMG_GamePad()
{
	p14 = 0xDF;
	p15 = 0xEF;
	column_id = 0;
	pad = 0;
	up_shadow = down_shadow = left_shadow = right_shadow = false;
	sensor_x = sensor_y = 2047;
	ir_delay = 0;
	joypad_irq = false;
}

/****** Initialize GamePad ******/
void DMG_GamePad::init()
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
DMG_GamePad::~DMG_GamePad() { }

/****** Handle Input From Keyboard ******/
void DMG_GamePad::handle_input(SDL_Event &event)
{
	u16 last_input = ((p14 << 8) | p15);
	u16 next_input = 0;

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

	next_input = ((p14 << 8) | p15);

	//Update Joypad Interrupt Flag
	if((last_input != next_input) && (next_input != 0xDFEF)) { joypad_irq = true; }
	else { joypad_irq = false; }
}

/****** Processes input based on unique pad # for keyboards ******/
void DMG_GamePad::process_keyboard(int pad, bool pressed)
{
	//Emulate A button press
	if((pad == config::gbe_key_a) && (pressed)) { p14 &= ~0x1; }

	//Emulate A button release
	else if((pad == config::gbe_key_a) && (!pressed)) { p14 |= 0x1; }

	//Emulate B button press
	else if((pad == config::gbe_key_b) && (pressed)) { p14 &= ~0x2; }

	//Emulate B button release
	else if((pad == config::gbe_key_b) && (!pressed)) { p14 |= 0x2; }

	//Emulate Select button press
	else if((pad == config::gbe_key_select) && (pressed)) { p14 &= ~0x4; }

	//Emulate Select button release
	else if((pad == config::gbe_key_select) && (!pressed)) { p14 |= 0x4; }

	//Emulate Start button press
	else if((pad == config::gbe_key_start) && (pressed)) { p14 &= ~0x8; }

	//Emulate Start button release
	else if((pad == config::gbe_key_start) && (!pressed)) { p14 |= 0x8; }

	//Emulate Right DPad press
	else if((pad == config::gbe_key_right) && (pressed)) { p15 &= ~0x1; p15 |= 0x2; right_shadow = true; }

	//Emulate Right DPad release
	else if((pad == config::gbe_key_right) && (!pressed)) 
	{
		right_shadow = false; 
		p15 |= 0x1;

		if(left_shadow) { p15 &= ~0x2; }
		else { p15 |= 0x2; }
	}

	//Emulate Left DPad press
	else if((pad == config::gbe_key_left) && (pressed)) { p15 &= ~0x2; p15 |= 0x1; left_shadow = true; }

	//Emulate Left DPad release
	else if((pad == config::gbe_key_left) && (!pressed)) 
	{
		left_shadow = false;
		p15 |= 0x2;
		
		if(right_shadow) { p15 &= ~0x1; }
		else { p15 |= 0x1; } 
	}

	//Emulate Up DPad press
	else if((pad == config::gbe_key_up) && (pressed)) { p15 &= ~0x4; p15 |= 0x8; up_shadow = true; }

	//Emulate Up DPad release
	else if((pad == config::gbe_key_up) && (!pressed)) 
	{
		up_shadow = false; 
		p15 |= 0x4;
		
		if(down_shadow) { p15 &= ~0x8; }
		else { p15 |= 0x8; }
	}

	//Emulate Down DPad press
	else if((pad == config::gbe_key_down) && (pressed)) { p15 &= ~0x8; p15 |= 0x4; down_shadow = true; }

	//Emulate Down DPad release
	else if((pad == config::gbe_key_down) && (!pressed)) 
	{
		down_shadow = false;
		p15 |= 0x8;

		if(up_shadow) { p15 &= ~0x4; }
		else { p15 |= 0x4; } 
	}

	//Emulate Gyroscope Left tilt press
	else if((pad == config::con_key_left) && (pressed))
	{
		gyro_flags |= 0x1;
		gyro_flags |= 0x10;

		gyro_flags &= ~0x2;
	}

	//Emulate Gyroscope Left tilt release
	else if((pad == config::con_key_left) && (!pressed))
	{
		gyro_flags &= ~0x1;
		gyro_flags &= ~0x10;

		if(gyro_flags & 0x20) { gyro_flags |= 0x2; }
		else { gyro_flags &= ~0x2; }
	}

	//Emulate Gyroscope Right tilt press
	else if((pad == config::con_key_right) && (pressed))
	{
		gyro_flags |= 0x2;
		gyro_flags |= 0x20;

		gyro_flags &= ~0x1;
	}

	//Emulate Gyroscope Right tilt release
	else if((pad == config::con_key_right) && (!pressed))
	{
		gyro_flags &= ~0x2;
		gyro_flags &= ~0x20;

		if(gyro_flags & 0x10) { gyro_flags |= 0x1; }
		else { gyro_flags &= ~0x1; }
	}

	//Emulate Gyroscope Up tilt press
	else if((pad == config::con_key_up) && (pressed))
	{
		gyro_flags |= 0x4;
		gyro_flags |= 0x40;

		gyro_flags &= ~0x8;

		//Reset constant IR light source
		if((config::ir_device == 5) && (ir_delay == 0)) { ir_delay = 10; }
	}

	//Emulate Gyroscope Up tilt release
	else if((pad == config::con_key_up) && (!pressed))
	{
		gyro_flags &= ~0x4;
		gyro_flags &= ~0x40;

		if(gyro_flags & 0x80) { gyro_flags |= 0x8; }
		else { gyro_flags &= ~0x8; }
	}

	//Emulate Gyroscope Down tilt press
	else if((pad == config::con_key_down) && (pressed))
	{
		gyro_flags |= 0x8;
		gyro_flags |= 0x80;

		gyro_flags &= ~0x4;
	}

	//Emulate Gyroscope Down tilt release
	else if((pad == config::con_key_down) && (!pressed))
	{
		gyro_flags &= ~0x8;
		gyro_flags &= ~0x80;

		if(gyro_flags & 0x40) { gyro_flags |= 0x4; }
		else { gyro_flags &= ~0x4; }
	}

	//Emulate R Trigger press - DMG/GBC on GBA ONLY
	else if((pad == config::gbe_key_r_trigger) && (pressed) && (config::gba_enhance))
	{
		config::request_resize = true;
		config::resize_mode--;
		
		if(config::resize_mode < 0) { config::resize_mode = 0; }
	}

	//Emulate L Trigger press - DMG/GBC on GBA ONLY
	else if((pad == config::gbe_key_l_trigger) && (pressed) && (config::gba_enhance))
	{
		config::request_resize = true;
		config::resize_mode++;

		if(config::resize_mode > 2) { config::resize_mode = 2; }
	}
}

/****** Processes input based on unique pad # for joysticks ******/
void DMG_GamePad::process_joystick(int pad, bool pressed)
{
	//Emulate A button press
	if((pad == config::gbe_joy_a) && (pressed)) { p14 &= ~0x1; }

	//Emulate A button release
	else if((pad == config::gbe_joy_a) && (!pressed)) { p14 |= 0x1; }

	//Emulate B button press
	else if((pad == config::gbe_joy_b) && (pressed)) { p14 &= ~0x2; }

	//Emulate B button release
	else if((pad == config::gbe_joy_b) && (!pressed)) { p14 |= 0x2; }

	//Emulate Select button press
	else if((pad == config::gbe_joy_select) && (pressed)) { p14 &= ~0x4; }

	//Emulate Select button release
	else if((pad == config::gbe_joy_select) && (!pressed)) { p14 |= 0x4; }

	//Emulate Start button press
	else if((pad == config::gbe_joy_start) && (pressed)) { p14 &= ~0x8; }

	//Emulate Start button release
	else if((pad == config::gbe_joy_start) && (!pressed)) { p14 |= 0x8; }

	//Emulate Right DPad press
	else if((pad == config::gbe_joy_right) && (pressed)) { p15 &= ~0x1; p15 |= 0x2; }

	//Emulate Right DPad release
	else if((pad == config::gbe_joy_right) && (!pressed)) { p15 |= 0x1; p15 |= 0x2;}

	//Emulate Left DPad press
	else if((pad == config::gbe_joy_left) && (pressed)) { p15 &= ~0x2; p15 |= 0x1; }

	//Emulate Left DPad release
	else if((pad == config::gbe_joy_left) && (!pressed)) { p15 |= 0x2; p15 |= 0x1; }

	//Emulate Up DPad press
	else if((pad == config::gbe_joy_up) && (pressed)) { p15 &= ~0x4; p15 |= 0x8; }

	//Emulate Up DPad release
	else if((pad == config::gbe_joy_up) && (!pressed)) { p15 |= 0x4; p15 |= 0x8;}

	//Emulate Down DPad press
	else if((pad == config::gbe_joy_down) && (pressed)) { p15 &= ~0x8; p15 |= 0x4;}

	//Emulate Down DPad release
	else if((pad == config::gbe_joy_down) && (!pressed)) { p15 |= 0x8; p15 |= 0x4; }

	//Emulate Gyroscope Left tilt press
	else if((pad == config::con_joy_left) && (pressed)) { gyro_flags |= 0x1; gyro_flags &= ~0x2; }

	//Emulate Gyroscope Left tilt release
	else if((pad == config::con_joy_left) && (!pressed)) { gyro_flags &= ~0x1; }

	//Emulate Gyroscope Right tilt press
	else if((pad == config::con_joy_right) && (pressed)) { gyro_flags |= 0x2; gyro_flags &= ~0x1; }

	//Emulate Gyroscope Right tilt release
	else if((pad == config::con_joy_right) && (!pressed)) { gyro_flags &= ~0x2; }

	//Emulate Gyroscope Up tilt press
	else if((pad == config::con_joy_up) && (pressed))
	{
		gyro_flags |= 0x4;
		gyro_flags &= ~0x8;

		//Reset constant IR light source
		if((config::ir_device == 5) && (ir_delay == 0)) { ir_delay = 10; }
	}

	//Emulate Gyroscope Up tilt release
	else if((pad == config::con_joy_up) && (!pressed)) { gyro_flags &= ~0x4; }

	//Emulate Gyroscope Down tilt press
	else if((pad == config::con_joy_down) && (pressed)) { gyro_flags |= 0x8; gyro_flags &= ~0x4; }

	//Emulate Gyroscope Down tilt release
	else if((pad == config::con_joy_down) && (!pressed)) { gyro_flags &= ~0x8; }

	//Emulate R Trigger press - DMG/GBC on GBA ONLY
	else if((pad == config::gbe_joy_r_trigger) && (pressed) && (config::gba_enhance))
	{
		config::request_resize = true;
		config::resize_mode--;
		
		if(config::resize_mode < 0) { config::resize_mode = 0; }
	}

	//Emulate L Trigger press - DMG/GBC on GBA ONLY
	else if((pad == config::gbe_joy_l_trigger) && (pressed) && (config::gba_enhance))
	{
		config::request_resize = true;
		config::resize_mode++;

		if(config::resize_mode > 2) { config::resize_mode = 2; }
	}
}

/****** Process gyroscope sensors - Only used for MBC7 ******/
void DMG_GamePad::process_gyroscope()
{
	//When pressing left, increase sensor_x
	if(gyro_flags & 0x1) 
	{
		sensor_x += 3;

		//Limit X to a max of 2197
		//When it's lower than the minimum, bump it up right away 
		if(sensor_x > 2197) { sensor_x = 2197; }
    		if(sensor_x < 2047) { sensor_x = 2057; }
	}

	//When pressing right, decrease sensor_x
	else if(gyro_flags & 0x2) 
	{
		sensor_x -= 3;

		//Limit X to a minimum of 1847
		//When it's lower than the minimum, bump it up right away 
    		if(sensor_x < 1897) { sensor_x = 1897; }
    		if(sensor_x > 2047) { sensor_x = 2037; }
  	}
	
	//When neither left or right is pressed, put the sensor in neutral
	else if(sensor_x > 2047) 
	{
    		sensor_x -= 2;
    		if(sensor_x < 2047) { sensor_x = 2047; } 
	}
	
	else if(sensor_x < 2047)
	{
    		sensor_x += 2;
    		if(sensor_x > 2047) { sensor_x = 2047; }
  	}

	//When pressing up, increase sensor_y
	if(gyro_flags & 0x4) 
	{
		sensor_y += 3;

		//Limit Y to a max of 2197
		//When it's lower than the minimum, bump it up right away 
		if(sensor_y > 2197) { sensor_y = 2197; }
    		if(sensor_y < 2047) { sensor_y = 2057; }
	}

	//When pressing down, decrease sensor_y
	else if(gyro_flags & 0x8) 
	{
		sensor_y -= 3;

		//Limit X to a minimum of 1847
		//When it's lower than the minimum, bump it up right away 
    		if(sensor_y < 1897) { sensor_y = 1897; }
    		if(sensor_y > 2047) { sensor_y = 2037; }
  	}
	
	//When neither up or down is pressed, put the sensor in neutral
	else if(sensor_y > 2047) 
	{
    		sensor_y -= 2;
    		if(sensor_y < 2047) { sensor_y = 2047; } 
	}
	
	else if(sensor_y < 2047)
	{
    		sensor_y += 2;
    		if(sensor_y > 2047) { sensor_y = 2047; }
  	}
}

/****** Start haptic force-feedback on joypad ******/
void DMG_GamePad::start_rumble()
{
	if((jstick != NULL) && (rumble != NULL) && (is_rumbling == false))
	{
		SDL_HapticRumblePlay(rumble, 1, -1);
		is_rumbling = true;
	}
}

/****** Stop haptic force-feedback on joypad ******/
void DMG_GamePad::stop_rumble()
{
	if((jstick != NULL) && (rumble != NULL) && (is_rumbling == true))
	{
		SDL_HapticRumbleStop(rumble);
       		is_rumbling = false;
	}
}

/****** Update P1 ******/
u8 DMG_GamePad::read()
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
void DMG_GamePad::write(u8 value) { column_id = (value & 0x30); }

/****** Grabs misc pad data ******/
u32 DMG_GamePad::get_pad_data(u32 index) { return 0; }

/****** Sets misc pad data ******/
void DMG_GamePad::set_pad_data(u32 index, u32 value) { }
