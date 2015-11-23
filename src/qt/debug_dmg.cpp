// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : debug_dmg.cpp
// Date : November 21, 2015
// Description : DMG/GBC debugging UI
//
// Dialog for DMG/GBC debugging
// Shows MMIO registers, CPU state, instructions, memory

#include "debug_dmg.h"
#include "main_menu.h"

/****** DMG/GBC debug constructor ******/
dmg_debug::dmg_debug(QWidget *parent) : QDialog(parent)
{
	//Set up tabs
	tabs = new QTabWidget(this);

	QDialog* io_regs = new QDialog;
	QDialog* ram = new QDialog;
	QDialog* cpu_instr = new QDialog;
	QDialog* vram = new QDialog;

	tabs->addTab(io_regs, tr("I/O"));
	tabs->addTab(ram, tr("RAM"));
	tabs->addTab(cpu_instr, tr("Disassembly"));
	tabs->addTab(vram, tr("Graphics"));

	//LCDC
	QWidget* lcdc_set = new QWidget(io_regs);
	QLabel* lcdc_label = new QLabel("0xFF40 - LCDC: \t");
	mmio_lcdc = new QLineEdit(io_regs);
	mmio_lcdc->setMaximumWidth(64);
	mmio_lcdc->setReadOnly(true);

	QHBoxLayout* lcdc_layout = new QHBoxLayout;
	lcdc_layout->addWidget(lcdc_label, 0, Qt::AlignLeft);
	lcdc_layout->addWidget(mmio_lcdc, 0, Qt::AlignLeft);
	lcdc_set->setLayout(lcdc_layout);

	//STAT
	QWidget* stat_set = new QWidget(io_regs);
	QLabel* stat_label = new QLabel("0xFF41 - STAT: \t");
	mmio_stat = new QLineEdit(io_regs);
	mmio_stat->setMaximumWidth(64);
	mmio_stat->setReadOnly(true);

	QHBoxLayout* stat_layout = new QHBoxLayout;
	stat_layout->addWidget(stat_label, 0, Qt::AlignLeft);
	stat_layout->addWidget(mmio_stat, 0, Qt::AlignLeft);
	stat_set->setLayout(stat_layout);

	//SX
	QWidget* sx_set = new QWidget(io_regs);
	QLabel* sx_label = new QLabel("0xFF41 - SX: \t");
	mmio_sx = new QLineEdit(io_regs);
	mmio_sx->setMaximumWidth(64);
	mmio_sx->setReadOnly(true);

	QHBoxLayout* sx_layout = new QHBoxLayout;
	sx_layout->addWidget(sx_label, 0, Qt::AlignLeft);
	sx_layout->addWidget(mmio_sx, 0, Qt::AlignLeft);
	sx_set->setLayout(sx_layout);

	//SY
	QWidget* sy_set = new QWidget(io_regs);
	QLabel* sy_label = new QLabel("0xFF42 - SY: \t");
	mmio_sy = new QLineEdit(io_regs);
	mmio_sy->setMaximumWidth(64);
	mmio_sy->setReadOnly(true);

	QHBoxLayout* sy_layout = new QHBoxLayout;
	sy_layout->addWidget(sy_label, 0, Qt::AlignLeft);
	sy_layout->addWidget(mmio_sy, 0, Qt::AlignLeft);
	sy_set->setLayout(sy_layout);

	//MMIO tab layout
	QVBoxLayout* io_layout = new QVBoxLayout;
	io_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	io_layout->addWidget(lcdc_set);
	io_layout->addWidget(stat_set);
	io_layout->addWidget(sx_set);
	io_layout->addWidget(sy_set);
	io_regs->setLayout(io_layout);

	tabs_button = new QDialogButtonBox(QDialogButtonBox::Close);

	//Final tab layout
	QVBoxLayout* main_layout = new QVBoxLayout;
	main_layout->addWidget(tabs);
	main_layout->addWidget(tabs_button);
	setLayout(main_layout);

	connect(tabs_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(tabs_button, SIGNAL(rejected()), this, SLOT(reject()));

	resize(800, 450);
	setWindowTitle(tr("DMG-GBC Debugger"));
}

/****** Refresh the display data ******/
void dmg_debug::refresh() 
{
	u16 temp = 0;

	//Update I/O regs
	temp = main_menu::gbe_plus->ex_read_u8(0xFF40);
	mmio_lcdc->setText(QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().prepend("0x"));

	temp = main_menu::gbe_plus->ex_read_u8(0xFF41);
	mmio_stat->setText(QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().prepend("0x"));

	temp = main_menu::gbe_plus->ex_read_u8(0xFF42);
	mmio_sx->setText(QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().prepend("0x"));

	temp = main_menu::gbe_plus->ex_read_u8(0xFF43);
	mmio_sy->setText(QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().prepend("0x"));
}

/****** Automatically refresh display data - Call this publically ******/
void dmg_debug::auto_refresh() { refresh(); }
