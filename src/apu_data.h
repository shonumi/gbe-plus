// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : apu_data.h
// Date : March 04, 2015
// Description : Core data
//
// Defines the APU data structures that the MMU will update whenever values are written in memory
// Only the APU should read values from this namespace. Only the MMU should write values to this namespace.

#ifndef GBA_APU_DATA
#define GBA_APU_DATA

#include "common.h"

struct apu_data
{
	//Sound Channels 1-4, nearly identical to the DMG/GBC
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
	} channel[4];

	//Digital channels, new to the GBA
	struct dma_channels
	{
		u32 output_frequency;
		u16 counter;
		u8 timer;
		u8 volume;
		int sample_length;

		bool playing;
		bool enable;
		bool right_enable;
		bool left_enable;
		bool length_flag;

		s8 buffer[0x4000];
	} dma[2];

	bool sound_on;
	bool stereo;

	u8 channel_left_volume;
	u8 channel_right_volume;
	u8 dma_left_volume;
	u8 dma_right_volume;

	u8 waveram_bank_play;
	u8 waveram_bank_rw;
	u8 waveram_sample;
	u8 waveram_size;
	u8 waveram_data[0x20];

	double noise_dividing_ratio;
	u32 noise_prescalar;
	u8 noise_stages;
	u8 noise_7_stage_lsfr;
	u16 noise_15_stage_lsfr;
};

#endif // GBA_APU_DATA
 
