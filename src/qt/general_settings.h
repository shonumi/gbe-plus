// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : general_settings.h
// Date : June 28, 2015
// Description : Main menu
//
// Dialog for various options
// Deals with Graphics, Audio, Input, Paths, etc

#ifndef SETTINGS_GBE_QT
#define SETTINGS_GBE_QT

#include <QtGui>

class gen_settings : public QDialog
{
	Q_OBJECT
	
	public:
	gen_settings(QWidget *parent = 0);

	QTabWidget* tabs;
	QDialogButtonBox* tabs_button;

	//General tab widgets
	QComboBox* sys_type;
	QCheckBox* bios;

	//Display tab widgets
	QComboBox* screen_scale;
	QCheckBox* ogl;

	//Sound tab widgets
	QComboBox* freq;
	QSlider* volume;
	QCheckBox* sound_on;

	double sample_rate;

	private slots:
	void volume_change();
	void sample_rate_change();
	void mute();
};

#endif //SETTINGS_GBE_QT 
