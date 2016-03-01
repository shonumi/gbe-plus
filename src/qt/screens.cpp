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

/****** Software screen constructor ******/
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

		std::cout<<"SIZE -> " << qt_gui::draw_surface->menu_bar->height() << "\n";
	}
}

/****** Hardware screen constructor ******/
hard_screen::hard_screen(QWidget *parent) : QGLWidget(parent) { }

/****** Hardware screen constructor ******/
void hard_screen::paintEvent(QPaintEvent* event) { }  
