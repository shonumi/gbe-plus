// GB Enhanced+ Copyright Daniel Baxter 2018
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : osd.cpp
// Date : April 14, 2018
// Description : GBE+ On-Screen Display
//
// Loads font for OSD messages. Draws OSD messages to a given SDL surface
// Also loads virtual cursor used for NDS input

#include <iostream>
#include <fstream>

#include "config.h"

/****** Loads a font into a buffer to be drawn by a core for OSD ******/
bool load_osd_font()
{
	//Should be called after parse_ini_file() to get correct data path
	std::string font_file = config::data_path + "fonts/block_font.bin";

	std::ifstream file(font_file.c_str(), std::ios::binary);

	if(!file.is_open())
	{
		std::cout<<"GBE::Could not open font file " << font_file << ". Check file path or permissions. \n";
		config::use_osd = false;
		return false; 
	}

	//Get the file size
	file.seekg(0, file.end);
	u32 file_size = file.tellg();
	file.seekg(0, file.beg);

	u8 font_byte = 0;
	u32 font_size = file_size / 64;
	u32 buffer_pos = 0;
	
	//Resize font buffer
	config::osd_font.resize(font_size * 64);

	//Expand 1BPP font to 32-bit ARGB
	for(u32 x = 0; x < font_size; x++)
	{
		for(u32 height = 0; height < 8; height++)
		{
			for(u32 width = 0; width < 8; width++)
			{
				file.read((char*)&font_byte, 0x1);
								
				buffer_pos = (x * 64) + (height * 8) + width;

				//Draw font to buffer (yellow color)
				config::osd_font[buffer_pos] = (font_byte) ? 0xFFFFE500 : 0xFF000000;
			}
		}
	}

	file.close();

	std::cout<<"GBE::Loaded OSD font\n";

	return true;
} 

/****** Draws an OSD message onto a given buffer ******/
void draw_osd_msg(std::string osd_text, std::vector <u32> &osd_surface, u8 x_offset, u8 y_offset)
{
	//Abort OSD drawing if 1) OSD disabled, 2) message size is zero, 3) given buffer is less than 20 8x8 tiles, 4) X offset is >= 20
	if(!config::use_osd) { return; }
	if(osd_text.size() == 0) { return; }
	if(osd_surface.size() < 1280) { return; }
	if(x_offset > ((config::sys_width / 8) - 1)) { return; }

	u32 chr_offset = 0;
	u32 buffer_pos = 0;
	u32 chr_pos = 0;
	u8 current_chr = 0;

	//Limit message size to 20 characters.
	u8 message_size = (osd_text.size() <= 20) ? osd_text.size() : 20;

	//Cycle through every character
	for(u32 x = 0; x < message_size; x++)
	{
		current_chr = osd_text[x];

		//Convert ASCII text to font offsets
		if(current_chr == 0x20) { chr_offset = 0; }
		if(current_chr == 0x2A) { chr_offset = 37; }
		else if((current_chr >= 0x30) && (current_chr <= 0x39)) { chr_offset = (current_chr - 0x2F); }
		else if((current_chr >= 0x41) && (current_chr <= 0x5A)) { chr_offset = (current_chr - 0x36); }
		else if((current_chr >= 0x61) && (current_chr <= 0x7A)) { chr_offset = (current_chr - 0x56); }
		else { chr_offset = 0; }

		for(u32 osd_h = 0; osd_h < 8; osd_h++)
		{
			for(u32 osd_w = 0; osd_w < 8; osd_w++)
			{
				if(((x_offset * 8) + (x * 8)) < config::sys_width)
				{
					buffer_pos = (x * 8) + (osd_h * config::sys_width) + osd_w;
					buffer_pos += (x_offset * 8);
					buffer_pos += (y_offset * 8 * config::sys_width);

					chr_pos = (chr_offset * 64) + (osd_h * 8) + osd_w;

					if(buffer_pos < osd_surface.size()) { osd_surface[buffer_pos] = config::osd_font[chr_pos]; }
				}
			}
		}
	}
}

/****** Draws an OSD message onto a given buffer that is not same size as game screen ******/
void draw_osd_msg(std::string osd_text, std::vector <u32> &osd_surface, u8 x_offset, u8 y_offset, u32 width)
{
	//Abort OSD drawing if 1) OSD disabled, 2) message size is zero, 3) given buffer is less than 20 8x8 tiles, 4) X offset is >= 20
	if(!config::use_osd) { return; }
	if(osd_text.size() == 0) { return; }
	if(osd_surface.size() < 1280) { return; }
	if(x_offset > ((width / 8) - 1)) { return; }

	u32 chr_offset = 0;
	u32 buffer_pos = 0;
	u32 chr_pos = 0;
	u8 current_chr = 0;

	//Limit message size to 20 characters.
	u8 message_size = (osd_text.size() <= 20) ? osd_text.size() : 20;

	//Cycle through every character
	for(u32 x = 0; x < message_size; x++)
	{
		current_chr = osd_text[x];

		//Convert ASCII text to font offsets
		if(current_chr == 0x20) { chr_offset = 0; }
		if(current_chr == 0x2A) { chr_offset = 37; }
		else if((current_chr >= 0x30) && (current_chr <= 0x39)) { chr_offset = (current_chr - 0x2F); }
		else if((current_chr >= 0x41) && (current_chr <= 0x5A)) { chr_offset = (current_chr - 0x36); }
		else if((current_chr >= 0x61) && (current_chr <= 0x7A)) { chr_offset = (current_chr - 0x56); }
		else { chr_offset = 0; }

		for(u32 osd_h = 0; osd_h < 8; osd_h++)
		{
			for(u32 osd_w = 0; osd_w < 8; osd_w++)
			{
				if(((x_offset * 8) + (x * 8)) < width)
				{
					buffer_pos = (x * 8) + (osd_h * width) + osd_w;
					buffer_pos += (x_offset * 8);
					buffer_pos += (y_offset * 8 * width);

					chr_pos = (chr_offset * 64) + (osd_h * 8) + osd_w;

					if(buffer_pos < osd_surface.size()) { osd_surface[buffer_pos] = config::osd_font[chr_pos]; }
				}
			}
		}
	}
}

/****** Loads an 8x8 BMP used as the NDS virtual cursor ******/
bool load_virtual_cursor()
{
	//Should be called after parse_ini_file() to get correct data path
	SDL_Surface* source = SDL_LoadBMP(config::vc_file.c_str());

	if(source == NULL)
	{
		//If the user-specified virtual cursor file cannot be opened, use the default
		std::string cursor_file = config::data_path + "misc/vc_default.bmp";
		source = SDL_LoadBMP(cursor_file.c_str());

		//If virtual cursor file still cannot be opened, abort
		if(source == NULL)
		{
			std::cout<<"GBE::Could not load any Virtual Cursor files\n";
			config::vc_enable = false;
			return false;
		}
	}

	//Reset Virtual Cursor data
	config::vc_data.clear();

	u8* pixel_data = (u8*)source->pixels;

	//Copy 32-bit pixel data to VC data
	for(int a = 0, b = 0; a < (source->w * source->h); a++, b+=3)
	{
		config::vc_data.push_back(0xFF000000 | (pixel_data[b+2] << 16) | (pixel_data[b+1] << 8) | (pixel_data[b]));
	}

	//Check final size
	if(config::vc_data.size() != 64)
	{
		std::cout<<"GBE::Virtual Cursor image is incorrect size\n";
		config::vc_enable = false;
		return false;
	}

	std::cout<<"GBE::Loaded Virtual Cursor file\n";

	return true;
}
