// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : data_dialog.h
// Date : November 17, 2016
// Description : Cheat menu GUI
//
// Displays and edits cheats

#include "cheat_menu.h"

#include "common/config.h"
#include "common/util.h"

/****** Cheat menu constructor ******/
cheat_menu::cheat_menu(QWidget *parent) : QWidget(parent)
{
	cheats_display = new QScrollArea;

	//Cheat menu layout
	QHBoxLayout* cheat_menu_layout = new QHBoxLayout;
	cheat_menu_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	cheat_menu_layout->addWidget(cheats_display);
	setLayout(cheat_menu_layout);

	resize(600, 400);
	hide();
}

/****** Grab cheats from config data ******/
void cheat_menu::fetch_cheats()
{
	parse_cheats_file();

	QGridLayout* temp_layout = new QGridLayout;
	temp_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	QWidget* fetched_cheat_set = new QWidget;

	u32 gs_count = 0;
	u32 gg_count = 0;

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
			code_type = "Game Genie: ";
			code_data = config::gg_cheats[gg_count++];
		}

		else { return; }

		QLabel* type_label = new QLabel(QString::fromStdString(code_type));
		QLabel* data_label = new QLabel(QString::fromStdString(code_data));
		QLabel* info_label = new QLabel(QString::fromStdString(current_cheat));
		QPushButton* edit_button = new QPushButton("Edit Cheat");

		//Spacer label
		QLabel* spacer_label = new QLabel(" ");

		temp_layout->addWidget(type_label, (x * 2), 0, 1, 1);
		temp_layout->addWidget(data_label, (x * 2), 1, 1, 1);
		temp_layout->addWidget(info_label, (x * 2), 2, 1, 1);
		temp_layout->addWidget(edit_button, (x * 2), 3, 1, 1);

		temp_layout->addWidget(spacer_label, (x * 2) + 1, 0, 4, 1);
	}

	temp_layout->setHorizontalSpacing(25);

	fetched_cheat_set->setLayout(temp_layout);	
	cheats_display->setWidget(fetched_cheat_set);
}
