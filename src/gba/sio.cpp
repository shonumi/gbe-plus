// GB Enhanced Copyright Daniel Baxter 2018
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : sio.cpp
// Date : August 14, 2018
// Description : Game Boy Advance Serial Input-Output emulation
//
// Sets up SDL networking
// Emulates GBA-to-GBA data transfers

#include <ctime>
#include <cstdlib>

#include "sio.h"
#include "common/util.h"

/****** SIO Constructor ******/
AGB_SIO::AGB_SIO()
{
	network_init = false;

	reset();
}

/****** SIO Destructor ******/
AGB_SIO::~AGB_SIO()
{
	#ifdef GBE_NETPLAY

	//Close SDL_net and any current connections
	if(server.host_socket != NULL)
	{
		SDLNet_TCP_DelSocket(tcp_sockets, server.host_socket);
		if(server.host_init) { SDLNet_TCP_Close(server.host_socket); }
	}

	if(server.remote_socket != NULL)
	{
		SDLNet_TCP_DelSocket(tcp_sockets, server.remote_socket);
		if(server.remote_init) { SDLNet_TCP_Close(server.remote_socket); }
	}

	if(sender.host_socket != NULL)
	{
		//Send disconnect byte to another system
		u8 temp_buffer[5] = {0, 0, 0, 0, 0x80} ;
		
		SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 5);

		SDLNet_TCP_DelSocket(tcp_sockets, sender.host_socket);
		if(sender.host_init) { SDLNet_TCP_Close(sender.host_socket); }
	}

	server.connected = false;
	sender.connected = false;

	server.host_init = false;
	server.remote_init = false;
	sender.host_init = false;

	SDLNet_Quit();

	network_init = false;

	#endif

	std::cout<<"SIO::Shutdown\n";
}

/****** Initialize SIO ******/
bool AGB_SIO::init()
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

	network_init = true;

	//Server info
	server.host_socket = NULL;
	server.host_init = false;
	server.remote_socket = NULL;
	server.remote_init = false;
	server.connected = false;
	server.port = config::netplay_server_port;

	//Client info
	sender.host_socket = NULL;
	sender.host_init = false;
	sender.connected = false;
	sender.port = config::netplay_client_port;

	//Abort initialization if server and client ports are the same
	if(config::netplay_server_port == config::netplay_client_port)
	{
		std::cout<<"SIO::Error - Server and client ports are the same. Could not initialize SDL_net\n";
		return false;
	}

	//Setup server, resolve the server with NULL as the hostname, the server will now listen for connections
	if(SDLNet_ResolveHost(&server.host_ip, NULL, server.port) < 0)
	{
		std::cout<<"SIO::Error - Server could not resolve hostname\n";
		return false;
	}

	//Open a connection to listen on host's port
	if(!(server.host_socket = SDLNet_TCP_Open(&server.host_ip)))
	{
		std::cout<<"SIO::Error - Server could not open a connection on Port " << server.port << "\n";
		return false;
	}

	server.host_init = true;

	//Setup client, listen on another port
	if(SDLNet_ResolveHost(&sender.host_ip, config::netplay_client_ip.c_str(), sender.port) < 0)
	{
		std::cout<<"SIO::Error - Client could not resolve hostname\n";
		return false;
	}

	//Create sockets sets
	tcp_sockets = SDLNet_AllocSocketSet(3);

	//Initialize hard syncing
	if(config::netplay_hard_sync)
	{
		//The instance with the highest server port will start off waiting in sync mode
		sio_stat.sync_counter = (config::netplay_server_port > config::netplay_client_port) ? 64 : 0;
	}

	#endif

	std::cout<<"SIO::Initialized\n";
	return true;
}

/****** Reset SIO ******/
void AGB_SIO::reset()
{
	//General SIO
	sio_stat.connected = false;
	sio_stat.active_transfer = false;
	sio_stat.internal_clock = false;
	sio_stat.sync_counter = 0;
	sio_stat.sync_clock = config::netplay_sync_threshold;
	sio_stat.sync = false;
	sio_stat.connection_ready = false;
	sio_stat.emu_device_ready = false;
	sio_stat.transfer_data = 0;
	sio_stat.shift_counter = 64;
	sio_stat.shift_clock = 0;
	sio_stat.shifts_left = 0;
	sio_stat.r_cnt = 0x8000;
	sio_stat.cnt = 0;
	sio_stat.player_id = config::netplay_id;

	switch(config::sio_device)
	{
		//Ignore invalid DMG/GBC devices
		case 0x1:
		case 0x2:
		case 0x3:
		case 0x4:
		case 0x5:
		case 0x6:
			sio_stat.sio_type = INVALID_GBA_DEVICE;
			break;

		//Reserved for other GBA SIO devices
		case 0x8:
			sio_stat.sio_type = GBA_PLAYER_RUMBLE;
			break;

		case 0x9:
			sio_stat.sio_type = GBA_SOUL_DOLL_ADAPTER;
			break;

		//Always wait until netplay connection is established to change to GBA_LINK
		default:
			sio_stat.sio_type = NO_GBA_DEVICE;
			break;
	}

	sio_stat.sio_mode = GENERAL_PURPOSE;

	//GBA Player Rumble
	player_rumble.sio_buffer.push_back(0x0000494E);
	player_rumble.sio_buffer.push_back(0x8888494E);
	player_rumble.sio_buffer.push_back(0xB6B1494E);
	player_rumble.sio_buffer.push_back(0xB6B1544E);
	player_rumble.sio_buffer.push_back(0xABB1544E);
	player_rumble.sio_buffer.push_back(0xABB14E45);
	player_rumble.sio_buffer.push_back(0xB1BA4E45);
	player_rumble.sio_buffer.push_back(0xB1BA4F44);
	player_rumble.sio_buffer.push_back(0xB0BB4F44);
	player_rumble.sio_buffer.push_back(0xB0BB8002);
	player_rumble.sio_buffer.push_back(0x10000010);
	player_rumble.sio_buffer.push_back(0x20000013);
	player_rumble.sio_buffer.push_back(0x30000003);
	player_rumble.sio_buffer.push_back(0x30000003);
	player_rumble.sio_buffer.push_back(0x30000003);
	player_rumble.sio_buffer.push_back(0x30000003);
	player_rumble.sio_buffer.push_back(0x30000003);
	player_rumble.sio_buffer.push_back(0x00000000);
	
	player_rumble.buffer_index = 0;
	player_rumble.current_state = GB_PLAYER_RUMBLE_INACTIVE;

	//Soul Doll Adapter
	sda.data.clear();
	sda.data_section = 0;
	sda.buffer_index = 0;
	sda.data_count = 0;
	sda.delay = 0;
	sda.flags = 0;
	sda.current_state = GBA_SOUL_DOLL_ADAPTER_INACTIVE;
	sda.prev_data = 0;
	sda.prev_write = 0;
	if(config::sio_device == 9) { soul_doll_adapter_load_data(config::external_data_file); }

	#ifdef GBE_NETPLAY

	//Close any current connections
	if(network_init)
	{	
		if((server.host_socket != NULL) && (server.host_init))
		{
			SDLNet_TCP_Close(server.host_socket);
		}

		if((server.remote_socket != NULL) && (server.remote_init))
		{
			SDLNet_TCP_Close(server.remote_socket);
		}

		if(sender.host_socket != NULL)
		{
			//Send disconnect byte to another system
			u8 temp_buffer[5] = {0, 0, 0, 0, 0x80} ;
		
			SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 5);

			if(sender.host_init) { SDLNet_TCP_Close(sender.host_socket); }
		}
	}

	#endif
}

/****** Transfers one data to another system ******/
bool AGB_SIO::send_data()
{
	#ifdef GBE_NETPLAY

	u8 temp_buffer[5];
	temp_buffer[0] = (sio_stat.transfer_data & 0xFF);
	temp_buffer[1] = ((sio_stat.transfer_data >> 8) & 0xFF);
	temp_buffer[2] = ((sio_stat.transfer_data >> 16) & 0xFF);
	temp_buffer[3] = ((sio_stat.transfer_data >> 24) & 0xFF);
	temp_buffer[4] = (0x40 | sio_stat.player_id);

	if(SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 5) < 5)
	{
		std::cout<<"SIO::Error - Host failed to send data to client\n";
		sio_stat.connected = false;
		server.connected = false;
		sender.connected = false;
		return false;
	}

	//Wait for other GBA to acknowledge
	if(SDLNet_TCP_Recv(server.remote_socket, temp_buffer, 5) > 0)
	{
		//Only process response if the emulated SIO connection is ready
		if(sio_stat.connection_ready)
		{
			//16-bit Multiplayer
			if(sio_stat.sio_mode == MULTIPLAY_16BIT)
			{
				switch(temp_buffer[4])
				{
					case 0x0:
						mem->memory_map[0x4000120] = temp_buffer[0];
						mem->memory_map[0x4000121] = temp_buffer[1];
						break;

					case 0x1:
						mem->memory_map[0x4000122] = temp_buffer[0];
						mem->memory_map[0x4000123] = temp_buffer[1];
						break; 

					case 0x2:
						mem->memory_map[0x4000124] = temp_buffer[0];
						mem->memory_map[0x4000125] = temp_buffer[1];
						break; 

					case 0x3:
						mem->memory_map[0x4000126] = temp_buffer[0];
						mem->memory_map[0x4000127] = temp_buffer[1];
						break;
				}

				//Set master data
				mem->write_u16_fast(0x4000120, sio_stat.transfer_data);

				//Raise SIO IRQ after sending byte
				if(sio_stat.cnt & 0x4000) { mem->memory_map[REG_IF] |= 0x80; }

				//Set SC and SO HIGH on master
				mem->write_u8(R_CNT, (mem->memory_map[R_CNT] | 0x9));
			}
		}

		//Otherwise delay the transfer
		else
		{
			sio_stat.active_transfer = true;
			sio_stat.shifts_left = 16;
			sio_stat.shift_counter = 0;
			mem->memory_map[SIO_CNT] |= 0x80;
		}
	}

	#endif

	return true;
}

/****** Receives one byte from another system ******/
bool AGB_SIO::receive_byte()
{
	#ifdef GBE_NETPLAY

	u8 temp_buffer[5] = {0, 0, 0, 0, 0} ;

	//Check the status of connection
	SDLNet_CheckSockets(tcp_sockets, 0);

	//If this socket is active, receive the transfer
	if(SDLNet_SocketReady(server.remote_socket))
	{
		if(SDLNet_TCP_Recv(server.remote_socket, temp_buffer, 5) > 0)
		{
			//Stop sync
			if(temp_buffer[4] == 0xFF)
			{
				//Check ID byte
				if(temp_buffer[3] == sio_stat.player_id)
				{
					std::cout<<"SIO::Error - Netplay IDs are the same. Closing connection.\n";
					sio_stat.connected = false;
					server.connected = false;
					sender.connected = false;
					return false;
				}

				sio_stat.connection_ready = (temp_buffer[2] == sio_stat.sio_mode) ? true : false;

				sio_stat.sync = false;
				sio_stat.sync_counter = 0;
				return true;
			}

			//Stop sync with acknowledgement
			if(temp_buffer[4] == 0xF0)
			{
				sio_stat.sync = false;
				sio_stat.sync_counter = 0;

				temp_buffer[4] = 0x1;

				//Send acknowlegdement
				SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 5);

				return true;
			}

			//Disconnect netplay
			else if(temp_buffer[4] == 0x80)
			{
				std::cout<<"SIO::Netplay connection terminated. Restart to reconnect.\n";
				sio_stat.connected = false;
				sio_stat.sync = false;

				return true;
			}

			//Process GBA SIO communications
			else if((temp_buffer[4] >= 0x40) && (temp_buffer[4] <= 0x43))
			{
				if(sio_stat.connection_ready)
				{
					//Reset transfer data
					mem->write_u16_fast(0x4000124, 0xFFFF);
					mem->write_u16_fast(0x4000126, 0xFFFF);

					//Raise SIO IRQ after sending byte
					if(sio_stat.cnt & 0x4000) { mem->memory_map[REG_IF] |= 0x80; }

					//Set SO HIGH on all children
					mem->write_u8(R_CNT, (mem->memory_map[R_CNT] | 0x8));

					//Store byte from transfer into SIO data registers - 16-bit Multiplayer
					if(sio_stat.sio_mode == MULTIPLAY_16BIT)
					{
						switch(temp_buffer[4])
						{
							case 0x40:
								mem->memory_map[0x4000120] = temp_buffer[0];
								mem->memory_map[0x4000121] = temp_buffer[1];
								break;

							case 0x41:
								mem->memory_map[0x4000122] = temp_buffer[0];
								mem->memory_map[0x4000123] = temp_buffer[1];
								break; 

							case 0x42:
								mem->memory_map[0x4000124] = temp_buffer[0];
								mem->memory_map[0x4000125] = temp_buffer[1];
								break; 

							case 0x43:
								mem->memory_map[0x4000126] = temp_buffer[0];
								mem->memory_map[0x4000127] = temp_buffer[1];
								break;
						}

						sio_stat.transfer_data = (mem->memory_map[SIO_DATA_8 + 1] << 8) | mem->memory_map[SIO_DATA_8];

						temp_buffer[0] = (sio_stat.transfer_data & 0xFF);
						temp_buffer[1] = ((sio_stat.transfer_data >> 8) & 0xFF);
						temp_buffer[2] = ((sio_stat.transfer_data >> 16) & 0xFF);
						temp_buffer[3] = ((sio_stat.transfer_data >> 24) & 0xFF);
					}

					temp_buffer[4] = sio_stat.player_id;
				}

				//Send acknowledgement
				if(SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 5) < 5)
				{
					std::cout<<"SIO::Error - Host failed to send data to client\n";
					sio_stat.connected = false;
					server.connected = false;
					sender.connected = false;
					return false;
				}

				return true;
			}
		}
	}

	#endif

	return true;
}

/****** Requests syncronization with another system ******/
bool AGB_SIO::request_sync()
{
	#ifdef GBE_NETPLAY

	u8 temp_buffer[5] = {0, 0, sio_stat.sio_mode, sio_stat.player_id, 0xFF} ;

	//Send the sync code 0xFF
	if(SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 5) < 5)
	{
		std::cout<<"SIO::Error - Host failed to send data to client\n";
		sio_stat.connected = false;
		server.connected = false;
		sender.connected = false;
		return false;
	}

	sio_stat.sync = true;

	#endif

	return true;
}

/****** Manages network communication via SDL_net ******/
void AGB_SIO::process_network_communication()
{
	#ifdef GBE_NETPLAY

	//If no communication with another GBE+ instance has been established yet, see if a connection can be made
	if((!sio_stat.connected) && (sio_stat.sio_type != INVALID_GBA_DEVICE))
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
				server.remote_init = true;
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
				sender.host_init = true;
			}
		}

		if((server.connected) && (sender.connected))
		{
			sio_stat.connected = true;
			sio_stat.sio_type = GBA_LINK;
		}
	}

	#endif
}

/****** Processes GB Player Rumble SIO communications ******/
void AGB_SIO::gba_player_rumble_process()
{
	//Check rumble status
	if(player_rumble.buffer_index == 17)
	{
		u8 rumble_stat = mem->memory_map[SIO_DATA_32_L];

		//Turn rumble on
		if(rumble_stat == 0x26) { mem->g_pad->start_rumble(); }

		//Turn rumble off
		else { mem->g_pad->stop_rumble(); }
	}

	//Send data to GBA
	mem->write_u32_fast(SIO_DATA_32_L, player_rumble.sio_buffer[player_rumble.buffer_index++]);

	//Raise SIO IRQ after sending byte
	if(sio_stat.cnt & 0x4000) { mem->memory_map[REG_IF] |= 0x80; }

	if(player_rumble.buffer_index == 18)
	{
		player_rumble.buffer_index = 0;
		sio_stat.emu_device_ready = false;
	}

	else
	{
		sio_stat.shifts_left = 32;
		sio_stat.shift_counter = 0;
	}

	sio_stat.active_transfer = false;	
}

/****** Loads Soul Doll Adapter Data ******/
bool AGB_SIO::soul_doll_adapter_load_data(std::string filename)
{
	std::ifstream doll_data(filename.c_str(), std::ios::binary);

	if(!doll_data.is_open()) 
	{ 
		std::cout<<"SIO::Soul Doll data could not be read. Check file path or permissions. \n";
		sda.flags |= 0x80;
		return false;
	}

	//Get file size
	doll_data.seekg(0, doll_data.end);
	u32 file_size = doll_data.tellg();
	doll_data.seekg(0, doll_data.beg);

	if(file_size != 0x9000)
	{
		std::cout<<"SIO::Error - Soul Doll data has incorrect size\n";
		sda.flags |= 0x80;
		return false;
	}

	sda.data.resize(file_size, 0);
	sda.flags &= ~0x80;

	u8* ex_data = &sda.data[0];

	doll_data.read((char*)ex_data, file_size); 
	doll_data.close();

	std::cout<<"SIO::Loaded Soul Doll data file " << filename << "\n";
	return true;
}

/****** Process Soul Doll Adapter ******/
void AGB_SIO::soul_doll_adapter_process()
{
	//Abort any processing if Soul Doll data was not loaded
	if(sda.flags & 0x80) { return; }

	//Soul Doll Adapter Inactive - Echo bytes
	if(sda.current_state == GBA_SOUL_DOLL_ADAPTER_INACTIVE)
	{
		//During this phase, echo everything save for the following
		switch(sio_stat.r_cnt)
		{
			case 0x802D:
				if(sda.prev_data == 0x80A5) { sio_stat.r_cnt = 0x8025; }
				break;

			case 0x802F:
				sio_stat.r_cnt = 0x8027;
				break;
		}

		if(sda.data_count < 3) { sda.data_count++; }
	}

	//Soul Doll Adapter Active - Reply with Doll data
	else if(sda.current_state == GBA_SOUL_DOLL_ADAPTER_ACTIVE)
	{
		sio_stat.r_cnt = (0x8000 | sda.data[sda.buffer_index++]);
		sda.data_count++;

		//1st 4608 transfer data segment
		if((sda.data_count == 4608) && (sda.data_section == 0))
		{
			sda.current_state = GBA_SOUL_DOLL_ADAPTER_DATA_WAIT;
			sda.data_count = 0;
			sda.data_section++;
		}

		//9216 transfer data segments 1-3
		else if((sda.data_count == 9216) && (sda.data_section >= 1) && (sda.data_section <= 3))
		{
			sda.current_state = GBA_SOUL_DOLL_ADAPTER_DATA_WAIT;
			sda.data_count = 0;
			sda.data_section++;
		}

		//2nd 4608 transfer data segment
		else if((sda.data_count == 4608) && (sda.data_section == 4))
		{
			sda.current_state = GBA_SOUL_DOLL_ADAPTER_INACTIVE;
			sda.buffer_index = 0;
			sda.data_count = 0;
		}
	}

	//Soul Doll Adapter Data Wait - Halt processing for certain number of transfers
	else if(sda.current_state == GBA_SOUL_DOLL_ADAPTER_DATA_WAIT)
	{
		sda.data_count++;

		switch(sda.data_section)
		{
			//Wait until 1st 4608 segment starts
			case 0x0:
				if(sda.data_count == 160)
				{
					sda.data_count = 0;
					sda.current_state = GBA_SOUL_DOLL_ADAPTER_ACTIVE;
					sda.delay = 0;
				}

				break;

			//1st 4608 data segment, wait until start signal (0x8020, 0x802D)
			case 0x1:

				//Detect whether or not jump to last 4608 data segment is necessary
				//Seems to be if a large amount of transfers happen here without sending the start signal
				//Sign of Nekrom seems to do this, but not Isle of Trial
				if(sda.data_count >= 1000)
				{	
					//Move buffer index to (4608 + (9216 * 3)), the last 4608 data segment
					sda.flags |= 1;
					sda.buffer_index = 0x7E00;
				}

				//Detect end of 4608 data segment
				if((sda.prev_write == 0x8020) && (mem->read_u16_fast(R_CNT) == 0x802D))
				{
					sda.delay = sda.data_count + 160;
				}

				//Return from DATA_WAIT to ACTIVE
				else if(sda.data_count == sda.delay)
				{
					sda.data_count = 0;
					sda.current_state = GBA_SOUL_DOLL_ADAPTER_ACTIVE;

					if(sda.flags & 0x1) { sda.data_section = 4; }
				}

				break;

			//1st, 2nd, and 3rd 9216 data segments, wait until 168 transfers
			case 0x2:
			case 0x3:
			case 0x4:

				//Return from DATA_WAIT to ACTIVE
				if(sda.data_count == 168)
				{
					sda.data_count = 0;
					sda.current_state = GBA_SOUL_DOLL_ADAPTER_ACTIVE;
				}

				break;
		}

		//During this phase, echo everything save for the following
		switch(sio_stat.r_cnt)
		{
			case 0x802D:
				if(sda.prev_data == 0x80A5) { sio_stat.r_cnt = 0x8025; }
				break;

			case 0x802F:
				sio_stat.r_cnt = 0x8027;
				break;
		}
	}	

	//Start Soul Doll Adapter data transfers
	if((sda.current_state == GBA_SOUL_DOLL_ADAPTER_INACTIVE) && (sda.prev_data == 0x802D) && (sio_stat.r_cnt == 0x802D) && (sda.data_count >= 3) && (sda.data_section == 0))
	{
		sda.current_state = GBA_SOUL_DOLL_ADAPTER_DATA_WAIT;
		sda.buffer_index = 0;
		sda.data_count = 0;
		sda.data_section = 0;
		sda.delay = 160;
		sda.flags &= ~0x1;
	}

	sio_stat.emu_device_ready = false;
	sda.prev_data = sio_stat.r_cnt;
	sda.prev_write = mem->read_u16_fast(R_CNT);
}
