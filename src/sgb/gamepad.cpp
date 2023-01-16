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
	con_flags = 0;
	con_update = false;
	joypad_irq = false;
	joy_init = false;
	up_shadow = down_shadow = left_shadow = right_shadow = false;

	packet.state = 0;
	packet.command = 0;
	packet.length = 0;
	packet.ptr = 0;
	packet.bit_count = 0;
	packet.mult_flag = 0xF0;
	packet.data.resize(0x80, 0);
	packet.lcd_command = false;

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
SGB_GamePad::~SGB_GamePad() { }

/****** Handle Input From Keyboard ******/
void SGB_GamePad::handle_input(SDL_Event &event)
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

	//Process turbo buttons
	if(turbo_button_enabled) { process_turbo_buttons(); }
}

/****** Processes input based on unique pad # for keyboards ******/
void SGB_GamePad::process_keyboard(int pad, bool pressed)
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

	//Toggle SGB border on
	else if((pad == config::gbe_key_r_trigger) && (pressed))
	{
		config::request_resize = true;
		config::resize_mode--;
		
		if(config::resize_mode < 0) { config::resize_mode = 0; }
	}

	//Toggle SGB border off
	else if((pad == config::gbe_key_l_trigger) && (pressed))
	{
		config::request_resize = true;
		config::resize_mode++;

		if(config::resize_mode > 1) { config::resize_mode = 1; }
	}

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
void SGB_GamePad::process_joystick(int pad, bool pressed)
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

	//Toggle SGB border on
	else if((pad == config::gbe_joy_r_trigger) && (pressed))
	{
		config::request_resize = true;
		config::resize_mode--;
		
		if(config::resize_mode < 0) { config::resize_mode = 0; }
	}

	//Toggle SGB border off
	else if((pad == config::gbe_joy_l_trigger) && (pressed))
	{
		config::request_resize = true;
		config::resize_mode++;

		if(config::resize_mode > 1) { config::resize_mode = 1; }
	}

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
void SGB_GamePad::process_gyroscope() { }

/****** Process turbo button input ******/
void SGB_GamePad::process_turbo_buttons()
{
	return;

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
			//Read Joypad ID
			if(packet.mult_flag & 0x3)
			{
				u8 old_id = (packet.mult_flag >> 4);
				u8 old_num = (packet.mult_flag & 0x3);
				u8 new_id = old_id - 1;
				packet.mult_flag &= ~0xF0;

				if(new_id == 0xB) { packet.mult_flag = 0xF0; new_id = 0xF; }
				else { packet.mult_flag = (new_id << 4); }

				packet.mult_flag |= old_num;

				return new_id;
			}

			//Read P14 OR P15
			else { return (p14 | p15); }

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
	u8 sgb_bit = 0x0;
	
	if((value & 0x30) == 0x20) { sgb_bit = 0; }
	else if((value & 0x30) == 0x10) { sgb_bit = 1; }
	else if(packet.state != 0) { return; }

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
					packet.data.clear();
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
			if(packet.bit_count < 3) { packet.length |= (sgb_bit << packet.bit_count); }
			else { packet.command |= (sgb_bit << (packet.bit_count - 3)); }

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
				if(packet.length == 0)
				{
					//MULT_REQ
					if(packet.command == 0x11)
					{
						packet.mult_flag &= ~0x3;
						packet.mult_flag |= (packet.data[0] & 0x3);
					}

					//MASK_EN
					else if(packet.command == 0x17)
					{
						packet.mask_mode = (packet.data[0] & 0x3);
						packet.lcd_command = true;
					}
					
					//Alert LCD to process all other SGB commands
					else { packet.lcd_command = true; }
				}
			}

			break;
	}
}

/****** Grabs misc pad data ******/
u32 SGB_GamePad::get_pad_data(u32 index)
{
	if(index & 0x8000) { return packet.data[index & 0x7F]; }
	
	switch(index & 0xFF)
	{
		case 0x0:
			return packet.lcd_command;

		case 0x1:
			return packet.mask_mode;

		case 0x2:
			return packet.command;

		//PAL_SET - Get System Color 0
		//PALXX - Get Color 0
		case 0x3:
			return (packet.data[1] << 8) | packet.data[0];

		//PAL_SET - Get System Color 1
		//PALXX - Get Color 1
		case 0x4:
			return (packet.data[3] << 8) | packet.data[2];

		//PAL_SET - Get System Color 2
		//PALXX - Get Color 2
		case 0x5:
			return (packet.data[5] << 8) | packet.data[4];

		//PAL_SET - Get System Color 3
		//PALXX - Get Color 3
		case 0x6:
			return (packet.data[7] << 8) | packet.data[6];

		//PALXX - Get Color 4
		case 0x7:
			return (packet.data[9] << 8) | packet.data[8];

		//PALXX - Get Color 5
		case 0x8:
			return (packet.data[11] << 8) | packet.data[10];

		//PALXX - Get Color 6
		case 0x9:
			return (packet.data[13] << 8) | packet.data[12];

		//PAL_SET - Get Attribute File
		case 0xA:
			return packet.data[8];

		//Get Command Length
		case 0xFF:
			return packet.length;

		default:
			return 0;
	}
}

/****** Sets misc pad data ******/
void SGB_GamePad::set_pad_data(u32 index, u32 value)
{
	switch(index)
	{
		case 0x0:
			packet.lcd_command = (value & 0x1) ? true : false;
			break;

		case 0x1:
			packet.mask_mode = (value & 0x3);
			break;
	}
}

