// EXT_DATA_USAGE_1
// GB Enhanced+ Copyright Daniel Baxter 2025
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : 8_bit.fs
// Date : October 7, 2025
// Description : GBE+ Fragment Shader - 8-Bit Mode
//
// 8-bit color simulation. Reduces palette to ~30 colors max + pixelates to recreate and 8-bit look and feel
// Intended for systems with a wide color palette (GBA, NDS)

#version 330 core
in vec2 texture_coordinates;

out vec4 color;

uniform sampler2D screen_texture;
uniform int screen_x_size;
uniform int screen_y_size;

uniform float ext_data_1;
uniform float ext_data_2;

const vec4 palette[37] = vec4[37]
(
	//Black-Gray-White
	vec4(0.00, 0.00, 0.00, 1.0),
	vec4(0.25, 0.25, 0.25, 1.0),
	vec4(0.50, 0.50, 0.50, 1.0),
	vec4(0.75, 0.75, 0.75, 1.0),
	vec4(1.00, 1.00, 1.00, 1.0),

	//Blue
	vec4(0.00, 0.10, 0.58, 1.0),
	vec4(0.04, 0.30, 0.85, 1.0),
	vec4(0.34, 0.64, 1.00, 1.0),
	vec4(0.74, 0.87, 1.00, 1.0),

	//Purple 1
	vec4(0.25, 0.00, 0.61, 1.0),
	vec4(0.44, 0.08, 0.95, 1.0),
	vec4(0.70, 0.42, 1.00, 1.0),
	vec4(0.87, 0.79, 1.00, 1.0),

	//Purple 2
	vec4(0.38, 0.00, 0.41, 1.0),
	vec4(0.60, 0.04, 0.72, 1.0),
	vec4(0.87, 0.37, 1.00, 1.0),
	vec4(0.94, 0.76, 1.00, 1.0),

	//Red 1
	vec4(0.43, 0.00, 0.14, 1.0),
	vec4(0.69, 0.07, 0.38, 1.0),
	vec4(0.97, 0.38, 0.77, 1.0),
	vec4(0.98, 0.76, 0.93, 1.0),

	//Red 2
	vec4(0.39, 0.01, 0.00, 1.0),
	vec4(0.66, 0.15, 0.01, 1.0),
	vec4(0.97, 0.45, 0.42, 1.0),
	vec4(0.99, 0.79, 0.80, 1.0),

	//Yellow
	vec4(0.28, 0.11, 0.00, 1.0),
	vec4(0.53, 0.27, 0.00, 1.0),
	vec4(0.87, 0.56, 0.12, 1.0),
	vec4(0.96, 0.83, 0.68, 1.0),

	//Green 1
	vec4(0.13, 0.21, 0.00, 1.0),
	vec4(0.34, 0.40, 0.00, 1.0),
	vec4(0.70, 0.68, 0.00, 1.0),
	vec4(0.90, 0.87, 0.61, 1.0),

	//Green 2
	vec4(0.00, 0.28, 0.00, 1.0),
	vec4(0.13, 0.49, 0.00, 1.0),
	vec4(0.50, 0.78, 0.00, 1.0),
	vec4(0.82, 0.91, 0.60, 1.0)
);

//Control variables - Adjust these to change how the shader's effects work

//Pixelate effect size in pixels. Should be power of 2.
int block_scale = 2;

//Get the weighted "distance" between two colors
float get_color_distance(in vec4 color_1, in vec4 color_2)
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

vec4 pixelate_color()
{
	vec2 final_pos;
	vec2 current_pos = texture_coordinates;

	int win_x_ratio = int(ext_data_1 / screen_x_size);
	int win_y_ratio = int(ext_data_2 / screen_y_size);

	int block_x_size = win_x_ratio * block_scale;
	int block_y_size = win_y_ratio * block_scale; 

	int pixel_x = int(current_pos.x * screen_x_size * win_x_ratio);
	int pixel_y = int(current_pos.y * screen_y_size * win_y_ratio);

	pixel_x -= (pixel_x % block_x_size);
	pixel_y -= (pixel_y % block_y_size);

	final_pos.x = (1.0 / ext_data_1) * float(pixel_x);
	final_pos.y = (1.0 / ext_data_2) * float(pixel_y);

	return texture(screen_texture, final_pos);
}

void main()
{
	vec4 current_color = pixelate_color();

	float min_dist = 1000.0;
	int pal_index = 0;

	for(int x = 0; x < 37; x++)
	{
		float dist = get_color_distance(current_color, palette[x]);

		if(dist < min_dist)
		{
			min_dist = dist;
			pal_index = x;
		}
	}

	color = palette[pal_index];
}
