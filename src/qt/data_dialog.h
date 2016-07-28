// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : data_dialog.h
// Date : March 26, 2016
// Description : Custom file dialog
//
// Customized file dialog widget for opening and viewing the GBE+ data folder

#ifndef DATADIALOG_GBE_QT
#define DATADIALOG_GBE_QT

#include <QtGui>

class data_dialog : public QFileDialog
{
	Q_OBJECT
	
	public:
	data_dialog(QWidget *parent = 0);

	QDir path;
	QString last_path;
	bool finish;

	void open_data_folder();
}; 

#endif //DATADIALOG_GBE_QT