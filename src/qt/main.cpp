// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : main.cpp
// Date : June 18, 2015
// Description : The emulator (Qt version)
//
// This is main. It all begins here ;)

#include "render.h"
 
int main(int argc, char* args[]) 
{
	config::use_external_interfaces = true;

	QApplication::setAttribute(Qt::AA_X11InitThreads);
	QApplication app(argc, args);

	//Initialize SDL subsystems
	SDL_Init(SDL_INIT_EVERYTHING);

	main_menu window;
	qt_gui::draw_surface = &window;

	QIcon icon(QString::fromStdString(config::cfg_path + "data/icons/gbe_plus.png")); 

	window.resize(450, 300);
	window.setWindowTitle("GBE+");
	window.setWindowIcon(icon);
	window.show();
 
	return app.exec();
} 
