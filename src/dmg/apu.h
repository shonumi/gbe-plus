// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : apu.h
// Date : May 30, 2015
// Description : Game Boy APU emulation
//
// Sets up SDL audio for mixing
// Generates and mixes samples for the GB's 4 sound channels

#ifndef GB_APU
#define GB_APU

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include "mmu.h"

class DMG_APU
{
	public:
	
	//Link to memory map
	DMG_MMU* mem;

	dmg_apu_data apu_stat;

	SDL_AudioSpec desired_spec;

	DMG_APU();
	~DMG_APU();

	bool init();
	void reset();

	//Serialize data for save state loading/saving
	bool apu_read(u32 offset, std::string filename);
	bool apu_write(std::string filename);
	u32 size();

	void generate_channel_1_samples(s16* stream, int length);
	void generate_channel_2_samples(s16* stream, int length);
	void generate_channel_3_samples(s16* stream, int length);
	void generate_channel_4_samples(s16* stream, int length);
}; 

/****** SDL Audio Callback ******/ 
void dmg_audio_callback(void* _apu, u8* _stream, int _length);

#endif // GB_APU 
