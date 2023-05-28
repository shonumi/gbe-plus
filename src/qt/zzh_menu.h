// GB Enhanced+ Copyright Daniel Baxter 2018
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : zzh_menu.h
// Date : February 24, 2018
// Description : Zok Zok Heroes Full Changer menu
//
// Changes the cosmic character transmitted to the GBC

#ifndef ZZHMENU_GBE_QT
#define ZZHMENU_GBE_QT

#include <QtWidgets>

class zzh_menu : public QDialog
{
	Q_OBJECT

	public:
	zzh_menu(QWidget *parent = 0);

	QDialogButtonBox* close_button;

	QComboBox* cosmic_character;

	private slots:
	void update_cosmic_character();
};

#endif //ZZHMENU_GBE_QT 
