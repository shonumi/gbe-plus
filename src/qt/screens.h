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

#ifdef GBE_QT_5
#include <QtWidgets>
#endif

#ifdef GBE_QT_4
#include <QtGui>
#endif

#include <QGLWidget>
#include <QGLFormat>

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
	GLuint lcd_texture;
	QGLFormat screen_format;

	GLuint program_id;
	GLuint vertex_buffer_object, vertex_array_object, element_buffer_object;
	GLfloat ogl_x_scale, ogl_y_scale;
	GLfloat ext_data_1, ext_data_2;
	u32 external_data_usage;

	bool old_aspect_flag;

	void reload_shaders();
	void calculate_screen_size();

	protected:
	void initializeGL();
	void paintGL();
	void resizeEvent(QResizeEvent* event);
};

#endif //SCREENS_GBE_QT 
