// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : apu.h
// Date : May, 2015
// Description : Game Boy Advance APU emulation
//
// Sets up SDL audio for mixing
// Generates and mixes samples for the GB's 4 sound channels 

#include <cmath>

#include "apu.h"

/****** APU Constructor ******/
DMG_APU::DMG_APU()
{
	reset();
}

/****** APU Destructor ******/
DMG_APU::~DMG_APU()
{
	SDL_CloseAudio();
	std::cout<<"APU::Shutdown\n";
}

/****** Reset APU ******/
void DMG_APU::reset()
{
	SDL_CloseAudio();

	apu_stat.sound_on = false;
	apu_stat.stereo = false;

	apu_stat.sample_rate = config::sample_rate;
	apu_stat.main_volume = 4;

	apu_stat.channel_master_volume = (config::volume >> 2);
	apu_stat.channel_left_volume = 0.0;
	apu_stat.channel_right_volume = 0.0;

	//Reset Channel 1-4 data
	for(int x = 0; x < 4; x++)
	{
		apu_stat.channel[x].raw_frequency = 0;
		apu_stat.channel[x].output_frequency = 0.0;

		apu_stat.channel[x].duration = 5000;
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

		apu_stat.channel[x].so1_output = false;
		apu_stat.channel[x].so2_output = false;
	}

	apu_stat.waveram_sample = 0;

	apu_stat.noise_dividing_ratio = 1;
	apu_stat.noise_prescalar = 1;
	apu_stat.noise_stages = 0;
	apu_stat.noise_7_stage_lsfr = 0x40;
	apu_stat.noise_15_stage_lsfr = 0x4000;
}

/****** Initialize APU with SDL ******/
bool DMG_APU::init()
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
    	desired_spec.samples = 1024;
    	desired_spec.callback = dmg_audio_callback;
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
		apu_stat.channel_master_volume = (config::volume >> 2);

		SDL_PauseAudio(0);
		std::cout<<"APU::Initialized\n";
		return true;
	}
}

/****** Read APU data from save state ******/
bool DMG_APU::apu_read(u32 offset, std::string filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary);
	
	if(!file.is_open()) { return false; }

	//Go to offset
	file.seekg(offset);

	//Serialize APU data from file stream
	file.read((char*)&apu_stat, sizeof(apu_stat));

	file.close();

	//Sanitize APU data
	if(apu_stat.noise_prescalar == 0) { apu_stat.noise_prescalar = 1; }
	if(apu_stat.noise_dividing_ratio == 0) { apu_stat.noise_dividing_ratio = 0.5; }

	if(apu_stat.channel[0].output_frequency == 0) { apu_stat.channel[0].output_frequency = 1.0; }
	if(apu_stat.channel[1].output_frequency == 0) { apu_stat.channel[1].output_frequency = 1.0; }
	if(apu_stat.channel[2].output_frequency == 0) { apu_stat.channel[2].output_frequency = 1.0; }
	if(apu_stat.channel[3].output_frequency == 0) { apu_stat.channel[3].output_frequency = 1.0; }

	if(apu_stat.channel[0].sample_length < 0) { apu_stat.channel[0].sample_length = 0; }
	if(apu_stat.channel[1].sample_length < 0) { apu_stat.channel[1].sample_length = 0; }
	if(apu_stat.channel[2].sample_length < 0) { apu_stat.channel[2].sample_length = 0; }
	if(apu_stat.channel[3].sample_length < 0) { apu_stat.channel[3].sample_length = 0; }

	apu_stat.channel[0].raw_frequency &= 0x7FF;
	apu_stat.channel[1].raw_frequency &= 0x7FF;
	apu_stat.channel[2].raw_frequency &= 0x7FF;
	apu_stat.channel[3].raw_frequency &= 0x7FF;

	return true;
}

/****** Write APU data to save state ******/
bool DMG_APU::apu_write(std::string filename)
{
	std::ofstream file(filename.c_str(), std::ios::binary | std::ios::app);
	
	if(!file.is_open()) { return false; }

	//Serialize APU data to file stream
	file.write((char*)&apu_stat, sizeof(apu_stat));

	file.close();
	return true;
}

/****** Gets the size of APU data for serialization ******/
u32 DMG_APU::size() { return sizeof(apu_stat); }

/******* Generate samples for GB sound channel 1 ******/
void DMG_APU::generate_channel_1_samples(s16* stream, int length)
{
	bool output_status = false;
	if((apu_stat.channel[0].so1_output) || (apu_stat.channel[0].so2_output)) { output_status = true; }

	//Generate samples from the last output of the channel
	if((apu_stat.channel[0].playing) && (apu_stat.sound_on) && (output_status))
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
						if((apu_stat.channel[0].sweep_shift >= 1) || (apu_stat.channel[0].raw_frequency >= 0x400))
						{
							pre_calc = (apu_stat.channel[0].raw_frequency >> apu_stat.channel[0].sweep_shift);
						}

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
							mem->memory_map[NR13] = (apu_stat.channel[0].raw_frequency & 0xFF);
							mem->memory_map[NR14] &= ~0x7;
							mem->memory_map[NR14] |= ((apu_stat.channel[0].raw_frequency >> 8) & 0x7);
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
							mem->memory_map[NR13] = (apu_stat.channel[0].raw_frequency & 0xFF);
							mem->memory_map[NR14] &= ~0x7;
							mem->memory_map[NR14] |= ((apu_stat.channel[0].raw_frequency >> 8) & 0x7);
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
					stream[x] = -32768 + (4369 * apu_stat.channel[0].volume);
				}

				//Generate low wave form if duty cycle is off OR volume is muted
				else { stream[x] = -32768; }
			}

			//Continuously generate sound if necessary
			else if((apu_stat.channel[0].sample_length == 0) && (!apu_stat.channel[0].length_flag)) { apu_stat.channel[0].sample_length = (apu_stat.channel[0].duration * apu_stat.sample_rate)/1000; }

			//Or stop sound after duration has been met, reset Sound 1 On Flag
			else if((apu_stat.channel[0].sample_length == 0) && (apu_stat.channel[0].length_flag)) 
			{ 
				stream[x] = -32768; 
				apu_stat.channel[0].sample_length = 0; 
				apu_stat.channel[0].playing = false; 
			}
		}
	}

	//Otherwise, generate silence
	else 
	{
		for(int x = 0; x < length; x++) { stream[x] = -32768; }
	}
}

/******* Generate samples for GB sound channel 2 ******/
void DMG_APU::generate_channel_2_samples(s16* stream, int length)
{
	bool output_status = false;
	if((apu_stat.channel[1].so1_output) || (apu_stat.channel[1].so2_output)) { output_status = true; }

	//Generate samples from the last output of the channel
	if((apu_stat.channel[1].playing) && (apu_stat.sound_on) && (output_status))
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
					stream[x] = -32768 + (4369 * apu_stat.channel[1].volume);
				}

				//Generate low wave form if duty cycle is off OR volume is muted
				else { stream[x] = -32768; }
			}

			//Continuously generate sound if necessary
			else if((apu_stat.channel[1].sample_length == 0) && (!apu_stat.channel[1].length_flag)) { apu_stat.channel[1].sample_length = (apu_stat.channel[1].duration * apu_stat.sample_rate)/1000; }

			//Or stop sound after duration has been met, reset Sound 2 On Flag
			else if((apu_stat.channel[1].sample_length == 0) && (apu_stat.channel[1].length_flag)) 
			{ 
				stream[x] = -32768; 
				apu_stat.channel[1].sample_length = 0; 
				apu_stat.channel[1].playing = false; 
			}
		}
	}

	//Otherwise, generate silence
	else 
	{
		for(int x = 0; x < length; x++) { stream[x] = -32768; }
	}
}

/******* Generate samples for GB sound channel 3 ******/
void DMG_APU::generate_channel_3_samples(s16* stream, int length)
{
	bool output_status = false;
	if((apu_stat.channel[2].so1_output) || (apu_stat.channel[2].so2_output)) { output_status = true; }
	if((output_status) && (mem->memory_map[NR30] & 0x80)) { output_status = true; }
	else { output_status = false; }

	//Generate samples from the last output of the channel
	if((apu_stat.channel[2].playing) && (apu_stat.sound_on) && (output_status))
	{
		//Determine amount of samples per waveform sample
		double wave_step = (apu_stat.sample_rate/apu_stat.channel[2].output_frequency) / 32.0;

		//Generate silence if samples per waveform sample is zero
		if(wave_step == 0.0) 
		{ 
			for(int x = 0; x < length; x++) { stream[x] = -32768; }
			return;
		}

		int frequency_samples = apu_stat.sample_rate/apu_stat.channel[2].output_frequency;

		for(int x = 0; x < length; x++, apu_stat.channel[2].sample_length--)
		{
			if(apu_stat.channel[2].sample_length > 0)
			{
				apu_stat.channel[2].frequency_distance++;
				
				//Reset frequency distance
				if(apu_stat.channel[2].frequency_distance >= frequency_samples) { apu_stat.channel[2].frequency_distance = 0; }

				//Determine which step in the waveform the current sample corresponds to
				u8 step = int(floor(apu_stat.channel[2].frequency_distance/wave_step));

				//Grab wave RAM sample data for even samples
				if(step % 2 == 0)
				{
					step >>= 1;
					apu_stat.waveram_sample = mem->memory_map[0xFF30 + step] >> 4;
					apu_stat.waveram_sample >>= apu_stat.channel[2].volume;
					stream[x] = -32768 + (4369 * apu_stat.waveram_sample);
				}

				//Grab wave RAM step data for odd steps
				else
				{
					step >>= 1;
					apu_stat.waveram_sample = mem->memory_map[0xFF30 + step] & 0xF;
					apu_stat.waveram_sample >>= apu_stat.channel[2].volume;
					stream[x] = -32768 + (4369 * apu_stat.waveram_sample);
				}
			}

			//Continuously generate sound if necessary
			else if((apu_stat.channel[2].sample_length == 0) && (!apu_stat.channel[2].length_flag)) { apu_stat.channel[2].sample_length = (apu_stat.channel[2].duration * apu_stat.sample_rate)/1000; }

			//Or stop sound after duration has been met, reset Sound 3 On Flag
			else if((apu_stat.channel[2].sample_length == 0) && (apu_stat.channel[2].length_flag)) 
			{ 
				stream[x] = -32768; 
				apu_stat.channel[2].sample_length = 0; 
				apu_stat.channel[2].playing = false; 
			}
		}
	}

	//Otherwise, generate silence
	else 
	{
		for(int x = 0; x < length; x++) { stream[x] = -32768; }
	}
}

/******* Generate samples for GB sound channel 4 ******/
void DMG_APU::generate_channel_4_samples(s16* stream, int length)
{
	bool output_status = false;
	if((apu_stat.channel[3].so1_output) || (apu_stat.channel[3].so2_output)) { output_status = true; }

	//Generate samples from the last output of the channel
	if((apu_stat.channel[3].playing) && (apu_stat.sound_on) && (output_status))
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
					stream[x] = -32768 + (4369 * apu_stat.channel[3].volume); 
				}

				else if((apu_stat.noise_stages == 7) && (apu_stat.noise_7_stage_lsfr & 0x1) && (apu_stat.channel[3].volume >= 1)) 
				{ 
					stream[x] = -32768 + (4369 * apu_stat.channel[3].volume); 
				}

				//Or generate low wave
				else { stream[x] = -32768; }
			}

			//Continuously generate sound if necessary
			else if((apu_stat.channel[3].sample_length == 0) && (!apu_stat.channel[3].length_flag)) 
			{
				apu_stat.channel[3].sample_length = (apu_stat.channel[3].duration * apu_stat.sample_rate)/1000;
			}

			//Or stop sound after duration has been met, reset Sound 4 On Flag
			else if((apu_stat.channel[3].sample_length == 0) && (apu_stat.channel[3].length_flag))
			{
				stream[x] = -32768;
				apu_stat.channel[3].sample_length = 0;
				apu_stat.channel[3].playing = false;
			}
		}
	}

	//Otherwise, generate silence
	else 
	{
		for(int x = 0; x < length; x++) { stream[x] = -32768; }
	}
}

/****** SDL Audio Callback ******/ 
void dmg_audio_callback(void* _apu, u8 *_stream, int _length)
{
	s16* stream = (s16*) _stream;
	int length = _length/2;

	s16 channel_1_stream[length];
	s16 channel_2_stream[length];
	s16 channel_3_stream[length];
	s16 channel_4_stream[length];

	DMG_APU* apu_link = (DMG_APU*) _apu;
	apu_link->generate_channel_1_samples(channel_1_stream, length);
	apu_link->generate_channel_2_samples(channel_2_stream, length);
	apu_link->generate_channel_3_samples(channel_3_stream, length);
	apu_link->generate_channel_4_samples(channel_4_stream, length);

	double volume_ratio = apu_link->apu_stat.channel_master_volume / 128.0;

	//Custom software mixing
	for(u32 x = 0; x < length; x++)
	{
		s32 out_sample = channel_1_stream[x] + channel_2_stream[x] + channel_3_stream[x] + channel_4_stream[x];
		out_sample *= volume_ratio;
		out_sample *= apu_link->apu_stat.channel_left_volume;
		out_sample /= 4;

		stream[x] = out_sample;
	} 
}
