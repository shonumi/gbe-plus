// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : cgfx.h
// Date : July 25, 2015
// Description : Custom graphics settings
//
// Dialog for various custom graphics options

#include "common/cgfx_common.h"
#include "common/util.h"
#include "cgfx.h"
#include "main_menu.h"
#include "render.h"

#include <fstream>

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

	QWidget* blank_set = new QWidget(config_tab);
	QLabel* blank_label = new QLabel("Ignore blank/empty tiles when dumping", blank_set);
	blank = new QCheckBox(blank_set);

	QWidget* advanced_set = new QWidget(config_tab);
	QLabel* advanced_label = new QLabel("Use advanced menu", blank_set);
	advanced = new QCheckBox(advanced_set);

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

	QImage temp_pix(128, 128, QImage::Format_ARGB32);
	temp_pix.fill(qRgb(0, 0, 0));

	current_layer = new QLabel;
	current_layer->setPixmap(QPixmap::fromImage(temp_img));
	current_layer->setMouseTracking(true);
	current_layer->installEventFilter(this);

	current_tile = new QLabel;
	current_tile->setPixmap(QPixmap::fromImage(temp_pix));

	//Layer Label Info
	QWidget* layer_info = new QWidget;

	tile_id = new QLabel("Tile ID : ");
	tile_addr = new QLabel("Tile Address : ");
	tile_size = new QLabel("Tile Size : ");
	h_v_flip = new QLabel("H-Flip :    V Flip : ");
	tile_palette = new QLabel("Tile Palette : ");
	hash_text = new QLabel("Tile Hash : ");

	QVBoxLayout* layer_info_layout = new QVBoxLayout;
	layer_info_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	layer_info_layout->addWidget(tile_id);
	layer_info_layout->addWidget(tile_addr);
	layer_info_layout->addWidget(tile_size);
	layer_info_layout->addWidget(h_v_flip);
	layer_info_layout->addWidget(tile_palette);
	layer_info_layout->addWidget(hash_text);
	layer_info->setLayout(layer_info_layout);

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

	//Layer section selector - X
	QWidget* section_x_set = new QWidget(layers_tab);
	QLabel* section_x_label = new QLabel("Tile X :\t");
	QLabel* section_w_label = new QLabel("Tile W :\t");
	
	rect_x = new QSpinBox(section_x_set);
	rect_x->setRange(0, 31);

	rect_w = new QSpinBox(section_x_set);
	rect_w->setRange(0, 31);

	QHBoxLayout* section_x_layout = new QHBoxLayout;
	section_x_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	section_x_layout->addWidget(section_x_label);
	section_x_layout->addWidget(rect_x);
	section_x_layout->addWidget(section_w_label);
	section_x_layout->addWidget(rect_w);
	section_x_set->setLayout(section_x_layout);

	//Layer section selector - Y
	QWidget* section_y_set = new QWidget(layers_tab);
	QLabel* section_y_label = new QLabel("Tile Y :\t");
	QLabel* section_h_label = new QLabel("Tile H :\t");
	
	rect_y = new QSpinBox(section_y_set);
	rect_y->setRange(0, 31);

	rect_h = new QSpinBox(section_y_set);
	rect_h->setRange(0, 31);

	QHBoxLayout* section_y_layout = new QHBoxLayout;
	section_y_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	section_y_layout->addWidget(section_y_label);
	section_y_layout->addWidget(rect_y);
	section_y_layout->addWidget(section_h_label);
	section_y_layout->addWidget(rect_h);
	section_y_set->setLayout(section_y_layout);

	//Layer GroupBox for section
	QGroupBox* section_set = new QGroupBox(tr("Dump Selection"));
	QPushButton* dump_section_button = new QPushButton("Dump Current Selection");
	QVBoxLayout* section_final_layout = new QVBoxLayout;
	section_final_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	section_final_layout->addWidget(section_x_set);
	section_final_layout->addWidget(section_y_set);
	section_final_layout->addWidget(dump_section_button);
	section_set->setLayout(section_final_layout);

	//Configure Tab layout
	QHBoxLayout* advanced_layout = new QHBoxLayout;
	advanced_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	advanced_layout->addWidget(advanced);
	advanced_layout->addWidget(advanced_label);
	advanced_set->setLayout(advanced_layout);

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

	QHBoxLayout* blank_layout = new QHBoxLayout;
	blank_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	blank_layout->addWidget(blank);
	blank_layout->addWidget(blank_label);
	blank_set->setLayout(blank_layout);

	QVBoxLayout* config_tab_layout = new QVBoxLayout;
	config_tab_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	config_tab_layout->addWidget(advanced_set);
	config_tab_layout->addWidget(auto_dump_obj_set);
	config_tab_layout->addWidget(auto_dump_bg_set);
	config_tab_layout->addWidget(blank_set);
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
	QGridLayout* layers_tab_layout = new QGridLayout;
	layers_tab_layout->addWidget(select_set, 0, 0, 1, 1);
	layers_tab_layout->addWidget(current_layer, 1, 0, 1, 1);

	layers_tab_layout->addWidget(current_tile, 1, 1, 1, 1);
	layers_tab_layout->addWidget(layer_info, 0, 1, 1, 1);
	layers_tab_layout->addWidget(section_set, 2, 1, 1, 1);
	layers_tab->setLayout(layers_tab_layout);
	
	//Final tab layout
	QVBoxLayout* main_layout = new QVBoxLayout;
	main_layout->addWidget(tabs);
	main_layout->addWidget(tabs_button);
	setLayout(main_layout);

	obj_signal = NULL;
	bg_signal = NULL;

	data_folder = new data_dialog;

	connect(tabs_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(tabs_button, SIGNAL(rejected()), this, SLOT(reject()));
	connect(tabs_button->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(close_cgfx()));
	connect(auto_dump_obj, SIGNAL(stateChanged(int)), this, SLOT(set_auto_obj()));
	connect(auto_dump_bg, SIGNAL(stateChanged(int)), this, SLOT(set_auto_bg()));
	connect(blank, SIGNAL(stateChanged(int)), this, SLOT(set_blanks()));
	connect(layer_select, SIGNAL(currentIndexChanged(int)), this, SLOT(layer_change()));
	connect(data_folder, SIGNAL(accepted()), this, SLOT(select_folder()));
	connect(data_folder, SIGNAL(rejected()), this, SLOT(reject_folder()));
	connect(rect_x, SIGNAL(valueChanged(int)), this, SLOT(update_selection()));
	connect(rect_y, SIGNAL(valueChanged(int)), this, SLOT(update_selection()));
	connect(rect_w, SIGNAL(valueChanged(int)), this, SLOT(update_selection()));
	connect(rect_h, SIGNAL(valueChanged(int)), this, SLOT(update_selection()));
	connect(dump_section_button, SIGNAL(clicked()), this, SLOT(dump_selection()));

	//CGFX advanced dumping pop-up box
	advanced_box = new QDialog();
	advanced_box->resize(500, 250);
	advanced_box->setWindowTitle("Advanced Tile Dumping");
	advanced_box->hide();

	//Advanced menu widgets
	QWidget* dest_set = new QWidget(advanced_box);
	dest_label = new QLabel("Destination Folder :  ");
	dest_browse = new QPushButton("Browse");
	dest_folder = new QLineEdit(dest_set);
	dest_folder->setReadOnly(true);
	dest_label->resize(100, dest_label->height());

	QWidget* name_set = new QWidget(advanced_box);
	name_label = new QLabel("Tile Name :  ");
	name_browse = new QPushButton("Browse");
	dest_name = new QLineEdit(name_set);

	QWidget* ext_vram_set = new QWidget(advanced_box);
	QLabel* ext_vram_label = new QLabel("EXT_VRAM_ADDR", ext_vram_set);
	ext_vram = new QCheckBox(ext_vram_set);

	QWidget* ext_bright_set = new QWidget(advanced_box);
	QLabel* ext_bright_label = new QLabel("EXT_AUTO_BRIGHT", ext_bright_set);
	ext_bright = new QCheckBox(ext_bright_set);

	dump_button = new QPushButton("Dump Tile", advanced_box);
	cancel_button = new QPushButton("Cancel", advanced_box);

	advanced_buttons = new QDialogButtonBox(advanced_box);
	advanced_buttons->setOrientation(Qt::Horizontal);
	advanced_buttons->addButton(dump_button, QDialogButtonBox::ActionRole);
	advanced_buttons->addButton(cancel_button, QDialogButtonBox::ActionRole);

	//Advanced menu layouts
	QHBoxLayout* dest_layout = new QHBoxLayout;
	dest_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	dest_layout->addWidget(dest_label);
	dest_layout->addWidget(dest_folder);
	dest_layout->addWidget(dest_browse);
	dest_set->setLayout(dest_layout);

	QHBoxLayout* name_layout = new QHBoxLayout;
	name_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	name_layout->addWidget(name_label);
	name_layout->addWidget(dest_name);
	name_layout->addWidget(name_browse);
	name_set->setLayout(name_layout);

	QHBoxLayout* ext_vram_layout = new QHBoxLayout;
	ext_vram_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	ext_vram_layout->addWidget(ext_vram);
	ext_vram_layout->addWidget(ext_vram_label);
	ext_vram_set->setLayout(ext_vram_layout);

	QHBoxLayout* ext_bright_layout = new QHBoxLayout;
	ext_bright_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	ext_bright_layout->addWidget(ext_bright);
	ext_bright_layout->addWidget(ext_bright_label);
	ext_bright_set->setLayout(ext_bright_layout);

	QVBoxLayout* advanced_box_layout = new QVBoxLayout;
	advanced_box_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	advanced_box_layout->addWidget(dest_set);
	advanced_box_layout->addWidget(name_set);
	advanced_box_layout->addWidget(ext_vram_set);
	advanced_box_layout->addWidget(ext_bright_set);
	advanced_box_layout->addWidget(advanced_buttons);
	advanced_box->setLayout(advanced_box_layout);

	connect(dump_button, SIGNAL(clicked()), this, SLOT(write_manifest_entry()));
	connect(cancel_button, SIGNAL(clicked()), this, SLOT(close_advanced()));
	connect(dest_browse, SIGNAL(clicked()), this, SLOT(browse_advanced_dir()));
	connect(name_browse, SIGNAL(clicked()), this, SLOT(browse_advanced_file()));

	estimated_palette.resize(384, 0);
	estimated_vram_bank.resize(384, 0);

	resize(800, 450);
	setWindowTitle(tr("Custom Graphics"));

	dump_type = 0;
	advanced_index = 0;
	last_custom_path = "";

	min_x_rect = min_y_rect = max_x_rect = max_y_rect = 255;

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
	connect(obj_signal, SIGNAL(mapped(int)), this, SLOT(show_advanced_obj(int))) ;
}

/****** Optionally shows the advanced menu before dumping - OBJ version ******/
void gbe_cgfx::show_advanced_obj(int index)
{
	if(advanced->isChecked()) 
	{
		QString path = QString::fromStdString(cgfx::dump_obj_path);
		if(last_custom_path != "") { path = QString::fromStdString(last_custom_path); }

		//Set the default destination
		if(!path.isNull())
		{
			//Use relative paths
			QDir folder(QString::fromStdString(config::data_path));
			path = folder.relativeFilePath(path);

			//Make sure path is complete, e.g. has the correct separator at the end
			//Qt doesn't append this automatically
			std::string temp_str = path.toStdString();
			std::string temp_chr = "";
			temp_chr = temp_str[temp_str.length() - 1];

			if((temp_chr != "/") && (temp_chr != "\\")) { path.append("/"); }
			path = QDir::toNativeSeparators(path);

			dest_folder->setText(path);
		}

		cgfx::dump_name = "";
		dest_name->setText("");

		dump_type = 1;
		advanced_index = index;
		advanced_box->show();
	}

	else { dump_obj(index); }
}

/****** Optionally shows the advanced menu before dumping - OBJ version ******/
void gbe_cgfx::show_advanced_bg(int index)
{
	//When estimating dumpable tiles, use estimated palettes + vram_banks
	if(config::gb_type == 2)
	{
		cgfx::gbc_bg_color_pal = estimated_palette[index];
		cgfx::gbc_bg_vram_bank = estimated_vram_bank[index];
	}

	//But if CGFX signals the emulator has a specific tile, use provided attributes
	//Palette and VRAM bank are already set, so no estimation required
	else if(config::gb_type == 10) { config::gb_type = 2; }

	if(advanced->isChecked()) 
	{
		QString path = QString::fromStdString(cgfx::dump_bg_path);
		if(last_custom_path != "") { path = QString::fromStdString(last_custom_path); }

		//Set the default destination
		if(!path.isNull())
		{
			//Use relative paths
			QDir folder(QString::fromStdString(config::data_path));
			path = folder.relativeFilePath(path);

			//Make sure path is complete, e.g. has the correct separator at the end
			//Qt doesn't append this automatically
			std::string temp_str = path.toStdString();
			std::string temp_chr = "";
			temp_chr = temp_str[temp_str.length() - 1];

			if((temp_chr != "/") && (temp_chr != "\\")) { path.append("/"); }
			path = QDir::toNativeSeparators(path);

			dest_folder->setText(path);
		}

		cgfx::dump_name = "";
		dest_name->setText("");

		dump_type = 0;
		advanced_index = index;
		advanced_box->show();
	}

	else { dump_bg(index); }
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
	if(obj_height == 8)
	{
		QImage final_image = raw_image.scaled(64, 64);
		return final_image;
	}

	//Scale final output to 32x64
	else
	{
		QImage final_image = raw_image.scaled(32, 64);
		return final_image;
	}
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
	if(obj_height == 8)
	{
		QImage final_image = raw_image.scaled(64, 64);
		return final_image;
	}

	//Scale final output to 32x64
	else
	{
		QImage final_image = raw_image.scaled(32, 64);
		return final_image;
	}
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
	connect(bg_signal, SIGNAL(mapped(int)), this, SLOT(show_advanced_bg(int))) ;
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
void gbe_cgfx::close_cgfx()
{
	if(!qt_gui::draw_surface->dmg_debugger->pause) { qt_gui::draw_surface->findChild<QAction*>("pause_action")->setEnabled(true); }

	pause = false;
	config::pause_emu = false;
	last_custom_path = "";
	advanced_box->hide();
}

/****** Closes the Advanced menu ******/
void gbe_cgfx::close_advanced()
{
	advanced_box->hide();
	cgfx::dump_name = "";
}

/****** Dumps the selected OBJ ******/
void gbe_cgfx::dump_obj(int obj_index) { main_menu::gbe_plus->dump_obj(obj_index); }

/****** Dumps the selected BG ******/
void gbe_cgfx::dump_bg(int bg_index) { main_menu::gbe_plus->dump_bg(bg_index); }

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

/****** Toggles whether to ignore blank dumps ******/
void gbe_cgfx::set_blanks()
{
	if(blank->isChecked()) { cgfx::ignore_blank_dumps = true; }
	else { cgfx::ignore_blank_dumps = false; }
}

/****** Changes the current viewable layer for dumping ******/
void gbe_cgfx::layer_change()
{
	//Draw DMG layers
	if(config::gb_type < 2)
	{
		switch(layer_select->currentIndex())
		{
			case 0: draw_dmg_bg(); break;
			case 1: draw_dmg_win(); break;
			case 2: draw_dmg_obj(); break;
		}
	}

	//Draw GBC layers
	else
	{
		switch(layer_select->currentIndex())
		{
			case 0: draw_gbc_bg(); break;
			case 1: draw_gbc_win(); break;
			case 2: draw_gbc_obj(); break;
		}
	}
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
			//Determine if this tile needs to be highlighted for selection dumping
			bool highlight = false;

			if(((scanline_pixel_counter / 8) >= min_x_rect) && ((scanline_pixel_counter / 8) <= max_x_rect)
			&& ((current_scanline / 8) >= min_y_rect) && ((current_scanline / 8) <= max_y_rect))
			{
				highlight = true;
			}

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

				//Highlight selected pixels, if applicable
				if(highlight)
				{
					u8 temp = scanline_pixel_counter - 1;
					scanline_pixel_buffer[temp] += 0x00700000;
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

/****** Draws the GBC BG layer ******/
void gbe_cgfx::draw_gbc_bg()
{
	if(main_menu::gbe_plus == NULL) { return; }

	std::vector<u32> bg_pixels;
	u32 scanline_pixel_buffer[256];

	//8 pixel (horizontal+vertical) flipping lookup generation
	u8 flip_8[8];
	for(int x = 0; x < 8; x++) { flip_8[x] = (7 - x); }

	//Determine BG Map & Tile address
	u16 bg_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x8) ? 0x9C00 : 0x9800;
	u16 bg_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;

	//Grab VRAM banks
	u8 current_vram_bank = main_menu::gbe_plus->ex_read_u8(REG_VBK);

	for(u8 current_scanline = 0; current_scanline < 144; current_scanline++)
	{
		//Determine where to start drawing
		u8 rendered_scanline = current_scanline + main_menu::gbe_plus->ex_read_u8(REG_SY);
		u8 scanline_pixel_counter = (0x100 - main_menu::gbe_plus->ex_read_u8(REG_SX));

		//Determine which tiles we should generate to get the scanline data - integer division ftw :p
		u16 tile_lower_range = (rendered_scanline / 8) * 32;
		u16 tile_upper_range = tile_lower_range + 32;

		//Generate background pixel data for selected tiles
		for(int x = tile_lower_range; x < tile_upper_range; x++)
		{
			//Determine if this tile needs to be highlighted for selection dumping
			bool highlight = false;

			if(((scanline_pixel_counter / 8) >= min_x_rect) && ((scanline_pixel_counter / 8) <= max_x_rect)
			&& ((current_scanline / 8) >= min_y_rect) && ((current_scanline / 8) <= max_y_rect))
			{
				highlight = true;
			}

			//Determine which line of the tiles to generate pixels for this scanline
			u8 tile_line = rendered_scanline % 8;

			//Read the tile number
			main_menu::gbe_plus->ex_write_u8(REG_VBK, 0);
			u8 map_entry = main_menu::gbe_plus->ex_read_u8(bg_map_addr + x);
			
			//Read the BG attributes
			main_menu::gbe_plus->ex_write_u8(REG_VBK, 1);
			u8 bg_attribute = main_menu::gbe_plus->ex_read_u8(bg_map_addr + x);
			u8 pal_num = (bg_attribute & 0x7);
			u8 vram_bank = (bg_attribute & 0x8) ? 1 : 0;

			//Setup palettes
			u32 bgp[4];

			u32* color = main_menu::gbe_plus->get_bg_palette(pal_num);

			bgp[0] = *color; color += 8;
			bgp[1] = *color; color += 8;
			bgp[2] = *color; color += 8;
			bgp[3] = *color; color += 8;

			u8 tile_pixel = 0;

			//Convert tile number to signed if necessary
			if(bg_tile_addr == 0x8800) 
			{
				if(map_entry <= 127) { map_entry += 128; }
				else { map_entry -= 128; }
			}

			//Account for vertical flipping
			if(bg_attribute & 0x40) { tile_line = flip_8[tile_line]; }

			//Calculate the address of the 8x1 pixel data based on map entry
			u16 tile_addr = (bg_tile_addr + (map_entry << 4) + (tile_line << 1));

			//Grab bytes from VRAM representing 8x1 pixel data
			main_menu::gbe_plus->ex_write_u8(REG_VBK, vram_bank);
			u16 tile_data = (main_menu::gbe_plus->ex_read_u8(tile_addr + 1) << 8) | main_menu::gbe_plus->ex_read_u8(tile_addr);

			for(int y = 7; y >= 0; y--)
			{
				//Calculate raw value of the tile's pixel
				if(bg_attribute & 0x20) 
				{
					tile_pixel = ((tile_data >> 8) & (1 << flip_8[y])) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << flip_8[y])) ? 1 : 0;
				}

				else 
				{
					tile_pixel = ((tile_data >> 8) & (1 << y)) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << y)) ? 1 : 0;
				}
				
				scanline_pixel_buffer[scanline_pixel_counter++] = bgp[tile_pixel];

				//Highlight selected pixels, if applicable
				if(highlight)
				{
					u8 temp = scanline_pixel_counter - 1;
					scanline_pixel_buffer[temp] += 0x00700000;
				}
			}

			main_menu::gbe_plus->ex_write_u8(REG_VBK, current_vram_bank);
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

/****** Draws the DMG Window layer ******/
void gbe_cgfx::draw_dmg_win()
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
	u16 win_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x40) ? 0x9C00 : 0x9800;
	u16 bg_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;

	for(u8 current_scanline = 0; current_scanline < 144; current_scanline++)
	{
		//Determine where to start drawing
		u8 rendered_scanline = current_scanline - main_menu::gbe_plus->ex_read_u8(REG_WY);
		u8 scanline_pixel_counter = main_menu::gbe_plus->ex_read_u8(REG_WX);
		scanline_pixel_counter = (scanline_pixel_counter < 7) ? 0 : (scanline_pixel_counter - 7); 

		bool draw_line = true;

		//Determine if scanline is within window, if not abort rendering
		if(current_scanline < main_menu::gbe_plus->ex_read_u8(REG_WY)) 
		{
			for(u8 pixel_counter = 0; pixel_counter < 160; pixel_counter++) { bg_pixels.push_back(0xFFFFFFFF); }
			draw_line = false;
		}

		//Determine which tiles we should generate to get the scanline data - integer division ftw :p
		u16 tile_lower_range = (rendered_scanline / 8) * 32;
		u16 tile_upper_range = tile_lower_range + 32;

		//Determine which line of the tiles to generate pixels for this scanline
		u8 tile_line = rendered_scanline % 8;

		//Generate background pixel data for selected tiles
		for(int x = tile_lower_range; x < tile_upper_range; x++)
		{
			//Determine if this tile needs to be highlighted for selection dumping
			bool highlight = false;

			if(((scanline_pixel_counter / 8) >= min_x_rect) && ((scanline_pixel_counter / 8) <= max_x_rect)
			&& ((current_scanline / 8) >= min_y_rect) && ((current_scanline / 8) <= max_y_rect))
			{
				highlight = true;
			}

			u8 map_entry = main_menu::gbe_plus->ex_read_u8(win_map_addr + x);
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

				if(scanline_pixel_counter >= 160) { scanline_pixel_buffer[scanline_pixel_counter++] = 0xFFFFFFFF; }

				else
				{
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

				//Highlight selected pixels, if applicable
				if(highlight)
				{
					u8 temp = scanline_pixel_counter - 1;
					scanline_pixel_buffer[temp] += 0x00700000;
				}
			}
		}

		//Copy scanline buffer to BG buffer
		if(draw_line)
		{
			for(u8 pixel_counter = 0; pixel_counter < 160; pixel_counter++)
			{
				bg_pixels.push_back(scanline_pixel_buffer[pixel_counter]);
			}
		}
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

/****** Draws the GBC Window layer ******/
void gbe_cgfx::draw_gbc_win()
{
	if(main_menu::gbe_plus == NULL) { return; }

	std::vector<u32> bg_pixels;
	u32 scanline_pixel_buffer[256];

	//8 pixel (horizontal+vertical) flipping lookup generation
	u8 flip_8[8];
	for(int x = 0; x < 8; x++) { flip_8[x] = (7 - x); }

	//Determine BG Map & Tile address
	u16 win_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x40) ? 0x9C00 : 0x9800;
	u16 bg_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;

	//Grab VRAM banks
	u8 current_vram_bank = main_menu::gbe_plus->ex_read_u8(REG_VBK);

	for(u8 current_scanline = 0; current_scanline < 144; current_scanline++)
	{
		//Determine where to start drawing
		u8 rendered_scanline = current_scanline - main_menu::gbe_plus->ex_read_u8(REG_WY);
		u8 scanline_pixel_counter = main_menu::gbe_plus->ex_read_u8(REG_WX);
		scanline_pixel_counter = (scanline_pixel_counter < 7) ? 0 : (scanline_pixel_counter - 7); 

		bool draw_line = true;

		//Determine if scanline is within window, if not abort rendering
		if(current_scanline < main_menu::gbe_plus->ex_read_u8(REG_WY)) 
		{
			for(u8 pixel_counter = 0; pixel_counter < 160; pixel_counter++) { bg_pixels.push_back(0xFFFFFFFF); }
			draw_line = false;
		}

		//Determine which tiles we should generate to get the scanline data - integer division ftw :p
		u16 tile_lower_range = (rendered_scanline / 8) * 32;
		u16 tile_upper_range = tile_lower_range + 32;

		//Generate background pixel data for selected tiles
		for(int x = tile_lower_range; x < tile_upper_range; x++)
		{
			//Determine if this tile needs to be highlighted for selection dumping
			bool highlight = false;

			if(((scanline_pixel_counter / 8) >= min_x_rect) && ((scanline_pixel_counter / 8) <= max_x_rect)
			&& ((current_scanline / 8) >= min_y_rect) && ((current_scanline / 8) <= max_y_rect))
			{
				highlight = true;
			}

			//Determine which line of the tiles to generate pixels for this scanline
			u8 tile_line = rendered_scanline % 8;

			//Read the tile number
			main_menu::gbe_plus->ex_write_u8(REG_VBK, 0);
			u8 map_entry = main_menu::gbe_plus->ex_read_u8(win_map_addr + x);
			
			//Read the BG attributes
			main_menu::gbe_plus->ex_write_u8(REG_VBK, 1);
			u8 bg_attribute = main_menu::gbe_plus->ex_read_u8(win_map_addr + x);
			u8 pal_num = (bg_attribute & 0x7);
			u8 vram_bank = (bg_attribute & 0x8) ? 1 : 0;

			//Setup palettes
			u32 bgp[4];

			u32* color = main_menu::gbe_plus->get_bg_palette(pal_num);

			bgp[0] = *color; color += 8;
			bgp[1] = *color; color += 8;
			bgp[2] = *color; color += 8;
			bgp[3] = *color; color += 8;

			u8 tile_pixel = 0;

			//Convert tile number to signed if necessary
			if(bg_tile_addr == 0x8800) 
			{
				if(map_entry <= 127) { map_entry += 128; }
				else { map_entry -= 128; }
			}

			//Account for vertical flipping
			if(bg_attribute & 0x40) { tile_line = flip_8[tile_line]; }

			//Calculate the address of the 8x1 pixel data based on map entry
			u16 tile_addr = (bg_tile_addr + (map_entry << 4) + (tile_line << 1));

			//Grab bytes from VRAM representing 8x1 pixel data
			main_menu::gbe_plus->ex_write_u8(REG_VBK, vram_bank);
			u16 tile_data = (main_menu::gbe_plus->ex_read_u8(tile_addr + 1) << 8) | main_menu::gbe_plus->ex_read_u8(tile_addr);

			for(int y = 7; y >= 0; y--)
			{
				//Calculate raw value of the tile's pixel
				if(bg_attribute & 0x20) 
				{
					tile_pixel = ((tile_data >> 8) & (1 << flip_8[y])) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << flip_8[y])) ? 1 : 0;
				}

				else 
				{
					tile_pixel = ((tile_data >> 8) & (1 << y)) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << y)) ? 1 : 0;
				}
				
				if(scanline_pixel_counter >= 160) { scanline_pixel_buffer[scanline_pixel_counter++] = 0xFFFFFFFF; }
				else { scanline_pixel_buffer[scanline_pixel_counter++] = bgp[tile_pixel]; }

				//Highlight selected pixels, if applicable
				if(highlight)
				{
					u8 temp = scanline_pixel_counter - 1;
					scanline_pixel_buffer[temp] += 0x00700000;
				}
			}

			main_menu::gbe_plus->ex_write_u8(REG_VBK, current_vram_bank);
		}

		//Copy scanline buffer to BG buffer
		if(draw_line)
		{
			for(u8 pixel_counter = 0; pixel_counter < 160; pixel_counter++)
			{
				bg_pixels.push_back(scanline_pixel_buffer[pixel_counter]);
			}
		}
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

/****** Draws the DMG OBJ layer ******/
void gbe_cgfx::draw_dmg_obj()
{
	if(main_menu::gbe_plus == NULL) { return; }

	std::vector<u32> obj_pixels;
	u32 scanline_pixel_buffer[256];

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

	//8 pixel (horizontal+vertical) flipping lookup generation
	u8 flip_8[8];
	for(int x = 0; x < 8; x++) { flip_8[x] = (7 - x); }

	for(u8 current_scanline = 0; current_scanline < 144; current_scanline++)
	{
		u8 obj_x_sort[40];
		u8 obj_sort_length = 0;

		u8 obj_render_list[10];
		int obj_render_length = -1;

		//Cycle through all of the sprites
		for(int x = 0; x < 40; x++)
		{
			u8 obj_y = main_menu::gbe_plus->ex_read_u8(OAM + (x * 4));
			obj_y -= 16;

			u8 test_top = ((obj_y + obj_height) > 0x100) ? 0 : obj_y;
			u8 test_bottom = (obj_y + obj_height);

			//Check to see if sprite is rendered on the current scanline
			if((current_scanline >= test_top) && (current_scanline < test_bottom))
			{
				obj_x_sort[obj_sort_length++] = x;
			}

			if(obj_sort_length == 10) { break; }
		}

		//Sort them based on X coordinate
		for(int scanline_pixel = 0; scanline_pixel < 256; scanline_pixel++)
		{
			for(int x = 0; x < obj_sort_length; x++)
			{
				u8 sprite_id = obj_x_sort[x];
				u8 obj_x = main_menu::gbe_plus->ex_read_u8(OAM + (sprite_id * 4) + 1);
				obj_x -= 8;

				if(obj_x == scanline_pixel) 
				{
					obj_render_length++;
					obj_render_list[obj_render_length] = sprite_id; 
				}

				//Enforce 10 sprite-per-scanline limit
				if(obj_render_length == 9) { break; }
			}
		}

		//White-out scanline pixel data before drawing
		for(int x = 0; x < 256; x++) { scanline_pixel_buffer[x] = 0xFFFFFFFF; } 

		//Cycle through all sprites that are rendering on this pixel, draw them according to their priority
		for(int x = obj_render_length; x >= 0; x--)
		{
			u8 sprite_id = obj_render_list[x];
			u8 obj_x = main_menu::gbe_plus->ex_read_u8(OAM + (sprite_id * 4) + 1);
			u8 obj_y = main_menu::gbe_plus->ex_read_u8(OAM + (sprite_id * 4));

			obj_x -= 8;
			obj_y -= 16;

			u8 pal_num = (main_menu::gbe_plus->ex_read_u8(OAM + (sprite_id * 4) + 3) & 0x10) ? 1 : 0;
			u8 tile_num = main_menu::gbe_plus->ex_read_u8(OAM + (sprite_id * 4) + 2);

			if(obj_height == 16) { tile_num &= ~0x1; }

			bool h_flip = (main_menu::gbe_plus->ex_read_u8(OAM + (sprite_id * 4) + 3) & 0x20) ? true : false;
			bool v_flip = (main_menu::gbe_plus->ex_read_u8(OAM + (sprite_id * 4) + 3) & 0x40) ? true : false;

			//Determine which line of the tiles to generate pixels for this scanline		
			u8 tile_line = (current_scanline - obj_y);

			//Account for vertical flipping
			if(v_flip) 
			{
				s16 flip = tile_line;
				flip -= (obj_height - 1);

				if(flip < 0) { flip *= -1; }

				tile_line = flip;
			}

			u8 tile_pixel = 0;

			//Calculate the address of the 8x1 pixel data based on map entry
			u16 tile_addr = (0x8000 + (tile_num << 4) + (tile_line << 1));

			//Grab bytes from VRAM representing 8x1 pixel data
			u16 tile_data = (main_menu::gbe_plus->ex_read_u8(tile_addr + 1) << 8) | main_menu::gbe_plus->ex_read_u8(tile_addr);

			for(int y = 7; y >= 0; y--)
			{
				bool draw_obj_pixel = true;

				//Calculate raw value of the tile's pixel
				if(h_flip) 
				{
					tile_pixel = ((tile_data >> 8) & (1 << flip_8[y])) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << flip_8[y])) ? 1 : 0;
				}

				else 
				{
					tile_pixel = ((tile_data >> 8) & (1 << y)) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << y)) ? 1 : 0;
				}

				//If raw color is zero, this is the sprite's transparency, abort rendering this pixel
				if(tile_pixel == 0) { draw_obj_pixel = false; obj_x++; }
				
				//Render sprite pixel
				if(draw_obj_pixel)
				{
					switch(obp[tile_pixel][pal_num])
					{
						case 0: 
							scanline_pixel_buffer[obj_x++] = config::DMG_OBJ_PAL[0][pal_num];
							break;

						case 1: 
							scanline_pixel_buffer[obj_x++] = config::DMG_OBJ_PAL[1][pal_num];
							break;

						case 2: 
							scanline_pixel_buffer[obj_x++] = config::DMG_OBJ_PAL[2][pal_num];
							break;

						case 3: 
							scanline_pixel_buffer[obj_x++] = config::DMG_OBJ_PAL[3][pal_num];
							break;
					}
				}
			}
		}

		//Copy scanline buffer to OBJ buffer
		for(u8 pixel_counter = 0; pixel_counter < 160; pixel_counter++)
		{
			obj_pixels.push_back(scanline_pixel_buffer[pixel_counter]);
		}
	}

	QImage raw_image(160, 144, QImage::Format_ARGB32);	

	//Copy raw pixels to QImage
	for(int x = 0; x < obj_pixels.size(); x++)
	{
		raw_image.setPixel((x % 160), (x / 160), obj_pixels[x]);
	}

	raw_image = raw_image.scaled(320, 288);

	//Set label Pixmap
	current_layer->setPixmap(QPixmap::fromImage(raw_image));
}

/****** Draws the GBC OBJ layer ******/
void gbe_cgfx::draw_gbc_obj()
{
	if(main_menu::gbe_plus == NULL) { return; }

	std::vector<u32> obj_pixels;
	u32 scanline_pixel_buffer[256];

	//Determine if in 8x8 or 8x16 mode
	u8 obj_height = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x04) ? 16 : 8;

	//8 pixel (horizontal+vertical) flipping lookup generation
	u8 flip_8[8];
	for(int x = 0; x < 8; x++) { flip_8[x] = (7 - x); }

	for(u8 current_scanline = 0; current_scanline < 144; current_scanline++)
	{
		u8 obj_render_list[10];
		int obj_render_length = -1;

		//Cycle through all of the sprites
		for(int x = 0; x < 40; x++)
		{
			u8 obj_y = main_menu::gbe_plus->ex_read_u8(OAM + (x * 4));
			obj_y -= 16;

			u8 test_top = ((obj_y + obj_height) > 0x100) ? 0 : obj_y;
			u8 test_bottom = (obj_y + obj_height);

			//Check to see if sprite is rendered on the current scanline
			if((current_scanline >= test_top) && (current_scanline < test_bottom))
			{
				obj_render_length++;
				obj_render_list[obj_render_length] = x; 
			}

			//Enforce 10 sprite-per-scanline limit
			if(obj_render_length == 9) { break; }
		}

		//White-out scanline pixel data before drawing
		for(int x = 0; x < 256; x++) { scanline_pixel_buffer[x] = 0xFFFFFFFF; } 

		//Cycle through all sprites that are rendering on this pixel, draw them according to their priority
		for(int x = obj_render_length; x >= 0; x--)
		{
			u8 current_vram_bank = main_menu::gbe_plus->ex_read_u8(REG_VBK);

			u8 sprite_id = obj_render_list[x];
			u8 obj_x = main_menu::gbe_plus->ex_read_u8(OAM + (sprite_id * 4) + 1);
			u8 obj_y = main_menu::gbe_plus->ex_read_u8(OAM + (sprite_id * 4));

			obj_x -= 8;
			obj_y -= 16;

			u8 pal_num = (main_menu::gbe_plus->ex_read_u8(OAM + (sprite_id * 4) + 3) & 0x7);
			u8 vram_bank = (main_menu::gbe_plus->ex_read_u8(OAM + (sprite_id * 4) + 3) & 0x08) ? 1 : 0;
			u8 tile_num = main_menu::gbe_plus->ex_read_u8(OAM + (sprite_id * 4) + 2);

			if(obj_height == 16) { tile_num &= ~0x1; }

			bool h_flip = (main_menu::gbe_plus->ex_read_u8(OAM + (sprite_id * 4) + 3) & 0x20) ? true : false;
			bool v_flip = (main_menu::gbe_plus->ex_read_u8(OAM + (sprite_id * 4) + 3) & 0x40) ? true : false;

			//Setup palettes
			u32 obp[4];
			u32* color = main_menu::gbe_plus->get_obj_palette(pal_num);

			obp[0] = *color; color += 8;
			obp[1] = *color; color += 8;
			obp[2] = *color; color += 8;
			obp[3] = *color; color += 8;

			//Determine which line of the tiles to generate pixels for this scanline		
			u8 tile_line = (current_scanline - obj_y);

			//Account for vertical flipping
			if(v_flip) 
			{
				s16 flip = tile_line;
				flip -= (obj_height - 1);

				if(flip < 0) { flip *= -1; }

				tile_line = flip;
			}

			u8 tile_pixel = 0;

			//Calculate the address of the 8x1 pixel data based on map entry
			u16 tile_addr = (0x8000 + (tile_num << 4) + (tile_line << 1));

			//Grab bytes from VRAM representing 8x1 pixel data
			main_menu::gbe_plus->ex_write_u8(REG_VBK, vram_bank);
			u16 tile_data = (main_menu::gbe_plus->ex_read_u8(tile_addr + 1) << 8) | main_menu::gbe_plus->ex_read_u8(tile_addr);

			for(int y = 7; y >= 0; y--)
			{
				bool draw_obj_pixel = true;

				//Calculate raw value of the tile's pixel
				if(h_flip) 
				{
					tile_pixel = ((tile_data >> 8) & (1 << flip_8[y])) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << flip_8[y])) ? 1 : 0;
				}

				else 
				{
					tile_pixel = ((tile_data >> 8) & (1 << y)) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << y)) ? 1 : 0;
				}

				//If raw color is zero, this is the sprite's transparency, abort rendering this pixel
				if(tile_pixel == 0) { draw_obj_pixel = false; obj_x++; }
				
				//Render sprite pixel
				if(draw_obj_pixel) { scanline_pixel_buffer[obj_x++] = obp[tile_pixel]; }
			}

			main_menu::gbe_plus->ex_write_u8(REG_VBK, current_vram_bank);
		}

		//Copy scanline buffer to OBJ buffer
		for(u8 pixel_counter = 0; pixel_counter < 160; pixel_counter++)
		{
			obj_pixels.push_back(scanline_pixel_buffer[pixel_counter]);
		}
	}

	QImage raw_image(160, 144, QImage::Format_ARGB32);	

	//Copy raw pixels to QImage
	for(int x = 0; x < obj_pixels.size(); x++)
	{
		raw_image.setPixel((x % 160), (x / 160), obj_pixels[x]);
	}

	raw_image = raw_image.scaled(320, 288);

	//Set label Pixmap
	current_layer->setPixmap(QPixmap::fromImage(raw_image));
}

/****** Update the preview for layers ******/
void gbe_cgfx::update_preview(u32 x, u32 y)
{
	if(main_menu::gbe_plus == NULL) { return; }

	x >>= 1;
	y >>= 1;

	//8 pixel (horizontal+vertical) flipping lookup generation
	u8 flip_8[8];
	for(int z = 0; z < 8; z++) { flip_8[z] = (7 - z); }

	//Update preview for DMG BG
	if((layer_select->currentIndex() == 0) && (config::gb_type < 2)) 
	{
		//Determine BG Map & Tile address
		u16 bg_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x8) ? 0x9C00 : 0x9800;
		u16 bg_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;

		//Determine the map entry from on-screen coordinates
		u8 tile_x = main_menu::gbe_plus->ex_read_u8(REG_SX) + x;
		u8 tile_y = main_menu::gbe_plus->ex_read_u8(REG_SY) + y;
		u16 map_entry = (tile_x / 8) + ((tile_y / 8) * 32);

		u8 map_value = main_menu::gbe_plus->ex_read_u8(bg_map_addr + map_entry);

		//Convert tile number to signed if necessary
		if(bg_tile_addr == 0x8800) 
		{
			if(map_value <= 127) { map_value += 128; }
			else { map_value -= 128; }
		}

		u16 bg_index = (((bg_tile_addr + (map_value << 4)) & ~0x8000) >> 4);

		std::string current_hash = "Tile Palette : " + hash_tile(x, y);

		QImage final_image = grab_dmg_bg_data(bg_index).scaled(128, 128);
		current_tile->setPixmap(QPixmap::fromImage(final_image));

		//Tile info - ID
		QString id("Tile ID : ");
		id += QString::number(bg_index);
		tile_id->setText(id);

		//Tile info - Address
		QString addr("Tile Address : 0x");
		addr += QString::number((bg_tile_addr + (map_value << 4)), 16).toUpper();
		tile_addr->setText(addr);

		//Tile info - Size
		QString size("Tile Size : 8x8");
		tile_size->setText(size);

		//Tile info - H/V Flip
		QString flip("H-Flip : N    V-Flip : N");
		h_v_flip->setText(flip);

		//Tile info - Palette
		QString pal("Tile Palette : BGP");
		tile_palette->setText(pal);

		//Tile info - Hash
		QString hashed = QString::fromStdString(current_hash);
		hash_text->setText(hashed);
	}

	//Update preview for DMG Window
	else if((layer_select->currentIndex() == 1) && (config::gb_type < 2)) 
	{
		//Determine BG Map & Tile address
		u16 win_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x40) ? 0x9C00 : 0x9800;
		u16 bg_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;

		u8 wx = main_menu::gbe_plus->ex_read_u8(REG_WX);
		wx = (wx < 7) ? 0 : (wx - 7); 
	
		//Determine the map entry from on-screen coordinates
		u8 tile_x = x - wx;
		u8 tile_y = y - main_menu::gbe_plus->ex_read_u8(REG_WY);
		u16 map_entry = (tile_x / 8) + ((tile_y / 8) * 32);

		u8 map_value = main_menu::gbe_plus->ex_read_u8(win_map_addr + map_entry);

		//Convert tile number to signed if necessary
		if(bg_tile_addr == 0x8800) 
		{
			if(map_value <= 127) { map_value += 128; }
			else { map_value -= 128; }
		}

		u16 bg_index = (((bg_tile_addr + (map_value << 4)) & ~0x8000) >> 4);

		std::string current_hash = "Tile Palette : " + hash_tile(x, y);

		QImage final_image = grab_dmg_bg_data(bg_index).scaled(128, 128);
		current_tile->setPixmap(QPixmap::fromImage(final_image));

		//Tile info - ID
		QString id("Tile ID : ");
		id += QString::number(bg_index);
		tile_id->setText(id); 

		//Tile info - Address
		QString addr("Tile Address : 0x");
		addr += QString::number((bg_tile_addr + (map_value << 4)), 16).toUpper();
		tile_addr->setText(addr);

		//Tile info - Size
		QString size("Tile Size : 8x8");
		tile_size->setText(size);

		//Tile info - H/V Flip
		QString flip("H-Flip : N    V-Flip : N");
		h_v_flip->setText(flip);

		//Tile info - Palette
		QString pal("Tile Palette : BGP");
		tile_palette->setText(pal);

		//Tile info - Hash
		QString hashed = QString::fromStdString(current_hash);
		hash_text->setText(hashed);
	}

	//Update preview for DMG OBJ 
	else if((layer_select->currentIndex() == 2) && (config::gb_type < 2))
	{
		//Determine if in 8x8 or 8x16 mode
		u8 obj_height = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x04) ? 16 : 8;

		for(int obj_index = 0; obj_index < 40; obj_index++)
		{
			//Grab X-Y OBJ coordinates
			u8 obj_x = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 1);
			u8 obj_y = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4));

			obj_x -= 8;
			obj_y -= 16;

			u8 test_left = ((obj_x + 8) > 0x100) ? 0 : obj_x;
			u8 test_right = (obj_x + 8);

			u8 test_top = ((obj_y + obj_height) > 0x100) ? 0 : obj_y;
			u8 test_bottom = (obj_y + obj_height);

			bool h_flip = (main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 3) & 0x20) ? true : false;
			bool v_flip = (main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 3) & 0x40) ? true : false;

			u8 obj_tile = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 2);
			if(obj_height == 16) { obj_tile &= ~0x1; }

			u8 obj_pal = (main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 3) & 0x10) ? 1 : 0;

			if((x >= test_left) && (x <= test_right) && (y >= test_top) && (y <= test_bottom))
			{
				if(obj_height == 8)
				{
					QImage final_image = grab_dmg_obj_data(obj_index).scaled(128, 128).mirrored(h_flip, v_flip);
					current_tile->setPixmap(QPixmap::fromImage(final_image));

					//Tile info - Size
					QString size("Tile Size : 8x8");
					tile_size->setText(size);
				}

				else
				{
					QImage final_image = grab_dmg_obj_data(obj_index).scaled(64, 128).mirrored(h_flip, v_flip);
					current_tile->setPixmap(QPixmap::fromImage(final_image));

					//Tile info - Size
					QString size("Tile Size : 8x16");
					tile_size->setText(size);
				}

				//Tile info - ID
				QString id("Tile ID : ");
				id += QString::number(obj_index);
				tile_id->setText(id);

				//Tile info - Address
				QString addr("Tile Address : 0x");
				addr += QString::number((0x8000 + (obj_tile << 4)), 16).toUpper();
				tile_addr->setText(addr);

				//Tile info - H/V Flip
				QString flip;
				
				if((!h_flip) && (!v_flip)) { flip = "H-Flip : N    V-Flip : N"; }
				else if((h_flip) && (!v_flip)) { flip = "H-Flip : Y    V-Flip : N"; }
				else if((!h_flip) && (v_flip)) { flip = "H-Flip : N    V-Flip : Y"; }
				else { flip = "H-Flip : Y    V-Flip : Y"; }				

				h_v_flip->setText(flip);

				//Tile info - Palette
				QString pal;
				pal = (obj_pal == 0) ? "Tile Palette : OBP0" : "Tile Palette : OBP1";
				tile_palette->setText(pal);
			}
		}
	}

	//Update preview for GBC BG
	else if((layer_select->currentIndex() == 0) && (config::gb_type == 2)) 
	{
		u32 bg_pixel_data[64];
		u8 bg_pixel_counter = 0;
		u8 tile_pixel = 0;

		//Determine BG Map & Tile address
		u16 bg_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x8) ? 0x9C00 : 0x9800;
		u16 bg_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;

		//Determine the map entry from on-screen coordinates
		u8 tile_x = main_menu::gbe_plus->ex_read_u8(REG_SX) + x;
		u8 tile_y = main_menu::gbe_plus->ex_read_u8(REG_SY) + y;
		u16 map_entry = (tile_x / 8) + ((tile_y / 8) * 32);

		u8 current_vram_bank = main_menu::gbe_plus->ex_read_u8(REG_VBK);
			
		//Read the BG attributes
		main_menu::gbe_plus->ex_write_u8(REG_VBK, 1);
		u8 bg_attribute = main_menu::gbe_plus->ex_read_u8(bg_map_addr + map_entry);
		u8 pal_num = (bg_attribute & 0x7);
		u8 vram_bank = (bg_attribute & 0x8) ? 1 : 0;
		bool h_flip = (bg_attribute & 0x20) ? true : false;
		bool v_flip = (bg_attribute & 0x40) ? true : false;

		//Setup palettes
		u32 bgp[4];

		u32* color = main_menu::gbe_plus->get_bg_palette(pal_num);

		bgp[0] = *color; color += 8;
		bgp[1] = *color; color += 8;
		bgp[2] = *color; color += 8;
		bgp[3] = *color; color += 8;

		main_menu::gbe_plus->ex_write_u8(REG_VBK, 0);
		u8 map_value = main_menu::gbe_plus->ex_read_u8(bg_map_addr + map_entry);

		//Convert tile number to signed if necessary
		if(bg_tile_addr == 0x8800) 
		{
			if(map_value <= 127) { map_value += 128; }
			else { map_value -= 128; }
		}

		u16 bg_index = (((bg_tile_addr + (map_value << 4)) & ~0x8000) >> 4);

		std::string current_hash = "Tile Palette : " + hash_tile(x, y);

		//Calculate the address of the BG pixel data based on map entry
		u16 vram_tile_addr = (bg_tile_addr + (map_value << 4));

		//Grab bytes from VRAM representing BG pixel data
		main_menu::gbe_plus->ex_write_u8(REG_VBK, vram_bank);

		for(int bg_x = 0; bg_x < 8; bg_x++)
		{
			u16 tile_data = (main_menu::gbe_plus->ex_read_u8(vram_tile_addr + 1) << 8) | main_menu::gbe_plus->ex_read_u8(vram_tile_addr);

			//Account for vertical flipping
			if(v_flip) { bg_pixel_counter = flip_8[bg_x] * 8; }

			for(int bg_y = 7; bg_y >= 0; bg_y--)
			{

				//Calculate raw value of the tile's pixel
				if(h_flip) 
				{
					tile_pixel = ((tile_data >> 8) & (1 << flip_8[bg_y])) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << flip_8[bg_y])) ? 1 : 0;
				}

				else
				{
					tile_pixel = ((tile_data >> 8) & (1 << bg_y)) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << bg_y)) ? 1 : 0;
				}				

				bg_pixel_data[bg_pixel_counter++] = bgp[tile_pixel];
			}

			vram_tile_addr += 2;
		}

		main_menu::gbe_plus->ex_write_u8(REG_VBK, current_vram_bank);

		QImage final_image(8, 8, QImage::Format_ARGB32);	

		//Copy raw pixels to QImage
		for(int bg_x = 0; bg_x < 64; bg_x++)
		{
			final_image.setPixel((bg_x % 8), (bg_x / 8), bg_pixel_data[bg_x]);
		}

		final_image = final_image.scaled(128, 128);
		current_tile->setPixmap(QPixmap::fromImage(final_image));

		//Tile info - ID
		QString id("Tile ID : ");
		id += QString::number(bg_index);
		tile_id->setText(id);

		//Tile info - Address
		QString addr("Tile Address : 0x");
		addr += QString::number((bg_tile_addr + (map_value << 4)), 16).toUpper();
		tile_addr->setText(addr);

		//Tile info - Size
		QString size("Tile Size : 8x8");
		tile_size->setText(size);

		//Tile info - H/V Flip
		QString flip("H-Flip : N    V-Flip : N");

		if((!h_flip) && (!v_flip)) { flip = "H-Flip : N    V-Flip : N"; }
		else if((h_flip) && (!v_flip)) { flip = "H-Flip : Y    V-Flip : N"; }
		else if((!h_flip) && (v_flip)) { flip = "H-Flip : N    V-Flip : Y"; }
		else { flip = "H-Flip : Y    V-Flip : Y"; }	

		h_v_flip->setText(flip);

		//Tile info - Palette
		QString pal;
				
		switch(pal_num)
		{
			case 0: pal = "Tile Palette : BCP0"; break;
			case 1: pal = "Tile Palette : BCP1"; break;
			case 2: pal = "Tile Palette : BCP2"; break;
			case 3: pal = "Tile Palette : BCP3"; break;
			case 4: pal = "Tile Palette : BCP4"; break;
			case 5: pal = "Tile Palette : BCP5"; break;
			case 6: pal = "Tile Palette : BCP6"; break;
			case 7: pal = "Tile Palette : BCP7"; break;
		}

		tile_palette->setText(pal);

		//Tile info - Hash
		QString hashed = QString::fromStdString(current_hash);
		hash_text->setText(hashed);
	}

	//Update preview for GBC Window
	else if((layer_select->currentIndex() == 1) && (config::gb_type == 2)) 
	{
		u32 win_pixel_data[64];
		u8 win_pixel_counter = 0;
		u8 tile_pixel = 0;

		//Determine BG Map & Tile address
		u16 win_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x40) ? 0x9C00 : 0x9800;
		u16 win_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;
	
		//Determine the map entry from on-screen coordinates
		u8 wx = main_menu::gbe_plus->ex_read_u8(REG_WX);
		wx = (wx < 7) ? 0 : (wx - 7); 

		u8 tile_x = x - wx;
		u8 tile_y = y - main_menu::gbe_plus->ex_read_u8(REG_WY);
		u16 map_entry = (tile_x / 8) + ((tile_y / 8) * 32);

		u8 current_vram_bank = main_menu::gbe_plus->ex_read_u8(REG_VBK);
			
		//Read the BG attributes
		main_menu::gbe_plus->ex_write_u8(REG_VBK, 1);
		u8 bg_attribute = main_menu::gbe_plus->ex_read_u8(win_map_addr + map_entry);
		u8 pal_num = (bg_attribute & 0x7);
		u8 vram_bank = (bg_attribute & 0x8) ? 1 : 0;
		bool h_flip = (bg_attribute & 0x20) ? true : false;
		bool v_flip = (bg_attribute & 0x40) ? true : false;

		//Setup palettes
		u32 bgp[4];

		u32* color = main_menu::gbe_plus->get_bg_palette(pal_num);

		bgp[0] = *color; color += 8;
		bgp[1] = *color; color += 8;
		bgp[2] = *color; color += 8;
		bgp[3] = *color; color += 8;

		main_menu::gbe_plus->ex_write_u8(REG_VBK, 0);
		u8 map_value = main_menu::gbe_plus->ex_read_u8(win_map_addr + map_entry);

		//Convert tile number to signed if necessary
		if(win_tile_addr == 0x8800) 
		{
			if(map_value <= 127) { map_value += 128; }
			else { map_value -= 128; }
		}

		u16 bg_index = (((win_tile_addr + (map_value << 4)) & ~0x8000) >> 4);

		std::string current_hash = "Tile Palette : " + hash_tile(x, y);

		//Calculate the address of the BG pixel data based on map entry
		u16 vram_tile_addr = (win_tile_addr + (map_value << 4));

		//Grab bytes from VRAM representing BG pixel data
		main_menu::gbe_plus->ex_write_u8(REG_VBK, vram_bank);

		for(int win_x = 0; win_x < 8; win_x++)
		{
			u16 tile_data = (main_menu::gbe_plus->ex_read_u8(vram_tile_addr + 1) << 8) | main_menu::gbe_plus->ex_read_u8(vram_tile_addr);

			//Account for vertical flipping
			if(v_flip) { win_pixel_counter = flip_8[win_x] * 8; }

			for(int win_y = 7; win_y >= 0; win_y--)
			{

				//Calculate raw value of the tile's pixel
				if(h_flip) 
				{
					tile_pixel = ((tile_data >> 8) & (1 << flip_8[win_y])) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << flip_8[win_y])) ? 1 : 0;
				}

				else
				{
					tile_pixel = ((tile_data >> 8) & (1 << win_y)) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << win_y)) ? 1 : 0;
				}				

				win_pixel_data[win_pixel_counter++] = bgp[tile_pixel];
			}

			vram_tile_addr += 2;
		}

		main_menu::gbe_plus->ex_write_u8(REG_VBK, current_vram_bank);

		QImage final_image(8, 8, QImage::Format_ARGB32);	

		//Copy raw pixels to QImage
		for(int win_x = 0; win_x < 64; win_x++)
		{
			final_image.setPixel((win_x % 8), (win_x / 8), win_pixel_data[win_x]);
		}

		final_image = final_image.scaled(128, 128);
		current_tile->setPixmap(QPixmap::fromImage(final_image));

		//Tile info - ID
		QString id("Tile ID : ");
		id += QString::number(bg_index);
		tile_id->setText(id);

		//Tile info - Address
		QString addr("Tile Address : 0x");
		addr += QString::number((win_tile_addr + (map_value << 4)), 16).toUpper();
		tile_addr->setText(addr);

		//Tile info - Size
		QString size("Tile Size : 8x8");
		tile_size->setText(size);

		//Tile info - H/V Flip
		QString flip("H-Flip : N    V-Flip : N");

		if((!h_flip) && (!v_flip)) { flip = "H-Flip : N    V-Flip : N"; }
		else if((h_flip) && (!v_flip)) { flip = "H-Flip : Y    V-Flip : N"; }
		else if((!h_flip) && (v_flip)) { flip = "H-Flip : N    V-Flip : Y"; }
		else { flip = "H-Flip : Y    V-Flip : Y"; }	

		h_v_flip->setText(flip);

		//Tile info - Palette
		QString pal;
				
		switch(pal_num)
		{
			case 0: pal = "Tile Palette : BCP0"; break;
			case 1: pal = "Tile Palette : BCP1"; break;
			case 2: pal = "Tile Palette : BCP2"; break;
			case 3: pal = "Tile Palette : BCP3"; break;
			case 4: pal = "Tile Palette : BCP4"; break;
			case 5: pal = "Tile Palette : BCP5"; break;
			case 6: pal = "Tile Palette : BCP6"; break;
			case 7: pal = "Tile Palette : BCP7"; break;
		}

		tile_palette->setText(pal);

		//Tile info - Hash
		QString hashed = QString::fromStdString(current_hash);
		hash_text->setText(hashed);
	}

	//Update preview for GBC OBJ 
	else if((layer_select->currentIndex() == 2) && (config::gb_type == 2))
	{
		//Determine if in 8x8 or 8x16 mode
		u8 obj_height = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x04) ? 16 : 8;

		for(int obj_index = 0; obj_index < 40; obj_index++)
		{
			//Grab X-Y OBJ coordinates
			u8 obj_x = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 1);
			u8 obj_y = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4));

			obj_x -= 8;
			obj_y -= 16;

			u8 test_left = ((obj_x + 8) > 0x100) ? 0 : obj_x;
			u8 test_right = (obj_x + 8);

			u8 test_top = ((obj_y + obj_height) > 0x100) ? 0 : obj_y;
			u8 test_bottom = (obj_y + obj_height);

			bool h_flip = (main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 3) & 0x20) ? true : false;
			bool v_flip = (main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 3) & 0x40) ? true : false;

			u8 obj_tile = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 2);
			if(obj_height == 16) { obj_tile &= ~0x1; }

			u8 obj_pal = (main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 3) & 0x7);

			if((x >= test_left) && (x <= test_right) && (y >= test_top) && (y <= test_bottom))
			{
				if(obj_height == 8)
				{
					QImage final_image = grab_gbc_obj_data(obj_index).scaled(128, 128).mirrored(h_flip, v_flip);
					current_tile->setPixmap(QPixmap::fromImage(final_image));

					//Tile info - Size
					QString size("Tile Size : 8x8");
					tile_size->setText(size);
				}

				else
				{
					QImage final_image = grab_gbc_obj_data(obj_index).scaled(64, 128).mirrored(h_flip, v_flip);
					current_tile->setPixmap(QPixmap::fromImage(final_image));

					//Tile info - Size
					QString size("Tile Size : 8x16");
					tile_size->setText(size);
				}

				//Tile info - ID
				QString id("Tile ID : ");
				id += QString::number(obj_index);
				tile_id->setText(id);

				//Tile info - Address
				QString addr("Tile Address : 0x");
				addr += QString::number((0x8000 + (obj_tile << 4)), 16).toUpper();
				tile_addr->setText(addr);

				//Tile info - H/V Flip
				QString flip;
				
				if((!h_flip) && (!v_flip)) { flip = "H-Flip : N    V-Flip : N"; }
				else if((h_flip) && (!v_flip)) { flip = "H-Flip : Y    V-Flip : N"; }
				else if((!h_flip) && (v_flip)) { flip = "H-Flip : N    V-Flip : Y"; }
				else { flip = "H-Flip : Y    V-Flip : Y"; }				

				h_v_flip->setText(flip);

				//Tile info - Palette
				QString pal;
				
				switch(obj_pal)
				{
					case 0: pal = "Tile Palette : OCP0"; break;
					case 1: pal = "Tile Palette : OCP1"; break;
					case 2: pal = "Tile Palette : OCP2"; break;
					case 3: pal = "Tile Palette : OCP3"; break;
					case 4: pal = "Tile Palette : OCP4"; break;
					case 5: pal = "Tile Palette : OCP5"; break;
					case 6: pal = "Tile Palette : OCP6"; break;
					case 7: pal = "Tile Palette : OCP7"; break;
				}

				tile_palette->setText(pal);
			}
		}
	}
}

/****** Dumps the tile from a given layer ******/
void gbe_cgfx::dump_layer_tile(u32 x, u32 y)
{
	if(main_menu::gbe_plus == NULL) { return; }

	x >>= 1;
	y >>= 1;

	//Dump from DMG BG
	if((layer_select->currentIndex() == 0) && (config::gb_type < 2)) 
	{
		//Determine BG Map & Tile address
		u16 bg_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x8) ? 0x9C00 : 0x9800;
		u16 bg_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;

		//Determine the map entry from on-screen coordinates
		u8 tile_x = main_menu::gbe_plus->ex_read_u8(REG_SX) + x;
		u8 tile_y = main_menu::gbe_plus->ex_read_u8(REG_SY) + y;
		u16 map_entry = (tile_x / 8) + ((tile_y / 8) * 32);

		u8 map_value = main_menu::gbe_plus->ex_read_u8(bg_map_addr + map_entry);

		//Convert tile number to signed if necessary
		if(bg_tile_addr == 0x8800) 
		{
			if(map_value <= 127) { map_value += 128; }
			else { map_value -= 128; }
		}

		u16 bg_index = (((bg_tile_addr + (map_value << 4)) & ~0x8000) >> 4);

		dump_type = 0;
		show_advanced_bg(bg_index);
	}

	//Dump from DMG Window
	else if((layer_select->currentIndex() == 1) && (config::gb_type < 2)) 
	{
		//Determine BG Map & Tile address
		u16 win_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x40) ? 0x9C00 : 0x9800;
		u16 bg_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;

		u8 wx = main_menu::gbe_plus->ex_read_u8(REG_WX);
		wx = (wx < 7) ? 0 : (wx - 7); 
	
		//Determine the map entry from on-screen coordinates
		u8 tile_x = x - wx;
		u8 tile_y = y - main_menu::gbe_plus->ex_read_u8(REG_WY);
		u16 map_entry = (tile_x / 8) + ((tile_y / 8) * 32);

		u8 map_value = main_menu::gbe_plus->ex_read_u8(win_map_addr + map_entry);

		//Convert tile number to signed if necessary
		if(bg_tile_addr == 0x8800) 
		{
			if(map_value <= 127) { map_value += 128; }
			else { map_value -= 128; }
		}

		u16 bg_index = (((bg_tile_addr + (map_value << 4)) & ~0x8000) >> 4);

		dump_type = 0;
		show_advanced_bg(bg_index);
	}

	//Dump from DMG or GBC OBJ 
	else if(layer_select->currentIndex() == 2) 
	{
		//Determine if in 8x8 or 8x16 mode
		u8 obj_height = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x04) ? 16 : 8;

		for(int obj_index = 0; obj_index < 40; obj_index++)
		{
			//Grab X-Y OBJ coordinates
			u8 obj_x = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4) + 1);
			u8 obj_y = main_menu::gbe_plus->ex_read_u8(OAM + (obj_index * 4));

			obj_x -= 8;
			obj_y -= 16;

			u8 test_left = ((obj_x + 8) > 0x100) ? 0 : obj_x;
			u8 test_right = (obj_x + 8);

			u8 test_top = ((obj_y + obj_height) > 0x100) ? 0 : obj_y;
			u8 test_bottom = (obj_y + obj_height);

			dump_type = 1;

			if((x >= test_left) && (x <= test_right) && (y >= test_top) && (y <= test_bottom)) { show_advanced_obj(obj_index); }
		}
	}

	//Dump from GBC BG
	else if((layer_select->currentIndex() == 0) && (config::gb_type == 2))
	{
		//Determine BG Map & Tile address
		u16 bg_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x8) ? 0x9C00 : 0x9800;
		u16 bg_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;

		//Determine the map entry from on-screen coordinates
		u8 tile_x = main_menu::gbe_plus->ex_read_u8(REG_SX) + x;
		u8 tile_y = main_menu::gbe_plus->ex_read_u8(REG_SY) + y;
		u16 map_entry = (tile_x / 8) + ((tile_y / 8) * 32);

		u8 current_vram_bank, original_vram_bank = main_menu::gbe_plus->ex_read_u8(REG_VBK);
		u8 original_pal = cgfx::gbc_bg_color_pal;
			
		//Read the BG attributes
		main_menu::gbe_plus->ex_write_u8(REG_VBK, 1);
		u8 bg_attribute = main_menu::gbe_plus->ex_read_u8(bg_map_addr + map_entry);
		u8 pal_num = (bg_attribute & 0x7);
		u8 vram_bank = (bg_attribute & 0x8) ? 1 : 0;

		main_menu::gbe_plus->ex_write_u8(REG_VBK, 0);
		u8 map_value = main_menu::gbe_plus->ex_read_u8(bg_map_addr + map_entry);

		//Convert tile number to signed if necessary
		if(bg_tile_addr == 0x8800) 
		{
			if(map_value <= 127) { map_value += 128; }
			else { map_value -= 128; }
		}

		u16 bg_index = (((bg_tile_addr + (map_value << 4)) & ~0x8000) >> 4);

		current_vram_bank = cgfx::gbc_bg_vram_bank;
		cgfx::gbc_bg_vram_bank = vram_bank;
		cgfx::gbc_bg_color_pal = pal_num;
		main_menu::gbe_plus->ex_write_u8(REG_VBK, original_vram_bank);

		//Signal no estimation required for VRAM bank and palette
		config::gb_type = 10;
		dump_type = 0;
		show_advanced_bg(bg_index);
	}

	//Dump from GBC Window
	else if((layer_select->currentIndex() == 1) && (config::gb_type == 2))
	{
		//Determine BG Map & Tile address
		u16 win_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x40) ? 0x9C00 : 0x9800;
		u16 win_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;
	
		//Determine the map entry from on-screen coordinates
		u8 wx = main_menu::gbe_plus->ex_read_u8(REG_WX);
		wx = (wx < 7) ? 0 : (wx - 7); 

		u8 tile_x = x - wx;
		u8 tile_y = y - main_menu::gbe_plus->ex_read_u8(REG_WY);
		u16 map_entry = (tile_x / 8) + ((tile_y / 8) * 32);

		u8 current_vram_bank, original_vram_bank = main_menu::gbe_plus->ex_read_u8(REG_VBK);
		u8 original_pal = cgfx::gbc_bg_color_pal;
			
		//Read the BG attributes
		main_menu::gbe_plus->ex_write_u8(REG_VBK, 1);
		u8 bg_attribute = main_menu::gbe_plus->ex_read_u8(win_map_addr + map_entry);
		u8 pal_num = (bg_attribute & 0x7);
		u8 vram_bank = (bg_attribute & 0x8) ? 1 : 0;

		main_menu::gbe_plus->ex_write_u8(REG_VBK, 0);
		u8 map_value = main_menu::gbe_plus->ex_read_u8(win_map_addr + map_entry);

		//Convert tile number to signed if necessary
		if(win_tile_addr == 0x8800) 
		{
			if(map_value <= 127) { map_value += 128; }
			else { map_value -= 128; }
		}

		u16 bg_index = (((win_tile_addr + (map_value << 4)) & ~0x8000) >> 4);

		current_vram_bank = cgfx::gbc_bg_vram_bank;
		cgfx::gbc_bg_vram_bank = vram_bank;
		cgfx::gbc_bg_color_pal = pal_num;
		main_menu::gbe_plus->ex_write_u8(REG_VBK, original_vram_bank);

		//Signal no estimation required for VRAM bank and palette
		config::gb_type = 10;
		dump_type = 0;
		show_advanced_bg(bg_index);
	}
}

/****** Event filter for settings window ******/
bool gbe_cgfx::eventFilter(QObject* target, QEvent* event)
{
	//Check to see if mouse is hovered over current layer
	if(event->type() == QEvent::MouseMove)
	{
		QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
		u32 x = mouse_event->x();
		u32 y = mouse_event->y();		

		//Update the preview
		if((mouse_event->x() <= 320) && (mouse_event->y() <= 288)) { update_preview(x, y); }
	}

	//Check to see if mouse is clicked over current layer
	else if(event->type() == QEvent::MouseButtonPress)
	{
		QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
		u32 x = mouse_event->x();
		u32 y = mouse_event->y();		

		//Update the preview
		if((mouse_event->x() <= 320) && (mouse_event->y() <= 288)) { dump_layer_tile(x, y); }
	}

	return QDialog::eventFilter(target, event);
}

/****** Updates the settings window ******/
void gbe_cgfx::paintEvent(QPaintEvent* event)
{
	//Update GUI elements of the advanced menu
	name_label->setMinimumWidth(dest_label->width());
}

/****** Dumps tiles and writes a manifest entry  ******/
void gbe_cgfx::write_manifest_entry()
{
	//Process File Name
	QString path = dest_name->text();

	if(!path.isNull()) { cgfx::dump_name = path.toStdString(); }
	else { cgfx::dump_name = ""; }

	//Dump BG
	if(dump_type == 0) 
	{
		std::string temp_bg_path = cgfx::dump_bg_path;
		cgfx::dump_bg_path = dest_folder->text().toStdString();
		dump_bg(advanced_index);
		cgfx::dump_bg_path = temp_bg_path;
	}

	//Dump OBJ
	else
	{
		std::string temp_obj_path = cgfx::dump_obj_path;
		cgfx::dump_obj_path = dest_folder->text().toStdString();
		dump_obj(advanced_index);
		cgfx::dump_obj_path = temp_obj_path;
	}

	//Prepare a string for the manifest entry
	//Hash + Hash.bmp + Type + EXT_VRAM_ADDR + EXT_AUTO_BRIGHT
	std::string entry = "";

	std::string gfx_name = "";
	
	if(dest_folder->text().isNull()) { gfx_name = cgfx::last_hash + ".bmp"; }
	else { gfx_name = dest_folder->text().toStdString() + dest_name->text().toStdString(); }
	
	std::string gfx_type = util::to_str(cgfx::last_type);
	std::string gfx_addr = (ext_vram->isChecked()) ? util::to_hex_str(cgfx::last_vram_addr).substr(2) : "0";
	std::string gfx_bright = (ext_bright->isChecked()) ? util::to_str(cgfx::last_palette) : "0";

	entry = "[" + cgfx::last_hash + ":'" + gfx_name + "':" + gfx_type + ":" + gfx_addr + ":" + gfx_bright + "]";

	//Open manifest file, then write to it
	std::ofstream file(cgfx::manifest_file.c_str(), std::ios::out | std::ios::app);

	//TODO - Add a Qt warning here
	if(!file.is_open()) { advanced_box->hide(); return; }

	file << "\n" << entry;
	file.close();
	
	advanced_box->hide();
}

/****** Browse for a directory to use in the advanced menu ******/
void gbe_cgfx::browse_advanced_dir()
{
	QString path;

	data_folder->open_data_folder();			

	while(!data_folder->finish) { QApplication::processEvents(); }
	
	path = data_folder->directory().path();
	path = data_folder->path.relativeFilePath(path);

	advanced_box->raise();

	if(path.isNull()) { return; }

	//Make sure path is complete, e.g. has the correct separator at the end
	//Qt doesn't append this automatically
	std::string temp_str = path.toStdString();
	std::string temp_chr = "";
	temp_chr = temp_str[temp_str.length() - 1];

	if((temp_chr != "/") && (temp_chr != "\\")) { path.append("/"); }
	path = QDir::toNativeSeparators(path);

	dest_folder->setText(path);
	last_custom_path = dest_folder->text().toStdString();
}

/****** Browse for a directory to use in the advanced menu ******/
void gbe_cgfx::browse_advanced_file()
{
	QString path;

	path = QFileDialog::getOpenFileName(this, tr("Open"), QString::fromStdString(config::data_path), tr("All files (*)"));
	advanced_box->raise();

	if(path.isNull()) { return; }

	//Use relative paths
	QFileInfo file(path);
	path = file.fileName();
	cgfx::dump_name = path.toStdString();

	dest_name->setText(path);
}

/****** Selects folder ******/
void gbe_cgfx::select_folder() { data_folder->finish = true; }

/****** Rejects folder ******/
void gbe_cgfx::reject_folder()
{
	data_folder->finish = true;
	data_folder->setDirectory(data_folder->last_path);
}

/****** Updates the dumping selection for multiple tiles in the layer tab ******/
void gbe_cgfx::update_selection()
{
	if((rect_x->value() == 0) || (rect_y->value() == 0) || (rect_w->value() == 0) || (rect_h->value() == 0))
	{
		min_x_rect = max_x_rect = min_y_rect = max_y_rect = 255;
		layer_change();
		return;
	}

	min_x_rect = rect_x->value();
	max_x_rect = (min_x_rect + rect_w->value()) - 1;

	min_y_rect = rect_y->value();
	max_y_rect = (min_y_rect + rect_h->value()) - 1;

	min_x_rect -= 1;
	max_x_rect -= 1;
	min_y_rect -= 1;
	max_y_rect -= 1;

	layer_change();
}

/****** Dumps the selection of multiple tiles to a file ******/
void gbe_cgfx::dump_selection()
{
	if(main_menu::gbe_plus == NULL) { return; }
	if((rect_x->value() == 0) || (rect_y->value() == 0) || (rect_w->value() == 0) || (rect_h->value() == 0)) { return; }
	if(layer_select->currentIndex() == 2) { return; }
	
	//Temporarily revert highlighting to extract image
	u8 temp_x1 = min_x_rect;
	u8 temp_x2 = max_x_rect;
	u8 temp_y1 = min_y_rect;
	u8 temp_y2 = max_y_rect;

	min_x_rect = max_x_rect = min_y_rect = max_y_rect = 255;
	layer_change();

	//Temporarily convert dimensions to X,Y and WxH format for Qt - DMG/GBC BG version
	if(layer_select->currentIndex() == 0)
	{
		min_x_rect = ((rect_x->value() - 1) * 8) + (main_menu::gbe_plus->ex_read_u8(REG_SX) % 8);
		max_x_rect = rect_w->value() * 8;
		min_y_rect = (rect_y->value() - 1) * 8 + (main_menu::gbe_plus->ex_read_u8(REG_SY) % 8);
		max_y_rect = rect_h->value() * 8;
	}

	//Temporarily convert dimension to X,Y and WxH format for Qt - DMG/GBC Window version
	else if(layer_select->currentIndex() == 1)
	{
		u8 wx = main_menu::gbe_plus->ex_read_u8(REG_WX);
		if(wx < 7) { wx = 0; }

		min_x_rect = ((rect_x->value() - 1) * 8) + (wx % 8);
		max_x_rect = rect_w->value() * 8;
		min_y_rect = (rect_y->value() - 1) * 8 + (main_menu::gbe_plus->ex_read_u8(REG_WY) % 8);
		max_y_rect = rect_h->value() * 8;
	}

	QImage raw_screen = current_layer->pixmap()->toImage().scaled(160, 144);
	QRect rect(min_x_rect, min_y_rect, max_x_rect, max_y_rect);
	QString file_path(QString::fromStdString(config::data_path + cgfx::dump_bg_path + "test.bmp"));

	raw_screen = raw_screen.copy(rect);
	raw_screen.save(file_path);

	//Restore original highlighting
	min_x_rect = temp_x1;
	max_x_rect = temp_x2;
	min_y_rect = temp_y1;
	max_y_rect = temp_y2;
	layer_change();

	//Open manifest file, then write to it
	std::ofstream file(cgfx::manifest_file.c_str(), std::ios::out | std::ios::app);
	std::string entry = "";

	//TODO - Add a Qt warning here
	if(!file.is_open()) { return; }

	//Write main entry
	//TODO - Fill this in QLineEdit values
	entry = "[" + config::data_path + cgfx::dump_bg_path + "test.bmp" + ":" + "TEST" + "]";
	file << "\n" << entry;

	u8 entry_count = 0;

	//Generate manifest entries for selected tiles
	for(int y = min_y_rect; y < (max_y_rect + 1); y++)
	{
		for(int x = min_x_rect; x < (max_x_rect + 1); x++)
		{
			std::string gfx_name = "TEST_" + util::to_str(entry_count++);
			std::string gfx_type = (config::gb_type == 2) ? "20" : "10";
			
			entry = "[" + hash_tile((x * 8), (y * 8)) + ":" + gfx_name + ":" + gfx_type + ":0:0]";
			file << "\n" << entry;
		}
	}

	file.close();
}

/****** Hashes the tile from a given layer ******/
std::string gbe_cgfx::hash_tile(u8 x, u8 y)
{
	//Hash from DMG BG
	if((layer_select->currentIndex() == 0) && (config::gb_type < 2)) 
	{
		//Determine BG Map & Tile address
		u16 bg_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x8) ? 0x9C00 : 0x9800;
		u16 bg_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;

		//Determine the map entry from on-screen coordinates
		u8 tile_x = (x + main_menu::gbe_plus->ex_read_u8(REG_SX)) / 8;
		u8 tile_y = (y + main_menu::gbe_plus->ex_read_u8(REG_SY)) / 8;
		u16 map_entry = tile_x + (tile_y * 32);

		u8 map_value = main_menu::gbe_plus->ex_read_u8(bg_map_addr + map_entry);

		//Convert tile number to signed if necessary
		if(bg_tile_addr == 0x8800) 
		{
			if(map_value <= 127) { map_value += 128; }
			else { map_value -= 128; }
		}

		bg_tile_addr += (map_value * 16);

		return main_menu::gbe_plus->get_hash(bg_tile_addr, 10);
	}

	//Hash from GBC BG
	else if((layer_select->currentIndex() == 0) && (config::gb_type == 2))
	{
 		//Determine BG Map & Tile address
		u16 bg_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x8) ? 0x9C00 : 0x9800;
		u16 bg_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;

		//Determine the map entry from on-screen coordinates
		u8 tile_x = (x + main_menu::gbe_plus->ex_read_u8(REG_SX)) / 8;
		u8 tile_y = (y + main_menu::gbe_plus->ex_read_u8(REG_SY)) / 8;

		u16 map_entry = tile_x + (tile_y * 32);

		u8 old_vram_bank = main_menu::gbe_plus->ex_read_u8(REG_VBK);
			
		//Read the BG attributes
		main_menu::gbe_plus->ex_write_u8(REG_VBK, 1);
		u8 bg_attribute = main_menu::gbe_plus->ex_read_u8(bg_map_addr + map_entry);
		u8 pal_num = (bg_attribute & 0x7);
		u8 vram_bank = (bg_attribute & 0x8) ? 1 : 0;

		main_menu::gbe_plus->ex_write_u8(REG_VBK, 0);
		u8 map_value = main_menu::gbe_plus->ex_read_u8(bg_map_addr + map_entry);

		//Convert tile number to signed if necessary
		if(bg_tile_addr == 0x8800) 
		{
			if(map_value <= 127) { map_value += 128; }
			else { map_value -= 128; }
		}

		bg_tile_addr += (map_value * 16);

		main_menu::gbe_plus->ex_write_u8(REG_VBK, old_vram_bank);

		cgfx::gbc_bg_vram_bank = vram_bank;
		cgfx::gbc_bg_color_pal = pal_num;

		return main_menu::gbe_plus->get_hash(bg_tile_addr, 20);
	}

	//Hash from DMG Window
	else if((layer_select->currentIndex() == 1) && (config::gb_type < 2)) 
	{
		//Determine BG Map & Tile address
		u16 win_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x40) ? 0x9C00 : 0x9800;
		u16 bg_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;

		//Determine the map entry from on-screen coordinates
		u8 wx = main_menu::gbe_plus->ex_read_u8(REG_WX);
		wx = (wx < 7) ? 0 : (wx - 7); 
	
		u8 tile_x = (x - wx) / 8;
		u8 tile_y = (y - main_menu::gbe_plus->ex_read_u8(REG_WY)) / 8;
		u16 map_entry = tile_x + (tile_y * 32);

		u8 map_value = main_menu::gbe_plus->ex_read_u8(win_map_addr + map_entry);

		//Convert tile number to signed if necessary
		if(bg_tile_addr == 0x8800) 
		{
			if(map_value <= 127) { map_value += 128; }
			else { map_value -= 128; }
		}

		bg_tile_addr += (map_value * 16);

		return main_menu::gbe_plus->get_hash(bg_tile_addr, 10);
	}

	//Hash from GBC Window
	else if((layer_select->currentIndex() == 1) && (config::gb_type == 2))
	{
		//Determine BG Map & Tile address
		u16 win_map_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x40) ? 0x9C00 : 0x9800;
		u16 bg_tile_addr = (main_menu::gbe_plus->ex_read_u8(REG_LCDC) & 0x10) ? 0x8000 : 0x8800;
	
		//Determine the map entry from on-screen coordinates
		u8 wx = main_menu::gbe_plus->ex_read_u8(REG_WX);
		wx = (wx < 7) ? 0 : (wx - 7); 
	
		u8 tile_x = (x - wx) / 8;
		u8 tile_y = (y - main_menu::gbe_plus->ex_read_u8(REG_WY)) / 8;
		u16 map_entry = tile_x + (tile_y * 32);

		u8 old_vram_bank = main_menu::gbe_plus->ex_read_u8(REG_VBK);
			
		//Read the BG attributes
		main_menu::gbe_plus->ex_write_u8(REG_VBK, 1);
		u8 bg_attribute = main_menu::gbe_plus->ex_read_u8(win_map_addr + map_entry);
		u8 pal_num = (bg_attribute & 0x7);
		u8 vram_bank = (bg_attribute & 0x8) ? 1 : 0;

		main_menu::gbe_plus->ex_write_u8(REG_VBK, 0);
		u8 map_value = main_menu::gbe_plus->ex_read_u8(win_map_addr + map_entry);

		//Convert tile number to signed if necessary
		if(bg_tile_addr == 0x8800) 
		{
			if(map_value <= 127) { map_value += 128; }
			else { map_value -= 128; }
		}

		bg_tile_addr += (map_value * 16);

		main_menu::gbe_plus->ex_write_u8(REG_VBK, old_vram_bank);

		cgfx::gbc_bg_vram_bank = vram_bank;
		cgfx::gbc_bg_color_pal = pal_num;

		return main_menu::gbe_plus->get_hash(bg_tile_addr, 20);
	}

	else { return ""; }
}