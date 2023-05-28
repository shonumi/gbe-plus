// GB Enhanced+ Copyright Daniel Baxter 2021
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mw_menu.h
// Date : September 15, 2021
// Description : Magical Watch menu
//
// Changes data emulated Magical Watch sends to GBA

#ifndef MWMENU_GBE_QT
#define MWMENU_GBE_QT

#include <QtWidgets>

class mw_menu : public QDialog
{
	Q_OBJECT

	public:
	mw_menu(QWidget *parent = 0);

	QDialogButtonBox* close_button;

	QSpinBox* mw_item_1;
	QSpinBox* mw_item_2;
	QSpinBox* mw_item_3;
	QSpinBox* mw_item_4;
	QSpinBox* mw_hours;
	QSpinBox* mw_mins;

	private slots:
	void update_item_1();
	void update_item_2();
	void update_item_3();
	void update_item_4();
	void update_hours();
	void update_mins();
};

#endif //MWMENU_GBE_QT  
