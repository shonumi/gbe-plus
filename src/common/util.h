// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : util.h
// Date : August 06, 2015
// Description : Misc. utilites
//
// Provides miscellaneous utilities for the emulator

#ifndef GBE_UTIL
#define GBE_UTIL

#include <string>

#include <SDL/SDL.h>

#include "common.h"

namespace util
{
	struct hsv
	{
		double hue;
		double saturation;
		double value;
	};

	bool save_png(SDL_Surface* source, std::string filename);
	u8 rgb_min(u32 color);
	u8 rgb_max(u32 color);

	u32 hsv_to_rgb(hsv color);
	hsv rgb_to_hsv(u32 color);

	u32 reflect(u32 src, u8 bit);
	void init_crc32_table();
	u32 get_crc32(u8* data, u32 length);

	u8 xbit(u8 a, u8 b);
	u16 xbit(u16 a, u16 b);
	u32 xbit(u32 a, u32 b);

	std::string to_hex_str(u32 input);
	bool from_hex_str(std::string input, u32 &result);

	extern u32 crc32_table[256];
	extern u32 poly32;
}

#endif // GBE_UTIL 
