// EXT_DATA_USAGE_0
// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : fragment.fs
// Date : August 13, 2016
// Description : GBE+ Fragment Shader
//
// Default fragment shader

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
	color = texture(screen_texture, texture_coordinates);
}
