// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : vertex.vs
// Date : August 13, 2016
// Description : GBE+ Vertex Shader
//
// Default vertex shader, scales to fit aspect ratio

#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 tex_coord;

out vec2 texture_coordinates;

uniform float x_scale;
uniform float y_scale;

void main()
{
	vec4 pos = vec4(position, 1.0f);
	pos.x = pos.x * x_scale;
	pos.y = pos.y * y_scale;

	gl_Position = pos;
	texture_coordinates = tex_coord;
}