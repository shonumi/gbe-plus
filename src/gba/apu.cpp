// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : apu.h
// Date : March 05, 2015
// Description : Game Boy Advance APU emulation
//
// Sets up SDL audio for mixing
// Generates and mixes samples for the GBA's 4 sound channels + DMA channels  

#include <cmath>

#include "apu.h"

/****** APU Constructor ******/
AGB_APU::AGB_APU()
{
	reset();
}

/****** APU Destructor ******/
AGB_APU::~AGB_APU()
{
	SDL_CloseAudio();
	std::cout<<"APU::Shutdown\n";
}

/****** Reset APU ******/
void AGB_APU::reset()
{
	SDL_CloseAudio();

	apu_stat.psg_needs_fill = true;
	apu_stat.psg_fill_rate = 0;

	apu_stat.sound_on = false;
	apu_stat.stereo = false;

	apu_stat.sample_rate = config::sample_rate;
	apu_stat.main_volume = 4;

	apu_stat.channel_master_volume = config::volume;
	apu_stat.channel_left_volume = 0.0;
	apu_stat.channel_right_volume = 0.0;

	apu_stat.dma_left_volume = 0;
	apu_stat.dma_right_volume = 0;

	//Reset Channel 1-4 data
	for(int x = 0; x < 4; x++)
	{
		apu_stat.channel[x].raw_frequency = 0;
		apu_stat.channel[x].output_frequency = 0.0;

		apu_stat.channel[x].duration = apu_stat.sample_rate;
		apu_stat.channel[x].volume = 0;

		apu_stat.channel[x].playing = false;
		apu_stat.channel[x].enable = false;
		apu_stat.channel[x].left_enable = false;
		apu_stat.channel[x].right_enable = false;
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

		apu_stat.channel[x].current_index = 0;
		apu_stat.channel[x].last_index = 0;
		apu_stat.channel[x].buffer_size = 0;

		for(int y = 0; y < 0x10000; y++) { apu_stat.channel[x].buffer[y] = -32768; }
	}

	apu_stat.waveram_bank_play = 0;
	apu_stat.waveram_bank_rw = 1;
	apu_stat.waveram_sample = 0;
	apu_stat.waveram_size = 0;

	apu_stat.noise_dividing_ratio = 1;
	apu_stat.noise_prescalar = 1;
	apu_stat.noise_stages = 0;
	apu_stat.noise_7_stage_lsfr = 0x40;
	apu_stat.noise_15_stage_lsfr = 0x4000;

	//Clear waveram data
	for(int x = 0; x < 0x20; x++) { apu_stat.waveram_data[x] = 0; }

	//Reset DMA channel data
	for(int x = 0; x < 2; x++)
	{
		apu_stat.dma[x].output_frequency = 0;
		apu_stat.dma[x].last_position = 0;
		apu_stat.dma[x].counter = 0;
		apu_stat.dma[x].length = 0;
		apu_stat.dma[x].timer = 0;
		apu_stat.dma[x].master_volume = config::volume;

		apu_stat.dma[x].playing = false;
		apu_stat.dma[x].enable = false;
		apu_stat.dma[x].right_enable = false;
		apu_stat.dma[x].left_enable = false;
	}

	//Reset DMA FIFO buffers
	for(int x = 0; x < 0x10000; x++) 
	{
		apu_stat.dma[0].buffer[x] = -127;
		apu_stat.dma[1].buffer[x] = -127;
	}
}

/****** Initialize APU with SDL ******/
bool AGB_APU::init()
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
    	desired_spec.callback = agb_audio_callback;
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
		apu_stat.dma[0].master_volume = config::volume;
		apu_stat.dma[1].master_volume = config::volume;

		apu_stat.psg_fill_rate = apu_stat.sample_rate / 60;

		SDL_PauseAudio(0);
		std::cout<<"APU::Initialized\n";
		return true;
	}
}

/******* Generate samples for GBA sound channel 1 ******/
void AGB_APU::generate_channel_1_samples(s16* stream, int length)
{
	//Determine if more data needs to be buffered
	while(apu_stat.channel[0].buffer_size < length)
	{
		buffer_channel_1();
		apu_stat.psg_needs_fill = false;
	}

	//Copy from last position in the buffer
	for(int x = 0; x < length; x++)
	{
		stream[x] = apu_stat.channel[0].buffer[apu_stat.channel[0].last_index++];
	}

	apu_stat.channel[0].buffer_size -= length;

	//Drain buffer if it gets too large
	if(apu_stat.channel[0].buffer_size >= 512)
	{
		apu_stat.channel[0].buffer_size = 0;
		apu_stat.channel[0].last_index = 0;
		apu_stat.channel[0].current_index = 0;
	}
}

/******* Generate samples for GBA sound channel 2 ******/
void AGB_APU::generate_channel_2_samples(s16* stream, int length)
{
	//Determine if more data needs to be buffered
	while(apu_stat.channel[1].buffer_size < length)
	{
		buffer_channel_2();
		apu_stat.psg_needs_fill = false;
	}

	//Copy from last position in the buffer
	for(int x = 0; x < length; x++)
	{
		stream[x] = apu_stat.channel[1].buffer[apu_stat.channel[1].last_index++];
	}

	apu_stat.channel[1].buffer_size -= length;

	//Drain buffer if it gets too large
	if(apu_stat.channel[1].buffer_size >= 512)
	{
		apu_stat.channel[1].buffer_size = 0;
		apu_stat.channel[1].last_index = 0;
		apu_stat.channel[1].current_index = 0;
	}
}

/******* Generate samples for GBA sound channel 3 ******/
void AGB_APU::generate_channel_3_samples(s16* stream, int length)
{
	//Determine if more data needs to be buffered
	while(apu_stat.channel[2].buffer_size < length)
	{
		buffer_channel_3();
		apu_stat.psg_needs_fill = false;
	}

	//Copy from last position in the buffer
	for(int x = 0; x < length; x++)
	{
		stream[x] = apu_stat.channel[2].buffer[apu_stat.channel[2].last_index++];
	}

	apu_stat.channel[2].buffer_size -= length;

	//Drain buffer if it gets too large
	if(apu_stat.channel[2].buffer_size >= 512)
	{
		apu_stat.channel[2].buffer_size = 0;
		apu_stat.channel[2].last_index = 0;
		apu_stat.channel[2].current_index = 0;
	}
}

/******* Generate samples for GBA sound channel 4 ******/
void AGB_APU::generate_channel_4_samples(s16* stream, int length)
{
	//Determine if more data needs to be buffered
	while(apu_stat.channel[3].buffer_size < length)
	{
		buffer_channel_4();
		apu_stat.psg_needs_fill = false;
	}

	//Copy from last position in the buffer
	for(int x = 0; x < length; x++)
	{
		stream[x] = apu_stat.channel[3].buffer[apu_stat.channel[3].last_index++];
	}

	apu_stat.channel[3].buffer_size -= length;

	//Drain buffer if it gets too large
	if(apu_stat.channel[3].buffer_size >= 512)
	{
		apu_stat.channel[3].buffer_size = 0;
		apu_stat.channel[3].last_index = 0;
		apu_stat.channel[3].current_index = 0;
	}
}

/******* Generate samples for GBA DMA channel A ******/
void AGB_APU::generate_dma_a_samples(s16* stream, int length)
{
	//Generate samples from the last output of the channel
	if((apu_stat.dma[0].left_enable || apu_stat.dma[0].right_enable) && (apu_stat.dma[0].length != 0))
	{
		double sample_ratio = apu_stat.dma[0].output_frequency/apu_stat.sample_rate;
		u8 buffer_sample = 0;
		s8 buffer_output = 0;
		u16 buffer_pos = 0;

		for(int x = 0; x < length; x++)
		{
			if((sample_ratio * x) < apu_stat.dma[0].length) { buffer_pos = apu_stat.dma[0].last_position + (sample_ratio * x); }
			buffer_sample = apu_stat.dma[0].buffer[buffer_pos];
			
			if(buffer_sample & 0x80)
			{
				buffer_sample--;
				buffer_sample = ~buffer_sample;
				buffer_output = 0 - buffer_sample;
			}

			else { buffer_output = buffer_sample; }
			

			//Scale S8 audio to S16
			stream[x] = buffer_output * 256;
		}

		//Reset DMA channel A buffer
		apu_stat.dma[0].counter = apu_stat.dma[0].last_position = buffer_pos;
	}

	//Otherwise, generate silence
	else 
	{
		for(int x = 0; x < length; x++) { stream[x] = -32768; }
		apu_stat.dma[0].counter = apu_stat.dma[0].last_position = 0;
	}

	if(apu_stat.dma[0].length > length) { apu_stat.dma[0].length -= length; }
	else { apu_stat.dma[0].length = 0; }
}

/******* Generate samples for GBA DMA channel B ******/
void AGB_APU::generate_dma_b_samples(s16* stream, int length)
{
	//Generate samples from the last output of the channel
	if((apu_stat.dma[1].left_enable || apu_stat.dma[1].right_enable) && (apu_stat.dma[1].length != 0))
	{
		double sample_ratio = apu_stat.dma[1].output_frequency/apu_stat.sample_rate;
		u8 buffer_sample = 0;
		s8 buffer_output = 0;
		u16 buffer_pos = 0;

		for(int x = 0; x < length; x++)
		{
			if((sample_ratio * x) < apu_stat.dma[1].length) { buffer_pos = apu_stat.dma[1].last_position + (sample_ratio * x); }
			buffer_sample = apu_stat.dma[1].buffer[buffer_pos];
			
			if(buffer_sample & 0x80)
			{
				buffer_sample--;
				buffer_sample = ~buffer_sample;
				buffer_output = 0 - buffer_sample;
			}

			else { buffer_output = buffer_sample; }
			

			//Scale S8 audio to S16
			stream[x] = buffer_output * 256;
		}

		//Reset DMA channel A buffer
		apu_stat.dma[1].counter = apu_stat.dma[1].last_position = buffer_pos;
	}

	//Otherwise, generate silence
	else 
	{
		for(int x = 0; x < length; x++) { stream[x] = -32768; }
		apu_stat.dma[1].counter = apu_stat.dma[1].last_position = 0;
	}

	if(apu_stat.dma[1].length > length) { apu_stat.dma[1].length -= length; }
	else { apu_stat.dma[1].length = 0; }
}	

/****** SDL Audio Callback ******/ 
void agb_audio_callback(void* _apu, u8 *_stream, int _length)
{
	s16* stream = (s16*) _stream;
	int length = _length/2;

	s16 channel_1_stream[length];
	s16 channel_2_stream[length];
	s16 channel_3_stream[length];
	s16 channel_4_stream[length];
	s16 dma_a_stream[length];
	s16 dma_b_stream[length];

	AGB_APU* apu_link = (AGB_APU*) _apu;
	apu_link->generate_channel_1_samples(channel_1_stream, length);
	apu_link->generate_channel_2_samples(channel_2_stream, length);
	apu_link->generate_channel_3_samples(channel_3_stream, length);
	apu_link->generate_channel_4_samples(channel_4_stream, length);
	apu_link->generate_dma_a_samples(dma_a_stream, length);
	apu_link->generate_dma_b_samples(dma_b_stream, length);

	double channel_ratio = apu_link->apu_stat.channel_master_volume / 128.0;
	double dma_a_ratio = apu_link->apu_stat.dma[0].master_volume / 128.0;
	double dma_b_ratio = apu_link->apu_stat.dma[1].master_volume / 128.0;

	//Custom software mixing
	for(u32 x = 0; x < length; x++)
	{
		//Add Sound Channels 1-4 and multiply by volume ratio
		s32 out_sample = (channel_1_stream[x] + channel_2_stream[x] + channel_3_stream[x] + channel_4_stream[x]) * channel_ratio;

		//Add DMA Channels A and B and multiply by volume ratio
		out_sample += (dma_a_stream[x] * dma_a_ratio) + (dma_b_stream[x] * dma_b_ratio);

		//Divide final wave by total amount of channels
		out_sample /= 6;

		stream[x] = out_sample;
	} 
}

/****** Fill PSG channels with audio data when buffering ******/
void AGB_APU::buffer_channels()
{
	buffer_channel_1();
	buffer_channel_2();
	buffer_channel_3();
	buffer_channel_4();
}

/****** Buffer GBA Channel 1 data ******/
void AGB_APU::buffer_channel_1()
{
	int length = apu_stat.psg_fill_rate;
	apu_stat.channel[0].buffer_size += length;

	//Generate samples from the last output of the channel
	if((apu_stat.channel[0].playing) && (apu_stat.channel[0].left_enable || apu_stat.channel[0].right_enable))
	{
		int frequency_samples = apu_stat.sample_rate/apu_stat.channel[0].output_frequency;

		for(int x = 0; x < length; x++, apu_stat.channel[0].sample_length--)
		{
			//Process audio sweep
			if(apu_stat.channel[0].sweep_time >= 1)
			{
				apu_stat.channel[0].sweep_counter++;

				if(apu_stat.channel[0].sweep_counter >= ((apu_stat.sample_rate/128) * apu_stat.channel[0].sweep_time))
				{
					int pre_calc = 0;

					//Increase frequency
					if(apu_stat.channel[0].sweep_direction == 0)
					{
						if(apu_stat.channel[0].sweep_shift >= 1) { pre_calc = (apu_stat.channel[0].raw_frequency >> apu_stat.channel[0].sweep_shift); }

						//When frequency is greater than 131KHz, stop sound
						if((apu_stat.channel[0].raw_frequency + pre_calc) >= 0x800) 
						{ 
							apu_stat.channel[0].volume = apu_stat.channel[0].sweep_shift = apu_stat.channel[0].envelope_step = apu_stat.channel[0].sweep_time = 0; 
							apu_stat.channel[0].playing = false; 
						}

						else 
						{ 
							apu_stat.channel[0].raw_frequency += pre_calc;
							apu_stat.channel[0].output_frequency = 131072.0/(2048 - apu_stat.channel[0].raw_frequency);
							mem->memory_map[SND1CNT_X] = (apu_stat.channel[0].raw_frequency & 0xFF);
							mem->memory_map[SND1CNT_X+1] &= ~0x7;
							mem->memory_map[SND1CNT_X+1] |= ((apu_stat.channel[0].raw_frequency >> 8) & 0x7);
						}
					}

						//Decrease frequency
						else if(apu_stat.channel[0].sweep_direction == 1)
						{
							if(apu_stat.channel[0].sweep_shift >= 1) { pre_calc = (apu_stat.channel[0].raw_frequency >> apu_stat.channel[0].sweep_shift); }

							//Only sweep down when result of frequency change is greater than zero
							if((apu_stat.channel[0].raw_frequency - pre_calc) >= 0) 
							{ 
								apu_stat.channel[0].raw_frequency -= pre_calc;
								apu_stat.channel[0].output_frequency = 131072.0/(2048 - apu_stat.channel[0].raw_frequency);
								mem->memory_map[SND1CNT_X] = (apu_stat.channel[0].raw_frequency & 0xFF);
								mem->memory_map[SND1CNT_X+1] &= ~0x7;
								mem->memory_map[SND1CNT_X+1] |= ((apu_stat.channel[0].raw_frequency >> 8) & 0x7);
							}
						}

						apu_stat.channel[0].sweep_counter = 0;
					}
				} 

			//Process audio envelope
			if(apu_stat.channel[0].envelope_step >= 1)
			{
				apu_stat.channel[0].envelope_counter++;

				if(apu_stat.channel[0].envelope_counter >= ((apu_stat.sample_rate/64) * apu_stat.channel[0].envelope_step)) 
				{		
					//Decrease volume
					if((apu_stat.channel[0].envelope_direction == 0) && (apu_stat.channel[0].volume >= 1)) { apu_stat.channel[0].volume--; }
				
					//Increase volume
					else if((apu_stat.channel[0].envelope_direction == 1) && (apu_stat.channel[0].volume < 0xF)) { apu_stat.channel[0].volume++; }

					apu_stat.channel[0].envelope_counter = 0;
				}
			}

			//Process audio waveform
			if(apu_stat.channel[0].sample_length > 0)
			{
				apu_stat.channel[0].frequency_distance++;

				//Reset frequency distance
				if(apu_stat.channel[0].frequency_distance >= frequency_samples) { apu_stat.channel[0].frequency_distance = 0; }
		
				//Generate high wave form if duty cycle is on AND volume is not muted
				if((apu_stat.channel[0].frequency_distance >= (frequency_samples/8) * apu_stat.channel[0].duty_cycle_start) 
				&& (apu_stat.channel[0].frequency_distance < (frequency_samples/8) * apu_stat.channel[0].duty_cycle_end)
				&& (apu_stat.channel[0].volume != 0))
				{
					apu_stat.channel[0].buffer[apu_stat.channel[0].current_index++] = -32768 + (apu_stat.channel_right_volume * apu_stat.channel[0].volume);
				}

				//Generate low wave form if duty cycle is off OR volume is muted
				else { apu_stat.channel[0].buffer[apu_stat.channel[0].current_index++] = -32768; }
			}

			//Continuously generate sound if necessary
			else if((apu_stat.channel[0].sample_length == 0) && (!apu_stat.channel[0].length_flag)) { apu_stat.channel[0].sample_length = apu_stat.sample_rate; }

			//Or stop sound after duration has been met, reset Sound 1 On Flag
			else if((apu_stat.channel[0].sample_length == 0) && (apu_stat.channel[0].length_flag)) 
			{ 
				apu_stat.channel[0].buffer[apu_stat.channel[0].current_index++] = -32768; 
				apu_stat.channel[0].sample_length = 0; 
				apu_stat.channel[0].playing = false; 
			}
		}
	}

	//Otherwise, generate silence
	else 
	{
		for(int x = 0; x < length; x++) { apu_stat.channel[0].buffer[apu_stat.channel[0].current_index++] = -32768; }
	}
}

/****** Buffer GBA Channel 2 data ******/
void AGB_APU::buffer_channel_2()
{
	int length = apu_stat.psg_fill_rate;
	apu_stat.channel[1].buffer_size += length;

	//Generate samples from the last output of the channel
	if((apu_stat.channel[1].playing) && (apu_stat.channel[1].left_enable || apu_stat.channel[1].right_enable))
	{
		int frequency_samples = apu_stat.sample_rate/apu_stat.channel[1].output_frequency;

		for(int x = 0; x < length; x++, apu_stat.channel[1].sample_length--)
		{
			//Process audio envelope
			if(apu_stat.channel[1].envelope_step >= 1)
			{
				apu_stat.channel[1].envelope_counter++;

				if(apu_stat.channel[1].envelope_counter >= ((apu_stat.sample_rate/64) * apu_stat.channel[1].envelope_step)) 
				{		
					//Decrease volume
					if((apu_stat.channel[1].envelope_direction == 0) && (apu_stat.channel[1].volume >= 1)) { apu_stat.channel[1].volume--; }
				
					//Increase volume
					else if((apu_stat.channel[1].envelope_direction == 1) && (apu_stat.channel[1].volume < 0xF)) { apu_stat.channel[1].volume++; }

					apu_stat.channel[1].envelope_counter = 0;
				}
			}

			//Process audio waveform
			if(apu_stat.channel[1].sample_length > 0)
			{
				apu_stat.channel[1].frequency_distance++;

				//Reset frequency distance
				if(apu_stat.channel[1].frequency_distance >= frequency_samples) { apu_stat.channel[1].frequency_distance = 0; }
		
				//Generate high wave form if duty cycle is on AND volume is not muted
				if((apu_stat.channel[1].frequency_distance >= (frequency_samples/8) * apu_stat.channel[1].duty_cycle_start) 
				&& (apu_stat.channel[1].frequency_distance < (frequency_samples/8) * apu_stat.channel[1].duty_cycle_end)
				&& (apu_stat.channel[1].volume != 0))
				{
					apu_stat.channel[1].buffer[apu_stat.channel[1].current_index++] = -32768 + (apu_stat.channel_right_volume * apu_stat.channel[1].volume);
				}

				//Generate low wave form if duty cycle is off OR volume is muted
				else { apu_stat.channel[1].buffer[apu_stat.channel[1].current_index++] = -32768; }
			}

			//Continuously generate sound if necessary
			else if((apu_stat.channel[1].sample_length == 0) && (!apu_stat.channel[1].length_flag)) { apu_stat.channel[1].sample_length = apu_stat.sample_rate; }

			//Or stop sound after duration has been met, reset Sound 1 On Flag
			else if((apu_stat.channel[1].sample_length == 0) && (apu_stat.channel[1].length_flag)) 
			{ 
				apu_stat.channel[1].buffer[apu_stat.channel[1].current_index++] = -32768; 
				apu_stat.channel[1].sample_length = 0; 
				apu_stat.channel[1].playing = false; 
			}
		}
	}

	//Otherwise, generate silence
	else 
	{
		for(int x = 0; x < length; x++) { apu_stat.channel[1].buffer[apu_stat.channel[1].current_index++] = -32768; }
	}
}


/****** Buffer GBA Channel 3 data ******/
void AGB_APU::buffer_channel_3()
{
	int length = apu_stat.psg_fill_rate;
	apu_stat.channel[2].buffer_size += length;

	//Generate samples from the last output of the channel
	if((apu_stat.channel[2].playing) && (apu_stat.channel[2].enable) && (apu_stat.channel[2].left_enable || apu_stat.channel[2].right_enable))
	{
		double waveform_frequency = apu_stat.channel[2].output_frequency;
		if(apu_stat.waveram_size == 64) { waveform_frequency /= 2.0; }

		//Determine amount of samples per waveform sample
		double wave_step = (apu_stat.sample_rate/waveform_frequency) / apu_stat.waveram_size;

		//Generate silence if samples per waveform sample is zero
		if(wave_step == 0) 
		{ 
			for(int x = 0; x < length; x++) { apu_stat.channel[2].buffer[apu_stat.channel[2].current_index++] = -32768; }
			return;
		}

		int frequency_samples = apu_stat.sample_rate/waveform_frequency;

		for(int x = 0; x < length; x++, apu_stat.channel[2].sample_length--)
		{
			if(apu_stat.channel[2].sample_length > 0)
			{
				apu_stat.channel[2].frequency_distance++;
				
				//Reset frequency distance
				if(apu_stat.channel[2].frequency_distance >= frequency_samples) { apu_stat.channel[2].frequency_distance = 0; }

				//Determine which step in the waveform the current sample corresponds to
				u8 step = int(floor(apu_stat.channel[2].frequency_distance/wave_step)) % apu_stat.waveram_size;

				//Grab wave RAM sample data for even samples
				if(step % 2 == 0)
				{
					step >>= 1;

					if(apu_stat.waveram_size == 32) 
					{
						apu_stat.waveram_sample = apu_stat.waveram_data[(apu_stat.waveram_bank_play << 4) + step] >> 4;
					}

					else
					{
						apu_stat.waveram_sample = apu_stat.waveram_data[step] >> 4;
					}
	
					//Scale waveform to S16 audio stream
					switch(apu_stat.channel[2].volume)
					{
						case 0x0:
							apu_stat.channel[2].buffer[apu_stat.channel[2].current_index++] = -32768;
							break;

						case 0x1:
							apu_stat.channel[2].buffer[apu_stat.channel[2].current_index++] = -32768 + (apu_stat.channel_right_volume * apu_stat.waveram_sample);
							break;

						case 0x2:
							apu_stat.channel[2].buffer[apu_stat.channel[2].current_index++] = (-32768 + (apu_stat.channel_right_volume * apu_stat.waveram_sample)) * 0.5;
							break;

						case 0x3:
							apu_stat.channel[2].buffer[apu_stat.channel[2].current_index++] = (-32768 + (apu_stat.channel_right_volume * apu_stat.waveram_sample)) * 0.25;
							break;

						case 0x4:
							apu_stat.channel[2].buffer[apu_stat.channel[2].current_index++] = (-32768 + (apu_stat.channel_right_volume * apu_stat.waveram_sample)) * 0.75;
							break;

						default:
							apu_stat.channel[2].buffer[apu_stat.channel[2].current_index++] = -32768;
							break;
					}
				}

				//Grab wave RAM step data for odd steps
				else
				{
					step >>= 1;

					if(apu_stat.waveram_size == 32) 
					{
						apu_stat.waveram_sample = apu_stat.waveram_data[(apu_stat.waveram_bank_play << 4) + step] & 0xF;
					}

					else
					{
						apu_stat.waveram_sample = apu_stat.waveram_data[step] & 0xF;
					}
	
					//Scale waveform to S16 audio stream
					switch(apu_stat.channel[2].volume)
					{
						case 0x0:
							apu_stat.channel[2].buffer[apu_stat.channel[2].current_index++] = -32768;
							break;

						case 0x1:
							apu_stat.channel[2].buffer[apu_stat.channel[2].current_index++] = -32768 + (apu_stat.channel_right_volume * apu_stat.waveram_sample);
							break;

						case 0x2:
							apu_stat.channel[2].buffer[apu_stat.channel[2].current_index++] = (-32768 + (apu_stat.channel_right_volume * apu_stat.waveram_sample)) * 0.5;
							break;

						case 0x3:
							apu_stat.channel[2].buffer[apu_stat.channel[2].current_index++] = (-32768 + (apu_stat.channel_right_volume * apu_stat.waveram_sample)) * 0.25;
							break;

						case 0x4:
							apu_stat.channel[2].buffer[apu_stat.channel[2].current_index++] = (-32768 + (apu_stat.channel_right_volume * apu_stat.waveram_sample)) * 0.75;
							break;

						default:
							apu_stat.channel[2].buffer[apu_stat.channel[2].current_index++] = -32768;
							break;
					}
				}
			}

			//Continuously generate sound if necessary
			else if((apu_stat.channel[2].sample_length == 0) && (!apu_stat.channel[2].length_flag)) { apu_stat.channel[2].sample_length = apu_stat.sample_rate; }

			//Or stop sound after duration has been met, reset Sound 1 On Flag
			else if((apu_stat.channel[2].sample_length == 0) && (apu_stat.channel[2].length_flag)) 
			{ 
				apu_stat.channel[2].buffer[apu_stat.channel[2].current_index++] = -32768; 
				apu_stat.channel[2].sample_length = 0; 
				apu_stat.channel[2].playing = false; 
			}
		}
	}

	//Otherwise, generate silence
	else 
	{
		for(int x = 0; x < length; x++) { apu_stat.channel[2].buffer[apu_stat.channel[2].current_index++] = -32768; }
	}
}


/****** Buffer GBA Channel 4 data ******/
void AGB_APU::buffer_channel_4()
{
	int length = apu_stat.psg_fill_rate;
	apu_stat.channel[3].buffer_size += length;

	//Generate samples from the last output of the channel
	if((apu_stat.channel[3].playing) && (apu_stat.channel[3].left_enable || apu_stat.channel[3].right_enable))
	{
		double samples_per_freq = apu_stat.channel[3].output_frequency/apu_stat.sample_rate;
		double samples_per_freq_counter = 0;
		u32 lsfr_runs = 0;

		for(int x = 0; x < length; x++, apu_stat.channel[3].sample_length--)
		{
			if(apu_stat.channel[3].sample_length > 0)
			{
				apu_stat.channel[3].frequency_distance++;
				samples_per_freq_counter += samples_per_freq;

				//Process audio envelope
				if(apu_stat.channel[3].envelope_step >= 1)
				{
					apu_stat.channel[3].envelope_counter++;

					if(apu_stat.channel[3].envelope_counter >= ((apu_stat.sample_rate/64) * apu_stat.channel[3].envelope_step)) 
					{		
						//Decrease volume
						if((apu_stat.channel[3].envelope_direction == 0) && (apu_stat.channel[3].volume >= 1)) { apu_stat.channel[3].volume--; }
				
						//Increase volume
						else if((apu_stat.channel[3].envelope_direction == 1) && (apu_stat.channel[3].volume < 0xF)) { apu_stat.channel[3].volume++; }

						apu_stat.channel[3].envelope_counter = 0;
					}
				}

				//Determine how many times to run LSFR
				if(samples_per_freq_counter >= 1)
				{
					lsfr_runs = 0;
					while(samples_per_freq_counter >= 1)
					{
						samples_per_freq_counter -= 1.0;
						lsfr_runs++;
					}

					//Run LSFR
					for(int y = 0; y < lsfr_runs; y++)
					{
						//7-stage
						if(apu_stat.noise_stages == 7)
						{
							u8 bit_0 = (apu_stat.noise_7_stage_lsfr & 0x1) ? 1 : 0;
							u8 bit_1 = (apu_stat.noise_7_stage_lsfr & 0x2) ? 1 : 0;
							u8 result = bit_0 ^ bit_1;
							apu_stat.noise_7_stage_lsfr >>= 1;
							
							if(result == 1) { apu_stat.noise_7_stage_lsfr |= 0x40; }
						}

						//15-stage
						else if(apu_stat.noise_stages == 15)
						{
							u8 bit_0 = (apu_stat.noise_15_stage_lsfr & 0x1) ? 1 : 0;
							u8 bit_1 = (apu_stat.noise_15_stage_lsfr & 0x2) ? 1 : 0;
							u8 result = bit_0 ^ bit_1;
							apu_stat.noise_15_stage_lsfr >>= 1;
							
							if(result == 1) { apu_stat.noise_15_stage_lsfr |= 0x4000; }
						}
					}
				}

				//Generate high wave if LSFR returns 1 from first byte and volume is not muted
				if((apu_stat.noise_stages == 15) && (apu_stat.noise_15_stage_lsfr & 0x1) && (apu_stat.channel[3].volume >= 1)) 
				{ 
					apu_stat.channel[3].buffer[apu_stat.channel[3].current_index++] = -32768 + (apu_stat.channel_right_volume * apu_stat.channel[3].volume); 
				}

				else if((apu_stat.noise_stages == 7) && (apu_stat.noise_7_stage_lsfr & 0x1) && (apu_stat.channel[3].volume >= 1)) 
				{ 
					apu_stat.channel[3].buffer[apu_stat.channel[3].current_index++] = -32768 + (apu_stat.channel_right_volume * apu_stat.channel[3].volume); 
				}

				//Or generate low wave
				else { apu_stat.channel[3].buffer[apu_stat.channel[3].current_index++] = -32768; }
			}

			//Continuously generate sound if necessary
			else if(apu_stat.channel[3].sample_length == 0) { apu_stat.channel[3].sample_length = apu_stat.sample_rate; }

			//Or stop sound after duration has been met, reset Sound 4 On Flag
			else
			{
				apu_stat.channel[3].sample_length = 0;
				apu_stat.channel[3].buffer[apu_stat.channel[3].current_index++] = -32768;
				apu_stat.channel[3].playing = false;
			}
		}
	}

	//Otherwise, generate silence
	else 
	{
		for(int x = 0; x < length; x++) { apu_stat.channel[3].buffer[apu_stat.channel[3].current_index++] = -32768; }
	}
}

/****** Read APU data from save state ******/
bool AGB_APU::apu_read(u32 offset, std::string filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary);
	
	if(!file.is_open()) { return false; }

	//Go to offset
	file.seekg(offset);

	//Serialize APU data from save state
	file.read((char*)&apu_stat, sizeof(apu_stat));

	file.close();
	return true;
}

/****** Write APU data to save state ******/
bool AGB_APU::apu_write(std::string filename)
{
	std::ofstream file(filename.c_str(), std::ios::binary | std::ios::app);
	
	if(!file.is_open()) { return false; }

	//Serialize APU data to save state
	file.write((char*)&apu_stat, sizeof(apu_stat));

	file.close();
	return true;
}

/****** Gets the size of APU data for serialization ******/
u32 AGB_APU::size() { return sizeof(apu_stat); }
