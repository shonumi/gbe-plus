// GB Enhanced+ Copyright Daniel Baxter 2019
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : turbo_file_menu.h
// Date : October 29, 2019
// Description : Turbo File GB and Turbo File Advance menu
//
// Sets write-protect switch on or off
// Enables or disabled memory card

#ifndef TBFMENU_GBE_QT
#define TBFMENU_GBE_QT

#ifdef GBE_QT_5
#include <QtWidgets>
#endif

#ifdef GBE_QT_4
#include <QtGui>
#endif

class tbf_menu : public QDialog
{
	Q_OBJECT

	public:
	tbf_menu(QWidget *parent = 0);

	QDialogButtonBox* close_button;

	QCheckBox* mem_card;
	QCheckBox* write_protect;

	private slots:
	void update_mem_card();
	void update_write_protect();
};

#endif //TBFMENU_GBE_QT  
