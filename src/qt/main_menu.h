// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : main_menu.h
// Date : June 18, 2015
// Description : Main menu
//
// Main menu for the main window
// Has options like File, Emulation, Options, About...

#ifndef MAINMENU_GBE_QT
#define MAINMENU_GBE_QT

#include <QtGui>

#include "general_settings.h"
#include "cgfx.h"

#include "gba/core.h"
#include "dmg/core.h"
#include "nds/core.h"

class main_menu : public QWidget
{
	Q_OBJECT
	
	public:
	main_menu(QWidget *parent = 0);
	static core_emu* gbe_plus;

	void pause_emu();

	protected:
	void paintEvent(QPaintEvent* event);
	void closeEvent(QCloseEvent* event);
	void keyPressEvent(QKeyEvent* event);
	void keyReleaseEvent(QKeyEvent* event);

	private slots:
	void open_file();
	void screenshot();
	void pause();
	void reset();
	void quit();
	void show_settings();
	void show_display_settings();
	void show_sound_settings();
	void show_control_settings();
	void show_cgfx();
	void show_about();

	private:
	gen_settings* settings;
	gbe_cgfx* cgfx;
	QWidget* about_box;

	int menu_height;
	u32 base_width;
	u32 base_height;

	void boot_game();
};

#endif //MAINMENU_GBE_QT
