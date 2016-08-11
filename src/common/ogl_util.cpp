// GB Enhanced Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : ogl_util.cpp
// Date : August 08, 2016
// Description : OpenGL math utilities
//
// Provides OpenGL math utilities such as matrix handling and transformations, vector stuff
// Handles loading shaders

#include "ogl_util.h"

/****** OpenGL Vector Constructor ******/
ogl_vector::ogl_vector()
{
	//When no arguments are given, generate a vector with 4 elements
	data.resize(4, 0.0);

	size = 4;
}

/****** OpenGL Vector Constructor ******/
ogl_vector::ogl_vector(u32 input_size)
{
	data.resize(input_size, 0.0);

	size = input_size;
}

/****** OpenGL Vector Destructor ******/
ogl_vector::~ogl_vector()
{
	data.clear();
}

/****** OpenGL Vector addition operator ******/
ogl_vector ogl_vector::operator+ (const ogl_vector &input_vector)
{
	ogl_vector output_vector(size);

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
ogl_vector ogl_vector::operator- (const ogl_vector &input_vector)
{
	ogl_vector output_vector(size);

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
ogl_vector ogl_vector::operator* (const ogl_vector &input_vector)
{
	ogl_vector output_vector(size);

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
ogl_vector operator* (float scalar, const ogl_vector &input_vector)
{
	ogl_vector output_vector(input_vector.size);

	for(u32 x = 0; x < output_vector.size; x++) { output_vector.data[x] = input_vector.data[x] * scalar; }

	return output_vector;
}

/****** OpenGL Vector multiplication operator - Scalar ******/
ogl_vector operator* (const ogl_vector &input_vector, float scalar)
{
	ogl_vector output_vector(input_vector.size);

	for(u32 x = 0; x < output_vector.size; x++) { output_vector.data[x] = input_vector.data[x] * scalar; }

	return output_vector;
}

/****** OpenGL Vector bracket operator - Getter ******/
float ogl_vector::operator[](u32 index) const
{
	return data[index];
}

/****** OpenGL Vector bracket operator - Setter ******/
float &ogl_vector::operator[](u32 index)
{
	return data[index];
}

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

/****** OpenGL Matrix multiplication operator - Matrix-Matrix ******/
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
		ogl_matrix output_matrix;
		return output_matrix;
	}
}

/****** OpenGL Matrix multiplication operator - Matrix-Vector ******/
ogl_vector operator* (const ogl_matrix &input_matrix, const ogl_vector &input_vector)
{
	//IMPORTANT - Matrix * Vector = VECTOR
	ogl_vector output_vector(input_vector.size);

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
ogl_matrix operator*(float scalar, const ogl_matrix &input_matrix)
{
	ogl_matrix output_matrix(input_matrix.rows, input_matrix.columns);

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
ogl_matrix operator*(const ogl_matrix &input_matrix, float scalar)
{
	ogl_matrix output_matrix(input_matrix.rows, input_matrix.columns);

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
std::vector<float> ogl_matrix::operator[](u32 index) const
{
	return data[index];
}

/****** OpenGL Matrix bracket operator - Setter ******/
std::vector<float> &ogl_matrix::operator[](u32 index)
{
	return data[index];
}

/****** Generates an orthogonal projection matrix ******/
ogl_matrix ortho_matrix(float width, float height, float z_far, float z_near)
{
	ogl_matrix output_matrix(4, 4);

	output_matrix[0][0] = 2.0 / width;
	output_matrix[3][0] = -1.0;

	output_matrix[1][1] = 2.0 / height;
	output_matrix[3][1] = 1.0;

	output_matrix[2][2] = -2.0 /(z_far - z_near);
	output_matrix[2][3] = -1.0 * ((z_far + z_near)/(z_far - z_near));

	output_matrix[3][3] = 1.0; 

	return output_matrix;
}
