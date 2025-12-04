// GB Enhanced+ Copyright Daniel Baxter 2025
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : wave_scanner_menu.h
// Date : December 01, 2025
// Description : Wave Scanner menu
//
// Sets the barcode for the Wave Scanner

#ifndef WAVEMENU_GBE_QT
#define WAVEMENU_GBE_QT

#include <QtWidgets>

class wav_menu : public QDialog
{
	Q_OBJECT

	public:
	wav_menu(QWidget *parent = 0);

	QDialogButtonBox* close_button;

	QWidget* barcode_set;
	QLabel* barcode_label;
	QLineEdit* barcode_line;
	QPushButton* load_button;
	QSpinBox* level;
	QComboBox* data_type;
	QComboBox* wave_type;

	private slots:
	void update_wave_barcode();
	void update_wave_level();
	void update_data_type();
	void update_wave_type();
	bool load_barcode();
};

#endif //WAVEMENU_GBE_QT  
