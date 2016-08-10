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
	double operator[](u32 index) const;
	double &operator[](u32 index);

	u32 size;

	//Vector data
	std::vector<double> data;
};

//Scalar multiplication - Non-member binary operators
ogl_vector operator*(double scalar, const ogl_vector &input_vector);
ogl_vector operator*(const ogl_vector &input_vector, double scalar);

//Matrix class
class ogl_matrix
{
	public:

	ogl_matrix();
	ogl_matrix(u32 input_columns, u32 input_rows);
	~ogl_matrix();

	//Matrix-Matrix multiplication
	ogl_matrix operator* (const ogl_matrix &input_matrix);
	
	//Access matrix data
	std::vector<double> operator[](u32 index) const;
	std::vector<double> &operator[](u32 index);

	u32 rows;
	u32 columns;

	//Matrix data
	std::vector< std::vector<double> > data;
};

//Scalar multiplication - Non-member binary operators
ogl_matrix operator*(double scalar, const ogl_matrix &input_matrix);
ogl_matrix operator*(const ogl_matrix &input_matrix, double scalar);

//Matrix-Vector multiplication - Non-member binary operators
ogl_vector operator* (const ogl_matrix &input_matrix, const ogl_vector &input_vector);

#endif // GBE_OGL_UTIL
