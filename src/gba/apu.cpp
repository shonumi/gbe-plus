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
	//Always free external audio buffer, safe to call this function with NULL pointer!
	SDL_FreeWAV(apu_stat.ext_audio.buffer);
	SDL_FreeWAV(apu_stat.ext_audio.karaoke_buffer);

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
	apu_stat.mic_init = false;
	apu_stat.is_mic_on = false;
	apu_stat.is_recording = false;
	apu_stat.save_recording = false;

	apu_stat.sample_rate = config::sample_rate;
	apu_stat.main_volume = 4;

	apu_stat.channel_master_volume = (config::volume >> 2);
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

	apu_stat.ext_audio.frequency = 0;
	apu_stat.ext_audio.length = 0;
	apu_stat.ext_audio.sample_pos = 0;
	apu_stat.ext_audio.last_pos = 0;
	apu_stat.ext_audio.channels = 0;
	apu_stat.ext_audio.volume = 0;
	apu_stat.ext_audio.id = 0;
	apu_stat.ext_audio.set_count = 0;
	apu_stat.ext_audio.current_set = 0;
	apu_stat.ext_audio.buffer = NULL;
	apu_stat.ext_audio.playing = false;
	apu_stat.ext_audio.use_headphones = false;

	apu_stat.ext_audio.karaoke_buffer = NULL;
	apu_stat.ext_audio.karaoke_length = 0;

	mic_buffer.clear();
	apu_stat.mic_id = 0;
}

/****** Initialize APU with SDL ******/
bool AGB_APU::init()
{
	bool init_status = false;

	//Override SDL audio driver if necessary
	if(!config::override_audio_driver.empty())
	{
		std::string env_var = "SDL_AUDIODRIVER=" + config::override_audio_driver;
		putenv(const_cast<char*>(env_var.c_str()));
	}

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
    	desired_spec.samples = (config::sample_size) ? config::sample_size : 4096;
    	desired_spec.callback = agb_audio_callback;
    	desired_spec.userdata = this;

    	//Open SDL audio for desired specifications
	if(SDL_OpenAudio(&desired_spec, NULL) < 0) 
	{ 
		std::cout<<"APU::Failed to open audio\n";
		init_status = false;
	}

	else
	{
		apu_stat.channel_master_volume = config::volume;
		apu_stat.dma[0].master_volume = config::volume;
		apu_stat.dma[1].master_volume = config::volume;

		apu_stat.psg_fill_rate = apu_stat.sample_rate / 60;

		SDL_PauseAudio(0);
		init_status = true;
		std::cout<<"APU::Initialized\n";
	}

	//Open microphone if enabled and if possible
	if(config::use_microphone)
	{
		SDL_AudioSpec final_spec;
		SDL_AudioDeviceID mic_id = 0;
		s32 max_devices = SDL_GetNumAudioDevices(1);

		//Setup the desired audio specifications
    		microphone_spec.freq = apu_stat.sample_rate;
		microphone_spec.format = AUDIO_S16SYS;
    		microphone_spec.channels = 1;
    		microphone_spec.samples = (config::sample_size) ? config::sample_size : 1024;
    		microphone_spec.callback = agb_microphone_callback;
    		microphone_spec.userdata = this;

		for(u32 x = 0; x < max_devices; x++)
		{
			//Open recording device
			mic_id = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(x, 1), 1, &microphone_spec, &final_spec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

			if(mic_id != 0)
			{
				if((!config::microphone_id) || (config::microphone_id == mic_id))
				{
					if(final_spec.format != AUDIO_S16SYS)
					{
						std::cout<<"APU::Microphone Recording Device - #" << std::dec << mic_id << " does not support S16 audio\n";
					}

					else
					{
						std::cout<<"APU::Microphone Recording Device - #" << std::dec << mic_id << " :: " << SDL_GetAudioDeviceName(x, 1) << "\n";
						std::cout<<"APU::Microphone Channels - " << u32(final_spec.channels) << std::hex << "\n";

						apu_stat.mic_init = true;
						apu_stat.mic_id = mic_id;

						break;
					}
				}
			}
		}

		if(!apu_stat.mic_init)
		{
			std::cout<<"APU::No Microphone Recording Device found\n";
		}
	}

	return init_status;
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

/****** Generate raw samples for playback on external audio channel ******/
void AGB_APU::generate_ext_audio_hi_samples(s16* stream, int length)
{
	if(apu_stat.ext_audio.buffer == NULL) { return; }

	double sample_ratio = apu_stat.ext_audio.frequency/apu_stat.sample_rate;
	u32 last_pos = apu_stat.ext_audio.sample_pos;
	u32 buffer_pos = 0;

	//Convert existing buffer to S16
	s16* e_stream = (s16*) apu_stat.ext_audio.buffer;
	s16* k_stream = (s16*) apu_stat.ext_audio.karaoke_buffer;

	u32 set_size = (apu_stat.sample_rate / 60.0) / 9.0;
	u32 stream_size = apu_stat.ext_audio.length / 2;
	u32 karaoke_size = apu_stat.ext_audio.karaoke_length / 2;

	//Play-Yan - Silence audio when seeking forwards/backwards through videos
	bool is_seek_video = (mem->play_yan.is_video_playing && mem->play_yan.is_media_paused);

	for(int x = 0; x < length; x++)
	{
		buffer_pos = last_pos + (sample_ratio * x);
		u32 temp_pos = (apu_stat.ext_audio.channels == 1) ? buffer_pos : (buffer_pos * 2);
		apu_stat.ext_audio.last_pos = temp_pos;

		u32 sample_limit = (apu_stat.ext_audio.channels) ? (apu_stat.ext_audio.channels - 1) : 0;
		sample_limit += temp_pos;

		//Pull audio from buffer if possible
		if(sample_limit < stream_size)
		{
			//Mono Audio
			if(apu_stat.ext_audio.channels == 1)
			{
				//Karaoke Audio
				if((mem->jukebox.enable_karaoke) && (mem->jukebox.io_regs[0x008F]) && (temp_pos < karaoke_size)) 
				{
					stream[x] = k_stream[temp_pos];

					//When recording, use the karaoke track samples
					if((mem->jukebox.current_category == 2) && (mem->jukebox.is_recording))
					{
						e_stream[temp_pos] = stream[x];
					}
				}

				//Normal Audio
				{
					stream[x] = (is_seek_video) ? -32768 : e_stream[temp_pos];
				}
			}

			//Stereo Audio
			else
			{
				//Karaoke Audio
				if((mem->jukebox.enable_karaoke) && (mem->jukebox.io_regs[0x008F]) && ((temp_pos + 1) < karaoke_size)) 
				{
					s32 out_sample = (k_stream[temp_pos] + k_stream[temp_pos + 1]) / 2;
					stream[x] = out_sample;

					//When recording, use the karaoke track samples
					if((mem->jukebox.current_category == 2) && (mem->jukebox.is_recording))
					{
						e_stream[temp_pos] = stream[x];
					}
				}

				//Normal Audio
				else
				{
					s32 out_sample = (e_stream[temp_pos] + e_stream[temp_pos + 1]) / 2;
					stream[x] = (is_seek_video) ? -32768 : out_sample;
				}
			}	
		}

		//Otherwise, generate silence
		else
		{
			stream[x] = -32768;
			apu_stat.ext_audio.playing = false;

			//Terminate Play-Yan SFX
			if(mem->play_yan.is_sfx_playing)
			{
				mem->play_yan.is_music_playing = false;
				mem->play_yan.is_media_playing = false;
				mem->play_yan.is_sfx_playing = false;
			}

			//Terminate Play-Yan Sleep Mode during music playback
			if(mem->play_yan.is_sleeping)
			{
				for(u32 x = 0; x < 8; x++) { mem->play_yan.irq_data[x] = 0; }
				mem->play_yan.irq_data[0] = 0x40000801;
				mem->memory_map[REG_IF + 1] |= 0x20;
			}
		}

		//GBA Jukebox - Average samples for spectrum analyzer
		if(apu_stat.ext_audio.id == 1)
		{
			apu_stat.ext_audio.set_count++;
			mem->jukebox.spectrum_values[apu_stat.ext_audio.current_set] += (u32(stream[x]) + 32768);

			if(apu_stat.ext_audio.set_count >= set_size)
			{
				apu_stat.ext_audio.set_count = 0;
				mem->jukebox.spectrum_values[apu_stat.ext_audio.current_set] /= set_size;
			
				u32 val_1 = mem->jukebox.spectrum_values[apu_stat.ext_audio.current_set];
				double val_2 = (val_1 / 65535.0);
				u16 final_val = 20 * val_2;
			
				mem->jukebox.io_regs[0x90 + apu_stat.ext_audio.current_set] = final_val;
				mem->jukebox.spectrum_values[apu_stat.ext_audio.current_set] = 0;

				apu_stat.ext_audio.current_set++;
				if(apu_stat.ext_audio.current_set >= 0x09) { apu_stat.ext_audio.current_set = 0; }
			}
		}	
	}

	apu_stat.ext_audio.sample_pos = buffer_pos;
}

/****** SDL Audio Callback ******/ 
void agb_audio_callback(void* _apu, u8 *_stream, int _length)
{
	s16* stream = (s16*) _stream;
	int length = _length/2;

	std::vector<s16> channel_1_stream(length);
	std::vector<s16> channel_2_stream(length);
	std::vector<s16> channel_3_stream(length);
	std::vector<s16> channel_4_stream(length);

	std::vector<s16> dma_a_stream(length);
	std::vector<s16> dma_b_stream(length);

	std::vector<s16> ext_stream(length);

	AGB_APU* apu_link = (AGB_APU*) _apu;
	apu_link->generate_channel_1_samples(&channel_1_stream[0], length);
	apu_link->generate_channel_2_samples(&channel_2_stream[0], length);
	apu_link->generate_channel_3_samples(&channel_3_stream[0], length);
	apu_link->generate_channel_4_samples(&channel_4_stream[0], length);
	apu_link->generate_dma_a_samples(&dma_a_stream[0], length);
	apu_link->generate_dma_b_samples(&dma_b_stream[0], length);

	double channel_ratio = apu_link->apu_stat.channel_master_volume / 128.0;
	double dma_a_ratio = apu_link->apu_stat.dma[0].master_volume / 128.0;
	double dma_b_ratio = apu_link->apu_stat.dma[1].master_volume / 128.0;

	double ext_ratio = apu_link->apu_stat.ext_audio.volume / 63.0;

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

	//Mix in external audio if necessary
	if(apu_link->apu_stat.ext_audio.playing)
	{
		//Generate raw samples (high quality)
		if(apu_link->apu_stat.ext_audio.use_headphones)
		{
			apu_link->generate_ext_audio_hi_samples(&ext_stream[0], length);
		}

		//Generate GBA samples (low quality)
		else
		{
			//TODO
		}

		//Custom software mixing
		for(u32 x = 0; x < length; x++)
		{
			s32 out_sample = stream[x] + (ext_stream[x] * ext_ratio);
			
			//Divide final wave by total amount of channels
			out_sample /= 2;

			stream[x] = out_sample;
		}
	}
}

/****** SDL Audio Callback - Microphone ******/ 
void agb_microphone_callback(void* _apu, u8 *_stream, int _length)
{
	s16* stream = (s16*) _stream;
	int length = _length/2;

	AGB_APU* apu_link = (AGB_APU*) _apu;
	u32 mic_volume = 0;

	if(apu_link->apu_stat.mic_init)
	{
		//Save samples from microphone to file
		if(apu_link->apu_stat.save_recording)
		{
			std::string filename = config::data_path + "jukebox/" + apu_link->mem->jukebox.recorded_file;
			std::ofstream file(filename.c_str(), std::ios::binary | std::ios::trunc);
			u32 file_size = 0;

			std::vector <u8> wav_header;

			if(!file.is_open()) 
			{
				std::cout<<"APU::Error - Could not save microphone recording " << filename << "\n";
				file.close();
			}

			else
			{
				//Resample current microphone buffer at 11025Hz
				//This matches output from a real GBA Music Recorder/Jukebox
				double target_freq = (apu_link->mem->jukebox.current_category == 0) ? 44100.0 : 11025.0; 
				double resample_rate = apu_link->microphone_spec.freq / target_freq;
				u32 temp_pos = 0;
				std::vector <s16> resampled_buffer;

				for(double x = 0; x < apu_link->mic_buffer.size(); x += resample_rate)
				{
					temp_pos = x;
					resampled_buffer.push_back(apu_link->mic_buffer[temp_pos]);
				}

				file_size = resampled_buffer.size() * 2;

				//For Karaoke files, mix in external audio buffer from Music file
				if(apu_link->mem->jukebox.current_category == 2)
				{
					resample_rate = apu_link->apu_stat.ext_audio.frequency / 11025.0;
					if(apu_link->apu_stat.ext_audio.channels == 2) { resample_rate *= 2; }

					s16* e_stream = (s16*) apu_link->apu_stat.ext_audio.buffer;
					s32 temp_sample = 0;
					double stream_pos = 0;
					temp_pos = 0;

					for(u32 x = 0; x < resampled_buffer.size(); x++)
					{
						temp_pos = stream_pos;
						temp_sample = (resampled_buffer[x] + e_stream[temp_pos]) / 2;
						resampled_buffer[x] = temp_sample;
						stream_pos += resample_rate;
					}
				}
					
				//Build WAV header - Chunk ID - "RIFF" in ASCII
				wav_header.push_back(0x52);
				wav_header.push_back(0x49);
				wav_header.push_back(0x46);
				wav_header.push_back(0x46);

				//Chunk Size - PCM data + 36
				wav_header.push_back((file_size + 32) & 0xFF);
				wav_header.push_back(((file_size + 32) >> 8) & 0xFF);
				wav_header.push_back(((file_size + 32) >> 16) & 0xFF);
				wav_header.push_back(((file_size + 32) >> 24) & 0xFF);

				//Wave ID - "WAVE" in ASCII
				wav_header.push_back(0x57);
				wav_header.push_back(0x41);
				wav_header.push_back(0x56);
				wav_header.push_back(0x45);

				//Chunk ID - "fmt " in ASCII
				wav_header.push_back(0x66);
				wav_header.push_back(0x6D);
				wav_header.push_back(0x74);
				wav_header.push_back(0x20);

				//Chunk Size - 16
				wav_header.push_back(0x10);
				wav_header.push_back(0x00);
				wav_header.push_back(0x00);
				wav_header.push_back(0x00);

				//Format Code - 1
				wav_header.push_back(0x01);
				wav_header.push_back(0x00);

				//Number of Channels - 1
				wav_header.push_back(0x01);
				wav_header.push_back(0x00);

				//Sampling Rate
				u32 rate = target_freq;
				wav_header.push_back(rate & 0xFF);
				wav_header.push_back((rate >> 8) & 0xFF);
				wav_header.push_back((rate >> 16) & 0xFF);
				wav_header.push_back((rate >> 24) & 0xFF);

				//Data Rate
				rate *= 2;
				wav_header.push_back(rate & 0xFF);
				wav_header.push_back((rate >> 8) & 0xFF);
				wav_header.push_back((rate >> 16) & 0xFF);
				wav_header.push_back((rate >> 24) & 0xFF);
				
				//Block Align - 2
				wav_header.push_back(0x02);
				wav_header.push_back(0x00);

				//Bits per sample - 16
				wav_header.push_back(0x10);
				wav_header.push_back(0x00);

				//Chunk ID - "data" in ASCII
				wav_header.push_back(0x64);
				wav_header.push_back(0x61);
				wav_header.push_back(0x74);
				wav_header.push_back(0x61);

				//Chunk Size
				wav_header.push_back(file_size & 0xFF);
				wav_header.push_back((file_size >> 8) & 0xFF);
				wav_header.push_back((file_size >> 16) & 0xFF);
				wav_header.push_back((file_size >> 24) & 0xFF);		

				std::cout<<"APU::Writing microphone recording " << filename << "\n";
				file.write(reinterpret_cast<char*> (&wav_header[0]), wav_header.size());
				file.write(reinterpret_cast<char*> (&resampled_buffer[0]), file_size);
				file.close();
			}

			apu_link->apu_stat.save_recording = false;
			apu_link->apu_stat.is_recording = false;
			apu_link->mic_buffer.clear();
		}	

		//Grab samples from microphone and add to the buffer
		else if(apu_link->apu_stat.is_mic_on)
		{
			if(config::cart_type == AGB_JUKEBOX)
			{
				for(u32 x = 0; x < length; x++)
				{
					if(apu_link->apu_stat.is_recording) { apu_link->mic_buffer.push_back(stream[x]); }
					mic_volume += std::abs(stream[x]);
				}

				//Calculate average mic volume
				mic_volume /= length;
				double ratio = (mic_volume / 32767.0);
				apu_link->mem->jukebox.io_regs[0x008B] = 0xFFEE + (22 * ratio);
			}

			else if(config::cart_type == AGB_CAMPHO)
			{
				for(u32 x = 0; x < length; x++)
				{
					apu_link->mem->campho.microphone_out_buffer.push_back(stream[x]);
				}

				if(!apu_link->mem->campho.microphone_out_buffer.empty())
				{
					apu_link->mem->campho.send_audio_data = true;
				}
			}
		}

		//Clear microphone buffer if turned off
		else
		{
			if(!apu_link->mic_buffer.empty()) { apu_link->mic_buffer.clear(); }
		}
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
