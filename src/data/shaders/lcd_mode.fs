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

//Returns the maximum RGB component
void rgb_max(in vec4 rgb_color, out float maximum)
{
	if(rgb_color.r > rgb_color.g) { maximum = rgb_color.r; }
	else { maximum = rgb_color.g; }

	if(maximum < rgb_color.b) { maximum = rgb_color.b; }
}

//Returns minumum RGB component
void rgb_min(in vec4 rgb_color, out float minimum)
{
	if(rgb_color.r < rgb_color.g) { minimum = rgb_color.r; }
	else { minimum = rgb_color.g; }

	if(minimum > rgb_color.b) { minimum = rgb_color.b; }
}

void hue_to_rgb(in float hue_f1, in float hue_f2, in float hue, out float rgb_comp)
{
	if(hue < 0.0) { hue += 1.0; }
	else if(hue > 1.0) { hue -= 1.0; }

	if((6 * hue) < 1.0) { rgb_comp = (hue_f1 + (hue_f2 - hue_f1) * 6 * hue); }
	else if((2 * hue) < 1.0) { rgb_comp = hue_f2; }
	else if((3 * hue) < 2.0) { rgb_comp = (hue_f1 + (hue_f2 - hue_f1) * ((2.0/3.0) - hue) * 6); }
	else { rgb_comp = hue_f1; }
} 

void rgb_to_hsl(in vec4 input_color, out vec4 hsl_values)
{
	//Maximum and minimum RGB components from input color
	float max_comp;
	float min_comp;
	float color_delta;

	//Calculate color delta
	rgb_max(input_color, max_comp);
	rgb_min(input_color, min_comp);
	color_delta = max_comp - min_comp;

	//Calculate lightness
	hsl_values[2] = (max_comp + min_comp) / 2.0;

	if(color_delta == 0)
	{
		hsl_values[0] = 0.0;
		hsl_values[1] = 0.0;
	}

	else
	{
		//Calculate saturation
		if(hsl_values[2] < 0.5) { hsl_values[1] = color_delta / (max_comp + min_comp); }
		else { hsl_values[1] = color_delta / (2.0 - max_comp - min_comp); }

		//Calculate hue
		float r_delta = (((max_comp - input_color.r ) / 6.0) + (color_delta / 2.0)) / color_delta;
		float g_delta = (((max_comp - input_color.g ) / 6.0) + (color_delta / 2.0)) / color_delta;
		float b_delta = (((max_comp - input_color.b ) / 6.0) + (color_delta / 2.0)) / color_delta;

		if(input_color.r == max_comp) { hsl_values[0] = b_delta - g_delta; }
		else if(input_color.g == max_comp) { hsl_values[0] = (1.0 / 3.0) + r_delta - b_delta; }
		else if(input_color.b == max_comp) { hsl_values[0] = (2.0 / 3.0 ) + g_delta - r_delta; }

		if(hsl_values[0] < 0.0 ) { hsl_values[0] += 1.0; }
   		else if(hsl_values[0] > 1.0 ) { hsl_values[0] -= 1.0; }
	}
}

void hsl_to_rgb(in vec4 hsl_values, out vec4 output_color)
{
	float r = 0.0;
	float g = 0.0;
	float b = 0.0;

	if(hsl_values[1] == 0)
	{
		r = hsl_values[2];
		g = hsl_values[2];
		b = hsl_values[2];
	}

	else
	{
		float hue_f1 = 0.0;
		float hue_f2 = 0.0;

		if(hsl_values[2] < 0.5) { hue_f2 = hsl_values[2] * (1.0 + hsl_values[1]); }
		else { hue_f2 = (hsl_values[2] + hsl_values[1]) - (hsl_values[1] * hsl_values[2]); }

		hue_f1 = (2.0 * hsl_values[2]) - hue_f2;

		hue_to_rgb(hue_f1, hue_f2, hsl_values[0] + (1.0/3.0), r);
		hue_to_rgb(hue_f1, hue_f2, hsl_values[0], g);
		hue_to_rgb(hue_f1, hue_f2, hsl_values[0] - (1.0/3.0), b);
	}

	output_color.r = r;
	output_color.g = g;
	output_color.b = b;
	output_color.a = 1.0;
}

void main()
{
	vec2 current_pos = texture_coordinates;
	vec4 current_color = texture(screen_texture, texture_coordinates);

	vec4 hsl = vec4(1.0, 1.0, 1.0, 1.0);
	rgb_to_hsl(current_color, hsl);

	//Screen ratios
	float x_size = (1.0 / screen_x_size);
	float y_size = (1.0 / screen_y_size);

	float scr_x = (ext_data_1 / screen_x_size);
	float scr_y = (ext_data_2 / screen_y_size);

	float screen_x_mod = mod(current_pos.x, x_size);
	float screen_y_mod = mod(current_pos.y, y_size);

	if(screen_x_mod < (x_size/scr_x))
	{
		hsl[2] -= 0.15;
		if(hsl[2] < 0.0) { hsl[2] == 0.0; }
	}

	else if(screen_y_mod < (y_size/scr_y))
	{
		hsl[2] -= 0.15;
		if(hsl[2] < 0.0) { hsl[2] == 0.0; }
	}

	hsl_to_rgb(hsl, current_color);

	color = current_color;
}
