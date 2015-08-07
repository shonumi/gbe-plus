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

} //Namespace
