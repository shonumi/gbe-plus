// GB Enhanced+ Copyright Daniel Baxter 2021
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : apu_data.h
// Date : May 06, 2021
// Description : Audio data
//
// Defines the APU data structures that the MMU will update whenever values are written in memory
// Only the APU should read values from this namespace. Only the MMU should write values to this namespace.

#ifndef PM_APU_DATA
#define PM_APU_DATA

#include "common.h"

struct min_apu_data
{

	u16 raw_frequency;
	double output_frequency;
	double duty_cycle;
	u32 frequency_distance;

	s16 buffer[0x10000];
	u16 buffer_size;
	u16 current_index;
	u16 last_index;

	bool pwm_needs_fill;
	u32 pwm_fill_rate;

	bool sound_on;
	u8 main_volume = 0;
	double sample_rate;
	u8 channel_master_volume;
};

#endif // PM_APU_DATA
 
 
