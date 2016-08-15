// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : sepia.fs
// Date : August 13, 2016
// Description : GBE+ Fragment Shader - Sepia
//
// Makes everything look like an old photograph

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

	output_color.r = (input_color.r * 0.393) + (input_color.g * 0.769) + (input_color.b * 0.189);
	output_color.g = (input_color.r * 0.349) + (input_color.g * 0.686) + (input_color.b * 0.168);
	output_color.b = (input_color.r * 0.272) + (input_color.g * 0.534) + (input_color.b * 0.131);

	color = output_color;
}
