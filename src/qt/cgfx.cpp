// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : cgfx.h
// Date : July 25, 2015
// Description : Custom graphics settings
//
// Dialog for various custom graphics options

#include "cgfx.h"
#include "main_menu.h"

/****** General settings constructor ******/
gbe_cgfx::gbe_cgfx(QWidget *parent) : QDialog(parent)
{
	//Set up tabs
	tabs = new QTabWidget(this);
	
	QDialog* obj_tab = new QDialog;
	QDialog* bg_tab = new QDialog;

	tabs->addTab(obj_tab, tr("OBJ"));
	tabs->addTab(bg_tab, tr("BG"));

	tabs_button = new QDialogButtonBox(QDialogButtonBox::Close);

	resize(600, 600);
	setWindowTitle(tr("Custom Graphics"));
}
