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
	delete_button = close_button->addButton("Delete Cheat", QDialogButtonBox::ActionRole);
	add_button = close_button->addButton("Add Cheat", QDialogButtonBox::ActionRole);

	apply_button = close_button->addButton("Apply Changes", QDialogButtonBox::ActionRole);
	cancel_button = close_button->addButton("Cancel", QDialogButtonBox::ActionRole);

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
	connect(cancel_button, SIGNAL(clicked()), this, SLOT(rebuild_cheats()));
	connect(apply_button, SIGNAL(clicked()), this, SLOT(update_cheats()));
	connect(add_button, SIGNAL(clicked()), this, SLOT(add_cheats()));
	connect(delete_button, SIGNAL(clicked()), this, SLOT(delete_cheats()));

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

	cancel_button->hide();
	apply_button->hide();

	add_set = new QWidget;
	add_label = new QLabel("Cheat Code Type: ");

	add_type = new QComboBox;
	add_type->addItem("Gameshark Code");
	add_type->addItem("Game Genie Code");

	add_set->hide();
	add_type->hide();
	add_label->hide();
	
	current_cheat_index = 0;

	parse_cheats_file();
}

/****** Grab cheats from config data ******/
void cheat_menu::fetch_cheats()
{
	//Rebuild cheat list layout
	if(data_set->layout() != NULL) { delete data_set->layout(); }
	if(info_set->layout() != NULL) { delete info_set->layout(); }
	if(add_set->layout() != NULL) { delete add_set->layout(); }
	if(layout() != NULL) { delete layout(); }

	//Cheat menu layout
	QVBoxLayout* cheat_menu_layout = new QVBoxLayout;
	cheat_menu_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	cheat_menu_layout->addWidget(cheats_display);
	cheat_menu_layout->addWidget(close_button);
	setLayout(cheat_menu_layout);

	cheats_display->show();
	cheats_display->clear();
	edit_button->show();
	delete_button->show();
	add_button->show();

	cancel_button->hide();
	apply_button->hide();

	data_set->hide();
	data_label->hide();
	data_line->hide();

	info_set->hide();
	info_label->hide();
	info_line->hide();

	add_set->hide();
	add_type->hide();
	add_label->hide();

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
	current_cheat_index = cheat_code_index;

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
	delete_button->hide();
	add_button->hide();

	cancel_button->show();
	apply_button->show();
}

/****** Rebuilds the cheat list if the dialog is closed or edit button is pressed ******/
void cheat_menu::rebuild_cheats() { fetch_cheats(); }

/****** Updates the cheat data ******/
void cheat_menu::update_cheats()
{
	int code_type = -1;

	int gs_count = -1;
	int gg_count = -1;

	//Add new cheat data
	if(current_cheat_index == -1)
	{
		//Add new Gameshark cheat
		if(add_type->currentIndex() == 0)
		{
			std::string code_data = data_line->text().toStdString();
			std::string code_info = info_line->text().toStdString();

			code_info += "*";

			//Cut down data if it larger than 8 characters
			if(code_data.length() > 8) { code_data = code_data.substr(0, 8); }

			u32 converted_cheat = 0;
			util::from_hex_str(code_data, converted_cheat);
	
			config::gs_cheats.push_back(converted_cheat);
			config::cheats_info.push_back(code_info);
		}

		//Add new Game Genie cheat
		else if(add_type->currentIndex() == 1)
		{
			//Parse code format
			std::string code_data = data_line->text().toStdString();
			std::string code_info = info_line->text().toStdString();

			code_info += "^";

			//Cut down data if it larger than 9 characters
			if(code_data.length() > 9) { code_data = code_data.substr(0, 9); }
	
			config::gg_cheats.push_back(code_data);
			config::cheats_info.push_back(code_info);
		}

		fetch_cheats();
		return;
	}

	//Otherwise, update existing cheat data
	for(int x = 0; x <= current_cheat_index; x++)
	{
		std::string current_cheat = config::cheats_info[x];

		std::string last_char = "";
		last_char += current_cheat[current_cheat.size() - 1];

		//GS code
		if(last_char == "*")
		{
			if(x == current_cheat_index) { code_type = 0; }

			gs_count++;
		}

		else if(last_char == "^")
		{
			if(x == current_cheat_index) { code_type = 1; }

			gg_count++;
		}
	}

	//Process GS code editing
	if(code_type == 0)
	{
		//Parse code format
		std::string code_data = data_line->text().toStdString();
		std::string code_info = info_line->text().toStdString();

		//Cut down data if it larger than 8 characters
		if(code_data.length() > 8) { code_data = code_data.substr(0, 8); }

		u32 converted_cheat = 0;
		util::from_hex_str(code_data, converted_cheat);
	
		config::gs_cheats[gs_count] = converted_cheat;
		config::cheats_info[current_cheat_index] = code_info + "*";
		
		fetch_cheats();
	}

	//Process GG code editing
	else if(code_type == 1)
	{
		//Parse code format
		std::string code_data = data_line->text().toStdString();
		std::string code_info = info_line->text().toStdString();

		//Cut down data if it larger than 9 characters
		if(code_data.length() > 9) { code_data = code_data.substr(0, 9); }
	
		config::gg_cheats[gg_count] = code_data;
		config::cheats_info[current_cheat_index] = code_info + "^";

		fetch_cheats();
	}
}

/****** Adds cheat data - Mostly switches layout, actual data is added in update_cheats() ******/
void cheat_menu::add_cheats()
{
	//Code type
	if(add_set->layout() != NULL) { delete add_set->layout(); }

	QHBoxLayout* add_layout = new QHBoxLayout;
	add_layout->addWidget(add_label);
	add_layout->addWidget(add_type);
	add_set->setLayout(add_layout);

	add_set->show();
	add_label->show();
	add_type->show();

	//Code data
	if(data_set->layout() != NULL) { delete data_set->layout(); }
	data_label->setText(QString::fromUtf8("Cheat Code Data: \t"));
	data_line->clear();

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
	info_line->clear();

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
	cheat_menu_layout->addWidget(add_set);
	cheat_menu_layout->addWidget(data_set);
	cheat_menu_layout->addWidget(info_set);
	cheat_menu_layout->addWidget(close_button);
	setLayout(cheat_menu_layout);

	cheats_display->hide();

	edit_button->hide();
	delete_button->hide();
	add_button->hide();

	cancel_button->show();
	apply_button->show();

	current_cheat_index = -1;
}

/****** Delete cheat data ******/
void cheat_menu::delete_cheats()
{
	int cheat_code_index = cheats_display->currentRow();
	current_cheat_index = cheat_code_index;

	int gs_count = 0;
	int gg_count = 0;
	int code_type = -1;

	//Search for specific cheat info
	for(int x = 0; x <= cheat_code_index; x++)
	{
		std::string current_cheat = config::cheats_info[x];

		std::string last_char = "";
		last_char += current_cheat[current_cheat.size() - 1];

		//GS code
		if(last_char == "*")
		{
			if(x == cheat_code_index) { code_type = 0; }

			gs_count++;
		}

		else if(last_char == "^")
		{
			if(x == cheat_code_index) { code_type = 1; }

			gg_count++;
		}
	}

	

	//Delete GS code
	if(code_type == 0)
	{
		if(gs_count > 0) { gs_count--; }

		config::gs_cheats.erase(config::gs_cheats.begin() + gs_count);
		config::cheats_info.erase(config::cheats_info.begin() + cheat_code_index);
	}

	//Delete GG code
	else if(code_type == 1)
	{
		if(gg_count > 0) { gg_count--; }

		config::gg_cheats.erase(config::gg_cheats.begin() + gg_count);
		config::cheats_info.erase(config::cheats_info.begin() + cheat_code_index);
	}

	fetch_cheats();
}
