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

	//Load Mobile Adapter data + internal server list
	if(sio_stat.sio_type == GBA_MOBILE_ADAPTER)
	{
		std::string mobile_conf_file = config::data_path + "gbma_conf.bin";
		std::ifstream mobile_conf(mobile_conf_file.c_str(), std::ios::binary);

		if(!mobile_conf.is_open()) 
		{ 
			std::cout<<"SIO::Mobile Adapter GB configuration data could not be read. Check file path or permissions. \n";
			return;
		}

		//Get file size
		mobile_conf.seekg(0, mobile_conf.end);
		u32 conf_size = mobile_conf.tellg();
		mobile_conf.seekg(0, mobile_conf.beg);

		if(conf_size != 0xC0)
		{
			std::cout<<"SIO::GB Mobile Adapter configuration data size was incorrect. Aborting attempt to read it. \n";
			mobile_conf.close();
			return;
		}

		u8* ex_data = &mobile_adapter.data[0];

		mobile_conf.read((char*)ex_data, 0xC0); 
		mobile_conf.close();

		std::cout<<"SIO::Loaded Mobile Adapter GB configuration data.\n";

		mobile_adapter_load_server_list();
	}
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

	//Save any new Legendz data for updated Soul Doll
	soul_doll_adapter_save_data();

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
		case 0x4:
		case 0x5:
		case 0x6:
			sio_stat.sio_type = INVALID_GBA_DEVICE;
			break;

		//Mobile Adapter GB
		case 0x3:
			sio_stat.sio_type = GBA_MOBILE_ADAPTER;
			break;

		//GB Player Rumble
		case 0x8:
			sio_stat.sio_type = GBA_PLAYER_RUMBLE;
			break;

		//Soul Doll Adapter
		case 0x9:
			sio_stat.sio_type = GBA_SOUL_DOLL_ADAPTER;
			break;

		//Battle Chip Gate, Progress Chip Gate, and Beast Link Gate
		case 0xA:
		case 0xB:
		case 0xC:
			sio_stat.sio_type = GBA_BATTLE_CHIP_GATE;
			break;

		//Power Antenna + Bug Sensor
		case 0xD:
			sio_stat.sio_type = GBA_POWER_ANTENNA;
			break;

		//Multi Plust On System
		case 0xF:
			sio_stat.sio_type = GBA_MULTI_PLUST_ON_SYSTEM;
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
	soul_doll_adapter_reset();

	if(config::sio_device == 9) { soul_doll_adapter_load_data(config::external_data_file); }

	//Battle Chip Gate
	chip_gate.data = 0x8E70;
	chip_gate.id = 0;
	chip_gate.data_inc = 3;
	chip_gate.data_dec = 3;
	chip_gate.data_count = 0;
	chip_gate.net_gate_count = 0;
	chip_gate.start = false;
	chip_gate.current_state = GBA_BATTLE_CHIP_GATE_STANDBY;

	//GB Mobile Adapter
	mobile_adapter.data.clear();
	mobile_adapter.data.resize(0xC0, 0x0);
	mobile_adapter.packet_buffer.clear();
	mobile_adapter.net_data.clear();
	mobile_adapter.packet_size = 0;
	mobile_adapter.current_state = AGB_GBMA_AWAITING_PACKET;
	mobile_adapter.srv_list_in.clear();
	mobile_adapter.srv_list_out.clear();

	mobile_adapter.command = 0;
	mobile_adapter.data_length = 0;
	mobile_adapter.checksum = 0;

	mobile_adapter.port = 0;
	mobile_adapter.ip_addr = 0;
	mobile_adapter.transfer_state = 0;
	mobile_adapter.line_busy = false;
	mobile_adapter.pop_session_started = false;
	mobile_adapter.http_session_started = false;
	mobile_adapter.smtp_session_started = false;
	mobile_adapter.http_data = "";

	//Multi Plust On System
	mpos.data.clear();
	mpos.current_state = AGB_MPOS_INIT;
	mpos.data_count = 0;
	mpos.id = config::mpos_id;

	mpos_generate_data();

	switch(config::sio_device)
	{
		case 0xA: chip_gate.unit_code = 0xFFC6; break;
		case 0xB: chip_gate.unit_code = 0xFFC7; break;
		case 0xC: chip_gate.unit_code = 0xFFC4; break;
		default: chip_gate.unit_code = 0;
	}

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
	if((!sio_stat.connected) && (sio_stat.sio_type != INVALID_GBA_DEVICE) && (!config::use_net_gate))
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

/****** Resets Soul Doll Adapter ******/
void AGB_SIO::soul_doll_adapter_reset()
{
	sda.data.clear();
	sda.stream_byte.clear();
	sda.stream_word.clear();
	sda.stop_signal = 0xFF2727FF;
	sda.eeprom_addr = 0;
	sda.prev_data = 0;
	sda.prev_write = 0;
	sda.data_count = 0;
	sda.slave_addr = 0xFF;
	sda.word_addr = 0;
	sda.eeprom_cmd = 0xFF;
	sda.flags = 0;
	sda.get_slave_addr = true;
	sda.update_soul_doll = false;
	sda.current_state = GBA_SOUL_DOLL_ADAPTER_ECHO;
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

	if(file_size != 0x400)
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

/****** Update binary file for Soul Doll ******/
bool AGB_SIO::soul_doll_adapter_save_data()
{
	if(sda.update_soul_doll)
	{
		std::ofstream out_data(config::external_data_file.c_str(), std::ios::binary);
		
		if(!out_data.is_open())
		{
			std::cout<<"SIO::Error - Could not update Soul Doll data file. Check file path or permissions. \n";
			return false;
		}

		out_data.write((char*)&sda.data[0], 0x400);
		out_data.close();
	}

	return true;
} 

/****** Process Soul Doll Adapter ******/
void AGB_SIO::soul_doll_adapter_process()
{
	//Abort any processing if Soul Doll data was not loaded
	if(sda.flags & 0x80) { return; }

	//Soul Doll Adapter Echo Mode - Echo bytes until EEPROM command detected
	if(sda.current_state == GBA_SOUL_DOLL_ADAPTER_ECHO)
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

		//Add LSB of RCNT to stream for later analysis
		sda.stream_byte.push_back(sio_stat.r_cnt & 0xFF);

		//Update data stream as 32-bit values
		if(sda.stream_byte.size() == 4)
		{
			u32 word = (sda.stream_byte[0] << 24) | (sda.stream_byte[1] << 16) | (sda.stream_byte[2] << 8) | sda.stream_byte[3];
			sda.stream_word.push_back(word);
			sda.stream_byte.clear();
		}

		//Check for device start signal V1 - Clear stream and wait for slave address
		if((sda.prev_write == 0x8020) && (mem->read_u16_fast(R_CNT) == 0x802D))
		{
			sda.stream_byte.clear();
			sda.stream_word.clear();
			sda.get_slave_addr = true;
		}
	}

	//Soul Doll Adapter Read Mode - Read back bytes from EEPROM
	else if(sda.current_state == GBA_SOUL_DOLL_ADAPTER_READ)
	{
		//Add LSB of RCNT to stream for later analysis
		sda.stream_byte.push_back(sio_stat.r_cnt & 0xFF);

		//Update data stream as 32-bit values
		if(sda.stream_byte.size() == 4)
		{
			u32 word = (sda.stream_byte[0] << 24) | (sda.stream_byte[1] << 16) | (sda.stream_byte[2] << 8) | sda.stream_byte[3];
			sda.stream_word.push_back(word);
			sda.stream_byte.clear();
		}

		//If 0xADAFAFAD received in Read Mode, this signals an end to Read Mode and a return to Echo Mode
		if(sda.stream_word.back() == 0xADAFAFAD)
		{
			sda.current_state = GBA_SOUL_DOLL_ADAPTER_ECHO;
			sda.data_count = 0;
			sda.stream_byte.clear();
			sda.stream_word.clear();
			sda.stop_signal = 0xFF2727FF;
			sda.get_slave_addr = true;
		}

		//Read from EEPROM for 32 transfers (1 byte), echo RCNT until stop signal
		if(sda.data_count < 32)
		{
			sio_stat.r_cnt = 0x8000;

			u8 eeprom_index = sda.data_count / 4;
			u8 eeprom_bit = (sda.data[sda.eeprom_addr] >> (7 - eeprom_index)) & 0x1;
			u32 eeprom_response = (eeprom_bit) ? 0x2D2F2F2D : 0x25272725;
		
			//Return the proper value for RCNT based on bit from EEPROM
			switch(sda.data_count % 4)
			{
				case 0x0:
				case 0x3:
					sio_stat.r_cnt |= (eeprom_bit) ? 0x2D : 0x25;
					break;

				case 0x1:
				case 0x2:
					sio_stat.r_cnt |= (eeprom_bit) ? 0x2F : 0x27;
					break;
			}

			sda.data_count++;
		}
	}

	//Soul Doll Adapter Write Mode - Write to EEPROM
	else if(sda.current_state == GBA_SOUL_DOLL_ADAPTER_WRITE)
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

		//Add LSB of RCNT to stream for later analysis
		sda.stream_byte.push_back(sio_stat.r_cnt & 0xFF);

		//Update data stream as 32-bit values
		if(sda.stream_byte.size() == 4)
		{
			u32 word = (sda.stream_byte[0] << 24) | (sda.stream_byte[1] << 16) | (sda.stream_byte[2] << 8) | sda.stream_byte[3];
			sda.stream_word.push_back(word);
			sda.stream_byte.clear();
		}
	}

	//Check for stop signal in 32-bit stream
	if(!sda.stream_word.empty())
	{
		u8 last_byte = 0;

		//Read and Write commands are formatted similarly
		//After a slave address and word address are received, examine the first word to determine which command it really is
		//If the first word received after slave + word addresses are sent == 0xADAFA7A5, then proceed with Read Mode on the next stop signal
		//Otherwise, entire Write Mode now
		if((sda.stream_word.size() == 1) && (sda.eeprom_cmd == 0) && (sda.get_slave_addr) && (sda.stream_word[0] != 0xADAFA7A5))
		{
			sda.current_state = GBA_SOUL_DOLL_ADAPTER_WRITE;
			sda.stop_signal = 0xFF2727FF;
			sda.data_count = 1;
			sda.eeprom_cmd = 0xFF;
		}

		//Stop signal should be xx2727xx or xxA7A7xx
		else if((sda.stream_word.back() | 0xFF0000FF) == sda.stop_signal)
		{
			//Discard last word if it is the stop signal
			sda.stream_word.pop_back();

			//Calculate last byte from 32-bit stream
			for(u32 x = 0; x < 8; x++)
			{
				u32 word = sda.stream_word[sda.stream_word.size() - x - 1];
				if(word & 0x800) { last_byte |= (1 << x); }
			}

			switch(sda.current_state)
			{
				//Echo Mode - Check for EEPROM command
				case GBA_SOUL_DOLL_ADAPTER_ECHO:

					//Check if the data is a slave address
					if(((last_byte >> 4) == 0xA) && (sda.get_slave_addr))
					{
						u8 last_eeprom_cmd = sda.eeprom_cmd;

						sda.eeprom_addr &= ~0x300;
						sda.eeprom_addr |= ((last_byte & 0x6) << 7);
						sda.slave_addr = (last_byte & 0x6);
						sda.eeprom_cmd = (last_byte & 0x1);
						sda.get_slave_addr = false;

						//If last EEPROM command was Dummy Write (0) and next EEPROM command is read (1), switch to Read Mode
						if((last_eeprom_cmd == 0) && (sda.eeprom_cmd == 1))
						{
							sda.current_state = GBA_SOUL_DOLL_ADAPTER_READ;
							sda.stop_signal = 0xFFA7A7FF;
							sda.data_count = 0;
							sda.eeprom_cmd = 0xFF;
						}
					}

					//Check if the data is a word address
					else if(!sda.get_slave_addr)
					{
						sda.eeprom_addr &= ~0xFF;
						sda.eeprom_addr |= last_byte;
						sda.get_slave_addr = true;
					}

					break;

				//Read Mode - Increment EEPROM address
				case GBA_SOUL_DOLL_ADAPTER_READ:
					sda.eeprom_addr++;
					sda.data_count = 0;
					break;

				//Write Mode - Write data to EEPROM address
				case GBA_SOUL_DOLL_ADAPTER_WRITE:
					sda.data[sda.eeprom_addr] = last_byte;
					sda.update_soul_doll = true;

					sda.eeprom_addr++;
					sda.data_count++;

					//End of Page Write - Return to Echo Mode
					if(sda.data_count == 17)
					{
						sda.current_state = GBA_SOUL_DOLL_ADAPTER_ECHO;
						sda.stop_signal = 0xFF2727FF;
						sda.data_count = 0;
						sda.eeprom_cmd = 0xFF;
						sda.get_slave_addr = false;
					}
						
					break;
			}

			sda.stream_word.clear();
		}
	}

	sio_stat.emu_device_ready = false;
	sda.prev_data = sio_stat.r_cnt;
	sda.prev_write = mem->read_u16_fast(R_CNT);
}

/****** Process Battle Chip Gate ******/
void AGB_SIO::battle_chip_gate_process()
{
	//Process Net Gate if necessary
	if(config::use_net_gate) { net_gate_process(); }

	//Update Battle Chip ID
	chip_gate.id = config::battle_chip_id;

	//Standby Mode
	if(chip_gate.current_state == GBA_BATTLE_CHIP_GATE_STANDBY)
	{
		//Reply with 0x00 for Normal32 transfers - Beast Gate Link
		if(sio_stat.sio_mode == NORMAL_32BIT)
		{
			mem->write_u32_fast(0x4000120, 0xFFFFFFFF);
			mem->memory_map[0x400012A] = 0;
		}

		//Reply with 0xFF6C for Child 1 data
		else if(sio_stat.sio_mode == MULTIPLAY_16BIT) { mem->write_u16_fast(0x4000122, chip_gate.unit_code); }

		//Raise SIO IRQ after sending byte
		if(sio_stat.cnt & 0x4000) { mem->memory_map[REG_IF] |= 0x80; }

		//Wait for potential start signal
		if((sio_stat.transfer_data == 0xA380) || (sio_stat.transfer_data == 0xA3D0) || (sio_stat.transfer_data == 0xA6C0)) { chip_gate.data_count = 0xFFFF; }

		//If start signal received, switch to Chip In Mode
		else if((sio_stat.transfer_data == 0) && (chip_gate.data_count == 0xFFFF))
		{
			chip_gate.current_state = GBA_BATTLE_CHIP_GATE_CHIP_IN;
			chip_gate.data_count = 1;
		}

		else
		{
			chip_gate.data_count++;
			chip_gate.data_count &= 0xFF;
		}
	}

	//Chip In Mode
	if(chip_gate.current_state == GBA_BATTLE_CHIP_GATE_CHIP_IN)
	{
		u8 data_0 = (chip_gate.data >> 8);
		u8 data_1 = chip_gate.data & 0xFF;

		//During Chip In Mode, immediately switch to Standby if start signal detected at any point
		if((sio_stat.transfer_data == 0xA380) || (sio_stat.transfer_data == 0xA3D0) || (sio_stat.transfer_data == 0xA6C0))
		{
			chip_gate.data_count = 0xFFFF;
			mem->write_u16_fast(0x4000122, chip_gate.unit_code);

			//Raise SIO IRQ after sending byte
			if(sio_stat.cnt & 0x4000) { mem->memory_map[REG_IF] |= 0x80; }

			//Clear Bit 7 of SIOCNT
			sio_stat.cnt &= ~0x80;
			mem->write_u16_fast(0x4000128, sio_stat.cnt);

			chip_gate.current_state = GBA_BATTLE_CHIP_GATE_STANDBY;

			return;
		}

		//Respond with sequenced data
		switch(chip_gate.data_count)
		{
			case 0x0:
				mem->write_u16_fast(0x4000122, chip_gate.unit_code);
				break;

			case 0x1:
			case 0x2:
				mem->write_u16_fast(0x4000122, 0xFFFF);
				break;

			case 0x3:
				mem->write_u16_fast(0x4000122, data_0);
				
				data_0 += chip_gate.data_inc;
				chip_gate.data &= ~0xFF00;
				chip_gate.data |= (data_0 << 8);

				break;

			case 0x4:
				mem->write_u16_fast(0x4000122, (0xFF00 | data_1));

				data_1 -= chip_gate.data_dec;
				chip_gate.data &= ~0xFF;
				chip_gate.data |= data_1;

				break;

			case 0x5:
				mem->write_u16_fast(0x4000122, chip_gate.id);
				break;

			default:
				mem->write_u16_fast(0x4000122, 0x0000);
				break;
		}

		chip_gate.data_count++;
		if(chip_gate.data_count == 9) { chip_gate.data_count = 0; }

		//Raise SIO IRQ after sending byte
		if(sio_stat.cnt & 0x4000) { mem->memory_map[REG_IF] |= 0x80; }
	}

	sio_stat.emu_device_ready = false;
	sio_stat.active_transfer = false;

	//Clear Bit 7 of SIOCNT
	sio_stat.cnt &= ~0x80;
	mem->write_u16_fast(0x4000128, sio_stat.cnt);
}

/****** Process Net Gate - Receive Battle Chip IDs via HTTP ******/
void AGB_SIO::net_gate_process()
{
	#ifdef GBE_NETPLAY

	if(network_init && config::use_netplay && !sio_stat.connected)
	{
		//Loop for a while to see if a connection will accept
		for(u32 x = 0; x < 100; x++)
		{
			//Check remote socket for any connections
			if(server.remote_socket = SDLNet_TCP_Accept(server.host_socket))
			{
				u8 temp_buffer[3] = {0, 0, 0};

				//Net Gate protocol is 1-shot, no response, 3 bytes
				if(SDLNet_TCP_Recv(server.remote_socket, temp_buffer, 3) > 0)
				{
					//Set Battle Chip ID from network data
					if(temp_buffer[0] == 0x80)
					{
						config::battle_chip_id = (temp_buffer[1] << 8) | temp_buffer[2];
						chip_gate.net_gate_count = 1024;
					}
				}

				x = 100;
			}
		}
	}

	if(chip_gate.net_gate_count)
	{
		chip_gate.net_gate_count--;
		if(!chip_gate.net_gate_count) { config::battle_chip_id = 0; }
	}

	#endif
}

/****** Process Multi Plust On System ******/
void AGB_SIO::mpos_process()
{
	if(sio_stat.r_cnt == 0x80F0) { mpos.current_state = AGB_MPOS_INIT; }
	else { mpos.current_state = AGB_MPOS_SEND_DATA; }

	switch(mpos.current_state)
	{
		//Init signal. Echo 0x80F0
		case AGB_MPOS_INIT:
			mpos.data_count = 0;
			break;

		//Send looping data
		case AGB_MPOS_SEND_DATA:
			sio_stat.r_cnt = mpos.data[mpos.data_count++];
			if(mpos.data_count == 37) { mpos.data_count = 0; }
			break;
	}

	sio_stat.emu_device_ready = false;
}

/****** Generates data for Multi Plust On System with a 16-bit ID ******/
void AGB_SIO::mpos_generate_data()
{
	mpos.data.clear();
	u16 mask = 0x8000;

	//First 4 responses from hardware do not change
	mpos.data.push_back(0x80B9);
	mpos.data.push_back(0x80B1);
	mpos.data.push_back(0x80BB);
	mpos.data.push_back(0x80BB);

	//Actual data from ID generated here
	while(mask)
	{
		//Grab and translate data, MSB first
		u8 data_bit = (mpos.id & mask) ? 1 : 0;

		//Push back response if data bit is 1
		if(data_bit)
		{
			mpos.data.push_back(0x80BE);
			mpos.data.push_back(0x80BC);
		}

		//Push back response if data bit is 0
		else
		{
			mpos.data.push_back(0x80B8);
			mpos.data.push_back(0x80BA);
		}

		mask >>= 1;
	}

	//Last 2 responses from hardware do not change
	mpos.data.push_back(0x80B8);
	mpos.data.push_back(0x80BA);
}
