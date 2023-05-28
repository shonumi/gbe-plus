// GB Enhanced+ Copyright Daniel Baxter 2019
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mpos_menu.h
// Date : July 28, 2019
// Description : Multi Plust On System menu
//
// Changes Pluster figurine

#ifndef MPOSMENU_GBE_QT
#define MPOSMENU_GBE_QT

#include <QtWidgets>

class mpos_menu : public QDialog
{
	Q_OBJECT

	public:
	mpos_menu(QWidget *parent = 0);

	QDialogButtonBox* close_button;

	QComboBox* pluster;

	private slots:
	void update_pluster_figurine();
};

#endif //PP2MENU_GBE_QT 
