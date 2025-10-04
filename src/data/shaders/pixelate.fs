// EXT_DATA_USAGE_0
// GB Enhanced+ Copyright Daniel Baxter 2025
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : pixelate.fs
// Date : October 4, 2025
// Description : GBE+ Fragment Shader - 2x2 Pixelation

#version 330 core
in vec2 texture_coordinates;

out vec4 color;

uniform sampler2D screen_texture;
uniform int screen_x_size;
uniform int screen_y_size;

uniform float ext_data_1;
uniform float ext_data_2;

//Grabs surrounding texel from current position
vec4 get_texel(in float x_shift, in float y_shift)
{
	float tex_x = 1.0 / screen_x_size;
	float tex_y = 1.0 / screen_y_size;

	vec2 src_pos = texture_coordinates;

	//Shift texel position in XY directions
	src_pos.x += (tex_x * x_shift);
	src_pos.y += (tex_y * -y_shift);

	return texture(screen_texture, src_pos);
}

void main()
{
	vec2 current_pos = texture_coordinates;
	vec4 current_color = texture(screen_texture, texture_coordinates);

	int pixel_x = int(current_pos.x * screen_x_size) % 2;
	int pixel_y = int(current_pos.y * screen_y_size) % 2;

	float x_offset = float(2.0 - pixel_x);
	float y_offset = float(2.0 - pixel_y);

	color = get_texel(x_offset, y_offset);
}
