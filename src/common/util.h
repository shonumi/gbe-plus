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
#include <vector>

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
	u32 adjust_contrast(u32 color, s16 factor);

	u32 reflect(u32 src, u8 bit);
	void init_crc32_table();
	u32 get_crc32(u8* data, u32 length);
	u32 get_file_crc32(std::string filename);

	u32 get_addler32(u8* data, u32 length);

	u32 switch_endian32(u32 input);

	std::string to_hex_str(u32 input);
	std::string to_hex_str(u32 input, u8 bit_level);
	bool from_hex_str(std::string input, u32 &result);

	std::string to_str(u32 input);
	std::string to_sstr(s32 input);
	std::string to_strf(float input);
	bool from_str(std::string input, u32 &result);

	std::string ip_to_str(u32 ip_addr);
	bool ip_to_u32(std::string ip_addr, u32 &result);

	std::string data_to_str(u8* data, u32 length);
	void str_to_data(u8* data, std::string input);

	std::string make_ascii_printable(std::string input);
	std::string get_utc_string();

	std::string get_filename_from_path(std::string path);
	std::string get_filename_no_ext(std::string filename);
	void get_files_in_dir(std::string dir_src, std::string extension, std::vector<std::string>& file_list, bool recursive, bool full_path);
	void get_folders_in_dir(std::string dir_src, std::vector<std::string>& folder_list);

	u32 get_bcd(u32 input);
	u32 get_bcd_int(u32 input);

	u32 bswap(u32 input);

	SDL_Surface* load_icon(std::string filename);

	void build_wav_header(std::vector<u8>& header, u32 sample_rate, u32 channels, u32 data_size); 

	extern u32 crc32_table[256];
	extern u32 poly32;

	extern std::string utc_day[7];
	extern std::string utc_mon[12];
	extern std::string utc_num[60];
}

#endif // GBE_UTIL 
