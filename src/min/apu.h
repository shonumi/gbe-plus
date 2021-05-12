// GB Enhanced Copyright Daniel Baxter 2021
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : apu.h
// Date : May 06, 2021
// Description : Game Boy Advance APU emulation
//
// Sets up SDL audio for mixing
// Generates and mixes samples for the PM's single sound channel 

#ifndef PM_APU
#define PM_APU

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include "mmu.h"

class MIN_APU
{
	public:
	
	//Link to memory map
	MIN_MMU* mem;

	min_apu_data apu_stat;

	SDL_AudioSpec desired_spec;

	MIN_APU();
	~MIN_APU();

	bool init();
	void reset();

	void buffer_channel();
	void generate_samples(s16* stream, int length);
};

/****** SDL Audio Callback ******/ 
void min_audio_callback(void* _apu, u8 *_stream, int _length);

#endif // PM_APU 

