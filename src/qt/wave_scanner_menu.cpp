// GB Enhanced+ Copyright Daniel Baxter 2025
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : wave_scanner_menu.cpp
// Date : September 20, 2025
// Description : Wave Scanner menu
//
// Sets the barcode for the Wave Scanner

#include <fstream>

#include "wave_scanner_menu.h"

#include "common/config.h"

/****** Wave Scanner menu constructor ******/
wav_menu::wav_menu(QWidget *parent) : QDialog(parent)
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

	QWidget* load_set = new QWidget;
	QLabel* load_label = new QLabel(" ", load_set);
	load_button = new QPushButton("Load Barcode File");
	load_button->setMinimumWidth(250);

	QHBoxLayout* load_layout = new QHBoxLayout;
	load_layout->addWidget(load_button, 0, Qt::AlignCenter);
	load_layout->addWidget(load_label, 1, Qt::AlignRight);
	load_set->setLayout(load_layout);

	QWidget* level_set = new QWidget;
	QLabel* level_label = new QLabel("Level : ");
	level = new QSpinBox;
	level->setMinimum(1);
	level->setMaximum(99);

	QHBoxLayout* level_layout = new QHBoxLayout;
	level_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	level_layout->addWidget(level_label);
	level_layout->addWidget(level);
	level_set->setLayout(level_layout);

	QVBoxLayout* final_layout = new QVBoxLayout;
	final_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	final_layout->addWidget(barcode_set);
	final_layout->addWidget(load_set);
	final_layout->addWidget(level_set);
	final_layout->addWidget(close_button);
	setLayout(final_layout);

	setMinimumWidth(300);
	setWindowTitle(QString("Wave Scanner Configuration"));
	setWindowIcon(QIcon(QString::fromStdString(config::cfg_path + "data/icons/gbe_plus.png")));
	hide();

	connect(close_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(close_button, SIGNAL(rejected()), this, SLOT(reject()));
	connect(barcode_line, SIGNAL(textChanged(const QString)), this, SLOT(update_wave_barcode()));
	connect(load_button, SIGNAL(clicked()), this, SLOT(load_barcode()));
}

/****** Sets the raw barcode (alphanumerical value) when updating QLineEdit ******/
void wav_menu::update_wave_barcode()
{
	std::string barcode = barcode_line->text().toStdString();
	std::string edit = "";

	//Limit all input to ASCII numerals + 12 characters max
	for(u32 x = 0; x < barcode.length(); x++)
	{
		u8 chr = barcode[x];

		if((chr >= 0x30) && (chr <= 0x39))
		{
			edit += barcode[x];
		}
	}

	if(edit.length() > 12)
	{
		edit = edit.substr(0, 12);
	}

	barcode_line->setText(QString::fromStdString(edit));
	config::raw_barcode = edit;
}

/****** Loads a barcode from a file and updates it as well (including current QLineEdit text) ******/
bool wav_menu::load_barcode()
{
	SDL_PauseAudio(1);

	QString filename = QFileDialog::getOpenFileName(this, tr("Open"), "", tr("Binary card data (*.bin)"));
	if(filename.isNull()) { SDL_PauseAudio(0); return false; }

	std::ifstream file(filename.toStdString().c_str(), std::ios::binary);

	if(!file.is_open()) 
	{
		return false;
	}

	//Get file size
	file.seekg(0, file.end);
	u32 file_size = file.tellg();
	file.seekg(0, file.beg);

	if(file_size != 12)
	{
		return false;
	}

	char ex_data[12];

	file.read((char*)ex_data, file_size); 
	file.close();


	std::string result = "";
	result.assign(ex_data, 12);
	barcode_line->setText(QString::fromStdString(result));

	SDL_PauseAudio(0);

	return true;
}

/****** Sets the Wave Scanner level when updating QSpinBox ******/
void wav_menu::update_wave_level()
{
	config::wave_scanner_level = level->value();
}
