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

	//Network stuff for IR comms
	bool init_ir();
	void disconnect_ir();
	void process_network_communication();
	bool process_ir();
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


	struct eeprom_save
	{
		std::vector <u8> data;
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

	struct ir_status
	{
		u8 network_id;
		u8 signal;
		s16 fade;

		bool connected;
		bool sync;
		bool init;
		bool send_signal;

		u32 sync_counter;
		u32 sync_clock;
		u32 debug_cycles;
		s32 sync_timeout;
		s32 sync_balance;
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
	} server;

	//Sending client
	struct tcp_sender
	{
		TCPsocket host_socket;
		IPaddress host_ip;
		bool connected;
		bool host_init;
		u16 port;
	} sender;

	SDLNet_SocketSet tcp_sockets;

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

	private:

	//Only the MMU and LCD should communicate through this structure
	min_lcd_data* lcd_stat;

	//Only the MMU and APU should communicate through this structure
	min_apu_data* apu_stat;
};

#endif // PM_MMU

