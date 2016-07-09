// GB Enhanced Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : sio.cpp
// Date : April 30, 2016
// Description : Game Boy Serial Input-Output emulation
//
// Sets up SDL networking
// Emulates Gameboy-to-Gameboy data transfers

#include "sio.h"

/****** SIO Constructor ******/
DMG_SIO::DMG_SIO()
{
	reset();
}

/****** SIO Destructor ******/
DMG_SIO::~DMG_SIO()
{
	#ifdef GBE_NETPLAY

	//Close SDL_net and any current connections
	if(server.host_socket != NULL)
	{
		SDLNet_TCP_DelSocket(tcp_sockets, server.host_socket);
		SDLNet_TCP_Close(server.host_socket);
	}

	if(server.remote_socket != NULL)
	{
		SDLNet_TCP_DelSocket(tcp_sockets, server.remote_socket);
		SDLNet_TCP_Close(server.remote_socket);
	}

	if(sender.host_socket != NULL)
	{
		SDLNet_TCP_DelSocket(tcp_sockets, sender.host_socket);
		SDLNet_TCP_Close(sender.host_socket);
	}

	SDLNet_Quit();

	#endif

	std::cout<<"SIO::Shutdown\n";
}

/****** Initialize SIO ******/
bool DMG_SIO::init()
{
	#ifdef GBE_NETPLAY

	//Do not set up SDL_net if netplay is not enabled
	if(!config::use_netplay)
	{
		std::cout<<"SIO::Initialized\n";
		return false;
	}

	//Setup SDL_net
	if(SDLNet_Init() < 0)
	{
		std::cout<<"SIO::Error - Could not initialize SDL_net\n";
		return false;
	}

	//Server info
	server.host_socket = NULL;
	server.remote_socket = NULL;
	server.connected = false;
	server.port = config::netplay_server_port;

	//Client info
	sender.host_socket = NULL;
	sender.connected = false;
	sender.port = config::netplay_client_port;

	//Setup server, resolve the server with NULL as the hostname, the server will now listen for connections
	if(SDLNet_ResolveHost(&server.host_ip, NULL, server.port) < 0)
	{
		std::cout<<"SIO::Error - Server could not resolve hostname\n";
		return -1;
	}

	//Open a connection to listen on host's port
	if(!(server.host_socket = SDLNet_TCP_Open(&server.host_ip)))
	{
		std::cout<<"SIO::Error - Server could not open a connection on Port " << server.port << "\n";
		return -1;
	}

	//Setup client, listen on another port
	if(SDLNet_ResolveHost(&sender.host_ip, "darkstar", sender.port) < 0)
	{
		std::cout<<"SIO::Error - Client could not resolve hostname\n";
		return -1;
	}

	//Create sockets sets
	tcp_sockets = SDLNet_AllocSocketSet(3);

	#endif

	std::cout<<"SIO::Initialized\n";
	return true;
}

/****** Reset SIO ******/
void DMG_SIO::reset()
{
	sio_stat.connected = false;
	sio_stat.active_transfer = false;
	sio_stat.double_speed = false;
	sio_stat.internal_clock = false;
	sio_stat.shifts_left = 0;
	sio_stat.shift_counter = 0;
	sio_stat.shift_clock = 512;
	sio_stat.transfer_byte = 0;
	sio_stat.sio_type = NO_GB_DEVICE;

	#ifdef GBE_NETPLAY

	//Server info
	server.host_socket = NULL;
	server.remote_socket = NULL;
	server.connected = false;
	server.port = config::netplay_server_port;

	//Client info
	sender.host_socket = NULL;
	sender.connected = false;
	sender.port = config::netplay_client_port;

	#endif
}

/****** Tranfers one byte to another system ******/
bool DMG_SIO::send_byte()
{
	#ifdef GBE_NETPLAY

	u8 temp_buffer[1];
	temp_buffer[0] = sio_stat.transfer_byte;

	if(SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 1) < 1)
	{
		std::cout<<"SIO::Error - Host failed to send data to client\n";
		return false;
	}

	//std::cout<<"Sending byte 0x" << std::hex << (u32)sio_stat.transfer_byte << "\n";

	//Wait for other Game Boy to send this one its SB
	//This is blocking, will effectively pause GBE+ until it gets something
	if(SDLNet_TCP_Recv(server.remote_socket, temp_buffer, 1) > 0)
	{
		mem->memory_map[REG_SB] = sio_stat.transfer_byte = temp_buffer[0];
	}

	//std::cout<<"Receiving echo byte 0x" << std::hex << (u32)sio_stat.transfer_byte << "\n";

	//Raise SIO IRQ after sending byte
	mem->memory_map[IF_FLAG] |= 0x08;

	#endif

	return true;
}

/****** Receives one byte from another system ******/
bool DMG_SIO::receive_byte()
{
	#ifdef GBE_NETPLAY

	u8 temp_buffer[1];
	temp_buffer[0] = 0;

	//Check the status of connection
	SDLNet_CheckSockets(tcp_sockets, 0);

	//If this socket is active, receive the transfer
	if(SDLNet_SocketReady(server.remote_socket))
	{
		if(SDLNet_TCP_Recv(server.remote_socket, temp_buffer, 1) > 0)
		{
			//Raise SIO IRQ after sending byte
			mem->memory_map[IF_FLAG] |= 0x08;

			//Store byte from transfer into SB
			sio_stat.transfer_byte = mem->memory_map[REG_SB];
			mem->memory_map[REG_SB] = temp_buffer[0];

			//Reset Bit 7 of SC
			mem->memory_map[REG_SC] &= ~0x80;

			//std::cout<<"Receiving byte 0x" << std::hex << (u32)mem->memory_map[REG_SB] << "\n";

			//Send other Game Boy the old SB value
			temp_buffer[0] = sio_stat.transfer_byte;
			sio_stat.transfer_byte = mem->memory_map[REG_SB];
			SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 1);

			//std::cout<<"Sending echo byte 0x" << std::hex << (u32)temp_buffer[0] << "\n";
		}
	}

	#endif

	return true;
}

/****** Manages network communication via SDL_net ******/
void DMG_SIO::process_network_communication()
{
	#ifdef GBE_NETPLAY

	//If no communication with another GBE+ instance has been established yet, see if a connection can be made
	if(!sio_stat.connected)
	{
		//Try to accept incoming connections to the server
		if(!server.connected)
		{
			if(server.remote_socket = SDLNet_TCP_Accept(server.host_socket))
			{
				std::cout<<"SIO::Client connected\n";
				SDLNet_TCP_AddSocket(tcp_sockets, server.host_socket);
				SDLNet_TCP_AddSocket(tcp_sockets, server.remote_socket);
				server.connected = true;
			}
		}

		//Try to establish an outgoing connection to the server
		if(!sender.connected)
		{
			//Open a connection to listen on host's port
			if(sender.host_socket = SDLNet_TCP_Open(&sender.host_ip))
			{
				std::cout<<"SIO::Connected to server\n";
				SDLNet_TCP_AddSocket(tcp_sockets, sender.host_socket);
				sender.connected = true;
			}
		}

		if((server.connected) && (sender.connected)) { sio_stat.connected = true; }
	}

	#endif
}
