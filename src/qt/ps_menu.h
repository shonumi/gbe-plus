// GB Enhanced+ Copyright Daniel Baxter 2018
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : ps_menu.h
// Date : March 21, 2018
// Description : Pocket Sakura menu
//
// Change the amount of points GBE+ will receive from the Pocket Sakura

#ifndef PSMENU_GBE_QT
#define PSMENU_GBE_QT

#ifdef GBE_QT_5
#include <QtWidgets>
#endif

#ifdef GBE_QT_4
#include <QtGui>
#endif 

class ps_menu : public QDialog
{
	Q_OBJECT

	public:
	ps_menu(QWidget *parent = 0);

	QDialogButtonBox* close_button;

	QComboBox* points;

	private slots:
	void update_points();
};

#endif //PSMENU_GBE_QT 
