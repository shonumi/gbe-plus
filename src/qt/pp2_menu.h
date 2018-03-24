// GB Enhanced+ Copyright Daniel Baxter 2018
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : pp2_menu.h
// Date : February 21, 2018
// Description : Pokemon Pikachu 2 menu
//
// Change the amount of watts GBE+ will receive from Pikachu

#ifndef PP2MENU_GBE_QT
#define PP2MENU_GBE_QT

#ifdef GBE_QT_5
#include <QtWidgets>
#endif

#ifdef GBE_QT_4
#include <QtGui>
#endif 

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
