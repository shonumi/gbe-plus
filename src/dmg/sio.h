// GB Enhanced Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : sio.h
// Date : April 30, 2016
// Description : Game Boy Serial Input-Output emulation
//
// Sets up SDL networking
// Emulates Gameboy-to-Gameboy data transfers

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

	#ifdef GBE_NETPLAY

	//Receiving server
	struct tcp_server
	{
		TCPsocket host_socket, remote_socket;
		IPaddress host_ip;
		bool connected;
		u16 port;
	} server;

	//Sending client
	struct tcp_sender
	{
		TCPsocket host_socket;
		IPaddress host_ip;
		bool connected;
		u16 port;
	} sender;

	SDLNet_SocketSet tcp_sockets;

	#endif

	DMG_SIO();
	~DMG_SIO();

	bool init();
	void reset();

	bool send_byte();
	bool send_ir_signal();
	bool receive_byte();
	bool request_sync();
	void process_network_communication();
};

#endif // GB_SIO
