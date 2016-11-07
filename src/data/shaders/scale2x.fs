// EXT_DATA_USAGE_1
// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : scale2x.fs
// Date : November 05, 2016
// Description : GBE+ Fragment Shader - Scale2x
//
// Applies Scale2x pixel scaling filter

#version 330 core
in vec2 texture_coordinates;

out vec4 color;

uniform sampler2D screen_texture;
uniform int screen_x_size;
uniform int screen_y_size;

uniform float ext_data_1;
uniform float ext_data_2;

//Grabs surrounding texel from current position
bool get_texel(in float x_shift, in float y_shift, out vec4 texel_color, in bool c_pass)
{
	bool pass = c_pass;

	float tex_x = 1.0 / screen_x_size;
	float tex_y = 1.0 / screen_y_size;

	vec2 src_pos = texture_coordinates;

	//Shift texel position in X direction
	src_pos.x += (tex_x * x_shift);
	if((src_pos.x > 1.0) || (src_pos.x < 0.0)) { pass = false; }

	//Shift texel position in Y direction
	src_pos.y += (tex_y * -y_shift);
	if((src_pos.y > 1.0) || (src_pos.y < 0.0)) { pass = false; }

	texel_color = texture(screen_texture, src_pos);

	return pass;
}

//Blend two colors
vec4 rgb_blend(in vec4 color_1, in vec4 color_2)
{
	vec4 final_color = vec4(1.0, 1.0, 1.0, 1.0);
	final_color.r = (color_1.r + color_2.r) / 2.0;
	final_color.g = (color_1.g + color_2.g) / 2.0;
	final_color.b = (color_1.b + color_2.b) / 2.0;
	return final_color;
}

void main()
{
	vec2 current_pos = texture_coordinates;
	vec4 current_color = texture(screen_texture, texture_coordinates);

	bool color_pass = true;

	vec4 top_color = current_color;
	vec4 bottom_color = current_color;
	vec4 left_color = current_color;
	vec4 right_color = current_color;

	//Grab texel above
	color_pass = get_texel(0.0, 1.0, top_color, color_pass);

	//Grab texel below
	color_pass = get_texel(0.0, -1.0, bottom_color, color_pass);

	//Grab texel left
	color_pass = get_texel(-1.0, 0.0, left_color, color_pass);

	//Grab texel right
	color_pass = get_texel(1.0, 0.0, right_color, color_pass);

	if((color_pass == true) && (top_color != bottom_color) && (left_color != right_color))
	{
		//Determine which quadrant this is, E0, E1, E2, or E3
		float quad_x = (1.0 / screen_x_size) / 2.0;
		float quad_y = (1.0 / screen_y_size) / 2.0;

		float texel_x = (current_pos.x / quad_x);
		texel_x = mod(texel_x, 2.0);

		float texel_y = (current_pos.y / quad_y);
		texel_y = mod(texel_y, 2.0);

		//E0
		if((texel_x <= 1.0) && (texel_y <= 1.0))
		{
			if(left_color == top_color) { current_color = left_color; }
		}

		//E1
		else if((texel_x > 1.0) && (texel_y <= 1.0))
		{
			if(right_color == top_color) { current_color = right_color; }
		}

		//E2
		else if((texel_x <= 1.0) && (texel_y > 1.0))
		{
			if(left_color == bottom_color) { current_color = left_color; }
		}

		//E3
		else if((texel_x > 1.0) && (texel_y > 1.0))
		{
			if(right_color == bottom_color) { current_color = right_color; }
		}
	}

	color = current_color;
}
