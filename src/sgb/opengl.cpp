// GB Enhanced+ Copyright Daniel Baxter 2017
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : opengl.cpp
// Date : June 17, 2017
// Description : Handles OpenGL functionality
//
// Sets up OpenGL for use in GBE+
// Handles blit operations to the screen

#ifndef GL3_PROTOTYPES
#define GL3_PROTOTYPES 1
#endif

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

#ifdef GBE_GLEW
#include "GL/glew.h"
#endif

#include <ctime>

#include "lcd.h"

/****** Initialize OpenGL through SDL ******/
bool SGB_LCD::opengl_init()
{
	#ifdef GBE_OGL

	bool result = false;

	SDL_GL_DeleteContext(gl_data::gl_context);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	u32 screen_width = (config::sys_width * config::scaling_factor);
	u32 screen_height = (config::sys_height * config::scaling_factor);

	if(config::flags & SDL_WINDOW_FULLSCREEN)
	{
		SDL_DisplayMode current_mode;
		SDL_GetCurrentDisplayMode(0, &current_mode);

		screen_width = current_mode.w;
		screen_height = current_mode.h;
	}

	//Create OpenGL Window, Surface, and OpenGL context
	final_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, config::sys_width, config::sys_height, 32, 0, 0, 0, 0);
	window = SDL_CreateWindow("GBE+", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, config::flags | SDL_WINDOW_OPENGL);
	SDL_GetWindowSize(window, &config::win_width, &config::win_height);
	gl_data::gl_context = SDL_GL_CreateContext(window);

	result = gx_init_opengl();
	
	return result;

	#endif

	return false;
}

