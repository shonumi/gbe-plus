// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : debug_dmg.cpp
// Date : November 21, 2015
// Description : DMG/GBC debugging UI
//
// Dialog for DMG/GBC debugging
// Shows MMIO registers, CPU state, instructions, memory

#include "common/util.h"

#include "debug_dmg.h"
#include "main_menu.h"

/****** DMG/GBC debug constructor ******/
dmg_debug::dmg_debug(QWidget *parent) : QDialog(parent)
{
	//Set up tabs
	tabs = new QTabWidget(this);

	QDialog* io_regs = new QDialog;
	QDialog* palettes = new QDialog;
	QDialog* memory = new QDialog;
	QDialog* cpu_instr = new QDialog;
	QDialog* vram = new QDialog;

	tabs->addTab(io_regs, tr("I/O"));
	tabs->addTab(palettes, tr("Palettes"));
	tabs->addTab(memory, tr("Memory"));
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

	//Background palettes
	QWidget* bg_pal_set = new QWidget(palettes);
	bg_pal_set->setMaximumHeight(180);
	QLabel* bg_pal_label = new QLabel("Background Palettes", bg_pal_set);

	bg_pal_table = new QTableWidget(bg_pal_set);
	bg_pal_table->setRowCount(8);
	bg_pal_table->setColumnCount(4);
	bg_pal_table->setShowGrid(true);
	bg_pal_table->verticalHeader()->setVisible(false);
	bg_pal_table->verticalHeader()->setDefaultSectionSize(16);
	bg_pal_table->horizontalHeader()->setVisible(false);
	bg_pal_table->horizontalHeader()->setDefaultSectionSize(32);
	bg_pal_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	bg_pal_table->setSelectionMode(QAbstractItemView::NoSelection);
	bg_pal_table->setMaximumWidth(132);
	bg_pal_table->setMaximumHeight(132);

	QBrush blank_brush(Qt::black, Qt::BDiagPattern);

	for(u8 y = 0; y < 8; y++)
	{
		for(u8 x = 0; x < 4; x++)
		{
			bg_pal_table->setItem(y, x, new QTableWidgetItem);
			bg_pal_table->item(y, x)->setBackground(blank_brush);
		}
	}
	
	//Background palettes layout
	QVBoxLayout* bg_pal_layout = new QVBoxLayout;
	bg_pal_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	bg_pal_layout->addWidget(bg_pal_label);
	bg_pal_layout->addWidget(bg_pal_table);
	bg_pal_set->setLayout(bg_pal_layout);

	//OBJ palettes
	QWidget* obj_pal_set = new QWidget(palettes);
	obj_pal_set->setMaximumHeight(180);
	QLabel* obj_pal_label = new QLabel("OBJ Palettes", obj_pal_set);

	obj_pal_table = new QTableWidget(obj_pal_set);
	obj_pal_table->setRowCount(8);
	obj_pal_table->setColumnCount(4);
	obj_pal_table->setShowGrid(true);
	obj_pal_table->verticalHeader()->setVisible(false);
	obj_pal_table->verticalHeader()->setDefaultSectionSize(16);
	obj_pal_table->horizontalHeader()->setVisible(false);
	obj_pal_table->horizontalHeader()->setDefaultSectionSize(32);
	obj_pal_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	obj_pal_table->setSelectionMode(QAbstractItemView::NoSelection);
	obj_pal_table->setMaximumWidth(132);
	obj_pal_table->setMaximumHeight(132);

	for(u8 y = 0; y < 8; y++)
	{
		for(u8 x = 0; x < 4; x++)
		{
			obj_pal_table->setItem(y, x, new QTableWidgetItem);
			obj_pal_table->item(y, x)->setBackground(blank_brush);
		}
	}
	
	//OBJ palettes layout
	QVBoxLayout* obj_pal_layout = new QVBoxLayout;
	obj_pal_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	obj_pal_layout->addWidget(obj_pal_label);
	obj_pal_layout->addWidget(obj_pal_table);
	obj_pal_set->setLayout(obj_pal_layout);

	//BG palette preview
	QImage temp_pix(128, 128, QImage::Format_ARGB32);
	temp_pix.fill(qRgb(0, 0, 0));
	bg_pal_preview = new QLabel;
	bg_pal_preview->setPixmap(QPixmap::fromImage(temp_pix));

	//BG palettes labels + layout
	QWidget* bg_label_set = new QWidget(palettes);
	bg_r_label = new QLabel("R : \t", bg_label_set);
	bg_g_label = new QLabel("G : \t", bg_label_set);
	bg_b_label = new QLabel("B : \t", bg_label_set);
	
	QVBoxLayout* bg_label_layout = new QVBoxLayout;
	bg_label_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	bg_label_layout->addWidget(bg_r_label);
	bg_label_layout->addWidget(bg_g_label);
	bg_label_layout->addWidget(bg_b_label);
	bg_label_set->setLayout(bg_label_layout);
	bg_label_set->setMaximumHeight(128);

	//OBJ palette preview
	obj_pal_preview = new QLabel;
	obj_pal_preview->setPixmap(QPixmap::fromImage(temp_pix));

	//OBJ palettes labels + layout
	QWidget* obj_label_set = new QWidget(palettes);
	obj_r_label = new QLabel("R : \t", obj_label_set);
	obj_g_label = new QLabel("G : \t", obj_label_set);
	obj_b_label = new QLabel("B : \t", obj_label_set);
	
	QVBoxLayout* obj_label_layout = new QVBoxLayout;
	obj_label_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	obj_label_layout->addWidget(obj_r_label);
	obj_label_layout->addWidget(obj_g_label);
	obj_label_layout->addWidget(obj_b_label);
	obj_label_set->setLayout(obj_label_layout);
	obj_label_set->setMaximumHeight(128);
	
	//Palettes layout
	QGridLayout* palettes_layout = new QGridLayout;
	palettes_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	palettes_layout->addWidget(bg_pal_set, 0, 0, 1, 1);
	palettes_layout->addWidget(obj_pal_set, 1, 0, 1, 1);

	palettes_layout->addWidget(bg_pal_preview, 0, 1, 1, 1);
	palettes_layout->addWidget(obj_pal_preview, 1, 1, 1, 1);

	palettes_layout->addWidget(bg_label_set, 0, 2, 1, 1);
	palettes_layout->addWidget(obj_label_set, 1, 2, 1, 1);

	palettes->setLayout(palettes_layout);
	palettes->setMaximumWidth(500);

	//Memory
	QFont mono_font("Monospace");
	mono_font.setStyleHint(QFont::TypeWriter);

	QWidget* mem_set = new QWidget(memory);
	mem_set->setMinimumHeight(350);

	mem_addr = new QTextEdit(mem_set);
	mem_addr->setReadOnly(true);
	mem_addr->setFixedWidth(80);
	mem_addr->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	mem_addr->setFont(mono_font);

	mem_values = new QTextEdit(mem_set);
	mem_values->setReadOnly(true);
	mem_values->setFixedWidth(500);
	mem_values->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	mem_values->setTabStopWidth(28);
	mem_values->setFont(mono_font);

	mem_ascii = new QTextEdit(mem_set);
	mem_ascii->setReadOnly(true);
	mem_ascii->setFixedWidth(160);
	mem_ascii->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	mem_ascii->setFont(mono_font);

	//ASCII lookup setup
	for(u8 x = 0; x < 0x20; x++) { ascii_lookup += "."; }
	
	ascii_lookup += " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

	for(u16 x = 0x7F; x < 0x100; x++) { ascii_lookup += "."; }

	//Memory QString setup
	for(u32 x = 0; x < 0xFFFF; x += 16)
	{
		if((x + 16) < 0xFFFF) { addr_text += (QString("%1").arg(x, 4, 16, QChar('0')).toUpper().prepend("0x").append("\n")); }
		else { addr_text += (QString("%1").arg(x, 4, 16, QChar('0')).toUpper().prepend("0x")); }
	}

	for(u32 x = 0; x < 0x10000; x++)
	{
		if((x % 16 == 0) && (x > 0))
		{
			values_text += (QString("%1").arg(0, 2, 16, QChar('0')).toUpper().append("\t").prepend("\n"));

			std::string ascii_string(".");
			ascii_text += QString::fromStdString(ascii_string).prepend("\n");
		}

		else
		{
			values_text += (QString("%1").arg(0, 2, 16, QChar('0')).toUpper().append("\t"));

			std::string ascii_string(".");
			ascii_text += QString::fromStdString(ascii_string);
		}
	}

	mem_addr->setText(addr_text);
	mem_values->setText(values_text);
	mem_ascii->setText(ascii_text);

	//Memory main scrollbar
	mem_scrollbar = new QScrollBar(mem_set);
	mem_scrollbar->setRange(0, 4095);
	mem_scrollbar->setOrientation(Qt::Vertical);

	//Memory layout
	QHBoxLayout* mem_layout = new QHBoxLayout;
	mem_layout->setSpacing(0);
	mem_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	mem_layout->addWidget(mem_addr);
	mem_layout->addWidget(mem_values);
	mem_layout->addWidget(mem_ascii);
	mem_layout->addWidget(mem_scrollbar);
	mem_set->setLayout(mem_layout);

	//Disassembler
	QWidget* dasm_set = new QWidget(cpu_instr);
	dasm_set->setMinimumHeight(350);

	counter = new QTextEdit(dasm_set);
	counter->setReadOnly(true);
	counter->setFixedWidth(80);
	counter->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	counter->setFont(mono_font);

	dasm = new QTextEdit(dasm_set);
	dasm->setReadOnly(true);
	dasm->setFixedWidth(500);
	dasm->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	dasm->setFont(mono_font);

	//Disassembler QString setup
	for(u32 x = 0; x < 0x10000; x++)
	{
		counter_text += QString("%1").arg(x, 4, 16, QChar('0')).toUpper().prepend("0x").append("\n");
	}

	counter->setText(counter_text);

	//Disassembler main scrollbar
	dasm_scrollbar = new QScrollBar(dasm_set);
	dasm_scrollbar->setRange(0, 0xFFFF);
	dasm_scrollbar->setOrientation(Qt::Vertical);

	//Disassembler layout
	QHBoxLayout* dasm_layout = new QHBoxLayout;
	dasm_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	dasm_layout->setSpacing(0);
	dasm_layout->addWidget(counter);
	dasm_layout->addWidget(dasm);
	dasm_layout->addWidget(dasm_scrollbar);
	dasm_set->setLayout(dasm_layout);

	//Register labels
	QWidget* regs_set = new QWidget(cpu_instr);
	regs_set->setFont(mono_font);
	af_label = new QLabel("A: 0x00    F: 0x00", regs_set);
	bc_label = new QLabel("B: 0x00    C: 0x00", regs_set);
	de_label = new QLabel("D: 0x00    E: 0x00", regs_set);
	hl_label = new QLabel("H: 0x00    L: 0x00", regs_set);
	sp_label = new QLabel("SP: 0x0000", regs_set);
	pc_label = new QLabel("PC: 0x0000", regs_set);
	flags_label = new QLabel("Flags: ZNHC", regs_set);

	db_next_button = new QPushButton("Next");
	db_set_bp_button = new QPushButton("Set Breakpoint");
	db_continue_button = new QPushButton("Continue");
	db_reset_button = new QPushButton("Reset");
	db_reset_run_button = new QPushButton("Reset + Run");

	//Register layout
	QVBoxLayout* regs_layout = new QVBoxLayout;
	regs_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	regs_layout->setSpacing(4);
	regs_layout->addWidget(af_label);
	regs_layout->addWidget(bc_label);
	regs_layout->addWidget(de_label);
	regs_layout->addWidget(hl_label);
	regs_layout->addWidget(sp_label);
	regs_layout->addWidget(pc_label);
	regs_layout->addWidget(flags_label);
	regs_layout->addWidget(db_next_button);
	regs_layout->addWidget(db_continue_button);
	regs_layout->addWidget(db_set_bp_button);
	regs_layout->addWidget(db_reset_button);
	regs_layout->addWidget(db_reset_run_button);
	regs_set->setLayout(regs_layout);
	
	QHBoxLayout* instr_layout = new QHBoxLayout;
	instr_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	instr_layout->setSpacing(0);
	instr_layout->addWidget(dasm_set);
	instr_layout->addWidget(regs_set);
	instr_layout->setStretchFactor(regs_set, 15);
	cpu_instr->setLayout(instr_layout);

	refresh_button = new QPushButton("Refresh");
	tabs_button = new QDialogButtonBox(QDialogButtonBox::Close);
	tabs_button->addButton(refresh_button, QDialogButtonBox::ActionRole);

	//Final tab layout
	QVBoxLayout* main_layout = new QVBoxLayout;
	main_layout->addWidget(tabs);
	main_layout->addWidget(tabs_button);
	setLayout(main_layout);

	connect(tabs_button, SIGNAL(accepted()), this, SLOT(accept()));
	connect(tabs_button, SIGNAL(rejected()), this, SLOT(reject()));
	connect(bg_pal_table, SIGNAL(cellClicked (int, int)), this, SLOT(preview_bg_color(int, int)));
	connect(obj_pal_table, SIGNAL(cellClicked (int, int)), this, SLOT(preview_obj_color(int, int)));
	connect(mem_scrollbar, SIGNAL(valueChanged(int)), this, SLOT(scroll_mem(int)));
	connect(dasm_scrollbar, SIGNAL(valueChanged(int)), this, SLOT(scroll_dasm(int)));
	connect(dasm, SIGNAL(cursorPositionChanged()), this, SLOT(highlight()));
	connect(db_next_button, SIGNAL(clicked()), this, SLOT(db_next()));
	connect(db_continue_button, SIGNAL(clicked()), this, SLOT(db_continue()));
	connect(db_set_bp_button, SIGNAL(clicked()), this, SLOT(db_set_bp()));
	connect(db_reset_button, SIGNAL(clicked()), this, SLOT(db_reset()));
	connect(db_reset_run_button, SIGNAL(clicked()), this, SLOT(db_reset_run()));
	connect(refresh_button, SIGNAL(clicked()), this, SLOT(refresh()));
	connect(tabs_button->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(close_debug()));

	QSignalMapper* text_mapper = new QSignalMapper(this);
	connect(mem_addr->verticalScrollBar(), SIGNAL(valueChanged(int)), text_mapper, SLOT(map()));
	connect(mem_values->verticalScrollBar(), SIGNAL(valueChanged(int)), text_mapper, SLOT(map()));
	connect(mem_ascii->verticalScrollBar(), SIGNAL(valueChanged(int)), text_mapper, SLOT(map()));

	text_mapper->setMapping(mem_addr->verticalScrollBar(), 0);
	text_mapper->setMapping(mem_values->verticalScrollBar(), 1);
	text_mapper->setMapping(mem_ascii->verticalScrollBar(), 2);
	connect(text_mapper, SIGNAL(mapped(int)), this, SLOT(scroll_text(int)));

	QSignalMapper* dasm_mapper = new QSignalMapper(this);
	connect(counter->verticalScrollBar(), SIGNAL(valueChanged(int)), dasm_mapper, SLOT(map()));
	connect(dasm->verticalScrollBar(), SIGNAL(valueChanged(int)), dasm_mapper, SLOT(map()));

	dasm_mapper->setMapping(counter->verticalScrollBar(), 0);
	dasm_mapper->setMapping(dasm->verticalScrollBar(), 1);
	connect(dasm_mapper, SIGNAL(mapped(int)), this, SLOT(scroll_count(int)));

	resize(800, 450);
	setWindowTitle(tr("DMG-GBC Debugger"));

	debug_reset = true;
	text_select = 0;
}

/****** Closes the debugging window ******/
void dmg_debug::closeEvent(QCloseEvent* event) { close_debug(); }

/****** Closes the debugging window ******/
void dmg_debug::close_debug() 
{
	main_menu::gbe_plus->db_unit.debug_mode = false;
	main_menu::gbe_plus->db_unit.last_command = "dq";
}

/****** Refresh the display data ******/
void dmg_debug::refresh() 
{
	u16 temp = 0;
	std::string temp_str = "";

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

	//DMG Palettes
	if(config::gb_type != 2)
	{
		//BG
		u8 ex_bgp[4];
		temp = main_menu::gbe_plus->ex_read_u8(REG_BGP);

		ex_bgp[0] = temp  & 0x3;
		ex_bgp[1] = (temp >> 2) & 0x3;
		ex_bgp[2] = (temp >> 4) & 0x3;
		ex_bgp[3] = (temp >> 6) & 0x3;
	
		for(u8 x = 0; x < 4; x++)
		{
			switch(ex_bgp[x])
			{
				case 0: bg_pal_table->item(0, x)->setBackground(QColor(0xFF, 0xFF, 0xFF)); break;
				case 1: bg_pal_table->item(0, x)->setBackground(QColor(0xC0, 0xC0, 0xC0)); break;
				case 2: bg_pal_table->item(0, x)->setBackground(QColor(0x60, 0x60, 0x60)); break;
				case 3: bg_pal_table->item(0, x)->setBackground(QColor(0, 0, 0)); break;
			}
		}

		//OBJ
		u8 ex_obp[4][2];
		temp = main_menu::gbe_plus->ex_read_u8(REG_OBP0);

		ex_obp[0][0] = temp  & 0x3;
		ex_obp[1][0] = (temp >> 2) & 0x3;
		ex_obp[2][0] = (temp >> 4) & 0x3;
		ex_obp[3][0] = (temp >> 6) & 0x3;

		temp = main_menu::gbe_plus->ex_read_u8(REG_OBP1);

		ex_obp[0][1] = temp  & 0x3;
		ex_obp[1][1] = (temp >> 2) & 0x3;
		ex_obp[2][1] = (temp >> 4) & 0x3;
		ex_obp[3][1] = (temp >> 6) & 0x3;

		for(u8 y = 0; y < 2; y++)
		{
			for(u8 x = 0; x < 4; x++)
			{
				switch(ex_obp[x][y])
				{
					case 0: obj_pal_table->item(y, x)->setBackground(QColor(0xFF, 0xFF, 0xFF)); break;
					case 1: obj_pal_table->item(y, x)->setBackground(QColor(0xC0, 0xC0, 0xC0)); break;
					case 2: obj_pal_table->item(y, x)->setBackground(QColor(0x60, 0x60, 0x60)); break;
					case 3: obj_pal_table->item(y, x)->setBackground(QColor(0, 0, 0)); break;
				}
			}
		}
	}

	//GBC Palettes
	else
	{
		//BG
		for(u8 y = 0; y < 8; y++)
		{
			u32* color = main_menu::gbe_plus->get_bg_palette(y);

			for(u8 x = 0; x < 4; x++)
			{
				u8 blue = (*color) & 0xFF;
				u8 green = ((*color) >> 8) & 0xFF;
				u8 red = ((*color) >> 16) & 0xFF;

				bg_pal_table->item(y, x)->setBackground(QColor(red, green, blue));
				color += 8;
			}
		}

		//OBJ
		for(u8 y = 0; y < 8; y++)
		{
			u32* color = main_menu::gbe_plus->get_obj_palette(y);

			for(u8 x = 0; x < 4; x++)
			{
				u8 blue = (*color) & 0xFF;
				u8 green = ((*color) >> 8) & 0xFF;
				u8 red = ((*color) >> 16) & 0xFF;

				obj_pal_table->item(y, x)->setBackground(QColor(red, green, blue));
				color += 8;
			}
		}
	}

	//Memory viewer
	values_text.clear();
	ascii_text.clear();

	for(u32 x = 0; x < 0x10000; x++)
	{
		temp = main_menu::gbe_plus->ex_read_u8(x); 

		if((x % 16 == 0) && (x > 0))
		{
			values_text += (QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().append("\t").prepend("\n"));

			std::string ascii_string = ascii_lookup.substr(temp, 1);
			ascii_text += QString::fromStdString(ascii_string).prepend("\n");
		}

		else
		{
			values_text += (QString("%1").arg(temp, 2, 16, QChar('0')).toUpper().append("\t"));

			std::string ascii_string = ascii_lookup.substr(temp, 1);
			ascii_text += QString::fromStdString(ascii_string);
		}
	}

	mem_values->setText(values_text);
	mem_ascii->setText(ascii_text);

	if(debug_reset)
	{
		//Populate initial disassembly text
		for(u32 x = 0; x < 0x10000; x++)
		{
			temp = main_menu::gbe_plus->ex_read_u8(x);
			temp_str = util::to_hex_str(temp);

			temp_str += " --> " + main_menu::gbe_plus->debug_get_mnemonic(x) + "\n";
			dasm_text += QString::fromStdString(temp_str);
		}

		dasm->setText(dasm_text);
		debug_reset = false;
		refresh();
	}

	//Update register values
	temp_str = "A: " + util::to_hex_str(main_menu::gbe_plus->ex_get_reg(0)) + "    F: " + util::to_hex_str(main_menu::gbe_plus->ex_get_reg(7));
	af_label->setText(QString::fromStdString(temp_str));

	temp_str = "B: " + util::to_hex_str(main_menu::gbe_plus->ex_get_reg(1)) + "    C: " + util::to_hex_str(main_menu::gbe_plus->ex_get_reg(2));
	bc_label->setText(QString::fromStdString(temp_str));

	temp_str = "D: " + util::to_hex_str(main_menu::gbe_plus->ex_get_reg(3)) + "    E: " + util::to_hex_str(main_menu::gbe_plus->ex_get_reg(4));
	de_label->setText(QString::fromStdString(temp_str));

	temp_str = "H: " + util::to_hex_str(main_menu::gbe_plus->ex_get_reg(5)) + "    L: " + util::to_hex_str(main_menu::gbe_plus->ex_get_reg(6));
	hl_label->setText(QString::fromStdString(temp_str));

	temp_str = "SP: " + util::to_hex_str(main_menu::gbe_plus->ex_get_reg(8));
	sp_label->setText(QString::fromStdString(temp_str));

	temp_str = "PC: " + util::to_hex_str(main_menu::gbe_plus->ex_get_reg(9));
	pc_label->setText(QString::fromStdString(temp_str));

	temp_str = "Flags: ";
	temp = main_menu::gbe_plus->ex_get_reg(7);

	temp_str += (temp & 0x80) ? "Z" : ".";
	temp_str += (temp & 0x40) ? "N" : ".";
	temp_str += (temp & 0x20) ? "H" : ".";
	temp_str += (temp & 0x10) ? "C" : ".";

	flags_label->setText(QString::fromStdString(temp_str));

	//Scroll disassembly to the PC
	temp = main_menu::gbe_plus->ex_get_reg(9);

	scroll_dasm(temp);
	text_select = true;
	highlight();

	if(main_menu::gbe_plus->db_unit.last_command != "c") { main_menu::gbe_plus->db_unit.last_command = ""; }
}

/****** Updates certain parts of the disassembly text (RAM) ******/
void dmg_debug::refresh_dasm() { }

/****** Updates a preview of the selected BG Color ******/
void dmg_debug::preview_bg_color(int y, int x)
{
	QImage temp_pix(128, 128, QImage::Format_ARGB32);
	temp_pix.fill(bg_pal_table->item(y, x)->background().color());
	bg_pal_preview->setPixmap(QPixmap::fromImage(temp_pix));

	int r, g, b = 0;
	bg_pal_table->item(y, x)->background().color().getRgb(&r, &g, &b);

	bg_r_label->setText(QString::number(r/8).prepend("R : ").append("\t"));
	bg_g_label->setText(QString::number(g/8).prepend("G : ").append("\t"));
	bg_b_label->setText(QString::number(b/8).prepend("B : ").append("\t"));
}

/****** Updates a preview of the selected OBJ Color ******/
void dmg_debug::preview_obj_color(int y, int x)
{
	QImage temp_pix(128, 128, QImage::Format_ARGB32);
	temp_pix.fill(obj_pal_table->item(y, x)->background().color());
	obj_pal_preview->setPixmap(QPixmap::fromImage(temp_pix));

	int r, g, b = 0;
	obj_pal_table->item(y, x)->background().color().getRgb(&r, &g, &b);

	obj_r_label->setText(QString::number(r/8).prepend("R : ").append("\t"));
	obj_g_label->setText(QString::number(g/8).prepend("G : ").append("\t"));
	obj_b_label->setText(QString::number(b/8).prepend("B : ").append("\t"));
}

/****** Scrolls everything in the memory tab via main scrollbar ******/
void dmg_debug::scroll_mem(int value)
{
	mem_addr->setTextCursor(QTextCursor(mem_addr->document()->findBlockByLineNumber(value)));
	mem_values->setTextCursor(QTextCursor(mem_values->document()->findBlockByLineNumber(value)));
	mem_ascii->setTextCursor(QTextCursor(mem_ascii->document()->findBlockByLineNumber(value)));
	mem_scrollbar->setValue(value);
}

/****** Scrolls every QTextEdit in the memory tab ******/
void dmg_debug::scroll_text(int type) 
{
	int line_number = 0;
	int value = 0;

	//Scroll based on Address
	if(type == 0)
	{
		line_number = mem_addr->textCursor().blockNumber();
		mem_scrollbar->setValue(line_number);

		value = mem_addr->verticalScrollBar()->value();
		mem_values->verticalScrollBar()->setValue(value);
		mem_ascii->verticalScrollBar()->setValue(value);
	}

	//Scroll based on Values
	else if(type == 1)
	{
		line_number = mem_values->textCursor().blockNumber();
		mem_scrollbar->setValue(line_number);

		value = mem_values->verticalScrollBar()->value();
		mem_addr->verticalScrollBar()->setValue(value);
		mem_ascii->verticalScrollBar()->setValue(value);
	}

	//Scroll based on ASCII
	else if(type == 2)
	{
		int line_number = mem_ascii->textCursor().blockNumber();
		mem_scrollbar->setValue(line_number);

		value = mem_ascii->verticalScrollBar()->value();
		mem_addr->verticalScrollBar()->setValue(value);
		mem_values->verticalScrollBar()->setValue(value);
	}
}

/****** Scrolls all everything in the disassembly tab via main scrollbar ******/
void dmg_debug::scroll_dasm(int value)
{
	text_select = false;
	counter->setTextCursor(QTextCursor(counter->document()->findBlockByLineNumber(value)));
	dasm->setTextCursor(QTextCursor(dasm->document()->findBlockByLineNumber(value)));
	dasm_scrollbar->setValue(value);

	highlighted_dasm_line = value;
}

/****** Scrolls every QTextEdit in the disassembly tab ******/
void dmg_debug::scroll_count(int type)
{
	int line_number = 0;
	int value = 0;

	//Scroll based on Counter
	if(type == 0)
	{
		line_number = counter->textCursor().blockNumber();
		dasm_scrollbar->setValue(line_number);

		value = counter->verticalScrollBar()->value();
		dasm->verticalScrollBar()->setValue(value);
	}

	//Scroll based on Disassembly
	else if(type == 1)
	{
		line_number = dasm->textCursor().blockNumber();
		dasm_scrollbar->setValue(line_number);

		value = dasm->verticalScrollBar()->value();
		counter->verticalScrollBar()->setValue(value);
	}
}

/****** Highlights a block of text for the disassembler ******/
void dmg_debug::highlight()
{
	if(text_select)
	{
		QTextCursor cursor = dasm->textCursor();
		int start_pos = cursor.block().position();
		int stop_pos = start_pos + cursor.block().length() - 1;

		cursor.setPosition(start_pos);
		cursor.setPosition(stop_pos, QTextCursor::KeepAnchor);

		dasm->setTextCursor(cursor);
	}

	else { text_select = true; }

	highlighted_dasm_line = dasm->textCursor().blockNumber();
}

/****** Automatically refresh display data - Call this publically ******/
void dmg_debug::auto_refresh() { refresh(); }

/****** Moves the debugger one instruction in disassembly ******/
void dmg_debug::db_next() 
{
	if(main_menu::gbe_plus->db_unit.last_command != "c") { main_menu::gbe_plus->db_unit.last_command = "n"; }
}

/****** Continues emulation until debugger hits breakpoint ******/
void dmg_debug::db_continue() { main_menu::gbe_plus->db_unit.last_command = "c"; }

/****** Sets breakpoint at current PC ******/
void dmg_debug::db_set_bp() 
{
	if(main_menu::gbe_plus->db_unit.last_command != "c") { main_menu::gbe_plus->db_unit.last_command = "bp"; }
}

/****** Resets emulation then stops ******/
void dmg_debug::db_reset() { main_menu::gbe_plus->db_unit.last_command = "rs"; }

/****** Resets emulation and runs in continue mode ******/
void dmg_debug::db_reset_run() { main_menu::gbe_plus->db_unit.last_command = "rsr"; }

/****** Steps through the debugger via the GUI ******/
void dmg_debug_step()
{
	bool halt = true;
	bool bp_continue = true;

	//Continue until breakpoint
	if(main_menu::gbe_plus->db_unit.last_command == "c")
	{
		if(main_menu::gbe_plus->db_unit.breakpoints.size() > 0)
		{
			for(int x = 0; x < main_menu::gbe_plus->db_unit.breakpoints.size(); x++)
			{
				//When a BP is matched, display info, wait for next input command
				if(main_menu::gbe_plus->ex_get_reg(9) == main_menu::gbe_plus->db_unit.breakpoints[x])
				{
					main_menu::gbe_plus->db_unit.last_command = "";
					bp_continue = false;
				}
			}
		}

		if(bp_continue) { return; }
	}

	main_menu::dmg_debugger->auto_refresh();

	//Wait for GUI action
	while((main_menu::gbe_plus->db_unit.last_command == "") && (halt))
	{
		SDL_Delay(16);
		QApplication::processEvents();
		if(SDL_GetAudioStatus() != SDL_AUDIO_PAUSED) { SDL_PauseAudio(1); }

		//Step once
		if(main_menu::gbe_plus->db_unit.last_command == "n") 
		{
			halt = false;
		}

		//Set breakpoint at current PC
		else if(main_menu::gbe_plus->db_unit.last_command == "bp")
		{
			main_menu::gbe_plus->db_unit.breakpoints.push_back(main_menu::dmg_debugger->highlighted_dasm_line);
			main_menu::gbe_plus->db_unit.last_command = "";
		}

		//Resets debugging, then stops
		else if(main_menu::gbe_plus->db_unit.last_command == "rs")
		{
			main_menu::gbe_plus->db_unit.last_command = "";
			main_menu::gbe_plus->reset();
			main_menu::dmg_debugger->auto_refresh();
		}

		//Resets debugging, then runs
		else if(main_menu::gbe_plus->db_unit.last_command == "rsr")
		{
			main_menu::gbe_plus->db_unit.last_command = "c";
			main_menu::gbe_plus->reset();
		}

		//Stop debugging
		else if(main_menu::gbe_plus->db_unit.last_command == "dq") 
		{
			halt = false;
			if(!config::pause_emu) { SDL_PauseAudio(0); }
		}

		//Continue waiting for a valid debugging command
		else { halt = true; }
	}
}
