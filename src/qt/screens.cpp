// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : screens.cpp
// Date : February 29, 2015
// Description : Software and OpenGL rendering widgets
//
// Widgets for rendering via software, or OpenGL for HW acceleration

#include "screens.h"
#include "render.h"

/****** Software screen constructor ******/
soft_screen::soft_screen(QWidget *parent) : QWidget(parent) { }

/****** Software screen paint event ******/
void soft_screen::paintEvent(QPaintEvent* event)
{
	if(qt_gui::screen == NULL)
	{
		QPainter painter(this);
		painter.setBrush(Qt::black);
		painter.drawRect(0, 0, width(), height());
	}

	else
	{
		QImage final_screen = qt_gui::screen->scaled(width(), height());
		QPainter painter(this);
		painter.drawImage(0, 0, final_screen);
	}
}

/****** Hardware screen constructor ******/
hard_screen::hard_screen(QWidget *parent) : QGLWidget(parent) { }

/****** Initializes OpenGL on the hardware screen ******/
void hard_screen::initializeGL()
{
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0, 0, 0, 0);

	glGenTextures(1, &lcd_texture);
	glBindTexture(GL_TEXTURE_2D, lcd_texture);
}

/****** Hardware screen paint event ******/
void hard_screen::paintGL()
{
	if(qt_gui::screen == NULL)
	{
		QPainter painter(this);
		painter.setBrush(Qt::black);
		painter.drawRect(0, 0, width(), height());
	}

	else
	{
		glViewport(0, 0, width(), height());
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		glTexImage2D(GL_TEXTURE_2D, 0, 4, config::sys_width, config::sys_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, qt_gui::final_screen->pixels);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		glTranslatef(-0.5, 0.5, 0);

		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(-0.5f, 0.5f);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(1.5f, 0.5f);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(1.5f, -1.5f);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(-0.5f, -1.5f);
		glEnd();
	}
}  
