// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : util.h
// Date : August 06, 2015
// Description : Misc. utilites
//
// Provides miscellaneous utilities for the emulator

#ifndef GL3_PROTOTYPES
#define GL3_PROTOTYPES 1
#endif

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

#ifdef GBE_GLEW
#include "GL/glew.h"
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "SDL_opengl.h"
#include "config.h"
#include "util.h"

namespace util
{

//CRC32 Polynomial
u32 poly32 = 0x04C11DB7;

//CRC lookup table
u32 crc32_table[256];

//UTC format LUT strings
std::string utc_day[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

std::string utc_num[60] =
{
	"00", "01", "02", "03", "04", "05", "06", "07", "08", "09",
	"10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
	"20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
	"30", "31", "32", "33", "34", "35", "36", "37", "38", "39",
	"40", "41", "42", "43", "44", "45", "46", "47", "48", "49",
	"50", "51", "52", "53", "54", "55", "56", "57", "58", "59"
};

std::string utc_mon[12] =
{
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

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

/****** Converts RGB to HSL ******/
hsl rgb_to_hsl(u32 color)
{
	hsl final_color;

	u8 r = (color >> 16);
	u8 g = (color >> 8);
	u8 b = color;

	//Calculate the color delta
	double max = rgb_max(color) / 255.0;
	double min = rgb_min(color) / 255.0;	
	double color_delta = max - min;

	//Calculate lightness
	final_color.lightness = (max + min) / 2.0;

	if(color_delta == 0) { final_color.hue = final_color.saturation = 0; }

	else
	{
		//Calculate saturation
		if(final_color.lightness < 0.5) { final_color.saturation = color_delta / (max + min); }
		else { final_color.saturation = color_delta / (2 - max - min); }

		//Calculate hue
		double hue_r = r / 255.0;
		double hue_g = g / 255.0;
		double hue_b = b / 255.0;

		double r_delta = (((max - hue_r ) / 6.0) + (color_delta / 2.0)) / color_delta;
		double g_delta = (((max - hue_g ) / 6.0) + (color_delta / 2.0)) / color_delta;
		double b_delta = (((max - hue_b ) / 6.0) + (color_delta / 2.0)) / color_delta;

		if(hue_r == max) { final_color.hue = b_delta - g_delta; }
		else if(hue_g == max) { final_color.hue = (1 / 3.0) + r_delta - b_delta; }
		else if(hue_b == max) { final_color.hue = (2 / 3.0 ) + g_delta - r_delta; }

		if(final_color.hue < 0 ) { final_color.hue += 1; }
   		else if(final_color.hue > 1 ) { final_color.hue -= 1; }
	}

	return final_color;
}
		
/****** Converts HSL to RGB ******/
u32 hsl_to_rgb(hsl color)
{
	u32 final_color = 0;
	
	u8 r, g, b = 0;

	if(color.saturation == 0) { r = g = b = (color.lightness * 255); }

	else
	{
		double hue_factor_1, hue_factor_2 = 0.0;

		if(color.lightness < 0.5) { hue_factor_2 = color.lightness * (1 + color.saturation); }
		else { hue_factor_2 = (color.lightness + color.saturation) - (color.saturation * color.lightness); }

		hue_factor_1 = (2 * color.lightness) - hue_factor_2;

		r = hue_to_rgb(hue_factor_1, hue_factor_2, color.hue + (1/3.0));
		g = hue_to_rgb(hue_factor_1, hue_factor_2, color.hue);
		b = hue_to_rgb(hue_factor_1, hue_factor_2, color.hue - (1/3.0));
	}

	final_color = 0xFF000000 | (r << 16) | (g << 8) | b;
	return final_color;	
}

/****** Converts an HSL hue to an RGB component ******/
u8 hue_to_rgb(double hue_factor_1, double hue_factor_2, double hue)
{
	if(hue < 0) { hue += 1; }
	else if(hue > 1) { hue -= 1; }

	if((6 * hue) < 1) { return 255 * (hue_factor_1 + (hue_factor_2 - hue_factor_1) * 6 * hue); }
	else if((2 * hue) < 1) { return 255 * hue_factor_2; }
	else if((3 * hue) < 2) { return 255 * (hue_factor_1 + (hue_factor_2 - hue_factor_1) * ((2/3.0) - hue) * 6); }

	return 255 * hue_factor_1;
}

/****** Get the perceived brightness of a pixel - Quick and dirty version ******/
u8 get_brightness_fast(u32 color)
{
	u8 r = (color >> 16);
	u8 g = (color >> 8);
	u8 b = color;

	return (r+r+r+g+g+g+g+b) >> 3;
}

/****** Blends the RGB channels of 2 colors ******/
u32 rgb_blend(u32 color_1, u32 color_2)
{
	if(color_1 == color_2) { return color_1; }

	u16 r = ((color_1 >> 16) & 0xFF) + ((color_2 >> 16) & 0xFF);
	u16 g = ((color_1 >> 8) & 0xFF) + ((color_2 >> 8) & 0xFF);
	u16 b = (color_1 & 0xFF) + (color_2 & 0xFF);

	r >>= 1;
	g >>= 1;
	b >>= 1;

	return 0xFF000000 | (r << 16) | (g << 8) | b;
}

/****** Adds a value to all RGB channels of a color ******/
u32 add_color_factor(u32 color, u32 factor)
{
	util::col32 output;
	output.color = color;

	s32 r_factor = output.r + factor;
	s32 g_factor = output.g + factor;
	s32 b_factor = output.b + factor;

	if(r_factor > 255) { output.r = 255; }
	else { output.r = r_factor; }

	if(g_factor > 255) { output.g = 255; }
	else { output.g = g_factor; }

	if(b_factor > 255) { output.b = 255; }
	else { output.b = b_factor; }

	return output.color;
}

/****** Subtracts a value from all RGB channels of a color ******/
u32 sub_color_factor(u32 color, u32 factor)
{
	util::col32 output;
	output.color = color;

	s32 r_factor = output.r - factor;
	s32 g_factor = output.g - factor;
	s32 b_factor = output.b - factor;

	if(r_factor < 0) { output.r = 0; }
	else { output.r = r_factor; }

	if(g_factor < 0) { output.g = 0; }
	else { output.g = g_factor; }

	if(b_factor < 0) { output.b = 0; }
	else { output.b = b_factor; }

	return output.color;
}

/****** Multiplies all RGB channels by a value ******/
u32 multiply_color_factor(u32 color, double factor)
{
	util::col32 output;
	output.color = color;

	s32 r_factor = output.r * factor;
	s32 g_factor = output.g * factor;
	s32 b_factor = output.b * factor;

	if(r_factor < 0) { output.r = 0; }
	else if(r_factor > 255) { output.r = 255; }
	else { output.r = r_factor; }

	if(g_factor < 0) { output.g = 0; }
	else if(g_factor > 255) { output.g = 255; }
	else { output.g = g_factor; }

	if(b_factor < 0) { output.b = 0; }
	else if(b_factor > 255) { output.b = 255; }
	else { output.b = b_factor; }

	return output.color;
}

/****** Changes an RGB color's contrast by factor ******/ 
u32 adjust_contrast(u32 color, s16 factor)
{
	//Truncate factor to -255 through +255
	if(factor < -255) { factor = -255; }
	else if(factor > 255) { factor = 255; }

	//Calculate contrast correction factor
	float correction_factor = (259.0 * (factor + 255)) / (255.0 * (259 - factor));

	//Change RGB channels and truncate results
	s16 r = (color >> 16) & 0xFF;
	s16 g = (color >> 8) & 0xFF;
	s16 b = (color & 0xFF);

	r = (correction_factor * (r - 128)) + 128;
	g = (correction_factor * (g - 128)) + 128;
	b = (correction_factor * (b - 128)) + 128;

	if(r < 0) { r = 0; }
	else if(r > 255) { r = 255; }

	if(g < 0) { g = 0; }
	else if(g > 255) { g = 255; }

	if(b < 0) { b = 0; }
	else if(b > 255) { b = 255; }

	return 0xFF000000 | (r << 16) | (g << 8) | b;
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

/****** Return CRC32 for given data ******/
u32 get_crc32(u8* data, u32 length)
{
	init_crc32_table();

	u32 crc32 = 0xFFFFFFFF;

	for(int x = 0; x < length; x++)
	{
		crc32 = (crc32 >> 8) ^ crc32_table[(crc32 & 0xFF) ^ (*data)];
		data++;
	}

	return (crc32 ^ 0xFFFFFFFF);
}

/****** Returns the CRC32 of a given file ******/
u32 get_file_crc32(std::string filename)
{
	u32 result = 0;
	std::vector<u8> file_data;
	std::ifstream file(filename.c_str(), std::ios::binary);

	if(!file.is_open()) 
	{
		std::cout<<"Could not get the CRC32 of the file " << filename << "\n";
		return false;
	}

	//Get the file size
	file.seekg(0, file.end);
	u32 file_size = file.tellg();
	file.seekg(0, file.beg);

	file_data.resize(file_size);
	u8* ex_mem = &file_data[0];
	file.read((char*)ex_mem, file_size);

	result = get_crc32(ex_mem, file_size);
	return result;
}

/****** Return Addler32 for given data ******/
u32 get_addler32(u8* data, u32 length)
{
	u16 a = 1;
	u16 b = 0;

	for(int x = 0; x < length; x++)
	{
		a += (*data);
		b += a;
	}

	a = a % 65521;
	b = b % 65521;

	u32 result = (b *= 65536) + a;
	return result;
}

/****** Switches endianness of a 32-bit integer ******/
u32 switch_endian32(u32 input)
{	
	u8 byte_1 = ((input >> 24) & 0xFF);
	u8 byte_2 = ((input >> 16) & 0xFF);
	u8 byte_3 = ((input >> 8) & 0xFF);
	u8 byte_4 = (input & 0xFF);

	return ((byte_4 << 24) | (byte_3 << 16) | (byte_2 << 8) | byte_1);
}

/****** Convert a number into hex as a C++ string ******/
std::string to_hex_str(u32 input)
{
	std::stringstream temp;

	//Auto fill with '0's
	if(input < 0x10) { temp << "0x0" << std::hex << std::uppercase << input; }
	else if((input < 0x1000) && (input >= 0x100)) { temp << "0x0" << std::hex << std::uppercase << input; }
	else if((input < 0x100000) && (input >= 0x10000)) { temp << "0x0" << std::hex << std::uppercase << input; }
	else if((input < 0x10000000) && (input >= 0x1000000)) { temp << "0x0" << std::hex << std::uppercase << input; }
	else { temp << "0x" << std::hex << std::uppercase << input; }

	return temp.str();
}

/****** Convert a number into hex as C++ string - Full 8, 16, 24, or 32-bit representations ******/
std::string to_hex_str(u32 input, u8 bit_level)
{
	std::stringstream temp;
	std::string result = "";
	u32 num = (input & 0xFF);

	//Limit to 32-bit max
	if(bit_level > 4) { bit_level = 4; }

	for(u32 x = 0; x < bit_level; x++)
	{
		temp << std::hex << std::uppercase << num;
		result += temp.str();
		if(num < 0x10) { result = "0" + result; }
		num = ((input >> ((x+1) * 8)) & 0xFF);
		temp.str(std::string());
	}

	result = "0x" + result;
	return result;
}

/****** Converts C++ string representing a hex number into an integer value ******/
bool from_hex_str(std::string input, u32 &result)
{
	//This function expects the hex string to contain only hexadecimal numbers and letters
	//E.g. it expects "8000" rather than "0x8000" or "$8000"
	//Returns false + result = 0 if it encounters any unexpected characters

	result = 0;
	u32 hex_size = (input.size() - 1);
	std::string hex_char = "";

	//Convert hex string into usable u32
	for(int x = hex_size, y = 0; x >= 0; x--, y+=4)
	{
		hex_char = input[x];

		if(hex_char == "0") { result += 0; }
		else if(hex_char == "1") { result += (1 << y); }
		else if(hex_char == "2") { result += (2 << y); }
		else if(hex_char == "3") { result += (3 << y); }
		else if(hex_char == "4") { result += (4 << y); }
		else if(hex_char == "5") { result += (5 << y); }
		else if(hex_char == "6") { result += (6 << y); }
		else if(hex_char == "7") { result += (7 << y); }
		else if(hex_char == "8") { result += (8 << y); }
		else if(hex_char == "9") { result += (9 << y); }
		else if(hex_char == "A") { result += (10 << y); }
		else if(hex_char == "a") { result += (10 << y); }
		else if(hex_char == "B") { result += (11 << y); }
		else if(hex_char == "b") { result += (11 << y); }
		else if(hex_char == "C") { result += (12 << y); }
		else if(hex_char == "c") { result += (12 << y); }
		else if(hex_char == "D") { result += (13 << y); }
		else if(hex_char == "d") { result += (13 << y); }
		else if(hex_char == "E") { result += (14 << y); }
		else if(hex_char == "e") { result += (14 << y); }
		else if(hex_char == "F") { result += (15 << y); }
		else if(hex_char == "f") { result += (15 << y); }
		else { result = 0; return false; }
	}

	return true;
}

/****** Convert an unsigned integer into a C++ string ******/
std::string to_str(u32 input)
{
	std::stringstream temp;
	temp << input;
	return temp.str();
}

/****** Convert a float into a C++ string ******/
std::string to_strf(float input)
{
	std::stringstream temp;
	temp << input;
	return temp.str();
}

/****** Convert a signed integer into a C++ string ******/
std::string to_sstr(s32 input)
{
	std::stringstream temp;
	temp << input;
	return temp.str();
}

/****** Converts a string into an integer value ******/
bool from_str(std::string input, u32 &result)
{
	result = 0;
	u32 size = (input.size() - 1);
	std::string value_char = "";

	if(input.size() == 0) { return false; }

	//Convert string into usable u32
	for(int x = size, y = 1; x >= 0; x--, y *= 10)
	{
		value_char = input[x];

		if(value_char == "0") { result += 0; }
		else if(value_char == "1") { result += (1 * y); }
		else if(value_char == "2") { result += (2 * y); }
		else if(value_char == "3") { result += (3 * y); }
		else if(value_char == "4") { result += (4 * y); }
		else if(value_char == "5") { result += (5 * y); }
		else if(value_char == "6") { result += (6 * y); }
		else if(value_char == "7") { result += (7 * y); }
		else if(value_char == "8") { result += (8 * y); }
		else if(value_char == "9") { result += (9 * y); }
		else if(value_char == "-") { result *= -1; }
		else { result = 0; return false; }
	}

	return true;
}

/****** Converts a series of bytes to ASCII ******/
std::string data_to_str(u8* data, u32 length)
{
	std::string temp = "";

	for(u32 x = 0; x < length; x++)
	{
		char ascii = *data;
		temp += ascii;
		data++;
	}

	return temp;
}

/****** Converts an ASCII string to a series of bytes ******/
void str_to_data(u8* data, std::string input)
{
	for(u32 x = 0; x < input.size(); x++)
	{
		char ascii = input[x];
		*data = ascii;
		data++;
	}
}

/****** Swaps unprintable characters in an ASCII string for spaces ******/
std::string make_ascii_printable(std::string input)
{
	std::string result = "";

	for(u32 x = 0; x < input.size(); x++)
	{
		char ascii = input[x];

		if((ascii >= 0x20) && (ascii <= 0x7E)) { result += ascii; }
		else { result += " "; }
	}

	return result;
}

/****** Gets current UTC time as a string ******/
std::string get_utc_string()
{
	std::string result = "";

	//Grab local time
	time_t system_time = time(0);
	tm* current_time = gmtime(&system_time);

	result = utc_day[current_time->tm_wday] + ", " + utc_num[current_time->tm_mday] + " " + utc_mon[current_time->tm_mon] + " " + to_str(current_time->tm_year + 1900) + " ";
	result += (utc_num[current_time->tm_hour] + "::" + utc_num[current_time->tm_min] + "::" + utc_num[current_time->tm_sec % 60]);

	std::cout<< result << "\n";

	return result;
}

/****** Converts a string IP address to an integer value ******/
bool ip_to_u32(std::string ip_addr, u32 &result)
{
	u8 digits[4] = { 0, 0, 0, 0 };
	u8 dot_count = 0;
	std::string temp = "";
	std::string current_char = "";
	u32 str_end = ip_addr.length() - 1;

	//IP address comes in the form 123.456.678.901
	//Grab each character between the dots and convert them into integer
	for(u32 x = 0; x < ip_addr.length(); x++)
	{
		current_char = ip_addr[x];

		//Check to make sure the character is valid for an IP address
		bool check_char = false;

		if(current_char == "0") { check_char = true; }
		else if(current_char == "1") { check_char = true; }
		else if(current_char == "2") { check_char = true; }
		else if(current_char == "3") { check_char = true; }
		else if(current_char == "4") { check_char = true; }
		else if(current_char == "5") { check_char = true; }
		else if(current_char == "6") { check_char = true; }
		else if(current_char == "7") { check_char = true; }
		else if(current_char == "8") { check_char = true; }
		else if(current_char == "9") { check_char = true; }
		else if(current_char == ".") { check_char = true; }

		//Quit now if invalid character
		if(!check_char)
		{
			result = 0;
			return false;
		}

		//Convert characters into u32 when a "." is encountered
		if((current_char == ".") || (x == str_end))
		{
			if(x == str_end) { temp += current_char; }

			//If somehow parsing more than 3 dots, string is malformed
			if(dot_count == 4)
			{
				result = 0;
				return false;
			}

			u32 digit = 0;

			//If the string can't be converted into a digit, quit now
			if(!from_str(temp, digit))
			{
				result = 0;
				return false;
			}

			//If the string is longer than 3 characters or zero, quit now
			if((temp.size() > 3) || (temp.size() == 0))
			{
				result = 0;
				return false;
			}


			digits[dot_count++] = digit & 0xFF;
			temp = "";
		}

		else { temp += current_char; }
	}

	//If the dot count is less than three, something is wrong with the IP address
	if(dot_count != 4)
	{
		result = 0;
		return false;
	}

	//Encode result in network byte order aka big endian
	result = (digits[3] << 24) | (digits[2] << 16) | (digits[1] << 8) | digits[0];

	return true;
}

/****** Converts an integers IP address to a string value ******/
std::string ip_to_str(u32 ip_addr)
{
	u32 mask = 0x000000FF;
	u32 shift = 0;
	std::string temp = "";

	for(u32 x = 0; x < 4; x++)
	{
		u32 digit = (ip_addr & mask) >> shift;
		temp += to_str(digit);
		
		if(x != 3) { temp += "."; }

		shift += 8;
		mask <<= 8;
	}

	return temp;
}

/****** Gets the filename from a full or partial path ******/
std::string get_filename_from_path(std::string path)
{
	std::size_t match = path.find_last_of("/\\");

	if(match != std::string::npos) { return path.substr(match + 1); }
	else { return path; }
}

/****** Stores a list of all files in a given directory inside the referenced vector ******/
void get_files_in_dir(std::string dir_src, std::string extension, std::vector<std::string>& file_list, bool recursive, bool full_path)
{
	//Check to see if folder exists, then grab all files there (non-recursive)
	std::filesystem::path fs_path { dir_src };

	if(!std::filesystem::exists(fs_path)) { return; }

	//Check to see that the path points to a folder
	if(!std::filesystem::is_directory(fs_path)) { return; }

	//Grab files recursively
	if(recursive)
	{
		//Cycle through all available files in the directories
		std::filesystem::recursive_directory_iterator fs_files;

		//Grab filenames
		for(fs_files = std::filesystem::recursive_directory_iterator(fs_path); fs_files != std::filesystem::recursive_directory_iterator(); fs_files++)
		{
			std::string ext_check = (extension.empty()) ? "" : fs_files->path().extension().string();

			//Only grab files, not folders
			if(!fs_files->is_directory() && fs_files->is_regular_file() && (ext_check == extension))
			{
				//Grab full filename
				if(full_path) { file_list.push_back(fs_files->path().string()); }

				//Grab filename only
				else { file_list.push_back(fs_files->path().filename().string()); }
			}
		}
	}

	//Only grab files in this directory
	else
	{
		//Cycle through all available files in the directory
		std::filesystem::directory_iterator fs_files;

		//Grab filenames
		for(fs_files = std::filesystem::directory_iterator(fs_path); fs_files != std::filesystem::directory_iterator(); fs_files++)
		{
			std::string ext_check = (extension.empty()) ? "" : fs_files->path().extension().string();

			//Only grab files, not folders
			if(!fs_files->is_directory() && fs_files->is_regular_file() && (ext_check == extension))
			{
				//Grab full filename
				if(full_path) { file_list.push_back(fs_files->path().string()); }

				//Grab filename only
				else { file_list.push_back(fs_files->path().filename().string()); }
			}
		}
	}

	return;
}

/****** Stores a list of all folders in a given directory inside the referenced vector ******/
void get_folders_in_dir(std::string dir_src, std::vector<std::string>& folder_list)
{
	//Check to see if folder exists, then grab all files there (non-recursive)
	std::filesystem::path fs_path { dir_src };

	if(!std::filesystem::exists(fs_path)) { return; }

	//Check to see that the path points to a folder
	if(!std::filesystem::is_directory(fs_path)) { return; }

	//Cycle through all available files in the directory
	std::filesystem::directory_iterator fs_files;

	//Grab filenames
	for(fs_files = std::filesystem::directory_iterator(fs_path); fs_files != std::filesystem::directory_iterator(); fs_files++)
	{
		//Only grab files, not folders
		if(fs_files->is_directory())
		{
			folder_list.push_back(fs_files->path().filename().string());
		}
	}

	return;
}

/****** Removes the file extension, if any ******/
std::string get_filename_no_ext(std::string filename)
{
	std::size_t match = filename.find_last_of(".");

	if(match != std::string::npos) { return filename.substr(0, match); }
	else { return filename; }
}

/****** Loads icon into SDL Surface ******/
SDL_Surface* load_icon(std::string filename)
{
	SDL_Surface* source = SDL_LoadBMP(filename.c_str());

	if(source == NULL)
	{
		std::cout<<"GBE::Error - Could not load icon file " << filename << ". Check file path or permissions. \n";
		return NULL;
	}

	SDL_Surface* output = SDL_CreateRGBSurface(SDL_SWSURFACE, source->w, source->h, 32, 0, 0, 0, 0);

	//Cycle through all pixels, then set the alpha of all green pixels to zero
	u8* in_pixel_data = (u8*)source->pixels;
	u32* out_pixel_data = (u32*)output->pixels;

	for(int a = 0, b = 0; a < (source->w * source->h); a++, b+=3)
	{
		out_pixel_data[a] = (0xFF000000 | (in_pixel_data[b+2] << 16) | (in_pixel_data[b+1] << 8) | (in_pixel_data[b]));

		SDL_SetColorKey(output, SDL_TRUE, 0xFF00FF00);
	}

	SDL_FreeSurface(source);

	return output;
}

/****** Saves an image file as BMP or PNG ******/
bool save_image(SDL_Surface* src, std::string filename)
{
	SDL_Surface* src_copy = src;
	bool result = false;

	//Special handling for OpenGL for SDL/CLI version
	//Manually grab data by glReadPixels for SDL_Surface conversion
	if(config::use_opengl)
	{
		std::vector<u8> temp_img;
		std::vector<u8> final_img;
		temp_img.resize(config::win_width * config::win_height * 4, 0);
		glReadPixels(0, 0, config::win_width, config::win_height, GL_BGRA, GL_UNSIGNED_BYTE, temp_img.data());

		//Vertically invert pixel data before further processing
		for(s32 y = (config::win_height - 1); y >= 0; y--)
		{
			for(u32 x = 0; x < (config::win_width * 4); x++)
			{
				u32 current_pos = (y * config::win_width * 4) + x;
				final_img.push_back(temp_img[current_pos]);
			}
		}

		src_copy = SDL_CreateRGBSurface(SDL_SWSURFACE, config::win_width, config::win_height, 32, 0, 0, 0, 0);

		if(SDL_MUSTLOCK(src_copy)){ SDL_LockSurface(src_copy); }
		u8* out_pixel_data = (u8*)src_copy->pixels;

		for(u32 x = 0; x < final_img.size(); x++)
		{
			out_pixel_data[x] = final_img[x];
		}

		if(SDL_MUSTLOCK(src_copy)){ SDL_UnlockSurface(src_copy); }
	}

	#ifdef GBE_IMAGE_FORMATS
	filename += ".png";
	result = IMG_SavePNG(src_copy, filename.c_str());
	#endif
		
	#ifndef GBE_IMAGE_FORMATS
	filename += ".bmp";
	result = SDL_SaveBMP(src_copy, filename.c_str());
	#endif

	//Make sure to free surface *only* if it's a local copy!
	if(config::use_opengl) { SDL_FreeSurface(src_copy); }

	return result;
}

/****** Converts an integer into a BCD ******/
u32 get_bcd(u32 input)
{
	//Convert to a string
	std::string temp = to_str(input);

	//Convert string back into an int
	from_hex_str(temp, input);

	return input;
}

/****** Converts a BCD into an integer ******/
u32 get_bcd_int(u32 input)
{
	//Convert to a string
	std::string temp = to_hex_str(input);
	temp = temp.substr(2);

	//Convert string back into an int
	from_str(temp, input);

	return input;
}

/****** Byte swaps a 32-bit value ******/
u32 bswap(u32 input)
{
	u32 result = (input >> 24);
	result |= (((input >> 16) & 0xFF) << 8);
	result |= (((input >> 8) & 0xFF) << 16);
	result |= ((input & 0xFF) << 24);

	return result;
}

/****** Builds a .WAV header for PCM-16 audio files ******/
void build_wav_header(std::vector<u8>& header, u32 sample_rate, u32 channels, u32 data_size)
{
	header.clear();

	//Build WAV header - Chunk ID - "RIFF" in ASCII
	header.push_back(0x52);
	header.push_back(0x49);
	header.push_back(0x46);
	header.push_back(0x46);

	//Chunk Size - PCM data + 32
	header.push_back((data_size + 32) & 0xFF);
	header.push_back(((data_size + 32) >> 8) & 0xFF);
	header.push_back(((data_size + 32) >> 16) & 0xFF);
	header.push_back(((data_size + 32) >> 24) & 0xFF);

	//Wave ID - "WAVE" in ASCII
	header.push_back(0x57);
	header.push_back(0x41);
	header.push_back(0x56);
	header.push_back(0x45);

	//Chunk ID - "fmt " in ASCII
	header.push_back(0x66);
	header.push_back(0x6D);
	header.push_back(0x74);
	header.push_back(0x20);

	//Chunk Size - 16
	header.push_back(0x10);
	header.push_back(0x00);
	header.push_back(0x00);
	header.push_back(0x00);

	//Format Code - 1
	header.push_back(0x01);
	header.push_back(0x00);

	//Number of Channels - 1
	header.push_back(channels & 0xFF);
	header.push_back((channels >> 8) & 0xFF);

	//Sampling Rate
	header.push_back(sample_rate & 0xFF);
	header.push_back((sample_rate >> 8) & 0xFF);
	header.push_back((sample_rate >> 16) & 0xFF);
	header.push_back((sample_rate >> 24) & 0xFF);

	//Data Rate
	sample_rate *= (2 * channels);
	header.push_back(sample_rate & 0xFF);
	header.push_back((sample_rate >> 8) & 0xFF);
	header.push_back((sample_rate >> 16) & 0xFF);
	header.push_back((sample_rate >> 24) & 0xFF);
	
	//Block Align - 2
	header.push_back(0x02);
	header.push_back(0x00);

	//Bits per sample - 16
	header.push_back(0x10);
	header.push_back(0x00);

	//Chunk ID - "data" in ASCII
	header.push_back(0x64);
	header.push_back(0x61);
	header.push_back(0x74);
	header.push_back(0x61);

	//Chunk Size
	header.push_back(data_size & 0xFF);
	header.push_back((data_size >> 8) & 0xFF);
	header.push_back((data_size >> 16) & 0xFF);
	header.push_back((data_size >> 24) & 0xFF);
}

/****** Applies an IPS patch to a ROM loaded in memory ******/
bool patch_ips(std::string filename, std::vector<u8>& mem_map, u32 mem_pos, u32 max_size)
{
	std::ifstream patch_file(filename.c_str(), std::ios::binary);

	if(!patch_file.is_open()) 
	{ 
		std::cout<<"MMU::" << filename << " IPS patch file could not be opened. Check file path or permissions. \n";
		return false;
	}

	//Get the file size
	patch_file.seekg(0, patch_file.end);
	u32 file_size = patch_file.tellg();
	patch_file.seekg(0, patch_file.beg);

	std::vector<u8> patch_data;
	patch_data.resize(file_size, 0);

	//Read patch file into buffer
	u8* ex_patch = &patch_data[0];
	patch_file.read((char*)ex_patch, file_size);

	//Check header for PATCH string
	if((patch_data[0] != 0x50) || (patch_data[1] != 0x41) || (patch_data[2] != 0x54) || (patch_data[3] != 0x43) || (patch_data[4] != 0x48))
	{
		std::cout<<"MMU::" << filename << " IPS patch file has invalid header\n";
		return false;
	}

	bool end_of_file = false;
	u32 patch_pos = 5;

	while((patch_pos < file_size) && (!end_of_file))
	{
		//Grab a record offset - 3 bytes
		if((patch_pos + 3) > file_size)
		{
			std::cout<<"MMU::" << filename << " file ends unexpectedly (OFFSET). Aborting further patching.\n";
		}

		u32 offset = (patch_data[patch_pos++] << 16) | (patch_data[patch_pos++] << 8) | patch_data[patch_pos++];

		//Quit if EOF marker is reached
		if(offset == 0x454F46) { end_of_file = true; break; }

		//Grab record size - 2 bytes
		if((patch_pos + 2) > file_size)
		{
			std::cout<<"MMU::" << filename << " file ends unexpectedly (DATA_SIZE). Aborting further patching.\n";
			return false;
		}

		u16 data_size = (patch_data[patch_pos++] << 8) | patch_data[patch_pos++];

		//Perform regular patching if size is non-zero
		if(data_size)
		{
			if((patch_pos + data_size) > file_size)
			{
				std::cout<<"MMU::" << filename << " file ends unexpectedly (DATA). Aborting further patching.\n";
				return false;
			}

			for(u32 x = 0; x < data_size; x++)
			{
				u8 patch_byte = patch_data[patch_pos++];

				if((mem_pos + offset) > max_size)
				{
					std::cout<<"MMU::" << filename << "patches beyond max ROM size. Aborting further patching.\n";
					return false;
				}

				mem_map[mem_pos + offset] = patch_byte;

				offset++;
			}
		}

		//Patch with RLE
		else
		{
			//Grab Run-length size and value - 3 bytes
			if((patch_pos + 3) > file_size)
			{
				std::cout<<"MMU::" << filename << " file ends unexpectedly (RLE). Aborting further patching.\n";
				return false;
			}

			u16 rle_size = (patch_data[patch_pos++] << 8) | patch_data[patch_pos++];
			u8 patch_byte = patch_data[patch_pos++];

			for(u32 x = 0; x < rle_size; x++)
			{
				if((mem_pos + offset) > max_size)
				{
					std::cout<<"MMU::" << filename << "patches beyond max ROM size. Aborting further patching.\n";
					return false;
				}

				mem_map[mem_pos + offset] = patch_byte;

				offset++;
			}
		}
	}

	patch_file.close();
	patch_data.clear();

	return true;
}

/****** Applies an UPS patch to a ROM loaded in memory ******/
bool patch_ups(std::string filename, std::vector<u8>& mem_map, u32 mem_pos, u32 max_size)
{

	std::ifstream patch_file(filename.c_str(), std::ios::binary);

	if(!patch_file.is_open()) 
	{ 
		std::cout<<"MMU::" << filename << " UPS patch file could not be opened. Check file path or permissions. \n";
		return false;
	}

	//Get the file size
	patch_file.seekg(0, patch_file.end);
	u32 file_size = patch_file.tellg();
	patch_file.seekg(0, patch_file.beg);

	std::vector<u8> patch_data;
	patch_data.resize(file_size, 0);

	//Read patch file into buffer
	u8* ex_patch = &patch_data[0];
	patch_file.read((char*)ex_patch, file_size);

	//Check header for UPS1 string
	if((patch_data[0] != 0x55) || (patch_data[1] != 0x50) || (patch_data[2] != 0x53) || (patch_data[3] != 0x31))
	{
		std::cout<<"MMU::" << filename << " UPS patch file has invalid header\n";
		return false;
	}

	u32 patch_pos = 4;
	u32 patch_size = file_size - 12;
	u32 file_pos = 0;

	//Grab file sizes
	for(u32 x = 0; x < 2; x++)
	{
		//Grab variable width integer
		u32 var_int = 0;
		bool var_end = false;
		u8 var_shift = 0;

		while(!var_end)
		{
			//Grab byte from patch file
			u8 var_byte = patch_data[patch_pos++];
			
			if(var_byte & 0x80)
			{
				var_int += ((var_byte & 0x7F) << var_shift);
				var_end = true;
			}

			else
			{
				var_int += ((var_byte | 0x80) << var_shift);
				var_shift += 7;
			}
		}
	}

	//Begin patching the source file
	while(patch_pos < patch_size)
	{
		//Grab variable width integer
		u32 var_int = 0;
		bool var_end = false;
		u8 var_shift = 0;

		while(!var_end)
		{
			//Grab byte from patch file
			u8 var_byte = patch_data[patch_pos++];
			
			if(var_byte & 0x80)
			{
				var_int += ((var_byte & 0x7F) << var_shift);
				var_end = true;
			}

			else
			{
				var_int += ((var_byte | 0x80) << var_shift);
				var_shift += 7;
			}
		}

		//XOR data at offset with patch
		var_end = false;
		file_pos += var_int;

		while(!var_end)
		{
			//Abort if patching greater than ROM size
			if(file_pos > max_size)
			{
				std::cout<<"MMU::" << filename << "patches beyond max ROM size. Aborting further patching.\n";
				return false;
			}

			u8 patch_byte = patch_data[patch_pos++];

			//Terminate patching for this chunk if encountering a zero byte
			if(patch_byte == 0) { var_end = true; }

			//Otherwise, use the byte to patch
			else
			{
				mem_map[mem_pos + file_pos] ^= patch_byte;
			}

			file_pos++;
		}
	}

	patch_file.close();
	patch_data.clear();

	return true;
}

} //Namespace
