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

	#ifdef GBE_NETPLAY

	//SDL2_net sockets, IPs, and info
	TCPsocket host_socket, remote_socket;
	SDLNet_SocketSet tcp_sockets;
	IPaddress host_ip;
	IPaddress* remote_ip;

	#endif

	bool tcp_accepted;
	u8 comms_mode;

	DMG_SIO();
	~DMG_SIO();

	bool init();
	void reset();

	bool send_byte();
	bool receive_byte();
	void process_network_communication();
};

#endif // GB_SIO
