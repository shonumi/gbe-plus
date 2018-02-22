// GB Enhanced+ Copyright Daniel Baxter 2018
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : pp2_menu.h
// Date : February 21, 2018
// Description : Pocket Pikachu 2 menu
//
// Change the amount of watts GBE+ will receive from Pikachu

#ifndef PP2MENU_GBE_QT
#define PP2MENU_GBE_QT

#include <QtGui> 

class pp2_menu : public QDialog
{
	Q_OBJECT

	public:
	pp2_menu(QWidget *parent = 0);

	QDialogButtonBox* close_button;

	QComboBox* watts;

	private slots:
	void update_watts();
};

#endif //PP2MENU_GBE_QT 
