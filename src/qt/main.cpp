// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : main.cpp
// Date : July 18, 2015
// Description : The emulator (Qt version)
//
// This is main. It all begins here ;)

#include "render.h"
 
int main(int argc, char* args[]) 
{
	QApplication::setAttribute(Qt::AA_X11InitThreads);
	QApplication app(argc, args);

	//Initialize SDL subsystems - Do not initialize audio/video
	SDL_Init(SDL_INIT_EVERYTHING);

	main_menu window;
	qt_gui::draw_surface = &window;

	window.resize(350, 150);
	window.setWindowTitle("GBE+");
	window.show();
 
	return app.exec();
} 
