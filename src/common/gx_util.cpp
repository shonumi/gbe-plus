// GB Enhanced Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gx_util.cpp
// Date : August 08, 2016
// Description : OpenGL math utilities
//
// Provides 3D math utilities such as matrix handling and transformations, vector stuff
// Handles OpenGL specific stuff like loading shaders

#include <iostream>
#include <cmath>
#include <ctime>

#ifndef GL3_PROTOTYPES
#define GL3_PROTOTYPES 1
#endif

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

#ifdef GBE_GLEW
#include "GL/glew.h"
#endif

#include "config.h"
#include "gx_util.h"

/****** OpenGL Matrix Constructor ******/
gx_matrix::gx_matrix()
{
	//When no arguments are given, generate a 4x4 identity matrix
	for(u32 x = 0; x < 16; x++) { data[x] = 0; }

	data[0] = 1.0;
	data[5] = 1.0;
	data[10] = 1.0;
	data[15] = 1.0;

	rows = 4;
	columns = 4;
}

/****** OpenGL Matrix Constructor ******/
gx_matrix::gx_matrix(u32 input_columns, u32 input_rows)
{
	if(input_columns > 4) { input_columns = 4; }
	if(input_rows > 4) { input_rows = 4; }

	for(u32 x = 0; x < 16; x++) { data[x] = 0; }

	rows = input_rows;
	columns = input_columns;
}

/****** OpenGL Matrix Destructor ******/
gx_matrix::~gx_matrix()
{
	for(u32 x = 0; x < 16; x++) { data[x] = 0; }
	rows = 0;
	columns = 0;
}

/****** OpenGL Matrix multiplication operator - Matrix-Matrix ******/
gx_matrix gx_matrix::operator*(const gx_matrix &input_matrix)
{
	//Determine if matrix can be multiplied
	if(columns == input_matrix.rows)
	{
		//Determine size of output matrix
		u32 output_rows = rows;
		u32 output_columns = input_matrix.columns;

		gx_matrix output_matrix(output_columns, output_rows);

		//Find the dot product for all values in the new matrix
		for(u32 y = 0; y < rows; y++)
		{
			for(u32 dot_product_count = 0; dot_product_count < output_columns; dot_product_count++)
			{
				float dot_product = 0.0;

				for(u32 x = 0; x < columns; x++)
				{
					dot_product += (data[(y << 2) + x] * input_matrix[(x << 2) + dot_product_count]);
				}

				output_matrix.data[(y << 2) + dot_product_count] = dot_product;
			}
		}
	
		return output_matrix;
	}

	//Otherwise return an identity matrix
	else
	{
		gx_matrix output_matrix;
		return output_matrix;
	}
}

/****** OpenGL Matrix bracket operator - Getter ******/
float gx_matrix::operator[](u32 index) const
{
	return data[index & 0xF];
}

/****** OpenGL Matrix bracket operator - Setter ******/
float &gx_matrix::operator[](u32 index)
{
	return data[index & 0xF];
}

/****** Makes an identity matrix of a given size ******/
void gx_matrix::make_identity(u32 size)
{
	if(!size) { return; }

	resize(size, size);

	for(u32 x = 0; x < 16; x++) { data[x] = 0; }

	data[0] = 1.0;
	data[5] = 1.0;
	data[10] = 1.0;
	data[15] = 1.0;
}

/****** Clears a matrix and resizes to given dimensions ******/
void gx_matrix::resize(u32 input_columns, u32 input_rows)
{
	if(!input_columns || !input_rows) { return; }

	columns = input_columns;
	rows = input_rows;
}

#ifdef GBE_OGL

namespace gl_data
{
	SDL_GLContext gl_context;
	GLuint lcd_texture;
	GLuint program_id;
	GLuint vertex_buffer_object, vertex_array_object, element_buffer_object;
	GLfloat x_scale, y_scale;
	GLfloat ext_data_1, ext_data_2;
	u32 external_data_usage;
};

/****** Loads and compiles GLSL vertex and fragment shaders ******/
GLuint gx_load_shader(std::string vertex_shader_file, std::string fragment_shader_file, u32 &ext_data_usage)
{
	GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

	std::string vs_code = "";
	std::string fs_code = "";
	std::string temp = "";

	std::ifstream vs_data;
	std::ifstream fs_data;

	vs_data.open(vertex_shader_file.c_str());

	std::size_t match;

	//Try to open vertex shader file
	if(!vs_data.is_open()) 
	{
		std::cout<<"OGL::Could not open vertex shader file  " << vertex_shader_file << "\n"; 
		return -1;
	}

	//Grab vertex shader source code
	while(getline(vs_data, temp)) { vs_code += "\n" + temp; }

	temp = "";
	vs_data.close();

	fs_data.open(fragment_shader_file.c_str());

	//Try to open vertex shader file
	if(!fs_data.is_open()) 
	{
		std::cout<<"OGL::Could not open fragment shader file  " << fragment_shader_file << "\n"; 
		return -1;
	}

	//Grab fragment shader source code
	while(getline(fs_data, temp))
	{
		fs_code += "\n" + temp;

		//Search for external data usage comment, if any
		match = temp.find("// EXT_DATA_USAGE_0");
		if(match != std::string::npos) { ext_data_usage = 0; }

		match = temp.find("EXT_DATA_USAGE_1");
		if(match != std::string::npos) { ext_data_usage = 1; }

		match = temp.find("EXT_DATA_USAGE_2");
		if(match != std::string::npos) { ext_data_usage = 2; }
	}

	temp = "";
	fs_data.close();

	GLint result = GL_FALSE;
	int log_length;

	//Compile vertex shader
	std::cout<<"OGL::Compiling vertex shader : " << vertex_shader_file << "\n"; 

	char const *vs_code_pointer = vs_code.c_str();
	glShaderSource(vertex_shader_id, 1, &vs_code_pointer, NULL);
	glCompileShader(vertex_shader_id);

	//Check vertex shader
	glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result);
	glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &log_length);

	if(log_length)
	{
		std::vector<char> vs_error(log_length);
		glGetShaderInfoLog(vertex_shader_id, log_length, NULL, &vs_error[0]);

		//Print any error messages from compiling vertex shader
		std::cout<<"OGL::Vertex Shader Error Message Log: " << &vs_error[0] << "\n";
	}
 
	//Compile fragment shader
	std::cout<<"OGL::Compiling fragment shader : " << fragment_shader_file << "\n"; 
    	char const * fs_code_pointer = fs_code.c_str();
    	glShaderSource(fragment_shader_id, 1, &fs_code_pointer, NULL);
    	glCompileShader(fragment_shader_id);
 
	//Check fragment Shader
	glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result);
	glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &log_length);

	if(log_length)
	{
		std::vector<char> fs_error(log_length);
		glGetShaderInfoLog(fragment_shader_id, log_length, NULL, &fs_error[0]);

		//Print any error messages from compiling fragment shader
		std::cout<<"OGL::Fragment Shader Error Message Log: " << &fs_error[0] << "\n";
	}
 
	//Link the program
	std::cout<<"OGL::Linking shaders...\n";

	GLuint program_id = glCreateProgram();
	glAttachShader(program_id, vertex_shader_id);
	glAttachShader(program_id, fragment_shader_id);
	glLinkProgram(program_id);
 
	//Check the program
	glGetProgramiv(program_id, GL_LINK_STATUS, &result);
	glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

	if(log_length)
	{
		std::vector<char> program_error(log_length);
		glGetProgramInfoLog(program_id, log_length, NULL, &program_error[0]);

		//Print any error messages from the linking process
		std::cout<<"OGL::Linking Error Message Log: " << &program_error[0] << "\n";
	}
	
	glDeleteShader(vertex_shader_id);
	glDeleteShader(fragment_shader_id);
 
	return program_id;
}

#endif

//OpenGL init and render for cores
bool gx_init_opengl()
{
	#ifdef GBE_OGL

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

			gl_data::x_scale =  max_x_size / config::win_width;
			gl_data::y_scale =  max_y_size / config::win_height;
		}

		else
		{
			float max_x_size = (max_height * config::sys_width);
			float max_y_size = (max_height * config::sys_height);

			gl_data::x_scale =  max_x_size / config::win_width;
			gl_data::y_scale =  max_y_size / config::win_height;
		}
	}

	else { gl_data::x_scale = gl_data::y_scale = 1.0; }

	gl_data::ext_data_1 = gl_data::ext_data_2 = 1.0;

	#ifdef GBE_GLEW
 	GLenum glew_err = glewInit();
 	if(glew_err != GLEW_OK)
	{
		std::cout<<"LCD::Error - Could not initialize GLEW: " << glewGetErrorString(glew_err) << "\n";
		return false;
  	}
	#endif

	glDeleteVertexArrays(1, &gl_data::vertex_array_object);
	glDeleteBuffers(1, &gl_data::vertex_buffer_object);
	glDeleteBuffers(1, &gl_data::element_buffer_object);

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
	glGenVertexArrays(1, &gl_data::vertex_array_object);
	glGenBuffers(1, &gl_data::vertex_buffer_object);
	glGenBuffers(1, &gl_data::element_buffer_object);

	//Bind the vertex array object
	glBindVertexArray(gl_data::vertex_array_object);

	//Bind vertices to vertex buffer object
	glBindBuffer(GL_ARRAY_BUFFER, gl_data::vertex_buffer_object);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//Bind element buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_data::element_buffer_object);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (5 * sizeof(GLfloat)), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (5 * sizeof(GLfloat)), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	//Unbind vertex array object
	glBindVertexArray(0);

	//Generate the screen texture
	glGenTextures(1, &gl_data::lcd_texture);

	gl_data::external_data_usage = 0;

	//Load the shader
	gl_data::program_id = gx_load_shader(config::vertex_shader, config::fragment_shader, gl_data::external_data_usage);

	if(gl_data::program_id == -1)
	{
		std::cout<<"LCD::Error - Could not generate shaders\n";
		return false;
	}

	return true;

	#endif

	return false;
}

//OpenGL render for cores
void gx_blit_opengl(SDL_Window *window, SDL_Surface* final_screen)
{
	#ifdef GBE_OGL

	//Determine what the shader's external data usage is
	switch(gl_data::external_data_usage)
	{
		//Shader requires no external data
		case 0: break;

		//Shader requires current window dimensions
		case 1:
			gl_data::ext_data_1 = config::win_width;
			gl_data::ext_data_2 = config::win_height;
			break;

		//Shader requires current time
		case 2:
			{
				time_t system_time = time(0);
				tm* current_time = localtime(&system_time);

				gl_data::ext_data_1 = current_time->tm_hour;
				gl_data::ext_data_2 = current_time->tm_min;
			}

			break;

		default: break;
	}

	//Bind screen texture, then generate texture from lcd pixels
	glBindTexture(GL_TEXTURE_2D, gl_data::lcd_texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, config::sys_width, config::sys_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, final_screen->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);

    	glClearColor(0,0,0,0);
    	glClear(GL_COLOR_BUFFER_BIT);

	//Use shader
	glUseProgram(gl_data::program_id);

	//Set vertex scaling
	glUniform1f(glGetUniformLocation(gl_data::program_id, "x_scale"), gl_data::x_scale);
	glUniform1f(glGetUniformLocation(gl_data::program_id, "y_scale"), gl_data::y_scale);

	glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gl_data::lcd_texture);
        glUniform1i(glGetUniformLocation(gl_data::program_id, "screen_texture"), 0);
        glUniform1i(glGetUniformLocation(gl_data::program_id, "screen_x_size"), config::sys_width);
        glUniform1i(glGetUniformLocation(gl_data::program_id, "screen_y_size"), config::sys_height);
        glUniform1f(glGetUniformLocation(gl_data::program_id, "ext_data_1"), gl_data::ext_data_1);
        glUniform1f(glGetUniformLocation(gl_data::program_id, "ext_data_2"), gl_data::ext_data_2);
	
        
        //Draw vertex array object
        glBindVertexArray(gl_data::vertex_array_object);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);	

	glUseProgram(0);

	SDL_GL_SwapWindow(window);

	#endif
}

/****** Returns distance between 2D vectors ******/
float dist(float x1, float y1, float x2, float y2)
{
	return sqrt(((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1)));
}

/****** Returns distance between 3D vectors ******/
float dist(float x1, float y1, float z1, float x2, float y2, float z2)
{
	return sqrt(((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1)) + ((z2 - z1) * (z2 - z1)));
}

/****** Serializes maxtrix data from binary file ******/
bool serialize_matrix(std::ifstream &file, gx_matrix &mat)
{
	//For simplicity, file must already be opened
	if(!file.is_open()) { return false; }

	file.read((char*)&mat.data, sizeof(mat.data));
	file.read((char*)&mat.rows, sizeof(mat.rows));
	file.read((char*)&mat.columns, sizeof(mat.columns));

	return true;
}

/****** Serializes maxtrix data from binary file ******/
bool serialize_matrix(std::ofstream &file, gx_matrix &mat)
{
	//For simplicity, file must already be opened
	if(!file.is_open()) { return false; }

	file.write((char*)&mat.data, sizeof(mat.data));
	file.write((char*)&mat.rows, sizeof(mat.rows));
	file.write((char*)&mat.columns, sizeof(mat.columns));

	return true;
}
