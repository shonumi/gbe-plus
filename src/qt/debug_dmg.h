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

	u32 highlighted_dasm_line;

	QTextEdit* dasm;
	QString dasm_text;
	QString counter_text;

	void auto_refresh();
	void clear_format();

	private:
	//MMIO registers
	QLineEdit* mmio_lcdc;
	QLineEdit* mmio_stat;
	QLineEdit* mmio_sx;
	QLineEdit* mmio_sy;
	QLineEdit* mmio_ly;
	QLineEdit* mmio_lyc;
	QLineEdit* mmio_dma;
	QLineEdit* mmio_bgp;
	QLineEdit* mmio_obp0;
	QLineEdit* mmio_obp1;
	QLineEdit* mmio_wx;
	QLineEdit* mmio_wy;

	QLineEdit* mmio_nr10;
	QLineEdit* mmio_nr11;
	QLineEdit* mmio_nr12;
	QLineEdit* mmio_nr13;
	QLineEdit* mmio_nr14;

	//Palette widgets
	QTableWidget* bg_pal_table;
	QTableWidget* obj_pal_table;

	QLabel* bg_pal_preview;
	QLabel* obj_pal_preview;

	QLabel* bg_r_label;
	QLabel* bg_g_label;
	QLabel* bg_b_label;

	QLabel* obj_r_label;
	QLabel* obj_g_label;
	QLabel* obj_b_label;

	//Memory widgets
	QTextEdit* mem_addr;
	QTextEdit* mem_values;
	QTextEdit* mem_ascii;

	QString addr_text;
	QString values_text;
	QString ascii_text;

	std::string ascii_lookup;

	QScrollBar* mem_scrollbar;

	//Disassembler widgets
	QTextEdit* counter;

	QScrollBar* dasm_scrollbar;

	QLabel* af_label;
	QLabel* bc_label;
	QLabel* de_label;
	QLabel* hl_label;
	QLabel* pc_label;
	QLabel* sp_label;
	QLabel* flags_label;

	QPushButton* db_next_button;
	QPushButton* db_set_bp_button;
	QPushButton* db_clear_bp_button;
	QPushButton* db_continue_button;
	QPushButton* db_reset_button;
	QPushButton* db_reset_run_button;

	QTextBlockFormat original_format;

	bool text_select;

	int last_start;
	int last_stop;

	bool debug_reset;

	protected:
	void closeEvent(QCloseEvent* event);

	private slots:
	void preview_bg_color(int y, int x);
	void preview_obj_color(int y, int x);
	void scroll_mem(int value);
	void scroll_text(int type);
	void scroll_dasm(int value);
	void scroll_count(int type);
	void highlight();
	void refresh();
	void refresh_dasm();
	void close_debug();

	void db_next();
	void db_continue();
	void db_set_bp();
	void db_clear_bp();
	void db_reset();
	void db_reset_run();
};

void dmg_debug_step();

#endif //DMG_DEBUG_GBE_QT
