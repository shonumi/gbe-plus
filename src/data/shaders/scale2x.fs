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
		float quad_x = (0.5 / screen_x_size);
		float quad_y = (0.5 / screen_y_size);

		float texel_x = (current_pos.x / quad_x);
		texel_x = mod(texel_x, 2.0);

		float texel_y = (current_pos.y / quad_y);
		texel_y = mod(texel_y, 2.0);
		
		int quadrant = -1;

		//E0
		if((texel_x <= 1.0) && (texel_y <= 1.0))
		{
			if(left_color == top_color) { current_color = left_color; }
			quadrant = 0;
		}

		//E1
		else if((texel_x > 1.0) && (texel_y <= 1.0))
		{
			if(right_color == top_color) { current_color = right_color; }
			quadrant = 1;
		}

		//E2
		else if((texel_x <= 1.0) && (texel_y > 1.0))
		{
			if(left_color == bottom_color) { current_color = left_color; }
			quadrant = 2;
		}

		//E3
		else if((texel_x > 1.0) && (texel_y > 1.0))
		{
			if(right_color == bottom_color) { current_color = right_color; }
			quadrant = 3;
		}

		//Calculate anti-aliasing if applicable
		if(enable_anti_aliasing)
		{
			//Regardless of what quadrant this is, calculate E0 - E3
			vec4 e0 = texture(screen_texture, texture_coordinates);
			vec4 e1 = e0;
			vec4 e2 = e0;
			vec4 e3 = e0;

			if(left_color == top_color) { e0 = left_color; }
			if(right_color == top_color) { e1 = right_color; }
			if(left_color == bottom_color) { e2 = left_color; }
			if(right_color == bottom_color) { e3 = right_color; }

			//Blend E0 if E1 and E2 are equal
			if((quadrant == 0) && (e1 == e2)) { current_color = mix(e0, e1, 0.5); }

			//Blend E1 if E0 and E3 are equal
			else if((quadrant == 1) && (e0 == e3)) { current_color = mix(e1, e0, 0.5); }

			//Blend E2 if E0 and E3 are equal
			else if((quadrant == 2) && (e0 == e3)) { current_color = mix(e2, e0, 0.5); }

			//Blend E3 if E1 and E2 are equal
			else if((quadrant == 3) && (e1 == e2)) { current_color = mix(e3, e1, 0.5); }
		}
	}

	color = current_color;
}
