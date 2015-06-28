// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : general_settings.h
// Date : June 28, 2015
// Description : Main menu
//
// Dialog for various options
// Deals with Graphics, Audio, Input, Paths, etc

#include "general_settings.h"

/****** General settings constructor ******/
gen_settings::gen_settings(QWidget *parent) : QDialog(parent)
{
	//Set up tabs
	tabs = new QTabWidget(this);
	
	QDialog* general = new QDialog;
	QDialog* display = new QDialog;
	QDialog* sound = new QDialog;
	QDialog* controls = new QDialog;

	tabs->addTab(general, tr("General"));
	tabs->addTab(display, tr("Display"));
	tabs->addTab(sound, tr("Sound"));
	tabs->addTab(controls, tr("Controls"));

	tabs_button = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	//General settings - Emulated system type
	QWidget* sys_type_set = new QWidget(general);

	QLabel* sys_type_label = new QLabel("Emulated System Type : ", sys_type_set);
	sys_type = new QComboBox(sys_type_set);
	sys_type->addItem("Auto");
	sys_type->addItem("Game Boy [DMG]");
	sys_type->addItem("Game Boy Color [GBC]");
	sys_type->addItem("Game Boy Advance [GBA]");

	QHBoxLayout* sys_type_layout = new QHBoxLayout;
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

	QVBoxLayout* gen_layout = new QVBoxLayout;
	gen_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	gen_layout->addWidget(sys_type_set);
	gen_layout->addWidget(bios_set);
	general->setLayout(gen_layout);

	connect(tabs_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(tabs_button, SIGNAL(rejected()), this, SLOT(reject()));

	QVBoxLayout* main_layout = new QVBoxLayout;
	main_layout->addWidget(tabs);
	main_layout->addWidget(tabs_button);
	setLayout(main_layout);


	resize(450, 450);
	setWindowTitle(tr("GBE+ Settings"));
}
