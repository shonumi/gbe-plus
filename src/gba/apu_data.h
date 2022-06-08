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

struct agb_apu_data
{
	//Sound Channels 1-4, nearly identical to the DMG/GBC
	struct sound_channels
	{
		u16 raw_frequency;
		double output_frequency;

		double duration;
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

		s16 buffer[0x10000];
		u16 buffer_size;
		u16 current_index;
		u16 last_index;
	} channel[4];

	//Digital channels, new to the GBA
	struct dma_channels
	{
		u32 output_frequency;
		u16 counter;
		u16 last_position;
		u16 length;
		u8 timer;
		u8 master_volume;

		bool playing;
		bool enable;
		bool right_enable;
		bool left_enable;

		s8 buffer[0x10000];
	} dma[2];

	//External audio channel for addittional sources of audio output
	//For examples, cartriges with headphone jacks
	struct extra_channels
	{
		u32 frequency;
		u32 length;
		u32 sample_pos;
		u32 set_count;
		u32 current_set;
		u8* buffer;
		u8 output_path;
		u8 channels;
		u8 volume;
		u8 id;
		bool playing;
	} ext_audio;

	bool psg_needs_fill;
	u32 psg_fill_rate;

	bool sound_on;
	bool stereo;

	u8 main_volume;
	double sample_rate;

	u8 channel_master_volume;
	double channel_left_volume;
	double channel_right_volume;

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
 
