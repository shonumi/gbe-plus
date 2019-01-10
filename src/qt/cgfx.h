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

#include <SDL2/SDL.h>

#ifdef GBE_QT_5
#include <QtWidgets>
#endif

#ifdef GBE_QT_4
#include <QtGui>
#endif

#include "common/common.h"
#include "data_dialog.h"

class gbe_cgfx : public QDialog
{
	Q_OBJECT
	
	public:
	gbe_cgfx(QWidget *parent = 0);

	bool parse_manifest_items();

	void draw_gb_layer(u8 layer);
	void draw_dmg_obj();
	void draw_gbc_obj();

	void reset_inputs();

	QTabWidget* tabs;
	QDialogButtonBox* tabs_button;

	//Configure tab widgets
	QCheckBox* advanced;
	QCheckBox* blank;
	QCheckBox* auto_trans;

	//Advanced menu
	QWidget* advanced_box;
	QCheckBox* ext_vram;
	QCheckBox* ext_bright;

	QLabel* dest_label;
	QLineEdit* dest_folder;
	QPushButton* dest_browse;

	QLabel* name_label;
	QLineEdit* dest_name;
	QPushButton* name_browse;

	QDialogButtonBox* advanced_buttons;
	QPushButton* cancel_button;
	QPushButton* dump_button;

	//Layers tab widgets
	QComboBox* layer_select;
	QSpinBox* render_stop_line;
	QLabel* tile_id;
	QLabel* tile_addr;
	QLabel* tile_size;
	QLabel* h_v_flip;
	QLabel* tile_palette;
	QLabel* hash_text;
	QLabel* vram_text;
	QLabel* bright_text;

	QSpinBox* rect_x;
	QSpinBox* rect_w;
	QSpinBox* rect_y;
	QSpinBox* rect_h;

	QCheckBox* use_vram_addr;
	QCheckBox* use_auto_bright;
	QLineEdit* meta_name;

	//OBJ Meta tile tab widgets
	QSpinBox* obj_meta_width;
	QSpinBox* obj_meta_height;
	QSpinBox* obj_meta_index;

	QLabel* obj_meta_img;
	QLabel* obj_select_img;
	QImage obj_meta_pixel_data;

	QCheckBox* obj_meta_vram_addr;
	QCheckBox* obj_meta_auto_bright;

	QLineEdit* obj_meta_name;

	std::vector<std::string> obj_meta_str;
	std::vector<u16> obj_meta_addr;

	//Manifest tab widgets
	QScrollArea* manifest_display;

	data_dialog* data_folder;

	//Pop-ups
	QMessageBox* manifest_warning;
	QMessageBox* manifest_write_fail;
	QMessageBox* save_fail;
	QMessageBox* redump_hash;

	bool pause;
	bool enable_manifest_warning;
	bool enable_manifest_critical;
	bool redump;

	QImage grab_obj_data(int obj_index);
	QImage grab_dmg_obj_data(int obj_index);
	QImage grab_gbc_obj_data(int obj_index);

	QImage grab_bg_data(int bg_index);
	QImage grab_dmg_bg_data(int bg_index);
	QImage grab_gbc_bg_data(int bg_index);

	protected:
	void closeEvent(QCloseEvent* event);
	bool eventFilter(QObject* target, QEvent* event);
	void paintEvent(QPaintEvent* event);

	private:
	QWidget* layers_set;

	QGridLayout* obj_layout;
	QGridLayout* bg_layout;
	QGridLayout* layers_layout;

	QPushButton* a_input;
	QPushButton* b_input;
	QPushButton* select_input;
	QPushButton* start_input;
	QPushButton* left_input;
	QPushButton* right_input;
	QPushButton* up_input;
	QPushButton* down_input;

	std::vector<u8> estimated_palette;
	std::vector<u8> estimated_vram_bank;

	QLabel* current_layer;
	QLabel* current_tile;

	void update_preview(u32 x, u32 y);
	void dump_layer_tile(u32 x, u32 y);
	std::string hash_tile(u8 x, u8 y);

	u8 dump_type;
	int advanced_index;

	std::string last_custom_path;

	u8 min_x_rect, max_x_rect;
	u8 min_y_rect, max_y_rect;

	u32 mouse_start_x, mouse_start_y;
	bool mouse_drag;
	bool meta_highlight;

	private slots:
	void close_cgfx();
	void close_advanced();
	void dump_obj(int obj_index);
	void dump_bg(int bg_index);
	void redump_tile();
	void dump_selection();
	void dump_obj_meta_tile();
	void write_manifest_entry();
	bool delete_manifest_entry(int index);
	void show_advanced_obj(int index);
	void show_advanced_bg(int index);
	void browse_advanced_dir();
	void browse_advanced_file();
	void set_blanks();
	void set_auto_trans();
	void layer_change();
	void select_folder();
	void reject_folder();
	void update_selection();
	void ignore_manifest_warnings();
	void ignore_manifest_criticals();
	void advance_next_frame();
	void update_input_control(int index);
	void update_obj_meta_size();
	void select_obj();
};

#endif //CGFX_GBE_QT 
