// EXT_DATA_USAGE_0
// GB Enhanced+ Copyright Daniel Baxter 2018
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : dmg_mode.fs
// Date : March 17, 2018
// Description : GBE+ Fragment Shader - DMG Mode
//
// Makes everything look black and white/shades of gray
// Like Grayscale, but reduces everything to 4 colors max

#version 330 core
in vec2 texture_coordinates;

out vec4 color;

uniform sampler2D screen_texture;
uniform int screen_x_size;
uniform int screen_y_size;

uniform float ext_data_1;
uniform float ext_data_2;

//Get perceived brightness, aka luma
void get_brightness(in vec4 input_color, out float luma)
{
	luma = (input_color.r * 0.2126) + (input_color.g * 0.7152) + (input_color.b * 0.0722);
}

void main()
{
	vec4 output_color = texture(screen_texture, texture_coordinates);

	float b_val = 0.0;
	get_brightness(output_color, b_val);

	//Color 0 - Black
	if(b_val < 0.25)
	{
		output_color.r = 0;
		output_color.g = 0;
		output_color.b = 0;
	}

	//Color 1 - 2nd darkest
	else if(b_val < 0.50)
	{
		output_color.r = 0.33;
		output_color.g = 0.33;
		output_color.b = 0.33;
	}

	//Color 2 - 2nd lightest
	else if(b_val < 0.75)
	{
		output_color.r = 0.66;
		output_color.g = 0.66;
		output_color.b = 0.66;
	}

	//Color 3 - White
	else
	{
		output_color.r = 1.0;
		output_color.g = 1.0;
		output_color.b = 1.0;
	}

	color = output_color;
}
