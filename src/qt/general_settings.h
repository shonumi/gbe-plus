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

#include <SDL/SDL.h>

#include <QtGui>

#include "data_dialog.h"

class gen_settings : public QDialog
{
	Q_OBJECT
	
	public:
	gen_settings(QWidget *parent = 0);

	void set_ini_options();

	QTabWidget* tabs;
	QDialogButtonBox* tabs_button;

	//General tab widgets
	QComboBox* sys_type;
	QCheckBox* bios;

	//Display tab widgets
	QComboBox* screen_scale;
	QComboBox* cgfx_scale;
	QComboBox* dmg_gbc_pal;
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

	QSlider* dead_zone;

	//Paths tab widgets
	QLineEdit* dmg_bios;
	QLineEdit* gbc_bios;
	QLineEdit* gba_bios;
	QLineEdit* manifest;
	QLineEdit* dump_bg;
	QLineEdit* dump_obj;
	QLineEdit* screenshot;

	QLabel* dmg_bios_label;
	QLabel* gbc_bios_label;
	QLabel* gba_bios_label;
	QLabel* manifest_label;
	QLabel* dump_bg_label;
	QLabel* dump_obj_label;
	QLabel* screenshot_label;

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

	void update_volume();

	protected:
	void paintEvent(QPaintEvent* event);
	void keyPressEvent(QKeyEvent* event);
	void closeEvent(QCloseEvent* event);
	bool eventFilter(QObject* target, QEvent* event);

	private slots:
	void set_bios();
	void screen_scale_change();
	void aspect_ratio_change();
	void dmg_gbc_pal_change();
	void set_cgfx();
	void volume_change();
	void sample_rate_change();
	void mute();
	void set_paths(int index);
	void input_device_change();
	void dead_zone_change();
	void configure_button(int button);
	void close_input();
	void select_folder();
	void reject_folder();

	private:
	void process_joystick_event();
	void input_delay(QPushButton* input_button); 

	SDL_Joystick* jstick;
	int input_type;
};

#endif //SETTINGS_GBE_QT 
