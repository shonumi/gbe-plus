// GB Enhanced Copyright Daniel Baxter 2018
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : sio.cpp
// Date : August 14, 2018
// Description : Game Boy Advance Serial Input-Output emulation
//
// Sets up SDL networking
// Emulates GBA-to-GBA data transfers

#ifndef GBA_SIO
#define GBA_SIO

#ifdef GBE_NETPLAY
#include <SDL2/SDL_net.h>
#endif

#include "mmu.h"
#include "sio_data.h" 

class AGB_SIO
{
	public:

	//Link to memory map
	AGB_MMU* mem;

	agb_sio_data sio_stat;

	bool network_init;
	bool is_master;
	u8 master_id;
	u8 max_clients;

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
	SDLNet_SocketSet four_player_tcp_sockets;

	#endif

	//GB Player Rumble
	struct gb_player_rumble
	{
		std::vector <u32> sio_buffer;
		u8 buffer_index;
		gb_player_rumble_state current_state;
	} player_rumble;

	//Soul Doll Adapter
	struct soul_doll_adapter
	{
		std::vector <u8> data;
		std::vector <u8> stream_byte;
		std::vector <u32> stream_word;
		u32 stop_signal;
		u16 eeprom_addr;
		u16 prev_data;
		u16 prev_write;
		u8 data_count;
		u8 slave_addr;
		u8 word_addr;
		u8 eeprom_cmd;
		u8 flags;
		bool get_slave_addr;
		bool update_soul_doll;
		soul_doll_adapter_state current_state;
	} sda;

	//Battle Chip Gate
	struct battle_chip_gate
	{
		u16 data;
		u16 id;
		u16 unit_code;
		u8 data_inc;
		u8 data_dec;
		u32 data_count;
		u32 net_gate_count;
		bool start;
		battle_chip_gate_state current_state;
	} chip_gate;

	//Mobile Adapter GB
	struct mobile_adapter_gb
	{
		std::vector <u8> data;
		std::vector <u8> packet_buffer;
		std::vector <u8> net_data;
		std::vector <std::string> srv_list_in;
		std::vector <std::string> srv_list_out;
		u32 packet_size;	
		agb_mobile_state current_state;
		std::string http_data;

		u8 command;
		u16 checksum;
		u8 data_length;
		u16 data_index;

		u16 port;
		u32 ip_addr;
		u32 transfer_state;
		bool pop_session_started;
		bool http_session_started;
		bool smtp_session_started;
		bool line_busy;
	} mobile_adapter;

	AGB_SIO();
	~AGB_SIO();

	bool init();
	void reset();

	bool send_data();
	bool receive_byte();
	bool request_sync();
	void process_network_communication();

	void gba_player_rumble_process();

	bool soul_doll_adapter_load_data(std::string filename);
	bool soul_doll_adapter_save_data();
	void soul_doll_adapter_reset();
	void soul_doll_adapter_process();

	void battle_chip_gate_process();
	void net_gate_process();

	void mobile_adapter_process();
	void mobile_adapter_process_pop();
	void mobile_adapter_process_http();
	void mobile_adapter_process_smtp();
	bool mobile_adapter_load_server_list();
};

#endif // GBA_SIO
