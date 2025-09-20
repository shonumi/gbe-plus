// GB Enhanced+ Copyright Daniel Baxter 2025
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : wantame_menu.cpp
// Date : September 20, 2025
// Description : Wantame Card Scanner menu
//
// Sets the barcode for the Wantame Card Scanner

#include "wantame_menu.h"

#include "common/config.h"

/****** WCS menu constructor ******/
wcs_menu::wcs_menu(QWidget *parent) : QDialog(parent)
{
	close_button = new QDialogButtonBox(QDialogButtonBox::Close);

	//Cosmic character drop down menu
	QWidget* barcode_set = new QWidget;
	QLabel* barcode_label = new QLabel("Barcode : ");

	barcode_line = new QLineEdit;

	QHBoxLayout* barcode_layout = new QHBoxLayout;
	barcode_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	barcode_layout->addWidget(barcode_label);
	barcode_layout->addWidget(barcode_line);
	barcode_set->setLayout(barcode_layout);

	QVBoxLayout* final_layout = new QVBoxLayout;
	final_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	final_layout->addWidget(barcode_set);
	final_layout->addWidget(close_button);
	setLayout(final_layout);

	setMinimumWidth(300);
	setWindowTitle(QString("Wantame Card Scanner Configuration"));
	setWindowIcon(QIcon(QString::fromStdString(config::cfg_path + "data/icons/gbe_plus.png")));
	hide();

	connect(close_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(close_button, SIGNAL(rejected()), this, SLOT(reject()));
}

/****** Sets the raw barcode (alphanumerical value) when updating QLineEdit ******/
void wcs_menu::update_wcs_barcode()
{

}
