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

//Turns on anti-aliasing to enhance the original Scale3x algorithm
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
				section = 0;
			}

			//E3
			else if(texel_y <= 2.0)
			{
				if((d == b) && (e != g)) { current_color = d; }
				else if((d == h) && (e != a)) { current_color = d; }
				section = 3;
			}

			//E6
			else
			{
				if(d == h) { current_color = d; }
				section = 6;
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
				section = 1;
			}

			//E4 - nothing to do here
			else if(texel_y <= 2.0) { }

			//E7
			else
			{
				if((d == h) && (e != i)) { current_color = h; }
				else if((h == f) && (e != g)) { current_color = h; }
				section = 7;
			}
		}

		//3rd column, E2, E5, E8
		else
		{
			//E2
			if(texel_y <= 1.0)
			{
				if(b == f) { current_color = b; }
				section = 2;
			}

			//E5
			else if(texel_y <= 2.0)
			{
				if((b == f) && (e == i)) { current_color = f; }
				else if((h == f) && (e != c)) { current_color = f; }
				section = 5;
			}

			//E8
			else
			{
				if(h == f) { current_color = f; }
				section = 8;
			}
		}

		//Calculate anti-aliasing if applicable
		if(enable_anti_aliasing)
		{
			//Regardless of what section this is, calculate E0 - E8
			vec4 e0 = texture(screen_texture, texture_coordinates);
			vec4 e1 = e0;
			vec4 e2 = e0;
			vec4 e3 = e0;
			vec4 e4 = e0;
			vec4 e5 = e0;
			vec4 e6 = e0;
			vec4 e7 = e0;
			vec4 e8 = e0;

			//E0
			if(d == b) { e0 = d; }

			//E1
			if((d == b) && (e != c)) { e1 = b; }
			else if((b == f) && (e != a)) { e1 = b; }

			//E2
			if(b == f) { e2 = b; }

			//E3
			if((d == b) && (e != g)) { e3 = d; }
			else if((d == h) && (e != a)) { e3 = d; }

			//E5
			if((b == f) && (e == i)) { e5 = f; }
			else if((h == f) && (e != c)) { e5 = f; }

			//E6
			if(d == h) { e6 = d; }

			//E7
			if((d == h) && (e != i)) { e7 = h; }
			else if((h == f) && (e != g)) { e7 = h; }

			//E8
			if(h == f) { e8 = f; }

			//Blend E0 with E1, E3
			if((section == 0) && (e1 == e3)) { current_color = mix(e0, e1, 0.5); }

			//Blend E1 with E0, E4
			else if((section == 1) && (e0 == e4)) { current_color = mix(e1, e0, 0.5); }

			//Blend E1 with E2, E4
			else if((section == 1) && (e2 == e4)) { current_color = mix(e1, e2, 0.5); }

			//Blend E2 with E1, E5
			else if((section == 2) && (e1 == e5)) { current_color = mix(e2, e1, 0.5); }

			//Blend E3 with E0, E4
			else if((section == 3) && (e0 == e4)) { current_color = mix(e3, e0, 0.5); }

			//Blend E3 with E4, E6
			else if((section == 3) && (e4 == e6)) { current_color = mix(e3, e4, 0.5); }

			//Blend E5 with E2, E4
			else if((section == 5) && (e2 == e4)) { current_color = mix(e5, e2, 0.5); }

			//Blend E5 with E4, E8
			else if((section == 5) && (e4 == e8)) { current_color = mix(e5, e4, 0.5); }

			//Blend E6 with E3, E7
			else if((section == 6) && (e3 == e7)) { current_color = mix(e6, e3, 0.5); }

			//Blend E7 with E4, E6
			else if((section == 7) && (e4 == e6)) { current_color = mix(e7, e4, 0.5); }

			//Blend E7 with E4, E8
			else if((section == 7) && (e4 == e8)) { current_color = mix(e7, e4, 0.5); }

			//Blend E8 with E5, E7
			else if((section == 8) && (e5 == e7)) { current_color = mix(e8, e5, 0.5); }
		}
			
	}

	color = current_color;
}
