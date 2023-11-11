// GB Enhanced+ Copyright Daniel Baxter 2020
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mmu.h
// Date : December 01, 2020
// Description : Pokemon Mini memory manager unit
//
// Handles reading and writing bytes to memory locations

#ifndef PM_MMU
#define PM_MMU

#ifdef GBE_NETPLAY
#include <SDL2/SDL_net.h>
#endif

#include <fstream>
#include <string>
#include <vector>
#include <iostream>

#include "common.h"
#include "gamepad.h"
#include "common/config.h"
#include "common/util.h"
#include "timer.h"
#include "lcd_data.h"
#include "apu_data.h"

enum min_lcd_commands
{ 
	SED1565_NOP,
	SED1565_RESET,
	SED1565_END,
	SET_COLUMN_HI,
	SET_COLUMN_LO,
	DISPLAY_LINE_START,
	SET_PAGE,
	SET_CONTRAST,
	SET_ENTIRE_DISPLAY,
	SET_DISPLAY_ON_OFF,
	SEGMENT_DRIVER_SELECTION,
	READ_MODIFY_WRITE,
};

class MIN_MMU
{
	public:

	std::vector <u8> memory_map;

	u8 read_u8(u32 address);
	s8 read_s8(u32 address);
	u16 read_u16(u32 address);
	s16 read_s16(u32 address);

	void write_u8(u32 address, u8 value);
	void write_u16(u32 address, u16 value);

	bool read_file(std::string filename);
	bool read_bios(std::string filename);
	bool load_backup(std::string filename);
	bool save_backup(std::string filename);

	u32 get_prescalar_1(u8 val);
	u32 get_prescalar_2(u8 val);
	double get_timer3_freq();
	void update_prescalar(u8 index);

	void update_irq_flags(u32 irq);
	void process_eeprom();
	void process_sed1565();

	//Network stuff for IR comms
	bool init_ir();
	void disconnect_ir();
	void process_network_communication();
	bool process_ir();
	void process_remote_signal();
	bool recv_byte();
	bool request_sync();
	bool stop_sync();

	u32 master_irq_flags;
	u8 irq_priority[32];
	bool irq_enable[32];
	u16 irq_vectors[32];

	bool osc_1_enable;
	bool osc_2_enable;

	bool save_eeprom;

	u32 rtc;
	u32 rtc_cycles;
	bool enable_rtc;

	struct eeprom_save
	{
		u8 data[0x2000];
		u8 clk;
		u8 last_clk;
		u8 sda;
		u8 last_sda;
		u8 state;
		u16 addr;
		u8 control;
		u8 addr_bit;
		u8 data_byte;

		bool clock_data;
		bool read_mode;
		bool start_cond;
		bool stop_cond;
	} eeprom;

	struct sed1565_controller
	{
		u8 cmd;
		u8 data;
		u8 lcd_x;
		u8 lcd_y;
		bool run_cmd;
		min_lcd_commands current_cmd;
	} sed;

	struct ir_status
	{
		u8 network_id;
		u8 temp_id;
		u8 signal;
		s16 fade;

		bool connected[11];
		bool sync;
		bool init;
		bool send_signal;

		u32 sync_counter;
		u32 sync_clock;
		u32 debug_cycles;
		s32 sync_timeout;
		s32 sync_balance;

		u32 remote_signal_cycles[128];
		u32 remote_signal_index;
		u32 remote_signal_size;
		s32 remote_signal_delay;
	} ir_stat;

	#ifdef GBE_NETPLAY

	//Receiving server
	struct tcp_server
	{
		TCPsocket host_socket, remote_socket;
		IPaddress host_ip;
		bool connected;
		bool host_init;
		bool remote_init;
		u16 port;
	} server[10];

	//Sending client
	struct tcp_sender
	{
		TCPsocket host_socket;
		IPaddress host_ip;
		bool connected;
		bool host_init;
		u16 port;
	} sender[10];

	SDLNet_SocketSet tcp_sockets[10];

	#endif

	//Advanced debugging
	#ifdef GBE_DEBUG
	bool debug_write;
	bool debug_read;
	u16 debug_addr;
	#endif

	void set_lcd_data(min_lcd_data* ex_lcd_stat);
	void set_apu_data(min_apu_data* ex_apu_stat);

	MIN_GamePad* g_pad;
	std::vector<min_timer>* timer;

	MIN_MMU();
	~MIN_MMU();

	void reset();

	//Serialize data for save state loading/saving
	bool mmu_read(u32 offset, std::string filename);
	bool mmu_write(std::string filename);
	u32 size();

	private:

	//Only the MMU and LCD should communicate through this structure
	min_lcd_data* lcd_stat;

	//Only the MMU and APU should communicate through this structure
	min_apu_data* apu_stat;
};

#endif // PM_MMU

