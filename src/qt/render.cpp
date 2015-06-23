// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : render.h
// Date : June 22, 2015
// Description : Qt screen rendering
//
// Renders the screen for an emulated system using Qt

#include "render.h"

#include "common/config.h"

namespace qt_gui
{
	QImage* screen = NULL;
	main_menu* draw_surface = NULL;
}

/****** Renders an LCD's screen buffer to a QImage ******/
void render_screen(std::vector<u32>& image) 
{
	int width, height = 0;

	//Determine the dimensions of the source image
	//GBA = 240x160, GB-GBC = 160x144
	if(config::gb_type == 3) { width = 240; height = 160; }
	else { width = 160; height = 144; }

	//Create new instance of the screen image
	if(qt_gui::screen == NULL)
	{
		qt_gui::screen = new QImage(reinterpret_cast<unsigned char*> (&image[0]), width, height, QImage::Format_ARGB32);
	}

	//Free memory the old instance, make a new one
	//TODO - There has to be a better way than this.
	else
	{
		delete qt_gui::screen;
		qt_gui::screen = new QImage(reinterpret_cast<unsigned char*> (&image[0]), width, height, QImage::Format_ARGB32);
	}

	if(qt_gui::draw_surface != NULL) { qt_gui::draw_surface->update(); }

	QApplication::processEvents();
}