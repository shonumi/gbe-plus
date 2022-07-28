// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gamepad.h
// Date : October 12, 2014
// Description : Game Boy Advance joypad emulation and input handling
//
// Reads and writes to the KEYINPUT and KEYCNT registers
// Handles input from keyboard using SDL events

#include "common/util.h"
#include "gamepad.h"

/****** GamePad Constructor *******/
AGB_GamePad::AGB_GamePad()
{
	pad = 0;
	key_input = 0x3FF;
	jstick = NULL;
	up_shadow = down_shadow = left_shadow = right_shadow = false;
	is_rumbling = false;
	is_gb_player = false;
	disable_input = false;

	con_flags = 0;

	gyro_value = 0x6C0;
	gyro_flags = 0;
	gyro_float = 1728.0;

	solar_value = 0xE8;

	sensor_x = 0x392;
	sensor_y = 0x3A0;

	joypad_irq = false;
	joy_init = false;
	key_cnt = 0;

	gc_sensor = NULL;
	sensor_init = false;
}

/****** Initialize GamePad ******/
void AGB_GamePad::init()
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
	else if((jstick == NULL) && (SDL_NumJoysticks() == 0)) { std::cout<<"JOY::No joysticks detected \n"; }

	joy_init = (jstick == NULL) ? true : false;

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

		//Emulate GB Player detection if rumble is enabled
		//Masks bits 4-7 of KEYINPUT until player gives input
		if(config::sio_device == 8) { is_gb_player = true; }
	}

	if(config::use_motion)
	{
		SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_SENSOR);

		gc_sensor = SDL_GameControllerOpen(config::joy_id);

		SDL_GameControllerSetSensorEnabled(gc_sensor, SDL_SENSOR_ACCEL, SDL_TRUE);
		SDL_GameControllerSetSensorEnabled(gc_sensor, SDL_SENSOR_GYRO, SDL_TRUE);

		if(config::cart_type == AGB_GYRO_SENSOR)
		{
			if(!SDL_GameControllerHasSensor(gc_sensor, SDL_SENSOR_GYRO))
			{
				std::cout<<"JOY::Controller does not have a gyroscope \n";
				gc_sensor = NULL;
			}

			else { sensor_init = true; }
		}

		if(config::cart_type == AGB_TILT_SENSOR)
		{
			if(!SDL_GameControllerHasSensor(gc_sensor, SDL_SENSOR_ACCEL))
			{
				std::cout<<"JOY::Controller does not have an accelerometer \n";
				gc_sensor = NULL;
			}

			else { sensor_init = true; }
		}

		if(sensor_init) { std::cout<<"JOY::Controller sensor detected\n"; }
	}
}

/****** GamePad Destructor ******/
AGB_GamePad::~AGB_GamePad() { } 

/****** Handle input from keyboard or joystick for processing ******/
void AGB_GamePad::handle_input(SDL_Event &event)
{
	if(disable_input) { return; }

	u16 last_input = key_input;
	u16 key_mask = (key_cnt & 0x3FF);

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

			if(config::cart_type == AGB_GYRO_SENSOR)
			{
				SDL_GameControllerGetSensorData(gc_sensor, SDL_SENSOR_GYRO, motion_data, 3);
				process_gyroscope(motion_data[1], 0);
	
				//std::cout<<"00 --> " << motion_data[0] << "\n";
				//std::cout<<"01 --> " << motion_data[1] << "\n\n";
				//std::cout<<"02 --> " << motion_data[2] << "\n\n";
			}

			else
			{
				SDL_GameControllerGetSensorData(gc_sensor, SDL_SENSOR_ACCEL, motion_data, 3);
				process_gyroscope(motion_data[0], motion_data[2]);
			}
		}
	}

	//Update Joypad Interrupt Flag
	if((last_input != key_input) && (key_input != 0x3FF))
	{
		//Logical OR mode
		if(((key_cnt & 0x8000) == 0) && (key_input & key_mask))  { joypad_irq = true; }

		//Logical AND mode
		else if ((key_cnt & 0x8000) && ((key_input & key_mask) == key_mask)) { joypad_irq = true; }
	}

	else { joypad_irq = false; }
}

/****** Processes input based on unique pad # for keyboards ******/
void AGB_GamePad::process_keyboard(int pad, bool pressed)
{
	//Emulate A button press
	if((pad == config::gbe_key_a) && (pressed)) { key_input &= ~0x1; }

	//Emulate A button release
	else if((pad == config::gbe_key_a) && (!pressed)) { key_input |= 0x1; }

	//Emulate B button press
	else if((pad == config::gbe_key_b) && (pressed)) { key_input &= ~0x2; }

	//Emulate B button release
	else if((pad == config::gbe_key_b) && (!pressed)) { key_input |= 0x2; }

	//Emulate Select button press
	else if((pad == config::gbe_key_select) && (pressed)) { key_input &= ~0x4; }

	//Emulate Select button release
	else if((pad == config::gbe_key_select) && (!pressed)) { key_input |= 0x4; }

	//Emulate Start button press
	else if((pad == config::gbe_key_start) && (pressed)) { key_input &= ~0x8; }

	//Emulate Start button release
	else if((pad == config::gbe_key_start) && (!pressed)) { key_input |= 0x8; }

	//Emulate Right DPad press
	else if((pad == config::gbe_key_right) && (pressed)) { key_input &= ~0x10; key_input |= 0x20; right_shadow = true; }

	//Emulate Right DPad release
	else if((pad == config::gbe_key_right) && (!pressed)) 
	{
		right_shadow = false; 
		key_input |= 0x10;

		if(left_shadow) { key_input &= ~0x20; }
		else { key_input |= 0x20; }
	}

	//Emulate Left DPad press
	else if((pad == config::gbe_key_left) && (pressed)) { key_input &= ~0x20; key_input |= 0x10; left_shadow = true; }

	//Emulate Left DPad release
	else if((pad == config::gbe_key_left) && (!pressed)) 
	{
		left_shadow = false;
		key_input |= 0x20;
		
		if(right_shadow) { key_input &= ~0x10; }
		else { key_input |= 0x10; }

		gyro_value = 0x6C0;
	}

	//Emulate Up DPad press
	else if((pad == config::gbe_key_up) && (pressed)) { key_input &= ~0x40; key_input |= 0x80; up_shadow = true; }

	//Emulate Up DPad release
	else if((pad == config::gbe_key_up) && (!pressed)) 
	{
		up_shadow = false; 
		key_input |= 0x40;
		
		if(down_shadow) { key_input &= ~0x80; }
		else { key_input |= 0x80; }
	}

	//Emulate Down DPad press
	else if((pad == config::gbe_key_down) && (pressed)) { key_input &= ~0x80; key_input |= 0x40; down_shadow = true; }

	//Emulate Down DPad release
	else if((pad == config::gbe_key_down) && (!pressed)) 
	{
		down_shadow = false;
		key_input |= 0x80;

		if(up_shadow) { key_input &= ~0x40; }
		else { key_input |= 0x40; } 
	}

	//Emulate R Trigger press
	else if((pad == config::gbe_key_r_trigger) && (pressed)) { key_input &= ~0x100; }

	//Emulate R Trigger release
	else if((pad == config::gbe_key_r_trigger) && (!pressed)) { key_input |= 0x100; }

	//Emulate L Trigger press
	else if((pad == config::gbe_key_l_trigger) && (pressed)) { key_input &= ~0x200; }

	//Emulate L Trigger release
	else if((pad == config::gbe_key_l_trigger) && (!pressed)) { key_input |= 0x200; }

	//Context Left press
	else if((pad == config::con_key_left) && (pressed))
	{
		//Emulate Gyroscope Left tilt press
		if((config::cart_type == AGB_GYRO_SENSOR) || (config::cart_type == AGB_TILT_SENSOR))
		{
			gyro_flags |= 0x1;
			gyro_flags |= 0x10;

			gyro_flags &= ~0x2;

			gyro_value = 0x6C0;
		}

		//Emulate Battle Chip insertion
		else if((config::sio_device >= 10) && (config::sio_device <= 12)) { config::battle_chip_id = config::chip_list[0]; }

		con_flags |= 0x1;
		con_flags |= 0x10;
		con_flags &= ~0x2;
	}

	//Context Left release
	else if((pad == config::con_key_left) && (!pressed))
	{
		//Emulate Gyroscope Left tilt release
		if((config::cart_type == AGB_GYRO_SENSOR) || (config::cart_type == AGB_TILT_SENSOR))
		{
			gyro_flags &= ~0x1;
			gyro_flags &= ~0x10;

			if(gyro_flags & 0x20) { gyro_flags |= 0x2; }
			else { gyro_flags &= ~0x2; }
		}

		//Emulate Battle Chip extraction
		else if((config::sio_device >= 10) && (config::sio_device <= 12)) { config::battle_chip_id = 0; }

		con_flags &= ~0x1;
		con_flags &= ~0x10;

		if(con_flags & 0x20) { con_flags |= 0x2; }
		else { con_flags &= ~0x2; }
	}

	//Context Right press
	else if((pad == config::con_key_right) && (pressed))
	{
		//Emulate Gyroscope Right tilt press
		if((config::cart_type == AGB_GYRO_SENSOR) || (config::cart_type == AGB_TILT_SENSOR))
		{
	
			gyro_flags |= 0x2;
			gyro_flags |= 0x20;

			gyro_flags &= ~0x1;

			gyro_value = 0x6C0;
		}

		//Emulate Battle Chip insertion
		else if((config::sio_device >= 10) && (config::sio_device <= 12)) { config::battle_chip_id = config::chip_list[1]; }

		con_flags |= 0x2;
		con_flags |= 0x20;
		con_flags &= ~0x1;
	}

	//Context Right release
	else if((pad == config::con_key_right) && (!pressed))
	{
		//Emulate Gyroscope Right tilt release
		if((config::cart_type == AGB_GYRO_SENSOR) || (config::cart_type == AGB_TILT_SENSOR))
		{
			gyro_flags &= ~0x2;
			gyro_flags &= ~0x20;

			if(gyro_flags & 0x10) { gyro_flags |= 0x1; }
			else { gyro_flags &= ~0x1; }
		}

		//Emulate Battle Chip extraction
		else if((config::sio_device >= 10) && (config::sio_device <= 12)) { config::battle_chip_id = 0; }

		con_flags &= ~0x2;
		con_flags &= ~0x20;

		if(con_flags & 0x10) { con_flags |= 0x1; }
		else { con_flags &= ~0x1; }
	}

	//Context Up press
	else if((pad == config::con_key_up) && (pressed))
	{
		//Decrease solar sensor value (increases in-game solar power)
		if(config::cart_type == AGB_SOLAR_SENSOR)
		{
			solar_value -= 0x8;
			if(solar_value < 0x50) { solar_value = 0x50; }

			float percent = (1.0 - ((solar_value - 0x50) / 152.0)) * 100.0; 
			config::osd_message = "SOLAR SENSOR " + util::to_str(u8(percent));
			config::osd_count = 180;
		}

		//Emulate upwards tilt press
		else if(config::cart_type == AGB_TILT_SENSOR)
		{
			gyro_flags |= 0x4;
			gyro_flags |= 0x40;

			gyro_flags &= ~0x8;
		}

		//Emulate Battle Chip insertion
		else if((config::sio_device >= 10) && (config::sio_device <= 12)) { config::battle_chip_id = config::chip_list[2]; }

		con_flags |= 0x4;
		con_flags |= 0x40;
		con_flags &= ~0x8;
	}

	//Context Up release
	else if((pad == config::con_key_up) && (!pressed))
	{
		//Emulate upwards tilt release
		if(config::cart_type == AGB_TILT_SENSOR)
		{
			gyro_flags &= ~0x4;
			gyro_flags &= ~0x40;

			if(gyro_flags & 0x80) { gyro_flags |= 0x8; }
			else { gyro_flags &= ~0x8; }
		}

		//Emulate Battle Chip extraction
		else if((config::sio_device >= 10) && (config::sio_device <= 12)) { config::battle_chip_id = 0; }

		con_flags &= ~0x4;
		con_flags &= ~0x40;

		if(con_flags & 0x80) { con_flags |= 0x8; }
		else { con_flags &= ~0x8; }
	}

	//Context Down press
	else if((pad == config::con_key_down) && (pressed))
	{
		//Increase solar sensor value (decreases in-game solar power)
		if(config::cart_type == AGB_SOLAR_SENSOR)
		{
			solar_value += 0x8;
			if(solar_value > 0xE8) { solar_value = 0xE8; }

			float percent = (1.0 - ((solar_value - 0x50) / 152.0)) * 100.0; 
			config::osd_message = "SOLAR SENSOR " + util::to_str(u8(percent));
			config::osd_count = 180;
		}

		//Emulate downwards tilt press
		else if(config::cart_type == AGB_TILT_SENSOR)
		{
			gyro_flags |= 0x8;
			gyro_flags |= 0x80;

			gyro_flags &= ~0x4;
		}

		//Emulate Battle Chip insertion
		else if((config::sio_device >= 10) && (config::sio_device <= 12)) { config::battle_chip_id = config::chip_list[3]; }

		con_flags |= 0x8;
		con_flags |= 0x80;
		con_flags &= ~0x4;
	}

	//Context Down release
	else if((pad == config::con_key_down) && (!pressed))
	{
		//Emulate downwards tilt release
		if(config::cart_type == AGB_TILT_SENSOR)
		{
			gyro_flags &= ~0x8;
			gyro_flags &= ~0x80;

			if(gyro_flags & 0x40) { gyro_flags |= 0x4; }
			else { gyro_flags &= ~0x4; }
		}

		//Emulate Battle Chip extraction
		else if((config::sio_device >= 10) && (config::sio_device <= 12)) { config::battle_chip_id = 0; }

		con_flags &= ~0x8;
		con_flags &= ~0x80;

		if(con_flags & 0x40) { con_flags |= 0x4; }
		else { con_flags &= ~0x4; }
	}

	//Misc Context Key 1 press
	else if((pad == config::con_key_1) && (pressed)) { con_flags |= 0x100; }
	
	//Misc Context Key 1 release
	else if((pad == config::con_key_1) && (!pressed)) { con_flags &= ~0x100; }

	//Misc Context Key 2 press
	else if((pad == config::con_key_2) && (pressed)) { con_flags |= 0x200; }
	
	//Misc Context Key 2 release
	else if((pad == config::con_key_2) && (!pressed)) { con_flags &= ~0x200; }

	is_gb_player = false;
}

/****** Processes input based on unique pad # for joysticks ******/
void AGB_GamePad::process_joystick(int pad, bool pressed)
{
	//Emulate A button press
	if((pad == config::gbe_joy_a) && (pressed)) { key_input &= ~0x1; }

	//Emulate A button release
	else if((pad == config::gbe_joy_a) && (!pressed)) { key_input |= 0x1; }

	//Emulate B button press
	else if((pad == config::gbe_joy_b) && (pressed)) { key_input &= ~0x2; }

	//Emulate B button release
	else if((pad == config::gbe_joy_b) && (!pressed)) { key_input |= 0x2; }

	//Emulate Select button press
	else if((pad == config::gbe_joy_select) && (pressed)) { key_input &= ~0x4; }

	//Emulate Select button release
	else if((pad == config::gbe_joy_select) && (!pressed)) { key_input |= 0x4; }

	//Emulate Start button press
	else if((pad == config::gbe_joy_start) && (pressed)) { key_input &= ~0x8; }

	//Emulate Start button release
	else if((pad == config::gbe_joy_start) && (!pressed)) { key_input |= 0x8; }

	//Emulate Right DPad press
	else if((pad == config::gbe_joy_right) && (pressed)) { key_input &= ~0x10; key_input |= 0x20; }

	//Emulate Right DPad release
	else if((pad == config::gbe_joy_right) && (!pressed)) { key_input |= 0x10; key_input |= 0x20;}

	//Emulate Left DPad press
	else if((pad == config::gbe_joy_left) && (pressed)) { key_input &= ~0x20; key_input |= 0x10; }

	//Emulate Left DPad release
	else if((pad == config::gbe_joy_left) && (!pressed)) { key_input |= 0x20; key_input |= 0x10; }

	//Emulate Up DPad press
	else if((pad == config::gbe_joy_up) && (pressed)) { key_input &= ~0x40; key_input |= 0x80; }

	//Emulate Up DPad release
	else if((pad == config::gbe_joy_up) && (!pressed)) { key_input |= 0x40; key_input |= 0x80;}

	//Emulate Down DPad press
	else if((pad == config::gbe_joy_down) && (pressed)) { key_input &= ~0x80; key_input |= 0x40;}

	//Emulate Down DPad release
	else if((pad == config::gbe_joy_down) && (!pressed)) { key_input |= 0x80; key_input |= 0x40; }

	//Emulate R Trigger press
	else if((pad == config::gbe_joy_r_trigger) && (pressed)) { key_input &= ~0x100; }

	//Emulate R Trigger release
	else if((pad == config::gbe_joy_r_trigger) && (!pressed)) { key_input |= 0x100; }

	//Emulate L Trigger press
	else if((pad == config::gbe_joy_l_trigger) && (pressed)) { key_input &= ~0x200; }

	//Emulate L Trigger release
	else if((pad == config::gbe_joy_l_trigger) && (!pressed)) { key_input |= 0x200; }

	//Context Left press
	else if((pad == config::con_joy_left) && (pressed))
	{
		//Emulate Gyroscope Left tilt press
		if((config::cart_type == AGB_GYRO_SENSOR) || (config::cart_type == AGB_TILT_SENSOR))
		{
			gyro_flags |= 0x1;
			gyro_flags |= 0x10;

			gyro_flags &= ~0x2;

			gyro_value = 0x6C0;
		}

		//Emulate Battle Chip insertion
		else if((config::sio_device >= 10) && (config::sio_device <= 12)) { config::battle_chip_id = config::chip_list[0]; }

		con_flags |= 0x1;
		con_flags &= ~0x2;
	}

	//Context Left release
	else if((pad == config::con_joy_left) && (!pressed))
	{
		//Emulate Gyroscope Left tilt release
		if((config::cart_type == AGB_GYRO_SENSOR) || (config::cart_type == AGB_TILT_SENSOR))
		{
			gyro_flags &= ~0x1;
			gyro_flags &= ~0x10;

			if(gyro_flags & 0x20) { gyro_flags |= 0x2; }
			else { gyro_flags &= ~0x2; }
		}

		//Emulate Battle Chip extraction
		else if((config::sio_device >= 10) && (config::sio_device <= 12)) { config::battle_chip_id = 0; }

		con_flags &= ~0x1;
	}

	//Context Right press
	else if((pad == config::con_joy_right) && (pressed))
	{
		//Emulate Gyroscope Right tilt press
		if((config::cart_type == AGB_GYRO_SENSOR) || (config::cart_type == AGB_TILT_SENSOR))
		{
			gyro_flags |= 0x2;
			gyro_flags |= 0x20;

			gyro_flags &= ~0x1;

			gyro_value = 0x6C0;
		}

		//Emulate Battle Chip insertion
		else if((config::sio_device >= 10) && (config::sio_device <= 12)) { config::battle_chip_id = config::chip_list[1]; }

		con_flags |= 0x2;
		con_flags &= ~0x1;
	}

	//Context Right release
	else if((pad == config::con_joy_right) && (!pressed))
	{
		//Emulate Gyroscope Right tilt release
		if((config::cart_type == AGB_GYRO_SENSOR) || (config::cart_type == AGB_TILT_SENSOR))
		{
			gyro_flags &= ~0x2;
			gyro_flags &= ~0x20;

			if(gyro_flags & 0x10) { gyro_flags |= 0x1; }
			else { gyro_flags &= ~0x1; }
		}

		//Emulate Battle Chip extraction
		else if((config::sio_device >= 10) && (config::sio_device <= 12)) { config::battle_chip_id = 0; }

		con_flags &= ~0x2;
	}

	//Context Up press
	else if((pad == config::con_joy_up) && (pressed))
	{
		//Decrease solar sensor value (increases in-game solar power)
		if(config::cart_type == AGB_SOLAR_SENSOR)
		{
			solar_value -= 0x8;
			if(solar_value < 0x50) { solar_value = 0x50; }

			float percent = (1.0 - ((solar_value - 0x50) / 152.0)) * 100.0; 
			config::osd_message = "SOLAR SENSOR " + util::to_str(u8(percent));
			config::osd_count = 180;
		}

		//Emulate upwards tilt press
		else if(config::cart_type == AGB_TILT_SENSOR)
		{
			gyro_flags |= 0x4;
			gyro_flags |= 0x40;

			gyro_flags &= ~0x8;
		}

		//Emulate Battle Chip insertion
		else if((config::sio_device >= 10) && (config::sio_device <= 12)) { config::battle_chip_id = config::chip_list[2]; }

		con_flags |= 0x4;
		con_flags &= ~0x8;
	}

	//Context Up release
	else if((pad == config::con_joy_up) && (!pressed))
	{
		//Emulate upwards tilt release
		if(config::cart_type == AGB_TILT_SENSOR)
		{
			gyro_flags &= ~0x4;
			gyro_flags &= ~0x40;

			if(gyro_flags & 0x80) { gyro_flags |= 0x8; }
			else { gyro_flags &= ~0x8; }
		}

		//Emulate Battle Chip extraction
		else if((config::sio_device >= 10) && (config::sio_device <= 12)) { config::battle_chip_id = 0; }

		con_flags &= ~0x4;
	}

	//Context Down press
	else if((pad == config::con_joy_down) && (pressed))
	{
		//Increase solar sensor value (decreases in-game solar power)
		if(config::cart_type == AGB_SOLAR_SENSOR)
		{
			solar_value += 0x8;
			if(solar_value > 0xE8) { solar_value = 0xE8; }

			float percent = (1.0 - ((solar_value - 0x50) / 152.0)) * 100.0; 
			config::osd_message = "SOLAR SENSOR " + util::to_str(u8(percent));
			config::osd_count = 180;
		}

		//Emulate downwards tilt press
		else if(config::cart_type == AGB_TILT_SENSOR)
		{
			gyro_flags |= 0x8;
			gyro_flags |= 0x80;

			gyro_flags &= ~0x4;
		}

		//Emulate Battle Chip insertion
		else if((config::sio_device >= 10) && (config::sio_device <= 12)) { config::battle_chip_id = config::chip_list[3]; }

		con_flags |= 0x8;
		con_flags &= ~0x4;
	}

	//Context Down release
	else if((pad == config::con_joy_down) && (!pressed))
	{
		//Emulate downwards tilt release
		if(config::cart_type == AGB_TILT_SENSOR)
		{
			gyro_flags &= ~0x8;
			gyro_flags &= ~0x80;

			if(gyro_flags & 0x40) { gyro_flags |= 0x4; }
			else { gyro_flags &= ~0x4; }
		}

		//Emulate Battle Chip extraction
		else if((config::sio_device >= 10) && (config::sio_device <= 12)) { config::battle_chip_id = 0; }

		con_flags &= ~0x8;
	}

	//Misc Context Key 1 press
	else if((pad == config::con_joy_1) && (pressed)) { con_flags |= 0x100; }
	
	//Misc Context Key 1 release
	else if((pad == config::con_joy_1) && (!pressed)) { con_flags &= ~0x100; }

	//Misc Context Key 2 press
	else if((pad == config::con_joy_2) && (pressed)) { con_flags |= 0x200; }
	
	//Misc Context Key 2 release
	else if((pad == config::con_joy_2) && (!pressed)) { con_flags &= ~0x200; }

	is_gb_player = false;
}

/****** Clears any existing input - Primarily used for the SoftReset SWI ******/
void AGB_GamePad::clear_input()
{
	key_input = 0x3FF;
	up_shadow = down_shadow = left_shadow = right_shadow = false;
}

/****** Start haptic force-feedback on joypad ******/
void AGB_GamePad::start_rumble()
{
	if((jstick != NULL) && (rumble != NULL) && (is_rumbling == false))
	{
		SDL_HapticRumblePlay(rumble, 1, -1);
		is_rumbling = true;
	}
}

/****** Stop haptic force-feedback on joypad ******/
void AGB_GamePad::stop_rumble()
{
	if((jstick != NULL) && (rumble != NULL) && (is_rumbling == true))
	{
		SDL_HapticRumbleStop(rumble);
       		is_rumbling = false;
	}
}

/******* Processes gryoscope sensors ******/
void AGB_GamePad::process_gyroscope()
{
	//If using real motion controls, abort this function to avoid interference
	if(sensor_init) { return; }

	//Gyro sensor
	if(config::cart_type == AGB_GYRO_SENSOR)
	{
		//Increase gyroscope value for clockwise motion
		if(gyro_flags & 0x2)
		{
			gyro_value += 32;
			if(gyro_value > 0x9E3) { gyro_value = 0x9E3; }
		}

		//Decrease gyroscope value for counter-clockwise motion
		else if(gyro_flags & 0x1)
		{
			gyro_value -= 32;
			if(gyro_value < 0x354) { gyro_value = 0x354; }
		}

		//When neither left or right is pressed, put the sensor in neutral
		else if(gyro_value > 0x6C0) 
		{
    			gyro_value -= 64;
    			if(gyro_value < 0x6C0) { gyro_value = 0x6C0; } 
		}
	
		else if(gyro_value < 0x6C0)
		{
    			gyro_value += 64;
    			if(gyro_value > 0x6C0) { gyro_value = 0x6C0; }
  		}
	}

	//Tilt sensor
	else if(config::cart_type == AGB_TILT_SENSOR)
	{
		//Decrease X value for left motion
		if(gyro_flags & 0x1) 
		{
			sensor_x -= 16;
			if(sensor_x < 0x2AF) { sensor_x = 0x2AF; }
		}

		//Increase X value for left motion
		else if(gyro_flags & 0x2)
		{
			sensor_x += 16;
			if(sensor_x > 0x477) { sensor_x = 0x477; }
		}

		//When neither left or right is pressed, put sensor in neutral
		else if(sensor_y > 0x392)
		{
			sensor_x -= 32;
			if(sensor_x < 0x392) { sensor_x = 0x392; }
		}

		else if(sensor_x < 0x392)
		{
			sensor_x += 32;
			if(sensor_x > 0x392) { sensor_x = 0x392; }

		}

		//Decrease Y value for down motion
		if(gyro_flags & 0x8) 
		{
			sensor_y -= 16;
			if(sensor_y < 0x2C3) { sensor_y = 0x2C3; }
		}

		//Increase Y value for up motion
		else if(gyro_flags & 0x4)
		{
			sensor_y += 16;
			if(sensor_y > 0x480) { sensor_y = 0x480; }
		}

		//When neither up or down is pressed, put sensor in neutral
		else if(sensor_y > 0x3A0)
		{
			sensor_y -= 32;
			if(sensor_y < 0x3A0) { sensor_y = 0x3A0; }
		}

		else if(sensor_y < 0x3A0)
		{
			sensor_y += 32;
			if(sensor_y > 0x3A0) { sensor_y = 0x3A0; }

		}
	}
}

/****** Process gyroscope sensors - Use real controller sensors ******/
void AGB_GamePad::process_gyroscope(float x, float y)
{
	float x_abs = (x < 0) ? -x : x;
	float y_abs = (y < 0) ? -y : y;
	float scaler = config::motion_scaler;
	float deadzone = config::motion_dead_zone;

	//Tilt sensor
	if(config::cart_type == AGB_TILT_SENSOR)
	{
		//When not tilting, put the sensors in neutral
		if((x_abs < deadzone) && (sensor_x > 0x392)) { sensor_x = 0x392; }
		else if((x_abs < deadzone) && (sensor_x < 0x392)) { sensor_x = 0x392; }

		else if(x_abs >= deadzone)
		{
			if(x > 0) { sensor_x = 0x392 + (x_abs * scaler); }
			else { sensor_x = 0x392 - (x_abs * scaler); }

			//Limit X min-max
    			if(sensor_x < 0x2AF) { sensor_x = 0x2AF; }
    			if(sensor_x > 0x477) { sensor_x = 0x477; }
		}

		//When not tilting, put the sensors in neutral
		if((y_abs < deadzone) && (sensor_y > 0x3A0)) { sensor_y = 0x3A0; }
		else if((y_abs < deadzone) && (sensor_y < 0x3A0)) { sensor_y = 0x3A0; }

		else if(y_abs >= deadzone)
		{
			if(y > 0) { sensor_y = 0x3A0 + (y_abs * scaler); }
			else { sensor_y = 0x3A0 - (y_abs * scaler); }

			//Limit Y min-max
    			if(sensor_y < 0x2C3) { sensor_y = 0x2C3; }
    			if(sensor_y > 0x480) { sensor_y = 0x480; }
		}
	}

	//Gyroscope
	else if(config::cart_type == AGB_GYRO_SENSOR)
	{
		if(x_abs >= 0.1)
		{
			if(x < 0) { gyro_value = 0x6C0 + (x_abs * scaler); }
			else { gyro_value = 0x6C0 - (x_abs * scaler); }

    			if(gyro_value < 0x354) { gyro_value = 0x354; }
    			if(gyro_value > 0x9E3) { gyro_value = 0x9E3; }
		}

		else { gyro_value = 0x6C0; }
	}
}
