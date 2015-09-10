// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : cgfx.h
// Date : July 25, 2015
// Description : Custom graphics settings
//
// Dialog for various custom graphics options

#include "common/cgfx_common.h"
#include "cgfx.h"
#include "main_menu.h"

/****** General settings constructor ******/
gbe_cgfx::gbe_cgfx(QWidget *parent) : QDialog(parent)
{
	//Set up tabs
	tabs = new QTabWidget(this);
	
	QDialog* config_tab = new QDialog;
	QDialog* obj_tab = new QDialog;
	QDialog* bg_tab = new QDialog;
	QDialog* layers_tab = new QDialog;

	tabs->addTab(config_tab, tr("Configure"));
	tabs->addTab(obj_tab, tr("OBJ Tiles"));
	tabs->addTab(bg_tab, tr("BG Tiles"));
	tabs->addTab(layers_tab, tr("Layers"));

	tabs_button = new QDialogButtonBox(QDialogButtonBox::Close);

	//Setup Configure widgets
	QWidget* auto_dump_obj_set = new QWidget(config_tab);
	QLabel* auto_dump_obj_label = new QLabel("Automatically dump OBJ tiles", auto_dump_obj_set);
	auto_dump_obj = new QCheckBox(auto_dump_obj_set);

	QWidget* auto_dump_bg_set = new QWidget(config_tab);
	QLabel* auto_dump_bg_label = new QLabel("Automatically dump BG tiles", auto_dump_bg_set);
	auto_dump_bg = new QCheckBox(auto_dump_bg_set);

	obj_set = new QWidget(obj_tab);
	bg_set = new QWidget(bg_tab);
	layers_set = new QWidget(layers_tab);

	obj_layout = new QGridLayout;
	bg_layout = new QGridLayout;
	layers_layout = new QGridLayout;

	setup_obj_window(8, 40);
	setup_bg_window(8, 384);

	//Setup scroll areas
	QScrollArea* obj_scroll = new QScrollArea;
	obj_scroll->setWidget(obj_set);

	QScrollArea* bg_scroll = new QScrollArea;
	bg_scroll->setWidget(bg_set);

	//Setup Layers widgets
	QImage temp_img(320, 288, QImage::Format_ARGB32);
	temp_img.fill(qRgb(0, 0, 0));

	current_layer = new QLabel;
	current_layer->setPixmap(QPixmap::fromImage(temp_img));

	//Layer combo-box
	QWidget* select_set = new QWidget(layers_tab);
	QLabel* select_set_label = new QLabel("Graphics Layer : ");
	layer_select = new QComboBox(select_set);
	layer_select->addItem("Background");
	layer_select->addItem("Window");
	layer_select->addItem("OBJ");

	QHBoxLayout* layer_select_layout = new QHBoxLayout;
	layer_select_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	layer_select_layout->addWidget(select_set_label);
	layer_select_layout->addWidget(layer_select);
	select_set->setLayout(layer_select_layout);

	//Configure Tab layout
	QHBoxLayout* auto_dump_obj_layout = new QHBoxLayout;
	auto_dump_obj_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	auto_dump_obj_layout->addWidget(auto_dump_obj);
	auto_dump_obj_layout->addWidget(auto_dump_obj_label);
	auto_dump_obj_set->setLayout(auto_dump_obj_layout);

	QHBoxLayout* auto_dump_bg_layout = new QHBoxLayout;
	auto_dump_bg_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	auto_dump_bg_layout->addWidget(auto_dump_bg);
	auto_dump_bg_layout->addWidget(auto_dump_bg_label);
	auto_dump_bg_set->setLayout(auto_dump_bg_layout);

	QVBoxLayout* config_tab_layout = new QVBoxLayout;
	config_tab_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	config_tab_layout->addWidget(auto_dump_obj_set);
	config_tab_layout->addWidget(auto_dump_bg_set);
	config_tab->setLayout(config_tab_layout);

	//OBJ Tab layout
	QVBoxLayout* obj_tab_layout = new QVBoxLayout;
	obj_tab_layout->addWidget(obj_scroll);
	obj_tab->setLayout(obj_tab_layout);

	//BG Tab layout
	QVBoxLayout* bg_tab_layout = new QVBoxLayout;
	bg_tab_layout->addWidget(bg_scroll);
	bg_tab->setLayout(bg_tab_layout);

	//Layers Tab layout
	QVBoxLayout* layers_tab_layout = new QVBoxLayout;
	layers_tab_layout->addWidget(select_set);
	layers_tab_layout->addWidget(current_layer);
	layers_tab->setLayout(layers_tab_layout);
	
	//Final tab layout
	QVBoxLayout* main_layout = new QVBoxLayout;
	main_layout->addWidget(tabs);
	main_layout->addWidget(tabs_button);
	setLayout(main_layout);

	obj_signal = NULL;
	bg_signal = NULL;

	connect(tabs_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(tabs_button, SIGNAL(rejected()), this, SLOT(reject()));
	connect(tabs_button->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(close_cgfx()));
	connect(auto_dump_obj, SIGNAL(stateChanged(int)), this, SLOT(set_auto_obj()));
	connect(auto_dump_bg, SIGNAL(stateChanged(int)), this, SLOT(set_auto_bg()));

	estimated_palette.resize(384, 0);
	estimated_vram_bank.resize(384, 0);

	resize(800, 450);
	setWindowTitle(tr("Custom Graphics"));

	pause = false;
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

	//Setup signal mapper
	if(obj_signal != NULL) { delete obj_signal; }
	obj_signal = new QSignalMapper;

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

		//Map signals
		connect(obj_button[x], SIGNAL(clicked()), obj_signal, SLOT(map()));
		obj_signal->setMapping(obj_button[x], x);
	}

	obj_set->setLayout(obj_layout);
	connect(obj_signal, SIGNAL(mapped(int)), this, SLOT(dump_obj(int))) ;
}

/****** Grabs an OBJ in VRAM and converts it to a QImage - DMG Version ******/
QImage gbe_cgfx::grab_obj_data(int obj_index)
{
	//Grab DMG OBJs
	if(config::gb_type < 2) { return grab_dmg_obj_data(obj_index); }

	else { return grab_gbc_obj_data(obj_index); }
}

/****** Grabs an OBJ in VRAM and converts it to a QImage - DMG Version ******/
QImage gbe_cgfx::grab_dmg_obj_data(int obj_index)
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

	if(obj_height == 16) { tile_num &= ~0x1; }

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

/****** Grabs an OBJ in VRAM and converts it to a QImage - GBC Version ******/
QImage gbe_cgfx::grab_gbc_obj_data(int obj_index)
{
	std::vector<u32> obj_pixels;

	//Determine if in 8x8 or 8x16 mode
	u8 obj_height = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x04) ? 16 : 8;

	//Grab palette number from OAM
	u8 pal_num = (main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 3) & 0x7);
	u8 tile_num = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 2);

	if(obj_height == 16) { tile_num &= ~0x1; }
	
	//Grab VRAM banks
	u8 current_vram_bank = main_menu::gbe_plus->ex_read_u8(REG_VBK);
	u8 obj_vram_bank = (main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 3) & 0x8) ? 1 : 0;
	main_menu::gbe_plus->ex_write_u8(REG_VBK, obj_vram_bank);

	//Setup palettes
	u32 obp[4];

	u32* color = main_menu::gbe_plus->get_obj_palette(pal_num);

	obp[0] = *color; color += 8;
	obp[1] = *color; color += 8;
	obp[2] = *color; color += 8;
	obp[3] = *color; color += 8;

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

			obj_pixels.push_back(obp[raw_pixel]);
		}

		obj_tile_addr += 2;
	}

	//Return VRAM bank to normal
	main_menu::gbe_plus->ex_write_u8(REG_VBK, current_vram_bank);

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

/****** Sets up the BG dumping window ******/
void gbe_cgfx::setup_bg_window(int rows, int count)
{
	//Clear out previous widgets
	cgfx_bg.clear();
	bg_button.clear();

	//Generate the correct number of QImages
	for(int x = 0; x < count; x++)
	{
		cgfx_bg.push_back(QImage(64, 64, QImage::Format_ARGB32));
		
		//Set QImage to black first
		for(int pixel_counter = 0; pixel_counter < 4096; pixel_counter++)
		{
			cgfx_bg[x].setPixel((pixel_counter % 64), (pixel_counter / 64), 0xFF000000);
		}

		//Wrap QImage in a QLabel
		QPushButton* label = new QPushButton;
		label->setIcon(QPixmap::fromImage(cgfx_bg[x]));
		label->setIconSize(QPixmap::fromImage(cgfx_bg[x]).rect().size());
		label->setFlat(true);
		bg_button.push_back(label);

		bg_layout->addWidget(bg_button[x], (x / rows), (x % rows));
	}

	bg_set->setLayout(bg_layout);
}

/****** Updates the BG dumping window ******/
void gbe_cgfx::update_bg_window(int rows, int count)
{
	if(main_menu::gbe_plus == NULL) { return; }

	//Clear out previous widgets
	for(int x = 0; x < bg_button.size(); x++)
	{
		//Remove QWidgets from the layout
		//Manual memory management is not ideal, but QWidgets can't be copied (can't be pushed back to a vector)
		bg_layout->removeItem(bg_layout->itemAt(x));
		delete bg_button[x];
	}

	cgfx_bg.clear();
	bg_button.clear();

	//Setup signal mapper
	if(bg_signal != NULL) { delete bg_signal; }
	bg_signal = new QSignalMapper;
	
	//Generate the correct number of QImages
	for(int x = 0; x < count; x++)
	{
		cgfx_bg.push_back(grab_bg_data(x));

		//Wrap QImage in a QLabel
		QPushButton* label = new QPushButton;
		label->setIcon(QPixmap::fromImage(cgfx_bg[x]));
		label->setIconSize(QPixmap::fromImage(cgfx_bg[x]).rect().size());
		label->setFlat(true);

		bg_button.push_back(label);
		bg_layout->addWidget(bg_button[x], (x / rows), (x % rows));

		//Map signals
		connect(bg_button[x], SIGNAL(clicked()), bg_signal, SLOT(map()));
		bg_signal->setMapping(bg_button[x], x);
	}

	bg_set->setLayout(bg_layout);
	connect(bg_signal, SIGNAL(mapped(int)), this, SLOT(dump_bg(int))) ;
}

/****** Grabs a BG tile in VRAM and converts it to a QImage ******/
QImage gbe_cgfx::grab_bg_data(int bg_index)
{
	//Grab DMG BG tiles
	if(config::gb_type < 2) { return grab_dmg_bg_data(bg_index); }

	else { return grab_gbc_bg_data(bg_index); }
}

/****** Grabs a BG tile in VRAM and converts it to a QImage - DMG version ******/
QImage gbe_cgfx::grab_dmg_bg_data(int bg_index)
{
	std::vector<u32> bg_pixels;

	//Setup palette
	u8 bgp[4];

	u8 value = main_menu::gbe_plus->ex_read_u8(REG_BGP);
	bgp[0] = value  & 0x3;
	bgp[1] = (value >> 2) & 0x3;
	bgp[2] = (value >> 4) & 0x3;
	bgp[3] = (value >> 6) & 0x3;

	//Grab bg tile addr from index
	u16 tile_num = bg_index;
	u16 bg_tile_addr = 0x8000 + (tile_num << 4);

	//Pull data from VRAM into the ARGB vector
	for(int x = 0; x < 8; x++)
	{
		//Grab bytes from VRAM representing 8x1 pixel data
		u16 raw_data = (main_menu::gbe_plus->ex_read_u8(bg_tile_addr + 1) << 8) | main_menu::gbe_plus->ex_read_u8(bg_tile_addr);

		//Grab individual pixels
		for(int y = 7; y >= 0; y--)
		{
			u8 raw_pixel = ((raw_data >> 8) & (1 << y)) ? 2 : 0;
			raw_pixel |= (raw_data & (1 << y)) ? 1 : 0;

			switch(bgp[raw_pixel])
			{
				case 0: 
					bg_pixels.push_back(0xFFFFFFFF);
					break;

				case 1: 
					bg_pixels.push_back(0xFFC0C0C0);
					break;

				case 2: 
					bg_pixels.push_back(0xFF606060);
					break;

				case 3: 
					bg_pixels.push_back(0xFF000000);
					break;
			}
		}

		bg_tile_addr += 2;
	}

	QImage raw_image(8, 8, QImage::Format_ARGB32);	

	//Copy raw pixels to QImage
	for(int x = 0; x < bg_pixels.size(); x++)
	{
		raw_image.setPixel((x % 8), (x / 8), bg_pixels[x]);
	}

	//Scale final output to 64x64
	QImage final_image = raw_image.scaled(64, 64);
	return final_image;
}

/****** Grabs a BG tile and converts it to a QImage - GBC Version ******/
QImage gbe_cgfx::grab_gbc_bg_data(int bg_index)
{
	std::vector<u32> bg_pixels;

	u8 pal_num = 0;
	int original_bg_index = bg_index;

	//Grab BG tile addr from index
	u16 tile_num = bg_index;
	u16 bg_tile_addr = 0x8000 + (tile_num << 4);

	//Estimate tile numbers
	if(bg_index > 255) { bg_index -= 255; }

	//Estimate the tilemap
	u16 tilemap_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x8) ? 0x9C00 : 0x9800; 

	//Grab VRAM banks
	u8 current_vram_bank = main_menu::gbe_plus->ex_read_u8(REG_VBK);
	u8 bg_vram_bank = 0;
	main_menu::gbe_plus->ex_write_u8(REG_VBK, 0);

	//Estimate the color palette
	for(u16 x = tilemap_addr; x < (tilemap_addr + 0x400); x++)
	{
		u8 map_entry = main_menu::gbe_plus->ex_read_u8(x);

		if(map_entry == bg_index)
		{
			main_menu::gbe_plus->ex_write_u8(REG_VBK, 1);
			pal_num = (main_menu::gbe_plus->ex_read_u8(x) & 0x7);
			bg_vram_bank = (main_menu::gbe_plus->ex_read_u8(x) & 0x8) ? 1 : 0;

			estimated_vram_bank[original_bg_index] = bg_vram_bank;
			estimated_palette[original_bg_index] = pal_num;
		}
	}

	main_menu::gbe_plus->ex_write_u8(REG_VBK, bg_vram_bank);

	//Setup palettes
	u32 bgp[4];

	u32* color = main_menu::gbe_plus->get_bg_palette(pal_num);

	bgp[0] = *color; color += 8;
	bgp[1] = *color; color += 8;
	bgp[2] = *color; color += 8;
	bgp[3] = *color; color += 8;

	//Pull data from VRAM into the ARGB vector
	for(int x = 0; x < 8; x++)
	{
		//Grab bytes from VRAM representing 8x1 pixel data
		u16 raw_data = (main_menu::gbe_plus->ex_read_u8(bg_tile_addr + 1) << 8) | main_menu::gbe_plus->ex_read_u8(bg_tile_addr);

		//Grab individual pixels
		for(int y = 7; y >= 0; y--)
		{
			u8 raw_pixel = ((raw_data >> 8) & (1 << y)) ? 2 : 0;
			raw_pixel |= (raw_data & (1 << y)) ? 1 : 0;

			bg_pixels.push_back(bgp[raw_pixel]);
		}

		bg_tile_addr += 2;
	}

	//Return VRAM bank to normal
	main_menu::gbe_plus->ex_write_u8(REG_VBK, current_vram_bank);

	QImage raw_image(8, 8, QImage::Format_ARGB32);	

	//Copy raw pixels to QImage
	for(int x = 0; x < bg_pixels.size(); x++)
	{
		raw_image.setPixel((x % 8), (x / 8), bg_pixels[x]);
	}

	//Scale final output to 64x64
	QImage final_image = raw_image.scaled(64, 64);
	return final_image;
}
/****** Closes the CGFX window ******/
void gbe_cgfx::closeEvent(QCloseEvent* event) { close_cgfx(); }

/****** Closes the CGFX window ******/
void gbe_cgfx::close_cgfx() { pause = false; config::pause_emu = false; }

/****** Dumps the selected OBJ ******/
void gbe_cgfx::dump_obj(int obj_index) { main_menu::gbe_plus->dump_obj(obj_index); }

/****** Dumps the selected BG ******/
void gbe_cgfx::dump_bg(int bg_index) 
{
	if(config::gb_type == 2)
	{
		cgfx::gbc_bg_color_pal = estimated_palette[bg_index];
		cgfx::gbc_bg_vram_bank = estimated_vram_bank[bg_index];
	}

	main_menu::gbe_plus->dump_bg(bg_index);
}

/****** Toggles automatic dumping of OBJ tiles ******/
void gbe_cgfx::set_auto_obj()
{
	if(auto_dump_obj->isChecked()) { cgfx::auto_dump_obj = true; }
	else { cgfx::auto_dump_obj = false; }
}

/****** Toggles automatic dumping of BG tiles ******/
void gbe_cgfx::set_auto_bg()
{
	if(auto_dump_bg->isChecked()) { cgfx::auto_dump_bg = true; }
	else { cgfx::auto_dump_bg = false; }
}

/****** Draws the DMG BG layer ******/
void gbe_cgfx::draw_dmg_bg()
{
	if(main_menu::gbe_plus == NULL) { return; }

	std::vector<u32> bg_pixels;
	u32 scanline_pixel_buffer[256];

	//Setup palette
	u8 bgp[4];

	u8 value = main_menu::gbe_plus->ex_read_u8(REG_BGP);
	bgp[0] = value  & 0x3;
	bgp[1] = (value >> 2) & 0x3;
	bgp[2] = (value >> 4) & 0x3;
	bgp[3] = (value >> 6) & 0x3;

	//Determine BG Map & Tile address
	u16 bg_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x8) ? 0x9C00 : 0x9800;
	u16 bg_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;

	for(u8 current_scanline = 0; current_scanline < 144; current_scanline++)
	{
		//Determine where to start drawing
		u8 rendered_scanline = current_scanline + main_menu::gbe_plus->ex_read_u8(REG_SY);
		u8 scanline_pixel_counter = (0x100 - main_menu::gbe_plus->ex_read_u8(REG_SX));

		//Determine which tiles we should generate to get the scanline data - integer division ftw :p
		u16 tile_lower_range = (rendered_scanline / 8) * 32;
		u16 tile_upper_range = tile_lower_range + 32;

		//Determine which line of the tiles to generate pixels for this scanline
		u8 tile_line = rendered_scanline % 8;

		//Generate background pixel data for selected tiles
		for(int x = tile_lower_range; x < tile_upper_range; x++)
		{
			u8 map_entry = main_menu::gbe_plus->ex_read_u8(bg_map_addr + x);
			u8 tile_pixel = 0;

			//Convert tile number to signed if necessary
			if(bg_tile_addr == 0x8800) 
			{
				if(map_entry <= 127) { map_entry += 128; }
				else { map_entry -= 128; }
			}

			//Calculate the address of the 8x1 pixel data based on map entry
			u16 tile_addr = (bg_tile_addr + (map_entry << 4) + (tile_line << 1));

			//Grab bytes from VRAM representing 8x1 pixel data
			u16 tile_data = (main_menu::gbe_plus->ex_read_u8(tile_addr + 1) << 8) | main_menu::gbe_plus->ex_read_u8(tile_addr);

			for(int y = 7; y >= 0; y--)
			{
				//Calculate raw value of the tile's pixel
				tile_pixel = ((tile_data >> 8) & (1 << y)) ? 2 : 0;
				tile_pixel |= (tile_data & (1 << y)) ? 1 : 0;
				
				switch(bgp[tile_pixel])
				{
					case 0: 
						scanline_pixel_buffer[scanline_pixel_counter++] = config::DMG_BG_PAL[0];
						break;

					case 1: 
						scanline_pixel_buffer[scanline_pixel_counter++] = config::DMG_BG_PAL[1];
						break;

					case 2: 
						scanline_pixel_buffer[scanline_pixel_counter++] = config::DMG_BG_PAL[2];
						break;

					case 3: 
						scanline_pixel_buffer[scanline_pixel_counter++] = config::DMG_BG_PAL[3];
						break;
				}
			}
		}

		//Copy scanline buffer to BG buffer
		for(u8 pixel_counter = 0; pixel_counter < 160; pixel_counter++) { bg_pixels.push_back(scanline_pixel_buffer[pixel_counter]); }
	}

	QImage raw_image(160, 144, QImage::Format_ARGB32);	

	//Copy raw pixels to QImage
	for(int x = 0; x < bg_pixels.size(); x++)
	{
		raw_image.setPixel((x % 160), (x / 160), bg_pixels[x]);
	}

	raw_image = raw_image.scaled(320, 288);

	//Set label Pixmap
	current_layer->setPixmap(QPixmap::fromImage(raw_image));
}
