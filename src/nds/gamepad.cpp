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
	}
}

/****** GamePad Destructor ******/
NTR_GamePad::~NTR_GamePad() { } 

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
		if(((key_cnt & 0x8000) == 0) && (key_input & key_mask))  { joypad_irq = true; }

		//Logical AND mode
		else if ((key_cnt & 0x8000) && ((key_input & key_mask) == key_mask)) { joypad_irq = true; }
	}

	else { joypad_irq = false; }
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
		vc_delta_x = -8;

		con_flags |= 0x1;
		con_flags |= 0x10;
		con_flags &= ~0x2;
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
	}

	//Context Right press
	else if((pad == config::con_key_right) && (pressed))
	{
		//Adjust Virtual Cursor X coordinate
		vc_delta_x = 8;

		con_flags |= 0x2;
		con_flags |= 0x20;
		con_flags &= ~0x1;
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
	}

	//Context Up press
	else if((pad == config::con_key_up) && (pressed))
	{
		//Adjust Virtual Cursor Y coordinate
		vc_delta_y = -8;

		con_flags |= 0x4;
		con_flags |= 0x40;
		con_flags &= ~0x8;
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
	}

	//Context Down press
	else if((pad == config::con_key_down) && (pressed))
	{
		//Adjust Virtual Cursor Y coordinate
		vc_delta_y = 8;

		con_flags |= 0x8;
		con_flags |= 0x80;
		con_flags &= ~0x4;
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

	//Update cursor X and Y after set number of frames
	if(vc_counter >= config::vc_wait)
	{
		vc_counter = 0;
		vc_x += vc_delta_x;
		vc_y += vc_delta_y;
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
