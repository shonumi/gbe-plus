// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : util.h
// Date : August 06, 2015
// Description : Misc. utilites
//
// Provides miscellaneous utilities for the emulator

#include <iostream>
#include <fstream>
#include <vector>

#include "util.h"

namespace util
{

//CRC32 Polynomial
u32 poly32 = 0x04C11DB7;

//CRC lookup table
u32 crc32_table[256];

/****** Saves an SDL Surface to a PNG file ******/
bool save_png(SDL_Surface* source, std::string filename)
{
	//TODO - Everything for this function

	if(source == NULL) 
	{
		std::cout<<"GBE::Error - Source data for " << filename << " is null\n";
		return false;
	}
	
	//Lock SDL_Surface
	if(SDL_MUSTLOCK(source)){ SDL_LockSurface(source); }
	u8* src_pixels = (u8*)source->pixels;

	std::vector<u8> png_bytes;

	//PNG Magic Number - 8 bytes
	png_bytes.push_back(0x89);
	png_bytes.push_back(0x50);
	png_bytes.push_back(0x4E);
	png_bytes.push_back(0x47);
	png_bytes.push_back(0x0D);
	png_bytes.push_back(0x0A);
	png_bytes.push_back(0x1A);
	png_bytes.push_back(0x0A);

	//IHDR byte length (always 0x0D for us) - 4 bytes
	png_bytes.push_back(0x00);
	png_bytes.push_back(0x00);
	png_bytes.push_back(0x00);
	png_bytes.push_back(0x0D);

	//"IHDR" ASCII string - 4 bytes
	png_bytes.push_back(0x49);
	png_bytes.push_back(0x48);
	png_bytes.push_back(0x44);
	png_bytes.push_back(0x52);

	//Image width - 4 bytes
	u32 width = source->w;

	for(int x = 24; x >= 0; x-= 8)
	{
		png_bytes.push_back((width >> x) & 0xFF);
	}

	//Image height - 4 bytes
	u32 height = source->h;

	for(int x = 24; x >= 0; x-= 8)
	{
		png_bytes.push_back((height >> x) & 0xFF);
	}

	//Bit depth (always 0x08 for us) - 1 byte
	png_bytes.push_back(0x08);

	//Color type (always 0x02 for us) 1 byte
	png_bytes.push_back(0x02);

	//Compression method (always 0x00 for us, use DEFLATE) - 1 byte
	//Filter method (always 0x00 for us) - 1 byte
	//Interlace method (always 0x00 for us) - 1 byte
	png_bytes.push_back(0x00);
	png_bytes.push_back(0x00);
	png_bytes.push_back(0x00);

	//CRC32 of IHDR chunk - 4 bytes
	u32 ihdr_crc = util::get_crc32((png_bytes.data() + 12), 17);

	for(int x = 24; x >= 0; x-= 8)
	{
		png_bytes.push_back((ihdr_crc >> x) & 0xFF);
	}

	//IDAT byte length - 4 bytes
	u32 dat_length = (source->w * source->h) * 3;

	for(int x = 24; x >= 0; x-= 8)
	{
		png_bytes.push_back((dat_length >> x) & 0xFF);
	}

	//"IDAT" ASCII string - 4 bytes
	png_bytes.push_back(0x49);
	png_bytes.push_back(0x44);
	png_bytes.push_back(0x41);
	png_bytes.push_back(0x54);

	//ZLib header - 4 bytes
	png_bytes.push_back(0x78);
	png_bytes.push_back(0x00);
	//TODO Addler32 checksum of data

	//IDAT Chunk header - TODO
	//Final chunk flag - 1 Byte
	//Chunk size byte length - 2 Bytes
	//Chunk size complement - 2 Bytes
	png_bytes.push_back(0x01);

	//Grab RGB values, then store them
	for(int x = 0; x < (source->w * source->h) * 4;)
	{
		//Skip Alpha values
		x++;

		png_bytes.push_back(src_pixels[x++]);
		png_bytes.push_back(src_pixels[x++]);
		png_bytes.push_back(src_pixels[x++]);
	}

	//CRC32 of IDAT chunk - 4 bytes
	u32 idat_crc = util::get_crc32((png_bytes.data() + 37), (dat_length + 4));

	for(int x = 24; x >= 0; x-= 8)
	{
		png_bytes.push_back((idat_crc >> x) & 0xFF);
	}

	//IEND byte length (always 0x00 for us) - 4 bytes
	png_bytes.push_back(0x00);
	png_bytes.push_back(0x00);
	png_bytes.push_back(0x00);
	png_bytes.push_back(0x00);

	//"IEND" ASCII string
	png_bytes.push_back(0x49);
	png_bytes.push_back(0x45);
	png_bytes.push_back(0x4E);
	png_bytes.push_back(0x44);

	//CRC32 of IEND - Precalculated
	png_bytes.push_back(0xAE);
	png_bytes.push_back(0x42);
	png_bytes.push_back(0x60);
	png_bytes.push_back(0x82);

	std::ofstream file(filename.c_str(), std::ios::binary);
	file.write(reinterpret_cast<char*> (&png_bytes[0]), png_bytes.size());
	file.close();

	//Unlock SDL_Surface
	if(SDL_MUSTLOCK(source)) { SDL_UnlockSurface(source); }

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

/****** Converts RGB to HSV ******/
hsv rgb_to_hsv(u32 color)
{
	hsv final_color;

	u8 r = (color >> 16);
	u8 g = (color >> 8);
	u8 b = color;

	u8 max_raw = rgb_max(color);

	//Calculate the color delta
	double max = rgb_max(color) / 255.0;
	double min = rgb_min(color) / 255.0;	
	double color_delta = max - min;

	//Calculate the hue
	double pre_hue = 0.0;
	double hue_r = r / 255.0;
	double hue_g = g / 255.0;
	double hue_b = b / 255.0;

	if(color_delta == 0)
	{
		//Assign a color delta of 0 to 0 degrees as the hue
		//If all the RGB components are the same, color is a pure shade of gray (black, white, somewhere in between)
		final_color.hue = 0.0;
	}

	//Calculation if Red is the greatest RGB component		
	else if(max_raw == r)
	{
		pre_hue = (hue_g - hue_b) / color_delta;
		final_color.hue = (pre_hue * 60.0);
	}

	//Calculation if Green is the greatest RGB component		
	else if(max_raw == g)
	{
		pre_hue = ((hue_b - hue_r) / color_delta) + 2;
		final_color.hue = (pre_hue * 60.0);
	}

	//Calculation if Blue is the greatest RGB component		
	else
	{
		pre_hue = ((hue_r - hue_g) / color_delta) + 4;
		final_color.hue = (pre_hue * 60.0);
	}

	if(final_color.hue < 0.0) { final_color.hue += 360.0; }

	//Calculate the saturation
	if(max > 0) { final_color.saturation = (color_delta / max); }
	else { final_color.saturation = 0.0; }

	//Calculate the value
	final_color.value = max;

	return final_color;
}

/****** Converts HSV to RGB ******/
u32 hsv_to_rgb(hsv color)
{
	u32 final_color = 0x0;

	double r, g, b = 0.0;

	s32 i;
	double f, p, q, t;

	//If there is zero saturation, this is a shade of gray
	if(color.saturation == 0) { r = g = b = color.value; }

	else 
	{
		color.hue /= 60.0;
		i = color.hue;
		f = color.hue - i;
		p = color.value * (1.0 - color.saturation);
		q = color.value * (1.0 - (color.saturation * f));
		t = color.value * (1.0 - (color.saturation * (1.0 - f)));

		switch(i)
		{
			case 0:
				r = color.value;
				g = t;
				b = p;
				break;

			case 1:
				r = q;
				g = color.value;
				b = p;
				break;

			case 2:
				r = p;
				g = color.value;
				b = t;
				break;

			case 3:
				r = p;
				g = q;
				b = color.value;
				break;

			case 4:
				r = t;
				g = p;
				b = color.value;
				break;

			default:		
				r = color.value;
				g = p;
				b = q;
				break;
		}
	}

	//Process final color
	//Convert RGB percentages to u8
	u8 out_r = (2.55 * (r * 100));
	u8 out_g = (2.55 * (g * 100));
	u8 out_b = (2.55 * (b * 100));

	final_color = 0xFF000000 | (out_r << 16) | (out_g << 8) | out_b;
	return final_color;
}

/****** Mirrors bits ******/
u32 reflect(u32 src, u8 bit)
{
	//2nd parameter 'bit' defines which to stop mirroring, e.g. generally Bit 7, Bit 15, or Bit 31 (count from zero)

	u32 out = 0;

	for(int x = 0; x <= bit; x++)
	{
		if(src & 0x1) { out |= (1 << (bit - x)); }
		src >>= 1;
	}

	return out;
}

/****** Sets up the CRC lookup table ******/
void init_crc32_table()
{
	for(int x = 0; x < 256; x++)
	{
		crc32_table[x] = (reflect(x, 7) << 24);

		for(int y = 0; y < 8; y++)
		{
			crc32_table[x] = (crc32_table[x] << 1) ^ (crc32_table[x] & (1 << 31) ? poly32 : 0);
		}

		crc32_table[x] = reflect(crc32_table[x], 31);
	}
}

/****** Return CRC for given data ******/
u32 get_crc32(u8* data, u32 length)
{
	u32 crc32 = 0xFFFFFFFF;

	for(int x = 0; x < length; x++)
	{
		crc32 = (crc32 >> 8) ^ crc32_table[(crc32 & 0xFF) ^ (*data)];
		data++;
	}

	return (crc32 ^ 0xFFFFFFFF);
}

/****** Determine if two 8-bit variables share the same bits set bits ******/
u8 xbit(u8 a, u8 b)
{
	u8 x = ~(a ^ b);
	return ~((a ^ x) | (b ^ x));
}

/****** Determine if two 16-bit variables share the same bits set bits ******/
u16 xbit(u16 a, u16 b)
{
	u16 x = ~(a ^ b);
	return ~((a ^ x) | (b ^ x));
}

/****** Determine if two 32-bit variables share the same bits set bits ******/
u32 xbit(u32 a, u32 b)
{
	u32 x = ~(a ^ b);
	return ~((a ^ x) | (b ^ x));
}

} //Namespace
