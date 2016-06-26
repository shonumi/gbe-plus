// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : general_settings.h
// Date : June 28, 2015
// Description : Main menu
//
// Dialog for various options
// Deals with Graphics, Audio, Input, Paths, etc

#include <iostream>

#include "general_settings.h"
#include "main_menu.h"
#include "qt_common.h"

#include "common/config.h"
#include "common/cgfx_common.h"

/****** General settings constructor ******/
gen_settings::gen_settings(QWidget *parent) : QDialog(parent)
{
	//Set up tabs
	tabs = new QTabWidget(this);
	
	general = new QDialog;
	display = new QDialog;
	sound = new QDialog;
	controls = new QDialog;
	paths = new QDialog;

	tabs->addTab(general, tr("General"));
	tabs->addTab(display, tr("Display"));
	tabs->addTab(sound, tr("Sound"));
	tabs->addTab(controls, tr("Controls"));
	tabs->addTab(paths, tr("Paths"));

	tabs_button = new QDialogButtonBox(QDialogButtonBox::Close);
	advanced_button = tabs_button->addButton("Advanced Controls", QDialogButtonBox::ActionRole);

	//General settings - Emulated system type
	QWidget* sys_type_set = new QWidget(general);
	QLabel* sys_type_label = new QLabel("Emulated System Type : ", sys_type_set);
	sys_type = new QComboBox(sys_type_set);
	sys_type->addItem("Auto");
	sys_type->addItem("Game Boy [DMG]");
	sys_type->addItem("Game Boy Color [GBC]");
	sys_type->addItem("Game Boy Advance [GBA]");

	QHBoxLayout* sys_type_layout = new QHBoxLayout;
	sys_type_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	sys_type_layout->addWidget(sys_type_label);
	sys_type_layout->addWidget(sys_type);
	sys_type_set->setLayout(sys_type_layout);

	//General settings - Use BIOS
	QWidget* bios_set = new QWidget(general);
	QLabel* bios_label = new QLabel("Use BIOS/Boot ROM", bios_set);
	bios = new QCheckBox(bios_set);

	QHBoxLayout* bios_layout = new QHBoxLayout;
	bios_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	bios_layout->addWidget(bios);
	bios_layout->addWidget(bios_label);
	bios_set->setLayout(bios_layout);

	//General settings - Emulate multicarts
	QWidget* multicart_set = new QWidget(general);
	QLabel* multicart_label = new QLabel("Emulate multicart ROMs (MBC1M)", multicart_set);
	multicart = new QCheckBox(multicart_set);

	QHBoxLayout* multicart_layout = new QHBoxLayout;
	multicart_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	multicart_layout->addWidget(multicart);
	multicart_layout->addWidget(multicart_label);
	multicart_set->setLayout(multicart_layout);

	QVBoxLayout* gen_layout = new QVBoxLayout;
	gen_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	gen_layout->addWidget(sys_type_set);
	gen_layout->addWidget(bios_set);
	gen_layout->addWidget(multicart_set);
	general->setLayout(gen_layout);

	//Display settings - Screen scale
	QWidget* screen_scale_set = new QWidget(display);
	QLabel* screen_scale_label = new QLabel("Screen Scale : ");
	screen_scale = new QComboBox(screen_scale_set);
	screen_scale->addItem("1x");
	screen_scale->addItem("2x");
	screen_scale->addItem("3x");
	screen_scale->addItem("4x");
	screen_scale->addItem("5x");
	screen_scale->addItem("6x");
	screen_scale->addItem("7x");
	screen_scale->addItem("8x");
	screen_scale->addItem("9x");
	screen_scale->addItem("10x");

	QHBoxLayout* screen_scale_layout = new QHBoxLayout;
	screen_scale_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	screen_scale_layout->addWidget(screen_scale_label);
	screen_scale_layout->addWidget(screen_scale);
	screen_scale_set->setLayout(screen_scale_layout);

	//Display settings - CGFX scale
	QWidget* cgfx_scale_set = new QWidget(display);
	QLabel* cgfx_scale_label = new QLabel("Custom Graphics (CGFX) Scale : ");
	cgfx_scale = new QComboBox(cgfx_scale_set);
	cgfx_scale->addItem("1x");
	cgfx_scale->addItem("2x");
	cgfx_scale->addItem("3x");
	cgfx_scale->addItem("4x");
	cgfx_scale->addItem("5x");
	cgfx_scale->addItem("6x");
	cgfx_scale->addItem("7x");
	cgfx_scale->addItem("8x");
	cgfx_scale->addItem("9x");
	cgfx_scale->addItem("10x");

	QHBoxLayout* cgfx_scale_layout = new QHBoxLayout;
	cgfx_scale_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	cgfx_scale_layout->addWidget(cgfx_scale_label);
	cgfx_scale_layout->addWidget(cgfx_scale);
	cgfx_scale_set->setLayout(cgfx_scale_layout);

	//Display settings - DMG on GBC palette
	QWidget* dmg_gbc_pal_set = new QWidget(display);
	QLabel* dmg_gbc_pal_label = new QLabel("DMG Color Palette : ");
	dmg_gbc_pal = new QComboBox(dmg_gbc_pal_set);
	dmg_gbc_pal->addItem("OFF");
	dmg_gbc_pal->addItem("GBC BOOTROM - NO INPUT");
	dmg_gbc_pal->addItem("GBC BOOTROM - UP");
	dmg_gbc_pal->addItem("GBC BOOTROM - DOWN");
	dmg_gbc_pal->addItem("GBC BOOTROM - LEFT");
	dmg_gbc_pal->addItem("GBC BOOTROM - RIGHT");
	dmg_gbc_pal->addItem("GBC BOOTROM - UP+A");
	dmg_gbc_pal->addItem("GBC BOOTROM - DOWN+A");
	dmg_gbc_pal->addItem("GBC BOOTROM - LEFT+A");
	dmg_gbc_pal->addItem("GBC BOOTROM - RIGHT+A");
	dmg_gbc_pal->addItem("GBC BOOTROM - UP+B");
	dmg_gbc_pal->addItem("GBC BOOTROM - DOWN+B");
	dmg_gbc_pal->addItem("GBC BOOTROM - LEFT+B");
	dmg_gbc_pal->addItem("GBC BOOTROM - RIGHT+B");
	dmg_gbc_pal->addItem("DMG - Classic Green");

	QHBoxLayout* dmg_gbc_pal_layout = new QHBoxLayout;
	dmg_gbc_pal_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	dmg_gbc_pal_layout->addWidget(dmg_gbc_pal_label);
	dmg_gbc_pal_layout->addWidget(dmg_gbc_pal);
	dmg_gbc_pal_set->setLayout(dmg_gbc_pal_layout);
	
	//Display settings - Use OpenGL
	QWidget* ogl_set = new QWidget(display);
	QLabel* ogl_label = new QLabel("Use OpenGL");
	ogl = new QCheckBox(ogl_set);

	QHBoxLayout* ogl_layout = new QHBoxLayout;
	ogl_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	ogl_layout->addWidget(ogl);
	ogl_layout->addWidget(ogl_label);
	ogl_set->setLayout(ogl_layout);

	//Display settings - Use CGFX
	QWidget* load_cgfx_set = new QWidget(display);
	QLabel* load_cgfx_label = new QLabel("Load Custom Graphics (CGFX)");
	load_cgfx = new QCheckBox(load_cgfx_set);

	QHBoxLayout* load_cgfx_layout = new QHBoxLayout;
	load_cgfx_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	load_cgfx_layout->addWidget(load_cgfx);
	load_cgfx_layout->addWidget(load_cgfx_label);
	load_cgfx_set->setLayout(load_cgfx_layout);

	//Display settings - Aspect Ratio
	QWidget* aspect_set = new QWidget(display);
	QLabel* aspect_label = new QLabel("Maintain Aspect Ratio");
	aspect_ratio = new QCheckBox(aspect_set);

	QHBoxLayout* aspect_layout = new QHBoxLayout;
	aspect_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	aspect_layout->addWidget(aspect_ratio);
	aspect_layout->addWidget(aspect_label);
	aspect_set->setLayout(aspect_layout);

	QVBoxLayout* disp_layout = new QVBoxLayout;
	disp_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	disp_layout->addWidget(screen_scale_set);
	disp_layout->addWidget(cgfx_scale_set);
	disp_layout->addWidget(dmg_gbc_pal_set);
	disp_layout->addWidget(ogl_set);
	disp_layout->addWidget(load_cgfx_set);
	disp_layout->addWidget(aspect_set);
	display->setLayout(disp_layout);

	//Sound settings - Output frequency
	QWidget* freq_set = new QWidget(sound);
	QLabel* freq_label = new QLabel("Output Frequency : ");
	freq = new QComboBox(freq_set);
	freq->addItem("48000Hz");
	freq->addItem("44100Hz");
	freq->addItem("22050Hz");
	freq->addItem("11025Hz");
	freq->setCurrentIndex(1);

	QHBoxLayout* freq_layout = new QHBoxLayout;
	freq_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	freq_layout->addWidget(freq_label);
	freq_layout->addWidget(freq);
	freq_set->setLayout(freq_layout);

	//Sound settings - Sound on/off
	QWidget* sound_on_set = new QWidget(sound);
	QLabel* sound_on_label = new QLabel("Enable Sound");
	sound_on = new QCheckBox(sound_on_set);
	sound_on->setChecked(true);

	QHBoxLayout* sound_on_layout = new QHBoxLayout;
	sound_on_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	sound_on_layout->addWidget(sound_on);
	sound_on_layout->addWidget(sound_on_label);
	sound_on_set->setLayout(sound_on_layout);

	//Sound settings - Volume
	QWidget* volume_set = new QWidget(sound);
	QLabel* volume_label = new QLabel("Volume : ");
	volume = new QSlider(sound);
	volume->setMaximum(128);
	volume->setMinimum(0);
	volume->setValue(128);
	volume->setOrientation(Qt::Horizontal);

	QHBoxLayout* volume_layout = new QHBoxLayout;
	volume_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	volume_layout->addWidget(volume_label);
	volume_layout->addWidget(volume);
	volume_set->setLayout(volume_layout);

	QVBoxLayout* audio_layout = new QVBoxLayout;
	audio_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	audio_layout->addWidget(freq_set);
	audio_layout->addWidget(sound_on_set);
	audio_layout->addWidget(volume_set);
	sound->setLayout(audio_layout);

	//Control settings - Device
	input_device_set = new QWidget(controls);
	QLabel* input_device_label = new QLabel("Input Device : ");
	input_device = new QComboBox(input_device_set);
	input_device->addItem("Keyboard");

	QHBoxLayout* input_device_layout = new QHBoxLayout;
	input_device_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	input_device_layout->addWidget(input_device_label);
	input_device_layout->addWidget(input_device);
	input_device_set->setLayout(input_device_layout);

	//Control settings- Dead-zone
	dead_zone_set = new QWidget(controls);
	QLabel* dead_zone_label = new QLabel("Dead Zone : ");
	dead_zone = new QSlider(sound);
	dead_zone->setMaximum(32767);
	dead_zone->setMinimum(0);
	dead_zone->setValue(16000);
	dead_zone->setOrientation(Qt::Horizontal);

	QHBoxLayout* dead_zone_layout = new QHBoxLayout;
	dead_zone_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	dead_zone_layout->addWidget(dead_zone_label);
	dead_zone_layout->addWidget(dead_zone);
	dead_zone_layout->setContentsMargins(6, 0, 0, 0);
	dead_zone_set->setLayout(dead_zone_layout);

	//Control settings - A button
	input_a_set = new QWidget(controls);
	QLabel* input_a_label = new QLabel("Button A : ");
	input_a = new QLineEdit(controls);
	config_a = new QPushButton("Configure");
	input_a->setMaximumWidth(100);
	config_a->setMaximumWidth(100);

	QHBoxLayout* input_a_layout = new QHBoxLayout;
	input_a_layout->addWidget(input_a_label, 0, Qt::AlignLeft);
	input_a_layout->addWidget(input_a, 0, Qt::AlignLeft);
	input_a_layout->addWidget(config_a, 0, Qt::AlignLeft);
	input_a_layout->setContentsMargins(6, 0, 0, 0);
	input_a_set->setLayout(input_a_layout);

	//Control settings - B button
	input_b_set = new QWidget(controls);
	QLabel* input_b_label = new QLabel("Button B : ");
	input_b = new QLineEdit(controls);
	config_b = new QPushButton("Configure");
	input_b->setMaximumWidth(100);
	config_b->setMaximumWidth(100);

	QHBoxLayout* input_b_layout = new QHBoxLayout;
	input_b_layout->addWidget(input_b_label, 0, Qt::AlignLeft);
	input_b_layout->addWidget(input_b, 0, Qt::AlignLeft);
	input_b_layout->addWidget(config_b, 0, Qt::AlignLeft);
	input_b_layout->setContentsMargins(6, 0, 0, 0);
	input_b_set->setLayout(input_b_layout);

	//Control settings - START button
	input_start_set = new QWidget(controls);
	QLabel* input_start_label = new QLabel("START : ");
	input_start = new QLineEdit(controls);
	config_start = new QPushButton("Configure");
	input_start->setMaximumWidth(100);
	config_start->setMaximumWidth(100);

	QHBoxLayout* input_start_layout = new QHBoxLayout;
	input_start_layout->addWidget(input_start_label, 0, Qt::AlignLeft);
	input_start_layout->addWidget(input_start, 0, Qt::AlignLeft);
	input_start_layout->addWidget(config_start, 0, Qt::AlignLeft);
	input_start_layout->setContentsMargins(6, 0, 0, 0);
	input_start_set->setLayout(input_start_layout);

	//Control settings - SELECT button
	input_select_set = new QWidget(controls);
	QLabel* input_select_label = new QLabel("SELECT : ");
	input_select = new QLineEdit(controls);
	config_select = new QPushButton("Configure");
	input_select->setMaximumWidth(100);
	config_select->setMaximumWidth(100);

	QHBoxLayout* input_select_layout = new QHBoxLayout;
	input_select_layout->addWidget(input_select_label, 0, Qt::AlignLeft);
	input_select_layout->addWidget(input_select, 0, Qt::AlignLeft);
	input_select_layout->addWidget(config_select, 0, Qt::AlignLeft);
	input_select_layout->setContentsMargins(6, 0, 0, 0);
	input_select_set->setLayout(input_select_layout);

	//Control settings - Left
	input_left_set = new QWidget(controls);
	QLabel* input_left_label = new QLabel("LEFT : ");
	input_left = new QLineEdit(controls);
	config_left = new QPushButton("Configure");
	input_left->setMaximumWidth(100);
	config_left->setMaximumWidth(100);

	QHBoxLayout* input_left_layout = new QHBoxLayout;
	input_left_layout->addWidget(input_left_label, 0, Qt::AlignLeft);
	input_left_layout->addWidget(input_left, 0, Qt::AlignLeft);
	input_left_layout->addWidget(config_left, 0, Qt::AlignLeft);
	input_left_layout->setContentsMargins(6, 0, 0, 0);
	input_left_set->setLayout(input_left_layout);

	//Control settings - Right
	input_right_set = new QWidget(controls);
	QLabel* input_right_label = new QLabel("RIGHT : ");
	input_right = new QLineEdit(controls);
	config_right = new QPushButton("Configure");
	input_right->setMaximumWidth(100);
	config_right->setMaximumWidth(100);

	QHBoxLayout* input_right_layout = new QHBoxLayout;
	input_right_layout->addWidget(input_right_label, 0, Qt::AlignLeft);
	input_right_layout->addWidget(input_right, 0, Qt::AlignLeft);
	input_right_layout->addWidget(config_right, 0, Qt::AlignLeft);
	input_right_layout->setContentsMargins(6, 0, 0, 0);
	input_right_set->setLayout(input_right_layout);

	//Control settings - Up
	input_up_set = new QWidget(controls);
	QLabel* input_up_label = new QLabel("UP : ");
	input_up = new QLineEdit(controls);
	config_up = new QPushButton("Configure");
	input_up->setMaximumWidth(100);
	config_up->setMaximumWidth(100);

	QHBoxLayout* input_up_layout = new QHBoxLayout;
	input_up_layout->addWidget(input_up_label, 0, Qt::AlignLeft);
	input_up_layout->addWidget(input_up, 0, Qt::AlignLeft);
	input_up_layout->addWidget(config_up, 0, Qt::AlignLeft);
	input_up_layout->setContentsMargins(6, 0, 0, 0);
	input_up_set->setLayout(input_up_layout);

	//Control settings - Down
	input_down_set = new QWidget(controls);
	QLabel* input_down_label = new QLabel("DOWN : ");
	input_down = new QLineEdit(controls);
	config_down = new QPushButton("Configure");
	input_down->setMaximumWidth(100);
	config_down->setMaximumWidth(100);

	QHBoxLayout* input_down_layout = new QHBoxLayout;
	input_down_layout->addWidget(input_down_label, 0, Qt::AlignLeft);
	input_down_layout->addWidget(input_down, 0, Qt::AlignLeft);
	input_down_layout->addWidget(config_down, 0, Qt::AlignLeft);
	input_down_layout->setContentsMargins(6, 0, 0, 0);
	input_down_set->setLayout(input_down_layout);

	//Control settings - Right Trigger
	input_r_set = new QWidget(controls);
	QLabel* input_r_label = new QLabel("Trigger R : ");
	input_r = new QLineEdit(controls);
	config_r = new QPushButton("Configure");
	input_r->setMaximumWidth(100);
	config_r->setMaximumWidth(100);

	QHBoxLayout* input_r_layout = new QHBoxLayout;
	input_r_layout->addWidget(input_r_label, 0, Qt::AlignLeft);
	input_r_layout->addWidget(input_r, 0, Qt::AlignLeft);
	input_r_layout->addWidget(config_r, 0, Qt::AlignLeft);
	input_r_layout->setContentsMargins(6, 0, 0, 0);
	input_r_set->setLayout(input_r_layout);

	//Control settings - Left Trigger
	input_l_set = new QWidget(controls);
	QLabel* input_l_label = new QLabel("Trigger L : ");
	input_l = new QLineEdit(controls);
	config_l = new QPushButton("Configure");
	input_l->setMaximumWidth(100);
	config_l->setMaximumWidth(100);

	QHBoxLayout* input_l_layout = new QHBoxLayout;
	input_l_layout->addWidget(input_l_label, 0, Qt::AlignLeft);
	input_l_layout->addWidget(input_l, 0, Qt::AlignLeft);
	input_l_layout->addWidget(config_l, 0, Qt::AlignLeft);
	input_l_layout->setContentsMargins(6, 0, 0, 0);
	input_l_set->setLayout(input_l_layout);

	//Advanced control settings - Enable rumble
	rumble_set = new QWidget(controls);
	QLabel* rumble_label = new QLabel("Enable rumble", rumble_set);
	rumble_on = new QCheckBox(rumble_set);

	QHBoxLayout* rumble_layout = new QHBoxLayout;
	rumble_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	rumble_layout->addWidget(rumble_on);
	rumble_layout->addWidget(rumble_label);
	rumble_set->setLayout(rumble_layout);

	controls_layout = new QVBoxLayout;
	controls_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	controls_layout->addWidget(input_device_set);
	controls_layout->addWidget(input_a_set);
	controls_layout->addWidget(input_b_set);
	controls_layout->addWidget(input_start_set);
	controls_layout->addWidget(input_select_set);
	controls_layout->addWidget(input_left_set);
	controls_layout->addWidget(input_right_set);
	controls_layout->addWidget(input_up_set);
	controls_layout->addWidget(input_down_set);
	controls_layout->addWidget(input_l_set);
	controls_layout->addWidget(input_r_set);
	controls_layout->addWidget(dead_zone_set);
	controls->setLayout(controls_layout);

	advanced_controls_layout = new QVBoxLayout;
	advanced_controls_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	advanced_controls_layout->addWidget(rumble_set);
	
	rumble_set->setVisible(false);

	//Path settings - DMG BIOS
	QWidget* dmg_bios_set = new QWidget(paths);
	dmg_bios_label = new QLabel("DMG Boot ROM :  ");
	QPushButton* dmg_bios_button = new QPushButton("Browse");
	dmg_bios = new QLineEdit(paths);
	dmg_bios->setReadOnly(true);

	QHBoxLayout* dmg_bios_layout = new QHBoxLayout;
	dmg_bios_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	dmg_bios_layout->addWidget(dmg_bios_label);
	dmg_bios_layout->addWidget(dmg_bios);
	dmg_bios_layout->addWidget(dmg_bios_button);
	dmg_bios_set->setLayout(dmg_bios_layout);
	dmg_bios_label->resize(50, dmg_bios_label->height());

	//Path settings - GBC BIOS
	QWidget* gbc_bios_set = new QWidget(paths);
	gbc_bios_label = new QLabel("GBC Boot ROM :  ");
	QPushButton* gbc_bios_button = new QPushButton("Browse");
	gbc_bios = new QLineEdit(paths);
	gbc_bios->setReadOnly(true);

	QHBoxLayout* gbc_bios_layout = new QHBoxLayout;
	gbc_bios_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	gbc_bios_layout->addWidget(gbc_bios_label);
	gbc_bios_layout->addWidget(gbc_bios);
	gbc_bios_layout->addWidget(gbc_bios_button);
	gbc_bios_set->setLayout(gbc_bios_layout);

	//Path settings - GBA BIOS
	QWidget* gba_bios_set = new QWidget(paths);
	gba_bios_label = new QLabel("GBA BIOS :  ");
	QPushButton* gba_bios_button = new QPushButton("Browse");
	gba_bios = new QLineEdit(paths);
	gba_bios->setReadOnly(true);

	QHBoxLayout* gba_bios_layout = new QHBoxLayout;
	gba_bios_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	gba_bios_layout->addWidget(gba_bios_label);
	gba_bios_layout->addWidget(gba_bios);
	gba_bios_layout->addWidget(gba_bios_button);
	gba_bios_set->setLayout(gba_bios_layout);

	//Path settings - CGFX Manifest
	QWidget* manifest_set = new QWidget(paths);
	manifest_label = new QLabel("CGFX Manifest :  ");
	QPushButton* manifest_button = new QPushButton("Browse");
	manifest = new QLineEdit(paths);
	manifest->setReadOnly(true);

	QHBoxLayout* manifest_layout = new QHBoxLayout;
	manifest_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	manifest_layout->addWidget(manifest_label);
	manifest_layout->addWidget(manifest);
	manifest_layout->addWidget(manifest_button);
	manifest_set->setLayout(manifest_layout);

	//Path settings - CGFX BG Tile Dump Folder
	QWidget* dump_bg_set = new QWidget(paths);
	dump_bg_label = new QLabel("BG Dump :  ");
	QPushButton* dump_bg_button = new QPushButton("Browse");
	dump_bg = new QLineEdit(paths);
	dump_bg->setReadOnly(true);

	QHBoxLayout* dump_bg_layout = new QHBoxLayout;
	dump_bg_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	dump_bg_layout->addWidget(dump_bg_label);
	dump_bg_layout->addWidget(dump_bg);
	dump_bg_layout->addWidget(dump_bg_button);
	dump_bg_set->setLayout(dump_bg_layout);
	dump_bg_label->resize(50, dump_bg_label->height());

	//Path settings - CGFX OBJ Tile Dump Folder
	QWidget* dump_obj_set = new QWidget(paths);
	dump_obj_label = new QLabel("OBJ Dump :  ");
	QPushButton* dump_obj_button = new QPushButton("Browse");
	dump_obj = new QLineEdit(paths);
	dump_obj->setReadOnly(true);

	QHBoxLayout* dump_obj_layout = new QHBoxLayout;
	dump_obj_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	dump_obj_layout->addWidget(dump_obj_label);
	dump_obj_layout->addWidget(dump_obj);
	dump_obj_layout->addWidget(dump_obj_button);
	dump_obj_set->setLayout(dump_obj_layout);
	dump_obj_label->resize(50, dump_obj_label->height());

	//Path settings - Screenshot
	QWidget* screenshot_set = new QWidget(paths);
	screenshot_label = new QLabel("Screenshots :  ");
	QPushButton* screenshot_button = new QPushButton("Browse");
	screenshot = new QLineEdit(paths);
	screenshot->setReadOnly(true);

	QHBoxLayout* screenshot_layout = new QHBoxLayout;
	screenshot_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	screenshot_layout->addWidget(screenshot_label);
	screenshot_layout->addWidget(screenshot);
	screenshot_layout->addWidget(screenshot_button);
	screenshot_set->setLayout(screenshot_layout);

	QVBoxLayout* paths_layout = new QVBoxLayout;
	paths_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	paths_layout->addWidget(dmg_bios_set);
	paths_layout->addWidget(gbc_bios_set);
	paths_layout->addWidget(gba_bios_set);
	paths_layout->addWidget(manifest_set);
	paths_layout->addWidget(dump_bg_set);
	paths_layout->addWidget(dump_obj_set);
	paths_layout->addWidget(screenshot_set);
	paths->setLayout(paths_layout);

	data_folder = new data_dialog;

	connect(tabs, SIGNAL(currentChanged(int)), this, SLOT(close_input()));
	connect(tabs_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(tabs_button, SIGNAL(rejected()), this, SLOT(reject()));
	connect(tabs_button->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(close_input()));
	connect(bios, SIGNAL(stateChanged(int)), this, SLOT(set_bios()));
	connect(screen_scale, SIGNAL(currentIndexChanged(int)), this, SLOT(screen_scale_change()));
	connect(aspect_ratio, SIGNAL(stateChanged(int)), this, SLOT(aspect_ratio_change()));
	connect(dmg_gbc_pal, SIGNAL(currentIndexChanged(int)), this, SLOT(dmg_gbc_pal_change()));
	connect(load_cgfx, SIGNAL(stateChanged(int)), this, SLOT(set_cgfx()));
	connect(volume, SIGNAL(valueChanged(int)), this, SLOT(volume_change()));
	connect(freq, SIGNAL(currentIndexChanged(int)), this, SLOT(sample_rate_change()));
	connect(sound_on, SIGNAL(stateChanged(int)), this, SLOT(mute()));
	connect(dead_zone, SIGNAL(valueChanged(int)), this, SLOT(dead_zone_change()));
	connect(input_device, SIGNAL(currentIndexChanged(int)), this, SLOT(input_device_change()));
	connect(advanced_button, SIGNAL(clicked()), this, SLOT(switch_control_layout()));
	connect(data_folder, SIGNAL(accepted()), this, SLOT(select_folder()));
	connect(data_folder, SIGNAL(rejected()), this, SLOT(reject_folder()));

	QSignalMapper* paths_mapper = new QSignalMapper(this);
	connect(dmg_bios_button, SIGNAL(clicked()), paths_mapper, SLOT(map()));
	connect(gbc_bios_button, SIGNAL(clicked()), paths_mapper, SLOT(map()));
	connect(gba_bios_button, SIGNAL(clicked()), paths_mapper, SLOT(map()));
	connect(manifest_button, SIGNAL(clicked()), paths_mapper, SLOT(map()));
	connect(dump_bg_button, SIGNAL(clicked()), paths_mapper, SLOT(map()));
	connect(dump_obj_button, SIGNAL(clicked()), paths_mapper, SLOT(map()));
	connect(screenshot_button, SIGNAL(clicked()), paths_mapper, SLOT(map()));

	paths_mapper->setMapping(dmg_bios_button, 0);
	paths_mapper->setMapping(gbc_bios_button, 1);
	paths_mapper->setMapping(gba_bios_button, 2);
	paths_mapper->setMapping(manifest_button, 3);
	paths_mapper->setMapping(screenshot_button, 4);
	paths_mapper->setMapping(dump_bg_button, 5);
	paths_mapper->setMapping(dump_obj_button, 6);
	connect(paths_mapper, SIGNAL(mapped(int)), this, SLOT(set_paths(int)));

	QSignalMapper* button_config = new QSignalMapper(this);
	connect(config_a, SIGNAL(clicked()), button_config, SLOT(map()));
	connect(config_b, SIGNAL(clicked()), button_config, SLOT(map()));
	connect(config_start, SIGNAL(clicked()), button_config, SLOT(map()));
	connect(config_select, SIGNAL(clicked()), button_config, SLOT(map()));
	connect(config_left, SIGNAL(clicked()), button_config, SLOT(map()));
	connect(config_right, SIGNAL(clicked()), button_config, SLOT(map()));
	connect(config_up, SIGNAL(clicked()), button_config, SLOT(map()));
	connect(config_down, SIGNAL(clicked()), button_config, SLOT(map()));
	connect(config_l, SIGNAL(clicked()), button_config, SLOT(map()));
	connect(config_r, SIGNAL(clicked()), button_config, SLOT(map()));

	button_config->setMapping(config_a, 0);
	button_config->setMapping(config_b, 1);
	button_config->setMapping(config_start, 2);
	button_config->setMapping(config_select, 3);
	button_config->setMapping(config_left, 4);
	button_config->setMapping(config_right, 5);
	button_config->setMapping(config_up, 6);
	button_config->setMapping(config_down, 7);
	button_config->setMapping(config_l, 8);
	button_config->setMapping(config_r, 9);
	connect(button_config, SIGNAL(mapped(int)), this, SLOT(configure_button(int))) ;

	//Final tab layout
	QVBoxLayout* main_layout = new QVBoxLayout;
	main_layout->addWidget(tabs);
	main_layout->addWidget(tabs_button);
	setLayout(main_layout);

	//Config button formatting
	config_a->setMinimumWidth(150);
	config_b->setMinimumWidth(150);
	config_start->setMinimumWidth(150);
	config_select->setMinimumWidth(150);
	config_left->setMinimumWidth(150);
	config_right->setMinimumWidth(150);
	config_up->setMinimumWidth(150);
	config_down->setMinimumWidth(150);
	config_l->setMinimumWidth(150);
	config_r->setMinimumWidth(150);

	input_a->setReadOnly(true);
	input_b->setReadOnly(true);
	input_start->setReadOnly(true);
	input_select->setReadOnly(true);
	input_left->setReadOnly(true);
	input_right->setReadOnly(true);
	input_up->setReadOnly(true);
	input_down->setReadOnly(true);
	input_l->setReadOnly(true);
	input_r->setReadOnly(true);

	//Install event filters
	config_a->installEventFilter(this);
	config_b->installEventFilter(this);
	config_start->installEventFilter(this);
	config_select->installEventFilter(this);
	config_left->installEventFilter(this);
	config_right->installEventFilter(this);
	config_up->installEventFilter(this);
	config_down->installEventFilter(this);
	config_l->installEventFilter(this);
	config_r->installEventFilter(this);

	input_a->installEventFilter(this);
	input_b->installEventFilter(this);
	input_start->installEventFilter(this);
	input_select->installEventFilter(this);
	input_left->installEventFilter(this);
	input_right->installEventFilter(this);
	input_up->installEventFilter(this);
	input_down->installEventFilter(this);
	input_l->installEventFilter(this);
	input_r->installEventFilter(this);

	//Set focus policies
	config_a->setFocusPolicy(Qt::NoFocus);
	config_b->setFocusPolicy(Qt::NoFocus);
	config_start->setFocusPolicy(Qt::NoFocus);
	config_select->setFocusPolicy(Qt::NoFocus);
	config_left->setFocusPolicy(Qt::NoFocus);
	config_right->setFocusPolicy(Qt::NoFocus);
	config_up->setFocusPolicy(Qt::NoFocus);
	config_down->setFocusPolicy(Qt::NoFocus);
	config_l->setFocusPolicy(Qt::NoFocus);
	config_r->setFocusPolicy(Qt::NoFocus);

	input_a->setFocusPolicy(Qt::NoFocus);
	input_b->setFocusPolicy(Qt::NoFocus);
	input_start->setFocusPolicy(Qt::NoFocus);
	input_select->setFocusPolicy(Qt::NoFocus);
	input_left->setFocusPolicy(Qt::NoFocus);
	input_right->setFocusPolicy(Qt::NoFocus);
	input_up->setFocusPolicy(Qt::NoFocus);
	input_down->setFocusPolicy(Qt::NoFocus);
	input_l->setFocusPolicy(Qt::NoFocus);
	input_r->setFocusPolicy(Qt::NoFocus);

	//Joystick handling
	jstick = SDL_JoystickOpen(0);
	
	for(int x = 0; x < SDL_NumJoysticks(); x++)
	{
		SDL_Joystick* jstick = SDL_JoystickOpen(x);
		std::string joy_name = SDL_JoystickName(jstick);
		input_device->addItem(QString::fromStdString(joy_name));
	}

	sample_rate = config::sample_rate;
	resize_screen = false;
	grab_input = false;
	config_advanced_controls = false;
	input_type = 0;

	resize(450, 450);
	setWindowTitle(tr("GBE+ Settings"));
}

/****** Sets various widgets to values based on the current config paramters (from .ini file) ******/
void gen_settings::set_ini_options()
{
	//Emulated system type
	sys_type->setCurrentIndex(config::gb_type);

	//BIOS or Boot ROM option
	if(config::use_bios) { bios->setChecked(true); }

	//Screen scale options
	screen_scale->setCurrentIndex(config::scaling_factor - 1);

	//CGFX scale options
	cgfx_scale->setCurrentIndex(cgfx::scaling_factor - 1);

	//DMG-on-GBC palette options
	dmg_gbc_pal->setCurrentIndex(config::dmg_gbc_pal);

	//OpenGL option
	if(config::use_opengl) { ogl->setChecked(true); }

	//CGFX option
	if(cgfx::load_cgfx) { load_cgfx->setChecked(true); }

	//Maintain aspect ratio option
	if(config::maintain_aspect_ratio) { aspect_ratio->setChecked(true); }

	//Sample rate option
	switch((int)config::sample_rate)
	{
		case 11025: freq->setCurrentIndex(3); break;
		case 22050: freq->setCurrentIndex(2); break;
		case 44100: freq->setCurrentIndex(1); break;
		case 48000: freq->setCurrentIndex(0); break;
	}

	//Volume option
	volume->setValue(config::volume);

	//Mute option
	if(config::mute == 1)
	{
		config::volume = 0;
		sound_on->setChecked(false);
		volume->setEnabled(false);
	}

	else
	{
		sound_on->setChecked(true);
		volume->setEnabled(true);
	}

	//Dead-zone
	dead_zone->setValue(config::dead_zone);

	//Keyboard controls
	input_a->setText(QString::number(config::agb_key_a));
	input_b->setText(QString::number(config::agb_key_b));
	input_start->setText(QString::number(config::agb_key_start));
	input_select->setText(QString::number(config::agb_key_select));
	input_left->setText(QString::number(config::agb_key_left));
	input_right->setText(QString::number(config::agb_key_right));
	input_up->setText(QString::number(config::agb_key_up));
	input_down->setText(QString::number(config::agb_key_down));
	input_l->setText(QString::number(config::agb_key_l_trigger));
	input_r->setText(QString::number(config::agb_key_r_trigger));

	//BIOS, Boot ROM and Manifest paths
	QString path_1(QString::fromStdString(config::dmg_bios_path));
	QString path_2(QString::fromStdString(config::gbc_bios_path));
	QString path_3(QString::fromStdString(config::agb_bios_path));
	QString path_4(QString::fromStdString(cgfx::manifest_file));
	QString path_5(QString::fromStdString(config::ss_path));
	QString path_6(QString::fromStdString(cgfx::dump_bg_path));
	QString path_7(QString::fromStdString(cgfx::dump_obj_path));

	//Rumble
	if(config::use_haptics) { rumble_on->setChecked(true); }

	dmg_bios->setText(path_1);
	gbc_bios->setText(path_2);
	gba_bios->setText(path_3);
	manifest->setText(path_4);
	screenshot->setText(path_5);
	dump_bg->setText(path_6);
	dump_obj->setText(path_7);
}

/****** Toggles whether to use the Boot ROM or BIOS ******/
void gen_settings::set_bios()
{
	if(bios->isChecked()) { config::use_bios = true; }
	else { config::use_bios = false; }
}

/****** Changes the display scale ******/
void gen_settings::screen_scale_change()
{
	config::scaling_factor = (screen_scale->currentIndex() + 1);
	resize_screen = true;
}

/****** Sets whether to maintain the aspect ratio or not ******/
void gen_settings::aspect_ratio_change()
{
	if(aspect_ratio->isChecked()) { config::maintain_aspect_ratio = true; }
	else { config::maintain_aspect_ratio = false; }
}

/****** Changes the emulated DMG-on-GBC palette ******/
void gen_settings::dmg_gbc_pal_change()
{
	config::dmg_gbc_pal = (dmg_gbc_pal->currentIndex());
	set_dmg_colors(config::dmg_gbc_pal);
}

/****** Toggles activation of custom graphics ******/
void gen_settings::set_cgfx()
{
	if(load_cgfx->isChecked()) { cgfx::load_cgfx = true; }
	else { cgfx::load_cgfx = false; }
}

/****** Dynamically changes the core's volume ******/
void gen_settings::volume_change() 
{
	//Update volume while playing
	if((main_menu::gbe_plus != NULL) && (sound_on->isChecked()))
	{
		main_menu::gbe_plus->update_volume(volume->value());
	}

	//Update the volume while using only the GUI
	else { config::volume = volume->value(); }	
}

/****** Updates the core's volume - Used when loading save states ******/
void gen_settings::update_volume() { mute(); }

/****** Mutes the core's volume ******/
void gen_settings::mute()
{
	//Mute/unmute while playing
	if(main_menu::gbe_plus != NULL)
	{
		//Unmute, use slider volume
		if(sound_on->isChecked())
		{
			main_menu::gbe_plus->update_volume(volume->value());
			volume->setEnabled(true);
		}

		//Mute
		else
		{
			main_menu::gbe_plus->update_volume(0);
			volume->setEnabled(false);
		}
	}

	//Mute/unmute while using only the GUI
	else
	{
		//Unmute, use slider volume
		if(sound_on->isChecked())
		{
			config::volume = volume->value();
			volume->setEnabled(true);
		}

		//Mute
		else
		{
			config::volume = 0;
			volume->setEnabled(false);
		}
	}
}

/****** Changes the core's sample rate ******/
void gen_settings::sample_rate_change()
{
	switch(freq->currentIndex())
	{
		case 0: sample_rate = 48000.0; break;
		case 1: sample_rate = 44100.0; break;
		case 2: sample_rate = 22050.0; break;
		case 3: sample_rate = 11025.0; break;
	}
}

/****** Sets a path via file browser ******/
void gen_settings::set_paths(int index)
{
	QString path;

	//Open file browser for Boot ROMs, BIOS, and manifests
	if(index < 4) 
	{
		path = QFileDialog::getOpenFileName(this, tr("Open"), "", tr("All files (*)"));
		if(path.isNull()) { return; }
	}

	//Open folder browser for screenshots, CGFX dumps
	else
	{
		//Open the data folder for CGFX dumps
		//On Linux or Unix, this is supposed to be a hidden folder, so we need a custom dialog
		if(index >= 5)
		{
			data_folder->open_data_folder();			

			while(!data_folder->finish) { QApplication::processEvents(); }
	
			path = data_folder->directory().path();
			path = data_folder->path.relativeFilePath(path);
		}

		else { path = QFileDialog::getExistingDirectory(this, tr("Open"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks); }
		
		if(path.isNull()) { return; }	

		//Make sure path is complete, e.g. has the correct separator at the end
		//Qt doesn't append this automatically
		std::string temp_str = path.toStdString();
		std::string temp_chr = "";
		temp_chr = temp_str[temp_str.length() - 1];

		if((temp_chr != "/") && (temp_chr != "\\")) { path.append("/"); }
		path = QDir::toNativeSeparators(path);
	}

	switch(index)
	{
		case 0: 
			config::dmg_bios_path = path.toStdString();
			dmg_bios->setText(path);
			break;

		case 1:
			config::gbc_bios_path = path.toStdString();
			gbc_bios->setText(path);
			break;

		case 2:
			config::agb_bios_path = path.toStdString();
			gba_bios->setText(path);
			break;

		case 3:
			cgfx::manifest_file = path.toStdString();
			manifest->setText(path);
			break;

		case 4:
			config::ss_path = path.toStdString();
			screenshot->setText(path);
			break;

		case 5:
			cgfx::dump_bg_path = path.toStdString();
			dump_bg->setText(path);
			break;

		case 6:
			cgfx::dump_obj_path = path.toStdString();
			dump_obj->setText(path);
			break;
	}
}

/****** Changes the input device to configure ******/
void gen_settings::input_device_change()
{
	input_type = input_device->currentIndex();

	//Switch to keyboard input configuration
	if(input_type == 0)
	{
		input_a->setText(QString::number(config::agb_key_a));
		input_b->setText(QString::number(config::agb_key_b));
		input_start->setText(QString::number(config::agb_key_start));
		input_select->setText(QString::number(config::agb_key_select));
		input_left->setText(QString::number(config::agb_key_left));
		input_right->setText(QString::number(config::agb_key_right));
		input_up->setText(QString::number(config::agb_key_up));
		input_down->setText(QString::number(config::agb_key_down));
		input_l->setText(QString::number(config::agb_key_l_trigger));
		input_r->setText(QString::number(config::agb_key_r_trigger));
	}

	else
	{
		input_a->setText(QString::number(config::agb_joy_a));
		input_b->setText(QString::number(config::agb_joy_b));
		input_start->setText(QString::number(config::agb_joy_start));
		input_select->setText(QString::number(config::agb_joy_select));
		input_left->setText(QString::number(config::agb_joy_left));
		input_right->setText(QString::number(config::agb_joy_right));
		input_up->setText(QString::number(config::agb_joy_up));
		input_down->setText(QString::number(config::agb_joy_down));
		input_l->setText(QString::number(config::agb_joy_l_trigger));
		input_r->setText(QString::number(config::agb_joy_r_trigger));

		//Use new joystick id
		config::joy_id = input_device->currentIndex() - 1;
		jstick = SDL_JoystickOpen(config::joy_id);
	}

	close_input();
}

/****** Dynamically changes the core pad's dead-zone ******/
void gen_settings::dead_zone_change() { config::dead_zone = dead_zone->value(); }	

/****** Prepares GUI to receive input for controller configuration ******/
void gen_settings::configure_button(int button)
{
	if(grab_input) { return; }

	grab_input = true;

	switch(button)
	{
		case 0: 
			input_delay(config_a);
			input_a->setFocus();
			input_index = 0;
			break;

		case 1: 
			input_delay(config_b);
			input_b->setFocus();
			input_index = 1;
			break;

		case 2: 
			input_delay(config_start);
			input_start->setFocus();
			input_index = 2;
			break;

		case 3: 
			input_delay(config_select);
			input_select->setFocus();
			input_index = 3;
			break;

		case 4: 
			input_delay(config_left);
			input_left->setFocus();
			input_index = 4;
			break;

		case 5: 
			input_delay(config_right);
			input_right->setFocus();
			input_index = 5;
			break;

		case 6: 
			input_delay(config_up);
			input_up->setFocus();
			input_index = 6;
			break;

		case 7: 
			input_delay(config_down);
			input_down->setFocus();
			input_index = 7;
			break;

		case 8: 
			input_delay(config_l);
			input_l->setFocus();
			input_index = 8;
			break;

		case 9: 
			input_delay(config_r);
			input_r->setFocus();
			input_index = 9;
			break;
	}

	if(input_type != 0) { process_joystick_event(); }
}			

/****** Delays input from the GUI ******/
void gen_settings::input_delay(QPushButton* input_button)
{
	//Delay input for joysticks
	if(input_type != 0)
	{
		input_button->setText("Enter Input - 3");
		QApplication::processEvents();
		input_button->setText("Enter Input - 3");
		input_button->update();
		QApplication::processEvents();
		SDL_Delay(1000);

		input_button->setText("Enter Input - 2");
		QApplication::processEvents();
		input_button->setText("Enter Input - 2");
		input_button->update();
		QApplication::processEvents();
		SDL_Delay(1000);

		input_button->setText("Enter Input - 1");
		QApplication::processEvents();
		input_button->setText("Enter Input - 1");
		input_button->update();
		QApplication::processEvents();
		SDL_Delay(1000);
	}

	//Grab input immediately for keyboards
	else { input_button->setText("Enter Input"); }	
}

/****** Handles joystick input ******/
void gen_settings::process_joystick_event()
{
	SDL_Event joy_event;
	int pad = 0;

	while(SDL_PollEvent(&joy_event))
	{
		//Generate pad id
		switch(joy_event.type)
		{
			case SDL_JOYBUTTONDOWN:
				pad = 100 + joy_event.jbutton.button; 
				grab_input = false;
				break;

			case SDL_JOYAXISMOTION:
				if(abs(joy_event.jaxis.value) >= config::dead_zone)
				{
					pad = 200 + (joy_event.jaxis.axis * 2);
					if(joy_event.jaxis.value > 0) { pad++; }
					grab_input = false;
				}

				break;

			case SDL_JOYHATMOTION:
				pad = 300 + (joy_event.jhat.hat * 4);
				grab_input = false;
						
				switch(joy_event.jhat.value)
				{
					case SDL_HAT_RIGHT: pad += 1; break;
					case SDL_HAT_UP: pad += 2; break;
					case SDL_HAT_DOWN: pad += 3; break;
				}

				break;
		}
	}

	switch(input_index)
	{
		case 0:
			if(pad != 0)
			{
				config::agb_joy_a = config::dmg_joy_a = pad;
				input_a->setText(QString::number(pad));
			}

			config_a->setText("Configure");
			input_a->clearFocus();
			break;

		case 1:
			if(pad != 0)
			{
				config::agb_joy_b = config::dmg_joy_b = pad;
				input_b->setText(QString::number(pad));
			}

			config_b->setText("Configure");
			input_b->clearFocus();
			break;

		case 2:
			if(pad != 0)
			{
				config::agb_joy_start = config::dmg_joy_start = pad;
				input_start->setText(QString::number(pad));
			}

			config_start->setText("Configure");
			input_start->clearFocus();
			break;

		case 3:
			if(pad != 0)
			{
				config::agb_joy_select = config::dmg_joy_select = pad;
				input_select->setText(QString::number(pad));
			}

			config_select->setText("Configure");
			input_select->clearFocus();
			break;

		case 4:
			if(pad != 0)
			{
				config::agb_joy_left = config::dmg_joy_left = pad;
				input_left->setText(QString::number(pad));
			}

			config_left->setText("Configure");
			input_left->clearFocus();
			break;

		case 5:
			if(pad != 0)
			{
				config::agb_joy_right = config::dmg_joy_right = pad;
				input_right->setText(QString::number(pad));
			}

			config_right->setText("Configure");
			input_right->clearFocus();
			break;

		case 6:
			if(pad != 0)
			{
				config::agb_joy_up = config::dmg_joy_up = pad;
				input_up->setText(QString::number(pad));
			}

			config_up->setText("Configure");
			input_up->clearFocus();
			break;

		case 7:
			if(pad != 0)
			{
				config::agb_joy_down = config::dmg_joy_down = pad;
				input_down->setText(QString::number(pad));
			}

			config_down->setText("Configure");
			input_down->clearFocus();
			break;

		case 8: 
			if(pad != 0)
			{
				config::agb_joy_l_trigger = pad;
				input_l->setText(QString::number(pad));
			}

			config_l->setText("Configure");
			input_l->clearFocus();
			break;

		case 9:
			if(pad != 0)
			{
				config::agb_joy_r_trigger = pad;
				input_r->setText(QString::number(pad));
			}

			config_r->setText("Configure");
			input_r->clearFocus();
			break;
	}

	input_index = -1;
	QApplication::processEvents();
	grab_input = false;
}

/****** Close all pending input configuration ******/
void gen_settings::close_input()
{
	config_a->setText("Configure");
	config_b->setText("Configure");
	config_start->setText("Configure");
	config_select->setText("Configure");
	config_left->setText("Configure");
	config_right->setText("Configure");
	config_up->setText("Configure");
	config_down->setText("Configure");
	config_l->setText("Configure");
	config_r->setText("Configure");

	input_index = -1;
	grab_input = false;

	//Additionally, set the Advanced Controls button visible or invisible when switching tabs
	if(tabs->currentIndex() == 3) { advanced_button->setVisible(true); }
	else { advanced_button->setVisible(false); }
}

/****** Changes Qt widget layout - Switches between advanced control configuration mode ******/
void gen_settings::switch_control_layout()
{
	//Switch from standard control layout to advanced control layout
	if(!config_advanced_controls)
	{
		//Set all advanced control widgets to visible
		for(int x = 0; x < advanced_controls_layout->count(); x++)
		{
			advanced_controls_layout->itemAt(x)->widget()->setVisible(true);
		}

		//Set all standard control widgets to invisible
		for(int x = 0; x < controls_layout->count(); x++)
		{
			controls_layout->itemAt(x)->widget()->setVisible(false);
		}

		config_advanced_controls = true;

		delete controls->layout();
		controls->setLayout(advanced_controls_layout);

		//Rebuild old layout (was deleted above)
		controls_layout = new QVBoxLayout;
		controls_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
		controls_layout->addWidget(input_device_set);
		controls_layout->addWidget(input_a_set);
		controls_layout->addWidget(input_b_set);
		controls_layout->addWidget(input_start_set);
		controls_layout->addWidget(input_select_set);
		controls_layout->addWidget(input_left_set);
		controls_layout->addWidget(input_right_set);
		controls_layout->addWidget(input_up_set);
		controls_layout->addWidget(input_down_set);
		controls_layout->addWidget(input_l_set);
		controls_layout->addWidget(input_r_set);
		controls_layout->addWidget(dead_zone_set);
	}

	//Switch from advanced control layout to standard control layout
	else
	{
		//Set all advanced control widgets to invisible
		for(int x = 0; x < advanced_controls_layout->count(); x++)
		{
			advanced_controls_layout->itemAt(x)->widget()->setVisible(false);
		}

		//Set all standard control widgets to invisible
		for(int x = 0; x < controls_layout->count(); x++)
		{
			controls_layout->itemAt(x)->widget()->setVisible(true);
		}

		config_advanced_controls = false;

		delete controls->layout();
		controls->setLayout(controls_layout);

		//Rebuild old layout (was deleted above)
		advanced_controls_layout = new QVBoxLayout;
		advanced_controls_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
		advanced_controls_layout->addWidget(rumble_set);
	}
}

/****** Updates the settings window ******/
void gen_settings::paintEvent(QPaintEvent* event)
{
	gbc_bios_label->setMinimumWidth(dmg_bios_label->width());
	gba_bios_label->setMinimumWidth(dmg_bios_label->width());
	manifest_label->setMinimumWidth(dmg_bios_label->width());
	dump_bg_label->setMinimumWidth(dmg_bios_label->width());
	dump_obj_label->setMinimumWidth(dmg_bios_label->width());
	screenshot_label->setMinimumWidth(dmg_bios_label->width());
}

/****** Closes the settings window ******/
void gen_settings::closeEvent(QCloseEvent* event)
{
	//Close any on-going input configuration
	close_input();
}

/****** Handle keypress input ******/
void gen_settings::keyPressEvent(QKeyEvent* event)
{
	if(grab_input)
	{
		last_key = qtkey_to_sdlkey(event->key());

		switch(input_index)
		{
			case 0:
				if(last_key != -1)
				{
					config::agb_key_a = config::dmg_key_a = last_key;
					input_a->setText(QString::number(last_key));
				}

				config_a->setText("Configure");
				input_a->clearFocus();
				break;

			case 1:
				if(last_key != -1)
				{
					config::agb_key_b = config::dmg_key_b = last_key;
					input_b->setText(QString::number(last_key));
				}

				config_b->setText("Configure");
				input_b->clearFocus();
				break;

			case 2:
				if(last_key != -1)
				{
					config::agb_key_start = config::dmg_key_start = last_key;
					input_start->setText(QString::number(last_key));
				}

				config_start->setText("Configure");
				input_start->clearFocus();
				break;

			case 3:
				if(last_key != -1)
				{
					config::agb_key_select = config::dmg_key_select = last_key;
					input_select->setText(QString::number(last_key));
				}

				config_select->setText("Configure");
				input_select->clearFocus();
				break;

			case 4:
				if(last_key != -1)
				{
					config::agb_key_left = config::dmg_key_left = last_key;
					input_left->setText(QString::number(last_key));
				}

				config_left->setText("Configure");
				input_left->clearFocus();
				break;

			case 5:
				if(last_key != -1)
				{
					config::agb_key_right = config::dmg_key_right = last_key;
					input_right->setText(QString::number(last_key));
				}

				config_right->setText("Configure");
				input_right->clearFocus();
				break;

			case 6:
				if(last_key != -1)
				{
					config::agb_key_up = config::dmg_key_up = last_key;
					input_up->setText(QString::number(last_key));
				}

				config_up->setText("Configure");
				input_up->clearFocus();
				break;

			case 7:
				if(last_key != -1)
				{
					config::agb_key_down = config::dmg_key_down = last_key;
					input_down->setText(QString::number(last_key));
				}

				config_down->setText("Configure");
				input_down->clearFocus();
				break;

			case 8: 
				if(last_key != -1)
				{
					config::agb_key_l_trigger = last_key;
					input_l->setText(QString::number(last_key));
				}

				config_l->setText("Configure");
				input_l->clearFocus();
				break;

			case 9:
				if(last_key != -1)
				{
					config::agb_key_r_trigger = last_key;
					input_r->setText(QString::number(last_key));
				}

				config_r->setText("Configure");
				input_r->clearFocus();
				break;
		}

		grab_input = false;
		input_index = -1;
	}
}

/****** Event filter for settings window ******/
bool gen_settings::eventFilter(QObject* target, QEvent* event)
{
	//Filter all keypresses, pass them along to input configuration if necessary
	if(event->type() == QEvent::KeyPress)
	{
		QKeyEvent* key_event = static_cast<QKeyEvent*>(event);
		keyPressEvent(key_event);
	}

	return QDialog::eventFilter(target, event);
}

/****** Selects folder ******/
void gen_settings::select_folder() { data_folder->finish = true; }

/****** Rejectss folder ******/
void gen_settings::reject_folder()
{
	data_folder->finish = true;
	data_folder->setDirectory(data_folder->last_path);
}
