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

#include "common/config.h"

/****** Main menu constructor ******/
main_menu::main_menu(QWidget *parent) : QWidget(parent)
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

	QMenuBar* menu_bar;
	menu_bar = new QMenuBar(this);

	//Setup File menu
	QMenu *file;

	file = new QMenu(tr("&File"), this);
	file->addAction(open);
	file->addSeparator();
	file->addAction(quit);
	menu_bar->addMenu(file);

	//Setup Emulation menu
	QMenu *emulation;

	emulation = new QMenu(tr("&Emulation"), this);
	emulation->addAction(pause);
	emulation->addAction(reset);
	emulation->addSeparator();
	emulation->addAction(fullscreen);
	emulation->addAction(screenshot);
	menu_bar->addMenu(emulation);

	//Setup Options menu
	QMenu *options;
	
	options = new QMenu(tr("&Options"), this);
	options->addAction(general);
	options->addSeparator();
	options->addAction(display);
	options->addAction(sound);
	options->addAction(controls);
	menu_bar->addMenu(options);

	//Setup Help menu
	QMenu *help;

	help = new QMenu(tr("&Help"), this);
	help->addAction(about);
	menu_bar->addMenu(help);

	//Setup signals
	connect(quit, SIGNAL(triggered()), qApp, SLOT(quit()));
	connect(open, SIGNAL(triggered()), this, SLOT(open_file()));

	QVBoxLayout *layout = new QVBoxLayout;
	layout->setMenuBar(menu_bar);
	setLayout(layout);

	gbe_plus = NULL;
}

/****** Open game file ******/
void main_menu::open_file()
{
	QString filename = QFileDialog::getOpenFileName(this, tr("Open"), "", tr("GBx files (*.gb *.gbc *.gba)"));
	if(filename.isNull()) { return; }

	config::rom_file = filename.toStdString();
	config::save_file = config::rom_file + ".sav";

	//Determine Gameboy type based on file name
	//Note, DMG and GBC games are automatically detected in the Gameboy MMU, so only check for GBA types here
	std::size_t dot = config::rom_file.find_last_of(".");
	std::string ext = config::rom_file.substr(dot);

	if(ext == ".gba") { config::gb_type = 3; }

	boot_game();
}

/****** Boots and starts emulation ******/
void main_menu::boot_game()
{
	//Start the appropiate system core - DMG/GBC or GBA
	if(config::gb_type == 3) { gbe_plus = new AGB_core(); }
	else { gbe_plus = new DMG_core(); }

	//Parse .ini options
	//if(!parse_ini_file()) { return 0; }

	//Parse command-line arguments
	//These will override .ini options!
	//if(!parse_cli_args()) { return 0; }

	//Read specified ROM file
	if(!gbe_plus->read_file(config::rom_file)) { return; }
	
	//Read BIOS file optionally
	if(config::use_bios) 
	{
		//If no bios file was passed from the command-line arguments, defer to .ini options
		if(config::bios_file == "")
		{
			switch(config::gb_type)
			{
				case 0x1 : config::bios_file = config::dmg_bios_path; break;
				case 0x2 : config::bios_file = config::gbc_bios_path; break;
				case 0x3 : config::bios_file = config::agb_bios_path; break;
			}
		}

		if(!gbe_plus->read_bios(config::bios_file)) { return; } 
	}

	//Engage the core
	gbe_plus->start();
	gbe_plus->db_unit.debug_mode = config::use_debugger;

	if(gbe_plus->db_unit.debug_mode) { SDL_CloseAudio(); }

	//Disbale mouse cursor in SDL, it's annoying
	SDL_ShowCursor(SDL_DISABLE);

	//Set program window caption
	SDL_WM_SetCaption("GBE+", NULL);

	//Actually run the core
	gbe_plus->run_core();

	gbe_plus->core_emu::~core_emu();

	config::gb_type = 0;
}

void main_menu::paintEvent(QPaintEvent *e)
{
	Q_UNUSED(e);

	QPainter painter(this);
	painter.setBrush(Qt::black);
	painter.drawRect(0, 0, width(), height());
}