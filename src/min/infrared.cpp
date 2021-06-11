// GB Enhanced Copyright Daniel Baxter 2021
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : infrared.cpp
// Date : April 19, 2021
// Description : Pokemon Mini IR emulation
//
// Transfers IR data between multiple instances of GBE+
// Responsible for network communications

#include "mmu.h"

/****** Initializes netplay for IR communications ******/
bool MIN_MMU::init_ir()
{
	ir_stat.init = false;
	ir_stat.network_id = 0;
	ir_stat.signal = 0;
	ir_stat.fade = 0;
	ir_stat.sync = false;
	ir_stat.send_signal = false;
	ir_stat.static_mode = true;
	ir_stat.sync_counter = 0;
	ir_stat.sync_clock = 0;
	ir_stat.sync_timeout = 0;
	ir_stat.sync_balance = 0;
	ir_stat.debug_cycles = 0xDEADBEEF;

	for(u32 x = 0; x < 6; x++) { ir_stat.connected[x] = false; }

	#ifdef GBE_NETPLAY

	//Do not set up SDL_net if netplay is not enabled
	if(!config::use_netplay)
	{
		return false;
	}
	
	//Setup SDL_net
	if(SDLNet_Init() < 0)
	{
		std::cout<<"IR::Error - Could not initialize SDL_net\n";
		return false;
	}

	ir_stat.init = true;
	ir_stat.sync_clock = config::netplay_sync_threshold;
	ir_stat.network_id = config::netplay_id;

	//Server info
	for(u32 x = 0; x < 6; x++)
	{
		server[x].host_socket = NULL;
		server[x].host_init = false;
		server[x].remote_socket = NULL;
		server[x].remote_init = false;
		server[x].connected = false;
		server[x].port = config::netplay_server_port + (10 * config::netplay_id) + x;
			
		//Client info
		sender[x].host_socket = NULL;
		sender[x].host_init = false;
		sender[x].connected = false;
		sender[x].port = config::netplay_server_port + (10 * x) + config::netplay_id;

		if(x != config::netplay_id)
		{
			//Setup server, resolve the server with NULL as the hostname, the server will now listen for connections
			if(SDLNet_ResolveHost(&server[x].host_ip, NULL, server[x].port) < 0)
			{
				std::cout<<"IR::Error - Server could not resolve hostname\n";
				return false;
			}

			//Open a connection to listen on host's port
			if(!(server[x].host_socket = SDLNet_TCP_Open(&server[x].host_ip)))
			{
				std::cout<<"IR::Error - Server could not open a connection on Port " << server[x].port << "\n";
				return false;
			}

			server[x].host_init = true;

			//Setup client, listen on another port
			if(SDLNet_ResolveHost(&sender[x].host_ip, config::netplay_client_ip.c_str(), sender[x].port) < 0)
			{
				std::cout<<"IR::Error - Client could not resolve hostname\n";
				return false;
			}

			//Create sockets sets
			tcp_sockets[x] = SDLNet_AllocSocketSet(3);
		}
	}

	//Initialize hard syncing
	if(config::netplay_hard_sync)
	{
		//The instance with the highest server port will start off waiting in sync mode
		ir_stat.sync_counter = (config::netplay_server_port > config::netplay_client_port) ? config::netplay_sync_threshold : 0;
		ir_stat.sync_balance = (config::netplay_server_port > config::netplay_client_port) ? 4 : 0;
	}

	#endif

	std::cout<<"IR::Initialized\n";
	return true;
}

/****** Disconnects netplay for IR communications ******/
void MIN_MMU::disconnect_ir()
{
	if(!ir_stat.init)
	{
		std::cout<<"IR::Shutdown\n";
		return;
	}

	#ifdef GBE_NETPLAY

	for(u8 x = 0; x < 6; x++)
	{

		if(x == config::netplay_id) { continue; }


		//Close SDL_net and any current connections
		if(server[x].host_socket != NULL)
		{
			SDLNet_TCP_DelSocket(tcp_sockets[x], server[x].host_socket);
			if(server[x].host_init) { SDLNet_TCP_Close(server[x].host_socket); }
		}

		if(server[x].remote_socket != NULL)
		{
			SDLNet_TCP_DelSocket(tcp_sockets[x], server[x].remote_socket);
			if(server[x].remote_init) { SDLNet_TCP_Close(server[x].remote_socket); }
		}

		if(sender[x].host_socket != NULL)
		{
			//Send disconnect byte to another system
			u8 temp_buffer[2];
			temp_buffer[0] = 0;
			temp_buffer[1] = 0x80;
		
			SDLNet_TCP_Send(sender[x].host_socket, (void*)temp_buffer, 2);

			SDLNet_TCP_DelSocket(tcp_sockets[x], sender[x].host_socket);
			if(sender[x].host_init) { SDLNet_TCP_Close(sender[x].host_socket); }
		}

		server[x].connected = false;
		sender[x].connected = false;

		server[x].host_init = false;
		server[x].remote_init = false;
		sender[x].host_init = false;

		ir_stat.connected[x] = false;
	}

	SDLNet_Quit();

	#endif

	ir_stat.sync_timeout = 0;
	ir_stat.sync = false;

	std::cout<<"IR::Shutdown\n";
}

/****** Sets up netplay for IR communications ******/
void MIN_MMU::process_network_communication()
{
	u8 id = ir_stat.network_id;

	if(!ir_stat.init) { return; }
	if(id == config::netplay_id) { return; }

	#ifdef GBE_NETPLAY

	//If no communication with another GBE+ instance has been established yet, see if a connection can be made
	if(!ir_stat.connected[id])
	{
		//Try to accept incoming connections to the server
		if(!server[id].connected)
		{
			if(server[id].remote_socket = SDLNet_TCP_Accept(server[id].host_socket))
			{
				std::cout<<"IR::Client connected\n";
				SDLNet_TCP_AddSocket(tcp_sockets[id], server[id].host_socket);
				SDLNet_TCP_AddSocket(tcp_sockets[id], server[id].remote_socket);
				server[id].connected = true;
				server[id].remote_init = true;
			}
		}

		//Try to establish an outgoing connection to the server
		if(!sender[id].connected)
		{
			//Open a connection to listen on host's port
			if(sender[id].host_socket = SDLNet_TCP_Open(&sender[id].host_ip))
			{
				std::cout<<"IR::Connected to server\n";
				SDLNet_TCP_AddSocket(tcp_sockets[id], sender[id].host_socket);
				sender[id].connected = true;
				sender[id].host_init = true;
			}
		}

		if((server[id].connected) && (sender[id].connected))
		{
			ir_stat.connected[id] = true;
		}
	}

	#endif
}

/****** Sends IR data over a network ******/
bool MIN_MMU::process_ir()
{
	u8 id = ir_stat.network_id;

	if(!ir_stat.init || !ir_stat.connected[id]) { return true; }
	if(memory_map[PM_IO_DATA] & 0x20) { return true; }
	if(id == config::netplay_id) { return true; }

	#ifdef GBE_NETPLAY

	u8 temp_buffer[2];

	//For IR signals, flag it properly
	//1st byte is IR signal data, second byte GBE+'s marker, 0x40
	temp_buffer[0] = (memory_map[PM_IO_DATA] & 0x1) ? 1 : 0;
	temp_buffer[1] = 0x40;

	ir_stat.signal = 0;
	ir_stat.debug_cycles = 0;

	if(SDLNet_TCP_Send(sender[id].host_socket, (void*)temp_buffer, 2) < 2)
	{
		std::cout<<"IR::Error - Host failed to send data to client\n";
		ir_stat.connected[id] = false;
		server[id].connected = false;
		sender[id].connected = false;
		return false;
	}

	//Wait for other instance of GBE+ to send an acknowledgement
	//This is blocking, will effectively pause GBE+ until it gets something
	if(SDLNet_TCP_Recv(server[id].remote_socket, temp_buffer, 2) > 0)
	{
		//Reset hard sync timeout at 1/4 emulated second
		ir_stat.sync_timeout = 524288;
		ir_stat.send_signal = true;

		return true;
	}

	#endif

	return true;
}

/****** Receive IR data over a network ******/
bool MIN_MMU::recv_byte()
{
	u8 id = ir_stat.network_id;

	if(!ir_stat.init || !ir_stat.connected[id]) { return true; }
	if(id == config::netplay_id) { return true; }

	#ifdef GBE_NETPLAY

	u8 temp_buffer[2];
	temp_buffer[0] = temp_buffer[1] = 0;

	//Check the status of connection
	SDLNet_CheckSockets(tcp_sockets[id], 0);

	//If this socket is active, receive the transfer
	if(SDLNet_SocketReady(server[id].remote_socket))
	{
		if(SDLNet_TCP_Recv(server[id].remote_socket, temp_buffer, 2) > 0)
		{
			//Stop sync
			if((temp_buffer[1] == 0xFF) && (ir_stat.sync))
			{
				ir_stat.sync = false;
				ir_stat.sync_counter = 0;
				ir_stat.sync_balance = temp_buffer[0];
				return true;
			}

			//Stop IR Hard Sync
			else if(temp_buffer[1] == 0xF1)
			{
				ir_stat.sync_timeout = 0;
				ir_stat.sync = false;
				if(!ir_stat.static_mode) { ir_stat.network_id = config::netplay_id; }
				return true;
			}

			//Stop sync with acknowledgement
			else if(temp_buffer[1] == 0xF0)
			{
				ir_stat.sync = false;
				ir_stat.sync_counter = 0;

				temp_buffer[1] = 0x1;

				//Send acknowlegdement
				SDLNet_TCP_Send(sender[id].host_socket, (void*)temp_buffer, 2);

				return true;
			}

			//Disconnect netplay
			else if(temp_buffer[1] == 0x80)
			{
				std::cout<<"IR::Netplay connection terminated. Restart to reconnect.\n";
				ir_stat.connected[id] = false;
				ir_stat.sync = false;

				return true;
			}

			//Receive IR signal
			else if(temp_buffer[1] == 0x40)
			{	
				u8 last_signal = ir_stat.signal;

				//Only receive data if IO port has IR enabled
				if((memory_map[PM_IO_DATA] & 0x20) == 0)
				{
					//Set Bit 0 of IO Port according to result
					if(temp_buffer[0] == 1)
					{
						memory_map[PM_IO_DATA] |= 0x2;
						ir_stat.signal = 0;
					}

					else
					{
						memory_map[PM_IO_DATA] &= ~0x2;
						ir_stat.signal = 1;
						ir_stat.fade = 64;
					}

					//Reset hard sync timeout at 1/4 emulated second
					ir_stat.sync_timeout = 524288;
				}

				//Raise IR IRQ when going from LOW to HIGH
				if((last_signal == 0) && (ir_stat.signal == 1)) { update_irq_flags(IR_RECEIVER_IRQ); }

				//Send acknowlegdement
				SDLNet_TCP_Send(sender[id].host_socket, (void*)temp_buffer, 2);

				return true;
			}
		}
	}

	#endif

	return true;
}

/****** Request sync with another instance of GBE+ over a network ******/
bool MIN_MMU::request_sync()
{
	u8 id = ir_stat.network_id;

	if(!ir_stat.init || !ir_stat.connected[id]) { return true; }
	if(id == config::netplay_id) { return true; }

	#ifdef GBE_NETPLAY

	s8 bal = ir_stat.sync_balance * -1;
	if(ir_stat.send_signal) { bal += 8; ir_stat.send_signal = false; }

	u8 temp_buffer[2];
	temp_buffer[0] = bal;
	temp_buffer[1] = 0xFF;

	//Send the sync code 0xFF
	if(SDLNet_TCP_Send(sender[id].host_socket, (void*)temp_buffer, 2) < 2)
	{
		std::cout<<"IR::Error - Host failed to send data to client\n";
		ir_stat.connected[id] = false;
		server[id].connected = false;
		sender[id].connected = false;
		return false;
	}

	ir_stat.sync = true;

	#endif

	return true;
}

/****** Instructs another instance of GBE+ to stop hard sync ******/
bool MIN_MMU::stop_sync()
{
	u8 id = ir_stat.network_id;

	if(!ir_stat.init || !ir_stat.connected[id]) { return true; }
	if(id == config::netplay_id) { return true; }

	#ifdef GBE_NETPLAY

	u8 temp_buffer[2];
	temp_buffer[0] = 0;
	temp_buffer[1] = 0xF1;

	//Send the stop hard sync code 0xF1
	if(SDLNet_TCP_Send(sender[id].host_socket, (void*)temp_buffer, 2) < 2)
	{
		std::cout<<"IR::Error - Host failed to send data to client\n";
		ir_stat.connected[id] = false;
		server[id].connected = false;
		sender[id].connected = false;
		return false;
	}

	ir_stat.sync = false;

	#endif

	return true;
}
