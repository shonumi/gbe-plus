// GB Enhanced Copyright Daniel Baxter 2017
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : apu.h
// Date : September 10, 2017
// Description : Nintendo DS APU emulation
//
// Sets up SDL audio for mixing
// Generates and mixes samples for the NDS' 16 sound channels 

#ifndef NDS_APU
#define NDS_APU

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include "mmu.h"

class NTR_APU
{
	public:
	
	//Link to memory map
	NTR_MMU* mem;

	ntr_apu_data apu_stat;

	SDL_AudioSpec desired_spec;
    	SDL_AudioSpec obtained_spec;

	NTR_APU();
	~NTR_APU();

	void generate_channel_samples(s32* stream, int length, u8 id);
	void decode_adpcm_samples(u8 id);

	bool init();
	void reset();
};

/****** SDL Audio Callback ******/ 
void ntr_audio_callback(void* _apu, u8 *_stream, int _length);

#endif // NDS_APU 
