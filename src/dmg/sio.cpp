// GB Enhanced Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : sio.cpp
// Date : April 30, 2016
// Description : Game Boy Serial Input-Output emulation
//
// Sets up SDL networking
// Emulates Gameboy-to-Gameboy data transfers

#include <ctime>
#include <cstdlib>

#include "sio.h"
#include "common/util.h"

/****** SIO Constructor ******/
DMG_SIO::DMG_SIO()
{
	network_init = false;
	dmg07_init = false;
	is_master = false;
	master_id = 0;

	reset();

	//Load Mobile Adapter data
	if(sio_stat.sio_type == GB_MOBILE_ADAPTER)
	{
		std::string mobile_conf_file = config::data_path + "gbma_conf.bin";
		std::ifstream mobile_conf(mobile_conf_file.c_str(), std::ios::binary);

		if(!mobile_conf.is_open()) 
		{ 
			std::cout<<"SIO::GB Mobile Adapter configuration data could not be read. Check file path or permissions. \n";
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

		std::cout<<"SIO::Loaded GB Mobile Adapter configuration data.\n";
	}
}

/****** SIO Destructor ******/
DMG_SIO::~DMG_SIO()
{
	#ifdef GBE_NETPLAY

	//Close any current connections - Four Player
	four_player_disconnect();
		
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
		u8 temp_buffer[2];
		temp_buffer[0] = 0;
		temp_buffer[1] = 0x80;
		
		SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 2);

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

	//Save GB Mobile Adapter data
	if(sio_stat.sio_type == GB_MOBILE_ADAPTER)
	{
		std::string mobile_conf_file = config::data_path + "gbma_conf.bin";
		std::ofstream mobile_conf(mobile_conf_file.c_str(), std::ios::binary);

		if(!mobile_conf.is_open()) 
		{ 
			std::cout<<"SIO::GB Mobile Adapter configuration data could not be written. Check file path or permissions. \n";
			std::cout<<"SIO::Shutdown\n";
			return;
		}

		mobile_conf.write(reinterpret_cast<char*> (&mobile_adapter.data[0]), 0xC0); 
		mobile_conf.close();

		std::cout<<"SIO::Wrote GB Mobile Adapter configuration data.\n";
	}

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

	network_init = true;

	//Initialize DMG-07
	if(sio_stat.sio_type == GB_FOUR_PLAYER_ADAPTER)
	{
		if(four_player_init())
		{
			std::cout<<"SIO::Initialized\n";
			return true;
		}

		else { return false; }
	}

	//Initialize other Link Cable communications normally

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

	//Default Four Player settings 
	for(u32 x = 0; x < 3; x++)
	{
		four_player_server[x].host_socket = NULL;
		four_player_server[x].remote_socket = NULL;
		four_player_server[x].connected = false;
		four_player_server[x].port = 0;

		//Client info
		four_player_sender[x].host_socket = NULL;
		four_player_sender[x].connected = false;
		four_player_sender[x].port = 0;
	}

	#endif

	std::cout<<"SIO::Initialized\n";
	return true;
}

/****** Reset SIO ******/
void DMG_SIO::reset()
{
	//General SIO
	sio_stat.connected = false;
	sio_stat.active_transfer = false;
	sio_stat.double_speed = false;
	sio_stat.internal_clock = false;
	sio_stat.shifts_left = 0;
	sio_stat.shift_counter = 0;
	sio_stat.shift_clock = 512;
	sio_stat.dmg07_clock = 2048;
	sio_stat.sync_counter = 0;
	sio_stat.sync_clock = config::netplay_sync_threshold;
	sio_stat.sync = false;
	sio_stat.transfer_byte = 0;
	sio_stat.network_id = 0;
	sio_stat.ping_count = 0;
	sio_stat.ping_finish = false;
	sio_stat.send_data = false;
	
	switch(config::sio_device)
	{
		//GB Printer
		case 2:
			sio_stat.sio_type = GB_PRINTER;
			break;

		//GB Mobile Adapter
		case 3: 
			sio_stat.sio_type = GB_MOBILE_ADAPTER;
			break;

		//Bardigun barcode scanner
		case 4:
			sio_stat.sio_type = GB_BARDIGUN_SCANNER;
			break;

		//Barcode Boy
		case 0x5:
			sio_stat.sio_type = GB_BARCODE_BOY;
			break;

		//4 Player Adapter
		case 0x6:
			sio_stat.sio_type = GB_FOUR_PLAYER_ADAPTER;
			break;

		//Always wait until netplay connection is established to change to GB_LINK
		//Also, any invalid types are ignored
		default:
			sio_stat.sio_type = NO_GB_DEVICE;
			break;
	}

	switch(config::ir_device)
	{
		//Full Changer
		case 0x1:
			sio_stat.ir_type = GBC_FULL_CHANGER;
			break;

		//Pokemon Pikachu 2
		case 0x2:
			sio_stat.ir_type = GBC_POKEMON_PIKACHU_2;
			break;

		//Pocket Sakura
		case 0x3:
			sio_stat.ir_type = GBC_POCKET_SAKURA;
			break;

		//TV Remote
		case 0x4:
			sio_stat.ir_type = GBC_TV_REMOTE;
			break;

		//Constant Light Source
		case 0x5:
			sio_stat.ir_type = GBC_LIGHT_SOURCE;
			break;

		//Use standard GBC IR port communication as the default (GBE+ will ignore it for DMG games)
		//Also, any invalid types are ignored
		default:
			sio_stat.ir_type = GBC_IR_PORT;
			break;
	}

	//GB Printer
	printer.scanline_buffer.clear();
	printer.scanline_buffer.resize(0x5A00, 0x0);
	printer.packet_buffer.clear();
	printer.packet_size = 0;
	printer.current_state = GBP_AWAITING_PACKET;
	printer.pal[0] = printer.pal[1] = printer.pal[2] = printer.pal[3] = 0;

	printer.command = 0;
	printer.compression_flag = 0;
	printer.strip_count = 0;
	printer.data_length = 0;
	printer.checksum = 0;
	printer.status = 0;

	//GB Mobile Adapter
	mobile_adapter.data.clear();
	mobile_adapter.data.resize(0xC0, 0x0);
	mobile_adapter.packet_buffer.clear();
	mobile_adapter.packet_size = 0;
	mobile_adapter.current_state = GBMA_AWAITING_PACKET;

	mobile_adapter.command = 0;
	mobile_adapter.data_length = 0;
	mobile_adapter.checksum = 0;

	mobile_adapter.port = 0;
	mobile_adapter.ip_addr = 0;
	mobile_adapter.transfer_state = 0;
	mobile_adapter.line_busy = false;
	mobile_adapter.pop_session_started = false;
	mobile_adapter.http_session_started = false;
	mobile_adapter.http_data = "";

	//Bardigun barcode scanner
	bardigun_scanner.data.clear();
	bardigun_scanner.current_state = BARDIGUN_INACTIVE;
	bardigun_scanner.inactive_counter = 0x500;
	bardigun_scanner.barcode_pointer = 0;
	if(config::sio_device == 4) { bardigun_load_barcode(config::external_card_file); }

	//Barcode Boy
	barcode_boy.data.clear();
	barcode_boy.current_state = BARCODE_BOY_INACTIVE;
	barcode_boy.counter = 0;
	barcode_boy.send_data = false;
	if(config::sio_device == 5) { barcode_boy_load_barcode(config::external_card_file); }

	//Full Changer
	full_changer.data.clear();
	full_changer.delay_counter = 0;
	full_changer.current_character = 0;
	full_changer.light_on = false;

	//TV Remote
	tv_remote.data.clear();
	tv_remote.delay_counter = 0;
	tv_remote.current_data = 0;
	tv_remote.light_on = false;

	//Pocket IR
	pocket_ir.data.clear();
	pocket_ir.db_step = 0;
	pocket_ir.current_data = 0;
	pocket_ir.light_on = false;

	srand(time(NULL));
	for(int x = 0; x < 64; x++)
	{
		int random_data = (rand() % 64) + 64;
		tv_remote.data.push_back(random_data);
	}
	
	if(config::ir_device == 1)
	{
		std::string database = config::data_path + "zzh_db.bin";
		full_changer_load_db(database);
	}

	else if(config::ir_device == 2)
	{
		std::string database = config::data_path + "pokemon_pikachu_db.bin";
		pocket_ir.db_step = 0x7D7;
		pocket_ir_load_db(database);
	}

	else if(config::ir_device == 3)
	{
		std::string database = config::data_path + "pocket_sakura_db.bin";
		pocket_ir.db_step = 0x647;
		pocket_ir_load_db(database);
	}

	#ifdef GBE_NETPLAY

	//Close any current connections
	if(network_init)
	{
		for(int x = 0; x < 3; x++)
		{
			//Send disconnect byte to other systems
			if((four_player_server[x].connected) && (four_player_sender[x].connected))
			{
				u8 temp_buffer[2];
				temp_buffer[0] = 0;
				temp_buffer[1] = 0x80;

				SDLNet_TCP_Send(four_player_sender[x].host_socket, (void*)temp_buffer, 2);
			}

			if((four_player_server[x].host_socket != NULL) && (four_player_server[x].connected))
			{
				SDLNet_TCP_Close(four_player_server[x].host_socket);
			}

			if((four_player_server[x].remote_socket != NULL) && (four_player_server[x].connected))
			{
				SDLNet_TCP_Close(four_player_server[x].remote_socket);
			}

			if((four_player_sender[x].host_socket != NULL) && (four_player_sender[x].connected))
			{
				SDLNet_TCP_Close(four_player_sender[x].host_socket);
			}

			four_player_server[x].connected = false;
			four_player_sender[x].connected = false;
		}
		
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
			u8 temp_buffer[2];
			temp_buffer[0] = 0;
			temp_buffer[1] = 0x80;
		
			SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 2);

			if(sender.host_init) { SDLNet_TCP_Close(sender.host_socket); }
		}
	}

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

	#endif

	//4 Player Adapter
	four_player.current_state = FOUR_PLAYER_INACTIVE;
	four_player.id = 1;
	four_player.status = 1;
	four_player.begin_network_sync = false;
	four_player.restart_network = false;
	four_player.buffer_pos = 0;
	four_player.buffer.clear();
	four_player.packet_size = 0;
	four_player.clock = 0;
}

/****** Transfers one byte to another system ******/
bool DMG_SIO::send_byte()
{
	#ifdef GBE_NETPLAY

	u8 temp_buffer[2];
	temp_buffer[0] = sio_stat.transfer_byte;
	temp_buffer[1] = 0;

	if(SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 2) < 2)
	{
		std::cout<<"SIO::Error - Host failed to send data to client\n";
		sio_stat.connected = false;
		server.connected = false;
		sender.connected = false;
		return false;
	}

	//Wait for other Game Boy to send this one its SB
	//This is blocking, will effectively pause GBE+ until it gets something
	if(SDLNet_TCP_Recv(server.remote_socket, temp_buffer, 2) > 0)
	{
		mem->memory_map[REG_SB] = sio_stat.transfer_byte = temp_buffer[0];
	}

	//Raise SIO IRQ after sending byte
	mem->memory_map[IF_FLAG] |= 0x08;

	#endif

	return true;
}

/****** Transfers one bit to another system's IR port ******/
bool DMG_SIO::send_ir_signal()
{
	#ifdef GBE_NETPLAY

	u8 temp_buffer[2];

	//For IR signals, flag it properly
	//1st byte is IR signal data, second byte GBE+'s marker, 0x40
	temp_buffer[0] = mem->ir_signal;
	temp_buffer[1] = 0x40;

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
	if(SDLNet_TCP_Recv(server.remote_socket, temp_buffer, 2) > 0)
	{
		mem->ir_send = false;
	}

	#endif

	return true;
}

/****** Receives one byte from another system ******/
bool DMG_SIO::receive_byte()
{
	#ifdef GBE_NETPLAY

	if(sio_stat.sio_type == GB_FOUR_PLAYER_ADAPTER) { return four_player_receive_byte(); }

	u8 temp_buffer[1];
	temp_buffer[0] = temp_buffer[1] = 0;

	//Check the status of connection
	SDLNet_CheckSockets(tcp_sockets, 0);

	//If this socket is active, receive the transfer
	if(SDLNet_SocketReady(server.remote_socket))
	{
		if(SDLNet_TCP_Recv(server.remote_socket, temp_buffer, 2) > 0)
		{
			//Stop sync
			if(temp_buffer[1] == 0xFF)
			{
				sio_stat.sync = false;
				sio_stat.sync_counter = 0;
				return true;
			}

			//Stop sync with acknowledgement
			if(temp_buffer[1] == 0xF0)
			{
				sio_stat.sync = false;
				sio_stat.sync_counter = 0;

				temp_buffer[1] = 0x1;

				//Send acknowlegdement
				SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 2);

				return true;
			}

			//Disconnect netplay
			else if(temp_buffer[1] == 0x80)
			{
				std::cout<<"SIO::Netplay connection terminated. Restart to reconnect.\n";
				sio_stat.connected = false;
				sio_stat.sync = false;

				return true;
			}

			//Receive IR signal
			else if(temp_buffer[1] == 0x40)
			{
				temp_buffer[1] = 0x41;
				
				//Clear out Bit 1 of RP if receiving signal
				if(temp_buffer[0] == 1)
				{
					mem->memory_map[REG_RP] &= ~0x2;
					mem->ir_counter = 12672;
				}

				//Set Bit 1 of RP if IR signal is normal
				else
				{
					mem->memory_map[REG_RP] |= 0x2;
					mem->ir_counter = 0;
				}

				//Send acknowlegdement
				SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 2);

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

			if(SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 2) < 2)
			{
				std::cout<<"SIO::Error - Host failed to send data to client\n";
				sio_stat.connected = false;
				server.connected = false;
				sender.connected = false;
				return false;
			}
		}
	}

	#endif

	return true;
}

/****** Requests syncronization with another system ******/
bool DMG_SIO::request_sync()
{
	#ifdef GBE_NETPLAY

	if(sio_stat.sio_type == GB_FOUR_PLAYER_ADAPTER)
	{
		sio_stat.sync = true;
		return true;
	}

	u8 temp_buffer[2];
	temp_buffer[0] = 0;
	temp_buffer[1] = 0xFF;

	//Send the sync code 0xFF
	if(SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 2) < 2)
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
void DMG_SIO::process_network_communication()
{
	#ifdef GBE_NETPLAY

	//Process DMG-07 connections separately
	if(sio_stat.sio_type == GB_FOUR_PLAYER_ADAPTER)
	{
		four_player_process_network_communication();
		return;
	}

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

			//Set the emulated SIO device type
			if(sio_stat.sio_type != GB_FOUR_PLAYER_ADAPTER) { sio_stat.sio_type = GB_LINK; }
		}
	}

	#endif
}

/****** Processes data sent to the GB Printer ******/
void DMG_SIO::printer_process()
{
	switch(printer.current_state)
	{
		//Receive packet data
		case GBP_AWAITING_PACKET:

			//Push data to packet buffer	
			printer.packet_buffer.push_back(sio_stat.transfer_byte);
			printer.packet_size++;

			//Check for magic number 0x88 0x33
			if((printer.packet_size == 2) && (printer.packet_buffer[0] == 0x88) && (printer.packet_buffer[1] == 0x33))
			{
				//Move to the next state
				printer.current_state = GBP_RECEIVE_COMMAND;
			}

			//If magic number not found, reset
			else if(printer.packet_size == 2)
			{
				printer.packet_size = 1;
				u8 temp = printer.packet_buffer[1];
				printer.packet_buffer.clear();
				printer.packet_buffer.push_back(temp);
			}

			//Send data back to GB + IRQ
			mem->memory_map[REG_SB] = 0x0;
			mem->memory_map[IF_FLAG] |= 0x08;
			
			break;

		//Receive command
		case GBP_RECEIVE_COMMAND:

			//Push data to packet buffer
			printer.packet_buffer.push_back(sio_stat.transfer_byte);
			printer.packet_size++;

			//Grab command. Check to see if the value is a valid command
			printer.command = printer.packet_buffer.back();

			//Abort if invalid command, wait for a new packet
			if((printer.command != 0x1) && (printer.command != 0x2) && (printer.command != 0x4) && (printer.command != 0xF))
			{
				std::cout<<"SIO::Warning - Invalid command sent to GB Printer -> 0x" << std::hex << (u32)printer.command << "\n";
				printer.current_state = GBP_AWAITING_PACKET;
			}

			else
			{
				//Move to the next state
				printer.current_state = GBP_RECEIVE_COMPRESSION_FLAG;
			}

			//Send data back to GB + IRQ
			mem->memory_map[REG_SB] = 0x0;
			mem->memory_map[IF_FLAG] |= 0x08;

			break;

		//Receive compression flag
		case GBP_RECEIVE_COMPRESSION_FLAG:

			//Push data to packet buffer
			printer.packet_buffer.push_back(sio_stat.transfer_byte);
			printer.packet_size++;

			//Grab compression flag
			printer.compression_flag = printer.packet_buffer.back();

			//Move to the next state
			printer.current_state = GBP_RECEIVE_LENGTH;

			//Send data back to GB + IRQ
			mem->memory_map[REG_SB] = 0x0;
			mem->memory_map[IF_FLAG] |= 0x08;

			break;

		//Receive data length
		case GBP_RECEIVE_LENGTH:

			//Push data to packet buffer
			printer.packet_buffer.push_back(sio_stat.transfer_byte);
			printer.packet_size++;

			//Grab LSB of data length
			if(printer.packet_size == 5)
			{
				printer.data_length = 0;
				printer.data_length |= printer.packet_buffer.back();
			}

			//Grab MSB of the data length, move to the next state
			else if(printer.packet_size == 6)
			{
				printer.packet_size = 0;
				printer.data_length |= (printer.packet_buffer.back() << 8);
				
				//Receive data only if the length is non-zero
				if(printer.data_length > 0) { printer.current_state = GBP_RECEIVE_DATA; }

				//Otherwise, move on to grab the checksum
				else { printer.current_state = GBP_RECEIVE_CHECKSUM; }
			}

			//Send data back to GB + IRQ
			mem->memory_map[REG_SB] = 0x0;
			mem->memory_map[IF_FLAG] |= 0x08;

			break;

		//Receive data
		case GBP_RECEIVE_DATA:

			//Push data to packet buffer
			printer.packet_buffer.push_back(sio_stat.transfer_byte);
			printer.packet_size++;

			//Once the specified amount of data is transferred, move to the next stage
			if(printer.packet_size == printer.data_length)
			{
				printer.packet_size = 0;
				printer.current_state = GBP_RECEIVE_CHECKSUM;
			}

			//Send data back to GB + IRQ
			mem->memory_map[REG_SB] = 0x0;
			mem->memory_map[IF_FLAG] |= 0x08;

			break;

		//Receive checksum
		case GBP_RECEIVE_CHECKSUM:

			//Push data to packet buffer
			printer.packet_buffer.push_back(sio_stat.transfer_byte);
			printer.packet_size++;
			
			//Grab LSB of checksum
			if(printer.packet_size == 1)
			{
				printer.checksum = 0;
				printer.checksum |= printer.packet_buffer.back();
			}

			//Grab MSB of the checksum, move to the next state
			else if(printer.packet_size == 2)
			{
				printer.packet_size = 0;
				printer.checksum |= (printer.packet_buffer.back() << 8);
				printer.current_state = GBP_ACKNOWLEDGE_PACKET;

				u16 checksum_match = 0;

				//Calculate checksum
				for(u32 x = 2; x < (printer.packet_buffer.size() - 2); x++)
				{
					checksum_match += printer.packet_buffer[x];
				}

				if(checksum_match != printer.checksum) { printer.status |= 0x1; }
				else { printer.status &= ~0x1; }
			}

			//Send data back to GB + IRQ
			mem->memory_map[REG_SB] = 0x0;
			mem->memory_map[IF_FLAG] |= 0x08;

			break;

		//Acknowledge packet and process command
		case GBP_ACKNOWLEDGE_PACKET:
		
			//GB Printer expects 2 0x0s, only continue if given them
			if(sio_stat.transfer_byte == 0)
			{
				//Push data to packet buffer
				printer.packet_buffer.push_back(sio_stat.transfer_byte);
				printer.packet_size++;

				//Send back 0x81 to GB + IRQ on 1st 0x0
				if(printer.packet_size == 1)
				{
					mem->memory_map[REG_SB] = 0x81;
					mem->memory_map[IF_FLAG] |= 0x08;
				}

				//Send back current status to GB + IRQ on 2nd 0x0, begin processing command
				else if(printer.packet_size == 2)
				{
					printer_execute_command();

					mem->memory_map[REG_SB] = printer.status;
					mem->memory_map[IF_FLAG] |= 0x08;

					printer.packet_buffer.clear();
					printer.packet_size = 0;
				}
			}

			break;
	}
}

/****** Executes commands send to the GB Printer ******/
void DMG_SIO::printer_execute_command()
{
	switch(printer.command)
	{
		//Initialize command
		case 0x1:
			printer.status = 0x0;
			printer.strip_count = 0;

			//Clear internal scanline data
			printer.scanline_buffer.clear();
			printer.scanline_buffer.resize(0x5A00, 0x0);
			
			break;

		//Print command
		case 0x2:
			print_image();
			printer.status = 0x4;

			break;

		//Data process command
		case 0x4:
			printer_data_process();
			
			//Only set Ready-To-Print status if some actual data was received
			if(printer.strip_count != 0) { printer.status = 0x8; }

			break;

		//Status command
		case 0xF:
			printer.status |= 0;

			break;

		default:
			std::cout<<"SIO::Warning - Invalid command sent to GB Printer -> 0x" << std::hex << (u32)printer.command << "\n";
			break;
	}

	printer.current_state = GBP_AWAITING_PACKET;
}

/****** Processes dot data sent to GB Printer ******/
void DMG_SIO::printer_data_process()
{
	u32 data_pointer = 6;
	u32 pixel_counter = printer.strip_count * 2560;
	u8 tile_pixel = 0;

	if(printer.strip_count >= 9)
	{
		for(u32 x = 0; x < 2560; x++) { printer.scanline_buffer.push_back(0x0); }	
	}

	//Process uncompressed dot data
	if(!printer.compression_flag)
	{
		//Cycle through all tiles given in the data, 40 in all
		for(u32 x = 0; x < 40; x++)
		{
			//Grab 16-bytes representing each tile, 2 bytes at a time
			for(u32 y = 0; y < 8; y++)
			{
				//Move pixel counter down one row in the tile
				pixel_counter = (printer.strip_count * 2560) + ((x % 20) * 8) + (160 * y);
				if(x >= 20) { pixel_counter += 1280; }

				//Grab 2-bytes representing 8x1 section
				u16 tile_data = (printer.packet_buffer[data_pointer + 1] << 8) | printer.packet_buffer[data_pointer];
				data_pointer += 2;

				//Determine color of each pixel in that 8x1 section
				for(int z = 7; z >= 0; z--)
				{
					//Calculate raw value of the tile's pixel
					tile_pixel = ((tile_data >> 8) & (1 << z)) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << z)) ? 1 : 0;

					switch(tile_pixel)
					{
						case 0: 
							printer.scanline_buffer[pixel_counter++] = config::DMG_BG_PAL[0];
							break;

						case 1: 
							printer.scanline_buffer[pixel_counter++] = config::DMG_BG_PAL[1];
							break;

						case 2:
							printer.scanline_buffer[pixel_counter++] = config::DMG_BG_PAL[2];
							break;

						case 3: 
							printer.scanline_buffer[pixel_counter++] = config::DMG_BG_PAL[3];
							break;
					}
				}
			}
		}
	}

	//Process compressed dot data
	else
	{
		std::vector<u8> dot_data;
		u8 data = 0;

		//Cycle through all the compressed data and calculate the RLE
		while(data_pointer < (printer.data_length + 6))
		{
			//Grab MSB of 1st byte in the run, if 1 the run is compressed, otherwise it is an uncompressed run
			u8 data = printer.packet_buffer[data_pointer++];

			//Compressed run
			if(data & 0x80)
			{
				u8 length = (data & 0x7F) + 2;
				data = printer.packet_buffer[data_pointer++];

				for(u32 x = 0; x < length; x++) { dot_data.push_back(data); }
			}

			//Uncompressed run
			else
			{
				u8 length = (data & 0x7F) + 1;
				
				for(u32 x = 0; x < length; x++)
				{
					data = printer.packet_buffer[data_pointer++];
					dot_data.push_back(data);
				}
			}
		}

		data_pointer = 0;

		//Cycle through all tiles given in the data, 40 in all
		for(u32 x = 0; x < 40; x++)
		{
			//Grab 16-bytes representing each tile, 2 bytes at a time
			for(u32 y = 0; y < 8; y++)
			{
				//Move pixel counter down one row in the tile
				pixel_counter = (printer.strip_count * 2560) + ((x % 20) * 8) + (160 * y);
				if(x >= 20) { pixel_counter += 1280; }

				//Grab 2-bytes representing 8x1 section
				u16 tile_data = (dot_data[data_pointer + 1] << 8) | dot_data[data_pointer];
				data_pointer += 2;

				//Determine color of each pixel in that 8x1 section
				for(int z = 7; z >= 0; z--)
				{
					//Calculate raw value of the tile's pixel
					tile_pixel = ((tile_data >> 8) & (1 << z)) ? 2 : 0;
					tile_pixel |= (tile_data & (1 << z)) ? 1 : 0;

					switch(tile_pixel)
					{
						case 0: 
							printer.scanline_buffer[pixel_counter++] = config::DMG_BG_PAL[0];
							break;

						case 1: 
							printer.scanline_buffer[pixel_counter++] = config::DMG_BG_PAL[1];
							break;

						case 2:
							printer.scanline_buffer[pixel_counter++] = config::DMG_BG_PAL[2];
							break;

						case 3: 
							printer.scanline_buffer[pixel_counter++] = config::DMG_BG_PAL[3];
							break;
					}
				}
			}
		}
	}

	//Only increment strip count if we actually received data
	if(printer.data_length != 0) { printer.strip_count++; }
}

/****** Save GB Printer image to a BMP ******/
void DMG_SIO::print_image()
{
	u32 height = (16 * printer.strip_count);
	u32 img_size = 160 * height;

	//Set up printing palette
	u8 data_pal = printer.packet_buffer[8];

	printer.pal[0] = data_pal & 0x3;
	printer.pal[1] = (data_pal >> 2) & 0x3;
	printer.pal[2] = (data_pal >> 4) & 0x3;
	printer.pal[3] = (data_pal >> 6) & 0x3;

	srand(SDL_GetTicks());

	std::string filename = config::ss_path + "gb_print_";
	filename += util::to_str(rand() % 1024);
	filename += util::to_str(rand() % 1024);
	filename += util::to_str(rand() % 1024);
	filename += ".bmp";

	//Create a 160x144 image from the buffer, save as BMP
	SDL_Surface *print_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, 160, height, 32, 0, 0, 0, 0);

	//Lock source surface
	if(SDL_MUSTLOCK(print_screen)){ SDL_LockSurface(print_screen); }
	u32* out_pixel_data = (u32*)print_screen->pixels;

	for(u32 x = 0; x < img_size; x++)
	{
		//Convert pixels to printer palette if necessary
		u8 tile_pixel = 0;
		
		if(printer.scanline_buffer[x] == config::DMG_BG_PAL[0]) { tile_pixel = 0; }
		else if(printer.scanline_buffer[x] == config::DMG_BG_PAL[1]) { tile_pixel = 1; }
		else if(printer.scanline_buffer[x] == config::DMG_BG_PAL[2]) { tile_pixel = 2; }
		else if(printer.scanline_buffer[x] == config::DMG_BG_PAL[3]) { tile_pixel = 3; }

		switch(printer.pal[tile_pixel])
		{
			case 0: 
				printer.scanline_buffer[x] = config::DMG_BG_PAL[0];
				break;

			case 1: 
				printer.scanline_buffer[x] = config::DMG_BG_PAL[1];
				break;

			case 2:
				printer.scanline_buffer[x] = config::DMG_BG_PAL[2];
				break;

			case 3: 
				printer.scanline_buffer[x] = config::DMG_BG_PAL[3];
				break;
		}
			
		out_pixel_data[x] = printer.scanline_buffer[x];
	}

	//Unlock source surface
	if(SDL_MUSTLOCK(print_screen)){ SDL_UnlockSurface(print_screen); }

	SDL_SaveBMP(print_screen, filename.c_str());
	SDL_FreeSurface(print_screen);

	printer.strip_count = 0;

	//OSD
	config::osd_message = "SAVED GB PRINTER IMG";
	config::osd_count = 180;
}

/****** Processes data sent to the GB Mobile Adapter ******/
void DMG_SIO::mobile_adapter_process()
{
	switch(mobile_adapter.current_state)
	{
		//Receive packet data
		case GBMA_AWAITING_PACKET:
			
			//Push data to packet buffer	
			mobile_adapter.packet_buffer.push_back(sio_stat.transfer_byte);
			mobile_adapter.packet_size++;

			//Check for magic number 0x99 0x66
			if((mobile_adapter.packet_size == 2) && (mobile_adapter.packet_buffer[0] == 0x99) && (mobile_adapter.packet_buffer[1] == 0x66))
			{
				//Move to the next state
				mobile_adapter.packet_size = 0;
				mobile_adapter.current_state = GBMA_RECEIVE_HEADER;
			}

			//If magic number not found, reset
			else if(mobile_adapter.packet_size == 2)
			{
				mobile_adapter.packet_size = 1;
				u8 temp = mobile_adapter.packet_buffer[1];
				mobile_adapter.packet_buffer.clear();
				mobile_adapter.packet_buffer.push_back(temp);
			}

			//Send data back to GB + IRQ
			mem->memory_map[REG_SB] = 0x4B;
			mem->memory_map[IF_FLAG] |= 0x08;
			
			break;

		//Receive header data
		case GBMA_RECEIVE_HEADER:

			//Push data to packet buffer
			mobile_adapter.packet_buffer.push_back(sio_stat.transfer_byte);
			mobile_adapter.packet_size++;

			//Process header data
			if(mobile_adapter.packet_size == 4)
			{
				mobile_adapter.command = mobile_adapter.packet_buffer[2];
				mobile_adapter.data_length = mobile_adapter.packet_buffer[5];

				//Move to the next state
				mobile_adapter.packet_size = 0;
				mobile_adapter.current_state = (mobile_adapter.data_length == 0) ? GBMA_RECEIVE_CHECKSUM : GBMA_RECEIVE_DATA;

				std::cout<<"SIO::Mobile Adapter - Command ID 0x" << std::hex << (u32)mobile_adapter.command << "\n";
			}
			
			//Send data back to GB + IRQ
			mem->memory_map[REG_SB] = 0x4B;
			mem->memory_map[IF_FLAG] |= 0x08;
			
			break;

		//Receive transferred data
		case GBMA_RECEIVE_DATA:

			//Push data to packet buffer
			mobile_adapter.packet_buffer.push_back(sio_stat.transfer_byte);
			mobile_adapter.packet_size++;

			//Move onto the next state once all data has been received
			if(mobile_adapter.packet_size == mobile_adapter.data_length)
			{
				mobile_adapter.packet_size = 0;
				mobile_adapter.current_state = GBMA_RECEIVE_CHECKSUM;
			}

			//Send data back to GB + IRQ
			mem->memory_map[REG_SB] = 0x4B;
			mem->memory_map[IF_FLAG] |= 0x08;

			break;

		//Receive packet checksum
		case GBMA_RECEIVE_CHECKSUM:

			//Push data to packet buffer
			mobile_adapter.packet_buffer.push_back(sio_stat.transfer_byte);
			mobile_adapter.packet_size++;

			//Grab MSB of the checksum
			if(mobile_adapter.packet_size == 1)
			{
				mobile_adapter.checksum = (sio_stat.transfer_byte << 8);

				//Send data back to GB + IRQ
				mem->memory_map[REG_SB] = 0x4B;
				mem->memory_map[IF_FLAG] |= 0x08;
			}

			//Grab LSB of the checksum, move on to next state
			else if(mobile_adapter.packet_size == 2)
			{
				mobile_adapter.checksum |= sio_stat.transfer_byte;

				//Verify the checksum data
				u16 real_checksum = 0;

				for(u32 x = 2; x < (mobile_adapter.data_length + 6); x++)
				{
					real_checksum += mobile_adapter.packet_buffer[x];
				}

				//Move on to packet acknowledgement
				if(mobile_adapter.checksum == real_checksum)
				{
					mobile_adapter.packet_size = 0;
					mobile_adapter.current_state = GBMA_ACKNOWLEDGE_PACKET;

					//Send data back to GB + IRQ
					mem->memory_map[REG_SB] = 0x4B;
					mem->memory_map[IF_FLAG] |= 0x08;
				}

				//Send and error and wait for the packet to restart
				else
				{
					mobile_adapter.packet_size = 0;
					mobile_adapter.current_state = GBMA_AWAITING_PACKET;

					//Send data back to GB + IRQ
					mem->memory_map[REG_SB] = 0xF1;
					mem->memory_map[IF_FLAG] |= 0x08;

					std::cout<<"SIO::Mobile Adapter - Checksum Fail \n";
				}
			}

			break;

		//Acknowledge packet
		case GBMA_ACKNOWLEDGE_PACKET:
		
			//Push data to packet buffer
			mobile_adapter.packet_buffer.push_back(sio_stat.transfer_byte);
			mobile_adapter.packet_size++;

			//Mobile Adapter expects 0x80 from GBC, sends back 0x88 through 0x8F (does not matter which)
			if(mobile_adapter.packet_size == 1)
			{
				if(sio_stat.transfer_byte == 0x80)
				{
					mem->memory_map[REG_SB] = 0x88;
					mem->memory_map[IF_FLAG] |= 0x08;
				}

				else
				{
					mobile_adapter.packet_size = 0;
					mem->memory_map[REG_SB] = 0x4B;
					mem->memory_map[IF_FLAG] |= 0x08;
				}	
			}

			//Send back 0x80 XOR current command + IRQ on 2nd byte, begin processing command
			else if(mobile_adapter.packet_size == 2)
			{
				mem->memory_map[REG_SB] = 0x80 ^ mobile_adapter.command;
				mem->memory_map[IF_FLAG] |= 0x08;

				//Process commands sent to the mobile adapter
				switch(mobile_adapter.command)
				{

					//For certain commands, the mobile adapter echoes the packet it received
					case 0x10:
					case 0x11:
						mobile_adapter.packet_size = 0;
						mobile_adapter.current_state = GBMA_ECHO_PACKET;

						//Echo packet needs to have the proper handshake with the adapter ID and command
						mobile_adapter.packet_buffer[mobile_adapter.packet_buffer.size() - 2] = 0x88;
						mobile_adapter.packet_buffer[mobile_adapter.packet_buffer.size() - 1] = 0x00;

						//Line busy status
						mobile_adapter.line_busy = false;
						
						break;

					//Dial telephone, close telephone, or wait for telephone call 
					case 0x12:
					case 0x13:
					case 0x14:
						//Start building the reply packet - Just send back and empty body
						mobile_adapter.packet_buffer.clear();

						//Magic bytes
						mobile_adapter.packet_buffer.push_back(0x99);
						mobile_adapter.packet_buffer.push_back(0x66);

						//Header
						if(mobile_adapter.command == 0x12) { mobile_adapter.packet_buffer.push_back(0x12); }
						else if(mobile_adapter.command == 0x13) { mobile_adapter.packet_buffer.push_back(0x13); }
						else { mobile_adapter.packet_buffer.push_back(0x14); }

						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0x00);

						//Checksum
						mobile_adapter.packet_buffer.push_back(0x00);

						if(mobile_adapter.command == 0x12) { mobile_adapter.packet_buffer.push_back(0x12); }
						else if(mobile_adapter.command == 0x13) { mobile_adapter.packet_buffer.push_back(0x13); }
						else { mobile_adapter.packet_buffer.push_back(0x14); }

						//Acknowledgement handshake
						mobile_adapter.packet_buffer.push_back(0x88);
						mobile_adapter.packet_buffer.push_back(0x00);

						//Send packet back
						mobile_adapter.packet_size = 0;
						mobile_adapter.current_state = GBMA_ECHO_PACKET;

						//Line busy status
						if(mobile_adapter.command == 0x12) { mobile_adapter.line_busy = true; }
						else if(mobile_adapter.command == 0x13) { mobile_adapter.line_busy = false; }

						break;

					//TCP transfer data
					case 0x15:
						//POP data transfer
						if(mobile_adapter.port == 110) { mobile_adapter_process_pop(); }

						//HTTP data transfer
						else if(mobile_adapter.port == 80) { mobile_adapter_process_http(); }

						//Unknown port
						else { std::cout<<"SIO::Warning - Mobile Adapter accessing TCP transfer on unknown port " << mobile_adapter.port << "\n"; }

						break;

					//Telephone status
					case 0x17:
						//Start building the reply packet
						mobile_adapter.packet_buffer.clear();

						//Magic bytes
						mobile_adapter.packet_buffer.push_back(0x99);
						mobile_adapter.packet_buffer.push_back(0x66);

						//Header
						mobile_adapter.packet_buffer.push_back(0x17);
						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0x01);

						//Body
						if(mobile_adapter.line_busy) { mobile_adapter.packet_buffer.push_back(0x05); }
						else { mobile_adapter.packet_buffer.push_back(0x00); }

						//Checksum
						mobile_adapter.packet_buffer.push_back(0x00);

						if(mobile_adapter.line_busy) { mobile_adapter.packet_buffer.push_back(0x1D); }
						else { mobile_adapter.packet_buffer.push_back(0x18); }

						//Acknowledgement handshake
						mobile_adapter.packet_buffer.push_back(0x88);
						mobile_adapter.packet_buffer.push_back(0x00);

						//Send packet back
						mobile_adapter.packet_size = 0;
						mobile_adapter.current_state = GBMA_ECHO_PACKET;

						break;

					//Read configuration data
					case 0x19:
						//Grab the offset and length to read. Two bytes of data
						if(mobile_adapter.data_length >= 2)
						{
							u8 config_offset = mobile_adapter.packet_buffer[6];
							u8 config_length = mobile_adapter.packet_buffer[7];

							//Start building the reply packet
							mobile_adapter.packet_buffer.clear();

							//Magic bytes
							mobile_adapter.packet_buffer.push_back(0x99);
							mobile_adapter.packet_buffer.push_back(0x66);

							//Header
							mobile_adapter.packet_buffer.push_back(0x19);
							mobile_adapter.packet_buffer.push_back(0x00);
							mobile_adapter.packet_buffer.push_back(0x00);
							mobile_adapter.packet_buffer.push_back(config_length + 1);

							//One byte of unknown significance
							//For now, simply returning the data length, as it doesn't seem to really matter
							mobile_adapter.packet_buffer.push_back(config_length);

							//Data from 192-byte configuration memory
							for(u32 x = config_offset; x < (config_offset + config_length); x++)
							{
								if(x < 0xC0) { mobile_adapter.packet_buffer.push_back(mobile_adapter.data[x]); }
								else { std::cout<<"SIO::Error - Mobile Adapter trying to read out-of-bounds memory\n"; return; }
							}

							//Checksum
							u16 checksum = 0;
							for(u32 x = 2; x < mobile_adapter.packet_buffer.size(); x++) { checksum += mobile_adapter.packet_buffer[x]; }

							mobile_adapter.packet_buffer.push_back((checksum >> 8) & 0xFF);
							mobile_adapter.packet_buffer.push_back(checksum & 0xFF);

							//Acknowledgement handshake
							mobile_adapter.packet_buffer.push_back(0x88);
							mobile_adapter.packet_buffer.push_back(0x00);

							//Send packet back
							mobile_adapter.packet_size = 0;
							mobile_adapter.current_state = GBMA_ECHO_PACKET;
						}

						else { std::cout<<"SIO::Error - Mobile Adapter requested unspecified configuration data\n"; return; }

						break;

					//Write configuration data
					case 0x1A:
						{
							//Grab the offset and length to write. Two bytes of data
							u8 config_offset = mobile_adapter.packet_buffer[6];

							//Write data from the packet into memory configuration
							for(u32 x = 7; x < (mobile_adapter.data_length + 6); x++)
							{
								if(config_offset < 0xC0) { mobile_adapter.data[config_offset++] = mobile_adapter.packet_buffer[x]; }
								else { std::cout<<"SIO::Error - Mobile Adapter trying to write out-of-bounds memory\n"; return; }
							}

							//Start building the reply packet - Empty body
							mobile_adapter.packet_buffer.clear();

							//Magic bytes
							mobile_adapter.packet_buffer.push_back(0x99);
							mobile_adapter.packet_buffer.push_back(0x66);

							//Header
							mobile_adapter.packet_buffer.push_back(0x1A);
							mobile_adapter.packet_buffer.push_back(0x00);
							mobile_adapter.packet_buffer.push_back(0x00);
							mobile_adapter.packet_buffer.push_back(0x00);

							//Checksum
							u16 checksum = 0;
							for(u32 x = 2; x < mobile_adapter.packet_buffer.size(); x++) { checksum += mobile_adapter.packet_buffer[x]; }

							mobile_adapter.packet_buffer.push_back((checksum >> 8) & 0xFF);
							mobile_adapter.packet_buffer.push_back(checksum & 0xFF);

							//Acknowledgement handshake
							mobile_adapter.packet_buffer.push_back(0x88);
							mobile_adapter.packet_buffer.push_back(0x00);

							//Send packet back
							mobile_adapter.packet_size = 0;
							mobile_adapter.current_state = GBMA_ECHO_PACKET;
						}
						
						break;

					//ISP Login
					case 0x21:
						//GBE+ doesn't care about the data sent to the emulated Mobile Adapter (not yet anyway)
						//Just return any random IP address as a response, e.g. localhost

						//Start building the reply packet
						mobile_adapter.packet_buffer.clear();

						//Magic bytes
						mobile_adapter.packet_buffer.push_back(0x99);
						mobile_adapter.packet_buffer.push_back(0x66);

						//Header
						mobile_adapter.packet_buffer.push_back(0x21);
						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0x04);

						//Body - 127.0.0.1
						mobile_adapter.packet_buffer.push_back(0x7F);
						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0x01);

						//Checksum
						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0xA5);

						//Acknowledgement handshake
						mobile_adapter.packet_buffer.push_back(0x88);
						mobile_adapter.packet_buffer.push_back(0x00);

						//Send packet back
						mobile_adapter.packet_size = 0;
						mobile_adapter.current_state = GBMA_ECHO_PACKET;

						break;

					//ISP Logout
					case 0x22:
						//Start building the reply packet - Empty body
						mobile_adapter.packet_buffer.clear();

						//Magic bytes
						mobile_adapter.packet_buffer.push_back(0x99);
						mobile_adapter.packet_buffer.push_back(0x66);

						//Header
						mobile_adapter.packet_buffer.push_back(0x22);
						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0x00);

						//Checksum
						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0x22);

						//Acknowledgement handshake
						mobile_adapter.packet_buffer.push_back(0x88);
						mobile_adapter.packet_buffer.push_back(0x00);

						//Send packet back
						mobile_adapter.packet_size = 0;
						mobile_adapter.current_state = GBMA_ECHO_PACKET;

						break;

					//TCP Open
					case 0x23:
						//GBE+ doesn't care about the data sent to the emulated Mobile Adapter (not yet anyway)
						//Reply body has one byte of unknown significance, can probably be something random
						//Command ID Bit 7 is set

						//Grab the IP address (4 bytes) and the port (2 bytes)
						if(mobile_adapter.data_length >= 6)
						{
							mobile_adapter.ip_addr = 0;
							mobile_adapter.ip_addr |= (mobile_adapter.packet_buffer[6] << 24);
							mobile_adapter.ip_addr |= (mobile_adapter.packet_buffer[7] << 16);
							mobile_adapter.ip_addr |= (mobile_adapter.packet_buffer[8] << 8);
							mobile_adapter.ip_addr |= mobile_adapter.packet_buffer[9];

							mobile_adapter.port = 0;
							mobile_adapter.port |= (mobile_adapter.packet_buffer[10] << 8);
							mobile_adapter.port |= mobile_adapter.packet_buffer[11];
						}

						else
						{
							std::cout<<"SIO::Error - GB Mobile Adapter tried opening a TCP connection without a server address or port\n";
							return;
						}

						//Start building the reply packet
						mobile_adapter.packet_buffer.clear();

						//Magic bytes
						mobile_adapter.packet_buffer.push_back(0x99);
						mobile_adapter.packet_buffer.push_back(0x66);

						//Header
						mobile_adapter.packet_buffer.push_back(0xA3);
						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0x01);

						//Body - Random byte
						mobile_adapter.packet_buffer.push_back(0x77);

						//Checksum
						mobile_adapter.packet_buffer.push_back(0x01);
						mobile_adapter.packet_buffer.push_back(0x1B);

						//Acknowledgement handshake
						mobile_adapter.packet_buffer.push_back(0x88);
						mobile_adapter.packet_buffer.push_back(0x00);

						//Send packet back
						mobile_adapter.packet_size = 0;
						mobile_adapter.current_state = GBMA_ECHO_PACKET;

						break;

					//TCP Close
					case 0x24:
						//GBE+ doesn't care about the data sent to the emulated Mobile Adapter (not yet anyway)
						//Reply body has one byte of unknown significance, can probably be something random

						//Start building the reply packet
						mobile_adapter.packet_buffer.clear();

						//Magic bytes
						mobile_adapter.packet_buffer.push_back(0x99);
						mobile_adapter.packet_buffer.push_back(0x66);

						//Header
						mobile_adapter.packet_buffer.push_back(0x24);
						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0x01);

						//Body - Random byte
						mobile_adapter.packet_buffer.push_back(0x77);

						//Checksum
						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0x9C);

						//Acknowledgement handshake
						mobile_adapter.packet_buffer.push_back(0x88);
						mobile_adapter.packet_buffer.push_back(0x00);

						//Send packet back
						mobile_adapter.packet_size = 0;
						mobile_adapter.current_state = GBMA_ECHO_PACKET;
						mobile_adapter.http_data = "";

						//Reset all sessions
						mobile_adapter.pop_session_started = false;
						mobile_adapter.http_session_started = false;

						break;

					//DNS Query
					case 0x28:
						//GBE+ doesn't care about the data sent to the emulated Mobile Adapter (not yet anyway)
						//Just return any random IP address as a response, e.g. 8.8.8.8

						//Start building the reply packet - Empty body
						mobile_adapter.packet_buffer.clear();

						//Magic bytes
						mobile_adapter.packet_buffer.push_back(0x99);
						mobile_adapter.packet_buffer.push_back(0x66);

						//Header
						mobile_adapter.packet_buffer.push_back(0x28);
						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0x04);

						//Body - 8.8.8.8
						mobile_adapter.packet_buffer.push_back(0x08);
						mobile_adapter.packet_buffer.push_back(0x08);
						mobile_adapter.packet_buffer.push_back(0x08);
						mobile_adapter.packet_buffer.push_back(0x08);

						//Checksum
						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0x4C);

						//Acknowledgement handshake
						mobile_adapter.packet_buffer.push_back(0x88);
						mobile_adapter.packet_buffer.push_back(0x00);

						//Send packet back
						mobile_adapter.packet_size = 0;
						mobile_adapter.current_state = GBMA_ECHO_PACKET;

						break;

					default:
						std::cout<<"SIO::Mobile Adapter - Unknown Command ID 0x" << std::hex << (u32)mobile_adapter.command << "\n";
						mobile_adapter.packet_buffer.clear();
						mobile_adapter.packet_size = 0;
						mobile_adapter.current_state = GBMA_AWAITING_PACKET;

						break;
				}
			}

			break;

		//Echo packet back to Game Boy
		case GBMA_ECHO_PACKET:

			//Check for communication errors
			switch(sio_stat.transfer_byte)
			{
				case 0xEE:
				case 0xF0:
				case 0xF1:
				case 0xF2:
					std::cout<<"SIO::Error - GB Mobile Adapter communication failure code 0x" << std::hex << (u32)sio_stat.transfer_byte << "\n";
					break;
			}

			//Send back the packet bytes
			if(mobile_adapter.packet_size < mobile_adapter.packet_buffer.size())
			{
				mem->memory_map[REG_SB] = mobile_adapter.packet_buffer[mobile_adapter.packet_size++];
				mem->memory_map[IF_FLAG] |= 0x08;

				if(mobile_adapter.packet_size == mobile_adapter.packet_buffer.size())
				{
					mobile_adapter.packet_buffer.clear();
					mobile_adapter.packet_size = 0;
					mobile_adapter.current_state = GBMA_AWAITING_PACKET;
				}
			}

			break;
	}
}

/****** Processes POP transfers from the emulated GB Mobile Adapter ******/
void DMG_SIO::mobile_adapter_process_pop()
{
	//For now, just initiate a POP session, but return errors for everything after that
	std::string pop_response = "";
	std::string pop_data = util::data_to_str(mobile_adapter.packet_buffer.data(), mobile_adapter.packet_buffer.size());
	u8 response_id = 0;
	u8 pop_command = 0xFF;

	for(u32 x = 0; x < mobile_adapter.packet_buffer.size(); x++)
	{
		std::cout<<"POP DAT " << std::dec << x << " --> 0x" << std::hex << (u32)mobile_adapter.packet_buffer[x] << " :: " <<  "\n";
	}

	std::size_t user_match = pop_data.find("USER");
	std::size_t pass_match = pop_data.find("PASS");
	std::size_t quit_match = pop_data.find("QUIT");
	std::size_t stat_match = pop_data.find("STAT");
	std::size_t top_match = pop_data.find("TOP");

	//Check POP command
	if(user_match != std::string::npos) { pop_command = 1; }
	else if(pass_match != std::string::npos) { pop_command = 2; }
	else if(quit_match != std::string::npos) { pop_command = 3; }
	else if(stat_match != std::string::npos) { pop_command = 4; }
	else if(top_match != std::string::npos) { pop_command = 5; }

	//Check for POP initiation
	else if((mobile_adapter.data_length == 1) && (!mobile_adapter.pop_session_started))
	{
		mobile_adapter.pop_session_started = true;
		pop_command = 0;
	}

	//Check for POP end
	else if((mobile_adapter.data_length == 1) && (mobile_adapter.pop_session_started))
	{
		mobile_adapter.pop_session_started = false;
		pop_command = 6;
	}

	switch(pop_command)
	{
		//Init + USER, PASS, and QUIT commands
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
			pop_response = "+OK\r\n";
			response_id = 0x95;
			break;

		//STAT command
		case 0x4:
			//When not connecting a real server, fake an inbox with 1 message
			pop_response = "+OK 1 10\r\n";
			response_id = 0x95;
			break;

		//TOP command
		case 0x5:
			//When not connecting to a real server, fake a message
			pop_response = "+OK 1 Hello GBE+";
			response_id = 0x95;
			break;

		//End
		case 0x6:
			pop_response = "+OK\r\n";
			response_id = 0x9F;
			break;

		//Error
		default:
			pop_response = "-ERR\r\n";
			response_id = 0x95;
			break;
	}

	//Start building the reply packet
	mobile_adapter.packet_buffer.clear();
	mobile_adapter.packet_buffer.resize(7 + pop_response.size(), 0x00);

	//Magic bytes
	mobile_adapter.packet_buffer[0] = (0x99);
	mobile_adapter.packet_buffer[1] = (0x66);

	//Header
	mobile_adapter.packet_buffer[2] = response_id;
	mobile_adapter.packet_buffer[3] = (0x00);
	mobile_adapter.packet_buffer[4] = (0x00);
	mobile_adapter.packet_buffer[5] = pop_response.size() + 1;

	//Body
	util::str_to_data(mobile_adapter.packet_buffer.data() + 7, pop_response);

	//Checksum
	u16 checksum = 0;
	for(u32 x = 2; x < mobile_adapter.packet_buffer.size(); x++) { checksum += mobile_adapter.packet_buffer[x]; }

	mobile_adapter.packet_buffer.push_back((checksum >> 8) & 0xFF);
	mobile_adapter.packet_buffer.push_back(checksum & 0xFF);

	//Acknowledgement handshake
	mobile_adapter.packet_buffer.push_back(0x88);
	mobile_adapter.packet_buffer.push_back(0x00);

	//Send packet back
	mobile_adapter.packet_size = 0;
	mobile_adapter.current_state = GBMA_ECHO_PACKET;	
}

/****** Processes HTTP transfers from the emulated GB Mobile Adapter ******/
void DMG_SIO::mobile_adapter_process_http()
{
	std::string http_response = "";
	u8 response_id = 0;
	bool not_found = true;

	//Send empty body until HTTP request is finished transmitting
	if(mobile_adapter.data_length != 1)
	{
		mobile_adapter.http_data += util::data_to_str(mobile_adapter.packet_buffer.data() + 7, mobile_adapter.packet_buffer.size() - 7);
		http_response = "";
		response_id = 0x95;
	}

	//Process HTTP request once initial line + headers + message body have been received.
	else
	{
		//Update transfer status
		if(!mobile_adapter.transfer_state) { mobile_adapter.transfer_state = 1; }

		//Determine if this request is GET or POST
		std::size_t get_match = mobile_adapter.http_data.find("GET");
		std::size_t post_match = mobile_adapter.http_data.find("POST");

		//Process GET requests
		if(get_match != std::string::npos)
		{
			//See if this is the homepage for Mobile Trainer
			if(mobile_adapter.http_data.find("/01/CGB-B9AJ/index.html") != std::string::npos) { not_found = false; }
		}

		//Respond to HTTP request
		switch(mobile_adapter.transfer_state)
		{
			//Status - 200 or 404
			case 0x1:
				if(not_found) { http_response = "HTTP/1.0 404 Not Found\r\n"; }
				else { http_response = "HTTP/1.0 200 OK\r\n"; }

				mobile_adapter.transfer_state = 2;
				response_id = 0x95;

				break;

			//Header or close connection if 404
			case 0x2:
				if(not_found)
				{
					http_response = "";
					response_id = 0x9F;
					mobile_adapter.transfer_state = 0;
					mobile_adapter.http_data = "";
				}

				else
				{
					http_response = "Content-Type: text/html\r\n\r\n";
					response_id = 0x95;
					mobile_adapter.transfer_state = 3;
				}

				break;

			//HTTP data payload
			//TODO - Remove hardcoding for Mobile Trainer
			case 0x3:
				http_response = "<html><body>Hello World</body></html>";
				response_id = 0x95;
				mobile_adapter.transfer_state = 4;
				break;

			//Close connection
			case 0x4:
				http_response = "";
				response_id = 0x9F;
				mobile_adapter.transfer_state = 0;
				mobile_adapter.http_data = "";
				break;
		}
	}

	//Start building the reply packet
	mobile_adapter.packet_buffer.clear();
	mobile_adapter.packet_buffer.resize(7 + http_response.size(), 0x00);

	//Magic bytes
	mobile_adapter.packet_buffer[0] = 0x99;
	mobile_adapter.packet_buffer[1] = 0x66;

	//Header
	mobile_adapter.packet_buffer[2] = response_id;
	mobile_adapter.packet_buffer[3] = 0x00;
	mobile_adapter.packet_buffer[4] = 0x00;
	mobile_adapter.packet_buffer[5] = http_response.size() + 1;

	//Body
	util::str_to_data(mobile_adapter.packet_buffer.data() + 7, http_response);

	//Checksum
	u16 checksum = 0;
	for(u32 x = 2; x < mobile_adapter.packet_buffer.size(); x++) { checksum += mobile_adapter.packet_buffer[x]; }

	mobile_adapter.packet_buffer.push_back((checksum >> 8) & 0xFF);
	mobile_adapter.packet_buffer.push_back(checksum & 0xFF);

	//Acknowledgement handshake
	mobile_adapter.packet_buffer.push_back(0x88);
	mobile_adapter.packet_buffer.push_back(0x00);

	//Send packet back
	mobile_adapter.packet_size = 0;
	mobile_adapter.current_state = GBMA_ECHO_PACKET;
}

/****** Processes Bardigun barcode data sent to the Game Boy ******/
void DMG_SIO::bardigun_process()
{
	switch(bardigun_scanner.current_state)
	{
		//Send 0x0 while scanner is inactive
		case BARDIGUN_INACTIVE:
			mem->memory_map[REG_SB] = 0x0;
			mem->memory_map[IF_FLAG] |= 0x08;
			bardigun_scanner.inactive_counter--;

			//Send barcode data after waiting a bit
			if(!bardigun_scanner.inactive_counter)
			{
				bardigun_scanner.inactive_counter = 0x500;

				if(bardigun_scanner.data.size() != 0)
				{
					bardigun_scanner.current_state = BARDIGUN_SEND_BARCODE;
					bardigun_scanner.barcode_pointer = 0;
				}
			}
			
			break;

		//Send barcode data
		case BARDIGUN_SEND_BARCODE:
			mem->memory_map[REG_SB] = bardigun_scanner.data[bardigun_scanner.barcode_pointer++];
			mem->memory_map[IF_FLAG] |= 0x08;

			if(bardigun_scanner.barcode_pointer == bardigun_scanner.data.size())
			{
				bardigun_scanner.current_state = BARDIGUN_INACTIVE;

				//OSD
				config::osd_message = "BARCODE SWIPED";
				config::osd_count = 180;
			}

			break;
	}
}		

/****** Loads a Bardigun barcode file ******/
bool DMG_SIO::bardigun_load_barcode(std::string filename)
{
	std::ifstream barcode(filename.c_str(), std::ios::binary);

	if(!barcode.is_open()) 
	{ 
		std::cout<<"SIO::Bardigun barcode data could not be read. Check file path or permissions. \n";
		return false;
	}

	//Get file size
	barcode.seekg(0, barcode.end);
	u32 barcode_size = barcode.tellg();
	barcode.seekg(0, barcode.beg);

	bardigun_scanner.data.resize(barcode_size, 0x0);

	u8* ex_data = &bardigun_scanner.data[0];

	barcode.read((char*)ex_data, barcode_size); 
	barcode.close();

	std::cout<<"SIO::Loaded Bardigun barcode data.\n";
	return true;
}

/****** Processes Barcode Boy barcode data sent to the Game Boy ******/
void DMG_SIO::barcode_boy_process()
{
	//Handle handshake bytes
	if((barcode_boy.current_state != BARCODE_BOY_INACTIVE) && ((sio_stat.transfer_byte == 0x10) || (sio_stat.transfer_byte == 0x7)))
	{
		//Ignore additional handshake bytes after sending handshake to DMG
		if(barcode_boy.current_state == BARCODE_BOY_ACTIVE)
		{
			mem->memory_map[REG_SB] = 0xFF;
			mem->memory_map[IF_FLAG] |= 0x08;
			return;
		}

		//Switch to inactive mode and redo handshake
		else if(barcode_boy.current_state == BARCODE_BOY_FINISH)
		{
			barcode_boy.current_state = BARCODE_BOY_INACTIVE;
			barcode_boy.counter = 0;
			barcode_boy.send_data = false;
		}
	}

	switch(barcode_boy.current_state)
	{
		//Confirm handshake with Game Boy
		case BARCODE_BOY_INACTIVE:

			if(barcode_boy.counter < 2)
			{
				mem->memory_map[REG_SB] = 0xFF;
				mem->memory_map[IF_FLAG] |= 0x08;
				barcode_boy.counter++;
			}	

			else if((barcode_boy.counter >= 2) && (barcode_boy.counter < 4))
			{
				if((sio_stat.transfer_byte == 0x10) && (barcode_boy.counter == 2))
				{
					mem->memory_map[REG_SB] = 0x10;
					mem->memory_map[IF_FLAG] |= 0x08;
					barcode_boy.counter++;
				}

				else if((sio_stat.transfer_byte == 0x7) && (barcode_boy.counter == 3))
				{
					mem->memory_map[REG_SB] = 0x7;
					mem->memory_map[IF_FLAG] |= 0x08;
					barcode_boy.counter++;
				}

				else
				{
					mem->memory_map[REG_SB] = 0x0;
					mem->memory_map[IF_FLAG] |= 0x08;
					barcode_boy.counter = 0;
				}
			}

			if(barcode_boy.counter == 4)
			{
				barcode_boy.current_state = BARCODE_BOY_ACTIVE;
				barcode_boy.counter = 0;
			}

			break;

		//Send Barcode data back to Game Boy
		case BARCODE_BOY_SEND_BARCODE:

			//Signal transmission start to Game Boy (1st barcode number)
			if(barcode_boy.counter == 0)
			{
				barcode_boy.byte = 0x2;
				barcode_boy.send_data = true;
				barcode_boy.counter++;			
			}

			//Send 13 digit JAN number
			else if((barcode_boy.counter > 0) && (barcode_boy.counter < 14))
			{
				barcode_boy.byte = barcode_boy.data[barcode_boy.counter - 1];
				barcode_boy.send_data = true;
				barcode_boy.counter++;
			}

			//Signal transmission stop to Game Boy (1st barcode number)
			else if(barcode_boy.counter == 14)
			{
				barcode_boy.byte = 0x3;
				barcode_boy.send_data = true;
				barcode_boy.counter++;
			}

			//Signal transmission start to Game Boy (2nd barcode number)
			else if(barcode_boy.counter == 15)
			{
				barcode_boy.byte = 0x2;
				barcode_boy.send_data = true;
				barcode_boy.counter++;
			}

			//Send 13 digit JAN number (again)
			else if((barcode_boy.counter > 15) && (barcode_boy.counter < 29))
			{
				barcode_boy.byte = barcode_boy.data[barcode_boy.counter - 16];
				barcode_boy.send_data = true;
				barcode_boy.counter++;
			}

			//Signal transmission stop to Game Boy (2nd barcode number)
			else if(barcode_boy.counter == 29)
			{
				barcode_boy.byte = 0x3;
				barcode_boy.send_data = true;
				barcode_boy.counter = 0;
				barcode_boy.current_state = BARCODE_BOY_FINISH;

				//OSD
				config::osd_message = "BARCODE SWIPED";
				config::osd_count = 180;
			}

			sio_stat.shifts_left = 8;
			sio_stat.shift_counter = 0;

			break;
	}
}

/****** Loads a Barcode Boy barcode file ******/
bool DMG_SIO::barcode_boy_load_barcode(std::string filename)
{
	std::ifstream barcode(filename.c_str(), std::ios::binary);

	if(!barcode.is_open()) 
	{ 
		std::cout<<"SIO::Barcode Boy barcode data could not be read. Check file path or permissions. \n";
		return false;
	}

	barcode_boy.data.resize(13, 0x0);

	u8* ex_data = &barcode_boy.data[0];

	barcode.read((char*)ex_data, 13); 
	barcode.close();

	std::cout<<"SIO::Loaded Barcode Boy barcode data.\n";
	return true;
}
