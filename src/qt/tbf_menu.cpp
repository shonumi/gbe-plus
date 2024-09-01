// GB Enhanced+ Copyright Daniel Baxter 2019
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : turbo_file_menu.h
// Date : October 29, 2019
// Description : Turbo File GB and Turbo File Advance menu
//
// Sets write-protect switch on or off
// Enables or disabled memory card

#include "tbf_menu.h"

#include "common/config.h"

/****** TBF menu constructor ******/
tbf_menu::tbf_menu(QWidget *parent) : QDialog(parent)
{
	close_button = new QDialogButtonBox(QDialogButtonBox::Close);

	//Memory card enable
	QWidget* card_set = new QWidget;
	QLabel* card_label = new QLabel("Enable Memory Card");
 	
	mem_card = new QCheckBox;

	QHBoxLayout* card_layout = new QHBoxLayout;
	card_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	card_layout->addWidget(mem_card);
	card_layout->addWidget(card_label);
	card_set->setLayout(card_layout);

	//Write-protect enable
	QWidget* wp_set = new QWidget;
	QLabel* wp_label = new QLabel("Enable Write Protect");
 	
	write_protect = new QCheckBox;

	QHBoxLayout* wp_layout = new QHBoxLayout;
	wp_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	wp_layout->addWidget(write_protect);
	wp_layout->addWidget(wp_label);
	wp_set->setLayout(wp_layout);

	QVBoxLayout* final_layout = new QVBoxLayout;
	final_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	final_layout->addWidget(card_set);
	final_layout->addWidget(wp_set);
	final_layout->addWidget(close_button);
	setLayout(final_layout);

	setMinimumWidth(300);
	setWindowTitle(QString("Turbo File Configuration"));
	setWindowIcon(QIcon(QString::fromStdString(config::cfg_path + "data/icons/gbe_plus.png")));
	hide();

	connect(close_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(close_button, SIGNAL(rejected()), this, SLOT(reject()));
	connect(mem_card, SIGNAL(stateChanged(int)), this, SLOT(update_mem_card()));
	connect(write_protect, SIGNAL(stateChanged(int)), this, SLOT(update_write_protect()));
}

/****** Updates memory card status of Turbo File ******/
void tbf_menu::update_mem_card()
{
	if(mem_card->isChecked()) { config::turbo_file_options |= 0x1; }
	else { config::turbo_file_options &= ~0x1; } 
}

/****** Updates write-protect status of Turbo File ******/
void tbf_menu::update_write_protect()
{
	if(write_protect->isChecked()) { config::turbo_file_options |= 0x2; }
	else { config::turbo_file_options &= ~0x2; } 
}
