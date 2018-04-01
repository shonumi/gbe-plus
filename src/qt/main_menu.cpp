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
	QAction* open = new QAction("Open...", this);
	QAction* select_card = new QAction("Select Card File", this);
	QAction* select_cam_file = new QAction("Select GB Camera Photo", this);
	QAction* quit = new QAction ("Quit", this);

	QAction* pause = new QAction("Pause", this);
	QAction* reset = new QAction("Reset", this);
	QAction* fullscreen = new QAction("Fullscreen", this);
	QAction* screenshot = new QAction("Screenshot", this);
	QAction* nplay_start = new QAction("Start Netplay", this);
	QAction* nplay_stop = new QAction("Stop Netplay", this);
	QAction* special_comm = new QAction("Start IR/SIO Device", this);

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
	special_comm->setShortcut(tr("F3"));
	debugging->setShortcut(tr("F7"));

	pause->setCheckable(true);
	pause->setObjectName("pause_action");
	fullscreen->setCheckable(true);
	fullscreen->setObjectName("fullscreen_action");
	custom_gfx->setObjectName("custom_gfx_action");
	debugging->setObjectName("debugging_action");

	custom_gfx->setEnabled(false);
	debugging->setEnabled(false);

	menu_bar = new QMenuBar(this);

	//Setup File menu
	QMenu* file;

	file = new QMenu(tr("File"), this);
	file->addAction(open);
	file->addSeparator();
	recent_list = file->addMenu(tr("Recent Files"));
	file->addAction(select_card);
	file->addAction(select_cam_file);
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
	emulation->addSeparator();
	emulation->addAction(special_comm);
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
	connect(select_card, SIGNAL(triggered()), this, SLOT(select_card_file()));
	connect(select_cam_file, SIGNAL(triggered()), this, SLOT(select_cam_file()));
	connect(pause, SIGNAL(triggered()), this, SLOT(pause()));
	connect(fullscreen, SIGNAL(triggered()), this, SLOT(fullscreen()));
	connect(screenshot, SIGNAL(triggered()), this, SLOT(screenshot()));
	connect(nplay_start, SIGNAL(triggered()), this, SLOT(start_netplay()));
	connect(nplay_stop, SIGNAL(triggered()), this, SLOT(stop_netplay()));
	connect(special_comm, SIGNAL(triggered()), this, SLOT(start_special_comm()));
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

	//Setup mouse tracking on the screens, for NDS touch support
	sw_screen->setMouseTracking(true);
	sw_screen->installEventFilter(this);

	hw_screen->setMouseTracking(true);
	hw_screen->installEventFilter(this);

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
	if(config::use_cheats) { parse_cheats_file(false); }

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

	QLabel* emu_title = new QLabel("GBE+ 1.2");
	QFont font = emu_title->font();
	font.setPointSize(18);
	font.setBold(true);
	emu_title->setFont(font);

	QImage logo(QString::fromStdString(config::cfg_path + "data/icons/gbe_plus.png"));
	logo = logo.scaled(128, 128);
	QLabel* emu_desc = new QLabel("A GB/GBC/GBA/NDS emulator with enhancements");
	QLabel* emu_copyright = new QLabel("Copyright D.S. Baxter 2014-2018");
	QLabel* emu_proj_copyright = new QLabel("Copyright GBE+ Team 2014-2018");
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

	//Setup warning message box
	warning_box = new QMessageBox;
	QPushButton* warning_box_ok = warning_box->addButton("OK", QMessageBox::AcceptRole);
	warning_box->setIcon(QMessageBox::Warning);
	warning_box->hide();

	display_width = QApplication::desktop()->screenGeometry().width();
	display_height = QApplication::desktop()->screenGeometry().height();

	fullscreen_mode = false;
	is_sgb_core = false;
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
		QString filename = QFileDialog::getOpenFileName(this, tr("Open"), "", tr("GBx/NDS files (*.gb *.gbc *.gba *.nds)"));
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

/****** Opens card file ******/
void main_menu::select_card_file()
{
	SDL_PauseAudio(1);

	QString filename = QFileDialog::getOpenFileName(this, tr("Open"), "", tr("Binary card data (*.bin)"));
	if(filename.isNull()) { SDL_PauseAudio(0); return; }

	config::external_card_file = filename.toStdString();

	//Tell DMG-GBC core to update data
	if((config::gb_type >= 0) && (config::gb_type <= 2) && (main_menu::gbe_plus != NULL))
	{
		if(main_menu::gbe_plus->get_core_data(1) == 0)
		{
			std::string mesg_text = "The card file: '" + config::external_card_file + "' could not be loaded"; 
			warning_box->setText(QString::fromStdString(mesg_text));
			warning_box->show();
		}
	}

	SDL_PauseAudio(0);
}

/****** Opens GB Camera image file ******/
void main_menu::select_cam_file()
{
	SDL_PauseAudio(1);

	QString filename = QFileDialog::getOpenFileName(this, tr("Open"), "", tr("Bitmap File(*.bmp)"));
	if(filename.isNull()) { SDL_PauseAudio(0); return; }

	config::external_camera_file = filename.toStdString();

	SDL_PauseAudio(0);
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

	config::dmg_bios_path = settings->dmg_bios->text().toStdString();
	config::gbc_bios_path = settings->gbc_bios->text().toStdString();
	config::agb_bios_path = settings->gba_bios->text().toStdString();
	cgfx::manifest_file = settings->manifest->text().toStdString();
	config::ss_path = settings->screenshot->text().toStdString();
	cgfx::dump_bg_path = settings->dump_bg->text().toStdString();
	cgfx::dump_obj_path = settings->dump_obj->text().toStdString();
	config::save_path = settings->game_saves->text().toStdString();
	config::cheats_path = settings->cheats_path->text().toStdString();

	switch(settings->freq->currentIndex())
	{
		case 0: config::sample_rate = 48000.0; break;
		case 1: config::sample_rate = 44100.0; break;
		case 2: config::sample_rate = 22050.0; break;
		case 3: config::sample_rate = 11025.0; break;
	}

	save_ini_file();
	save_cheats_file();

	//Close SDL
	SDL_Quit();

	//Exit the application
	exit(0);
}

/****** Boots and starts emulation ******/
void main_menu::boot_game()
{
	//Check to see if the ROM file actually exists
	QFile test_file(QString::fromStdString(config::rom_file));
	
	if(!test_file.exists())
	{
		std::string mesg_text = "The specified file: '" + config::rom_file + "' could not be loaded"; 
		warning_box->setText(QString::fromStdString(mesg_text));
		warning_box->show();
		return;
	}

	std::string test_bios_path = "";
	u8 system_type = get_system_type_from_file(config::rom_file);

	switch(system_type)
	{
		case 0x1: test_bios_path = config::dmg_bios_path; break;
		case 0x2: test_bios_path = config::gbc_bios_path; break;
		case 0x3: test_bios_path = config::agb_bios_path; break;
		case 0x4: test_bios_path = config::nds7_bios_path; break;
		case 0x5: config::use_bios = false;
	}

	test_file.setFileName(QString::fromStdString(test_bios_path));

	if(!test_file.exists() && config::use_bios)
	{
		std::string mesg_text;

		if(!test_bios_path.empty()) { mesg_text = "The BIOS file: '" + test_bios_path + "' could not be loaded"; }
		
		else
		{
			if(system_type == 4)
			{
				mesg_text = "ARM7 BIOS file not specified.\nPlease check your Paths settings or disable the 'Use BIOS/Boot ROM' option";
			} 
				
			else 
			{
				mesg_text = "No BIOS file specified for this system.\nPlease check your Paths settings or disable the 'Use BIOS/Boot ROM' option";
			}
		} 

		warning_box->setText(QString::fromStdString(mesg_text));
		warning_box->show();
		return;
	}

	//Perform a second test for NDS9 BIOS
	if(system_type == 4)
	{
		test_file.setFileName(QString::fromStdString(config::nds9_bios_path));

		if(!test_file.exists() && config::use_bios)
		{
			std::string mesg_text;

			if(!test_bios_path.empty()) { mesg_text = "The BIOS file: '" + test_bios_path + "' could not be loaded"; }
			else { mesg_text = "ARM9 BIOS file not specified.\nPlease check your Paths settings or disable the 'Use BIOS/Boot ROM' option"; } 

			warning_box->setText(QString::fromStdString(mesg_text));
			warning_box->show();
			return;
		}
	}

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
		parse_cheats_file(false);
	}

	else { config::use_cheats = false; }

	//Check special cart status
	switch(settings->special_cart->currentIndex())
	{
		case 0x0: config::cart_type = NORMAL_CART; break;
		case 0x1: config::cart_type = DMG_MBC1M; break;
		case 0x2: config::cart_type = DMG_MMM01; break;
		case 0x3: config::cart_type = AGB_RTC; break;
		case 0x4: config::cart_type = AGB_SOLAR_SENSOR; break;
		case 0x5: config::cart_type = AGB_RUMBLE; break;
		case 0x6: config::cart_type = AGB_GYRO_SENSOR; break;
		case 0x7: config::cart_type = AGB_TILT_SENSOR; break;
	}

	//Check rumble status
	if(settings->rumble_on->isChecked()) { config::use_haptics = true; }
	else { config::use_haptics = false; }

	findChild<QAction*>("pause_action")->setChecked(false);

	menu_height = menu_bar->height();

	//Determine Gameboy type based on file name
	//Note, DMG and GBC games are automatically detected in the Gameboy MMU, so only check for GBA and NDS types here
	std::size_t dot = config::rom_file.find_last_of(".");
	std::string ext = config::rom_file.substr(dot);

	config::gb_type = settings->sys_type->currentIndex();
	
	if(ext == ".gba") { config::gb_type = 3; }
	else if(ext == ".nds") { config::gb_type = 4; }
	else if((ext != ".gba") && (config::gb_type == 3)) { config::gb_type = 2; config::gba_enhance = true; }
	else { config::gba_enhance = false; }

	if((config::gb_type == 5) || (config::gb_type == 6)) { config::gb_type = get_system_type_from_file(config::rom_file); }

	//Determine CGFX scaling factor
	cgfx::scaling_factor = (settings->cgfx_scale->currentIndex() + 1);
	if(!cgfx::load_cgfx) { cgfx::scaling_factor = 1; }

	//Start the appropiate system core - DMG, GBC, GBA, or NDS
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

		//Disable CGFX menu
		findChild<QAction*>("custom_gfx_action")->setEnabled(false);

		//Disable debugging menu
		findChild<QAction*>("debugging_action")->setEnabled(false);
	}

	else if(config::gb_type == 4)
	{
		base_width = 256;
		base_height = 384;

		main_menu::gbe_plus = new NTR_core();
		resize((base_width * config::scaling_factor), (base_height * config::scaling_factor) + menu_height);
		qt_gui::screen = new QImage(256, 384, QImage::Format_ARGB32);

		//Resize drawing screens
		if(config::use_opengl) { hw_screen->resize((base_width * config::scaling_factor), (base_height * config::scaling_factor)); }
		else { sw_screen->resize((base_width * config::scaling_factor), (base_height * config::scaling_factor)); }

		//Disable CGFX menu
		findChild<QAction*>("custom_gfx_action")->setEnabled(false);

		//Disable debugging menu
		findChild<QAction*>("debugging_action")->setEnabled(false);
	}	

	else 
	{
		base_width = (160 * cgfx::scaling_factor);
		base_height = (144 * cgfx::scaling_factor);

		if((config::gb_type == 5) || (config::gb_type == 6))
		{
			main_menu::gbe_plus = new SGB_core();
			is_sgb_core = true;
		}

		else
		{
			main_menu::gbe_plus = new DMG_core();
			is_sgb_core = false;
		}

		resize((base_width * config::scaling_factor), (base_height * config::scaling_factor) + menu_height);

		//Resize drawing screens
		if(config::use_opengl) { hw_screen->resize((base_width * config::scaling_factor), (base_height * config::scaling_factor)); }
		else { sw_screen->resize((base_width * config::scaling_factor), (base_height * config::scaling_factor)); }

		if(qt_gui::screen != NULL) { delete qt_gui::screen; }
		qt_gui::screen = new QImage(base_width, base_height, QImage::Format_ARGB32);

		//Enable CGFX menu
		findChild<QAction*>("custom_gfx_action")->setEnabled(true);

		//Enable debugging menu
		findChild<QAction*>("debugging_action")->setEnabled(true);
	}

	//Read specified ROM file
	main_menu::gbe_plus->read_file(config::rom_file);
	
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
	if(config::flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
	{
		findChild<QAction*>("fullscreen_action")->setChecked(true);
		config::flags &= ~SDL_WINDOW_FULLSCREEN_DESKTOP;
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
			if((config::resize_mode > 0) && (config::sys_width != 240) && (config::sys_width != 256) && (config::sys_width != 512)) { return; }

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
	
	config::dmg_bios_path = settings->dmg_bios->text().toStdString();
	config::gbc_bios_path = settings->gbc_bios->text().toStdString();
	config::agb_bios_path = settings->gba_bios->text().toStdString();
	cgfx::manifest_file = settings->manifest->text().toStdString();
	config::ss_path = settings->screenshot->text().toStdString();
	cgfx::dump_bg_path = settings->dump_bg->text().toStdString();
	cgfx::dump_obj_path = settings->dump_obj->text().toStdString();
	config::save_path = settings->game_saves->text().toStdString();
	config::cheats_path = settings->cheats_path->text().toStdString();

	switch(settings->freq->currentIndex())
	{
		case 0: config::sample_rate = 48000.0; break;
		case 1: config::sample_rate = 44100.0; break;
		case 2: config::sample_rate = 22050.0; break;
		case 3: config::sample_rate = 11025.0; break;
	}

	save_ini_file();
	save_cheats_file();

	//Close SDL
	SDL_Quit();

	//Exit the application
	exit(0);
}

/****** Handle keypress input ******/
void main_menu::keyPressEvent(QKeyEvent* event)
{
	//Disallow key repeats
	if(event->isAutoRepeat()) { return; }

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
	//Disallow key repeats
	if(event->isAutoRepeat()) { return; }

	int sdl_key = qtkey_to_sdlkey(event->key());
	
	//Force input processing in the core
	if(main_menu::gbe_plus != NULL)
	{
		gbe_plus->feed_key_input(sdl_key, false);
	}
}

/****** Event SW or HW screen ******/
bool main_menu::eventFilter(QObject* target, QEvent* event)
{
	//Only process NDS touchscreen events
	if((config::gb_type != 4) || (main_menu::gbe_plus == NULL)) { return QWidget::eventFilter(target, event); }

	//Single click - Press
	else if(event->type() == QEvent::MouseButtonPress)
	{
		if((target == sw_screen) || (target == hw_screen))
		{
			QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
			float x_scaling_factor, y_scaling_factor = 0.0;
			u32 x, y = 0;

			if(config::maintain_aspect_ratio)
			{
				u32 w = (target == sw_screen) ? sw_screen->width() : hw_screen->width();
				u32 h = (target == sw_screen) ? sw_screen->height() : hw_screen->height();
				u32 off_x, off_y = 0;

				get_nds_ar_size(w, h, off_x, off_y);

				x_scaling_factor = w / 256.0;
				y_scaling_factor = h / 384.0;

				x = ((mouse_event->x() - off_x) / x_scaling_factor);
				y = ((mouse_event->y() - off_y) / y_scaling_factor);

				if((mouse_event->x() < off_x) || (mouse_event->x() > (w + off_x))) { return QWidget::eventFilter(target, event); }
				if((mouse_event->y() < off_y) || (mouse_event->y() > (h + off_y))) { return QWidget::eventFilter(target, event); }
			}

			else
			{
				x_scaling_factor = (target == sw_screen) ? (sw_screen->width() / 256.0) : (hw_screen->width() / 256.0);
				y_scaling_factor = (target == sw_screen) ? (sw_screen->height() / 384.0) : (hw_screen->height() / 384.0);

				x = (mouse_event->x() / x_scaling_factor);
			 	y = (mouse_event->y() / y_scaling_factor);
			}

			//Adjust Y for bottom touchscreen
			bool is_bottom = false;

			if((y < 192) && (config::lcd_config & 0x1)) { is_bottom = true; }
			else if((y > 192) && ((config::lcd_config & 0x1) == 0))
			{
				is_bottom = true;
				y -= 192;
			}

			if(is_bottom)
			{
				//Pack Pad, X, Y into a 24-bit number to send to the NDS core
				x &= 0xFF;
				y &= 0xFF;

				u8 pad = 0;

				if(mouse_event->buttons() == Qt::LeftButton) { pad = 1; }
				else if(mouse_event->buttons() == Qt::RightButton) { pad = 2; }
				else { return QWidget::eventFilter(target, event); }

				u32 pack = (pad << 16) | (y << 8) | (x);

				main_menu::gbe_plus->feed_key_input(pack, true);
			}
		}
	}

	//Single click - Release
	else if(event->type() == QEvent::MouseButtonRelease)
	{
		if((target == sw_screen) || (target == hw_screen))
		{
			QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
			float x_scaling_factor, y_scaling_factor = 0.0;
			u32 x, y = 0;

			if(config::maintain_aspect_ratio)
			{
				u32 w = (target == sw_screen) ? sw_screen->width() : hw_screen->width();
				u32 h = (target == sw_screen) ? sw_screen->height() : hw_screen->height();
				u32 off_x, off_y = 0;

				get_nds_ar_size(w, h, off_x, off_y);

				x_scaling_factor = w / 256.0;
				y_scaling_factor = h / 384.0;

				x = ((mouse_event->x() - off_x) / x_scaling_factor);
				y = ((mouse_event->y() - off_y) / y_scaling_factor);

				if((mouse_event->x() < off_x) || (mouse_event->x() > (w + off_x))) { return QWidget::eventFilter(target, event); }
				if((mouse_event->y() < off_y) || (mouse_event->y() > (h + off_y))) { return QWidget::eventFilter(target, event); }
			}

			else
			{
				x_scaling_factor = (target == sw_screen) ? (sw_screen->width() / 256.0) : (hw_screen->width() / 256.0);
				y_scaling_factor = (target == sw_screen) ? (sw_screen->height() / 384.0) : (hw_screen->height() / 384.0);

				x = (mouse_event->x() / x_scaling_factor);
			 	y = (mouse_event->y() / y_scaling_factor);
			}

			//Adjust Y for bottom touchscreen
			bool is_bottom = false;

			if((y < 192) && (config::lcd_config & 0x1)) { is_bottom = true; }
			else if((y > 192) && ((config::lcd_config & 0x1) == 0))
			{
				is_bottom = true;
				y -= 192;
			}

			if(is_bottom)
			{
				//Pack Pad, X, Y into a 24-bit number to send to the NDS core
				x &= 0xFF;
				y &= 0xFF;

				u8 pad = 0;

				if(mouse_event->button() == Qt::LeftButton) { pad = 1; }
				else if(mouse_event->button() == Qt::RightButton) { pad = 2; }
				else { return QWidget::eventFilter(target, event); }

				u32 pack = (pad << 16) | (y << 8) | (x);

				main_menu::gbe_plus->feed_key_input(pack, false);
			}
		}
	}

	//Mouse motion
	else if(event->type() == QEvent::MouseMove)
	{
		if((target == sw_screen) || (target == hw_screen))
		{
			//Only process mouse motion if touch_by_mouse has been set in NDS core
			if(main_menu::gbe_plus->get_core_data(2) == 0) { return QWidget::eventFilter(target, event); }

			QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
			float x_scaling_factor, y_scaling_factor = 0.0;
			u32 x, y = 0;

			if(config::maintain_aspect_ratio)
			{
				u32 w = (target == sw_screen) ? sw_screen->width() : hw_screen->width();
				u32 h = (target == sw_screen) ? sw_screen->height() : hw_screen->height();
				u32 off_x, off_y = 0;

				get_nds_ar_size(w, h, off_x, off_y);

				x_scaling_factor = w / 256.0;
				y_scaling_factor = h / 384.0;

				x = ((mouse_event->x() - off_x) / x_scaling_factor);
				y = ((mouse_event->y() - off_y) / y_scaling_factor);

				if((mouse_event->x() < off_x) || (mouse_event->x() > (w + off_x))) { return QWidget::eventFilter(target, event); }
				if((mouse_event->y() < off_y) || (mouse_event->y() > (h + off_y))) { return QWidget::eventFilter(target, event); }
			}

			else
			{
				x_scaling_factor = (target == sw_screen) ? (sw_screen->width() / 256.0) : (hw_screen->width() / 256.0);
				y_scaling_factor = (target == sw_screen) ? (sw_screen->height() / 384.0) : (hw_screen->height() / 384.0);

				x = (mouse_event->x() / x_scaling_factor);
			 	y = (mouse_event->y() / y_scaling_factor);
			}

			//Adjust Y for bottom touchscreen
			bool is_bottom = false;

			if((y < 192) && (config::lcd_config & 0x1)) { is_bottom = true; }
			else if((y > 192) && ((config::lcd_config & 0x1) == 0))
			{
				is_bottom = true;
				y -= 192;
			}

			if(is_bottom)
			{
				//Pack Pad, X, Y into a 24-bit number to send to the NDS core
				x &= 0xFF;
				y &= 0xFF;

				u8 pad = 4;
				u32 pack = (pad << 16) | (y << 8) | (x);

				main_menu::gbe_plus->feed_key_input(pack, true);
			}
		}
	}

	return QWidget::eventFilter(target, event);
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

		QFile test_file;
		std::string test_bios_path = "";
		u8 system_type = get_system_type_from_file(config::rom_file);

		switch(system_type)
		{
			case 0x1: test_bios_path = config::dmg_bios_path; break;
			case 0x2: test_bios_path = config::gbc_bios_path; break;
			case 0x3: test_bios_path = config::agb_bios_path; break;
			case 0x4: test_bios_path = config::nds7_bios_path; break;
			case 0x5: config::use_bios = false;
		}

		test_file.setFileName(QString::fromStdString(test_bios_path));

		if(!test_file.exists() && config::use_bios)
		{
			std::string mesg_text;

			if(!test_bios_path.empty()) { mesg_text = "The BIOS file: '" + test_bios_path + "' could not be loaded"; }
		
			else
			{
				if(system_type == 4)
				{
					mesg_text = "ARM7 BIOS file not specified.\nPlease check your Paths settings or disable the 'Use BIOS/Boot ROM' option";
				} 
				
				else 
				{
					mesg_text = "No BIOS file specified for this system.\nPlease check your Paths settings or disable the 'Use BIOS/Boot ROM' option";
				}
			} 

			warning_box->setText(QString::fromStdString(mesg_text));
			warning_box->show();
			return;
		}

		//Perform a second test for NDS9 BIOS
		if(system_type == 4)
		{
			test_file.setFileName(QString::fromStdString(config::nds9_bios_path));

			if(!test_file.exists() && config::use_bios)
			{
				std::string mesg_text;

				if(!test_bios_path.empty()) { mesg_text = "The BIOS file: '" + test_bios_path + "' could not be loaded"; }
				else { mesg_text = "ARM9 BIOS file not specified.\nPlease check your Paths settings or disable the 'Use BIOS/Boot ROM' option"; } 

				warning_box->setText(QString::fromStdString(mesg_text));
				warning_box->show();
				return;
			}
		}

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
			QApplication::setOverrideCursor(Qt::BlankCursor);
			fullscreen_mode = true;
			setWindowState(Qt::WindowFullScreen);
			menu_bar->hide();
			showFullScreen();
		}

		else
		{
			QApplication::setOverrideCursor(Qt::ArrowCursor);
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
	settings->controls_combo->setVisible(false);
}

/****** Shows the Display settings dialog ******/
void main_menu::show_display_settings()
{
	settings->show();
	settings->tabs->setCurrentIndex(1);
	settings->controls_combo->setVisible(false);
}

/****** Shows the Sound settings dialog ******/
void main_menu::show_sound_settings()
{
	settings->show();
	settings->tabs->setCurrentIndex(2);
	settings->controls_combo->setVisible(false);
}

/****** Shows the Control settings dialog ******/
void main_menu::show_control_settings()
{
	settings->show();
	settings->tabs->setCurrentIndex(3);
	settings->controls_combo->setVisible(true);
}

/****** Shows the Netplay settings dialog ******/
void main_menu::show_netplay_settings()
{
	settings->show();
	settings->tabs->setCurrentIndex(4);
	settings->controls_combo->setVisible(false);
}

/****** Shows the Paths settings dialog ******/
void main_menu::show_paths_settings()
{
	settings->show();
	settings->tabs->setCurrentIndex(5);
	settings->controls_combo->setVisible(false);
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

	//Do nothing for SGB for now
	else if(is_sgb_core) { return; }

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

	//Setup OBJ meta tile tab
	if(main_menu::gbe_plus != NULL)
	{
		//Setup 8x8 or 8x16 mode
		u8 obj_height = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x04) ? 16 : 8;

		if(obj_height == 16) { cgfx->obj_meta_height->setSingleStep(2); }
		else { cgfx->obj_meta_height->setSingleStep(1); }

		//Also update OBJ meta tile resource
		int obj_index = cgfx->obj_meta_index->value();
		QImage selected_img = (obj_height == 16) ? cgfx->grab_obj_data(obj_index).scaled(128, 256) : cgfx->grab_obj_data(obj_index).scaled(256, 256);
		cgfx->obj_select_img->setPixmap(QPixmap::fromImage(selected_img));
	}

	cgfx->reset_inputs();
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
		if((config::gb_type <= 2) && (!is_sgb_core)) 
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
	//Check to see if the file actually exists
	QFile test_file(QString::fromStdString(config::recent_files[file_id]));

	if(!test_file.exists())
	{
		std::string mesg_text = "The specified file: '" + config::recent_files[file_id] + "' could not be loaded"; 
		warning_box->setText(QString::fromStdString(mesg_text));
		warning_box->show();
		return;
	}

	std::string test_bios_path = "";
	u8 system_type = get_system_type_from_file(config::recent_files[file_id]);

	switch(system_type)
	{
		case 0x1: test_bios_path = config::dmg_bios_path; break;
		case 0x2: test_bios_path = config::gbc_bios_path; break;
		case 0x3: test_bios_path = config::agb_bios_path; break;
		case 0x4: test_bios_path = config::nds7_bios_path; break;
		case 0x5: config::use_bios = false;
	}

	test_file.setFileName(QString::fromStdString(test_bios_path));

	if(!test_file.exists() && config::use_bios)
	{
		std::string mesg_text;

		if(!test_bios_path.empty()) { mesg_text = "The BIOS file: '" + test_bios_path + "' could not be loaded"; }
		
		else
		{
			if(system_type == 4)
			{
				mesg_text = "ARM7 BIOS file not specified.\nPlease check your Paths settings or disable the 'Use BIOS/Boot ROM' option";
			} 
				
			else 
			{
				mesg_text = "No BIOS file specified for this system.\nPlease check your Paths settings or disable the 'Use BIOS/Boot ROM' option";
			}
		} 

		warning_box->setText(QString::fromStdString(mesg_text));
		warning_box->show();
		return;
	}

	//Perform a second test for NDS9 BIOS
	if(system_type == 4)
	{
		test_file.setFileName(QString::fromStdString(config::nds9_bios_path));

		if(!test_file.exists() && config::use_bios)
		{
			std::string mesg_text;

			if(!test_bios_path.empty()) { mesg_text = "The BIOS file: '" + test_bios_path + "' could not be loaded"; }
			else { mesg_text = "ARM9 BIOS file not specified.\nPlease check your Paths settings or disable the 'Use BIOS/Boot ROM' option"; } 

			warning_box->setText(QString::fromStdString(mesg_text));
			warning_box->show();
			return;
		}
	}

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

/****** Starts special communications (IR or SIO device) ******/
void main_menu::start_special_comm()
{
	if(main_menu::gbe_plus != NULL)
	{
		//This just feeds a simulated F3 keypress to the core
		gbe_plus->feed_key_input(SDLK_F3, true);
	}
}

/****** Calculates the NDS screen size + offsets when maintaining proper aspect ratio ******/
void main_menu::get_nds_ar_size(u32 &width, u32 &height, u32 &offset_x, u32 &offset_y)
{
	float w_ratio = 256.0 / width;
	float h_ratio = 384.0 / height;

	u32 original_w = width;
	u32 original_h = height;

	offset_x = 0;
	offset_y = 0;

	//Calculate height to maintain aspect ratio
	if(w_ratio > h_ratio)
	{
		height = (384 * (width / 256.0)) ;

		//Calculate offsets
		offset_x = 0;
		offset_y = (original_h - height) / 2;
	}

	else if(h_ratio > w_ratio)
	{
		width = (256 * (height / 384.0)) ;

		//Calculate offsets
		offset_x = (original_w - width) / 2;
		offset_y = 0;
	}
}

/****** Static definitions ******/
core_emu* main_menu::gbe_plus = NULL;
dmg_debug* main_menu::dmg_debugger = NULL;
