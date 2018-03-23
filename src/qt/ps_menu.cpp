// GB Enhanced+ Copyright Daniel Baxter 2018
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : ps_menu.cpp
// Date : March 21, 2018
// Description : Pocket Sakura menu
//
// Change the amount of points GBE+ will receive from the Pocket Sakura

#include "ps_menu.h"

#include "common/config.h"

/****** Pocket Sakura menu constructor ******/
ps_menu::ps_menu(QWidget *parent) : QDialog(parent)
{
	close_button = new QDialogButtonBox(QDialogButtonBox::Close);

	//Watts drop down menu
	QWidget* points_set = new QWidget;
	QLabel* points_label = new QLabel("Number of Points : ");
 	
	points = new QComboBox;
	points->addItem("1 Point");
	points->addItem("5 Points");
	points->addItem("10 Points");

	QHBoxLayout* points_layout = new QHBoxLayout;
	points_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	points_layout->addWidget(points_label);
	points_layout->addWidget(points);
	points_set->setLayout(points_layout);

	QVBoxLayout* final_layout = new QVBoxLayout;
	final_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	final_layout->addWidget(points_set);
	final_layout->addWidget(close_button);
	setLayout(final_layout);

	setMinimumWidth(300);
	setWindowTitle(QString("Pocket Sakura Configuration"));
	hide();

	connect(close_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(close_button, SIGNAL(rejected()), this, SLOT(reject()));
	connect(points, SIGNAL(currentIndexChanged(int)), this, SLOT(update_points()));
}

/****** Update amount of points to transfer ******/
void ps_menu::update_points()
{
	config::ir_db_index = points->currentIndex();
}
