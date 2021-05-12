// GB Enhanced Copyright Daniel Baxter 2021
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : apu.cpp
// Date : May 06, 2021
// Description : Game Boy Advance APU emulation
//
// Sets up SDL audio for mixing
// Generates and mixes samples for the PM's single sound channel  

#include <cmath>

#include "apu.h"

/****** APU Constructor ******/
MIN_APU::MIN_APU()
{
	reset();
}

/****** APU Destructor ******/
MIN_APU::~MIN_APU()
{
	SDL_CloseAudio();
	std::cout<<"APU::Shutdown\n";
}

/****** Reset APU ******/
void MIN_APU::reset()
{
	SDL_CloseAudio();

	apu_stat.raw_frequency = 0.0;
	apu_stat.output_frequency = 0.0;
	apu_stat.duty_cycle = 0.0;
	apu_stat.frequency_distance = 0;

	apu_stat.buffer_size = 0;
	apu_stat.current_index = 0;
	apu_stat.last_index = 0;

	for(u32 x = 0; x < 0x10000; x++) { apu_stat.buffer[x] = -32768; }

	apu_stat.pwm_needs_fill = true;
	apu_stat.pwm_fill_rate = 0;

	apu_stat.sound_on = false;
	apu_stat.main_volume = 0;
	apu_stat.sample_rate = config::sample_rate;
	apu_stat.channel_master_volume = config::volume;
}

/****** Initialize APU with SDL ******/
bool MIN_APU::init()
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
    	desired_spec.samples = 256;
    	desired_spec.callback = min_audio_callback;
    	desired_spec.userdata = this;

    	//Open SDL audio for desired specifications
	if(SDL_OpenAudio(&desired_spec, NULL) < 0) 
	{ 
		std::cout<<"APU::Failed to open audio\n";
		return false; 
	}

	else
	{
		apu_stat.channel_master_volume = config::volume;

		apu_stat.pwm_fill_rate = apu_stat.sample_rate / 144;

		SDL_PauseAudio(0);
		std::cout<<"APU::Initialized\n";
		return true;
	}
}

/******* Generate samples for the single sound channel ******/
void MIN_APU::generate_samples(s16* stream, int length)
{
	//Determine if more data needs to be buffered
	while(apu_stat.buffer_size < length)
	{
		buffer_channel();
		apu_stat.pwm_needs_fill = false;
	}

	//Copy from last position in the buffer
	for(int x = 0; x < length; x++)
	{
		stream[x] = apu_stat.buffer[apu_stat.last_index++];
	}

	apu_stat.buffer_size -= length;

	//Drain buffer if it gets too large
	if(apu_stat.buffer_size >= 512)
	{
		apu_stat.buffer_size = 0;
		apu_stat.last_index = 0;
		apu_stat.current_index = 0;
	}
}

/****** Buffer sound channel data ******/
void MIN_APU::buffer_channel()
{
	int length = apu_stat.pwm_fill_rate;
	apu_stat.buffer_size += length;

	apu_stat.output_frequency = mem->get_timer3_freq();

	//Determine if sound is on
	if((!apu_stat.duty_cycle) || (apu_stat.duty_cycle >= 1.0)) { apu_stat.sound_on = false; }
	else if((mem->timer->at(2).osc_lo == 0) && (!mem->osc_1_enable)) { apu_stat.sound_on = false; }
	else if((mem->timer->at(2).osc_lo == 1) && (!mem->osc_2_enable)) { apu_stat.sound_on = false; }
	else if(!mem->timer->at(2).enable_scalar_lo || !mem->timer->at(2).enable_lo) { apu_stat.sound_on = false; }
	else if(mem->timer->at(2).enable_lo) { apu_stat.sound_on = true; }

	//Generate samples from the last output of the channel
	if(apu_stat.sound_on)
	{
		int frequency_samples = apu_stat.sample_rate/apu_stat.output_frequency;

		for(int x = 0; x < length; x++)
		{
			apu_stat.frequency_distance++;

			//Reset frequency distance
			if(apu_stat.frequency_distance >= frequency_samples) { apu_stat.frequency_distance = 0; }
		
			//Generate wave form based on duty cycle
			if(apu_stat.frequency_distance < (frequency_samples * apu_stat.duty_cycle))
			{
				switch(apu_stat.main_volume & 0x3)
				{
					case 0:
						apu_stat.buffer[apu_stat.current_index++] = -32768;
						break;

					case 1:
					case 2:
						apu_stat.buffer[apu_stat.current_index++] = 0;
						break;

					case 3:
						apu_stat.buffer[apu_stat.current_index++] = 32767;
						break;
				}
			}

			else { apu_stat.buffer[apu_stat.current_index++] = -32768; }
		}
	}

	//Otherwise, generate silence
	else 
	{
		for(int x = 0; x < length; x++) { apu_stat.buffer[apu_stat.current_index++] = -32768; }
	}
}

/****** SDL Audio Callback ******/ 
void min_audio_callback(void* _apu, u8 *_stream, int _length)
{
	s16* stream = (s16*) _stream;
	int length = _length/2;

	std::vector<s16> channel_stream(length);

	MIN_APU* apu_link = (MIN_APU*) _apu;
	apu_link->generate_samples(&channel_stream[0], length);

	double channel_ratio = apu_link->apu_stat.channel_master_volume / 128.0;

	//Custom software mixing
	for(u32 x = 0; x < length; x++)
	{
		//Multiply output by volume ratio
		s16 out_sample = channel_stream[x] * channel_ratio;

		stream[x] = out_sample;
	}
}
