// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : screens.h
// Date : February 29, 2015
// Description : Software and OpenGL rendering widgets
//
// Widgets for rendering via software, or OpenGL for HW acceleration

#ifndef SCREENS_GBE_QT
#define SCREENS_GBE_QT

#include <QtWidgets>
#include <QGLWidget>
#include <QGLFormat>

#include "ogl_manager.h"

#include "common/common.h"

class soft_screen : public QWidget
{
	Q_OBJECT
	
	public:
	soft_screen(QWidget *parent = 0);

	protected:
	void paintEvent(QPaintEvent* event);
	void resizeEvent(QResizeEvent* event);
};

class hard_screen : public QGLWidget
{
	Q_OBJECT
	
	public:
	hard_screen(QWidget *parent = 0);
	QGLFormat screen_format;

	ogl_manager gwin;

	bool old_aspect_flag;

	void reload_shaders();
	void calculate_screen_size();

	protected:
	void initializeGL();
	void paintGL();
	void resizeEvent(QResizeEvent* event);
};

#endif //SCREENS_GBE_QT 
