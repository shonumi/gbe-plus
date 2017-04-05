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
	SDL_Surface* final_screen = NULL;
}

/****** Renders an LCD's screen buffer to a QImage ******/
void render_screen_sw(std::vector<u32>& image) 
{
	//Manually request screen size before PaintEvent takes effect
	//Used for DMG/GBC games on GBA
	if(config::request_resize)
	{
		if(qt_gui::screen != NULL) { delete qt_gui::screen; }
		qt_gui::screen = new QImage(config::sys_width, config::sys_height, QImage::Format_ARGB32);
	}

	int width, height = 0;

	//Determine the dimensions of the source image
	//GBA = 240x160, GB-GBC = 160x144, NDS = 256x384
	if(config::gb_type == 3) { width = 240; height = 160; }
	else if(config::gb_type == 4) { width = 256; height = 384; }
	else { width = config::sys_width; height = config::sys_height; }

	//Fill in image with pixels from the emulated LCD
	for(int y = 0; y < height; y++)
	{
		u32* pixel_data = (u32*)qt_gui::screen->scanLine(y);

		for(int x = 0; x < width; x++) { pixel_data[x] = image[x + (y*width)]; }
	}

	if(qt_gui::draw_surface != NULL) { qt_gui::draw_surface->update(); }

	QApplication::processEvents();
}

/****** Renders an LCD's screen buffer to an SDL Surface ******/
void render_screen_hw(SDL_Surface* image) 
{
	if(config::request_resize) { qt_gui::draw_surface->update(); }

	qt_gui::final_screen = image;

	if(qt_gui::draw_surface != NULL) { qt_gui::draw_surface->hw_screen->updateGL(); }

	QApplication::processEvents();
}
