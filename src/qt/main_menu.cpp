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
#include "render.h"

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
	connect(quit, SIGNAL(triggered()), this, SLOT(quit()));
	connect(open, SIGNAL(triggered()), this, SLOT(open_file()));
	connect(screenshot, SIGNAL(triggered()), this, SLOT(screenshot()));

	QVBoxLayout *layout = new QVBoxLayout;
	layout->setMenuBar(menu_bar);
	setLayout(layout);

	menu_height = menu_bar->height();

	gbe_plus = NULL;
}

/****** Open game file ******/
void main_menu::open_file()
{
	SDL_PauseAudio(1);

	QString filename = QFileDialog::getOpenFileName(this, tr("Open"), "", tr("GBx files (*.gb *.gbc *.gba)"));
	if(filename.isNull()) { SDL_PauseAudio(0); return; }

	SDL_PauseAudio(0);

	config::rom_file = filename.toStdString();
	config::save_file = config::rom_file + ".sav";

	//Determine Gameboy type based on file name
	//Note, DMG and GBC games are automatically detected in the Gameboy MMU, so only check for GBA types here
	std::size_t dot = config::rom_file.find_last_of(".");
	std::string ext = config::rom_file.substr(dot);

	if(ext == ".gba") { config::gb_type = 3; }
	else { config::gb_type = 0; }

	config::sdl_render = false;
	config::render_external = render_screen;

	if(qt_gui::screen != NULL) { delete qt_gui::screen; }
	qt_gui::screen = NULL;

	//Close the core
	if(gbe_plus != NULL) 
	{
		gbe_plus->shutdown();
		gbe_plus->core_emu::~core_emu();
	}

	boot_game();
}

void main_menu::quit()
{
	//Close the core
	if(gbe_plus != NULL) 
	{
		gbe_plus->shutdown();
		gbe_plus->core_emu::~core_emu();
	}

	//Close SDL
	SDL_Quit();

	//Exit the application
	exit(0);
}

/****** Boots and starts emulation ******/
void main_menu::boot_game()
{
	//Start the appropiate system core - DMG/GBC or GBA
	if(config::gb_type == 3) 
	{ 
		gbe_plus = new AGB_core();
		resize(480, 320+menu_height);
		qt_gui::screen = new QImage(240, 160, QImage::Format_ARGB32);
	}

	else 
	{ 
		gbe_plus = new DMG_core();
		resize(320, 288+menu_height);
		qt_gui::screen = new QImage(160, 144, QImage::Format_ARGB32);
	}

	//Parse .ini options
	parse_ini_file();

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

	//Actually run the core
	gbe_plus->run_core();
}

/****** Updates the main window ******/
void main_menu::paintEvent(QPaintEvent *e)
{
	if(qt_gui::screen == NULL)
	{
		QPainter painter(this);
		painter.setBrush(Qt::black);
		painter.drawRect(0, 0, width(), height());
	}

	else
	{
		QImage final_screen = qt_gui::screen->scaled(width(), height()-menu_height);
		QPainter painter(this);
		painter.drawImage(0, menu_height, final_screen);
	}
}

/****** Closes the main window ******/
void main_menu::closeEvent(QCloseEvent *e)
{
	//Close the core
	if(gbe_plus != NULL) 
	{
		gbe_plus->shutdown();
		gbe_plus->core_emu::~core_emu();
	}

	//Close SDL
	SDL_Quit();

	//Exit the application
	exit(0);
}

/****** Pauses emulation ******/
void main_menu::pause() { }

/****** Takes screenshot ******/
void main_menu::screenshot()
{
	if(gbe_plus != NULL)
	{
		std::stringstream save_stream;
		std::string save_name = "";

		//Prefix SDL Ticks to screenshot name
		save_stream << SDL_GetTicks();
		save_name += save_stream.str();
		save_stream.str(std::string());

		//Append random number to screenshot name
		srand(SDL_GetTicks());
		save_stream << rand() % 1024 << rand() % 1024 << rand() % 1024;
		save_name += save_stream.str() + ".png";

		QString qt_save_name = QString::fromStdString(save_name);
	
		qt_gui::screen->save(qt_save_name, "PNG");
	}
}

