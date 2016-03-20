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
		//Maintain aspect ratio
		if(config::maintain_aspect_ratio)
		{
			QImage final_screen = qt_gui::screen->scaled(width(), height(), Qt::KeepAspectRatio);
			QPainter painter(this);

			int x_offset = (width() - final_screen.width()) / 2;
			int y_offset = (height() - final_screen.height()) / 2;

			painter.drawImage(x_offset, y_offset, final_screen);	
		}

		//Ignore aspect ratio
		else
		{
			QImage final_screen = qt_gui::screen->scaled(width(), height());
			QPainter painter(this);
			painter.drawImage(0, 0, final_screen);
		}
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

		//Maintain aspect ratio
		if(config::maintain_aspect_ratio)
		{
			double max_width, max_height, ratio = 0.0;

			//Find the maximum dimensions that maintain the original aspect ratio
			if(width() <= height())
			{
				ratio = width() / (double)config::sys_width;
				max_width = width();
				max_height = config::sys_height * ratio;
			}

			else
			{
				ratio = height() / (double)config::sys_height;
				max_height = height();
				max_width = config::sys_width * ratio;
			}

			max_width = max_width / width();
			max_height = max_height / height();

			//Convert those dimensions to OpenGL coordinates
			double left, right, top, bottom = 0.0;			
			left = .5 - max_width;
			right = .5 + max_width;
			top = -.5 + max_height;
			bottom = -.5 - max_height;

			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(left, top);
			glTexCoord2f(1.0f, 0.0f); glVertex2f(right, top);
			glTexCoord2f(1.0f, 1.0f); glVertex2f(right, bottom);
			glTexCoord2f(0.0f, 1.0f); glVertex2f(left, bottom);
			glEnd();
		}

		//Ignore aspect ratio
		else
		{
			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(-0.5f, 0.5f);
			glTexCoord2f(1.0f, 0.0f); glVertex2f(1.5f, 0.5f);
			glTexCoord2f(1.0f, 1.0f); glVertex2f(1.5f, -1.5f);
			glTexCoord2f(0.0f, 1.0f); glVertex2f(-0.5f, -1.5f);
			glEnd();
		}
	}
}  
