// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : opengl.cpp
// Date : May 29, 2015
// Description : Handles OpenGL functionality
//
// Sets up OpenGL for use in GBE+
// Handles blit operations to the screen

#ifndef GL3_PROTOTYPES
#define GL3_PROTOTYPES 1
#endif

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

#ifdef GBE_GLEW
#include "GL/glew.h"
#endif

#include <ctime>

#include "lcd.h"
#include "common/gx_util.h"

/****** Initialize OpenGL through SDL ******/
void DMG_LCD::opengl_init()
{
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	u32 screen_width = (config::sys_width * config::scaling_factor);
	u32 screen_height = (config::sys_height * config::scaling_factor);

	if(config::flags & SDL_WINDOW_FULLSCREEN)
	{
		SDL_DisplayMode current_mode;
		SDL_GetCurrentDisplayMode(0, &current_mode);

		screen_width = current_mode.w;
		screen_height = current_mode.h;
	}
		
	window = SDL_CreateWindow("GBE+", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, config::flags | SDL_WINDOW_OPENGL);
	SDL_GetWindowSize(window, &config::win_width, &config::win_height);

	//Calculate new temporary scaling factor
	float max_width = (float)config::win_width / config::sys_width;
	float max_height = (float)config::win_height / config::sys_height;

	//Find the maximum dimensions that maintain the original aspect ratio
	if(config::flags & SDL_WINDOW_FULLSCREEN)
	{
		if(max_width <= max_height)
		{
			float max_x_size = (max_width * config::sys_width);
			float max_y_size = (max_width * config::sys_height);

			ogl_x_scale =  max_x_size / config::win_width;
			ogl_y_scale =  max_y_size / config::win_height;
		}

		else
		{
			float max_x_size = (max_height * config::sys_width);
			float max_y_size = (max_height * config::sys_height);

			ogl_x_scale =  max_x_size / config::win_width;
			ogl_y_scale =  max_y_size / config::win_height;
		}
	}

	else { ogl_x_scale = ogl_y_scale = 1.0; }

	ext_data_1 = ext_data_2 = 1.0;

	gl_context = SDL_GL_CreateContext(window);

	glDeleteVertexArrays(1, &vertex_array_object);
	glDeleteBuffers(1, &vertex_buffer_object);
	glDeleteBuffers(1, &element_buffer_object);

	//Define vertices and texture coordinates for the screen texture
    	GLfloat vertices[] =
	{
		//Vertices		//Texture Coordinates
		1.0f, 1.0f, 0.0f,	1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,	1.0f, 1.0f,
		-1.0f, -1.0f, 0.0f,	0.0f, 1.0f,
		-1.0f,  1.0f, 0.0f, 	0.0f, 0.0f
	};

	//Define indices for the screen texture's triangles
	GLuint indices[] =
	{
        	0, 1, 3,
		1, 2, 3
    	};

	//Generate a vertex array object for the screen texture + generate vertex and element buffer
	glGenVertexArrays(1, &vertex_array_object);
	glGenBuffers(1, &vertex_buffer_object);
	glGenBuffers(1, &element_buffer_object);

	//Bind the vertex array object
	glBindVertexArray(vertex_array_object);

	//Bind vertices to vertex buffer object
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//Bind element buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer_object);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (5 * sizeof(GLfloat)), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (5 * sizeof(GLfloat)), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	//Unbind vertex array object
	glBindVertexArray(0);

	//Generate the screen texture
	glGenTextures(1, &lcd_texture);

	final_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, config::sys_width, config::sys_height, 32, 0, 0, 0, 0);
	external_data_usage = 0;

	//Load the shader
	program_id = gx_load_shader(config::vertex_shader, config::fragment_shader, external_data_usage);

	if(program_id == -1) { std::cout<<"LCD::Error - Could not generate shaders\n"; }
}

/****** Blit using OpenGL ******/
void DMG_LCD::opengl_blit()
{
	//Determine what the shader's external data usage is
	switch(external_data_usage)
	{
		//Shader requires no external data
		case 0: break;

		//Shader requires current window dimensions
		case 1:
			ext_data_1 = config::win_width;
			ext_data_2 = config::win_height;
			break;

		//Shader requires current time
		case 2:
			{
				time_t system_time = time(0);
				tm* current_time = localtime(&system_time);

				ext_data_1 = current_time->tm_hour;
				ext_data_2 = current_time->tm_min;
			}

			break;

		default: break;
	}

	//Bind screen texture, then generate texture from lcd pixels
	glBindTexture(GL_TEXTURE_2D, lcd_texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, config::sys_width, config::sys_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, final_screen->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);

    	glClearColor(0,0,0,0);
    	glClear(GL_COLOR_BUFFER_BIT);

	//Use shader
	glUseProgram(program_id);

	//Set vertex scaling
	glUniform1f(glGetUniformLocation(program_id, "x_scale"), ogl_x_scale);
	glUniform1f(glGetUniformLocation(program_id, "y_scale"), ogl_y_scale);

	glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, lcd_texture);
        glUniform1i(glGetUniformLocation(program_id, "screen_texture"), 0);
        glUniform1i(glGetUniformLocation(program_id, "screen_x_size"), config::sys_width);
        glUniform1i(glGetUniformLocation(program_id, "screen_y_size"), config::sys_height);
        glUniform1f(glGetUniformLocation(program_id, "ext_data_1"), ext_data_1);
        glUniform1f(glGetUniformLocation(program_id, "ext_data_2"), ext_data_2);
	
        
        //Draw vertex array object
        glBindVertexArray(vertex_array_object);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);	

	glUseProgram(0);

	SDL_GL_SwapWindow(window);
}
