// GB Enhanced+ Copyright Daniel Baxter 2019
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mpos_menu.h
// Date : July 28, 2019
// Description : Multi Plust On System menu
//
// Changes Pluster figurine

#include "mpos_menu.h"

#include "common/config.h"

/****** MPOS menu constructor ******/
mpos_menu::mpos_menu(QWidget *parent) : QDialog(parent)
{
	close_button = new QDialogButtonBox(QDialogButtonBox::Close);

	//Pluster drop down menu
	QWidget* pluster_set = new QWidget;
	QLabel* pluster_label = new QLabel("Pluster Figurine : ");
 	
	pluster = new QComboBox;
	pluster->addItem("PF001 - Beetma");
	pluster->addItem("PF002 - Wyburst");
	pluster->addItem("PF003 - Gabrian");
	pluster->addItem("PF004 - Molly");
	pluster->addItem("PF005 - Hania");
	pluster->addItem("PF006 - Zagarian");
	pluster->addItem("PF007 - Tan Q");
	pluster->addItem("PF008 - Warrion");
	pluster->addItem("PF009 - Doryuun");
	pluster->addItem("PF010 - Fezard");
	pluster->addItem("PF011 - Mashanta");
	pluster->addItem("PF012 - Gingardo");
	pluster->addItem("PF013 - Torastorm");
	pluster->addItem("PF014 - Gongoragon");
	pluster->addItem("PF015 - Mighty V");
	pluster->addItem("PF016 - Dorastorm");
	pluster->addItem("PF EX001 - Beetma EX");
	pluster->addItem("PF EX002 - Varouze");
	pluster->addItem("PF EX003 - Gigajoule");
	pluster->addItem("PF EX004 - Badnick");
	pluster->addItem("PF EX005 - Poseihorn");
	pluster->addItem("PF EX006 - Terra");
	
	QHBoxLayout* pluster_layout = new QHBoxLayout;
	pluster_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	pluster_layout->addWidget(pluster_label);
	pluster_layout->addWidget(pluster);
	pluster_set->setLayout(pluster_layout);

	QVBoxLayout* final_layout = new QVBoxLayout;
	final_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	final_layout->addWidget(pluster_set);
	final_layout->addWidget(close_button);
	setLayout(final_layout);

	setMinimumWidth(300);
	setWindowTitle(QString("Multi Plust On System Configuration"));
	setWindowIcon(QIcon(QString::fromStdString(config::cfg_path + "data/icons/gbe_plus.png")));
	hide();

	connect(close_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(close_button, SIGNAL(rejected()), this, SLOT(reject()));
	connect(pluster, SIGNAL(currentIndexChanged(int)), this, SLOT(update_pluster_figurine()));
}

/****** Update Pluster figurine ID ******/
void mpos_menu::update_pluster_figurine()
{
	switch(pluster->currentIndex())
	{
		case 0: config::mpos_id = 0x16C0; break;
		case 1: config::mpos_id = 0x16A0; break;
		case 2: config::mpos_id = 0x1630; break;
		case 3: config::mpos_id = 0x1672; break;
		case 4: config::mpos_id = 0x1628; break;
		case 5: config::mpos_id = 0x1614; break;
		case 6: config::mpos_id = 0x16D4; break;
		case 7: config::mpos_id = 0x1612; break;
		case 8: config::mpos_id = 0x1678; break;
		case 9: config::mpos_id = 0x1618; break;
		case 10: config::mpos_id = 0x1624; break;
		case 11: config::mpos_id = 0x1674; break;
		case 12: config::mpos_id = 0x16CC; break;
		case 13: config::mpos_id = 0x16AC; break;
		case 14: config::mpos_id = 0x169C; break;
		case 15: config::mpos_id = 0x16FC; break;
		case 16: config::mpos_id = 0x1666; break;
		case 17: config::mpos_id = 0x1636; break;
		case 18: config::mpos_id = 0x164E; break;
		case 19: config::mpos_id = 0x161E; break;
		case 20: config::mpos_id = 0x167E; break;
		case 21: config::mpos_id = 0x1621; break;
	}
}
