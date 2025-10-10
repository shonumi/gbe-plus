// EXT_DATA_USAGE_1
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

//Control variables - Adjust these to change how the shader's effects work

//Pixelate effect size in pixels. Should be power of 2.
int block_scale = 2;

//Mixes all pixels within a block
bool block_blur = true;

void main()
{
	vec2 final_pos;
	vec2 current_pos = texture_coordinates;
	vec4 current_color = texture(screen_texture, texture_coordinates);

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

	//Simply repeat the 1st color of a block for the entirety of a block
	//This works, but is rather choppy in motion
	if(block_blur == false)
	{
		color = texture(screen_texture, final_pos);
	}

	//Take average of all pixels within a block
	//Looks better/more consistent in motion
	else
	{
		vec4 mix_color = vec4(0.0, 0.0, 0.0, 1.0);
		vec4 temp_color = vec4(0.0, 0.0, 0.0, 1.0);
		vec2 temp_pos = vec2(0.0, 0.0);

		for(int y = 0; y < block_y_size; y++)
		{
			temp_pos.y = (1.0 / ext_data_2) * float(pixel_y + y);

			for(int x = 0; x < block_x_size; x++)
			{
				temp_pos.x = (1.0 / ext_data_1) * float(pixel_x + x);
				temp_color = texture(screen_texture, temp_pos);
				mix_color += temp_color;
			}
		}

		mix_color /= float(block_x_size * block_y_size);
		color = mix_color;
	}
}
