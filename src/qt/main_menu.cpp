// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : main_menu.cpp
// Date : June 18, 2015
// Description : Main menu
//
// Main menu for the main window
// Has options like File, Emulation, Options, About...

#include "main_menu.h"
#include "qt_common.h"
#include "render.h"

#include "common/config.h"
#include "common/cgfx_common.h"
#include "common/util.h"

/****** Main menu constructor ******/
main_menu::main_menu(QWidget *parent) : QWidget(parent)
{
	//Setup actions
	QAction* open = new QAction("Open", this);
	QAction* quit = new QAction ("Quit", this);

	QAction* pause = new QAction("Pause", this);
	QAction* reset = new QAction("Reset", this);
	QAction* fullscreen = new QAction("Fullscreen", this);
	QAction* screenshot = new QAction("Screenshot", this);
	QAction* nplay_start = new QAction("Start Netplay", this);
	QAction* nplay_stop = new QAction("Stop Netplay", this);

	QAction* general = new QAction("General Settings...", this);
	QAction* display = new QAction("Display", this);
	QAction* sound = new QAction("Sound", this);
	QAction* controls = new QAction("Controls", this);
	QAction* netplay = new QAction("Netplay", this);
	QAction* paths = new QAction("Paths", this);

	QAction* custom_gfx = new QAction("Custom Graphics...", this);
	QAction* debugging = new QAction("Debugger", this);

	QAction* about = new QAction("About", this);

	//Set shortcuts for actions
	open->setShortcut(tr("CTRL+O"));
	quit->setShortcut(tr("CTRL+Q"));

	pause->setShortcut(tr("CTRL+P"));
	reset->setShortcut(tr("F8"));
	fullscreen->setShortcut(tr("F12"));
	screenshot->setShortcut(tr("F9"));
	nplay_start->setShortcut(tr("F5"));
	nplay_stop->setShortcut(tr("F6"));

	pause->setCheckable(true);
	pause->setObjectName("pause_action");
	fullscreen->setCheckable(true);
	fullscreen->setObjectName("fullscreen_action");

	menu_bar = new QMenuBar(this);

	//Setup File menu
	QMenu* file;

	file = new QMenu(tr("File"), this);
	file->addAction(open);
	recent_list = file->addMenu(tr("Recent Files"));
	file->addSeparator();
	state_save_list = file->addMenu(tr("Save State"));
	state_load_list = file->addMenu(tr("Load State"));
	file->addSeparator();
	file->addAction(quit);
	menu_bar->addMenu(file);

	//Setup Emulation menu
	QMenu* emulation;

	emulation = new QMenu(tr("Emulation"), this);
	emulation->addAction(pause);
	emulation->addAction(reset);
	emulation->addSeparator();
	emulation->addAction(fullscreen);
	emulation->addAction(screenshot);
	emulation->addSeparator();
	emulation->addAction(nplay_start);
	emulation->addAction(nplay_stop);
	menu_bar->addMenu(emulation);

	//Setup Options menu
	QMenu* options;
	
	options = new QMenu(tr("Options"), this);
	options->addAction(general);
	options->addSeparator();
	options->addAction(display);
	options->addAction(sound);
	options->addAction(controls);
	options->addAction(netplay);
	options->addAction(paths);
	menu_bar->addMenu(options);

	//Advanced menu
	QMenu* advanced;

	advanced = new QMenu(tr("Advanced"), this);
	advanced->addAction(custom_gfx);
	advanced->addAction(debugging);
	menu_bar->addMenu(advanced);

	//Setup Help menu
	QMenu* help;

	help = new QMenu(tr("Help"), this);
	help->addAction(about);
	menu_bar->addMenu(help);

	//Setup signals
	connect(quit, SIGNAL(triggered()), this, SLOT(quit()));
	connect(open, SIGNAL(triggered()), this, SLOT(open_file()));
	connect(pause, SIGNAL(triggered()), this, SLOT(pause()));
	connect(fullscreen, SIGNAL(triggered()), this, SLOT(fullscreen()));
	connect(screenshot, SIGNAL(triggered()), this, SLOT(screenshot()));
	connect(nplay_start, SIGNAL(triggered()), this, SLOT(start_netplay()));
	connect(nplay_stop, SIGNAL(triggered()), this, SLOT(stop_netplay()));
	connect(reset, SIGNAL(triggered()), this, SLOT(reset()));
	connect(general, SIGNAL(triggered()), this, SLOT(show_settings()));
	connect(display, SIGNAL(triggered()), this, SLOT(show_display_settings()));
	connect(sound, SIGNAL(triggered()), this, SLOT(show_sound_settings()));
	connect(controls, SIGNAL(triggered()), this, SLOT(show_control_settings()));
	connect(netplay, SIGNAL(triggered()), this, SLOT(show_netplay_settings()));
	connect(paths, SIGNAL(triggered()), this, SLOT(show_paths_settings()));
	connect(custom_gfx, SIGNAL(triggered()), this, SLOT(show_cgfx()));
	connect(debugging, SIGNAL(triggered()), this, SLOT(show_debugger()));
	connect(about, SIGNAL(triggered()), this, SLOT(show_about()));

	sw_screen = new soft_screen();
	hw_screen = new hard_screen();

	QVBoxLayout* layout = new QVBoxLayout;
	layout->setContentsMargins(0, 0, 0, -1);
	layout->addWidget(sw_screen);
	layout->addWidget(hw_screen);
	layout->setMenuBar(menu_bar);
	setLayout(layout);

	config::scaling_factor = 2;

	hw_screen->hide();
	hw_screen->setEnabled(false);

	//Parse .ini options
	parse_ini_file();

	//Parse cheats file
	if(config::use_cheats) { parse_cheats_file(); }

	//Parse command-line arguments
	//These will override .ini options!
	if(!parse_cli_args()) { exit(0); }

	//Some command-line arguments are invalid for the Qt version
	config::use_debugger = false;

	//Setup Recent Files
	list_mapper = new QSignalMapper(this);

	for(int x = (config::recent_files.size() - 1); x >= 0; x--)
	{
		QString path = QString::fromStdString(config::recent_files[x]);
		QFileInfo file(path);
		path = file.fileName();

		QAction* temp = new QAction(path, this);
		recent_list->addAction(temp);

		connect(temp, SIGNAL(triggered()), list_mapper, SLOT(map()));
		list_mapper->setMapping(temp, x);
	}

	connect(list_mapper, SIGNAL(mapped(int)), this, SLOT(load_recent(int)));

	//Setup Save States
	QSignalMapper* save_mapper = new QSignalMapper(this);

	for(int x = 0; x < 10; x++)
	{
		QAction* temp;

		if(x == 0) 
		{
			temp = new QAction(tr("Quick Save"), this);
			temp->setShortcut(tr("F1"));
		}
		
		else
		{
			std::string slot_id = "Slot " + util::to_str(x);
			QString slot_name = QString::fromStdString(slot_id);
			temp = new QAction(slot_name, this);
		}

		state_save_list->addAction(temp);

		connect(temp, SIGNAL(triggered()), save_mapper, SLOT(map()));
		save_mapper->setMapping(temp, x);
	}

	connect(save_mapper, SIGNAL(mapped(int)), this, SLOT(save_state(int)));

	//Setup Load States
	QSignalMapper* load_mapper = new QSignalMapper(this);

	for(int x = 0; x < 10; x++)
	{
		QAction* temp;

		if(x == 0)
		{
			temp = new QAction(tr("Quick Load"), this);
			temp->setShortcut(tr("F2"));
		}
		
		else
		{
			std::string slot_id = "Slot " + util::to_str(x);
			QString slot_name = QString::fromStdString(slot_id);
			temp = new QAction(slot_name, this);
		}

		state_load_list->addAction(temp);

		connect(temp, SIGNAL(triggered()), load_mapper, SLOT(map()));
		load_mapper->setMapping(temp, x);
	}

	connect(load_mapper, SIGNAL(mapped(int)), this, SLOT(load_state(int)));

	//Set up settings dialog
	settings = new gen_settings();
	settings->set_ini_options();

	//Set up custom graphics dialog
	cgfx = new gbe_cgfx();
	cgfx->hide();
	cgfx->advanced->setChecked(true);

	//Set up DMG-GBC debugger
	main_menu::dmg_debugger = new dmg_debug();
	main_menu::dmg_debugger->hide();

	//Setup About pop-up
	about_box = new QWidget();
	about_box->resize(300, 250);
	about_box->setWindowTitle("About GBE+");

	QDialogButtonBox* about_button = new QDialogButtonBox(QDialogButtonBox::Close);
	connect(about_button->button(QDialogButtonBox::Close), SIGNAL(clicked()), about_box, SLOT(close()));

	QLabel* emu_title = new QLabel("GBE+ 1.0");
	QFont font = emu_title->font();
	font.setPointSize(18);
	font.setBold(true);
	emu_title->setFont(font);

	QImage logo(QString::fromStdString(config::cfg_path + "data/icons/gbe_plus.png"));
	logo = logo.scaled(128, 128);
	QLabel* emu_desc = new QLabel("A GB/GBC/GBA emulator with enhancements");
	QLabel* emu_copyright = new QLabel("Copyright D.S. Baxter 2014-2016");
	QLabel* emu_proj_copyright = new QLabel("Copyright GBE+ Team 2014-2016");
	QLabel* emu_license = new QLabel("This program is licensed under the GNU GPLv2");
	QLabel* emu_site = new QLabel("<a href=\"https://github.com/shonumi/gbe-plus/\">GBE+ on GitHub</a>");
	emu_site->setOpenExternalLinks(true);
	QLabel* emu_logo = new QLabel;
	emu_logo->setPixmap(QPixmap::fromImage(logo));

	QVBoxLayout* about_layout = new QVBoxLayout;
	about_layout->addWidget(emu_title, 0, Qt::AlignCenter | Qt::AlignTop);
	about_layout->addWidget(emu_desc, 0, Qt::AlignCenter | Qt::AlignTop);
	about_layout->addWidget(emu_copyright, 0, Qt::AlignCenter | Qt::AlignTop);
	about_layout->addWidget(emu_proj_copyright, 0, Qt::AlignCenter | Qt::AlignTop);
	about_layout->addWidget(emu_license, 0, Qt::AlignCenter | Qt::AlignTop);
	about_layout->addWidget(emu_site, 0, Qt::AlignCenter | Qt::AlignTop);
	about_layout->addWidget(emu_logo, 0, Qt::AlignCenter | Qt::AlignTop);
	about_layout->addWidget(about_button);
	about_box->setLayout(about_layout);
	about_box->setWindowIcon(QIcon(QString::fromStdString(config::cfg_path + "data/icons/gbe_plus.png")));
	
	about_box->hide();

	display_width = QApplication::desktop()->screenGeometry().width();
	display_height = QApplication::desktop()->screenGeometry().height();

	fullscreen_mode = false;
}

/****** Opens a file from the CLI arguments ******/
void main_menu::open_first_file()
{
	//If command-line arguments are used and they are valid, try opening a ROM right away
	if(!config::cli_args.empty()) { open_file(); }
}

/****** Open game file ******/
void main_menu::open_file()
{
	SDL_PauseAudio(1);

	if(config::cli_args.empty())
	{
		QString filename = QFileDialog::getOpenFileName(this, tr("Open"), "", tr("GBx files (*.gb *.gbc *.gba)"));
		if(filename.isNull()) { SDL_PauseAudio(0); return; }

		config::rom_file = filename.toStdString();
		config::save_file = config::rom_file + ".sav";
	}

	else
	{
		parse_filenames();
		config::cli_args.clear();
	}

	SDL_PauseAudio(0);

	//Close the core
	if(main_menu::gbe_plus != NULL) 
	{
		main_menu::gbe_plus->shutdown();
		main_menu::gbe_plus->core_emu::~core_emu();
	}

	config::sdl_render = false;
	config::render_external_sw = render_screen_sw;
	config::render_external_hw = render_screen_hw;
	config::sample_rate = settings->sample_rate;

	if(qt_gui::screen != NULL) { delete qt_gui::screen; }
	qt_gui::screen = NULL;

	//Search the recent files list and add this path to it
	bool add_recent = true;

	for(int x = 0; x < config::recent_files.size(); x++)
	{
		if(config::recent_files[x] == config::rom_file) { add_recent = false; }
	}

	if(add_recent)
	{
		config::recent_files.push_back(config::rom_file);

		//Delete the earliest element
		if(config::recent_files.size() > 10) { config::recent_files.erase(config::recent_files.begin()); }


		//Update the recent list
		recent_list->clear();

		for(int x = (config::recent_files.size() - 1); x >= 0; x--)
		{
			QString path = QString::fromStdString(config::recent_files[x]);
			QFileInfo file(path);
			path = file.fileName();

			QAction* temp = new QAction(path, this);
			recent_list->addAction(temp);

			connect(temp, SIGNAL(triggered()), list_mapper, SLOT(map()));
			list_mapper->setMapping(temp, x);
		}

		connect(list_mapper, SIGNAL(mapped(int)), this, SLOT(load_recent(int)));
	}

	boot_game();
}

/****** Exits the emulator ******/
void main_menu::quit()
{
	//Close the core
	if(main_menu::gbe_plus != NULL) 
	{
		main_menu::gbe_plus->shutdown();
		main_menu::gbe_plus->core_emu::~core_emu();
	}

	//Save .ini options
	config::gb_type = settings->sys_type->currentIndex();
	config::use_cheats = (settings->cheats->isChecked()) ? true : false;
	config::mute = (settings->sound_on->isChecked()) ? false : true;
	config::volume = settings->volume->value();
	config::use_haptics = (settings->rumble_on->isChecked()) ? true : false;

	switch(settings->freq->currentIndex())
	{
		case 0: config::sample_rate = 48000.0; break;
		case 1: config::sample_rate = 44100.0; break;
		case 2: config::sample_rate = 22050.0; break;
		case 3: config::sample_rate = 11025.0; break;
	}

	save_ini_file();

	//Close SDL
	SDL_Quit();

	//Exit the application
	exit(0);
}

/****** Boots and starts emulation ******/
void main_menu::boot_game()
{
	config::sample_rate = settings->sample_rate;
	config::pause_emu = false;

	//Check OpenGL status
	if(settings->ogl->isChecked())
	{
		config::use_opengl = true;
		sw_screen->setEnabled(false);
		sw_screen->hide();
		hw_screen->setEnabled(true);
		hw_screen->show();
	}

	else
	{
		config::use_opengl = false;
		sw_screen->setEnabled(true);
		sw_screen->show();
		hw_screen->setEnabled(false);
		hw_screen->hide();
	}

	//Check cheats status
	if(settings->cheats->isChecked())
	{
		config::use_cheats = true;
		parse_cheats_file();
	}

	else { config::use_cheats = false; }

	//Check multicart status
	if(settings->multicart->isChecked()) { config::use_multicart = true; }
	else { config::use_multicart = false; }

	//Check rumble status
	if(settings->rumble_on->isChecked()) { config::use_haptics = true; }
	else { config::use_haptics = false; }

	findChild<QAction*>("pause_action")->setChecked(false);

	menu_height = menu_bar->height();

	//Determine Gameboy type based on file name
	//Note, DMG and GBC games are automatically detected in the Gameboy MMU, so only check for GBA types here
	std::size_t dot = config::rom_file.find_last_of(".");
	std::string ext = config::rom_file.substr(dot);

	config::gb_type = settings->sys_type->currentIndex();
	
	if(ext == ".gba") { config::gb_type = 3; }
	else if((ext != ".gba") && (config::gb_type == 3)) { config::gb_type = 2; config::gba_enhance = true; }
	else { config::gba_enhance = false; }

	//Determine CGFX scaling factor
	cgfx::scaling_factor = (settings->cgfx_scale->currentIndex() + 1);
	if(!cgfx::load_cgfx) { cgfx::scaling_factor = 1; }

	//Start the appropiate system core - DMG/GBC or GBA
	if(config::gb_type == 3) 
	{
		base_width = 240;
		base_height = 160;

		main_menu::gbe_plus = new AGB_core();
		resize((base_width * config::scaling_factor), (base_height * config::scaling_factor) + menu_height);
		qt_gui::screen = new QImage(240, 160, QImage::Format_ARGB32);

		//Resize drawing screens
		if(config::use_opengl) { hw_screen->resize((base_width * config::scaling_factor), (base_height * config::scaling_factor)); }
		else { sw_screen->resize((base_width * config::scaling_factor), (base_height * config::scaling_factor)); }
	}

	else 
	{
		base_width = (160 * cgfx::scaling_factor);
		base_height = (144 * cgfx::scaling_factor);

		main_menu::gbe_plus = new DMG_core();
		resize((base_width * config::scaling_factor), (base_height * config::scaling_factor) + menu_height);

		//Resize drawing screens
		if(config::use_opengl) { hw_screen->resize((base_width * config::scaling_factor), (base_height * config::scaling_factor)); }
		else { sw_screen->resize((base_width * config::scaling_factor), (base_height * config::scaling_factor)); }

		if(qt_gui::screen != NULL) { delete qt_gui::screen; }
		qt_gui::screen = new QImage(base_width, base_height, QImage::Format_ARGB32);
	}

	//Read specified ROM file
	if(!main_menu::gbe_plus->read_file(config::rom_file))
	{
		main_menu::gbe_plus->shutdown();
		main_menu::gbe_plus->core_emu::~core_emu();
		return;
	}
	
	//Read BIOS file optionally
	if(config::use_bios) 
	{
		switch(config::gb_type)
		{
			case 0x1 : config::bios_file = config::dmg_bios_path; reset_dmg_colors(); break;
			case 0x2 : config::bios_file = config::gbc_bios_path; reset_dmg_colors(); break;
			case 0x3 : config::bios_file = config::agb_bios_path; break;
		}

		if(!main_menu::gbe_plus->read_bios(config::bios_file)) { return; } 
	}

	//Reset GUI debugger
	dmg_debugger->debug_reset = true;

	//If the fullscreen command-line argument was passed, be sure to boot into fullscreen mode
	if(config::flags & 0x80000000)
	{
		findChild<QAction*>("fullscreen_action")->setChecked(true);
		config::flags &= ~0x80000000;
		fullscreen();
	}

	//Engage the core
	main_menu::gbe_plus->start();
	main_menu::gbe_plus->db_unit.debug_mode = config::use_debugger;

	if(main_menu::gbe_plus->db_unit.debug_mode) { SDL_CloseAudio(); }

	//Actually run the core
	main_menu::gbe_plus->run_core();
}

/****** Updates the main window ******/
void main_menu::paintEvent(QPaintEvent* event)
{
	if(qt_gui::screen == NULL)
	{
		QPainter painter(this);
		painter.setBrush(Qt::black);
		painter.drawRect(0, 0, width(), height());
	}

	else
	{
		//Check for resize
		if(settings->resize_screen)
		{
			resize((base_width * config::scaling_factor), (base_height * config::scaling_factor) + menu_height);
			settings->resize_screen = false;

			//Resize drawing screens
			if(config::use_opengl) { hw_screen->resize((base_width * config::scaling_factor), (base_height * config::scaling_factor)); }
			else { sw_screen->resize((base_width * config::scaling_factor), (base_height * config::scaling_factor)); }
		}

		else if(config::request_resize)
		{
			if((config::resize_mode > 0) && (config::sys_width != 240)) { return; }

			base_width = config::sys_width;
			base_height = config::sys_height;

			if(qt_gui::screen != NULL) { delete qt_gui::screen; }
			qt_gui::screen = new QImage(config::sys_width, config::sys_height, QImage::Format_ARGB32);

			resize((base_width * config::scaling_factor), (base_height * config::scaling_factor) + menu_height);
			config::request_resize = false;

			//Resize drawing screens
			if(config::use_opengl) { hw_screen->resize((base_width * config::scaling_factor), (base_height * config::scaling_factor)); }
			else { sw_screen->resize((base_width * config::scaling_factor), (base_height * config::scaling_factor)); }
		}
	}
}

/****** Closes the main window ******/
void main_menu::closeEvent(QCloseEvent* event)
{
	//Close the core
	if(main_menu::gbe_plus != NULL) 
	{
		main_menu::gbe_plus->shutdown();
		main_menu::gbe_plus->core_emu::~core_emu();
	}

	//Save .ini options
	config::gb_type = settings->sys_type->currentIndex();
	config::use_cheats = (settings->cheats->isChecked()) ? true : false;
	config::mute = (settings->sound_on->isChecked()) ? false : true;
	config::volume = settings->volume->value();
	config::use_opengl = (settings->ogl->isChecked()) ? true : false;
	config::use_haptics = (settings->rumble_on->isChecked()) ? true : false;
	
	switch(settings->freq->currentIndex())
	{
		case 0: config::sample_rate = 48000.0; break;
		case 1: config::sample_rate = 44100.0; break;
		case 2: config::sample_rate = 22050.0; break;
		case 3: config::sample_rate = 11025.0; break;
	}

	save_ini_file();

	//Close SDL
	SDL_Quit();

	//Exit the application
	exit(0);
}

/****** Handle keypress input ******/
void main_menu::keyPressEvent(QKeyEvent* event)
{
	int sdl_key = qtkey_to_sdlkey(event->key());

	//Force input processing in the core
	if(main_menu::gbe_plus != NULL)
	{
		gbe_plus->feed_key_input(sdl_key, true);

		//Handle fullscreen hotkeys if necessary
		if(findChild<QAction*>("fullscreen_action")->isChecked())
		{
			switch(sdl_key)
			{
				//Quick Save State
				case SDLK_F1:
					save_state(0);
					break;

				//Netplay start
				case SDLK_F5:
					start_netplay();
					break;

				//Netplay stop
				case SDLK_F6:
					stop_netplay();
					break;

				//Reset
				case SDLK_F8:
					reset();
					break;
				
				//Screenshot
				case SDLK_F9:
					screenshot();
					break;

				//Quick Load State
				case SDLK_F2:
					load_state(0);
					break;

				//Fullscreen
				case SDLK_F12:
					findChild<QAction*>("fullscreen_action")->setChecked(false);
					fullscreen();
					break;
			}
		}

		//Mute audio
		if(sdl_key == config::hotkey_mute)
		{
			if(settings->sound_on->isChecked()) { settings->sound_on->setChecked(false); }
			else { settings->sound_on->setChecked(true); }

			settings->update_volume();
		}
	}
}

/****** Handle key release input ******/
void main_menu::keyReleaseEvent(QKeyEvent* event)
{
	int sdl_key = qtkey_to_sdlkey(event->key());
	
	//Force input processing in the core
	if(main_menu::gbe_plus != NULL)
	{


		gbe_plus->feed_key_input(sdl_key, false);
	}
}

/****** Qt SLOT to pause the emulator ******/
void main_menu::pause()
{
	if(main_menu::gbe_plus != NULL)
	{
		//Unpause
		if(config::pause_emu) 
		{
			if(cgfx->pause) { return; }

			config::pause_emu = false; 
		}

		//Pause
		else
		{
			config::pause_emu = true;
			pause_emu();
		}
	}
}

/****** Pauses the emulator ******/
void main_menu::pause_emu()
{
	SDL_PauseAudio(1);

	while(config::pause_emu) 
	{
		SDL_Delay(16);
		QApplication::processEvents();
	}

	SDL_PauseAudio(0);

	if(dmg_debugger->pause) { return; }

	//If CGFX is open, continue pause
	if(cgfx->pause) { pause(); }

	//Continue pause if GUI option is still selected - Check this when closing CGFX or debugger
	if(findChild<QAction*>("pause_action")->isChecked()) { pause(); }
}

/****** Resets emulation ******/
void main_menu::reset()
{
	if(main_menu::gbe_plus != NULL) 
	{
		main_menu::gbe_plus->shutdown();
		main_menu::gbe_plus->core_emu::~core_emu();
		boot_game();
	}
}	

/****** Switches to fullscreen mode ******/
void main_menu::fullscreen()
{
	if(main_menu::gbe_plus != NULL)
	{
		//Set fullscreen
		if(findChild<QAction*>("fullscreen_action")->isChecked())
		{
			fullscreen_mode = true;
			setWindowState(Qt::WindowFullScreen);
			menu_bar->hide();
			showFullScreen();
		}

		else
		{
			fullscreen_mode = false;
			setWindowState(Qt::WindowNoState);
			menu_bar->show();
			showNormal();
		}
	}

	else { findChild<QAction*>("fullscreen_action")->setChecked(false); }
}

/****** Takes screenshot ******/
void main_menu::screenshot()
{
	if(main_menu::gbe_plus != NULL)
	{
		std::stringstream save_stream;
		std::string save_name = config::ss_path;

		//Prefix SDL Ticks to screenshot name
		save_stream << SDL_GetTicks();
		save_name += save_stream.str();
		save_stream.str(std::string());

		//Append random number to screenshot name
		srand(SDL_GetTicks());
		save_stream << rand() % 1024 << rand() % 1024 << rand() % 1024;
		save_name += save_stream.str() + ".png";

		QString qt_save_name = QString::fromStdString(save_name);

		//Save OpenGL screen
		if(config::use_opengl) { hw_screen->grabFrameBuffer().save(qt_save_name, "PNG"); }

		//Save software screen
		else { qt_gui::screen->save(qt_save_name, "PNG"); }
	}
}

/****** Shows the General settings dialog ******/
void main_menu::show_settings()
{
	settings->show();
	settings->tabs->setCurrentIndex(0);
	settings->advanced_button->setVisible(false);
}

/****** Shows the Display settings dialog ******/
void main_menu::show_display_settings()
{
	settings->show();
	settings->tabs->setCurrentIndex(1);
	settings->advanced_button->setVisible(false);
}

/****** Shows the Sound settings dialog ******/
void main_menu::show_sound_settings()
{
	settings->show();
	settings->tabs->setCurrentIndex(2);
	settings->advanced_button->setVisible(false);
}

/****** Shows the Control settings dialog ******/
void main_menu::show_control_settings()
{
	settings->show();
	settings->tabs->setCurrentIndex(3);
	settings->advanced_button->setVisible(true);
}

/****** Shows the Netplay settings dialog ******/
void main_menu::show_netplay_settings()
{
	settings->show();
	settings->tabs->setCurrentIndex(4);
	settings->advanced_button->setVisible(false);
}

/****** Shows the Paths settings dialog ******/
void main_menu::show_paths_settings()
{
	settings->show();
	settings->tabs->setCurrentIndex(5);
	settings->advanced_button->setVisible(false);
}

/****** Shows the Custom Graphics dialog ******/
void main_menu::show_cgfx() 
{
	//Draw GBA layers
	if(config::gb_type == 3)
	{
		//Do nothing for now
		return;
	}

	findChild<QAction*>("pause_action")->setEnabled(false);

	//Wait until LY equals the selected line to stop on, or LCD is turned off
	bool spin_emu = (main_menu::gbe_plus == NULL) ? false : true;
	u8 target_ly = 0;
	u8 on_status = 0;

	while(spin_emu)
	{
		on_status = main_menu::gbe_plus->ex_read_u8(REG_LCDC);
		target_ly = main_menu::gbe_plus->ex_read_u8(REG_LY);

		if((on_status & 0x80) == 0) { spin_emu = false; }
		else if(target_ly == cgfx->render_stop_line->value()) { spin_emu = false; }
		else { main_menu::gbe_plus->step(); }
	}

	cgfx->update_obj_window(8, 40);
	cgfx->update_bg_window(8, 384);

	//Draw DMG layers
	if(config::gb_type < 2)
	{
		switch(cgfx->layer_select->currentIndex())
		{
			case 0: cgfx->draw_dmg_bg(); break;
			case 1: cgfx->draw_dmg_win(); break;
			case 2: cgfx->draw_dmg_obj(); break;
		}
	}

	//Draw GBC layers
	else
	{
		switch(cgfx->layer_select->currentIndex())
		{
			case 0: cgfx->draw_gbc_bg(); break;
			case 1: cgfx->draw_gbc_win(); break;
			case 2: cgfx->draw_gbc_obj(); break;
		}
	}

	cgfx->show();
	cgfx->parse_manifest_items();
	cgfx->pause = true;
	
	if(!dmg_debugger->pause) { pause(); }
}

/****** Shows the debugger ******/
void main_menu::show_debugger()
{
	if(main_menu::gbe_plus != NULL)
	{
		//Show DMG-GBC debugger
		if(config::gb_type <= 2) 
		{
			findChild<QAction*>("pause_action")->setEnabled(false);

			SDL_PauseAudio(1);
			main_menu::dmg_debugger->old_pause = config::pause_emu;
			main_menu::dmg_debugger->pause = true;
			config::pause_emu = false;

			config::debug_external = dmg_debug_step;
			main_menu::dmg_debugger->auto_refresh();
			main_menu::dmg_debugger->show();
			main_menu::gbe_plus->db_unit.debug_mode = true;
		}
	}
}

/****** Shows the About box ******/
void main_menu::show_about() 
{ 
	if(about_box->isHidden()) { about_box->show(); }
	else if ((!about_box->isMinimized()) && (!about_box->isHidden())){ about_box->hide(); }
}

/****** Loads recent file from list ******/
void main_menu::load_recent(int file_id)
{
	//Close the core
	if(main_menu::gbe_plus != NULL) 
	{
		main_menu::gbe_plus->shutdown();
		main_menu::gbe_plus->core_emu::~core_emu();
	}

	config::rom_file = config::recent_files[file_id];
	config::save_file = config::rom_file + ".sav";

	config::sdl_render = false;
	config::render_external_sw = render_screen_sw;
	config::render_external_hw = render_screen_hw;
	config::sample_rate = settings->sample_rate;

	if(qt_gui::screen != NULL) { delete qt_gui::screen; }
	qt_gui::screen = NULL;

	boot_game();
}

/****** Saves a save state ******/
void main_menu::save_state(int slot)
{
	if(main_menu::gbe_plus != NULL)  { main_menu::gbe_plus->save_state(slot); }
}

/****** Loads a save state ******/
void main_menu::load_state(int slot)
{
	if(main_menu::gbe_plus != NULL)
	{
		main_menu::gbe_plus->load_state(slot);

		//Apply current volume settings
		settings->update_volume();
	}
}

/****** Starts the core's netplay features ******/
void main_menu::start_netplay()
{
	if(main_menu::gbe_plus != NULL)
	{
		main_menu::gbe_plus->start_netplay();
	}
}

/****** Stops the core's netplay features ******/
void main_menu::stop_netplay()
{
	if(main_menu::gbe_plus != NULL)
	{
		main_menu::gbe_plus->stop_netplay();
	}
}

/****** Static definitions ******/
core_emu* main_menu::gbe_plus = NULL;
dmg_debug* main_menu::dmg_debugger = NULL;
