// GB Enhanced Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : ogl_util.h
// Date : August 08, 2016
// Description : OpenGL math utilities
//
// Provides OpenGL math utilities such as matrix handling and transformations
// Handles loading shaders

#ifndef GBE_OGL_UTIL
#define GBE_OGL_UTIL

#include <vector>

#include "common.h"

//Matrix class
class ogl_matrix
{
	public:

	ogl_matrix();
	ogl_matrix(u32 input_columns, u32 input_rows);
	~ogl_matrix();

	ogl_matrix operator* (const ogl_matrix &input_matrix);

	u32 rows;
	u32 columns;

	//Matrix data
	std::vector< std::vector<double> > data;
};

#endif // GBE_OGL_UTIL