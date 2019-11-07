// GB Enhanced+ Copyright Daniel Baxter 2018
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : zzh_menu.h
// Date : February 24, 2018
// Description : Zok Zok Heroes Full Changer menu
//
// Changes the cosmic character transmitted to the GBC

#include "zzh_menu.h"

#include "common/config.h"

/****** ZZH menu constructor ******/
zzh_menu::zzh_menu(QWidget *parent) : QDialog(parent)
{
	close_button = new QDialogButtonBox(QDialogButtonBox::Close);

	//Cosmic character drop down menu
	QWidget* cosmic_character_set = new QWidget;
	QLabel* cosmic_character_label = new QLabel("Cosmic Character : ");
 	
	cosmic_character = new QComboBox;
	cosmic_character->addItem("A - Alkaline Powered");
	cosmic_character->addItem("I - In Water");
	cosmic_character->addItem("U - Ultra Runner");
	cosmic_character->addItem("E - Aero Power");
	cosmic_character->addItem("O - Ochaapa");
	cosmic_character->addItem("Ka - Kaizer Edge");
	cosmic_character->addItem("Ki - King Batter");
	cosmic_character->addItem("Ku - Crash Car");
	cosmic_character->addItem("Ke - Cellphone Tiger");
	cosmic_character->addItem("Ko - Cup Ace");

	QHBoxLayout* cosmic_character_layout = new QHBoxLayout;
	cosmic_character_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	cosmic_character_layout->addWidget(cosmic_character_label);
	cosmic_character_layout->addWidget(cosmic_character);
	cosmic_character_set->setLayout(cosmic_character_layout);

	QVBoxLayout* final_layout = new QVBoxLayout;
	final_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	final_layout->addWidget(cosmic_character_set);
	final_layout->addWidget(close_button);
	setLayout(final_layout);

	setMinimumWidth(300);
	setWindowTitle(QString("Full Changer Configuration"));
	hide();

	connect(close_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(close_button, SIGNAL(rejected()), this, SLOT(reject()));
	connect(cosmic_character, SIGNAL(currentIndexChanged(int)), this, SLOT(update_cosmic_character()));
}

/****** Update amount of cosmic_character to transfer ******/
void zzh_menu::update_cosmic_character()
{
	config::ir_db_index = cosmic_character->currentIndex();
}
