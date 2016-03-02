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

	glViewport(0, 0, (config::sys_width * config::scaling_factor), (config::sys_height * config::scaling_factor));
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0, (config::sys_width * config::scaling_factor), (config::sys_height * config::scaling_factor), 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glGenTextures(1, &lcd_texture);
	glBindTexture(GL_TEXTURE_2D, lcd_texture);

	makeCurrent();
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
		QImage final_screen = QGLWidget::convertToGLFormat(*qt_gui::screen);

		glTexImage2D(GL_TEXTURE_2D, 0, 4, config::sys_width, config::sys_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, final_screen.bits());

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
	}
}  
