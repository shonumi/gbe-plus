// GB Enhanced+ Copyright Daniel Baxter 2021
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mw_menu.cpp
// Date : September 15, 2021
// Description : Magical Watch menu
//
// Changes data emulated Magical Watch sends to GBA 


#include "mw_menu.h"

#include "common/config.h"

/****** Magical Watch menu constructor ******/
mw_menu::mw_menu(QWidget *parent) : QDialog(parent)
{
	close_button = new QDialogButtonBox(QDialogButtonBox::Close);

	//Set up Item 1
	QWidget* mw_item_1_set = new QWidget;
	QLabel* mw_item_1_label = new QLabel("Small Drop of Time:");
	mw_item_1 = new QSpinBox;
	mw_item_1->setMinimum(0);
	mw_item_1->setMaximum(99);

	QHBoxLayout* mw_item_1_layout = new QHBoxLayout;
	mw_item_1_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	mw_item_1_layout->addWidget(mw_item_1_label, 1, Qt::AlignLeft);
	mw_item_1_layout->addWidget(mw_item_1, 0, Qt::AlignRight);
	mw_item_1_set->setLayout(mw_item_1_layout);

	//Set up Item 2
	QWidget* mw_item_2_set = new QWidget;
	QLabel* mw_item_2_label = new QLabel("Large Drop of Time:");
	mw_item_2 = new QSpinBox;
	mw_item_2->setMinimum(0);
	mw_item_2->setMaximum(99);

	QHBoxLayout* mw_item_2_layout = new QHBoxLayout;
	mw_item_2_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	mw_item_2_layout->addWidget(mw_item_2_label, 1, Qt::AlignLeft);
	mw_item_2_layout->addWidget(mw_item_2, 0, Qt::AlignRight);
	mw_item_2_set->setLayout(mw_item_2_layout);

	//Set up Item 3
	QWidget* mw_item_3_set = new QWidget;
	QLabel* mw_item_3_label = new QLabel("Lucky Jewel:");
	mw_item_3 = new QSpinBox;
	mw_item_3->setMinimum(0);
	mw_item_3->setMaximum(99);

	QHBoxLayout* mw_item_3_layout = new QHBoxLayout;
	mw_item_3_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	mw_item_3_layout->addWidget(mw_item_3_label, 1, Qt::AlignLeft);
	mw_item_3_layout->addWidget(mw_item_3, 0, Qt::AlignRight);
	mw_item_3_set->setLayout(mw_item_3_layout);

	//Set up Item 4
	QWidget* mw_item_4_set = new QWidget;
	QLabel* mw_item_4_label = new QLabel("Screw of Time:");
	mw_item_4 = new QSpinBox;
	mw_item_4->setMinimum(0);
	mw_item_4->setMaximum(99);

	QHBoxLayout* mw_item_4_layout = new QHBoxLayout;
	mw_item_4_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	mw_item_4_layout->addWidget(mw_item_4_label, 1, Qt::AlignLeft);
	mw_item_4_layout->addWidget(mw_item_4, 0, Qt::AlignRight);
	mw_item_4_set->setLayout(mw_item_4_layout);

	//Set up Hours
	QWidget* mw_hours_set = new QWidget;
	QLabel* mw_hours_label = new QLabel("Hours:");
	mw_hours = new QSpinBox;
	mw_hours->setMinimum(0);
	mw_hours->setMaximum(2);

	QHBoxLayout* mw_hours_layout = new QHBoxLayout;
	mw_hours_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	mw_hours_layout->addWidget(mw_hours_label, 1, Qt::AlignLeft);
	mw_hours_layout->addWidget(mw_hours, 0, Qt::AlignRight);
	mw_hours_set->setLayout(mw_hours_layout);

	//Set up Minutes
	QWidget* mw_mins_set = new QWidget;
	QLabel* mw_mins_label = new QLabel("Minutes:");
	mw_mins = new QSpinBox;
	mw_mins->setMinimum(0);
	mw_mins->setMaximum(59);

	QHBoxLayout* mw_mins_layout = new QHBoxLayout;
	mw_mins_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	mw_mins_layout->addWidget(mw_mins_label, 1, Qt::AlignLeft);
	mw_mins_layout->addWidget(mw_mins, 0, Qt::AlignRight);
	mw_mins_set->setLayout(mw_mins_layout);

	//Final layout
	QVBoxLayout* final_layout = new QVBoxLayout;
	final_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	final_layout->addWidget(mw_item_1_set);
	final_layout->addWidget(mw_item_2_set);
	final_layout->addWidget(mw_item_3_set);
	final_layout->addWidget(mw_item_4_set);
	final_layout->addWidget(mw_hours_set);
	final_layout->addWidget(mw_mins_set);
	final_layout->addWidget(close_button);
	setLayout(final_layout);

	setMinimumWidth(300);
	setWindowTitle(QString("Magical Watch Configuration"));
	hide();

	connect(close_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(close_button, SIGNAL(rejected()), this, SLOT(reject()));
	connect(mw_item_1, SIGNAL(valueChanged(int)), this, SLOT(update_item_1()));
	connect(mw_item_2, SIGNAL(valueChanged(int)), this, SLOT(update_item_2()));
	connect(mw_item_3, SIGNAL(valueChanged(int)), this, SLOT(update_item_3()));
	connect(mw_item_4, SIGNAL(valueChanged(int)), this, SLOT(update_item_4()));
	connect(mw_hours, SIGNAL(valueChanged(int)), this, SLOT(update_hours()));
	connect(mw_mins, SIGNAL(valueChanged(int)), this, SLOT(update_mins()));
}

/****** Updates Magical Watch Item 1 ******/
void mw_menu::update_item_1() { config::mw_data[0] = mw_item_1->value(); }

/****** Updates Magical Watch Item 2 ******/
void mw_menu::update_item_2() { config::mw_data[1] = mw_item_2->value(); }

/****** Updates Magical Watch Item 3 ******/
void mw_menu::update_item_3() { config::mw_data[2] = mw_item_3->value(); }

/****** Updates Magical Watch Item 4 ******/
void mw_menu::update_item_4() { config::mw_data[3] = mw_item_4->value(); }

/****** Updates Magical Watch Hours ******/
void mw_menu::update_hours() { config::mw_data[4] = mw_hours->value(); }

/****** Updates Magical Watch Minutes ******/
void mw_menu::update_mins() { config::mw_data[5] = mw_mins->value(); }
