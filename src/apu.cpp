// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : apu.h
// Date : March 05, 2015
// Description : Game Boy Advance APU emulation
//
// Sets up SDL audio for mixing
// Generates and mixes samples for the GBA's 4 sound channels + DMA channels  

#include "apu.h"

/****** APU Constructor ******/
APU::APU()
{
	reset();
}

/****** APU Destructor ******/
APU::~APU()
{
	SDL_CloseAudio();
	std::cout<<"APU::Shutdown\n";
}

/****** Reset APU ******/
void APU::reset()
{
	setup = false;

	apu_stat.sound_on = false;
	apu_stat.stereo = false;

	apu_stat.channel_left_volume = 0;
	apu_stat.channel_right_volume = 0;
	apu_stat.dma_left_volume = 0;
	apu_stat.dma_right_volume = 0;
	
	//Reset Channel 1-4 data
	for(int x = 0; x < 4; x++)
	{
		apu_stat.channel[x].raw_frequency = 0;
		apu_stat.channel[x].output_frequency = 0.0;

		apu_stat.channel[x].duration = 0;
		apu_stat.channel[x].volume = 0;

		apu_stat.channel[x].playing = false;
		apu_stat.channel[x].length_flag = false;

		apu_stat.channel[x].duty_cycle_start = 0;
		apu_stat.channel[x].duty_cycle_end = 4;

		apu_stat.channel[x].envelope_direction = 2;
		apu_stat.channel[x].envelope_step = 0;
		apu_stat.channel[x].envelope_counter = 0;

		apu_stat.channel[x].sweep_direction = 2;
		apu_stat.channel[x].sweep_shift = 0;
		apu_stat.channel[x].sweep_time = 0;
		apu_stat.channel[x].sweep_counter = 0;

		apu_stat.channel[x].frequency_distance = 0;
		apu_stat.channel[x].sample_length = 0;
	}
}

/****** Initialize APU with SDL ******/
bool APU::init()
{
	//Initialize audio subsystem
	SDL_InitSubSystem(SDL_INIT_AUDIO);

	//Setup the desired audio specifications
    	desired_spec.freq = 44100;
	desired_spec.format = AUDIO_S16SYS;
    	desired_spec.channels = 1;
    	desired_spec.samples = 2048;
    	desired_spec.callback = audio_callback;
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
		SDL_PauseAudio(0);
		std::cout<<"APU::Initialized\n";
		return true;
	}
}

/****** Run APU for one cycle ******/
void APU::step() { }		

/****** SDL Audio Callback ******/ 
void audio_callback(void* _apu, u8 *_stream, int _length) { }
		