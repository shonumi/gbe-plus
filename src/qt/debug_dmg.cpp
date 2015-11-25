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
	QDialog* palettes = new QDialog;
	QDialog* ram = new QDialog;
	QDialog* cpu_instr = new QDialog;
	QDialog* vram = new QDialog;

	tabs->addTab(io_regs, tr("I/O"));
	tabs->addTab(palettes, tr("Palettes"));
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
	QLabel* sx_label = new QLabel("0xFF43 - SX: \t");
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

	//LY
	QWidget* ly_set = new QWidget(io_regs);
	QLabel* ly_label = new QLabel("0xFF44 - LY: \t");
	mmio_ly = new QLineEdit(io_regs);
	mmio_ly->setMaximumWidth(64);
	mmio_ly->setReadOnly(true);

	QHBoxLayout* ly_layout = new QHBoxLayout;
	ly_layout->addWidget(ly_label, 0, Qt::AlignLeft);
	ly_layout->addWidget(mmio_ly, 0, Qt::AlignLeft);
	ly_set->setLayout(ly_layout);

	//LYC
	QWidget* lyc_set = new QWidget(io_regs);
	QLabel* lyc_label = new QLabel("0xFF45 - LYC: \t");
	mmio_lyc = new QLineEdit(io_regs);
	mmio_lyc->setMaximumWidth(64);
	mmio_lyc->setReadOnly(true);

	QHBoxLayout* lyc_layout = new QHBoxLayout;
	lyc_layout->addWidget(lyc_label, 0, Qt::AlignLeft);
	lyc_layout->addWidget(mmio_lyc, 0, Qt::AlignLeft);
	lyc_set->setLayout(lyc_layout);

	//DMA
	QWidget* dma_set = new QWidget(io_regs);
	QLabel* dma_label = new QLabel("0xFF46 - DMA: \t");
	mmio_dma = new QLineEdit(io_regs);
	mmio_dma->setMaximumWidth(64);
	mmio_dma->setReadOnly(true);

	QHBoxLayout* dma_layout = new QHBoxLayout;
	dma_layout->addWidget(dma_label, 0, Qt::AlignLeft);
	dma_layout->addWidget(mmio_dma, 0, Qt::AlignLeft);
	dma_set->setLayout(dma_layout);

	//BGP
	QWidget* bgp_set = new QWidget(io_regs);
	QLabel* bgp_label = new QLabel("0xFF47 - BGP: \t");
	mmio_bgp = new QLineEdit(io_regs);
	mmio_bgp->setMaximumWidth(64);
	mmio_bgp->setReadOnly(true);

	QHBoxLayout* bgp_layout = new QHBoxLayout;
	bgp_layout->addWidget(bgp_label, 0, Qt::AlignLeft);
	bgp_layout->addWidget(mmio_bgp, 0, Qt::AlignLeft);
	bgp_set->setLayout(bgp_layout);

	//OBP0
	QWidget* obp0_set = new QWidget(io_regs);
	QLabel* obp0_label = new QLabel("0xFF48 - OBP0: \t");
	mmio_obp0 = new QLineEdit(io_regs);
	mmio_obp0->setMaximumWidth(64);
	mmio_obp0->setReadOnly(true);

	QHBoxLayout* obp0_layout = new QHBoxLayout;
	obp0_layout->addWidget(obp0_label, 0, Qt::AlignLeft);
	obp0_layout->addWidget(mmio_obp0, 0, Qt::AlignLeft);
	obp0_set->setLayout(obp0_layout);

	//OBP1
	QWidget* obp1_set = new QWidget(io_regs);
	QLabel* obp1_label = new QLabel("0xFF49 - OBP1: \t");
	mmio_obp1 = new QLineEdit(io_regs);
	mmio_obp1->setMaximumWidth(64);
	mmio_obp1->setReadOnly(true);

	QHBoxLayout* obp1_layout = new QHBoxLayout;
	obp1_layout->addWidget(obp1_label, 0, Qt::AlignLeft);
	obp1_layout->addWidget(mmio_obp1, 0, Qt::AlignLeft);
	obp1_set->setLayout(obp1_layout);

	//WY
	QWidget* wy_set = new QWidget(io_regs);
	QLabel* wy_label = new QLabel("0xFF4A - WY: \t");
	mmio_wy = new QLineEdit(io_regs);
	mmio_wy->setMaximumWidth(64);
	mmio_wy->setReadOnly(true);

	QHBoxLayout* wy_layout = new QHBoxLayout;
	wy_layout->addWidget(wy_label, 0, Qt::AlignLeft);
	wy_layout->addWidget(mmio_wy, 0, Qt::AlignLeft);
	wy_set->setLayout(wy_layout);

	//WX
	QWidget* wx_set = new QWidget(io_regs);
	QLabel* wx_label = new QLabel("0xFF4B - WX: \t");
	mmio_wx = new QLineEdit(io_regs);
	mmio_wx->setMaximumWidth(64);
	mmio_wx->setReadOnly(true);

	QHBoxLayout* wx_layout = new QHBoxLayout;
	wx_layout->addWidget(wx_label, 0, Qt::AlignLeft);
	wx_layout->addWidget(mmio_wx, 0, Qt::AlignLeft);
	wx_set->setLayout(wx_layout);

	//NR10
	QWidget* nr10_set = new QWidget(io_regs);
	QLabel* nr10_label = new QLabel("0xFF10 - NR10: \t");
	mmio_nr10 = new QLineEdit(io_regs);
	mmio_nr10->setMaximumWidth(64);
	mmio_nr10->setReadOnly(true);

	QHBoxLayout* nr10_layout = new QHBoxLayout;
	nr10_layout->addWidget(nr10_label, 0, Qt::AlignLeft);
	nr10_layout->addWidget(mmio_nr10, 0, Qt::AlignLeft);
	nr10_set->setLayout(nr10_layout);

	//NR11
	QWidget* nr11_set = new QWidget(io_regs);
	QLabel* nr11_label = new QLabel("0xFF11 - NR11: \t");
	mmio_nr11 = new QLineEdit(io_regs);
	mmio_nr11->setMaximumWidth(64);
	mmio_nr11->setReadOnly(true);

	QHBoxLayout* nr11_layout = new QHBoxLayout;
	nr11_layout->addWidget(nr11_label, 0, Qt::AlignLeft);
	nr11_layout->addWidget(mmio_nr11, 0, Qt::AlignLeft);
	nr11_set->setLayout(nr11_layout);

	//NR12
	QWidget* nr12_set = new QWidget(io_regs);
	QLabel* nr12_label = new QLabel("0xFF12 - NR12: \t");
	mmio_nr12 = new QLineEdit(io_regs);
	mmio_nr12->setMaximumWidth(64);
	mmio_nr12->setReadOnly(true);

	QHBoxLayout* nr12_layout = new QHBoxLayout;
	nr12_layout->addWidget(nr12_label, 0, Qt::AlignLeft);
	nr12_layout->addWidget(mmio_nr12, 0, Qt::AlignLeft);
	nr12_set->setLayout(nr12_layout);

	//NR13
	QWidget* nr13_set = new QWidget(io_regs);
	QLabel* nr13_label = new QLabel("0xFF13 - NR13: \t");
	mmio_nr13 = new QLineEdit(io_regs);
	mmio_nr13->setMaximumWidth(64);
	mmio_nr13->setReadOnly(true);

	QHBoxLayout* nr13_layout = new QHBoxLayout;
	nr13_layout->addWidget(nr13_label, 0, Qt::AlignLeft);
	nr13_layout->addWidget(mmio_nr13, 0, Qt::AlignLeft);
	nr13_set->setLayout(nr13_layout);

	//NR14
	QWidget* nr14_set = new QWidget(io_regs);
	QLabel* nr14_label = new QLabel("0xFF14 - NR14: \t");
	mmio_nr14 = new QLineEdit(io_regs);
	mmio_nr14->setMaximumWidth(64);
	mmio_nr14->setReadOnly(true);

	QHBoxLayout* nr14_layout = new QHBoxLayout;
	nr14_layout->addWidget(nr14_label, 0, Qt::AlignLeft);
	nr14_layout->addWidget(mmio_nr14, 0, Qt::AlignLeft);
	nr14_set->setLayout(nr14_layout);

	//MMIO tab layout
	QGridLayout* io_layout = new QGridLayout;
	io_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	io_layout->addWidget(lcdc_set, 0, 0, 1, 1);
	io_layout->addWidget(stat_set, 1, 0, 1, 1);
	io_layout->addWidget(sy_set, 2, 0, 1, 1);
	io_layout->addWidget(sx_set, 3, 0, 1, 1);
	io_layout->addWidget(ly_set, 4, 0, 1, 1);
	io_layout->addWidget(lyc_set, 5, 0, 1, 1);
	io_layout->addWidget(dma_set, 6, 0, 1, 1);
	io_layout->addWidget(bgp_set, 7, 0, 1, 1);
	io_layout->addWidget(obp0_set, 8, 0, 1, 1);

	io_layout->addWidget(obp1_set, 0, 1, 1, 1);
	io_layout->addWidget(wy_set, 1, 1, 1, 1);
	io_layout->addWidget(wx_set, 2, 1, 1, 1);
	io_layout->addWidget(nr10_set, 3, 1, 1, 1);
	io_layout->addWidget(nr11_set, 4, 1, 1, 1);
	io_layout->addWidget(nr12_set, 5, 1, 1, 1);
	io_layout->addWidget(nr13_set, 6, 1, 1, 1);
	io_layout->addWidget(nr14_set, 7, 1, 1, 1);

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
	temp = main_menu::gbe_plus->ex_read_u8(REG_LCDC);
	mmio_lcdc->setText(QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().prepend("0x"));

	temp = main_menu::gbe_plus->ex_read_u8(REG_STAT);
	mmio_stat->setText(QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().prepend("0x"));

	temp = main_menu::gbe_plus->ex_read_u8(REG_SY);
	mmio_sy->setText(QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().prepend("0x"));

	temp = main_menu::gbe_plus->ex_read_u8(REG_SX);
	mmio_sx->setText(QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().prepend("0x"));

	temp = main_menu::gbe_plus->ex_read_u8(REG_LY);
	mmio_ly->setText(QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().prepend("0x"));

	temp = main_menu::gbe_plus->ex_read_u8(REG_LYC);
	mmio_lyc->setText(QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().prepend("0x"));

	temp = main_menu::gbe_plus->ex_read_u8(REG_DMA);
	mmio_dma->setText(QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().prepend("0x"));

	temp = main_menu::gbe_plus->ex_read_u8(REG_BGP);
	mmio_bgp->setText(QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().prepend("0x"));

	temp = main_menu::gbe_plus->ex_read_u8(REG_OBP0);
	mmio_obp0->setText(QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().prepend("0x"));

	temp = main_menu::gbe_plus->ex_read_u8(REG_OBP1);
	mmio_obp1->setText(QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().prepend("0x"));

	temp = main_menu::gbe_plus->ex_read_u8(REG_WY);
	mmio_wy->setText(QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().prepend("0x"));

	temp = main_menu::gbe_plus->ex_read_u8(REG_WX);
	mmio_wx->setText(QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().prepend("0x"));

	temp = main_menu::gbe_plus->ex_read_u8(NR10);
	mmio_nr10->setText(QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().prepend("0x"));

	temp = main_menu::gbe_plus->ex_read_u8(NR11);
	mmio_nr11->setText(QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().prepend("0x"));

	temp = main_menu::gbe_plus->ex_read_u8(NR12);
	mmio_nr12->setText(QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().prepend("0x"));

	temp = main_menu::gbe_plus->ex_read_u8(NR13);
	mmio_nr13->setText(QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().prepend("0x"));

	temp = main_menu::gbe_plus->ex_read_u8(NR14);
	mmio_nr14->setText(QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().prepend("0x"));
}

/****** Automatically refresh display data - Call this publically ******/
void dmg_debug::auto_refresh() { refresh(); }
