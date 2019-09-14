// EXT_DATA_USAGE_0
// GB Enhanced+ Copyright Daniel Baxter 2019
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gba_gamma.fs
// Date : September 13, 2019
// Description : GBE+ Fragment Shader - GBC Gamma
//
// Adjusts the gamma level for GBC games
// Color correction formula courtesy of byuu

#version 330 core
in vec2 texture_coordinates;

out vec4 color;

uniform sampler2D screen_texture;
uniform int screen_x_size;
uniform int screen_y_size;

uniform float ext_data_1;
uniform float ext_data_2;

void main()
{
	vec4 output_color = texture(screen_texture, texture_coordinates);

	//Convert GLSL RGB floats to RGB555
	int r = int(output_color.r * 255.0);
	int g = int(output_color.g * 255.0);
	int b = int(output_color.b * 255.0);

	r /= 8;
	g /= 8;
	b /= 8;

	int rf = (r * 26) + (g * 4) + (b * 2);
	int gf = (g * 24) + (b * 8);
	int bf = (r * 6) + (g * 4) + (b * 22);

	rf = min(960, rf) / 4;
	gf = min(960, gf) / 4;
	bf = min(960, bf) / 4;

	float ri = float(rf);
	float gi = float(gf);
	float bi = float(bf);

	output_color.r = (ri / 255.0);
	output_color.b = (bi / 255.0);
	output_color.g = (gi / 255.0);

	color = output_color;
}
