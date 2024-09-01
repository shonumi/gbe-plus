// GB Enhanced+ Copyright Daniel Baxter 2018
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : pp2_menu.cpp
// Date : February 21, 2018
// Description : Pokemon Pikachu 2 menu
//
// Change the amount of watts GBE+ will receive from Pikachu

#include "pp2_menu.h"

#include "common/config.h"

/****** PP2 menu constructor ******/
pp2_menu::pp2_menu(QWidget *parent) : QDialog(parent)
{
	close_button = new QDialogButtonBox(QDialogButtonBox::Close);

	//Watts drop down menu
	QWidget* watts_set = new QWidget;
	QLabel* watts_label = new QLabel("Number of Watts : ");
 	
	watts = new QComboBox;
	watts->addItem("1W - Pokemon Crystal");
	watts->addItem("100W - Pokemon Crystal");
	watts->addItem("200W - Pokemon Crystal");
	watts->addItem("300W - Pokemon Crystal");
	watts->addItem("400W - Pokemon Crystal");
	watts->addItem("500W - Pokemon Crystal");

	QHBoxLayout* watts_layout = new QHBoxLayout;
	watts_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	watts_layout->addWidget(watts_label);
	watts_layout->addWidget(watts);
	watts_set->setLayout(watts_layout);

	QVBoxLayout* final_layout = new QVBoxLayout;
	final_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	final_layout->addWidget(watts_set);
	final_layout->addWidget(close_button);
	setLayout(final_layout);

	setMinimumWidth(300);
	setWindowTitle(QString("Pokemon Pikachu 2 Configuration"));
	setWindowIcon(QIcon(QString::fromStdString(config::cfg_path + "data/icons/gbe_plus.png")));
	hide();

	connect(close_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(close_button, SIGNAL(rejected()), this, SLOT(reject()));
	connect(watts, SIGNAL(currentIndexChanged(int)), this, SLOT(update_watts()));
}

/****** Update amount of watts to transfer ******/
void pp2_menu::update_watts()
{
	config::ir_db_index = watts->currentIndex();
}
