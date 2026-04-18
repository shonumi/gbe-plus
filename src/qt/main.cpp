// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : main.cpp
// Date : June 18, 2015
// Description : The emulator (Qt version)
//
// This is main. It all begins here ;)

#include "render.h"
#include "common/info.h"
 
#include <SDL_main.h>

int main(int argc, char* args[]) 
{
	std::cout<<"GBE+ 1.10 [Qt]\n";

	if(!gbe_info::get_hash().empty())
	{
		std::cout<<"Revision: " << gbe_info::get_hash() << "\n\n";
	}

	else
	{
		std::cout<<"Revision: N/A\n\n";
	}

	config::use_external_interfaces = true;

	QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
	QApplication app(argc, args);

	//Grab command-line arguments
	for(int x = 1; x < argc; x++) 
	{ 
		std::string temp_arg = args[x];
		config::cli_args.push_back(temp_arg);
	}

	main_menu window;
	qt_gui::draw_surface = &window;

	//Initialize SDL subsystems and hints, report specific init errors later in the core
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");

	QIcon icon(QString::fromStdString(config::cfg_path + "data/icons/gbe_plus.png")); 

	window.resize(450, 300);
	window.setWindowTitle("GBE+");
	window.setWindowIcon(icon);
	window.show();
	window.open_first_file();
 
	return app.exec();
} 
