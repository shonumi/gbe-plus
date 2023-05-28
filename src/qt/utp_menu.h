// GB Enhanced+ Copyright Daniel Baxter 2020
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : utp_menu.h
// Date : February 14, 2020
// Description : Ubisoft Thrustmaster Pedometer menu
//
// Sets steps for pedometer

#ifndef UTPMENU_GBE_QT
#define UTPMENU_GBE_QT

#include <QtWidgets>

class utp_menu : public QDialog
{
	Q_OBJECT

	public:
	utp_menu(QWidget *parent = 0);

	QDialogButtonBox* close_button;

	QSpinBox* qt_steps;

	private slots:
	void update_steps();
};

#endif //UTPMENU_GBE_QT 
