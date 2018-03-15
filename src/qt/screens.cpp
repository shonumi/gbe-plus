// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : screens.cpp
// Date : February 29, 2015
// Description : Software and OpenGL rendering widgets
//
// Widgets for rendering via software, or OpenGL for HW acceleration

#include <ctime>

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

/****** Software screen resize event ******/
void soft_screen::resizeEvent(QResizeEvent* event)
{
	if(main_menu::gbe_plus == NULL) { return; }

	//Grab test dimensions, look at max resolution for fullscreen dimensions
	u32 test_width = qt_gui::draw_surface->fullscreen_mode ? qt_gui::draw_surface->display_width : qt_gui::draw_surface->width();
	u32 test_height = qt_gui::draw_surface->fullscreen_mode ? qt_gui::draw_surface->display_height : (qt_gui::draw_surface->height() - qt_gui::draw_surface->menu_height);

	//Adjust screen to fit expected dimensions no matter what
	if((width() != test_width) || (height() != test_height))
	{
		resize(test_width, test_height);
	}
}

/****** Hardware screen constructor ******/
hard_screen::hard_screen(QWidget *parent) : QGLWidget(parent)
{
	//Set up Qt to use OpenGL 3.3
	screen_format.setVersion(3, 3);
	screen_format.setProfile(QGLFormat::CoreProfile);
	setFormat(screen_format);

	old_aspect_flag = config::maintain_aspect_ratio;
}

/****** Initializes OpenGL on the hardware screen ******/
void hard_screen::initializeGL()
{
	config::win_width = width();
	config::win_height = height();

	//Calculate new temporary scaling factor
	float max_width = (float)config::win_width / config::sys_width;
	float max_height = (float)config::win_height / config::sys_height;

	//Find the maximum dimensions that maintain the original aspect ratio
	if(config::flags & SDL_WINDOW_FULLSCREEN)
	{
		if(max_width <= max_height)
		{
			float max_x_size = (max_width * config::sys_width);
			float max_y_size = (max_width * config::sys_height);

			gwin.ogl_x_scale =  max_x_size / config::win_width;
			gwin.ogl_y_scale =  max_y_size / config::win_height;
		}

		else
		{
			float max_x_size = (max_height * config::sys_width);
			float max_y_size = (max_height * config::sys_height);

			gwin.ogl_x_scale =  max_x_size / config::win_width;
			gwin.ogl_y_scale =  max_y_size / config::win_height;
		}
	}

	else
	{
		gwin.ogl_x_scale = (float)config::win_width / (config::sys_width * config::scaling_factor);
		gwin.ogl_y_scale = (float)config::win_height / (config::sys_height * config::scaling_factor);
	}

	gwin.ext_data_1 = gwin.ext_data_2 = 1.0;
	gwin.init();
	gwin.resize(width(), height());
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
		if(old_aspect_flag != config::maintain_aspect_ratio)
		{
			old_aspect_flag = config::maintain_aspect_ratio;
			calculate_screen_size();
		}

		//Determine what the shader's external data usage is
		switch(gwin.external_data_usage)
		{
			//Shader requires no external data
			case 0: break;

			//Shader requires current window dimensions
			case 1:
				gwin.ext_data_1 = config::win_width;
				gwin.ext_data_2 = config::win_height;
				break;

			//Shader requires current time
			case 2:
				{
					time_t system_time = time(0);
					tm* current_time = localtime(&system_time);

					gwin.ext_data_1 = current_time->tm_hour;
					gwin.ext_data_2 = current_time->tm_min;
				}

				break;

			default: break;
		}

		gwin.pixel_data = qt_gui::final_screen->pixels;
		gwin.paint();
	}
}

/****** Hardware screen resize event ******/
void hard_screen::resizeEvent(QResizeEvent* event)
{
	if(main_menu::gbe_plus == NULL) { return; }

	//Grab test dimensions, look at max resolution for fullscreen dimensions
	u32 test_width = qt_gui::draw_surface->fullscreen_mode ? qt_gui::draw_surface->display_width : qt_gui::draw_surface->width();
	u32 test_height = qt_gui::draw_surface->fullscreen_mode ? qt_gui::draw_surface->display_height : (qt_gui::draw_surface->height() - qt_gui::draw_surface->menu_height);

	//Adjust screen to fit expected dimensions no matter what
	if((width() != test_width) || (height() != test_height))
	{
		resize(test_width, test_height);
	}

	config::win_width = width();
	config::win_height = height();

	gwin.resize(width(), height());
	calculate_screen_size();
}

/****** Reloads fragment and vertex shaders ******/
void hard_screen::reload_shaders()
{
	gwin.program_id = gx_load_shader(config::vertex_shader, config::fragment_shader, gwin.external_data_usage);
}

/****** Calculates aspect ratio or stretched ******/
void hard_screen::calculate_screen_size()
{
	if(config::maintain_aspect_ratio)
	{
		float max_width = (float)config::win_width / config::sys_width;
		float max_height = (float)config::win_height / config::sys_height;

		if(max_width <= max_height)
		{
			float max_x_size = (max_width * config::sys_width);
			float max_y_size = (max_width * config::sys_height);

			gwin.ogl_x_scale =  max_x_size / config::win_width;
			gwin.ogl_y_scale =  max_y_size / config::win_height;
		}

		else
		{
			float max_x_size = (max_height * config::sys_width);
			float max_y_size = (max_height * config::sys_height);

			gwin.ogl_x_scale =  max_x_size / config::win_width;
			gwin.ogl_y_scale =  max_y_size / config::win_height;
		}
	}

	else { gwin.ogl_x_scale = gwin.ogl_y_scale = 1.0; }
}
