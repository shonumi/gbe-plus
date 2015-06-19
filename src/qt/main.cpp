// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : main.cpp
// Date : July 18, 2015
// Description : The emulator (Qt version)
//
// This is main. It all begins here ;)

#include <QApplication>

#include "main_menu.h"
 
int main(int argc, char* args[]) 
{
	QApplication app(argc, args);
 
	main_menu window;

	window.resize(350, 150);
	window.setWindowTitle("GBE+");
	window.show();
 
	return app.exec();
} 
