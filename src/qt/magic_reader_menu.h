// GB Enhanced+ Copyright Daniel Baxter 2020
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : magic_reader_menu.h
// Date : August 7, 2020
// Description : Konami Magic Reader menu
//
// Sets ID for Magic Reader

#ifndef MRMENU_GBE_QT
#define MRMENU_GBE_QT

#include <QtWidgets>

class mr_menu : public QDialog
{
	Q_OBJECT

	public:
	mr_menu(QWidget *parent = 0);

	QDialogButtonBox* close_button;

	QSpinBox* qt_id;

	private slots:
	void update_mr_id();
};

#endif //MRMENU_GBE_QT 
