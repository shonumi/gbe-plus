// GB Enhanced+ Copyright Daniel Baxter 2018
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : constant_ir_menu.h
// Date : March 19, 2018
// Description : Constant IR Light menu
//
// Change the mode of the constant IR light source
// Static vs. Interactive

#ifndef CON_IR_MENU_GBE_QT
#define CON_IR_MENU_GBE_QT

#ifdef GBE_QT_5
#include <QtWidgets>
#endif

#ifdef GBE_QT_4
#include <QtGui>
#endif 

class con_ir_menu : public QDialog
{
	Q_OBJECT

	public:
	con_ir_menu(QWidget *parent = 0);

	QDialogButtonBox* close_button;

	QComboBox* ir_mode;

	private slots:
	void update_ir_mode();
};

#endif //CON_IR_MENU_GBE_QT 
