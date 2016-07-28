// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : data_dialog.cpp
// Date : March 26, 2016
// Description : Custom file dialog
//
// Customized file dialog widget for opening and viewing the GBE+ data folder

#include "data_dialog.h"

#include "common/config.h"

/****** Data dialog constructor ******/
data_dialog::data_dialog(QWidget *parent) : QFileDialog(parent)
{
	path.setPath(QString::fromStdString(config::data_path));
	path.setFilter(QDir::Hidden);

	setDirectory(path);
	setFileMode(QFileDialog::Directory);
	setOption(QFileDialog::ShowDirsOnly);
	setOption(QFileDialog::DontResolveSymlinks);

	finish = false;
}

/****** Opens the data folder ******/
void data_dialog::open_data_folder()
{
	//Save last path
	last_path = path.path();

	path.setPath(QString::fromStdString(config::data_path));
	path.setFilter(QDir::Hidden);

	setDirectory(path);
	setFileMode(QFileDialog::Directory);
	setOption(QFileDialog::ShowDirsOnly);
	setOption(QFileDialog::DontResolveSymlinks);

	finish = false;

	open();
}
