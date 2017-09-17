// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : apu.cpp
// Date : September 10, 2017
// Description : NDS APU emulation
//
// Sets up SDL audio for mixing
// Generates and mixes samples for the NDS's 16 sound channels  

#include <cmath>

#include "apu.h"

/****** APU Constructor ******/
NTR_APU::NTR_APU()
{
	reset();
}

/****** APU Destructor ******/
NTR_APU::~NTR_APU()
{
	SDL_CloseAudio();
	std::cout<<"APU::Shutdown\n";
}

/****** Reset APU ******/
void NTR_APU::reset()
{
	SDL_CloseAudio();

	apu_stat.sound_on = false;
	apu_stat.stereo = false;

	apu_stat.sample_rate = config::sample_rate;
	apu_stat.main_volume = 4;

	apu_stat.channel_master_volume = config::volume;
	apu_stat.channel_left_volume = 0.0;
	apu_stat.channel_right_volume = 0.0;

	//Reset Channel 1-16 data
	for(int x = 0; x < 16; x++)
	{
		apu_stat.channel[x].output_frequency = 0.0;
		apu_stat.channel[x].data_src = 0;
		apu_stat.channel[x].data_pos = 0;
		apu_stat.channel[x].loop_start = 0;
		apu_stat.channel[x].length = 0;
		apu_stat.channel[x].samples = 0;
		apu_stat.channel[x].cnt = 0;

		apu_stat.channel[x].playing = false;
		apu_stat.channel[x].enable = false;
		apu_stat.channel[x].right_enable = false;
		apu_stat.channel[x].left_enable = false;
	}
}

/****** Initialize APU with SDL ******/
bool NTR_APU::init()
{
	//Initialize audio subsystem
	if(SDL_InitSubSystem(SDL_INIT_AUDIO) == -1)
	{
		std::cout<<"APU::Error - Could not initialize SDL audio\n";
		return false;
	}

	//Setup the desired audio specifications
    	desired_spec.freq = apu_stat.sample_rate;
	desired_spec.format = AUDIO_S16SYS;
    	desired_spec.channels = 1;
    	desired_spec.samples = 4096;
    	desired_spec.callback = ntr_audio_callback;
    	desired_spec.userdata = this;

    	//Open SDL audio for desired specifications
	if(SDL_OpenAudio(&desired_spec, &obtained_spec) < 0) 
	{ 
		std::cout<<"APU::Failed to open audio\n";
		return false; 
	}
	else if(desired_spec.format != obtained_spec.format) 
	{ 
		std::cout<<"APU::Could not obtain desired audio format\n";
		return false;
	}

	else
	{
		apu_stat.channel_master_volume = config::volume;

		SDL_PauseAudio(0);
		std::cout<<"APU::Initialized\n";
		return true;
	}
}

/****** Generates samples for NDS sound channels ******/
void NTR_APU::generate_channel_samples(s32* stream, int length, u8 id)
{
	double sample_ratio = (apu_stat.channel[id].output_frequency / apu_stat.sample_rate);
	u32 sample_pos = apu_stat.channel[id].data_pos;
	u8 format = ((apu_stat.channel[id].cnt >> 29) & 0x3);

	s8 nds_sample_8 = 0;
	s16 nds_sample_16 = 0;
	u32 samples_played = 0;

	for(u32 x = 0; x < length; x++)
	{
		//Channel 0 should set default data
		if(id == 0) { stream[x] = 0; }

		//Pull data from NDS memory
		if((apu_stat.channel[id].samples) && (apu_stat.channel[id].playing))
		{
			//PCM8
			if(format == 0)
			{
				nds_sample_8 = mem->memory_map[sample_pos + (sample_ratio * x)];
				apu_stat.channel[id].samples--;

				//Scale S8 audio to S16
				stream[x] += (nds_sample_8 * 256);
			}

			else { stream[x] += -32768; }

			samples_played++;

			if(apu_stat.channel[id].samples == 0) { apu_stat.channel[id].playing = false; }
		}

		//Generate silence if sound has run out of samples or is not playing
		else { stream[x] += -32768; }
	}

	//Advance data pointer to sound samples
	apu_stat.channel[id].data_pos += (sample_ratio * samples_played);
}

/****** SDL Audio Callback ******/ 
void ntr_audio_callback(void* _apu, u8 *_stream, int _length)
{
	s16* stream = (s16*) _stream;
	int length = _length/2;
	s32 channel_stream[length];

	NTR_APU* apu_link = (NTR_APU*) _apu;

	//Generate samples
	for(u32 x = 0; x < 16; x++) { apu_link->generate_channel_samples(channel_stream, length, x); }

	//Custom software mixing
	for(u32 x = 0; x < length; x++)
	{
		channel_stream[x] /= 16;
		stream[x] = channel_stream[x];
	}
}
