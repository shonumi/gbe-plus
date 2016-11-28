// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : cheat_menu.h
// Date : November 17, 2016
// Description : Cheat menu GUI
//
// Displays and edits cheats

#ifndef CHEATMENU_GBE_QT
#define CHEATMENU_GBE_QT

#include <vector>

#include <QtGui>

class cheat_menu : public QDialog
{
	Q_OBJECT
	
	public:
	cheat_menu(QWidget *parent = 0);

	QListWidget* cheats_display;
	QSignalMapper* edit_signal;

	QDialogButtonBox* close_button;
	QPushButton* edit_button;
	QPushButton* delete_button;
	QPushButton* add_button;

	QPushButton* cancel_button;
	QPushButton* apply_button;

	QWidget* data_set;
	QLabel* data_label;
	QLineEdit* data_line;

	QWidget* info_set;
	QLabel* info_label;
	QLineEdit* info_line;

	QWidget* add_set;
	QComboBox* add_type;
	QLabel* add_label;

	int current_cheat_index;

	void fetch_cheats();

	private slots:
	void rebuild_cheats();
	void edit_cheat_data();
	void update_cheats();
	void add_cheats();
	void delete_cheats();
}; 

#endif //CHEATMENU_GBE_QT 
