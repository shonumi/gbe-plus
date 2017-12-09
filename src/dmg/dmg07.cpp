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

/****** Broadcasts data to all connected Game Boys when using the 4 Player Adapter ******/
void DMG_SIO::four_player_broadcast(u8 data_one, u8 data_two)
{
	#ifdef GBE_NETPLAY

	//Process incoming data first
	receive_byte();

	if(!sio_stat.connected) { return; }

	u8 temp_buffer[2];
	temp_buffer[0] = data_one;
	temp_buffer[1] = data_two;

	if(SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 2) < 2)
	{
		std::cout<<"SIO::Error - Host failed to send data to client\n";
		sio_stat.connected = false;
		server.connected = false;
		sender.connected = false;
		return;
	}

	//Wait for other instance of GBE+ to send an acknowledgement
	//This is blocking, will effectively pause GBE+ until it gets something
	SDLNet_TCP_Recv(server.remote_socket, temp_buffer, 2);

	#endif

	return;
}

/****** Requests data from a specific Game Boys when using the 4 Player Adapter ******/
u8 DMG_SIO::four_player_request(u8 data_one, u8 data_two)
{
	#ifdef GBE_NETPLAY

	//Process incoming data first
	receive_byte();

	if(!sio_stat.connected) { return 0; }

	u8 temp_buffer[2];
	temp_buffer[0] = data_one;
	temp_buffer[1] = data_two;

	if(SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 2) < 2)
	{
		std::cout<<"SIO::Error - Host failed to send data to client\n";
		sio_stat.connected = false;
		server.connected = false;
		sender.connected = false;
		return false;
	}

	//Wait for other instance of GBE+ to send an acknowledgement
	//This is blocking, will effectively pause GBE+ until it gets something
	SDLNet_TCP_Recv(server.remote_socket, temp_buffer, 2);

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

	u8 req_byte = 0;
	u8 buff_pos = 0;

	switch(four_player.current_state)
	{
		case FOUR_PLAYER_INACTIVE: return;

		//Handle ping
		case FOUR_PLAYER_PING:

			//Wait for other players to send bytes
			while(four_player.wait_flags) {	four_player_broadcast(0, 0xF0); }

			//Reset wait flags
			if(sio_stat.connected) { four_player.wait_flags |= 0x2; }
				
			//Update current link status
			if((sio_stat.ping_count == 1) || (sio_stat.ping_count == 2))
			{
				if((sio_stat.transfer_byte == 0x88) || (four_player.begin_network_sync)) { four_player.status |= (1 << (four_player.id + 3)); }
				else { four_player.status &= ~(1 << (four_player.id + 3)); }
			}

			//Update remote link status
			req_byte = four_player_request(four_player.status, 0xFD);
			four_player_update_status(req_byte);

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
			while(four_player.wait_flags) {	four_player_broadcast(0, 0xF0); }

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
			while(four_player.wait_flags) {	four_player_broadcast(0, 0xF0); }

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
				u8 p_data = four_player_request(0, 0xFB);
				four_player.buffer[temp_pos + four_player.packet_size] = p_data;

				//Grab Player 3 data
				p_data = 0x00;
				four_player.buffer[temp_pos + (four_player.packet_size * 2)] = p_data;

				//Grab Player 4 data
				p_data = 0x00;
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
			while(four_player.wait_flags) {	four_player_broadcast(0, 0xF0); }

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
	u8 id = (status & 0xF);
	u8 status_bit = status & (1 << (id + 3));
				
	four_player.status &= ~(1 << (id + 3));
	four_player.status |= status_bit;
}
