// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : main_menu.h
// Date : July 18, 2015
// Description : Main menu
//
// Main menu for the main window
// Has options like File, Emulation, Options, About...

#ifndef MAINMENU_GBE_QT
#define MAINMENU_GBE_QT

#include <QtGui>

#include "gba/core.h"
#include "dmg/core.h"

class main_menu : public QWidget
{
	Q_OBJECT
	
	public:
	main_menu(QWidget *parent = 0);

	protected:
	void paintEvent(QPaintEvent *event);
	void closeEvent(QCloseEvent *event);

	private slots:
	void open_file();
	void quit();

	private:
	core_emu* gbe_plus;

	int menu_height;

	void boot_game();
};

#endif
