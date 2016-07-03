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
	if(sio_stat.connected)
	{
		//Kill remote socket
		if((config::is_host) && (remote_socket != NULL))
		{
			SDLNet_TCP_DelSocket(tcp_sockets, remote_socket);
			SDLNet_TCP_Close(remote_socket);
		}

		//Kill host socket
		if(host_socket != NULL)
		{
			SDLNet_TCP_DelSocket(tcp_sockets, host_socket);
			SDLNet_TCP_Close(host_socket);
		}
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

	//Setup socket set
	tcp_sockets = SDLNet_AllocSocketSet(2);

	//Resolve the host - Host version
	if(config::is_host)
	{
		//When this instance of GBE+ is the host, listen on the port (hostname parameter is set to NULL)
		if(SDLNet_ResolveHost(&host_ip, NULL, config::netplay_port) < 0)
		{
			std::cout<<"SIO::Error - Host could not resolve hostname\n";
			return false;
		}
	}

	//Resolve the host - Client version
	else
	{
		//Listen to host on the specified port
		//Not sure if hostname string has to be valid or just any random string
		if(SDLNet_ResolveHost(&host_ip, "darkstar", config::netplay_port) < 0)
		{
			std::cout<<"SIO::Error - Client could not resolve hostname\n";
			return false;
		}
	}

	//Open a connection to listen on host's port
	if(!(host_socket = SDLNet_TCP_Open(&host_ip)))
	{
		std::cout<<"SIO::Error - Could not open a connection on Port " << config::netplay_port << "\n";
		return false;
	}

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
	sio_stat.transfer_byte = 0;
	sio_stat.sio_type = NO_GB_DEVICE;

	#ifdef GBE_NETPLAY

	host_socket = NULL;
	remote_socket = NULL;

	#endif

	tcp_accepted = false;
	comms_mode = (config::is_host) ? 1 : 0;
}

/****** Tranfers one byte to another system ******/
bool DMG_SIO::send_byte()
{
	#ifdef GBE_NETPLAY

	u8 temp_buffer[1];
	temp_buffer[0] = sio_stat.transfer_byte;

	//Host - Send transfer byte
	if(config::is_host)
	{
		if(SDLNet_TCP_Send(remote_socket, (void*)temp_buffer, 1) < 1)
		{
			std::cout<<"SIO::Error - Host failed to send data to client\n";
			return false;
		}

		//Raise SIO IRQ after sending byte
		mem->memory_map[IF_FLAG] |= 0x08;

		//Flip comms mode
		comms_mode = 0;

		std::cout<<"Sending byte 0x" << std::hex << (u32)sio_stat.transfer_byte << "\n";
	}

	//Client - Send transfer byte
	else
	{
		if(SDLNet_TCP_Send(host_socket, (void*)temp_buffer, 1) < 1)
		{
			std::cout<<"SIO::Error - Client failed to send data to host\n";
			return false;
		}

		//Raise SIO IRQ after sending byte
		mem->memory_map[IF_FLAG] |= 0x08;

		//Flip comms mode
		comms_mode = 0;

		std::cout<<"Sending byte 0x" << std::hex << (u32)sio_stat.transfer_byte << "\n";
	}

	#endif

	return true;
}

/****** Receives one byte from another system ******/
bool DMG_SIO::receive_byte()
{
	#ifdef GBE_NETPLAY

	//Check sockets so SDLNet_TCP_Recv() can be called non-blocking
	SDLNet_CheckSockets(tcp_sockets, 0);

	u8 temp_buffer[1];

	//Host - Receive transfer byte
	if((config::is_host) && (SDLNet_SocketReady(remote_socket)))
	{
		if(SDLNet_TCP_Recv(remote_socket, temp_buffer, 1) > 0)
		{
			//Raise SIO IRQ after receiving byte
			mem->memory_map[IF_FLAG] |= 0x08;

			//Store byte from transfer into SB
			mem->memory_map[REG_SB] = sio_stat.transfer_byte = temp_buffer[0];

			//Flip comms mode
			comms_mode = 1;

			std::cout<<"Receiving byte 0x" << std::hex << (u32)sio_stat.transfer_byte << "\n";

			return true;
		}
	}

	//Client - Receive transfer byte
	else if((!config::is_host) && (SDLNet_SocketReady(host_socket)))
	{
		if(SDLNet_TCP_Recv(host_socket, temp_buffer, 1) > 0)
		{
			//Raise SIO IRQ after receiving byte
			mem->memory_map[IF_FLAG] |= 0x08;

			//Store byte from transfer into SB
			mem->memory_map[REG_SB] = sio_stat.transfer_byte = temp_buffer[0];

			//Flip comms mode
			comms_mode = 1;

			std::cout<<"Receiving byte 0x" << std::hex << (u32)sio_stat.transfer_byte << "\n";

			return true;
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
	if(!tcp_accepted)
	{
		//Host - Accept a pending connection on the remote socket
		if((config::is_host) && (remote_socket = SDLNet_TCP_Accept(host_socket)))
		{
			tcp_accepted = true;
			SDLNet_TCP_AddSocket(tcp_sockets, host_socket);
			SDLNet_TCP_AddSocket(tcp_sockets, remote_socket);
		}

		//Client - Create a connection 
		else if((!config::is_host) && (host_socket = SDLNet_TCP_Open(&host_ip)))
		{
			tcp_accepted = true;
			SDLNet_TCP_AddSocket(tcp_sockets, host_socket);
		}
	}

	#endif
}
