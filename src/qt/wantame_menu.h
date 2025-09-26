// GB Enhanced+ Copyright Daniel Baxter 2025
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : wantame_menu.h
// Date : September 20, 2025
// Description : Wantame Card Scanner menu
//
// Sets the barcode for the Wantame Card Scanner

#ifndef WCSMENU_GBE_QT
#define WCSMENU_GBE_QT

#include <QtWidgets>

class wcs_menu : public QDialog
{
	Q_OBJECT

	public:
	wcs_menu(QWidget *parent = 0);

	QDialogButtonBox* close_button;

	QWidget* barcode_set;
	QLabel* barcode_label;
	QLineEdit* barcode_line;
	QPushButton* load_button;

	private slots:
	void update_wcs_barcode();
	bool load_barcode();
};

#endif //WCSMENU_GBE_QT  
