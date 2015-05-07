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

#include <SDL/SDL.h>
#include <SDL/SDL_audio.h>
#include "mmu.h"

class AGB_APU
{
	public:
	
	//Link to memory map
	AGB_MMU* mem;

	apu_data apu_stat;

	SDL_AudioSpec desired_spec;
    	SDL_AudioSpec obtained_spec;

	AGB_APU();
	~AGB_APU();

	bool init();
	void reset();

	void generate_channel_1_samples(s16* stream, int length);
	void generate_channel_2_samples(s16* stream, int length);
	void generate_channel_3_samples(s16* stream, int length);
	void generate_channel_4_samples(s16* stream, int length);
	void generate_dma_a_samples(s16* stream, int length);
	void generate_dma_b_samples(s16* stream, int length);
};

/****** SDL Audio Callback ******/ 
void audio_callback(void* _apu, u8 *_stream, int _length);

#endif // GBA_APU 
