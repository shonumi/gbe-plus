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
#include "debug_dmg.h"
#include "screens.h"

#include "gba/core.h"
#include "dmg/core.h"
#include "nds/core.h"

class main_menu : public QWidget
{
	Q_OBJECT
	
	public:
	main_menu(QWidget *parent = 0);

	static core_emu* gbe_plus;
	static dmg_debug* dmg_debugger;

	gbe_cgfx* cgfx;
	gen_settings* settings;

	QMenuBar* menu_bar;

	soft_screen* sw_screen;
	hard_screen* hw_screen;
	
	u32 screen_height;
	u32 screen_width;
	u32 display_height;
	u32 display_width;
	int menu_height;

	bool fullscreen_mode;

	void pause_emu();
	void open_first_file();

	protected:
	void paintEvent(QPaintEvent* event);
	void closeEvent(QCloseEvent* event);
	void keyPressEvent(QKeyEvent* event);
	void keyReleaseEvent(QKeyEvent* event);
	bool eventFilter(QObject* target, QEvent* event);

	private slots:
	void open_file();
	void fullscreen();
	void screenshot();
	void pause();
	void reset();
	void quit();
	void show_settings();
	void show_display_settings();
	void show_sound_settings();
	void show_control_settings();
	void show_netplay_settings();
	void show_paths_settings();
	void show_cgfx();
	void show_debugger();
	void show_about();
	void load_recent(int file_id);
	void save_state(int slot);
	void load_state(int slot);
	void start_netplay();
	void stop_netplay();

	private:
	QWidget* about_box;

	QMenu* recent_list;
	QMenu* state_save_list;
	QMenu* state_load_list;

	QSignalMapper* list_mapper;

	QMessageBox* warning_box;

	u32 base_width;
	u32 base_height;

	void boot_game();
};

#endif //MAINMENU_GBE_QT
