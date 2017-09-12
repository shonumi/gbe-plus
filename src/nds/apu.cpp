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

	apu_stat.channel_master_volume = (config::volume >> 2);
	apu_stat.channel_left_volume = 0.0;
	apu_stat.channel_right_volume = 0.0;

	//Reset Channel 1-16 data
	for(int x = 0; x < 4; x++)
	{
		apu_stat.channel[x].output_frequency = 0;
		apu_stat.channel[x].data_src = 0;
		apu_stat.channel[x].loop_start = 0;
		apu_stat.channel[x].length = 0;
		apu_stat.channel[x].cnt = 0;

		apu_stat.channel[x].playing = 0;
		apu_stat.channel[x].enable = 0;
		apu_stat.channel[x].right_enable = 0;
		apu_stat.channel[x].left_enable = 0;

		for(int y = 0; y < 0x10000; y++) { apu_stat.channel[x].buffer[y] = -32768; }
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

/****** SDL Audio Callback ******/ 
void ntr_audio_callback(void* _apu, u8 *_stream, int _length)
{
	s16* stream = (s16*) _stream;
	int length = _length/2;

	//Custom software mixing
	for(u32 x = 0; x < length; x++)
	{
		stream[x] = -32768;
	} 
}
