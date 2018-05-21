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
#include <fstream>
#include <cmath>

#ifndef GL3_PROTOTYPES
#define GL3_PROTOTYPES 1
#endif

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

#ifdef GBE_GLEW
#include "GL/glew.h"
#endif

#include "gx_util.h"

/****** OpenGL Vector Constructor ******/
gx_vector::gx_vector()
{
	//When no arguments are given, generate a vector with 4 elements
	data.resize(4, 0.0);

	size = 4;
}

/****** OpenGL Vector Constructor ******/
gx_vector::gx_vector(u32 input_size)
{
	data.resize(input_size, 0.0);

	size = input_size;
}

/****** OpenGL Vector Destructor ******/
gx_vector::~gx_vector()
{
	data.clear();
}

/****** OpenGL Vector addition operator ******/
gx_vector gx_vector::operator+ (const gx_vector &input_vector)
{
	gx_vector output_vector(size);

	if(size == input_vector.size)
	{
		for(u32 x = 0; x < size; x++) { output_vector.data[x] = data[x] + input_vector.data[x]; }
	}

	//If the vectors are different sizes, just return the 1st one
	else
	{
		for(u32 x = 0; x < size; x++) { output_vector.data[x] = data[x]; }
	}

	return output_vector;
}

/****** OpenGL Vector subtraction operator ******/
gx_vector gx_vector::operator- (const gx_vector &input_vector)
{
	gx_vector output_vector(size);

	if(size == input_vector.size)
	{
		for(u32 x = 0; x < size; x++) { output_vector.data[x] = data[x] - input_vector.data[x]; }
	}

	//If the vectors are different sizes, just return the 1st one
	else
	{
		for(u32 x = 0; x < size; x++) { output_vector.data[x] = data[x]; }
	}

	return output_vector;
}

/****** OpenGL Vector multiplication operator - Vector-Vector ******/
gx_vector gx_vector::operator* (const gx_vector &input_vector)
{
	gx_vector output_vector(size);

	if(size == input_vector.size)
	{
		for(u32 x = 0; x < size; x++) { output_vector.data[x] = data[x] * input_vector.data[x]; }
	}

	//If the vectors are different sizes, just return the 1st one
	else
	{
		for(u32 x = 0; x < size; x++) { output_vector.data[x] = data[x]; }
	}

	return output_vector;
}

/****** OpenGL Vector multiplication operator - Scalar ******/
gx_vector operator* (float scalar, const gx_vector &input_vector)
{
	gx_vector output_vector(input_vector.size);

	for(u32 x = 0; x < output_vector.size; x++) { output_vector.data[x] = input_vector.data[x] * scalar; }

	return output_vector;
}

/****** OpenGL Vector multiplication operator - Scalar ******/
gx_vector operator* (const gx_vector &input_vector, float scalar)
{
	gx_vector output_vector(input_vector.size);

	for(u32 x = 0; x < output_vector.size; x++) { output_vector.data[x] = input_vector.data[x] * scalar; }

	return output_vector;
}

/****** OpenGL Vector bracket operator - Getter ******/
float gx_vector::operator[](u32 index) const
{
	return data[index];
}

/****** OpenGL Vector bracket operator - Setter ******/
float &gx_vector::operator[](u32 index)
{
	return data[index];
}

/****** OpenGL Matrix Constructor ******/
gx_matrix::gx_matrix()
{
	//When no arguments are given, generate a 4x4 identity matrix
	data.resize(4);

	for(u32 x = 0; x < 4; x++)
	{
		data[x].resize(4, 0.0);
	}

	data[0][0] = 1;
	data[1][1] = 1;
	data[2][2] = 1;
	data[3][3] = 1;

	rows = 4;
	columns = 4;
}

/****** OpenGL Matrix Constructor ******/
gx_matrix::gx_matrix(u32 input_columns, u32 input_rows)
{
	//Generate the columns
	data.resize(input_columns);

	//Generate the rows
	for(u32 x = 0; x < input_columns; x++)
	{
		data[x].resize(input_rows);
	}

	rows = input_rows;
	columns = input_columns;
}

/****** OpenGL Matrix Destructor ******/
gx_matrix::~gx_matrix()
{
	data.clear();
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
					dot_product += (data[x][y] * input_matrix.data[dot_product_count][x]);
				}

				output_matrix.data[dot_product_count][y] = dot_product;
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

/****** OpenGL Matrix multiplication operator - Matrix-Vector ******/
gx_vector operator* (const gx_matrix &input_matrix, const gx_vector &input_vector)
{
	//IMPORTANT - Matrix * Vector = VECTOR
	gx_vector output_vector(input_vector.size);

	//Determine if matrix can be multiplied
	if(input_matrix.columns == input_vector.size)
	{
		//This is essentially multiplying the input matrix by a 1-column matrix
		for(u32 y = 0; y < input_matrix.rows; y++)
		{
			float dot_product = 0.0;

			for(u32 x = 0; x < input_matrix.columns; x++)
			{
				dot_product += (input_matrix.data[x][y] * input_vector.data[x]);
			}

			output_vector.data[y] = dot_product;
		}
	}

	//Otherwise, return original vector
	else
	{
		for(u32 x = 0; x < input_vector.size; x++) { output_vector.data[x] = input_vector.data[x]; }
	}

	return output_vector;	
}

/****** OpenGL Matrix multiplication operator - Scalar ******/
gx_matrix operator*(float scalar, const gx_matrix &input_matrix)
{
	gx_matrix output_matrix(input_matrix.rows, input_matrix.columns);

	for(u32 y = 0; y < output_matrix.rows; y++)
	{
		for(u32 x = 0; x < output_matrix.columns; x++)
		{
			output_matrix.data[x][y] = input_matrix.data[x][y];
			output_matrix.data[x][y] *= scalar;
		}
	}

	return output_matrix;
}

/****** OpenGL Matrix multiplication operator - Scalar ******/
gx_matrix operator*(const gx_matrix &input_matrix, float scalar)
{
	gx_matrix output_matrix(input_matrix.rows, input_matrix.columns);

	for(u32 y = 0; y < output_matrix.rows; y++)
	{
		for(u32 x = 0; x < output_matrix.columns; x++)
		{
			output_matrix.data[x][y] = input_matrix.data[x][y];
			output_matrix.data[x][y] *= scalar;
		}
	}

	return output_matrix;
}

/****** OpenGL Matrix bracket operator - Getter ******/
std::vector<float> gx_matrix::operator[](u32 index) const
{
	return data[index];
}

/****** OpenGL Matrix bracket operator - Setter ******/
std::vector<float> &gx_matrix::operator[](u32 index)
{
	return data[index];
}

/****** Generates an orthogonal projection matrix ******/
gx_matrix ortho_matrix(float width, float height, float z_far, float z_near)
{
	gx_matrix output_matrix(4, 4);

	output_matrix[0][0] = 2.0 / width;
	output_matrix[3][0] = -1.0;

	output_matrix[1][1] = 2.0 / height;
	output_matrix[3][1] = 1.0;

	output_matrix[2][2] = -2.0 /(z_far - z_near);
	output_matrix[2][3] = -1.0 * ((z_far + z_near)/(z_far - z_near));

	output_matrix[3][3] = 1.0; 

	return output_matrix;
}

/****** Inverts a 2x2 matrix if applicable ******/
bool gx_matrix::invert_2x2()
{
	//Check matrix size first - Do nothing if this is not a 2x2 matrix
	if((rows != 2) || (columns != 2)) { return false; }

	//Check determinant (AD - BC) - Do nothing if zero
	float determinant = (data[0][0] * data[1][1]) - (data[1][0] * data[0][1]);
	if(determinant == 0) { return false; }

	determinant = 1.0 / determinant;

	float a = data[0][0];
	float b = data[1][0];
	float c = data[0][1];
	float d = data[1][1];

	data[0][0] = d;
	data[1][0] = -b;
	data[0][1] = -c;
	data[1][1] = a;

	//Multiply matrix values
	data[0][0] *= determinant;
	data[1][0] *= determinant;
	data[0][1] *= determinant;
	data[1][1] *= determinant;

	return true;
}

/****** Makes an identity matrix of a given size ******/
void gx_matrix::make_identity(u32 size)
{
	if(!size) { return; }

	resize(size, size);

	//Zero out matrix
	clear();

	//Make identity
	for(int x = 0; x < size; x++) { data[x][x] = 1.0; }
}

/****** Clears all of the matrix data (sets everything to zero) ******/
void gx_matrix::clear()
{
	//This does not delete the matrix, just the data inside
	for(u32 y = 0; y < rows; y++)
	{
		for(u32 x = 0; x < columns; x++)
		{
			data[x][y] = 0;
		}
	}
}

/****** Clears a matrix and resizes to given dimensions ******/
void gx_matrix::resize(u32 input_columns, u32 input_rows)
{
	if(!input_columns || !input_rows) { return; }

	data.resize(input_columns);

	for(u32 x = 0; x < input_columns; x++) { data[x].resize(input_rows, 0.0); }

	columns = input_columns;
	rows = input_rows;
}

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
	std::vector<char> vs_error(log_length);
	glGetShaderInfoLog(vertex_shader_id, log_length, NULL, &vs_error[0]);

	//Print any error messages from compiling vertex shader
	std::cout<<"OGL::Vertex Shader Error Message Log: " << &vs_error[0] << "\n";
 
	//Compile fragment shader
	std::cout<<"OGL::Compiling fragment shader : " << fragment_shader_file << "\n"; 
    	char const * fs_code_pointer = fs_code.c_str();
    	glShaderSource(fragment_shader_id, 1, &fs_code_pointer, NULL);
    	glCompileShader(fragment_shader_id);
 
	//Check fragment Shader
	glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result);
	glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &log_length);
	std::vector<char> fs_error(log_length);
	glGetShaderInfoLog(fragment_shader_id, log_length, NULL, &fs_error[0]);

	//Print any error messages from compiling fragment shader
	std::cout<<"OGL::Fragment Shader Error Message Log: " << &fs_error[0] << "\n";
 
	//Link the program
	std::cout<<"OGL::Linking shaders...\n";

	GLuint program_id = glCreateProgram();
	glAttachShader(program_id, vertex_shader_id);
	glAttachShader(program_id, fragment_shader_id);
	glLinkProgram(program_id);
 
	//Check the program
	glGetProgramiv(program_id, GL_LINK_STATUS, &result);
	glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
	std::vector<char> program_error(log_length);
	glGetProgramInfoLog(program_id, log_length, NULL, &program_error[0]);

	//Print any error messages from the linking process
	std::cout<<"OGL::Linking Error Message Log: " << &program_error[0] << "\n";
	
	glDeleteShader(vertex_shader_id);
	glDeleteShader(fragment_shader_id);
 
	return program_id;
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
