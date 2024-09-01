// GB Enhanced+ Copyright Daniel Baxter 2020
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : utp_menu.h
// Date : February 14, 2020
// Description : Ubisoft Thrustmaster Pedometer menu
//
// Sets steps for pedometer

#include "utp_menu.h"

#include "common/config.h"
#include "common/util.h"

/****** UTP menu constructor ******/
utp_menu::utp_menu(QWidget *parent) : QDialog(parent)
{
	close_button = new QDialogButtonBox(QDialogButtonBox::Close);

	//Pedometer steps menu
	QWidget* steps_set = new QWidget;
	QLabel* steps_label = new QLabel("Steps : ");
 	
	qt_steps = new QSpinBox;
	qt_steps->setMinimum(0);
	qt_steps->setMaximum(99999);

	QHBoxLayout* steps_layout = new QHBoxLayout;
	steps_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	steps_layout->addWidget(steps_label);
	steps_layout->addWidget(qt_steps);
	steps_set->setLayout(steps_layout);

	QVBoxLayout* final_layout = new QVBoxLayout;
	final_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	final_layout->addWidget(steps_set);
	final_layout->addWidget(close_button);
	setLayout(final_layout);

	setMinimumWidth(300);
	setWindowTitle(QString("Ubisoft Pedometer Configuration"));
	setWindowIcon(QIcon(QString::fromStdString(config::cfg_path + "data/icons/gbe_plus.png")));
	hide();

	connect(close_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(close_button, SIGNAL(rejected()), this, SLOT(reject()));
	connect(qt_steps, SIGNAL(valueChanged(int)), this, SLOT(update_steps()));
}

/****** Update number of steps ******/
void utp_menu::update_steps()
{
	u32 temp_val = qt_steps->value();
	std::string temp_str = util::to_str(temp_val);

	util::from_hex_str(temp_str, temp_val);
	config::utp_steps = temp_val;
}
