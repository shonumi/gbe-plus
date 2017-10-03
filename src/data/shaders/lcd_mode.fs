// EXT_DATA_USAGE_1
// GB Enhanced+ Copyright Daniel Baxter 2017
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : lcd_mode.fs
// Date : October 2, 2017
// Description : GBE+ Fragment Shader - LCD Mode
//
// Cheap, crappy LCD-like effect

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
	vec2 current_pos = texture_coordinates;
	vec4 current_color = texture(screen_texture, texture_coordinates);

	//Screen ratios
	float x_size = (1.0 / screen_x_size);
	float y_size = (1.0 / screen_y_size);

	float scr_x = (ext_data_1 / screen_x_size);
	float scr_y = (ext_data_2 / screen_y_size);

	float screen_x_mod = mod(current_pos.x, x_size);
	float screen_y_mod = mod(current_pos.y, y_size);

	if(screen_x_mod < (x_size/scr_x))
	{
		current_color.r = 0.0;
		current_color.g = 0.0;
		current_color.b = 0.0;
	}

	if(screen_y_mod < (y_size/scr_y))
	{
		current_color.r = 0.0;
		current_color.g = 0.0;
		current_color.b = 0.0;
	}

	color = current_color;
}
