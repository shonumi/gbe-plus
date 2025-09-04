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
	//Digital channels
	struct digital_channels
	{
		double output_frequency;
		u32 data_src;
		u32 data_pos;
		u32 loop_start;
		u32 length;
		u32 samples;
		u32 cnt;
		u16 timer;
		u8 volume;

		bool playing;
		bool enable;

		u32 adpcm_header;
		u32 adpcm_pos;
		u8 adpcm_index;
		u16 adpcm_val;
		std::vector<s16> adpcm_buffer;
		bool decode_adpcm;
	} channel[16];

	//IMA-ADPCM table
	u16 adpcm_table[128];

	//Index table
	s8 index_table[8];

	bool sound_on;
	bool stereo;

	u8 main_volume;
	double sample_rate;

	u8 channel_master_volume;

	u16 mic_out;
};

#endif // NDS_APU_DATA
 
