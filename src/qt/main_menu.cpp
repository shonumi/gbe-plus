// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : main_menu.cpp
// Date : July 18, 2015
// Description : Main menu
//
// Main menu for the main window
// Has options like File, Emulation, Options, About...

#include "main_menu.h"
#include <QMenu>
#include <QMenuBar>

/****** Main menu constructor ******/
main_menu::main_menu(QWidget *parent) : QMainWindow(parent)
{
	//Setup actions
	QAction *open = new QAction("&Open", this);
	QAction *quit = new QAction ("&Quit", this);

	//Set shortcuts for actions
	open->setShortcut(tr("CTRL+O"));
	quit->setShortcut(tr("CTRL+Q"));

	//Setup File menu
	QMenu *file;

	file = menuBar()->addMenu("&File");
	file->addAction(open);
	file->addSeparator();

	file->addAction(quit);
}