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

class cheat_menu : public QWidget
{
	Q_OBJECT
	
	public:
	cheat_menu(QWidget *parent = 0);

	QScrollArea* cheats_display;
	QSignalMapper* edit_signal;
	std::vector<QPushButton*> button_list;

	void fetch_cheats();

	private slots:
	void edit_cheat_data(int cheat_code_index);
}; 

#endif //CHEATMENU_GBE_QT 
