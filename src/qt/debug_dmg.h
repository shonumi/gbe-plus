// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : debug_dmg.h
// Date : November 21, 2015
// Description : DMG/GBC debugging UI
//
// Dialog for DMG/GBC debugging
// Shows MMIO registers, CPU state, instructions, memory

#ifndef DMG_DEBUG_GBE_QT
#define DMG_DEBUG_GBE_QT

#include <QtGui>

class dmg_debug : public QDialog
{
	Q_OBJECT
	
	public:
	dmg_debug(QWidget *parent = 0);

	QTabWidget* tabs;
	QDialogButtonBox* tabs_button;
	QPushButton* refresh_button;

	void auto_refresh();

	private:
	//MMIO registers
	QLineEdit* mmio_lcdc;
	QLineEdit* mmio_stat;
	QLineEdit* mmio_sx;
	QLineEdit* mmio_sy;
	QLineEdit* mmio_ly;
	QLineEdit* mmio_lyc;
	QLineEdit* mmio_dma;
	QLineEdit* mmio_bgp;
	QLineEdit* mmio_obp0;
	QLineEdit* mmio_obp1;
	QLineEdit* mmio_wx;
	QLineEdit* mmio_wy;

	QLineEdit* mmio_nr10;
	QLineEdit* mmio_nr11;
	QLineEdit* mmio_nr12;
	QLineEdit* mmio_nr13;
	QLineEdit* mmio_nr14;

	//Palette widgets
	QTableWidget* bg_pal_table;
	QTableWidget* obj_pal_table;

	QLabel* bg_pal_preview;
	QLabel* obj_pal_preview;

	QLabel* bg_r_label;
	QLabel* bg_g_label;
	QLabel* bg_b_label;

	QLabel* obj_r_label;
	QLabel* obj_g_label;
	QLabel* obj_b_label;

	private slots:
	void preview_bg_color(int y, int x);
	void preview_obj_color(int y, int x);
	void refresh();
};

#endif //DMG_DEBUG_GBE_QT
