// EXT_DATA_USAGE_2
// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : chrono.fs
// Date : August 16, 2016
// Description : GBE+ Fragment Shader - Chrono
//
// Shades the screen with different colors depending on the current time
// Intended for black and white GB games to set the mood according to what time you're playing

#version 330 core
in vec2 texture_coordinates;

out vec4 color;

uniform sampler2D screen_texture;
uniform int screen_x_size;
uniform int screen_y_size;

uniform float ext_data_1;
uniform float ext_data_2;

//Blend two colors
void rgb_blend(in vec4 color_1, in vec4 color_2, out vec4 final_color)
{
	final_color.r = (color_1.r + color_2.r + color_2.r) / 3.0;
	final_color.g = (color_1.g + color_2.g + color_2.g) / 3.0;
	final_color.b = (color_1.b + color_2.b + color_2.b) / 3.0;
}

void main()
{
	vec4 current_color = texture(screen_texture, texture_coordinates);
	vec4 blend_color = vec4(1.0, 1.0, 1.0, 1.0);
	vec4 next_color = vec4(1.0, 1.0, 1.0, 1.0);

	float r_dist;
	float g_dist;
	float b_dist;

	//ext_data_1 is the current hour, 0-23
	float chrono_hour = ext_data_1;
	float chrono_minute = ext_data_2;

	//00:00 - 03:59 - Dark Blue to Blue
	if(chrono_hour >= 0)
	{
		if(chrono_hour < 4)
		{
			chrono_minute += (chrono_hour * 60);

			blend_color = vec4(0.0, 0.14, 0.89, 1.0);
			next_color = vec4(0.09, 0.24, 0.75, 1.0);

			r_dist = (next_color.r - blend_color.r)/240.0;
			r_dist *= chrono_minute;
			blend_color.r += r_dist;

			g_dist = (next_color.g - blend_color.g)/240.0;
			g_dist *= chrono_minute;
			blend_color.g += g_dist;

			b_dist = (next_color.b - blend_color.b)/240.0;
			b_dist *= chrono_minute;
			blend_color.b += b_dist;

			rgb_blend(current_color, blend_color, current_color);
		}
	}

	//04:00 - 06:59 Blue to Red
	if(chrono_hour >= 4)
	{
		if(chrono_hour < 7)
		{
			chrono_minute += ((chrono_hour - 4) * 60);

			blend_color = vec4(0.09, 0.24, 0.75, 1.0);
			next_color = vec4(0.75, 0.16, 0.09, 1.0);

			r_dist = (next_color.r - blend_color.r)/180.0;
			r_dist *= chrono_minute;
			blend_color.r += r_dist;

			g_dist = (next_color.g - blend_color.g)/180.0;
			g_dist *= chrono_minute;
			blend_color.g += g_dist;

			b_dist = (next_color.b - blend_color.b)/180.0;
			b_dist *= chrono_minute;
			blend_color.b += b_dist;

			rgb_blend(current_color, blend_color, current_color);
		}
	}

	//07:00 - 9:59 Red to Yellow
	if(chrono_hour >= 7)
	{
		if(chrono_hour < 10)
		{
			chrono_minute += ((chrono_hour - 7) * 60);

			blend_color = vec4(0.75, 0.16, 0.09, 1.0);
			next_color = vec4(0.88, 0.88, 0.05, 1.0);

			r_dist = (next_color.r - blend_color.r)/180.0;
			r_dist *= chrono_minute;
			blend_color.r += r_dist;

			g_dist = (next_color.g - blend_color.g)/180.0;
			g_dist *= chrono_minute;
			blend_color.g += g_dist;

			b_dist = (next_color.b - blend_color.b)/180.0;
			b_dist *= chrono_minute;
			blend_color.b += b_dist;

			rgb_blend(current_color, blend_color, current_color);
		}
	}

	//10:00 - 12:59 Yellow to White-Yellow
	if(chrono_hour >= 10)
	{
		if(chrono_hour < 13)
		{
			chrono_minute += ((chrono_hour - 10) * 60);

			blend_color = vec4(0.88, 0.88, 0.05, 1.0);
			next_color = vec4(0.88, 0.88, 0.74, 1.0);

			r_dist = (next_color.r - blend_color.r)/180.0;
			r_dist *= chrono_minute;
			blend_color.r += r_dist;

			g_dist = (next_color.g - blend_color.g)/180.0;
			g_dist *= chrono_minute;
			blend_color.g += g_dist;

			b_dist = (next_color.b - blend_color.b)/180.0;
			b_dist *= chrono_minute;
			blend_color.b += b_dist;

			rgb_blend(current_color, blend_color, current_color);
		}
	}

	//13:00 - 16:59 White-Yellow to Red
	if(chrono_hour >= 13)
	{
		if(chrono_hour < 17)
		{
			chrono_minute += ((chrono_hour - 13) * 60);

			blend_color = vec4(0.88, 0.88, 0.74, 1.0);
			next_color = vec4(0.75, 0.16, 0.09, 1.0);

			r_dist = (next_color.r - blend_color.r)/240.0;
			r_dist *= chrono_minute;
			blend_color.r += r_dist;

			g_dist = (next_color.g - blend_color.g)/240.0;
			g_dist *= chrono_minute;
			blend_color.g += g_dist;

			b_dist = (next_color.b - blend_color.b)/240.0;
			b_dist *= chrono_minute;
			blend_color.b += b_dist;

			rgb_blend(current_color, blend_color, current_color);
		}
	}

	//17:00 - 19:59 Red to Light Blue
	if(chrono_hour >= 17)
	{
		if(chrono_hour < 20)
		{
			chrono_minute += ((chrono_hour - 17) * 60);

			blend_color = vec4(0.75, 0.16, 0.09, 1.0);
			next_color = vec4(0.03, 0.54, 1.0, 1.0);

			r_dist = (next_color.r - blend_color.r)/180.0;
			r_dist *= chrono_minute;
			blend_color.r += r_dist;

			g_dist = (next_color.g - blend_color.g)/180.0;
			g_dist *= chrono_minute;
			blend_color.g += g_dist;

			b_dist = (next_color.b - blend_color.b)/180.0;
			b_dist *= chrono_minute;
			blend_color.b += b_dist;

			rgb_blend(current_color, blend_color, current_color);
		}
	}

	//20:00 - 23:00 Light Blue to Dark Blue
	if(chrono_hour >= 20)
	{
		chrono_minute += ((chrono_hour - 20) * 60);

		blend_color = vec4(0.03, 0.54, 1.0, 1.0);
		next_color = vec4(0.0, 0.14, 0.89, 1.0);

		r_dist = (next_color.r - blend_color.r)/180.0;
		r_dist *= chrono_minute;
		blend_color.r += r_dist;

		g_dist = (next_color.g - blend_color.g)/180.0;
		g_dist *= chrono_minute;
		blend_color.g += g_dist;

		b_dist = (next_color.b - blend_color.b)/180.0;
		b_dist *= chrono_minute;
		blend_color.b += b_dist;

		rgb_blend(current_color, blend_color, current_color);
	}

	color = current_color;
}
