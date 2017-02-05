// EXT_DATA_USAGE_1
// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : spotlight.fs
// Date : August 19, 2016
// Description : GBE+ Fragment Shader - Spotlight
//
// Simulates a spotlight on the center of the screen

#version 330 core
in vec2 texture_coordinates;

out vec4 color;

uniform sampler2D screen_texture;
uniform int screen_x_size;
uniform int screen_y_size;

uniform float ext_data_1;
uniform float ext_data_2;

//Control variables - Adjust these to change how the shader's effects work

//Determines how close to the center the dark edges are. Increase to push the edges away, decrease to close them in
float max_factor = 1.0;

//Amount to lower a color's brightness
float dark_factor = 0.5;

//Focal point coordinates of the spotlight, defaults to center of screen
vec2 focal_point = vec2((ext_data_1 * 0.5), (ext_data_2 * 0.5)); 

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
}

//Blend two colors
void rgb_blend(in vec4 color_1, in vec4 color_2, out vec4 final_color)
{
	final_color.r = (color_1.r + color_2.r) / 2.0;
	final_color.g = (color_1.g + color_2.g) / 2.0;
	final_color.b = (color_1.b + color_2.b) / 2.0;
}

//Find the distance between two points
void dist(in vec2 pos_1, in vec2 pos_2, out float distance)
{
	float final_x = (pos_2.x - pos_1.x);
	final_x *= final_x;

	float final_y = (pos_2.y - pos_1.y);
	final_y *= final_y;

	distance = sqrt(final_x + final_y);
}

void main()
{
	vec2 current_pos = texture_coordinates;
	vec4 current_color = texture(screen_texture, texture_coordinates);

	//Find current screen scale
	float scale_x = ext_data_1 / screen_x_size;
	float scale_y = ext_data_2 / screen_y_size;

	vec2 screen_pos;
	screen_pos.x = gl_FragCoord.x;
	screen_pos.y = gl_FragCoord.y;

	vec2 max_pos = vec2(0, 0);	

	//Find the maximum distance
	float max_dist = 0.0;
	dist(max_pos, focal_point, max_dist);

	max_dist *= max_factor;

	//Find current distance
	float current_dist = 0.0;
	dist(screen_pos, focal_point, current_dist);

	vec4 hsl = vec4(1.0, 1.0, 1.0, 1.0);
	rgb_to_hsl(current_color, hsl);

	//Darken fragments based on distance
	hsl[2] -= ((current_dist/max_dist) * dark_factor);
	if(hsl[2] < 0.0) { hsl[2] = 0.0; }

	hsl_to_rgb(hsl, current_color);

	color = current_color;
}
