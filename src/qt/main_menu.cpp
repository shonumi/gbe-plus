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

	QAction *pause = new QAction("&Pause", this);
	QAction *reset = new QAction("&Reset", this);
	QAction *fullscreen = new QAction("Fullscreen", this);
	QAction *screenshot = new QAction("Screenshot", this);

	QAction *general = new QAction("General Settings...", this);
	QAction *display = new QAction("Display", this);
	QAction *sound = new QAction("Sound", this);
	QAction *controls = new QAction("Controls", this);

	QAction *about = new QAction("About", this);

	//Set shortcuts for actions
	open->setShortcut(tr("CTRL+O"));
	quit->setShortcut(tr("CTRL+Q"));

	pause->setShortcut(tr("CTRL+P"));
	reset->setShortcut(tr("CTRL+R"));

	fullscreen->setCheckable(true);

	//Setup File menu
	QMenu *file;

	file = menuBar()->addMenu("&File");
	file->addAction(open);
	file->addSeparator();
	file->addAction(quit);

	//Setup Emulation menu
	QMenu *emulation;

	emulation = menuBar()->addMenu("&Emulation");
	emulation->addAction(pause);
	emulation->addAction(reset);
	emulation->addSeparator();
	emulation->addAction(fullscreen);
	emulation->addAction(screenshot);

	//Setup Options menu
	QMenu *options;
	
	options = menuBar()->addMenu("&Options");
	options->addAction(general);
	options->addSeparator();
	options->addAction(display);
	options->addAction(sound);
	options->addAction(controls);

	//Setup Help menu
	QMenu *help;

	help = menuBar()->addMenu("&Help");
	help->addAction(about);
}
