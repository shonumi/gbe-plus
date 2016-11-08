// EXT_DATA_USAGE_1
// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : scale3x.fs
// Date : November 07, 2016
// Description : GBE+ Fragment Shader - Scale3x
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

//Control variables - Adjust these to change how the shader's effects work

//Turns on anti-aliasing to enhance the original Scale2x algorithm
bool enable_anti_aliasing = false;

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

	vec4 a, b, c;
	vec4 d, e, f;
	vec4 g, h, i;

	//Grab top texels
	color_pass = get_texel(-1.0, 1.0, a, color_pass);
	color_pass = get_texel(0.0, 1.0, b, color_pass);
	color_pass = get_texel(1.0, 1.0, c, color_pass);

	//Grab middle texels
	color_pass = get_texel(-1.0, 0.0, d, color_pass);
	color_pass = get_texel(0.0, 0.0, e, color_pass);
	color_pass = get_texel(1.0, 0.0, f, color_pass);

	//Grab bottom texels
	color_pass = get_texel(-1.0, -1.0, g, color_pass);
	color_pass = get_texel(0.0, -1.0, h, color_pass);
	color_pass = get_texel(1.0, -1.0, i, color_pass);

	if((color_pass == true) && (b != h) && (d != f))
	{
		//Determine which section this is, E0...E8
		float quad_x = (1.0 / screen_x_size) / 3.0;
		float quad_y = (1.0 / screen_y_size) / 3.0;

		float texel_x = (current_pos.x / quad_x);
		texel_x = mod(texel_x, 3.0);

		float texel_y = (current_pos.y / quad_y);
		texel_y = mod(texel_y, 3.0);
		
		int section = -1;

		//1st column, E0, E3, E6
		if(texel_x <= 1.0)
		{
			//E0
			if(texel_y <= 1.0)
			{
				if(d == b) { current_color = d; }
			}

			//E3
			else if(texel_y <= 2.0)
			{
				if((d == b) && (e != g)) { current_color = d; }
				else if((d == h) && (e != a)) { current_color = d; }

			}

			//E6
			else
			{
				if(d == h) { current_color = d; }
			}
		}

		//2nd column, E1, E4, E7
		else if(texel_x <= 2.0)
		{
			//E1
			if(texel_y <= 1.0)
			{
				if((d == b) && (e != c)) { current_color = b; }
				else if((b == f) && (e != a)) { current_color = b; }
			}

			//E4 - nothing to do here
			else if(texel_y <= 2.0) { }

			//E7
			else
			{
				if((d == h) && (e != i)) { current_color = h; }
				else if((h == f) && (e != g)) { current_color = h; }
			}
		}

		//3rd column, E2, E5, E8
		else
		{
			//E2
			if(texel_y <= 1.0)
			{
				if(b == f) { current_color = b; }
			}

			//E5
			else if(texel_y <= 2.0)
			{
				if((b == f) && (e == i)) { current_color = f; }
				else if((h == f) && (e != c)) { current_color = f; }
			}

			//E8
			else
			{
				if(h == f) { current_color = f; }
			}
		}
	}

	color = current_color;
}
