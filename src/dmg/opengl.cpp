// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : opengl.cpp
// Date : May 29, 2015
// Description : Handles OpenGL functionality
//
// Sets up OpenGL for use in GBE+
// Handles blit operations to the screen

#include "lcd.h"

/****** Initialize OpenGL through SDL ******/
void DMG_LCD::opengl_init()
{
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

	window = SDL_CreateWindow("GBE+", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, (config::sys_width * config::scaling_factor), (config::sys_height * config::scaling_factor), config::flags | SDL_WINDOW_OPENGL);
	SDL_GetWindowSize(window, &config::win_width, &config::win_height);

	gl_context = SDL_GL_CreateContext(window);

	//Calculate new temporary scaling factor
	u32 max_width = config::win_width / config::sys_width;
	u32 max_height = config::win_height / config::sys_height;

	//Find the maximum dimensions that maintain the original aspect ratio
	if(max_width <= max_height) { config::scaling_factor = max_width; }
	else { config::scaling_factor = max_height; }

	//Calculate shifting X-Y if entering fullscreen mode
	u32 shift_x = (config::win_width / 2) - ((config::sys_width * config::scaling_factor) / 2);
	u32 shift_y = (config::win_height / 2) - ((config::sys_height * config::scaling_factor) / 2);
	
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0, 0, 0, 0);

	glViewport(shift_x, -shift_y, config::win_width, config::win_height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0, config::win_width, config::win_height, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glGenTextures(1, &lcd_texture);
	glBindTexture(GL_TEXTURE_2D, lcd_texture);

	final_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, config::sys_width, config::sys_height, 32, 0, 0, 0, 0);
}

/****** Blit using OpenGL ******/
void DMG_LCD::opengl_blit()
{
	glTexImage2D(GL_TEXTURE_2D, 0, 4, config::sys_width, config::sys_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, final_screen->pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	int width = ((config::sys_width >> 1) * config::scaling_factor);
	int height = ((config::sys_height >> 1) * config::scaling_factor);

	glTranslatef(width, height, 0);

	glBindTexture(GL_TEXTURE_2D, lcd_texture);
	glBegin(GL_QUADS);

	//Top Left
	glTexCoord2i(0, 1);
	glVertex2i(-width, height);

	//Top Right
	glTexCoord2i(1, 1);
	glVertex2i(width, height);

	//Bottom Right
	glTexCoord2i(1, 0);
	glVertex2i(width, -height);

	//Bottom Left
	glTexCoord2f(0, 0);
	glVertex2i(-width, -height);
	glEnd();

	glLoadIdentity();

	SDL_GL_SwapWindow(window);
}