// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : grayscale.fs
// Date : August 14, 2016
// Description : GBE+ Fragment Shader - Grayscale
//
// Makes everything look black and white/shades of gray

#version 330 core
in vec2 texture_coordinates;

out vec4 color;

uniform sampler2D screen_texture;
uniform int screen_x_size;
uniform int screen_y_size;

void main()
{
	vec4 input_color = texture(screen_texture, texture_coordinates);
	vec4 output_color = vec4(0, 0, 0, 0);

	output_color.r = (input_color.r * 0.21) + (input_color.g * 0.72) + (input_color.b * 0.07);
	output_color.g = output_color.r;
	output_color.b = output_color.r;

	color = output_color;
}
