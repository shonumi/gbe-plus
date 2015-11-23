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

	private slots:
	void refresh();
};

#endif //DMG_DEBUG_GBE_QT
