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
#include <fstream>

#include <SDL.h>
#include <SDL_opengl.h>

#include "common.h"

//Matrix class
class gx_matrix
{
	public:

	gx_matrix();
	gx_matrix(u32 input_columns, u32 input_rows);
	~gx_matrix();

	//Matrix-Matrix multiplication
	gx_matrix operator* (const gx_matrix &input_matrix);

	//Access matrix data
	float operator[](u32 index) const;
	float &operator[](u32 index);

	void resize(u32 input_columns, u32 input_rows);
	void make_identity(u32 size);	

	u32 rows;
	u32 columns;

	//Matrix data
	float data[16];
};

#ifdef GBE_OGL

//OpenGL structure for cores
struct open_gl_data
{
	SDL_GLContext gl_context;
	GLuint lcd_texture;
	GLuint program_id;
	GLuint vertex_buffer_object, vertex_array_object, element_buffer_object;
	GLfloat x_scale, y_scale;
	GLfloat ext_data_1, ext_data_2;
	u32 external_data_usage;
};

//Initialize OpenGL for cores
bool gx_init_opengl(open_gl_data &ogl);

//OpenGL render for cores
void gx_blit_opengl(open_gl_data &ogl, SDL_Window *window, SDL_Surface* final_screen);

//GLSL vertex and fragment shader loader
GLuint gx_load_shader(std::string vertex_shader_file, std::string fragment_shader_file, u32 &external_data_usage);

#endif

//2D distance
float dist(float x1, float y1, float x2, float y2);

//3D distance
float dist(float x1, float y1, float z1, float x2, float y2, float z2);

//Serialize matrix data to/from binary file
bool serialize_matrix(std::ifstream &file, gx_matrix &mat);
bool serialize_matrix(std::ofstream &file, gx_matrix &mat);

#endif // GBE_GX_UTIL
