// GB Enhanced+ Copyright Daniel Baxter 2017
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : dmg_core_pad.h
// Date : June 13, 2017
// Description : Abstracted DMG gamepad interface
//
// Gamepad interface that can be used for different systems (DMG, GBC, SGB)
// Intended for DMG and SGB cores only

#ifndef DMG_CORE_PAD
#define DMG_CORE_PAD

#include <SDL2/SDL.h>

#include "common/common.h"

class dmg_core_pad
{
	public:

	dmg_core_pad() {};
	~dmg_core_pad() {};

	u8 p14, p15;
	u8 column_id;

	u16 sensor_x;
	u16 sensor_y;
	u8 gyro_flags;
	u8 con_flags;
	u16 ir_delay;

	int pad;

	bool joypad_irq;
	bool joy_init;

	//Shadow status for keyboard input
	bool up_shadow, down_shadow, left_shadow, right_shadow;

	SDL_Joystick* jstick;
    	SDL_Haptic* rumble;
	bool is_rumbling;

	virtual void handle_input(SDL_Event &event) = 0;
	virtual void init() = 0;

	virtual void process_keyboard(int pad, bool pressed) = 0;
	virtual void process_joystick(int pad, bool pressed) = 0;
	virtual void process_gyroscope() = 0;
	virtual void start_rumble() = 0;
	virtual void stop_rumble() = 0;
	virtual u8 read() = 0;
	virtual void write(u8 value) = 0;
	virtual u32 get_pad_data(u32 index) = 0;
	virtual void set_pad_data(u32 index, u32 value) = 0;
};

#endif // DMG_CORE_PAD 
