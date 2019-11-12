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
	cosmic_character->addItem("Sa - Sakanard");
	cosmic_character->addItem("Shi - Thin Delta");
	cosmic_character->addItem("Su - Skateboard Rider");
	cosmic_character->addItem("Se - Celery Star");
	cosmic_character->addItem("So - Cleaning Killer");
	cosmic_character->addItem("Ta - Takoyaki Kid");
	cosmic_character->addItem("Chi - Chinkoman");
	cosmic_character->addItem("Tsu - Tsukai Stater");
	cosmic_character->addItem("Te - Teppangar");
	cosmic_character->addItem("To - Tongararin");
	cosmic_character->addItem("Na - Nagashiman");
	cosmic_character->addItem("Ni - Ninja");
	cosmic_character->addItem("Nu - Plushy-chan");
	cosmic_character->addItem("Ne - Screw Razor");
	cosmic_character->addItem("No - Nobel Brain");
	cosmic_character->addItem("Ha - Hard Hammer");
	cosmic_character->addItem("Hi - Heat Man");
	cosmic_character->addItem("Fu - Flame Gourmet");
	cosmic_character->addItem("He - Hercules Army");
	cosmic_character->addItem("Ho - Hot Card");
	cosmic_character->addItem("Ma - Mr. Muscle");
	cosmic_character->addItem("Mi - Mist Water");
	cosmic_character->addItem("Mu - Mushimushi Man");
	cosmic_character->addItem("Me - Megaaten");
	cosmic_character->addItem("Mo - Mobile Robot X");
	cosmic_character->addItem("Ya - Yaki Bird");
	cosmic_character->addItem("Yu - Utron");
	cosmic_character->addItem("Yo - Yo-Yo Mask");
	cosmic_character->addItem("Ra - Radial Road");
	cosmic_character->addItem("Ri - Remote-Control Man");
	cosmic_character->addItem("Ru - Ruby Hook");
	cosmic_character->addItem("Re - Retro Sounder");
	cosmic_character->addItem("Ro - Rocket");
	cosmic_character->addItem("Wa - Wild Sword");
	cosmic_character->addItem("Ga - Guts Lago");
	cosmic_character->addItem("Gi - Giniun");
	cosmic_character->addItem("Gu - Great Fire");
	cosmic_character->addItem("Ge - Gamemark");
	cosmic_character->addItem("Go - Gorilla Killa");
	cosmic_character->addItem("Za - The Climber");
	cosmic_character->addItem("Ji - G Shark");
	cosmic_character->addItem("Zu - Zoom Laser");
	cosmic_character->addItem("Ze - Zenmai");
	cosmic_character->addItem("Zo - Elephant Shower");
	cosmic_character->addItem("Da - Diamond Mall");
	cosmic_character->addItem("Di - Digronyan");
	cosmic_character->addItem("Dzu - Ziza One"); 
	cosmic_character->addItem("De - Danger Red");
	cosmic_character->addItem("Do - Dohatsuten");
	cosmic_character->addItem("Ba - Balloon");
	cosmic_character->addItem("Bi - Videoja");
	cosmic_character->addItem("Bu - Boo Boo");
	cosmic_character->addItem("Be - Belt Jain");
	cosmic_character->addItem("Bo - Boat Ron");
	cosmic_character->addItem("Pa - Perfect Sun");
	cosmic_character->addItem("Pi - Pinspawn");
	cosmic_character->addItem("Pu - Press Arm");
	cosmic_character->addItem("Pe - Pegasus Boy");
	cosmic_character->addItem("Po - Pop Thunder");
	cosmic_character->addItem("N - Ndjamenas");

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
