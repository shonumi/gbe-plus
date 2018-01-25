// GB Enhanced Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gx_util.h
// Date : August 08, 2016
// Description : General 3D math utilities
//
// Provides 3D math utilities such as matrix handling and transformations, vector stuff
// Handles OpenGL specific stuff like loading shaders

#ifndef GBE_GX_UTIL
#define GBE_GX_UTIL

#include <vector>
#include <string>

#include <SDL2/SDL_opengl.h>

#include "common.h"

//Vector class
class gx_vector
{
	public:
	
	gx_vector();
	gx_vector(u32 input_size);
	~gx_vector();

	//Vector-Vector addition, subtraction, multiplication
	gx_vector operator+ (const gx_vector &input_vector);
	gx_vector operator- (const gx_vector &input_vector);
	gx_vector operator* (const gx_vector &input_vector);

	//Access vector data
	float operator[](u32 index) const;
	float &operator[](u32 index);

	u32 size;

	//Vector data
	std::vector<float> data;
};

//Scalar multiplication - Non-member binary operators
gx_vector operator*(float scalar, const gx_vector &input_vector);
gx_vector operator*(const gx_vector &input_vector, float scalar);

//Matrix class
class gx_matrix
{
	public:

	gx_matrix();
	gx_matrix(u32 input_columns, u32 input_rows);
	~gx_matrix();

	//Matrix-Matrix multiplication
	gx_matrix operator* (const gx_matrix &input_matrix);

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
gx_matrix operator*(float scalar, const gx_matrix &input_matrix);
gx_matrix operator*(const gx_matrix &input_matrix, float scalar);

//Matrix-Vector multiplication - Non-member binary operators
gx_vector operator* (const gx_matrix &input_matrix, const gx_vector &input_vector);

//Projection matrix
gx_matrix ortho_matrix(float width, float height, float z_far, float z_near);

//GLSL vertex and fragment shader loader
GLuint gx_load_shader(std::string vertex_shader_file, std::string fragment_shader_file, u32 &external_data_usage);

#endif // GBE_GX_UTIL
