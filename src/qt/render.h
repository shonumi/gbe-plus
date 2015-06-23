// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : render.h
// Date : June 22, 2015
// Description : Qt screen rendering
//
// Renders the screen for an emulated system using Qt

#ifndef QT_RENDER
#define QT_RENDER

#include "main_menu.h"

#include <vector>

#include "common/common.h"

void render_screen(std::vector<u32>& image);

namespace qt_gui
{
	extern QImage* screen;
	extern main_menu* draw_surface;
}

#endif // QT_RENDER 
