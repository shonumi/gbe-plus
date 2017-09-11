// GB Enhanced+ Copyright Daniel Baxter 2017
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : apu_data.h
// Date : September 10, 2017
// Description : Core data
//
// Defines the APU data structures that the MMU will update whenever values are written in memory
// Only the APU should read values from this namespace. Only the MMU should write values to this namespace.

#ifndef NDS_APU_DATA
#define NDS_APU_DATA

#include "common.h"

struct ntr_apu_data
{
	//Digital channels, new to the GBA
	struct digital_channels
	{
		u32 output_frequency;
		u32 data_src;
		u32 loop_start;
		u32 length;

		bool playing;
		bool enable;
		bool right_enable;
		bool left_enable;

		s16 buffer[0x10000];
	} channel[16];

	bool sound_on;
	bool stereo;

	u8 main_volume;
	double sample_rate;

	u8 channel_master_volume;
	double channel_left_volume;
	double channel_right_volume;
};

#endif // NDS_APU_DATA
 
