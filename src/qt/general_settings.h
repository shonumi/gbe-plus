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

#ifdef GBE_QT_5
#include <QtWidgets>
#endif

#ifdef GBE_QT_4
#include <QtGui>
#endif

#include "common/common.h"
#include "data_dialog.h"
#include "cheat_menu.h"
#include "rtc_menu.h"
#include "pp2_menu.h"
#include "ps_menu.h"
#include "zzh_menu.h"
#include "con_ir_menu.h"
#include "mpos_menu.h"
#include "tbf_menu.h"
#include "utp_menu.h"
#include "magic_reader_menu.h"
#include "mw_menu.h"

class gen_settings : public QDialog
{
	Q_OBJECT
	
	public:
	gen_settings(QWidget *parent = 0);

	void set_ini_options();

	QTabWidget* tabs;
	QDialogButtonBox* tabs_button;
	QComboBox* controls_combo;
	u8 last_control_id;

	//General tab widgets
	QComboBox* sys_type;
	QCheckBox* bios;
	QCheckBox* firmware;
	QComboBox* special_cart;
	QComboBox* overclock;
	QCheckBox* cheats;
	QPushButton* edit_cheats;
	QPushButton* config_sio;
	QPushButton* config_ir;
	QPushButton* config_slot2;
	QComboBox* sio_dev;
	QComboBox* ir_dev;
	QComboBox* slot2_dev;
	QCheckBox* auto_patch;

	//Display tab widgets
	QComboBox* screen_scale;
	QComboBox* cgfx_scale;
	QComboBox* dmg_gbc_pal;
	QComboBox* ogl_frag_shader;
	QComboBox* ogl_vert_shader;
	QCheckBox* ogl;
	QCheckBox* load_cgfx;
	QCheckBox* aspect_ratio;
	QCheckBox* osd_enable;

	//Sound tab widgets
	QComboBox* freq;
	QSlider* volume;
	QCheckBox* sound_on;
	QCheckBox* stereo_enable;

	data_dialog* data_folder;

	double sample_rate;
	bool resize_screen;

	bool grab_input;
	int last_key;
	int input_index;

	QVBoxLayout* controls_layout;
	QVBoxLayout* advanced_controls_layout;
	QVBoxLayout* hotkey_controls_layout;
	QVBoxLayout* bcg_controls_layout;
	QVBoxLayout* vc_controls_layout;

	QSlider* dead_zone;

	bool is_sgb_core;

	//Paths tab widgets
	QLineEdit* dmg_bios;
	QLineEdit* gbc_bios;
	QLineEdit* gba_bios;
	QLineEdit* nds_firmware;
	QLineEdit* manifest;
	QLineEdit* dump_bg;
	QLineEdit* dump_obj;
	QLineEdit* screenshot;
	QLineEdit* game_saves;
	QLineEdit* cheats_path;

	QLabel* dmg_bios_label;
	QLabel* gbc_bios_label;
	QLabel* gba_bios_label;
	QLabel* nds_firmware_label;
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
	QLineEdit* input_x;
	QLineEdit* input_y;
	QLineEdit* input_start;
	QLineEdit* input_select;
	QLineEdit* input_left;
	QLineEdit* input_right;
	QLineEdit* input_up;
	QLineEdit* input_down;
	QLineEdit* input_l;
	QLineEdit* input_r;
	QLineEdit* input_con_left;
	QLineEdit* input_con_right;
	QLineEdit* input_con_up;
	QLineEdit* input_con_down;
	QLineEdit* input_con_1;
	QLineEdit* input_con_2;
	QLineEdit* input_turbo;
	QLineEdit* input_mute;
	QLineEdit* input_camera;
	QLineEdit* input_swap_screen;
	QLineEdit* input_shift_screen;

	QPushButton* config_a;
	QPushButton* config_b;
	QPushButton* config_x;
	QPushButton* config_y;
	QPushButton* config_start;
	QPushButton* config_select;
	QPushButton* config_left;
	QPushButton* config_right;
	QPushButton* config_up;
	QPushButton* config_down;
	QPushButton* config_l;
	QPushButton* config_r;
	QPushButton* config_con_left;
	QPushButton* config_con_right;
	QPushButton* config_con_up;
	QPushButton* config_con_down;
	QPushButton* config_con_1;
	QPushButton* config_con_2;
	QPushButton* config_turbo;
	QPushButton* config_mute;
	QPushButton* config_camera;
	QPushButton* config_swap_screen;
	QPushButton* config_shift_screen;

	//Advanced controls tab widget
	QCheckBox* rumble_on;

	//Battle Chip Gate tab widgets
	QComboBox* chip_gate_type;
	QComboBox* battle_chip_1;
	QComboBox* battle_chip_2;
	QComboBox* battle_chip_3;
	QComboBox* battle_chip_4;

	//Virtual Cursor controls tab widgets
	QCheckBox* vc_on;
	QSpinBox* vc_opacity;
	QSpinBox* vc_timeout;
	QLineEdit* vc_path;

	QLabel* vc_path_label;

	//Netplay tab widgets
	QCheckBox* enable_netplay;
	QCheckBox* hard_sync;
	QCheckBox* net_gate;
	QCheckBox* real_server;
	QSpinBox* sync_threshold;
	QSpinBox* server_port;
	QSpinBox* client_port;
	QLineEdit* ip_address;
	QPushButton* ip_update;
	QLineEdit* gbma_address;
	QPushButton* gbma_update;

	//Misc widgets
	cheat_menu* dmg_cheat_menu;
	rtc_menu* real_time_clock_menu;
	pp2_menu* pokemon_pikachu_menu;
	ps_menu* pocket_sakura_menu;
	zzh_menu* full_changer_menu;
	con_ir_menu* chalien_menu;
	mpos_menu* multi_plust_menu;
	tbf_menu* turbo_file_menu;
	utp_menu* ubisoft_pedometer_menu;
	mr_menu* magic_reader_menu;
	mw_menu* magical_watch_menu;
	QMessageBox* warning_box;

	void update_volume();

	protected:
	void paintEvent(QPaintEvent* event);
	void keyPressEvent(QKeyEvent* event);
	void closeEvent(QCloseEvent* event);
	bool eventFilter(QObject* target, QEvent* event);

	private slots:
	void set_bios();
	void set_firmware();
	void sio_dev_change();
	void ir_dev_change();
	void slot2_dev_change();
	void overclock_change();
	void set_patches();
	void show_cheats();
	void show_rtc();
	void show_sio_config();
	void show_ir_config();
	void show_slot2_config();
	void set_ogl();
	void screen_scale_change();
	void aspect_ratio_change();
	void set_osd();
	void dmg_gbc_pal_change();
	void ogl_frag_change();
	void ogl_vert_change();
	void set_cgfx();
	void volume_change();
	void sample_rate_change();
	void mute();
	void set_paths(int index);
	void rebuild_input_index();
	void input_device_change();
	void dead_zone_change();
	void set_netplay();
	void set_hard_sync();
	void set_net_gate();
	void set_real_server();
	void get_chip_list();
	void set_battle_chip();
	void update_vc_opacity();
	void update_vc_timeout();
	void update_sync_threshold();
	void update_server_port();
	void update_client_port();
	void update_ip_addr();
	void update_gbma_addr();
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
	QWidget* input_x_set;
	QWidget* input_y_set;
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
	QWidget* con_up_set;
	QWidget* con_down_set;
	QWidget* con_left_set;
	QWidget* con_right_set;
	QWidget* con_1_set;
	QWidget* con_2_set;

	QWidget* hotkey_turbo_set;
	QWidget* hotkey_mute_set;
	QWidget* hotkey_camera_set;
	QWidget* hotkey_swap_screen_set;
	QWidget* hotkey_shift_screen_set;

	QWidget* bcg_gate_set;
	QWidget* bcg_chip_1_set;
	QWidget* bcg_chip_2_set;
	QWidget* bcg_chip_3_set;
	QWidget* bcg_chip_4_set;

	QWidget* vc_enable_set;
	QWidget* vc_opacity_set;
	QWidget* vc_timeout_set;
	QWidget* vc_path_set;

	void process_joystick_event();
	void input_delay(QPushButton* input_button); 

	SDL_Joystick* jstick;
	int input_type;
	u32 joystick_count;
	u16 chip_list[512];
	u16 init_chip_list[4];
};

#endif //SETTINGS_GBE_QT 
