// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : apu_data.h
// Date : May 30, 2015
// Description : Core data
//
// Defines the APU data structures that the MMU will update whenever values are written in memory
// Only the APU should read values from this namespace. Only the MMU should write values to this namespace.

#ifndef GB_APU_DATA
#define GB_APU_DATA

#include "common.h"

struct dmg_apu_data
{
	//Sound Channels 1-4
	struct sound_channels
	{
		u16 raw_frequency;
		double output_frequency;

		u32 duration;
		u32 volume;

		bool playing;
		bool enable;
		bool right_enable;
		bool left_enable;
		bool length_flag;

		u8 duty_cycle;
		u8 duty_cycle_start;
		u8 duty_cycle_end;

		u8 envelope_direction;
		u8 envelope_step;
		u32 envelope_counter;

		u8 sweep_shift;
		u8 sweep_direction;
		u8 sweep_time;
		u32 sweep_counter;
		bool sweep_on;

		u32 frequency_distance;
		int sample_length;

		bool so1_output;
		bool so2_output;
	} channel[4];

	bool sound_on;
	bool stereo;

	u8 main_volume;
	double sample_rate;

	u8 channel_master_volume;
	double channel_left_volume;
	double channel_right_volume;

	u8 waveram_sample;
	u8 waveram_data[0x20];

	double noise_dividing_ratio;
	u32 noise_prescalar;
	u8 noise_stages;
	u8 noise_7_stage_lsfr;
	u16 noise_15_stage_lsfr;
};

#endif // GB_APU_DATA
 
