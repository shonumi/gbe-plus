// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : data_dialog.h
// Date : November 17, 2016
// Description : Cheat menu GUI
//
// Displays and edits cheats

#include <iostream>

#include "cheat_menu.h"

#include "common/config.h"
#include "common/util.h"

/****** Cheat menu constructor ******/
cheat_menu::cheat_menu(QWidget *parent) : QDialog(parent)
{
	cheats_display = new QListWidget;
	edit_signal = NULL;

	close_button = new QDialogButtonBox(QDialogButtonBox::Close);
	edit_button = close_button->addButton("Edit Cheat", QDialogButtonBox::ActionRole);

	//Cheat menu layout
	QVBoxLayout* cheat_menu_layout = new QVBoxLayout;
	cheat_menu_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	cheat_menu_layout->addWidget(cheats_display);
	cheat_menu_layout->addWidget(close_button);
	setLayout(cheat_menu_layout);

	connect(close_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(close_button, SIGNAL(rejected()), this, SLOT(reject()));
	connect(close_button->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(rebuild_cheats()));
	connect(edit_button, SIGNAL(clicked()), this, SLOT(edit_cheat_data()));

	resize(600, 400);
	hide();

	data_set = new QWidget;
	data_label = new QLabel;
	data_line = new QLineEdit;

	data_set->hide();
	data_label->hide();
	data_line->hide();
	
	info_set = new QWidget;
	info_label = new QLabel;
	info_line = new QLineEdit;

	info_set->hide();
	info_label->hide();
	info_line->hide();
}

/****** Grab cheats from config data ******/
void cheat_menu::fetch_cheats()
{
	//Rebuild cheat list layout
	if(data_set->layout() != NULL) { delete data_set->layout(); }
	if(info_set->layout() != NULL) { delete info_set->layout(); }
	if(layout() != NULL) { delete layout(); }

	//Cheat menu layout
	QVBoxLayout* cheat_menu_layout = new QVBoxLayout;
	cheat_menu_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	cheat_menu_layout->addWidget(cheats_display);
	cheat_menu_layout->addWidget(close_button);
	setLayout(cheat_menu_layout);

	cheats_display->show();
	edit_button->show();

	data_set->hide();
	data_label->hide();
	data_line->hide();

	info_set->hide();
	info_label->hide();
	info_line->hide();

	parse_cheats_file();

	u32 gs_count = 0;
	u32 gg_count = 0;

	//Setup signal mapper
	if(edit_signal != NULL) { delete edit_signal; }
	edit_signal = new QSignalMapper;

	for(u32 x = 0; x < config::cheats_info.size(); x++)
	{
		std::string current_cheat = config::cheats_info[x];
		std::string code_type;
		std::string code_data;
		
		std::string last_char = "";
		last_char += current_cheat[current_cheat.size() - 1];

		//Fetch GS code
		if(last_char == "*")
		{
			current_cheat.resize(current_cheat.size() - 1);
			code_type = "Gameshark Code: ";
			code_data = util::to_hex_str(config::gs_cheats[gs_count++]);
			code_data = code_data.substr(2);
		}

		else if(last_char == "^")
		{
			current_cheat.resize(current_cheat.size() - 1);
			code_type = "Game Genie Code: ";
			code_data = config::gg_cheats[gg_count++];
		}

		else { return; }

		std::string final_str = code_type + "\t" + code_data + "\n" + "Code Description: \t" + current_cheat + "\n";
		cheats_display->addItem(QString::fromStdString(final_str));
	}
}

/****** Edits a specific cheat code ******/
void cheat_menu::edit_cheat_data()
{
	int cheat_code_index = cheats_display->currentRow();
	std::string current_cheat = "";
	std::string code_data = "";

	int gs_count = 0;
	int gg_count = 0;

	std::string data_str;

	//Search for specific cheat info
	for(int x = 0; x <= cheat_code_index; x++)
	{
		current_cheat = config::cheats_info[x];

		std::string last_char = "";
		last_char += current_cheat[current_cheat.size() - 1];

		//GS code
		if(last_char == "*")
		{
			if(x == cheat_code_index)
			{
				current_cheat.resize(current_cheat.size() - 1);
				code_data = util::to_hex_str(config::gs_cheats[gs_count]);
				code_data = code_data.substr(2);

				data_str = "Gameshark Code: \t";
			}

			gs_count++;
		}

		else if(last_char == "^")
		{
			if(x == cheat_code_index)
			{
				current_cheat.resize(current_cheat.size() - 1);
				code_data = config::gg_cheats[gg_count];

				data_str = "Game Genie Code: \t";
			}

			gg_count++;
		}
	}

	//Code data
	if(data_set->layout() != NULL) { delete data_set->layout(); }
	data_label->setText(QString::fromStdString(data_str));	
	data_line->setText(QString::fromStdString(code_data));

	QHBoxLayout* data_layout = new QHBoxLayout;
	data_layout->addWidget(data_label);
	data_layout->addWidget(data_line);
	data_set->setLayout(data_layout);

	data_set->show();
	data_label->show();
	data_line->show();

	//Code info
	if(info_set->layout() != NULL) { delete info_set->layout();}
	info_label->setText(QString::fromUtf8("Cheat Comments: \t"));
	info_line->setText(QString::fromStdString(current_cheat));

	QHBoxLayout* info_layout = new QHBoxLayout;
	info_layout->addWidget(info_label);
	info_layout->addWidget(info_line);
	info_set->setLayout(info_layout);

	info_set->show();
	info_label->show();
	info_line->show();

	//Change main layout
	delete layout();

	QVBoxLayout* cheat_menu_layout = new QVBoxLayout;
	cheat_menu_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	cheat_menu_layout->addWidget(data_set);
	cheat_menu_layout->addWidget(info_set);
	cheat_menu_layout->addWidget(close_button);
	setLayout(cheat_menu_layout);

	cheats_display->hide();
	edit_button->hide();
}

/****** Rebuilds the cheat list if the dialog is closed or edit button is pressed ******/
void cheat_menu::rebuild_cheats() { fetch_cheats(); }
