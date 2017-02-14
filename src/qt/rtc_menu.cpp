// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : rtc_menu.cpp
// Date : December 10, 2016
// Description : Real-time clock menu
//
// Adjusts the RTC offsets 

#include "rtc_menu.h"

#include "common/config.h"
#include "common/util.h"

/****** RTC menu constructor ******/
rtc_menu::rtc_menu(QWidget *parent) : QDialog(parent)
{
	close_button = new QDialogButtonBox(QDialogButtonBox::Close);

	//Set up seconds
	QWidget* secs_set = new QWidget;
	QLabel* secs_label = new QLabel("Seconds Offset:");
	secs_offset = new QSpinBox;
	secs_offset->setMinimum(0);
	secs_offset->setMaximum(59);

	QHBoxLayout* secs_layout = new QHBoxLayout;
	secs_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	secs_layout->addWidget(secs_label, 1, Qt::AlignLeft);
	secs_layout->addWidget(secs_offset, 0, Qt::AlignRight);
	secs_set->setLayout(secs_layout);

	//Set up minutes
	QWidget* mins_set = new QWidget;
	QLabel* mins_label = new QLabel("Minutes Offset:");
	mins_offset = new QSpinBox;
	mins_offset->setMinimum(0);
	mins_offset->setMaximum(59);

	QHBoxLayout* mins_layout = new QHBoxLayout;
	mins_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	mins_layout->addWidget(mins_label, 1, Qt::AlignLeft);
	mins_layout->addWidget(mins_offset, 0, Qt::AlignRight);
	mins_set->setLayout(mins_layout);

	//Set up hours
	QWidget* hours_set = new QWidget;
	QLabel* hours_label = new QLabel("Hours Offset:");
	hours_offset = new QSpinBox;
	hours_offset->setMinimum(0);
	hours_offset->setMaximum(23);

	QHBoxLayout* hours_layout = new QHBoxLayout;
	hours_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	hours_layout->addWidget(hours_label, 1, Qt::AlignLeft);
	hours_layout->addWidget(hours_offset, 0, Qt::AlignRight);
	hours_set->setLayout(hours_layout);

	//Set up days
	QWidget* days_set = new QWidget;
	QLabel* days_label = new QLabel("Days Offset:");
	days_offset = new QSpinBox;
	days_offset->setMinimum(0);
	days_offset->setMaximum(365);

	QHBoxLayout* days_layout = new QHBoxLayout;
	days_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	days_layout->addWidget(days_label, 1, Qt::AlignLeft);
	days_layout->addWidget(days_offset, 0, Qt::AlignRight);
	days_set->setLayout(days_layout);

	//Set up months
	QWidget* months_set = new QWidget;
	QLabel* months_label = new QLabel("Months Offset:");
	months_offset = new QSpinBox;
	months_offset->setMinimum(0);
	months_offset->setMaximum(11);

	QHBoxLayout* months_layout = new QHBoxLayout;
	months_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	months_layout->addWidget(months_label, 1, Qt::AlignLeft);
	months_layout->addWidget(months_offset, 0, Qt::AlignRight);
	months_set->setLayout(months_layout);

	//Set up years
	QWidget* years_set = new QWidget;
	QLabel* years_label = new QLabel("Years Offset:");
	years_offset = new QSpinBox;
	years_offset->setMinimum(0);
	years_offset->setMaximum(100);

	QHBoxLayout* years_layout = new QHBoxLayout;
	years_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	years_layout->addWidget(years_label, 1, Qt::AlignLeft);
	years_layout->addWidget(years_offset, 0, Qt::AlignRight);
	years_set->setLayout(years_layout);

	//Final layout
	QVBoxLayout* final_layout = new QVBoxLayout;
	final_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	final_layout->addWidget(secs_set);
	final_layout->addWidget(mins_set);
	final_layout->addWidget(hours_set);
	final_layout->addWidget(days_set);
	final_layout->addWidget(months_set);
	final_layout->addWidget(years_set);
	final_layout->addWidget(close_button);
	setLayout(final_layout);

	hide();

	connect(close_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(close_button, SIGNAL(rejected()), this, SLOT(reject()));
	connect(secs_offset, SIGNAL(valueChanged(int)), this, SLOT(update_secs()));
	connect(mins_offset, SIGNAL(valueChanged(int)), this, SLOT(update_mins()));
	connect(hours_offset, SIGNAL(valueChanged(int)), this, SLOT(update_hours()));
	connect(days_offset, SIGNAL(valueChanged(int)), this, SLOT(update_days()));
	connect(months_offset, SIGNAL(valueChanged(int)), this, SLOT(update_months()));
	connect(years_offset, SIGNAL(valueChanged(int)), this, SLOT(update_years()));
}

/****** Updates the RTC offsets - Seconds ******/
void rtc_menu::update_secs() { config::rtc_offset[0] = secs_offset->value(); }

/****** Updates the RTC offsets - Minutes ******/
void rtc_menu::update_mins() { config::rtc_offset[1] = mins_offset->value(); }

/****** Updates the RTC offsets - Hours ******/
void rtc_menu::update_hours() { config::rtc_offset[2] = hours_offset->value(); }

/****** Updates the RTC offsets - Days ******/
void rtc_menu::update_days() { config::rtc_offset[3] = days_offset->value(); }

/****** Updates the RTC offsets - Months ******/
void rtc_menu::update_months() { config::rtc_offset[4] = months_offset->value(); }

/****** Updates the RTC offsets - Years ******/
void rtc_menu::update_years() { config::rtc_offset[5] = years_offset->value(); }
