// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : general_settings.h
// Date : June 28, 2015
// Description : Main menu
//
// Dialog for various options
// Deals with Graphics, Audio, Input, Paths, etc

#ifndef SETTINGS_GBE_QT
#define SETTINGS_GBE_QT

#include <SDL2/SDL.h>

#include <QtGui>

#include "data_dialog.h"
#include "cheat_menu.h"

class gen_settings : public QDialog
{
	Q_OBJECT
	
	public:
	gen_settings(QWidget *parent = 0);

	void set_ini_options();

	QTabWidget* tabs;
	QDialogButtonBox* tabs_button;
	QPushButton* advanced_button;

	//General tab widgets
	QComboBox* sys_type;
	QCheckBox* bios;
	QCheckBox* multicart;
	QCheckBox* cheats;
	QPushButton* edit_cheats;
	QComboBox* sio_dev;
	QCheckBox* auto_patch;

	//Display tab widgets
	QComboBox* screen_scale;
	QComboBox* cgfx_scale;
	QComboBox* dmg_gbc_pal;
	QComboBox* ogl_frag_shader;
	QCheckBox* ogl;
	QCheckBox* load_cgfx;
	QCheckBox* aspect_ratio;

	//Sound tab widgets
	QComboBox* freq;
	QSlider* volume;
	QCheckBox* sound_on;

	data_dialog* data_folder;

	double sample_rate;
	bool resize_screen;

	bool grab_input;
	int last_key;
	int input_index;

	QVBoxLayout* controls_layout;
	QVBoxLayout* advanced_controls_layout;
	bool config_advanced_controls;

	QSlider* dead_zone;

	//Paths tab widgets
	QLineEdit* dmg_bios;
	QLineEdit* gbc_bios;
	QLineEdit* gba_bios;
	QLineEdit* manifest;
	QLineEdit* dump_bg;
	QLineEdit* dump_obj;
	QLineEdit* screenshot;
	QLineEdit* game_saves;
	QLineEdit* cheats_path;

	QLabel* dmg_bios_label;
	QLabel* gbc_bios_label;
	QLabel* gba_bios_label;
	QLabel* manifest_label;
	QLabel* dump_bg_label;
	QLabel* dump_obj_label;
	QLabel* screenshot_label;
	QLabel* game_saves_label;
	QLabel* cheats_path_label;

	//Controls tab widget
	QComboBox* input_device;

	QLineEdit* input_a;
	QLineEdit* input_b;
	QLineEdit* input_start;
	QLineEdit* input_select;
	QLineEdit* input_left;
	QLineEdit* input_right;
	QLineEdit* input_up;
	QLineEdit* input_down;
	QLineEdit* input_l;
	QLineEdit* input_r;
	QLineEdit* input_gyro_left;
	QLineEdit* input_gyro_right;
	QLineEdit* input_gyro_up;
	QLineEdit* input_gyro_down;

	QPushButton* config_a;
	QPushButton* config_b;
	QPushButton* config_start;
	QPushButton* config_select;
	QPushButton* config_left;
	QPushButton* config_right;
	QPushButton* config_up;
	QPushButton* config_down;
	QPushButton* config_l;
	QPushButton* config_r;
	QPushButton* config_gyro_left;
	QPushButton* config_gyro_right;
	QPushButton* config_gyro_up;
	QPushButton* config_gyro_down;

	//Advanced controls tab widget
	QCheckBox* rumble_on;

	//Netplay tab widgets
	QCheckBox* enable_netplay;
	QCheckBox* hard_sync;
	QSpinBox* server_port;
	QSpinBox* client_port;
	QLineEdit* ip_address;
	QPushButton* ip_update;

	//Misc widgets
	cheat_menu* dmg_cheat_menu;
	QMessageBox* warning_box;

	void update_volume();

	protected:
	void paintEvent(QPaintEvent* event);
	void keyPressEvent(QKeyEvent* event);
	void closeEvent(QCloseEvent* event);
	bool eventFilter(QObject* target, QEvent* event);

	private slots:
	void set_bios();
	void sio_dev_change();
	void set_patches();
	void show_cheats();
	void set_ogl();
	void screen_scale_change();
	void aspect_ratio_change();
	void dmg_gbc_pal_change();
	void ogl_frag_change();
	void set_cgfx();
	void volume_change();
	void sample_rate_change();
	void mute();
	void set_paths(int index);
	void input_device_change();
	void dead_zone_change();
	void set_netplay();
	void set_hard_sync();
	void update_server_port();
	void update_client_port();
	void update_ip_addr();
	void configure_button(int button);
	void close_input();
	void close_settings();
	void switch_control_layout();
	void select_folder();
	void reject_folder();

	private:
	QDialog* general;
	QDialog* display;
	QDialog* sound;
	QDialog* controls;
	QDialog* netplay;
	QDialog* paths;

	QWidget* input_device_set;
	QWidget* input_a_set;
	QWidget* input_b_set;
	QWidget* input_start_set;
	QWidget* input_select_set;
	QWidget* input_left_set;
	QWidget* input_right_set;
	QWidget* input_up_set;
	QWidget* input_down_set;
	QWidget* input_l_set;
	QWidget* input_r_set;
	QWidget* dead_zone_set;

	QWidget* rumble_set;
	QWidget* gyro_up_set;
	QWidget* gyro_down_set;
	QWidget* gyro_left_set;
	QWidget* gyro_right_set;

	void process_joystick_event();
	void input_delay(QPushButton* input_button); 

	SDL_Joystick* jstick;
	int input_type;
};

#endif //SETTINGS_GBE_QT 
