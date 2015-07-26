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

	tabs->addTab(obj_tab, tr("OBJ Tiles"));
	tabs->addTab(bg_tab, tr("BG Tiles"));

	tabs_button = new QDialogButtonBox(QDialogButtonBox::Close);

	obj_set = new QWidget(obj_tab);
	bg_set = new QWidget(bg_set);

	obj_layout = new QGridLayout;
	bg_layout = new QGridLayout;

	setup_obj_window(8, 40);

	//OBJ Tab layout
	QVBoxLayout* obj_tab_layout = new QVBoxLayout;
	obj_tab_layout->addWidget(obj_set);
	obj_tab->setLayout(obj_tab_layout);

	//Final tab layout
	QVBoxLayout* main_layout = new QVBoxLayout;
	main_layout->addWidget(tabs);
	main_layout->addWidget(tabs_button);
	setLayout(main_layout);

	connect(tabs_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(tabs_button, SIGNAL(rejected()), this, SLOT(reject()));

	resize(600, 600);
	setWindowTitle(tr("Custom Graphics"));
}

/****** Sets up the OBJ dumping window ******/
void gbe_cgfx::setup_obj_window(int rows, int count)
{
	//Clear out previous widgets
	if(!cgfx_obj.empty())
	{
		for(int x = 0; x < cgfx_obj.size(); x++)
		{
			//Remove QImages from the layout
			obj_layout->removeItem(obj_layout->itemAt(x));
		}

		cgfx_obj.clear();
	}

	//Generate the correct number of QImages
	for(int x = 0; x < count; x++)
	{
		cgfx_obj.push_back(QImage(64, 64, QImage::Format_ARGB32));
		
		//Set QImage to black first
		for(int pixel_counter = 0; pixel_counter < 4096; pixel_counter++)
		{
			cgfx_obj[x].setPixel((pixel_counter % 64), (pixel_counter / 64), 0xFF000000);
		}
		
		//Wrap QImage in a QLabel
		QLabel* obj_label = new QLabel;
		obj_label->setPixmap(QPixmap::fromImage(cgfx_obj[x]));

		obj_layout->addWidget(obj_label, (x / rows), (x % rows));
	}

	obj_set->setLayout(obj_layout);
}
		