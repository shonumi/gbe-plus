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
cheat_menu::cheat_menu(QWidget *parent) : QWidget(parent)
{
	cheats_display = new QScrollArea;
	edit_signal = NULL;

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

	button_list.clear();

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
			code_type = "Game Genie: ";
			code_data = config::gg_cheats[gg_count++];
		}

		else { return; }

		QLabel* type_label = new QLabel(QString::fromStdString(code_type));
		QLabel* data_label = new QLabel(QString::fromStdString(code_data));
		QLabel* info_label = new QLabel(QString::fromStdString(current_cheat));

		QPushButton* edit_button = new QPushButton("Edit Cheat");
		button_list.push_back(edit_button);

		//Spacer label
		QLabel* spacer_label = new QLabel(" ");

		temp_layout->addWidget(type_label, (x * 2), 0, 1, 1);
		temp_layout->addWidget(data_label, (x * 2), 1, 1, 1);
		temp_layout->addWidget(info_label, (x * 2), 2, 1, 1);
		temp_layout->addWidget(button_list.back(), (x * 2), 3, 1, 1);

		temp_layout->addWidget(spacer_label, (x * 2) + 1, 0, 4, 1);

		//Map signals
		connect(button_list[x], SIGNAL(clicked()), edit_signal, SLOT(map()));
		edit_signal->setMapping(button_list[x], x+1);
	}

	temp_layout->setHorizontalSpacing(25);

	fetched_cheat_set->setLayout(temp_layout);	
	cheats_display->setWidget(fetched_cheat_set);

	connect(edit_signal, SIGNAL(mapped(int)), this, SLOT(edit_cheat_data(int))) ;
}

/****** Edits a specific cheat code ******/
void cheat_menu::edit_cheat_data(int cheat_code_index)
{
	std::string current_cheat = "";
	std::string code_data = "";

	int cheat_info_index = 0;
	int gs_count = 0;
	int gg_count = 0;

	QLabel* data_label = new QLabel(" ");	

	//Search for specific cheat info
	for(int x = 0; x < cheat_code_index; x++)
	{
		current_cheat = config::cheats_info[x];

		std::string last_char = "";
		last_char += current_cheat[current_cheat.size() - 1];

		//GS code
		if(last_char == "*")
		{
			if((x + 1) == cheat_code_index)
			{
				current_cheat.resize(current_cheat.size() - 1);
				code_data = util::to_hex_str(config::gs_cheats[gs_count]);
				code_data = code_data.substr(2);

				data_label->setText("Gameshark Code: ");
			}

			gs_count++;
		}

		else if(last_char == "^")
		{
			if((x + 1) == cheat_code_index)
			{
				current_cheat.resize(current_cheat.size() - 1);
				code_data = config::gg_cheats[gg_count];

				data_label->setText("Game Genie Code: ");
			}

			gg_count++;
		}
	}

	//Change main layout
	QGridLayout* temp_layout = new QGridLayout;
	temp_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	QWidget* edited_cheat_set = new QWidget;

	//Rest of the widgets
	QLabel* info_label = new QLabel("Cheat Comments: ");
	QLineEdit* data_line = new QLineEdit(QString::fromStdString(code_data));
	QLineEdit* info_line = new QLineEdit(QString::fromStdString(current_cheat));

	//Data layout
	QWidget* data_set = new QWidget;
	QHBoxLayout* data_layout = new QHBoxLayout;
	data_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	data_layout->addWidget(data_label);
	data_layout->addWidget(data_line);
	data_set->setLayout(data_layout);

	//Info layout
	QWidget* info_set = new QWidget;
	QHBoxLayout* info_layout = new QHBoxLayout;
	info_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	info_layout->addWidget(info_label);
	info_layout->addWidget(info_line);
	info_set->setLayout(info_layout);

	temp_layout->addWidget(data_set, 0, 0, 1, 1);
	temp_layout->addWidget(info_set, 1, 0, 1, 1);

	edited_cheat_set->setLayout(temp_layout);	
	cheats_display->setWidget(edited_cheat_set);
}
