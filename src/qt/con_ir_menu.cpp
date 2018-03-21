// GB Enhanced+ Copyright Daniel Baxter 2018
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : con_ir_menu.cpp
// Date : February 21, 2018
// Description : Constant IR Light menu
//
// Change the mode of the constant IR light source
// Static vs. Interactive

#include "con_ir_menu.h"

#include "common/config.h"

/****** Constant IR menu constructor ******/
con_ir_menu::con_ir_menu(QWidget *parent) : QDialog(parent)
{
	close_button = new QDialogButtonBox(QDialogButtonBox::Close);

	//Watts drop down menu
	QWidget* ir_mode_set = new QWidget;
	QLabel* ir_mode_label = new QLabel("Constant IR Light Mode : ");
 	
	ir_mode = new QComboBox;
	ir_mode->addItem("Static");
	ir_mode->addItem("Interactive");

	QHBoxLayout* ir_mode_layout = new QHBoxLayout;
	ir_mode_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	ir_mode_layout->addWidget(ir_mode_label);
	ir_mode_layout->addWidget(ir_mode);
	ir_mode_set->setLayout(ir_mode_layout);

	QVBoxLayout* final_layout = new QVBoxLayout;
	final_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	final_layout->addWidget(ir_mode_set);
	final_layout->addWidget(close_button);
	setLayout(final_layout);

	setMinimumWidth(300);
	setWindowTitle(QString("Constant IR Light Configuration"));
	hide();

	connect(close_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(close_button, SIGNAL(rejected()), this, SLOT(reject()));
	connect(ir_mode, SIGNAL(currentIndexChanged(int)), this, SLOT(update_ir_mode()));
}

/****** Update IR mode  ******/
void con_ir_menu::update_ir_mode()
{
	config::ir_db_index = ir_mode->currentIndex();
}
