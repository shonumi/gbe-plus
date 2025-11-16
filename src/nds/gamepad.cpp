// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gamepad.h
// Date : November 13, 2016
// Description : NDS joypad emulation and input handling
//
// Reads and writes to the KEYINPUT, EXTKEYIN and KEYCNT registers
// Handles inputfrom controllers, keyboard, and mouse using SDL events

#include "gamepad.h"

/****** GamePad Constructor *******/
NTR_GamePad::NTR_GamePad()
{
	pad = 0;
	key_input = 0x3FF;
	ext_key_input = 0x7F;
	jstick = NULL;
	mouse_x = 0;
	mouse_y = 0xFFF;
	up_shadow = down_shadow = left_shadow = right_shadow = false;
	touch_hold = false;
	touch_by_mouse = false;
	is_rumbling = false;
	con_flags = 0;

	vc_x = 128;
	vc_y = 96;
	vc_counter = 0;
	vc_delta_x = 0;
	vc_delta_y = 0;
	vc_pause = 0;

	nds7_input_irq = NULL;
	nds9_input_irq = NULL;

	joypad_irq = false;
	joy_init = false;
	key_cnt = 0;

	sdl_fs_ratio = 1;
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
	config::joy_sdl_id = SDL_JoystickInstanceID(jstick);

	if((jstick == NULL) && (SDL_NumJoysticks() >= 1)) { std::cout<<"JOY::Could not initialize joystick \n"; }
	else if((jstick == NULL) && (SDL_NumJoysticks() == 0)) { std::cout<<"JOY::No joysticks detected \n"; }

	joy_init = (jstick != NULL) ? true : false;

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

/****** GamePad Destructor ******/
NTR_GamePad::~NTR_GamePad() { } 

/****** Close SDL Joystick - Only used for hot plugging ******/
void NTR_GamePad::close_joystick()
{
	joy_init = false;

	if(jstick != NULL) { SDL_JoystickClose(jstick); }
	if(rumble != NULL) { SDL_HapticClose(rumble); }
}

/****** Handle input from keyboard or joystick for processing ******/
void NTR_GamePad::handle_input(SDL_Event &event)
{
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

	//Mouse button press
	else if(event.type == SDL_MOUSEBUTTONDOWN)
	{
		pad = 400;

		//Calculate coordinates in SDL fullscreen
		if((config::sdl_render) && (config::flags & SDL_WINDOW_FULLSCREEN_DESKTOP) && (!config::use_opengl))
		{
			mouse_x = event.button.x - ((config::win_width - (config::sys_width * sdl_fs_ratio)) >> 1);
			mouse_y = event.button.y - ((config::win_height - (config::sys_height * sdl_fs_ratio)) >> 1);

			mouse_x /= sdl_fs_ratio;
			mouse_y /= sdl_fs_ratio;

			//Top screen cannot be touched
			if((mouse_y < 192) && (config::lcd_config == 0)) { return; }
			else if((mouse_y > 192) && (config::lcd_config == 1)) { return; }
			else if((mouse_x < 256) && (config::lcd_config == 2)) { return; }
			else if((mouse_x > 256) && (config::lcd_config == 3)) { return; }
		}

		//Calculate coordinates normally
		else
		{
			//Top screen cannot be touched
			if(((event.button.y / config::scaling_factor) < 192) && (config::lcd_config == 0)) { return; }
			else if(((event.button.y / config::scaling_factor) > 192) && (config::lcd_config == 1)) { return; }
			else if(((event.button.x / config::scaling_factor) < 256) && (config::lcd_config == 2)) { return; }
			else if(((event.button.x / config::scaling_factor) > 256) && (config::lcd_config == 3)) { return; }

			mouse_x = (event.button.x / config::scaling_factor);
			mouse_y = (event.button.y / config::scaling_factor);
		}

		//Adjust mouse X and Y coordinates to NDS coordinate
		switch(config::lcd_config)
		{
			//Normal vertical mode
			case 0x0:
				mouse_y -= 192;
				break;

			//Normal horizontal mode
			case 0x2:
				mouse_x -= 256;
				break;
		}

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

		bool is_top = false;

		//Calculate coordinates in SDL fullscreen
		if((config::sdl_render) && (config::flags & SDL_WINDOW_FULLSCREEN_DESKTOP) && (!config::use_opengl))
		{
			mouse_x = event.button.x - ((config::win_width - (config::sys_width * sdl_fs_ratio)) >> 1);
			mouse_y = event.button.y - ((config::win_height - (config::sys_height * sdl_fs_ratio)) >> 1);

			mouse_x /= sdl_fs_ratio;
			mouse_y /= sdl_fs_ratio;

			//Top screen cannot be touched
			if((mouse_y < 192) && (config::lcd_config == 0)) { return; }
			else if((mouse_y > 192) && (config::lcd_config == 1)) { return; }
			else if((mouse_x < 256) && (config::lcd_config == 2)) { return; }
			else if((mouse_x > 256) && (config::lcd_config == 3)) { return; }
		}

		//Calculate coordinates normally
		else
		{
			//Top screen cannot be touched
			if(((event.button.y / config::scaling_factor) < 192) && (config::lcd_config == 0)) { is_top = true; }
			else if(((event.button.y / config::scaling_factor) > 192) && (config::lcd_config == 1)) { is_top = true; }
			else if(((event.button.x / config::scaling_factor) < 256) && (config::lcd_config == 2)) { is_top = true; }
			else if(((event.button.x / config::scaling_factor) > 256) && (config::lcd_config == 3)) { is_top = true; }

			if(is_top)
			{
				mouse_x = 0;
				mouse_y = 0xFFF;
				return;
			}

			mouse_x = (event.button.x / config::scaling_factor);
			mouse_y = (event.button.y / config::scaling_factor);
		}

		//Adjust mouse X and Y coordinates to NDS coordinate
		switch(config::lcd_config)
		{
			//Normal vertical mode
			case 0x0:
				mouse_y -= 192;
				break;

			//Normal horizontal mode
			case 0x2:
				mouse_x -= 256;
				break;
		}

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

	//Mouse motion
	else if(event.type == SDL_MOUSEMOTION)
	{
		//Only process mouse motion if the emulated stylus is down
		if(!touch_by_mouse) { return; }

		//Calculate coordinates in SDL fullscreen
		if((config::sdl_render) && (config::flags & SDL_WINDOW_FULLSCREEN_DESKTOP) && (!config::use_opengl))
		{
			mouse_x = event.button.x - ((config::win_width - (config::sys_width * sdl_fs_ratio)) >> 1);
			mouse_y = event.button.y - ((config::win_height - (config::sys_height * sdl_fs_ratio)) >> 1);

			mouse_x /= sdl_fs_ratio;
			mouse_y /= sdl_fs_ratio;

			//Top screen cannot be touched
			if((mouse_y < 192) && (config::lcd_config == 0)) { return; }
			else if((mouse_y > 192) && (config::lcd_config == 1)) { return; }
			else if((mouse_x < 256) && (config::lcd_config == 2)) { return; }
			else if((mouse_x > 256) && (config::lcd_config == 3)) { return; }
		}

		//Calculate coordinates normally
		else
		{
			//Top screen cannot be touched
			if(((event.button.y / config::scaling_factor) < 192) && (config::lcd_config == 0)) { return; }
			else if(((event.button.y / config::scaling_factor) > 192) && (config::lcd_config == 1)) { return; }
			else if(((event.button.x / config::scaling_factor) < 256) && (config::lcd_config == 2)) { return; }
			else if(((event.button.x / config::scaling_factor) > 256) && (config::lcd_config == 3)) { return; }

			mouse_x = (event.button.x / config::scaling_factor);
			mouse_y = (event.button.y / config::scaling_factor);
		}

		//Adjust mouse X and Y coordinates to NDS coordinate
		switch(config::lcd_config)
		{
			//Normal vertical mode
			case 0x0:
				mouse_y -= 192;
				break;

			//Normal horizontal mode
			case 0x2:
				mouse_x -= 256;
				break;
		}
	}

	//Update Joypad Interrupt Flag
	if((last_input != key_input) && (key_input != 0x3FF))
	{
		//Logical OR mode
		if(((key_cnt & 0x8000) == 0) && (~key_input & key_mask))  { joypad_irq = true; }

		//Logical AND mode
		else if ((key_cnt & 0x8000) && ((~key_input & key_mask) == key_mask)) { joypad_irq = true; }
	}

	else { joypad_irq = false; }

	//Process turbo buttons
	if(turbo_button_enabled) { process_turbo_buttons(); }
}	

/****** Processes input based on unique pad # for keyboards ******/
void NTR_GamePad::process_keyboard(int pad, bool pressed)
{
	//Emulate A button press
	if((pad == config::gbe_key_a) && (pressed)) { key_input &= ~0x1; }

	//Emulate A button release
	else if((pad == config::gbe_key_a) && (!pressed)) { key_input |= 0x1; }

	//Emulate B button press
	else if((pad == config::gbe_key_b) && (pressed)) { key_input &= ~0x2; }

	//Emulate B button release
	else if((pad == config::gbe_key_b) && (!pressed)) { key_input |= 0x2; }

	//Emulate X button press
	else if((pad == config::gbe_key_x) && (pressed)) { ext_key_input &= ~0x1; }

	//Emulate X button release
	else if((pad == config::gbe_key_x) && (!pressed)) { ext_key_input |= 0x1; }

	//Emulate Y button press
	else if((pad == config::gbe_key_y) && (pressed)) { ext_key_input &= ~0x2; }

	//Emulate Y button release
	else if((pad == config::gbe_key_y) && (!pressed)) { ext_key_input |= 0x2; }

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

	//Misc Context Key 1 press
	else if((pad == config::con_key_1) && (pressed))
	{
		//Virtual Cursor - Emulate Stylus Down
		if(config::vc_enable)
		{
			ext_key_input &= ~0x40;
			mouse_x = vc_x;
			mouse_y = vc_y;
		}

		con_flags |= 0x100;
	}
	
	//Misc Context Key 1 release
	else if((pad == config::con_key_1) && (!pressed))
	{
		//Virtual Cursor - Emulate Stylus Up
		if(config::vc_enable)
		{
			ext_key_input |= 0x40;
			mouse_x = 0;
			mouse_y = 0xFFF;
		}

		con_flags &= ~0x100;
	}

	//Misc Context Key 2 press
	else if((pad == config::con_key_2) && (pressed)) { con_flags |= 0x200; }
	
	//Misc Context Key 2 release
	else if((pad == config::con_key_2) && (!pressed)) { con_flags &= ~0x200; }

	//Misc Context Left press
	else if((pad == config::con_key_left) && (pressed))
	{
		//Adjust Virtual Cursor X coordinate
		vc_delta_x = -2;

		con_flags |= 0x1;
		con_flags |= 0x10;
		con_flags &= ~0x2;

		vc_pause = 0;
	}

	//Context Left release
	else if((pad == config::con_key_left) && (!pressed))
	{
		//Adjust Virtual Cursor X coordinate
		vc_delta_x = 0;

		con_flags &= ~0x1;
		con_flags &= ~0x10;

		if(con_flags & 0x20) { con_flags |= 0x2; }
		else { con_flags &= ~0x2; }

		vc_pause = 0;
	}

	//Context Right press
	else if((pad == config::con_key_right) && (pressed))
	{
		//Adjust Virtual Cursor X coordinate
		vc_delta_x = 2;

		con_flags |= 0x2;
		con_flags |= 0x20;
		con_flags &= ~0x1;

		vc_pause = 0;
	}

	//Context Right release
	else if((pad == config::con_key_right) && (!pressed))
	{
		//Adjust Virtual Cursor X coordinate
		vc_delta_x = 0;

		con_flags &= ~0x2;
		con_flags &= ~0x20;

		if(con_flags & 0x10) { con_flags |= 0x1; }
		else { con_flags &= ~0x1; }

		vc_pause = 0;
	}

	//Context Up press
	else if((pad == config::con_key_up) && (pressed))
	{
		//Adjust Virtual Cursor Y coordinate
		vc_delta_y = -2;

		con_flags |= 0x4;
		con_flags |= 0x40;
		con_flags &= ~0x8;

		vc_pause = 0;
	}

	//Context Up release
	else if((pad == config::con_key_up) && (!pressed))
	{
		//Adjust Virtual Cursor Y coordinate
		vc_delta_y = 0;

		con_flags &= ~0x4;
		con_flags &= ~0x40;

		if(con_flags & 0x80) { con_flags |= 0x8; }
		else { con_flags &= ~0x8; }

		vc_pause = 0;
	}

	//Context Down press
	else if((pad == config::con_key_down) && (pressed))
	{
		//Adjust Virtual Cursor Y coordinate
		vc_delta_y = 2;

		con_flags |= 0x8;
		con_flags |= 0x80;
		con_flags &= ~0x4;

		vc_pause = 0;
	}

	//Context Down release
	else if((pad == config::con_key_down) && (!pressed))
	{
		//Adjust Virtual Cursor Y coordinate
		vc_delta_y = 0;

		con_flags &= ~0x8;
		con_flags &= ~0x80;

		if(con_flags & 0x40) { con_flags |= 0x4; }
		else { con_flags &= ~0x4; }

		vc_pause = 0;
	}

	//Emulate Lid Close
	else if((pad == config::con_key_down) && (pressed))
	{
		ext_key_input |= 0x80;
		
		//Trigger Lid hardware IRQ
		if(nds7_input_irq != NULL) { *nds7_input_irq |= 0x400000; }
	}
	
	//Emulate Lid Open
	else if((pad == config::con_key_down) && (!pressed))
	{
		ext_key_input &= ~0x80;

		//Trigger Lid hardware IRQ
		if(nds7_input_irq != NULL) { *nds7_input_irq |= 0x400000; }
	}

	//Map keyboard input to touchscreen coordinates
	for(int x = 0; x < 10; x++)
	{
		if((config::touch_zone_pad[x]) && (pad == config::touch_zone_pad[x]))
		{
			//Emulate stylus down
			if(pressed)
			{
				ext_key_input &= ~0x40;
				mouse_x = config::touch_zone_x[x];
				mouse_y = config::touch_zone_y[x];
			}

			//Emulate stylus up
			else
			{
				ext_key_input |= 0x40;
				mouse_x = 0;
				mouse_y = 0xFFF;
			}
		}
	}

	//Terminate Turbo Buttons
	if(turbo_button_enabled)
	{
		if(pad == config::gbe_key_a) { turbo_button_end[0] = (pressed) ? false : true; }
		else if(pad == config::gbe_key_b) { turbo_button_end[1] = (pressed) ? false : true; }
		else if(pad == config::gbe_key_x) { turbo_button_end[2] = (pressed) ? false : true; }
		else if(pad == config::gbe_key_y) { turbo_button_end[3] = (pressed) ? false : true; }
		else if(pad == config::gbe_key_start) { turbo_button_end[4] = (pressed) ? false : true; }
		else if(pad == config::gbe_key_select) { turbo_button_end[5] = (pressed) ? false : true; }
		else if(pad == config::gbe_key_left) { turbo_button_end[6] = (pressed) ? false : true; }
		else if(pad == config::gbe_key_right) { turbo_button_end[7] = (pressed) ? false : true; }
		else if(pad == config::gbe_key_up) { turbo_button_end[8] = (pressed) ? false : true; }
		else if(pad == config::gbe_key_down) { turbo_button_end[9] = (pressed) ? false : true; }
		else if(pad == config::gbe_key_l_trigger) { turbo_button_end[10] = (pressed) ? false : true; }
		else if(pad == config::gbe_key_r_trigger) { turbo_button_end[11] = (pressed) ? false : true; }
	}
}

/****** Processes input based on unique pad # for joysticks ******/
void NTR_GamePad::process_joystick(int pad, bool pressed)
{
	//Emulate A button press
	if((pad == config::gbe_joy_a) && (pressed)) { key_input &= ~0x1; }

	//Emulate A button release
	else if((pad == config::gbe_joy_a) && (!pressed)) { key_input |= 0x1; }

	//Emulate B button press
	else if((pad == config::gbe_joy_b) && (pressed)) { key_input &= ~0x2; }

	//Emulate B button release
	else if((pad == config::gbe_joy_b) && (!pressed)) { key_input |= 0x2; }

	//Emulate X button press
	else if((pad == config::gbe_joy_x) && (pressed)) { ext_key_input &= ~0x1; }

	//Emulate X button release
	else if((pad == config::gbe_joy_x) && (!pressed)) { ext_key_input |= 0x1; }

	//Emulate Y button press
	else if((pad == config::gbe_joy_y) && (pressed)) { ext_key_input &= ~0x2; }

	//Emulate Y button release
	else if((pad == config::gbe_joy_y) && (!pressed)) { ext_key_input |= 0x2; }

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

	//Misc Context Key 1 press
	else if((pad == config::con_joy_1) && (pressed))
	{
		//Virtual Cursor - Emulate Stylus Down
		if(config::vc_enable)
		{
			ext_key_input &= ~0x40;
			mouse_x = vc_x;
			mouse_y = vc_y;
		}

		con_flags |= 0x100;
	}
	
	//Misc Context Key 1 release
	else if((pad == config::con_joy_1) && (!pressed))
	{
		//Virtual Cursor - Emulate Stylus Up
		if(config::vc_enable)
		{
			ext_key_input |= 0x40;
			mouse_x = 0;
			mouse_y = 0xFFF;
		}

		con_flags &= ~0x100;
	}

	//Misc Context Key 2 press
	else if((pad == config::con_joy_2) && (pressed)) { con_flags |= 0x200; }
	
	//Misc Context Key 2 release
	else if((pad == config::con_joy_2) && (!pressed)) { con_flags &= ~0x200; }

	//Misc Context Left press
	else if((pad == config::con_joy_left) && (pressed))
	{
		//Adjust Virtual Cursor X coordinate
		vc_delta_x = -2;

		con_flags |= 0x1;
		con_flags |= 0x10;
		con_flags &= ~0x2;

		vc_pause = 0;
	}

	//Context Left release
	else if((pad == config::con_joy_left) && (!pressed))
	{
		//Adjust Virtual Cursor X coordinate
		vc_delta_x = 0;

		con_flags &= ~0x1;
		con_flags &= ~0x10;

		if(con_flags & 0x20) { con_flags |= 0x2; }
		else { con_flags &= ~0x2; }

		vc_pause = 0;
	}

	//Context Right press
	else if((pad == config::con_joy_right) && (pressed))
	{
		//Adjust Virtual Cursor X coordinate
		vc_delta_x = 2;

		con_flags |= 0x2;
		con_flags |= 0x20;
		con_flags &= ~0x1;

		vc_pause = 0;
	}

	//Context Right release
	else if((pad == config::con_joy_right) && (!pressed))
	{
		//Adjust Virtual Cursor X coordinate
		vc_delta_x = 0;

		con_flags &= ~0x2;
		con_flags &= ~0x20;

		if(con_flags & 0x10) { con_flags |= 0x1; }
		else { con_flags &= ~0x1; }

		vc_pause = 0;
	}

	//Context Up press
	else if((pad == config::con_joy_up) && (pressed))
	{
		//Adjust Virtual Cursor Y coordinate
		vc_delta_y = -2;

		con_flags |= 0x4;
		con_flags |= 0x40;
		con_flags &= ~0x8;

		vc_pause = 0;
	}

	//Context Up release
	else if((pad == config::con_joy_up) && (!pressed))
	{
		//Adjust Virtual Cursor Y coordinate
		vc_delta_y = 0;

		con_flags &= ~0x4;
		con_flags &= ~0x40;

		if(con_flags & 0x80) { con_flags |= 0x8; }
		else { con_flags &= ~0x8; }

		vc_pause = 0;
	}

	//Context Down press
	else if((pad == config::con_joy_down) && (pressed))
	{
		//Adjust Virtual Cursor Y coordinate
		vc_delta_y = 2;

		con_flags |= 0x8;
		con_flags |= 0x80;
		con_flags &= ~0x4;

		vc_pause = 0;
	}

	//Context Down release
	else if((pad == config::con_joy_down) && (!pressed))
	{
		//Adjust Virtual Cursor Y coordinate
		vc_delta_y = 0;

		con_flags &= ~0x8;
		con_flags &= ~0x80;

		if(con_flags & 0x40) { con_flags |= 0x4; }
		else { con_flags &= ~0x4; }

		vc_pause = 0;
	}

	//Terminate Turbo Buttons
	if(turbo_button_enabled)
	{
		if(pad == config::gbe_joy_a) { turbo_button_end[0] = (pressed) ? false : true; }
		else if(pad == config::gbe_joy_b) { turbo_button_end[1] = (pressed) ? false : true; }
		else if(pad == config::gbe_joy_x) { turbo_button_end[2] = (pressed) ? false : true; }
		else if(pad == config::gbe_joy_y) { turbo_button_end[3] = (pressed) ? false : true; }
		else if(pad == config::gbe_joy_start) { turbo_button_end[4] = (pressed) ? false : true; }
		else if(pad == config::gbe_joy_select) { turbo_button_end[5] = (pressed) ? false : true; }
		else if(pad == config::gbe_joy_left) { turbo_button_end[6] = (pressed) ? false : true; }
		else if(pad == config::gbe_joy_right) { turbo_button_end[7] = (pressed) ? false : true; }
		else if(pad == config::gbe_joy_up) { turbo_button_end[8] = (pressed) ? false : true; }
		else if(pad == config::gbe_joy_down) { turbo_button_end[9] = (pressed) ? false : true; }
		else if(pad == config::gbe_joy_l_trigger) { turbo_button_end[10] = (pressed) ? false : true; }
		else if(pad == config::gbe_joy_r_trigger) { turbo_button_end[11] = (pressed) ? false : true; }
	}
}

/****** Processes input based on the mouse ******/
void NTR_GamePad::process_mouse(int pad, bool pressed)
{
	//Unpack special pad data when receiving input from an external source (see NTR core feed_key_input())
	if((pad != 400) && (pad != 402))
	{
		mouse_x = pad & 0xFF;
		mouse_y = ((pad >> 8) & 0xFF);
		
		//Only unpack X and Y coordinates if mouse motion flag is set
		if(pad & 0x40000) { pad = 0; }
		else { pad = 400 + (((pad & 0x30000) >> 17) << 1); }
	}

	//Emulate touchscreen press (NDS mode only, DSi does not use this) - Manual Hold Mode
	if((pad == 400) && (pressed) && (!touch_hold))
	{
		touch_by_mouse = true;
		ext_key_input &= ~0x40;
	}

	//Emulate touchscreen release (NDS mode only, DSi does not use this) - Manual Hold Mode
	else if((pad == 400) && (!pressed) && (!touch_hold))
	{
		touch_by_mouse = false;
		ext_key_input |= 0x40;

		mouse_x = 0;
		mouse_y = 0xFFF;
	}

	//End Auto Hold Mode if left-click happens
	else if((pad == 400) && (touch_hold))
	{
		touch_hold = false;
		ext_key_input |= 0x40;

		mouse_x = 0;
		mouse_y = 0xFFF;
	}

	//Emulate touchscreen press (NDS mode only, DSi does not use this) - Auto Hold Mode
	else if((pad == 402) && (pressed) && (!touch_hold))
	{
		touch_hold = true;
		touch_by_mouse = true;
		ext_key_input &= ~0x40;
	}

	//Emulate touchscreen release (NDS mode only, DSi does not use this) - Auto Hold Mode
	else if((pad == 402) && (pressed) && (touch_hold))
	{
		touch_hold = false;
		touch_by_mouse = false;
		ext_key_input |= 0x40;

		mouse_x = 0;
		mouse_y = 0xFFF;
	}
}

/****** Updates Virtual Cursor coordinates ******/
void NTR_GamePad::process_virtual_cursor()
{
	vc_counter++;
	vc_pause++;

	//Update cursor X and Y after set number of frames
	if(vc_counter >= config::vc_wait)
	{
		vc_counter = 0;

		if(vc_pause < config::vc_timeout)
		{
			vc_x += vc_delta_x;
			vc_y += vc_delta_y;

			if(vc_x >= 252) { vc_x = 252; }
			if(vc_x <= 4) { vc_x = 4; }

			if(vc_y >= 188) { vc_y = 188; }
			if(vc_y <= 4) { vc_y = 4; }
		}
	}
}
		

/****** Start haptic force-feedback on joypad ******/
void NTR_GamePad::start_rumble(s32 len)
{
	if((jstick != NULL) && (rumble != NULL) && (is_rumbling == false))
	{
		SDL_HapticRumblePlay(rumble, 1, len);
		is_rumbling = true;
	}
}

/****** Stop haptic force-feedback on joypad ******/
void NTR_GamePad::stop_rumble()
{
	if((jstick != NULL) && (rumble != NULL) && (is_rumbling == true))
	{
		SDL_HapticRumbleStop(rumble);
       		is_rumbling = false;
	}
}

/****** Clears any existing input - Primarily used for the SoftReset SWI ******/
void NTR_GamePad::clear_input()
{
	key_input = 0x3FF;
	ext_key_input = 0x7F;
	up_shadow = down_shadow = left_shadow = right_shadow = false;
	mouse_x = 0;
	mouse_y = 0xFFF;
	touch_hold = false;
	touch_by_mouse = false;
}

/****** Process turbo button input ******/
void NTR_GamePad::process_turbo_buttons()
{
	for(u32 x = 0; x < 12; x++)
	{
		u16 *input = NULL;
		u16 mask = 0;

		//Grab the appropiate mask for each button
		//Cycle through all turbo buttons (0 - 11), and only use those that apply to this core
		switch(x)
		{
			case 0: input = &key_input; mask = 0x01; break;
			case 1: input = &key_input; mask = 0x02; break;
			case 2: input = &ext_key_input; mask = 0x01; break;
			case 3: input = &ext_key_input; mask = 0x02; break;
			case 4: input = &key_input; mask = 0x08; break;
			case 5: input = &key_input; mask = 0x04; break;
			case 6: input = &key_input; mask = 0x20; break;
			case 7: input = &key_input; mask = 0x10; break;
			case 8: input = &key_input; mask = 0x40; break;
			case 9: input = &key_input; mask = 0x80; break;
			case 10: input = &key_input; mask = 0x200; break;
			case 11: input = &key_input; mask = 0x100; break;
		}

		//Continue only if turbo button is used on this core
		if(mask)
		{
			//Turbo Button Start
			if(((*input & mask) == 0) && (config::gbe_turbo_button[x]) && (!turbo_button_stat[x]))
			{
				turbo_button_stat[x] = true;
				turbo_button_val[x] = 0;
				*input |= mask;
			}

			//Turbo Button Delay
			else if(turbo_button_stat[x])
			{
				turbo_button_val[x]++;

				if(turbo_button_val[x] >= config::gbe_turbo_button[x])
				{
					turbo_button_stat[x] = false;
					*input &= ~mask;
				}
			}

			//Turbo Button End
			if(turbo_button_end[x])
			{
				turbo_button_end[x] = false;
				turbo_button_stat[x] = false;
				turbo_button_val[x] = 0;
				*input |= mask;
			}
		}
	}	
}
