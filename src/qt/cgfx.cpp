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
	cgfx_obj.clear();
	obj_button.clear();

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
		QPushButton* label = new QPushButton;
		label->setIcon(QPixmap::fromImage(cgfx_obj[x]));
		label->setIconSize(QPixmap::fromImage(cgfx_obj[x]).rect().size());
		label->setFlat(true);
		obj_button.push_back(label);

		obj_layout->addWidget(obj_button[x], (x / rows), (x % rows));
	}

	obj_set->setLayout(obj_layout);
}

/****** Updates the OBJ dumping window ******/
void gbe_cgfx::update_obj_window(int rows, int count)
{
	if(main_menu::gbe_plus == NULL) { return; }

	//Clear out previous widgets
	for(int x = 0; x < obj_button.size(); x++)
	{
		//Remove QWidgets from the layout
		//Manual memory management is not ideal, but QWidgets can't be copied (can't be pushed back to a vector)
		obj_layout->removeItem(obj_layout->itemAt(x));
		delete obj_button[x];
	}

	cgfx_obj.clear();
	obj_button.clear();

	//Generate the correct number of QImages
	for(int x = 0; x < count; x++)
	{
		cgfx_obj.push_back(grab_obj_data(x));

		//Wrap QImage in a QLabel
		QPushButton* label = new QPushButton;
		label->setIcon(QPixmap::fromImage(cgfx_obj[x]));
		label->setIconSize(QPixmap::fromImage(cgfx_obj[x]).rect().size());
		label->setFlat(true);

		obj_button.push_back(label);

		obj_layout->addWidget(obj_button[x], (x / rows), (x % rows));
	}

	obj_set->setLayout(obj_layout);
}

/****** Grabs an OBJ in VRAM and converts it to a QImage ******/
QImage gbe_cgfx::grab_obj_data(int obj_index)
{
	std::vector<u32> obj_pixels;

	//Determine if in 8x8 or 8x16 mode
	u8 obj_height = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x04) ? 16 : 8;

	//Setup palettes
	u8 obp[4][2];

	u8 value = main_menu::gbe_plus->ex_read_u8(REG_OBP0);
	obp[0][0] = value  & 0x3;
	obp[1][0] = (value >> 2) & 0x3;
	obp[2][0] = (value >> 4) & 0x3;
	obp[3][0] = (value >> 6) & 0x3;

	value = main_menu::gbe_plus->ex_read_u8(REG_OBP1);
	obp[0][1] = value  & 0x3;
	obp[1][1] = (value >> 2) & 0x3;
	obp[2][1] = (value >> 4) & 0x3;
	obp[3][1] = (value >> 6) & 0x3;

	//Grab palette number from OAM
	u8 pal_num = (main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 3) & 0x10) ? 1 : 0;
	u8 tile_num = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 2);

	//Grab OBJ tile addr from index
	u16 obj_tile_addr = 0x8000 + (tile_num << 4);

	//Pull data from VRAM into the ARGB vector
	for(int x = 0; x < obj_height; x++)
	{
		//Grab bytes from VRAM representing 8x1 pixel data
		u16 raw_data = (main_menu::gbe_plus->ex_read_u8(obj_tile_addr + 1) << 8) | main_menu::gbe_plus->ex_read_u8(obj_tile_addr);

		//Grab individual pixels
		for(int y = 7; y >= 0; y--)
		{
			u8 raw_pixel = ((raw_data >> 8) & (1 << y)) ? 2 : 0;
			raw_pixel |= (raw_data & (1 << y)) ? 1 : 0;

			switch(obp[raw_pixel][pal_num])
			{
				case 0: 
					obj_pixels.push_back(0xFFFFFFFF);
					break;

				case 1: 
					obj_pixels.push_back(0xFFC0C0C0);
					break;

				case 2: 
					obj_pixels.push_back(0xFF606060);
					break;

				case 3: 
					obj_pixels.push_back(0xFF000000);
					break;
			}
		}

		obj_tile_addr += 2;
	}

	QImage raw_image(8, obj_height, QImage::Format_ARGB32);	

	//Copy raw pixels to QImage
	for(int x = 0; x < obj_pixels.size(); x++)
	{
		raw_image.setPixel((x % 8), (x / 8), obj_pixels[x]);
	}

	//Scale final output to 64x64
	QImage final_image = raw_image.scaled(64, 64);
	return final_image;
}
