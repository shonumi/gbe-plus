// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : apu.h
// Date : March 05, 2015
// Description : Game Boy Advance APU emulation
//
// Sets up SDL audio for mixing
// Generates and mixes samples for the GBA's 4 sound channels + DMA channels 

#ifndef GBA_APU
#define GBA_APU

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include "mmu.h"

class AGB_APU
{
	public:
	
	//Link to memory map
	AGB_MMU* mem;

	agb_apu_data apu_stat;

	SDL_AudioSpec desired_spec;

	AGB_APU();
	~AGB_APU();

	bool init();
	void reset();

	void buffer_channels();
	void buffer_channel_1();
	void buffer_channel_2();
	void buffer_channel_3();
	void buffer_channel_4();

	void generate_channel_1_samples(s16* stream, int length);
	void generate_channel_2_samples(s16* stream, int length);
	void generate_channel_3_samples(s16* stream, int length);
	void generate_channel_4_samples(s16* stream, int length);
	void generate_dma_a_samples(s16* stream, int length);
	void generate_dma_b_samples(s16* stream, int length);

	//Serialize data for save state loading/saving
	bool apu_read(u32 offset, std::string filename);
	bool apu_write(std::string filename);
	u32 size();
};

/****** SDL Audio Callback ******/ 
void agb_audio_callback(void* _apu, u8 *_stream, int _length);

#endif // GBA_APU 
