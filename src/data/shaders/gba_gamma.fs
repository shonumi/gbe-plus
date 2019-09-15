// EXT_DATA_USAGE_0
// GB Enhanced+ Copyright Daniel Baxter 2019
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gba_gamma.fs
// Date : September 14, 2019
// Description : GBE+ Fragment Shader - GBA Gamma
//
// Adjusts the gamma level for GBA games
// Color correction formula courtesy of Talarubi and byuu

#version 330 core
in vec2 texture_coordinates;

out vec4 color;

uniform sampler2D screen_texture;
uniform int screen_x_size;
uniform int screen_y_size;

uniform float ext_data_1;
uniform float ext_data_2;

float lcd_gamma = 4.0;
float out_gamma = 2.2;

void main()
{
	vec4 output_color = texture(screen_texture, texture_coordinates);

	//Convert GLSL RGB floats to RGB555
	float r = (output_color.r * 255.0) / 8.0;
	float g = (output_color.g * 255.0) / 8.0;
	float b = (output_color.b * 255.0) / 8.0;

	float lr = pow((r / 31.0), lcd_gamma);
	float lg = pow((g / 31.0), lcd_gamma);
	float lb = pow((b / 31.0), lcd_gamma);

	r = pow( (((0.0 * lb) + (50.0 * lg) + (255.0 * lr)) / 255.0), (1.0 / out_gamma) );
	g = pow( (((30.0 * lb) + (230.0 * lg) + (10.0 * lr)) / 255.0), (1.0 / out_gamma) );
	b = pow( (((220.0 * lb) + (10.0 * lg) + (50.0 * lr)) / 255.0), (1.0 / out_gamma) );

	output_color.r = r;
	output_color.g = g;
	output_color.b = b;

	color = output_color;
}
