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

#include <SDL2/SDL.h>

#include "common.h"

namespace util
{
	struct hsv
	{
		double hue;
		double saturation;
		double value;
	};

	struct hsl
	{
		double hue;
		double saturation;
		double lightness;
	};

	union col32
	{
		struct 
		{
			u8 b;
			u8 g;
			u8 r;
			u8 a;
		};

		u32 color;
	};

	bool save_png(SDL_Surface* source, std::string filename);

	u8 rgb_min(u32 color);
	u8 rgb_max(u32 color);

	u32 hsv_to_rgb(hsv color);
	hsv rgb_to_hsv(u32 color);

	u32 hsl_to_rgb(hsl color);
	hsl rgb_to_hsl(u32 color);

	u8 hue_to_rgb(double hue_factor_1, double hue_factor_2, double hue);

	u8 get_brightness_fast(u32 color);
	u32 rgb_blend(u32 color_1, u32 color_2);

	u32 add_color_factor(u32 color, u32 factor);
	u32 sub_color_factor(u32 color, u32 factor);
	u32 multiply_color_factor(u32 color, double factor);

	u32 reflect(u32 src, u8 bit);
	void init_crc32_table();
	u32 get_crc32(u8* data, u32 length);

	u32 get_addler32(u8* data, u32 length;

	std::string to_hex_str(u32 input);
	bool from_hex_str(std::string input, u32 &result);

	std::string to_str(u32);
	bool from_str(std::string input, u32 &result);

	std::string ip_to_str(u32 ip_addr);
	bool ip_to_u32(std::string ip_addr, u32 &result);

	std::string data_to_str(u8* data, u32 length);
	void str_to_data(u8* data, std::string input);

	std::string get_filename_from_path(std::string path);

	SDL_Surface* load_icon(std::string filename);

	extern u32 crc32_table[256];
	extern u32 poly32;
}

#endif // GBE_UTIL 
