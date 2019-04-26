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
		apu_stat.channel[x].volume = 0;

		apu_stat.channel[x].playing = false;
		apu_stat.channel[x].enable = false;

		apu_stat.channel[x].adpcm_header = 0;
		apu_stat.channel[x].adpcm_pos = 0;
		apu_stat.channel[x].adpcm_index = 0;
		apu_stat.channel[x].adpcm_val = 0;
		apu_stat.channel[x].adpcm_buffer.clear();
		apu_stat.channel[x].decode_adpcm = false;
	}

	//Setup IMA-ADPCM table
	for(u32 x = 0x776D2, a = 0; a < 128; a++)
	{
		if(a == 3) { apu_stat.adpcm_table[a] = 0xA; }
		else if(a == 4) { apu_stat.adpcm_table[a] = 0xB; }
		else if(a == 88) { apu_stat.adpcm_table[a] = 0x7FFF; }
		else if(a >= 89) { apu_stat.adpcm_table[a] = 0; }
		else { apu_stat.adpcm_table[a] = (x >> 16); }
		
		x = x + (x / 10);
	}

	//Setup index table
	apu_stat.index_table[0] = apu_stat.index_table[1] = apu_stat.index_table[2] = apu_stat.index_table[3] = -1;
	apu_stat.index_table[4] = 2;
	apu_stat.index_table[5] = 4;
	apu_stat.index_table[6] = 6;
	apu_stat.index_table[7] = 8;
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
	if(SDL_OpenAudio(&desired_spec, NULL) < 0) 
	{ 
		std::cout<<"APU::Failed to open audio\n";
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
	u8 loop_mode = ((apu_stat.channel[id].cnt >> 27) & 0x3);

	//Calculate volume
	float vol = (apu_stat.channel[id].volume != 0) ? (apu_stat.channel[id].volume / 127.0) : 0;
	vol *= (apu_stat.main_volume / 127.0);
	vol *= (config::volume / 128.0);

	s8 nds_sample_8 = 0;
	s16 nds_sample_16 = 0;
	u32 samples_played = 0;
	u32 adpcm_pos = 0;

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
				u32 data_addr = (sample_pos + (sample_ratio * x));
				nds_sample_8 = mem->memory_map[sample_pos + (sample_ratio * x)];

				//Scale S8 audio to S16
				stream[x] += (nds_sample_8 * 256);

				//Adjust volume level
				stream[x] *= vol;

				if(data_addr >= (apu_stat.channel[id].data_src + apu_stat.channel[id].samples))
				{
					//Loop sound
					if(loop_mode == 1)
					{
						apu_stat.channel[id].data_src += (apu_stat.channel[id].loop_start * 4);
						apu_stat.channel[id].data_pos = apu_stat.channel[id].data_src;
						apu_stat.channel[id].samples = (apu_stat.channel[id].length * 4);
					}
					
					//Stop sound
					else if(loop_mode == 2)
					{
						apu_stat.channel[id].playing = false;
						apu_stat.channel[id].cnt &= ~0x80000000;
					}
				}	
			}

			//PCM16
			else if(format == 1)
			{
				u32 data_addr = (sample_pos + (sample_ratio * x));
				data_addr &= ~0x1;
				nds_sample_16 = mem->read_u16_fast(data_addr);

				stream[x] += nds_sample_16;

				//Adjust volume level
				stream[x] *= vol;

				if(data_addr >= (apu_stat.channel[id].data_src + apu_stat.channel[id].samples))
				{
					//Loop sound
					if(loop_mode == 1)
					{
						apu_stat.channel[id].data_src += (apu_stat.channel[id].loop_start * 2);
						apu_stat.channel[id].data_pos = apu_stat.channel[id].data_src;
						apu_stat.channel[id].samples = (apu_stat.channel[id].length * 2);
					}
					
					//Stop sound
					else if(loop_mode == 2)
					{	
						apu_stat.channel[id].playing = false;
						apu_stat.channel[id].cnt &= ~0x80000000;
					}
				}	
			}

			//IMA-ADPCM
			else if(format == 2)
			{
				u32 data_pos = (apu_stat.channel[id].adpcm_pos + (sample_ratio * x));
				if(data_pos > apu_stat.channel[id].adpcm_buffer.size()) { data_pos = (apu_stat.channel[id].adpcm_buffer.size() - 1); }
				nds_sample_16 = apu_stat.channel[id].adpcm_buffer[data_pos];

				stream[x] += nds_sample_16;

				//Adjust volume level
				stream[x] *= vol;

				if(data_pos >= apu_stat.channel[id].samples)
				{
					//Loop sound
					if(loop_mode == 1)
					{
						apu_stat.channel[id].data_pos = apu_stat.channel[id].data_src;
						apu_stat.channel[id].samples = ((apu_stat.channel[id].length - 1) * 8);
					}

					//Stop sound
					else if(loop_mode == 2)
					{
						apu_stat.channel[id].playing = false;
						apu_stat.channel[id].cnt &= ~0x80000000;
					}
				}

			}

			else { stream[x] += (-32768 * vol); }

			samples_played++;
		}

		//Generate silence if sound has run out of samples or is not playing
		else { stream[x] += (-32768 * vol); }
	}

	//Advance data pointer to sound samples
	switch(format)
	{
		case 0x0:
			apu_stat.channel[id].data_pos += (sample_ratio * samples_played);
			break;

		case 0x1:
			apu_stat.channel[id].data_pos += (sample_ratio * samples_played);
			apu_stat.channel[id].data_pos &= ~0x1;
			break;

		case 0x2:
			apu_stat.channel[id].adpcm_pos += (sample_ratio * samples_played);
			break;
	} 
}

/****** Decodes IMA-ADPCM data into samples, stores into channel buffer ******/
void NTR_APU::decode_adpcm_samples(u8 id)
{
	//Clear channel buffer and reset flag
	apu_stat.channel[id].adpcm_buffer.clear();
	apu_stat.channel[id].decode_adpcm = false;

	u32 current_pos = 0;
	u8 half_byte = 0;
	u8 full_byte = 0;
	u32 diff = 0;
	s32 high_result, low_result = 0;
	s16 next_index;

	//Decode IMA-ADPCM from memory
	while(current_pos < apu_stat.channel[id].samples)
	{
		//Grab data from memory, 1 byte at a time for every 2 samples
		//Also determine if current sample uses upper or lower half of byte from memory
		if((current_pos & 0x1) == 0)
		{
			full_byte = mem->memory_map[apu_stat.channel[id].data_src + (current_pos >> 1)];
			half_byte = (full_byte & 0xF);
		}

		else { half_byte = ((full_byte >> 4) & 0xF); }

		//Calculate difference
		diff = (apu_stat.adpcm_table[apu_stat.channel[id].adpcm_index] / 8);

		if(half_byte & 0x1) { diff += (apu_stat.adpcm_table[apu_stat.channel[id].adpcm_index] / 4); }
		if(half_byte & 0x2) { diff += (apu_stat.adpcm_table[apu_stat.channel[id].adpcm_index] / 2); }
		if(half_byte & 0x4) { diff += apu_stat.adpcm_table[apu_stat.channel[id].adpcm_index]; }

		high_result = apu_stat.channel[id].adpcm_val + diff;
		if(high_result > 32767) { high_result = 32767; }

		low_result = apu_stat.channel[id].adpcm_val - diff;
		if(low_result < -32768) { low_result = -32768; }

		if(half_byte & 0x8) { apu_stat.channel[id].adpcm_val = high_result; }
		else { apu_stat.channel[id].adpcm_val = low_result; }

		//Calculate next index
		next_index = apu_stat.channel[id].adpcm_index + apu_stat.index_table[half_byte & 0x7];
		if(next_index > 88) { next_index = 88; }
		else if(next_index < 0) { next_index = 0; }

		//Set next index and push sample to buffer
		apu_stat.channel[id].adpcm_index = next_index;
		apu_stat.channel[id].adpcm_buffer.push_back(apu_stat.channel[id].adpcm_val);

		current_pos++;
	}
}	

/****** SDL Audio Callback ******/ 
void ntr_audio_callback(void* _apu, u8 *_stream, int _length)
{
	s16* stream = (s16*) _stream;
	int length = _length/2;
	std::vector<s32> channel_stream(length);

	NTR_APU* apu_link = (NTR_APU*) _apu;

	//Generate samples
	for(u32 x = 0; x < 16; x++)
	{
		//Decode IMA-ADPCM samples first
		if(apu_link->apu_stat.channel[x].decode_adpcm) { apu_link->decode_adpcm_samples(x); }

		//Grab samples
		apu_link->generate_channel_samples(&channel_stream[0], length, x);
	}

	//Custom software mixing
	for(u32 x = 0; x < length; x++)
	{
		channel_stream[x] /= 16;
		stream[x] = channel_stream[x];
	}
}
