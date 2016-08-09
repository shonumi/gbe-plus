// GB Enhanced Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : ogl_util.cpp
// Date : August 08, 2016
// Description : OpenGL math utilities
//
// Provides OpenGL math utilities such as matrix handling and transformations
// Handles loading shaders

#include "ogl_util.h"

/****** OpenGL Matrix Constructor ******/
ogl_matrix::ogl_matrix()
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
ogl_matrix::ogl_matrix(u32 input_columns, u32 input_rows)
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
ogl_matrix::~ogl_matrix()
{
	data.clear();
}

/****** OpenGL Matrix multiplication operator ******/
ogl_matrix ogl_matrix::operator*(const ogl_matrix &input_matrix)
{
	//Determine if matrix can be multiplied
	if(columns == input_matrix.rows)
	{
		//Determine size of output matrix
		u32 output_rows = rows;
		u32 output_columns = input_matrix.columns;

		ogl_matrix output_matrix(output_rows, output_columns);

		//Find the dot product for all values in the new matrix
		for(u32 y = 0; y < rows; y++)
		{
			for(u32 dot_product_count = 0; dot_product_count < output_columns; dot_product_count++)
			{
				double dot_product = 0.0;

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
		ogl_matrix output_matrix;
		return output_matrix;
	}
}
