// GB Enhanced Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : ogl_util.h
// Date : August 08, 2016
// Description : OpenGL math utilities
//
// Provides OpenGL math utilities such as matrix handling and transformations, vector stuff
// Handles loading shaders

#ifndef GBE_OGL_UTIL
#define GBE_OGL_UTIL

#include <vector>
#include <string>

#include <SDL2/SDL_opengl.h>

#include "common.h"

//Vector class
class ogl_vector
{
	public:
	
	ogl_vector();
	ogl_vector(u32 input_size);
	~ogl_vector();

	//Vector-Vector addition, subtraction, multiplication
	ogl_vector operator+ (const ogl_vector &input_vector);
	ogl_vector operator- (const ogl_vector &input_vector);
	ogl_vector operator* (const ogl_vector &input_vector);

	//Access vector data
	float operator[](u32 index) const;
	float &operator[](u32 index);

	u32 size;

	//Vector data
	std::vector<float> data;
};

//Scalar multiplication - Non-member binary operators
ogl_vector operator*(float scalar, const ogl_vector &input_vector);
ogl_vector operator*(const ogl_vector &input_vector, float scalar);

//Matrix class
class ogl_matrix
{
	public:

	ogl_matrix();
	ogl_matrix(u32 input_columns, u32 input_rows);
	~ogl_matrix();

	//Matrix-Matrix multiplication
	ogl_matrix operator* (const ogl_matrix &input_matrix);

	//Matrix 2x2 Inversion
	bool invert_2x2();	

	//Access matrix data
	std::vector<float> operator[](u32 index) const;
	std::vector<float> &operator[](u32 index);

	void clear();

	u32 rows;
	u32 columns;

	//Matrix data
	std::vector< std::vector<float> > data;
};

//Scalar multiplication - Non-member binary operators
ogl_matrix operator*(float scalar, const ogl_matrix &input_matrix);
ogl_matrix operator*(const ogl_matrix &input_matrix, float scalar);

//Matrix-Vector multiplication - Non-member binary operators
ogl_vector operator* (const ogl_matrix &input_matrix, const ogl_vector &input_vector);

//Projection matrix
ogl_matrix ortho_matrix(float width, float height, float z_far, float z_near);

//GLSL vertex and fragment shader loader
GLuint ogl_load_shader(std::string vertex_shader_file, std::string fragment_shader_file, u32 &external_data_usage);

#endif // GBE_OGL_UTIL
