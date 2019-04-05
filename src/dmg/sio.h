// GB Enhanced Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : sio.h
// Date : April 30, 2016
// Description : Game Boy Serial Input-Output emulation
//
// Sets up SDL networking
// Emulates Gameboy-to-Gameboy data transfers
// Emulates GBC IR port (not exactly SIO, but the RP register is related to pin 4 of the link port)
// Emulates the GB Printer

#include <queue>

#ifndef GB_SIO
#define GB_SIO

#ifdef GBE_NETPLAY
#include <SDL2/SDL_net.h>
#endif

#include "mmu.h"
#include "sio_data.h"

class DMG_SIO
{
	public:

	//Link to memory map
	DMG_MMU* mem;

	dmg_sio_data sio_stat;

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

	//Receiving server (4-Player)
	struct four_player_tcp_server
	{
		TCPsocket host_socket, remote_socket;
		IPaddress host_ip;
		bool connected;
		u16 port;
	} four_player_server[3];

	//Sending client (4-Player)
	struct four_player_tcp_sender
	{
		TCPsocket host_socket;
		IPaddress host_ip;
		bool connected;
		u16 port;
	} four_player_sender[3];

	SDLNet_SocketSet tcp_sockets;
	SDLNet_SocketSet four_player_tcp_sockets;

	#endif

	//GB Printer
	struct gb_printer
	{
		std::vector <u32> scanline_buffer;
		std::vector <u8> packet_buffer;
		u32 packet_size;
		printer_state current_state;

		u8 command;
		u8 compression_flag;
		u8 strip_count;
		u16 data_length;
		u16 checksum;
		u8 status;
		u8 pal[4];
	} printer;

	//GB Mobile Adapter
	struct gb_mobile_adapter
	{
		std::vector <u8> data;
		std::vector <u8> packet_buffer;
		std::vector <u8> net_data;
		std::vector <std::string> srv_list_in;
		std::vector <std::string> srv_list_out;
		u32 packet_size;	
		mobile_state current_state;
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

	//Bardigun barcode scanner
	struct gb_bardigun_barcode_scanner
	{
		std::vector <u8> data;
		bardigun_state current_state;
		u16 inactive_counter;
		u32 barcode_pointer;
	} bardigun_scanner;

	//Barcode Boy
	struct gb_barcode_boy_scanner
	{
		std::vector <u8> data;
		u16 counter;
		barcode_boy_state current_state;
		u8 byte;
		bool send_data;
	} barcode_boy;

	//Full Changer
	struct gb_full_changer
	{
		std::vector<u16> data;
		u32 delay_counter;
		u8 current_character;
		bool light_on;
		full_changer_state current_state;
	} full_changer;

	//TV Remote
	struct gb_tv_remote
	{
		std::vector<u16> data;
		u32 delay_counter;
		u8 current_data;
		bool light_on;
		tv_remote_state current_state;
	} tv_remote;

	//Pocket IR device - Pokemon Pikachu 2 and Pocket Sakura
	struct gb_pocket_ir
	{
		std::vector<u16> data;
		u32 current_data;
		u32 db_step;
		bool light_on;
		pocket_ir_state current_state;
	} pocket_ir;
	
	//4 Player Adapter
	struct gb_four_player_adapter
	{
		u8 id;
		u8 status;
		std::vector<u8> buffer;
		u8 incoming[4];
		u8 wait_flags;
		u8 buffer_pos;
		u8 packet_size;
		u32 clock;
		bool begin_network_sync;
		bool restart_network;
		four_player_state current_state;
	} four_player;

	//Power Antenna
	bool power_antenna_on;

	bool dmg07_init;

	DMG_SIO();
	~DMG_SIO();

	bool init();
	void reset();

	bool send_byte();
	bool send_ir_signal();
	bool receive_byte();
	bool request_sync();
	void process_network_communication();

	void printer_process();
	void printer_execute_command();
	void printer_data_process();
	void print_image();

	void mobile_adapter_process();
	void mobile_adapter_process_pop();
	void mobile_adapter_process_http();
	void mobile_adapter_process_smtp();
	bool mobile_adapter_load_server_list();

	void bardigun_process();
	bool bardigun_load_barcode(std::string filename);

	void barcode_boy_process();
	bool barcode_boy_load_barcode(std::string filename);

	void full_changer_process();
	bool full_changer_load_db(std::string filename);

	void tv_remote_process();

	void pocket_ir_process();
	bool pocket_ir_load_db(std::string filename);

	bool four_player_init();
	void four_player_disconnect();
	void four_player_process_network_communication();
	bool four_player_receive_byte();
	bool four_player_request_sync();
	bool four_player_data_status();
	void four_player_broadcast(u8 data_one, u8 data_two);
	u8 four_player_request(u8 data_one, u8 data_two, u8 id);

	void four_player_process();
	void four_player_update_status(u8 status);
};

#endif // GB_SIO
