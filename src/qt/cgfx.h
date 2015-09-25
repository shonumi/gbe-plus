// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : cgfx.h
// Date : July 25, 2015
// Description : Custom graphics settings
//
// Dialog for various custom graphics options

#ifndef CGFX_GBE_QT
#define CGFX_GBE_QT

#include <vector>

#include <SDL/SDL.h>

#include <QtGui>

#include "common/common.h"

class gbe_cgfx : public QDialog
{
	Q_OBJECT
	
	public:
	gbe_cgfx(QWidget *parent = 0);

	void update_obj_window(int rows, int count);
	void update_bg_window(int rows, int count);

	void draw_dmg_bg();
	void draw_dmg_win();
	void draw_dmg_obj();

	QTabWidget* tabs;
	QDialogButtonBox* tabs_button;

	//Configure tab widgets
	QCheckBox* auto_dump_obj;
	QCheckBox* auto_dump_bg;
	QCheckBox* blank;

	//Layers tab widgets
	QComboBox* layer_select;
	QLabel* tile_id;
	QLabel* tile_addr;
	QLabel* tile_size;
	QLabel* h_v_flip;
	QLabel* tile_palette;

	//OBJ tab widgets
	std::vector<QImage> cgfx_obj;
	std::vector<QPushButton*> obj_button;

	//BG tab widgets
	std::vector<QImage> cgfx_bg;
	std::vector<QPushButton*> bg_button;

	bool pause;

	protected:
	void closeEvent(QCloseEvent* event);
	bool eventFilter(QObject* target, QEvent* event);

	private:
	QWidget* obj_set;
	QWidget* bg_set;
	QWidget* layers_set;

	QGridLayout* obj_layout;
	QGridLayout* bg_layout;
	QGridLayout* layers_layout;

	QSignalMapper* obj_signal;
	QSignalMapper* bg_signal;

	std::vector<u8> estimated_palette;
	std::vector<u8> estimated_vram_bank;

	QLabel* current_layer;
	QLabel* current_tile;

	void setup_obj_window(int rows, int count);
	void update_preview(u32 x, u32 y);
	void dump_layer_tile(u32 x, u32 y);

	QImage grab_obj_data(int obj_index);
	QImage grab_dmg_obj_data(int obj_index);
	QImage grab_gbc_obj_data(int obj_index);

	void setup_bg_window(int rows, int count);

	QImage grab_bg_data(int bg_index);
	QImage grab_dmg_bg_data(int bg_index);
	QImage grab_gbc_bg_data(int bg_index);

	private slots:
	void close_cgfx();
	void dump_obj(int obj_index);
	void dump_bg(int bg_index);
	void set_auto_obj();
	void set_auto_bg();
	void set_blanks();
	void layer_change();
};

#endif //CGFX_GBE_QT 
