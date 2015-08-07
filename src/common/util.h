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
}

#endif // GBE_UTIL 
