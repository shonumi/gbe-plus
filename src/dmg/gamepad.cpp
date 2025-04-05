// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gamepad.cpp
// Date : May 12, 2015
// Description : Game Boy joypad emulation and input handling
//
// Reads and writes to the P1 register
// Handles input from keyboard using SDL events

#include "SDL_sensor.h"

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
	con_flags = 0;
	con_update = false;
	joypad_irq = false;
	joy_init = false;
	ddr_was_mapped = false;
	sensor_init = false;
	gc_sensor = NULL;

	//Swap inputs when using DDR Finger Pad mode
	if(config::use_ddr_mapping)
	{
		//Save original mappings to restore later
		ddr_key_mapping[0] = config::gbe_key_a;
		ddr_key_mapping[1] = config::gbe_key_b;
		ddr_key_mapping[2] = config::gbe_key_right;

		ddr_joy_mapping[0] = config::gbe_joy_a;
		ddr_joy_mapping[1] = config::gbe_joy_b;
		ddr_joy_mapping[2] = config::gbe_joy_right;

		//Switch to new mappings, temporarily, until core shuts down
		config::gbe_key_a = config::gbe_key_up;
		config::gbe_key_b = config::gbe_key_right;
		config::gbe_key_right = config::gbe_key_left;

		config::gbe_joy_a = config::gbe_joy_up;
		config::gbe_joy_b = config::gbe_joy_right;
		config::gbe_joy_right = config::gbe_joy_left;

		ddr_was_mapped = true;
	}

	//Check for turbo button enabled status by adding all frame delays
	//As long as the total amount of frame delays is non-zero, enable turbo buttons
	turbo_button_enabled = 0;

	for(u32 x = 0; x < 12; x++)
	{
		turbo_button_enabled += config::gbe_turbo_button[x];
		turbo_button_val[x] = 0;
		turbo_button_stat[x] = false;
		turbo_button_end[x] = false;
	}
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

	if(config::use_motion)
	{
		if(SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_SENSOR) == 0)
		{
			gc_sensor = SDL_GameControllerOpen(config::joy_id);

			SDL_GameControllerSetSensorEnabled(gc_sensor, SDL_SENSOR_ACCEL, SDL_TRUE);

			if(!SDL_GameControllerHasSensor(gc_sensor, SDL_SENSOR_ACCEL))
			{
				std::cout<<"JOY::Controller does not have an accelerometer \n";
				gc_sensor = NULL;
			}

			else
			{
				std::cout<<"JOY::Controller sensor detected\n";
				sensor_init = true;
			}
		}

		else { std::cout<<"JOY::Could not initialize SDL sensors\n"; }
	}
}

/****** GamePad Destructor *******/
DMG_GamePad::~DMG_GamePad()
{
	//When shutting down core, restore keyboard and joystick mappings if using DDR mapping
	//This is intended for an external interface that calls save_ini_file()
	if(config::use_ddr_mapping && ddr_was_mapped)
	{
		config::gbe_key_a = ddr_key_mapping[0];
		config::gbe_key_b = ddr_key_mapping[1];
		config::gbe_key_right = ddr_key_mapping[2];

		config::gbe_joy_a = ddr_joy_mapping[0];
		config::gbe_joy_b = ddr_joy_mapping[1];
		config::gbe_joy_right = ddr_joy_mapping[2];
	}
}

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

	//Controller accelerometer
	else if(event.type == SDL_CONTROLLERSENSORUPDATE)
	{
		if(gc_sensor != NULL)
		{
			float motion_data[3] = { 0.0, 0.0, 0.0 };
			SDL_GameControllerGetSensorData(gc_sensor, SDL_SENSOR_ACCEL, motion_data, 3);
			process_gyroscope(motion_data[0], motion_data[2]);
		}
	}

	next_input = ((p14 << 8) | p15);

	//Update Joypad Interrupt Flag
	if((last_input != next_input) && (next_input != 0xDFEF)) { joypad_irq = true; }
	else { joypad_irq = false; }

	//Process turbo buttons
	if(turbo_button_enabled) { process_turbo_buttons(); }
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
	//Shift sewing point left
	else if((pad == config::con_key_left) && (pressed))
	{
		gyro_flags |= 0x1;
		gyro_flags |= 0x10;
		gyro_flags &= ~0x2;

		con_flags |= 0x1;
		con_flags |= 0x10;
		con_flags &= ~0x2;
	}

	//Emulate Gyroscope Left tilt release
	//Shift sewing point left
	else if((pad == config::con_key_left) && (!pressed))
	{
		gyro_flags &= ~0x1;
		gyro_flags &= ~0x10;

		if(gyro_flags & 0x20) { gyro_flags |= 0x2; }
		else { gyro_flags &= ~0x2; }

		con_flags &= ~0x1;
		con_flags &= ~0x10;

		if(con_flags & 0x20) { con_flags |= 0x2; }
		else { con_flags &= ~0x2; }
	}

	//Emulate Gyroscope Right tilt press
	//Shift sewing point right
	else if((pad == config::con_key_right) && (pressed))
	{
		gyro_flags |= 0x2;
		gyro_flags |= 0x20;
		gyro_flags &= ~0x1;

		con_flags |= 0x2;
		con_flags |= 0x20;
		con_flags &= ~0x1;
	}

	//Emulate Gyroscope Right tilt release
	//Shift sewing point right
	else if((pad == config::con_key_right) && (!pressed))
	{
		gyro_flags &= ~0x2;
		gyro_flags &= ~0x20;

		if(gyro_flags & 0x10) { gyro_flags |= 0x1; }
		else { gyro_flags &= ~0x1; }

		con_flags &= ~0x2;
		con_flags &= ~0x20;

		if(con_flags & 0x10) { con_flags |= 0x1; }
		else { con_flags &= ~0x1; }
	}

	//Emulate Gyroscope Up tilt press
	//Shift sewing point up
	else if((pad == config::con_key_up) && (pressed))
	{
		gyro_flags |= 0x4;
		gyro_flags |= 0x40;
		gyro_flags &= ~0x8;

		con_flags |= 0x4;
		con_flags |= 0x40;
		con_flags &= ~0x8;

		//Reset constant IR light source
		if((config::ir_device == 5) && (ir_delay == 0)) { ir_delay = 10; }
	}

	//Emulate Gyroscope Up tilt release
	//Shift sewing point up
	else if((pad == config::con_key_up) && (!pressed))
	{
		gyro_flags &= ~0x4;
		gyro_flags &= ~0x40;

		if(gyro_flags & 0x80) { gyro_flags |= 0x8; }
		else { gyro_flags &= ~0x8; }

		con_flags &= ~0x4;
		con_flags &= ~0x40;

		if(con_flags & 0x80) { con_flags |= 0x8; }
		else { con_flags &= ~0x8; }
	}

	//Emulate Gyroscope Down tilt press
	//Shift sewing point down
	else if((pad == config::con_key_down) && (pressed))
	{
		gyro_flags |= 0x8;
		gyro_flags |= 0x80;
		gyro_flags &= ~0x4;

		con_flags |= 0x8;
		con_flags |= 0x80;
		con_flags &= ~0x4;
	}

	//Emulate Gyroscope Down tilt release
	//Shift sewing point down
	else if((pad == config::con_key_down) && (!pressed))
	{
		gyro_flags &= ~0x8;
		gyro_flags &= ~0x80;

		if(gyro_flags & 0x40) { gyro_flags |= 0x4; }
		else { gyro_flags &= ~0x4; }

		con_flags &= ~0x8;
		con_flags &= ~0x80;

		if(con_flags & 0x40) { con_flags |= 0x4; }
		else { con_flags &= ~0x4; }
	}

	//Emulate R Trigger press - DMG/GBC on GBA or sewing machine ONLY
	else if((pad == config::gbe_key_r_trigger) && (pressed) && (config::gba_enhance || (config::sio_device == 14)))
	{
		config::request_resize = true;
		config::resize_mode--;
		
		if(config::resize_mode < 0) { config::resize_mode = 0; }
		if(config::sio_device == 14) { config::resize_mode = 0; }
	}

	//Emulate L Trigger press - DMG/GBC on GBA or sewing machine ONLY
	else if((pad == config::gbe_key_l_trigger) && (pressed) && (config::gba_enhance || (config::sio_device == 14)))
	{
		config::request_resize = true;
		config::resize_mode++;

		if(config::resize_mode > 2) { config::resize_mode = 2; }
		if(config::sio_device == 14) { config::resize_mode = 3; }
	}

	//Misc Context Key 1 press
	else if((pad == config::con_key_1) && (pressed)) { con_flags |= 0x100; }
	
	//Misc Context Key 1 release
	else if((pad == config::con_key_1) && (!pressed)) { con_flags &= ~0x100; }

	//Misc Context Key 2 press
	else if((pad == config::con_key_2) && (pressed)) { con_flags |= 0x200; }
	
	//Misc Context Key 2 release
	else if((pad == config::con_key_2) && (!pressed)) { con_flags &= ~0x200; }

	//Alert core of context key changes
	if(con_flags & 0x7FF) { con_flags |= 0x800; }

	//Terminate Turbo Buttons
	if(turbo_button_enabled)
	{
		if(pad == config::gbe_key_a) { turbo_button_end[0] = (pressed) ? false : true; }
		else if(pad == config::gbe_key_b) { turbo_button_end[1] = (pressed) ? false : true; }
		else if(pad == config::gbe_key_start) { turbo_button_end[4] = (pressed) ? false : true; }
		else if(pad == config::gbe_key_select) { turbo_button_end[5] = (pressed) ? false : true; }
		else if(pad == config::gbe_key_left) { turbo_button_end[6] = (pressed) ? false : true; }
		else if(pad == config::gbe_key_right) { turbo_button_end[7] = (pressed) ? false : true; }
		else if(pad == config::gbe_key_up) { turbo_button_end[8] = (pressed) ? false : true; }
		else if(pad == config::gbe_key_down) { turbo_button_end[9] = (pressed) ? false : true; }
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
	//Shift sewing point left
	else if((pad == config::con_joy_left) && (pressed))
	{
		gyro_flags |= 0x1;
		gyro_flags &= ~0x2;

		con_flags |= 0x1;
		con_flags &= ~0x2;
	}

	//Emulate Gyroscope Left tilt release
	//Shift sewing point left
	else if((pad == config::con_joy_left) && (!pressed))
	{
		gyro_flags &= ~0x1;
		con_flags &= ~0x1;
	}

	//Emulate Gyroscope Right tilt press
	//Shift sewing point right
	else if((pad == config::con_joy_right) && (pressed))
	{
		gyro_flags |= 0x2;
		gyro_flags &= ~0x1;

		con_flags |= 0x2;
		con_flags &= ~0x1;
	}

	//Emulate Gyroscope Right tilt release
	//Shift sewing point right
	else if((pad == config::con_joy_right) && (!pressed))
	{
		gyro_flags &= ~0x2;
		con_flags &= ~0x2;
	}

	//Emulate Gyroscope Up tilt press
	//Shift sewing point up
	else if((pad == config::con_joy_up) && (pressed))
	{
		gyro_flags |= 0x4;
		gyro_flags &= ~0x8;

		con_flags |= 0x4;
		con_flags &= ~0x8;

		//Reset constant IR light source
		if((config::ir_device == 5) && (ir_delay == 0)) { ir_delay = 10; }
	}

	//Emulate Gyroscope Up tilt release
	//Shift sewing point up
	else if((pad == config::con_joy_up) && (!pressed))
	{
		gyro_flags &= ~0x4;
		con_flags &= ~0x4;
	}

	//Emulate Gyroscope Down tilt press
	//Shift sewing point down
	else if((pad == config::con_joy_down) && (pressed))
	{
		gyro_flags |= 0x8;
		gyro_flags &= ~0x4;

		con_flags |= 0x8;
		con_flags &= ~0x4;
	}

	//Emulate Gyroscope Down tilt release
	//Shift sewing point down
	else if((pad == config::con_joy_down) && (!pressed))
	{
		gyro_flags &= ~0x8;
		con_flags &= ~0x8;
	}

	//Emulate R Trigger press - DMG/GBC on GBA or sewing machine ONLY
	else if((pad == config::gbe_joy_r_trigger) && (pressed) && (config::gba_enhance || (config::sio_device == 14)))
	{
		config::request_resize = true;
		config::resize_mode--;
		
		if(config::resize_mode < 0) { config::resize_mode = 0; }
		if(config::sio_device == 14) { config::resize_mode = 0; }
	}

	//Emulate L Trigger press - DMG/GBC on GBA or sewing machine ONLY
	else if((pad == config::gbe_joy_l_trigger) && (pressed) && (config::gba_enhance || (config::sio_device == 14)))
	{
		config::request_resize = true;
		config::resize_mode++;

		if(config::resize_mode > 2) { config::resize_mode = 2; }
		if(config::sio_device == 14) { config::resize_mode = 3; }
	}

	//Misc Context Key 1 press
	else if((pad == config::con_joy_1) && (pressed)) { con_flags |= 0x100; }
	
	//Misc Context Key 1 release
	else if((pad == config::con_joy_1) && (!pressed)) { con_flags &= ~0x100; }

	//Misc Context Key 2 press
	else if((pad == config::con_joy_2) && (pressed)) { con_flags |= 0x200; }
	
	//Misc Context Key 2 release
	else if((pad == config::con_joy_2) && (!pressed)) { con_flags &= ~0x200; }

	//Alert core of context key changes
	if(con_flags & 0x7FF) { con_flags |= 0x800; }

	//Terminate Turbo Buttons
	if(turbo_button_enabled)
	{
		if(pad == config::gbe_joy_a) { turbo_button_end[0] = (pressed) ? false : true; }
		else if(pad == config::gbe_joy_b) { turbo_button_end[1] = (pressed) ? false : true; }
		else if(pad == config::gbe_joy_start) { turbo_button_end[4] = (pressed) ? false : true; }
		else if(pad == config::gbe_joy_select) { turbo_button_end[5] = (pressed) ? false : true; }
		else if(pad == config::gbe_joy_left) { turbo_button_end[6] = (pressed) ? false : true; }
		else if(pad == config::gbe_joy_right) { turbo_button_end[7] = (pressed) ? false : true; }
		else if(pad == config::gbe_joy_up) { turbo_button_end[8] = (pressed) ? false : true; }
		else if(pad == config::gbe_joy_down) { turbo_button_end[9] = (pressed) ? false : true; }
	}
}

/****** Process gyroscope sensors - Only used for MBC7 ******/
void DMG_GamePad::process_gyroscope()
{
	//If using real motion controls, abort this function to avoid interference
	if(sensor_init) { return; }

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

/****** Process gyroscope sensors - Only used for MBC7 - Use real controller sensors ******/
void DMG_GamePad::process_gyroscope(float x, float y)
{
	float x_abs = (x < 0) ? -x : x;
	float y_abs = (y < 0) ? -y : y;
	float scaler = config::motion_scaler;
	float deadzone = config::motion_dead_zone;

	//When not tilting, put the sensors in neutral
	if((x_abs < deadzone) && (sensor_x > 2047)) { sensor_x = 2047; }
	else if((x_abs < deadzone) && (sensor_x < 2047)) { sensor_x = 2047; }

	else if(x_abs >= deadzone)
	{
		if(x > 0) { sensor_x = 2047 + (x_abs * scaler); }
		else { sensor_x = 2047 - (x_abs * scaler); }

		//Limit X min-max
    		if(sensor_x < 1897) { sensor_x = 1897; }
    		if(sensor_x > 2197) { sensor_x = 2197; }
	}

	//When not tilting, put the sensors in neutral
	if((y_abs < deadzone) && (sensor_y > 2047)) { sensor_y = 2047; }
	else if((y_abs < deadzone) && (sensor_y < 2047)) { sensor_y = 2047; }

	else if(y_abs >= deadzone)
	{
		if(y > 0) { sensor_y = 2047 + (y_abs * scaler); }
		else { sensor_y = 2047 - (y_abs * scaler); }

		//Limit Y min-max
    		if(sensor_y < 1897) { sensor_y = 1897; }
    		if(sensor_y > 2197) { sensor_y = 2197; }
	}

}

/****** Process turbo button input ******/
void DMG_GamePad::process_turbo_buttons()
{
	for(u32 x = 0; x < 12; x++)
	{
		u8 *p1 = NULL;
		u8 mask = 0;

		//Grab P14 or P15 depending on button
		//Also grab the appropiate mask for said button
		//Cycle through all turbo buttons (0 - 11), and only use those that apply to this core
		switch(x)
		{
			case 0: p1 = &p14; mask = 0x01; break;
			case 1: p1 = &p14; mask = 0x02; break;
			case 4: p1 = &p14; mask = 0x08; break;
			case 5: p1 = &p14; mask = 0x04; break;
			case 6: p1 = &p15; mask = 0x02; break;
			case 7: p1 = &p15; mask = 0x01; break;
			case 8: p1 = &p15; mask = 0x04; break;
			case 9: p1 = &p15; mask = 0x08; break;
		}

		//Continue only if turbo button is used on this core
		if(mask)
		{
			//Turbo Button Start
			if(((*p1 & mask) == 0) && (config::gbe_turbo_button[x]) && (!turbo_button_stat[x]))
			{
				turbo_button_stat[x] = true;
				turbo_button_val[x] = 0;
				*p1 |= mask;
			}

			//Turbo Button Delay
			else if(turbo_button_stat[x])
			{
				turbo_button_val[x]++;

				if(turbo_button_val[x] >= config::gbe_turbo_button[x])
				{
					turbo_button_stat[x] = false;
					*p1 &= ~mask;
				}
			}

			//Turbo Button End
			if(turbo_button_end[x])
			{
				turbo_button_end[x] = false;
				turbo_button_stat[x] = false;
				turbo_button_val[x] = 0;
				*p1 |= mask;
			}
		}
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
