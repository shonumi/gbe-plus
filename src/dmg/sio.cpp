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
		if((config::is_host) && (remote_socket != NULL)) { SDLNet_TCP_Close(remote_socket); }

		//Kill host socket
		if(host_socket != NULL) { SDLNet_TCP_Close(host_socket); }
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
}

/****** Tranfers one byte to another system ******/
bool DMG_SIO::send_byte() { return true; }

/****** Receives one byte from another system ******/
bool DMG_SIO::receive_byte() { return true; }
