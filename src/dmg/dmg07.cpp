// GB Enhanced Copyright Daniel Baxter 2017
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : dmg07.cpp
// Date : December 09, 2017
// Description : DMG-07 emulation
//
// Handles 4-player networking
// Emulates the DMG-07 hardware and protocol

#include "sio.h"
#include "common/util.h"

/****** Initializes DMG-07 specific stuff ******/
bool DMG_SIO::four_player_init()
{
	#ifdef GBE_NETPLAY

	network_init = true;
	bool server_init = false;

	//Determine master-slave relationship to the network
	is_master = (config::netplay_server_port < config::netplay_client_port) ? true : false;

	for(int x = 0; x < 3; x++)
	{
		//Server info
		four_player_server[x].host_socket = NULL;
		four_player_server[x].remote_socket = NULL;
		four_player_server[x].connected = false;
		four_player_server[x].port = config::netplay_server_port + (x * 2);

		//Client info
		four_player_sender[x].host_socket = NULL;
		four_player_sender[x].connected = false;
		four_player_sender[x].port = config::netplay_client_port + (x * 2);

		//Setup server, resolve the server with NULL as the hostname, the server will now listen for connections
		if(SDLNet_ResolveHost(&four_player_server[x].host_ip, NULL, four_player_server[x].port) < 0)
		{
			std::cout<<"SIO::Error - Server could not resolve hostname\n";
			return false;
		}

		//Open a connection to listen on host's port
		if((!server_init) || (is_master))
		{
			if(!(four_player_server[x].host_socket = SDLNet_TCP_Open(&four_player_server[x].host_ip)))
			{
				if((is_master) || ((x == 2) && (!is_master)))
				{
					std::cout<<"SIO::Error - Server could not open a connection on Port " << four_player_server[x].port << "\n";
					return false;
				}
			}

			else { server_init = true; }
		}

		//Setup client, listen on another port
		if(SDLNet_ResolveHost(&four_player_sender[x].host_ip, config::netplay_client_ip.c_str(), four_player_sender[x].port) < 0)
		{
			std::cout<<"SIO::Error - Client could not resolve hostname\n";
			return false;
		}
	}

	//Create sockets sets
	four_player_tcp_sockets = SDLNet_AllocSocketSet(12);

	#endif

	return true;
}

/****** Disengages all network communication for the DMG-07 ******/
void DMG_SIO::four_player_disconnect()
{
	#ifdef GBE_NETPLAY

	for(int x = 0; x < 3; x++)
	{
		//Close SDL_net and any current connections
		if(four_player_server[x].host_socket != NULL)
		{
			SDLNet_TCP_DelSocket(four_player_tcp_sockets, four_player_server[x].host_socket);
			SDLNet_TCP_Close(four_player_server[x].host_socket);
		}

		if(four_player_server[x].remote_socket != NULL)
		{
			SDLNet_TCP_DelSocket(four_player_tcp_sockets, four_player_server[x].remote_socket);
			SDLNet_TCP_Close(four_player_server[x].remote_socket);
		}

		if(four_player_sender[x].host_socket != NULL)
		{
			//Update 4 Player status
			four_player.status &= ~0xF0;
			//four_player_send_byte(four_player.status, 0xFD);

			//Send disconnect byte to another system
			u8 temp_buffer[2];
			temp_buffer[0] = 0;
			temp_buffer[1] = 0x80;
		
			if(four_player_sender[master_id].host_socket != NULL)
			{
				SDLNet_TCP_Send(four_player_sender[master_id].host_socket, (void*)temp_buffer, 2);
			}

			SDLNet_TCP_DelSocket(four_player_tcp_sockets, four_player_sender[x].host_socket);
			SDLNet_TCP_Close(four_player_sender[x].host_socket);
		}
	}

	#endif
}

/****** Manages network communication via SDL_net for DMG-07 ******/
void DMG_SIO::four_player_process_network_communication()
{
	for(int x = 0; x < 3; x++)
	{
		//Try to accept incoming connections to the server
		if(!four_player_server[x].connected)
		{
			if(four_player_server[x].host_socket != NULL)
			{
				if(four_player_server[x].remote_socket = SDLNet_TCP_Accept(four_player_server[x].host_socket))
				{
					std::cout<<"SIO::Client #" << (x + 1) << " connected\n";
					SDLNet_TCP_AddSocket(four_player_tcp_sockets, four_player_server[x].host_socket);
					SDLNet_TCP_AddSocket(four_player_tcp_sockets, four_player_server[x].remote_socket);
					four_player_server[x].connected = true;
				}
			}
		}

		//Try to establish an outgoing connection to the server
		if(!four_player_sender[x].connected)
		{
			//Open a connection to listen on host's port
			if(four_player_sender[x].host_socket = SDLNet_TCP_Open(&four_player_sender[x].host_ip))
			{
				std::cout<<"SIO::Connected to server\n";
				SDLNet_TCP_AddSocket(four_player_tcp_sockets, four_player_sender[x].host_socket);
				four_player_sender[x].connected = true;
			}
		}

		//Finalize connections
		if((four_player_server[x].connected) && (four_player_sender[x].connected))
		{
			sio_stat.connected = true;

			//For 4 Player adapter, set ID based on port number
			four_player.id = (is_master) ? 1 : (x + 2);
			four_player.status = four_player.id;
				
			sio_stat.network_id = four_player.id;
			
			if(four_player.id == 1)
			{
				sio_stat.network_id |= 0x80;
				four_player.wait_flags = 1;

				if(sio_stat.send_data)
				{
					sio_stat.active_transfer = true;
					sio_stat.shifts_left = 8;
					sio_stat.shift_counter = 0;
					sio_stat.shift_clock = 2048;
				}
			}

			else
			{
				sio_stat.network_id |= 0x40;
				master_id = x;
				four_player.current_state = FOUR_PLAYER_PING;
				return;
			}
		}
	}

	max_clients = 0;

	for(int x = 0; x < 3; x++)
	{
		if((four_player_server[x].connected) && (four_player_sender[x].connected)) { max_clients++; }
	}
}

/****** Requests syncronization with another system ******/
bool DMG_SIO::four_player_request_sync()
{
	#ifdef GBE_NETPLAY

	//Process incoming data first
	four_player_receive_byte();

	bool all_connected = true;

	for(int x = 0; x < max_clients; x++)
	{
		u8 sync_stat = four_player_request(0, 0xF8, x);
		if(!sync_stat) { all_connected = false; }
	}

	if(all_connected)
	{
		four_player_broadcast(0, 0xF0);
		sio_stat.sync = false;
		sio_stat.sync_counter = 0;
	}

	#endif

	return true;
}

/****** Requests data status from another system ******/
bool DMG_SIO::four_player_data_status()
{
	#ifdef GBE_NETPLAY

	//Process syncronication first
	four_player_request_sync();

	bool data_ready = true;

	for(int x = 0; x < max_clients; x++)
	{
		u8 data_stat = four_player_request(0, 0xFE, x);
		if(!data_stat) { data_ready = false; }
	}

	if(data_ready)
	{
		four_player_broadcast(0, 0xF7);
		four_player.wait_flags = 0;
	} 

	#endif

	return true;
}

/****** Receives one byte from another system for the DMG-07 ******/
bool DMG_SIO::four_player_receive_byte()
{
	#ifdef GBE_NETPLAY

	u8 temp_buffer[1];
	temp_buffer[0] = temp_buffer[1] = 0;

	//Check the status of connection
	SDLNet_CheckSockets(four_player_tcp_sockets, 0);

	//If this socket is active, receive the transfer
	for(int x = 0; x < 3; x++)
	{
		if((four_player_server[x].remote_socket != NULL) && (SDLNet_SocketReady(four_player_server[x].remote_socket)))
		{
			if(SDLNet_TCP_Recv(four_player_server[x].remote_socket, temp_buffer, 2) > 0)
			{
				//4-Player - Confirm SB write for Players 2, 3, and 4
				if(temp_buffer[1] == 0xFE)
				{
					temp_buffer[0] = sio_stat.send_data ? 1 : 0;
					temp_buffer[1] = 0x1;

					//Send acknowlegdement
					SDLNet_TCP_Send(four_player_sender[x].host_socket, (void*)temp_buffer, 2);

					return true;
				}

				//4-Player - Receive status update from Player 1
				else if(temp_buffer[1] == 0xFD)
				{
					four_player_update_status(temp_buffer[0]);
	
					temp_buffer[0] = four_player.status;
					temp_buffer[1] = 0x1;

					//Send acknowlegdement
					SDLNet_TCP_Send(four_player_sender[x].host_socket, (void*)temp_buffer, 2);

					return true;
				}

				//4-Player - Receive and process serial out byte
				else if(temp_buffer[1] == 0xFC)
				{
					mem->memory_map[REG_SB] = temp_buffer[0];
					mem->memory_map[IF_FLAG] |= 0x08;

					temp_buffer[1] = 0x1;

					if((four_player.current_state == FOUR_PLAYER_PING) && (temp_buffer[0] == 0xCC))
					{
						four_player.current_state = FOUR_PLAYER_SYNC;
						sio_stat.ping_count = 0;
					}

					else if((four_player.current_state == FOUR_PLAYER_SYNC) && (temp_buffer[0] != 0xCC))
					{
						four_player.current_state = FOUR_PLAYER_PROCESS_NETWORK;
						sio_stat.ping_count = 0;
					}

					switch(four_player.current_state)
					{
						//Handle ping
						case FOUR_PLAYER_PING:

							//Update current link status
							if((sio_stat.ping_count == 1) || (sio_stat.ping_count == 2))
							{
								if(sio_stat.transfer_byte == 0x88) { four_player.status |= (1 << (four_player.id + 3)); }
								else { four_player.status &= ~(1 << (four_player.id + 3)); }
							}

							//Return magic byte for 1st byte
							if(sio_stat.ping_count == 0) { mem->memory_map[REG_SB] = 0xFE; }

							//Otherwise, return status byte
							else { mem->memory_map[REG_SB] = four_player.status; }

							sio_stat.ping_count++;
							sio_stat.ping_count &= 0x3;

							break;
					}

					//Send acknowlegdement
					SDLNet_TCP_Send(four_player_sender[x].host_socket, (void*)temp_buffer, 2);

					return true;
				}

				//4-Player - Return current transfer value
				else if(temp_buffer[1] == 0xFB)
				{
					temp_buffer[0] = sio_stat.transfer_byte;
					temp_buffer[1] = 0x1;

					//Send acknowlegdement
					SDLNet_TCP_Send(four_player_sender[x].host_socket, (void*)temp_buffer, 2);

					return true;
				}

				//4-Player - Reset status
				else if(temp_buffer[1] == 0xFA)
				{
					temp_buffer[1] = 0x1;
	
					four_player.status &= 0x7;
					four_player.current_state = FOUR_PLAYER_PING;
					sio_stat.ping_count = 0;

					//Send acknowlegdement
					SDLNet_TCP_Send(four_player_sender[x].host_socket, (void*)temp_buffer, 2);

					return true;
				}

				//Request sync status
				if(temp_buffer[1] == 0xF8)
				{
					temp_buffer[0] = sio_stat.sync ? 1 : 0;
					temp_buffer[1] = 0x1;

					//Send acknowlegdement
					SDLNet_TCP_Send(four_player_sender[x].host_socket, (void*)temp_buffer, 2);

					return true;
				}

				//4-Player - Confirm SB write for Players 2, 3, and 4
				if(temp_buffer[1] == 0xF7)
				{
					sio_stat.send_data = false;
					temp_buffer[1] = 0x1;

					//Send acknowlegdement
					SDLNet_TCP_Send(four_player_sender[x].host_socket, (void*)temp_buffer, 2);

					return true;
				}

				//Stop sync with acknowledgement
				if(temp_buffer[1] == 0xF0)
				{
					sio_stat.sync = false;
					sio_stat.sync_counter = 0;

					temp_buffer[1] = 0x1;

					//Send acknowlegdement
					SDLNet_TCP_Send(four_player_sender[x].host_socket, (void*)temp_buffer, 2);

					return true;
				}

				//Disconnect netplay
				else if(temp_buffer[1] == 0x80)
				{
					std::cout<<"SIO::Netplay connection terminated. Restart to reconnect.\n";
					sio_stat.connected = false;
					sio_stat.sync = false;

					if(sio_stat.sio_type == GB_FOUR_PLAYER_ADAPTER)
					{
						four_player.wait_flags = 0;
					
						//Resume ping for Players 2-4
						if((sio_stat.network_id & 0x40) && (four_player.current_state))
						{
							sio_stat.network_id &= ~0x40;
							sio_stat.network_id |= 0x80;

							sio_stat.active_transfer = true;
							sio_stat.shifts_left = 8;
							sio_stat.shift_counter = 0;
							sio_stat.shift_clock = 4096;
						}
					}

					return true;
				}

				else if(temp_buffer[1] != 0) { return true; }

				//Raise SIO IRQ after sending byte
				mem->memory_map[IF_FLAG] |= 0x08;

				//Store byte from transfer into SB
				sio_stat.transfer_byte = mem->memory_map[REG_SB];
				mem->memory_map[REG_SB] = temp_buffer[0];

				//Reset Bit 7 of SC
				mem->memory_map[REG_SC] &= ~0x80;

				//Send other Game Boy the old SB value
				temp_buffer[0] = sio_stat.transfer_byte;
				sio_stat.transfer_byte = mem->memory_map[REG_SB];

				if(SDLNet_TCP_Send(four_player_sender[x].host_socket, (void*)temp_buffer, 2) < 2)
				{
					std::cout<<"SIO::Error - Host failed to send data to client\n";
					sio_stat.connected = false;
					four_player_server[x].connected = false;
					four_player_sender[x].connected = false;
					return false;
				}
			}
		}
	}

	#endif

	return true;
}

/****** Broadcasts data to all connected Game Boys when using the 4 Player Adapter ******/
void DMG_SIO::four_player_broadcast(u8 data_one, u8 data_two)
{
	#ifdef GBE_NETPLAY

	//Process incoming data first
	four_player_receive_byte();

	if(!sio_stat.connected) { return; }

	u8 temp_buffer[2];

	for(int x = 0; x < 3; x++)
	{
		//Only broadcast to instances where connection is verified
		if((four_player_server[x].connected) && (four_player_sender[x].connected))
		{
			temp_buffer[0] = data_one;
			temp_buffer[1] = data_two;

			if(SDLNet_TCP_Send(four_player_sender[x].host_socket, (void*)temp_buffer, 2) < 2)
			{
				std::cout<<"SIO::Error - Host failed to send data to client\n";
				sio_stat.connected = false;
				four_player_server[x].connected = false;
				four_player_sender[x].connected = false;
				return;
			}

			//Wait for other instance of GBE+ to send an acknowledgement
			//This is blocking, will effectively pause GBE+ until it gets something
			SDLNet_TCP_Recv(four_player_server[x].remote_socket, temp_buffer, 2);
		}
	}

	#endif

	return;
}

/****** Master requests data from a specific Game Boys when using the 4 Player Adapter ******/
u8 DMG_SIO::four_player_request(u8 data_one, u8 data_two, u8 id)
{
	#ifdef GBE_NETPLAY

	//Process incoming data first
	four_player_receive_byte();

	if(!sio_stat.connected || !is_master) { return 0; }

	u8 temp_buffer[2];
	temp_buffer[0] = data_one;
	temp_buffer[1] = data_two;

	if(!four_player_server[id].connected || !four_player_sender[id].connected) { return 0; }

	if(SDLNet_TCP_Send(four_player_sender[id].host_socket, (void*)temp_buffer, 2) < 2)
	{
		std::cout<<"SIO::Error - Host failed to send data to client\n";
		sio_stat.connected = false;
		four_player_server[id].connected = false;
		four_player_sender[id].connected = false;
		return false;
	}

	//Wait for other instance of GBE+ to send an acknowledgement
	//This is blocking, will effectively pause GBE+ until it gets something
	SDLNet_TCP_Recv(four_player_server[id].remote_socket, temp_buffer, 2);

	return temp_buffer[0];

	#endif

	return 0;
}

/****** Processes data sent to the Game Boy via 4 Player Adapter ******/
void DMG_SIO::four_player_process()
{
	if(sio_stat.internal_clock) { four_player.current_state = FOUR_PLAYER_INACTIVE; }
	
	//Start Link Cable sync
	else if((four_player.current_state == FOUR_PLAYER_PING) && (sio_stat.transfer_byte == 0xAA) && (!four_player.begin_network_sync))
	{
		four_player.begin_network_sync = true;
	}

	//Start Link Cable ping
	else if((four_player.current_state != FOUR_PLAYER_SYNC) && (four_player.current_state != FOUR_PLAYER_PROCESS_NETWORK) && (four_player.current_state != FOUR_PLAYER_RESTART_NETWORK))
	{
		four_player.current_state = FOUR_PLAYER_PING;
	}

	//End network comms and start ping again
	else if((four_player.current_state == FOUR_PLAYER_PROCESS_NETWORK) && (sio_stat.transfer_byte == 0xFF) && (!four_player.restart_network))
	{
		four_player.restart_network = true;
	}

	u8 req_byte[3] = {0, 0, 0};
	u8 buff_pos = 0;

	switch(four_player.current_state)
	{
		case FOUR_PLAYER_INACTIVE: return;

		//Handle ping
		case FOUR_PLAYER_PING:

			//Wait for other players to send bytes
			while(four_player.wait_flags) {	four_player_data_status(); }

			//Reset wait flags
			if(sio_stat.connected) { four_player.wait_flags |= 0x2; }
				
			//Update current link status
			if((sio_stat.ping_count == 1) || (sio_stat.ping_count == 2))
			{
				if((sio_stat.transfer_byte == 0x88) || (four_player.begin_network_sync)) { four_player.status |= (1 << (four_player.id + 3)); }
				else { four_player.status &= ~(1 << (four_player.id + 3)); }
			}

			//Grab remote link status Player 2
			req_byte[0] = four_player_request(four_player.status, 0xFD, 0);
			if(req_byte[0] & 0x20) { four_player.status |= 0x20; }
			else { four_player.status &= ~0x20; }

			//Grab remote link status Player 3
			req_byte[1] = four_player_request(four_player.status, 0xFD, 1);
			if(req_byte[1] & 0x40) { four_player.status |= 0x40; }
			else { four_player.status &= ~0x40; }

			//Grab remote link status Player 4
			req_byte[2] = four_player_request(four_player.status, 0xFD, 2);
			if(req_byte[2] & 0x80) { four_player.status |= 0x80; }
			else { four_player.status &= ~0x80; }

			four_player_broadcast(four_player.status, 0xFD);

			//Player 1 - Return magic byte for 1st byte
			if(sio_stat.ping_count == 0) { mem->memory_map[REG_SB] = 0xFE; }

			//Player 1 - Otherwise, return status byte
			else { mem->memory_map[REG_SB] = four_player.status; }

			mem->memory_map[IF_FLAG] |= 0x08;

			//Process Player 2-4
			four_player_broadcast(0, 0xFC);

			sio_stat.ping_count++;
			sio_stat.ping_count &= 0x3;

			//Grab packet size
			if((sio_stat.ping_count == 1) && (!four_player.begin_network_sync)) { four_player.packet_size = sio_stat.transfer_byte; }

			//Grab clock rate
			if((sio_stat.ping_count == 0) && (!four_player.begin_network_sync)) { four_player.clock = (6 * sio_stat.transfer_byte) + 512; }

			//Wait until current packet is finished before starting network sync
			if((sio_stat.ping_count == 0) && (four_player.begin_network_sync))
			{
				four_player.begin_network_sync = false;
				four_player.current_state = FOUR_PLAYER_SYNC;

				//Change DMG-07 clock based on input from master DMG
				sio_stat.dmg07_clock = four_player.clock;
			}

			break;

		//Sync via Link Cable
		case FOUR_PLAYER_SYNC:

			//Wait for other players to send bytes
			while(four_player.wait_flags) {	four_player_data_status(); }

			//Reset wait flags
			if(sio_stat.connected) { four_player.wait_flags |= 0x2; }

			//Player 1 - Return 0xCC
			mem->memory_map[REG_SB] = 0xCC;
			mem->memory_map[IF_FLAG] |= 0x08;

			//Players 2-4 - Return 0xCC
			four_player_broadcast(0xCC, 0xFC);

			sio_stat.ping_count++;
			sio_stat.ping_count &= 0x3;

			if(sio_stat.ping_count == 0)
			{
				four_player.current_state = FOUR_PLAYER_PROCESS_NETWORK;
				four_player.buffer_pos = 0;
				four_player.buffer.clear();

				//Data in initial buffer is technically garbage. Set to zero for now.
				for(u32 x = 0; x < (four_player.packet_size * 8); x++)
				{
					four_player.buffer.push_back(0);
				}
			}

			break;

		//Send and receive data via Link Cable
		case FOUR_PLAYER_PROCESS_NETWORK:

			//Wait for other players to send bytes
			while(four_player.wait_flags) {	four_player_data_status(); }

			//Reset wait flags
			if(sio_stat.connected) { four_player.wait_flags |= 0x2; }

			//Player 1 - Return buffered data
			mem->memory_map[REG_SB] = four_player.buffer[four_player.buffer_pos];
			mem->memory_map[IF_FLAG] |= 0x08;

			//Players 2-4 - Return buffered data
			four_player_broadcast(four_player.buffer[four_player.buffer_pos], 0xFC);

			buff_pos = four_player.buffer_pos % (four_player.packet_size * 4);

			//Update buffer
			if((buff_pos > 0) && (buff_pos < (four_player.packet_size + 1)))
			{
				u8 temp_pos = ((four_player.buffer_pos - 1) + (four_player.packet_size * 4)) % (four_player.packet_size * 8);

				//Grab Player 1 data
				four_player.buffer[temp_pos] = sio_stat.transfer_byte;

				//Grab Player 2 data
				u8 p_data = four_player_request(0, 0xFB, 0);
				four_player.buffer[temp_pos + four_player.packet_size] = p_data;

				//Grab Player 3 data
				p_data = four_player_request(0, 0xFB, 1);
				four_player.buffer[temp_pos + (four_player.packet_size * 2)] = p_data;

				//Grab Player 4 data
				p_data = four_player_request(0, 0xFB, 2);
				four_player.buffer[temp_pos + (four_player.packet_size * 3)] = p_data;
			}

			sio_stat.ping_count++;
			sio_stat.ping_count = sio_stat.ping_count % (four_player.packet_size * 4);

			four_player.buffer_pos++;
			four_player.buffer_pos = four_player.buffer_pos % (four_player.packet_size * 8);

			if((sio_stat.ping_count == 0) && (four_player.restart_network))
			{
				four_player.restart_network = false;
				four_player.current_state = FOUR_PLAYER_RESTART_NETWORK;
			}

			break;

		//Restart network and enter ping phase
		case FOUR_PLAYER_RESTART_NETWORK:

			//Wait for other players to send bytes
			while(four_player.wait_flags) {	four_player_data_status(); }

			//Reset wait flags
			if(sio_stat.connected) { four_player.wait_flags |= 0x2; }

			//Player 1 - Return 0xFF
			mem->memory_map[REG_SB] = 0xFF;
			mem->memory_map[IF_FLAG] |= 0x08;

			//Players 2-4 - Return 0xFF
			four_player_broadcast(0xFF, 0xFC);

			sio_stat.ping_count++;
			sio_stat.ping_count = sio_stat.ping_count % (four_player.packet_size * 4);

			if(sio_stat.ping_count == 0)
			{
				four_player.current_state = FOUR_PLAYER_PING;
				four_player.status &= 0x7;

				//Players 2-4 - Clear status
				four_player_broadcast(0, 0xFA);
			}

			break;
	}
}

/****** Updates current status for 4 Player Adapter ******/
void DMG_SIO::four_player_update_status(u8 status)
{
	u8 status_bits = status;
			
	switch(four_player.id)
	{
		case 0x1: status_bits &= 0xE0; four_player.status &= ~0xE0; break;
		case 0x2: status_bits &= 0xD0; four_player.status &= ~0xD0; break;
		case 0x3: status_bits &= 0xB0; four_player.status &= ~0xB0; break;
		case 0x4: status_bits &= 0x70; four_player.status &= ~0x70; break;
	}
	
	four_player.status |= status_bits;
}
