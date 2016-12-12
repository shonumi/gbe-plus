// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : rtc_menu.h
// Date : December 10, 2016
// Description : Real-time clock menu
//
// Adjusts the RTC offsets

#ifndef RTCMENU_GBE_QT
#define RTCMENU_GBE_QT

#include <QtGui> 

class rtc_menu : public QDialog
{
	Q_OBJECT

	public:
	rtc_menu(QWidget *parent = 0);

	QDialogButtonBox* close_button;

	QSpinBox* secs_offset;
	QSpinBox* mins_offset;
	QSpinBox* hours_offset;
	QSpinBox* days_offset;

	private slots:
	void update_secs();
	void update_mins();
	void update_hours();
	void update_days();
};

#endif //RTCMENU_GBE_QT