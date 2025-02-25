// GB Enhanced+ Copyright Daniel Baxter 2018
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : ogl_manager.cpp
// Date : March 10, 2018
// Description : OpenGL management for Qt
//
// Abstracts OpenGL management away from Qt
// Uses OpenGL directly and sometimes with GLEW

#include <iostream>

#ifndef GL3_PROTOTYPES
#define GL3_PROTOTYPES 1
#endif

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

#ifdef GBE_GLEW
#include "GL/glew.h"
#endif

#include "ogl_manager.h"

#include "common/config.h"
#include "common/gx_util.h"

/****** OpenGL Manager Constructor ******/
ogl_manager::ogl_manager()
{
	lcd_texture = 0;
	program_id = 0;
	vertex_buffer_object = 0;
	vertex_array_object = 0;
	element_buffer_object = 0;
	ogl_x_scale = 0.0;
	ogl_y_scale = 0.0;
	ext_data_1 = 0.0;
	ext_data_2 = 0.0;
	external_data_usage = 0;
}

/****** OpenGL Manager Destructor ******/
ogl_manager::~ogl_manager() { }

/****** OpenGL Manager - Init ******/
void ogl_manager::init()
{
	#ifdef GBE_GLEW
 	GLenum glew_err = glewInit();
 	if(glew_err != GLEW_OK)
	{
		std::cout<<"LCD::Error - Could not initialize GLEW: " << glewGetErrorString(glew_err) << "\n";
		return;
  	}
	#endif

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

	external_data_usage = 0;

	//Load the shader
	program_id = gx_load_shader(config::vertex_shader, config::fragment_shader, external_data_usage);

	if(program_id == -1) { std::cout<<"LCD::Error - Could not generate shaders\n"; }
}

/****** OpenGL Manager - Paint ******/
void ogl_manager::paint()
{
	//Bind screen texture, then generate texture from lcd pixels
	glBindTexture(GL_TEXTURE_2D, lcd_texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, config::sys_width, config::sys_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixel_data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);

    	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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
}

/****** OpenGL Manager - Resize ******/
void ogl_manager::resize(u32 w, u32 h)
{
	glViewport(0, 0, w, h);
}
