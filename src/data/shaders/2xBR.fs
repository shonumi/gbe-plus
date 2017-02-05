// EXT_DATA_USAGE_1
// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : 2xBR.fs
// Date : November 12, 2016
// Description : GBE+ Fragment Shader - 2xBR
//
// High-quality pixel scaling via xBR

#version 330 core
in vec2 texture_coordinates;

out vec4 color;

uniform sampler2D screen_texture;
uniform int screen_x_size;
uniform int screen_y_size;

uniform float ext_data_1;
uniform float ext_data_2;

//Get the weighted "distance" between two colors
float color_dist(in vec4 color_1, in vec4 color_2)
{
	float r, g, b = 0.0;
	float y, u, v = 0.0;

	r = abs(color_1.r - color_2.r);
	g = abs(color_1.g - color_2.g);
	b = abs(color_1.b - color_2.b);

	y = abs((0.299 * r) + (0.587 * g) + (0.114 * b));
   	u = abs((-0.169 * r) - (0.331 * g) + (0.500 * b));
   	v = abs((0.500 * r) - (0.419 * g) - (0.081 * b));

	float color_distance = (y * 48) + (u * 7) + (v * 6);
	return color_distance;
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

void main()
{
	vec2 current_pos = texture_coordinates;
	vec2 next_color_pos;
	vec4 current_color = texture(screen_texture, texture_coordinates);

	bool color_pass = true;

	//Colors for surrounding pixels
	vec4 a1 = current_color; 
	vec4 b1 = current_color;
	vec4 c1 = current_color;
	vec4 a0 = current_color;
	vec4 a = current_color;
	vec4 b = current_color; 
	vec4 c = current_color; 
	vec4 c4 = current_color;
	vec4 d0 = current_color;
	vec4 d = current_color;
	vec4 e = current_color;
	vec4 f = current_color;
	vec4 f4 = current_color;
	vec4 g0 = current_color;
	vec4 g = current_color;
	vec4 h = current_color;
	vec4 i = current_color;
	vec4 i4 = current_color;
	vec4 g5 = current_color;
	vec4 h5 = current_color;
	vec4 i5 = current_color;

	color_pass = get_texel(-1.0, 2.0, a1, color_pass);
	color_pass = get_texel(0.0, 2.0, b1, color_pass);
	color_pass = get_texel(1.0, 2.0, c1, color_pass);

	color_pass = get_texel(-2.0, 1.0, a0, color_pass);
	color_pass = get_texel(-1.0, 1.0, a, color_pass);
	color_pass = get_texel(0.0, 1.0, b, color_pass);
	color_pass = get_texel(1.0, 1.0, c, color_pass);
	color_pass = get_texel(2.0, 1.0, c4, color_pass);

	color_pass = get_texel(-2.0, 0.0, d0, color_pass);
	color_pass = get_texel(-1.0, 0.0, d, color_pass);
	color_pass = get_texel(1.0, 0.0, f, color_pass);
	color_pass = get_texel(2.0, 0.0, f4, color_pass);

	color_pass = get_texel(-2.0, -1.0, g0, color_pass);
	color_pass = get_texel(-1.0, -1.0, g, color_pass);
	color_pass = get_texel(0.0, -1.0, h, color_pass);
	color_pass = get_texel(1.0, -1.0, i, color_pass);
	color_pass = get_texel(2.0, -1.0, i4, color_pass);

	color_pass = get_texel(-1.0, -2.0, g5, color_pass);
	color_pass = get_texel(0.0, -2.0, h5, color_pass);
	color_pass = get_texel(1.0, -2.0, i5, color_pass);

	float red_weight = 0.0;
	float blue_weight = 0.0;

	//Process 2x2 expansion
	if(color_pass == true)
	{
		//Determine which quadrant this is, E0, E1, E2, or E3
		float quad_x = (1.0 / screen_x_size) / 2.0;
		float quad_y = (1.0 / screen_y_size) / 2.0;

		float texel_x = (current_pos.x / quad_x);
		texel_x = mod(texel_x, 2.0);

		float texel_y = (current_pos.y / quad_y);
		texel_y = mod(texel_y, 2.0);
		
		int quadrant = -1;

		//E0
		if((texel_x <= 1.0) && (texel_y <= 1.0))
		{
			red_weight = color_dist(e, g) + color_dist(e, c) + color_dist(a, d0) + color_dist(a, b1) + (4 * color_dist(d, b));
			blue_weight = color_dist(d, h) + color_dist(d, a0) + color_dist(b, f) + color_dist(b, a1) + (4 * color_dist(e, a));

			if(red_weight < blue_weight)
			{
				if(color_dist(e, d) <= color_dist(e, b)) { current_color = rgb_blend(e, d); }
				else { current_color = rgb_blend(e, b); }
			}
		}

		//E1
		else if((texel_x > 1.0) && (texel_y <= 1.0))
		{
			red_weight = color_dist(e, i) + color_dist(e, a) + color_dist(c, b1) + color_dist(c, f4) + (4 * color_dist(b, f));
			blue_weight = color_dist(f, h) + color_dist(f, c4) + color_dist(d, b) + color_dist(b, c1) + (4 * color_dist(e, c));

			if(red_weight < blue_weight)
			{
				if(color_dist(e, b) <= color_dist(e, f)) { current_color = rgb_blend(e, b); }
				else { current_color = rgb_blend(e, f); }
			}
		}

		//E2
		else if((texel_x <= 1.0) && (texel_y > 1.0))
		{
			red_weight = color_dist(e, a) + color_dist(e, i) + color_dist(g, d0) + color_dist(g, h5) + (4 * color_dist(d, h));
			blue_weight = color_dist(d, b) + color_dist(d, g0) + color_dist(f, h) + color_dist(h, g5) + (4 * color_dist(e, g));

			if(red_weight < blue_weight)
			{
				if(color_dist(e, d) <= color_dist(e, h)) { current_color = rgb_blend(e, d); }
				else { current_color = rgb_blend(e, h); }
			}
		}

		//E3
		else if((texel_x > 1.0) && (texel_y > 1.0))
		{
			red_weight = color_dist(e, c) + color_dist(e, g) + color_dist(i, f4) + color_dist(i, h5) + (4 * color_dist(f, h));
			blue_weight = color_dist(d, h) + color_dist(h, i5) + color_dist(h, i4) + color_dist(b, f) + (4 * color_dist(e, i));

			if(red_weight < blue_weight)
			{
				if(color_dist(e, f) <= color_dist(e, h)) { current_color = rgb_blend(e, f); }
				else { current_color = rgb_blend(e, h); }
			}
		}
	}

	color = current_color;
}
