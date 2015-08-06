// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : util.h
// Date : August 06, 2015
// Description : Misc. utilites
//
// Provides miscellaneous utilities for the emulator

#include <iostream>

#include "util.h"

namespace util
{

/****** Saves an SDL Surface to a PNG file ******/
bool save_png(SDL_Surface* source, std::string filename)
{
	//TODO - Everything for this function

	if(source == NULL) 
	{
		std::cout<<"GBE::Error - Source data for " << filename << " is null\n";
		return false;
	}

	return true;
}

/****** Returns the minimum RGB component of a color ******/
u8 rgb_min(u32 color)
{
	u8 r = (color >> 16);
	u8 g = (color >> 8);
	u8 b = color;

	u8 min = r < g ? r : g;
	min = min < b ? min : b;

	return min;
}

/****** Returns the maximum RGB component of a color ******/
u8 rgb_max(u32 color)	
{
	u8 r = (color >> 16);
	u8 g = (color >> 8);
	u8 b = color;

	u8 max = r > g ? r : g;
	max = max > b ? max : b;

	return max;
}

} //Namespace
