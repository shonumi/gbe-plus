// GB Enhanced+ Copyright Daniel Baxter 2020
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mr_menu.h
// Date : August 7, 2020
// Description : Konami Magic Reader menu
//
// Sets ID for Magic Reader

#include "magic_reader_menu.h"

#include "common/config.h"
#include "common/util.h"

/****** Magic Reader menu constructor ******/
mr_menu::mr_menu(QWidget *parent) : QDialog(parent)
{
	close_button = new QDialogButtonBox(QDialogButtonBox::Close);

	//Magic Reader ID menu
	QWidget* id_set = new QWidget;
	QLabel* id_label = new QLabel("Magic Reader Index : ");
 	
	qt_id = new QSpinBox;
	qt_id->setMinimum(0);
	qt_id->setMaximum(0xFFEF);

	QHBoxLayout* id_layout = new QHBoxLayout;
	id_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	id_layout->addWidget(id_label);
	id_layout->addWidget(qt_id);
	id_set->setLayout(id_layout);

	QVBoxLayout* final_layout = new QVBoxLayout;
	final_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	final_layout->addWidget(id_set);
	final_layout->addWidget(close_button);
	setLayout(final_layout);

	setMinimumWidth(300);
	setWindowTitle(QString("Magic Reader Configuration"));
	hide();

	connect(close_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(close_button, SIGNAL(rejected()), this, SLOT(reject()));
	connect(qt_id, SIGNAL(valueChanged(int)), this, SLOT(update_mr_id()));
}

/****** Update Magic Reader ID ******/
void mr_menu::update_mr_id()
{
	config::magic_reader_id = qt_id->value() | 0x500000;
}
