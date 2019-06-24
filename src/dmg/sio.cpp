// GB Enhanced Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : sio.cpp
// Date : April 30, 2016
// Description : Game Boy Serial Input-Output emulation
//
// Sets up SDL networking
// Emulates Gameboy-to-Gameboy data transfers
// Emulates various SIO devices like GB Printer and more

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

	//Load Mobile Adapter data + internal server list
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

		mobile_adapter_load_server_list();
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
		case 5:
			sio_stat.sio_type = GB_BARCODE_BOY;
			break;

		//4 Player Adapter
		case 6:
			sio_stat.sio_type = GB_FOUR_PLAYER_ADAPTER;
			break;

		//Power Antenna
		case 13:
			sio_stat.sio_type = GB_POWER_ANTENNA;
			break;

		//Singer IZEK 1500
		case 14:
			sio_stat.sio_type = GB_SINGER_IZEK;
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
		case 1:
			sio_stat.ir_type = GBC_FULL_CHANGER;
			break;

		//Pokemon Pikachu 2
		case 2:
			sio_stat.ir_type = GBC_POKEMON_PIKACHU_2;
			break;

		//Pocket Sakura
		case 3:
			sio_stat.ir_type = GBC_POCKET_SAKURA;
			break;

		//TV Remote
		case 4:
			sio_stat.ir_type = GBC_TV_REMOTE;
			break;

		//Constant Light Source
		case 5:
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
	mobile_adapter.net_data.clear();
	mobile_adapter.packet_size = 0;
	mobile_adapter.current_state = GBMA_AWAITING_PACKET;
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

	//Power Antenna
	power_antenna_on = false;

	//Singer IZEK 1500
	singer_izek.data.clear();
	singer_izek.x_plot.clear();
	singer_izek.y_plot.clear();
	singer_izek.stitch_buffer.clear();
	singer_izek.next_x_plot = 0;
	singer_izek.next_y_plot = 0;
	singer_izek.start_flag = 0;
	singer_izek.plot_count = 0;
	singer_izek.current_state = SINGER_PING;
	singer_izek.counter = 0;

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

/****** Processes data sent from the Singer Izek to the Game Boy ******/
void DMG_SIO::singer_izek_process()
{
	std::cout<<"0x" << (u16)sio_stat.transfer_byte << "\n";

	switch(singer_izek.current_state)
	{
		case SINGER_PING:
			//Wait for new start flag to appear
			if(sio_stat.transfer_byte == 0x86) { singer_izek.counter++; }

			//Wait 1 transfer after new start flag to switch back to data mode
			else if(singer_izek.counter)
			{
				singer_izek.current_state = SINGER_SEND_DATA;
				singer_izek.counter = 0;
			}

			break;

		case SINGER_SEND_DATA:
			singer_izek.counter++;

			//Grab number of plot points
			if(((singer_izek.counter == 4 || singer_izek.counter == 5)) && (sio_stat.transfer_byte))
			{
				singer_izek.plot_count = sio_stat.transfer_byte;
				singer_izek.next_x_plot = 0;
				singer_izek.next_y_plot = 0;
			}


			//Grab first X stitch offset
			else if(((singer_izek.counter == 6) || (singer_izek.counter == 7)) && (sio_stat.transfer_byte))
			{
				singer_izek.x_plot.clear();
				singer_izek.x_plot.push_back(sio_stat.transfer_byte);
			}

			//Grab first Y stitch offset
			else if(((singer_izek.counter == 8) || (singer_izek.counter == 9)) && (sio_stat.transfer_byte))
			{
				singer_izek.y_plot.clear();
				singer_izek.y_plot.push_back(sio_stat.transfer_byte);
			}

			//Grab stitch start flag
			else if(((singer_izek.counter == 10 || singer_izek.counter == 11)) && (sio_stat.transfer_byte))
			{
				singer_izek.start_flag = sio_stat.transfer_byte;
			}

			//Grab additional stitch offsets if necessary
			else if((singer_izek.plot_count) && (sio_stat.transfer_byte))
			{
				if(!singer_izek.next_x_plot)
				{
					singer_izek.next_x_plot = singer_izek.counter;
					singer_izek.next_y_plot = singer_izek.counter + 1;
				}
					

				if(sio_stat.transfer_byte == 0xBC) { singer_izek.plot_count = 0; }

				//Additional X stitch offset
				else if(singer_izek.counter == singer_izek.next_x_plot)
				{
					singer_izek.x_plot.push_back(sio_stat.transfer_byte);
				}

				//Additional Y stitch offset
				else if(singer_izek.counter == singer_izek.next_y_plot)
				{
					singer_izek.y_plot.push_back(sio_stat.transfer_byte);
					singer_izek.next_x_plot = singer_izek.counter + 1;
					singer_izek.next_y_plot = singer_izek.counter + 2;
				}
			}

			//Exit data transmission and enter status mode
			else if(singer_izek.counter == 128)
			{
				singer_izek.counter = 0;
				singer_izek.current_state = SINGER_PING;

				singer_izek_fill_buffer();
			}

			break;
	}

	//Respond with 0x00 for external clock transfers
	//Respond with 0xFF for internal clock transfers
	if(!sio_stat.internal_clock) { mem->memory_map[REG_SB] = 0x00; }
	else { mem->memory_map[REG_SB] = 0xFF; }

	mem->memory_map[IF_FLAG] |= 0x08;
}

/****** Applies stitch coordinates to the stitch buffer ******/
void DMG_SIO::singer_izek_fill_buffer()
{
	//Use plots to create stitch buffer for visual output
	singer_izek.stitch_buffer.clear();
	singer_izek.stitch_buffer.resize(0x5A00, 0xFFFFFFFF);

	u32 x_base = 1;
	u32 y_base = 32;

	singer_izek.current_x = x_base + singer_izek.x_plot[0];
	singer_izek.current_y = y_base;

	singer_izek.last_x = 0;
	singer_izek.last_y = 0;

	singer_izek.diagonal = true;

	for(u32 i = 0; i < singer_izek.x_plot.size(); i++)
	{
		switch(singer_izek.start_flag)
		{
			//Custom stitching + Letters
			case 0xC1:
				break;

			//Generally stitching that goes in all directions
			case 0xC2:
				singer_izek_stitch_c2(i);
				break;

			//Generally stitching that goes from left to right and downwards
			case 0xC3:
				singer_izek_stitch_c3(i);
				break;

			//Stitching that goes down in a straight line
			case 0xC4:
				singer_izek_stitch_c4(i);
				break;

			//Unknown stitch type
			default:
				std::cout<<"SIO::Error - Unknown IZEK 1500 or JN-100 Stitch Type\n";
				return;
		}

		//Draw line between stitch coordinates
		if(i >= 1) { singer_izek_draw_line(); }

		singer_izek.last_x = singer_izek.current_x;
		singer_izek.last_y = singer_izek.current_y;
	}
}

/****** Plots points for C2 stitching ******/
void DMG_SIO::singer_izek_stitch_c2(u8 index)
{
	//X0 = Current X, X1 = Previous X
	u8 x0 = singer_izek.x_plot[index];
	u8 x1 = (index >= 1) ? singer_izek.x_plot[index-1] : 0;
	u8 x2 = ((index + 1) < singer_izek.x_plot.size()) ? singer_izek.x_plot[index+1] : singer_izek.x_plot[0];

	//Y0 = Current Y, Y1 = Previous Y, Y2 = Next Y 
	u8 y0 = singer_izek.y_plot[index];
	u8 y1 = (index >= 1) ? singer_izek.y_plot[index-1] : 0;
	u8 y2 = ((index + 1) < singer_izek.y_plot.size()) ? singer_izek.y_plot[index+1] : singer_izek.y_plot[0];

	//Vertical Line - X0 == X1
	if((x0 == x1) && (x1))
	{
		//Go up
		if(singer_izek.y_plot[index] < singer_izek.y_plot[index-1]) { singer_izek.current_y -= (0x1C - singer_izek.y_plot[index]); }

		//Go down
		else { singer_izek.current_y += (singer_izek.y_plot[index] - 1); }

		singer_izek.diagonal = false;
	}

	//Horizontal Line - X0 != X1 AND Y0 != Y1
	if((x0 != x1) && (x0 == x2) && (x1) && (x2) && (!singer_izek.diagonal))
	{
		singer_izek.current_x = singer_izek.x_plot[index];
	}

	//Diagonal Line - X0 != X1 AND Y0 == Y1
	else if(((x0 != x1) && (x1)) || (singer_izek.diagonal))
	{
		singer_izek.current_x = singer_izek.x_plot[index];

		//Go up
		if(singer_izek.y_plot[index] < singer_izek.y_plot[index-1]) { singer_izek.current_y -= (0x1C - singer_izek.y_plot[index]); }

		//Go down
		else { singer_izek.current_y += (singer_izek.y_plot[index] - 1); }

		singer_izek.diagonal = (singer_izek.diagonal) ? false : true;
	}
}

/****** Plots points for C3 stitching ******/
void DMG_SIO::singer_izek_stitch_c3(u8 index)
{
	//X0 = Current X, X1 = Previous X
	u8 x0 = singer_izek.x_plot[index];
	u8 x1 = (index >= 1) ? singer_izek.x_plot[index-1] : 0;

	//Y0 = Current Y, Y1 = Previous Y, Y2 = Next Y 
	u8 y0 = singer_izek.y_plot[index];
	u8 y1 = (index >= 1) ? singer_izek.y_plot[index-1] : 0;
	u8 y2 = ((index + 1) < singer_izek.y_plot.size()) ? singer_izek.y_plot[index+1] : 0;

	//Vertical Line - X0 == X1 AND Y0 == Y1
	if((x0 == x1) && (y0 == y1) && (x1) && (y1))
	{
		singer_izek.current_y += (singer_izek.y_plot[index] - 1);
	}

	//Horizontal - X0 != X1 AND Y0 == Y2 AND Y0 != Y1
	else if((x0 != x1) && (y0 == y2) && (y0 != y1) && (x1) && (y1) && (y2))
	{
		singer_izek.current_x = singer_izek.x_plot[index];
	}

	//Diagonal - X0 != X1
	else if((x0 != x1) && (x1))
	{
		singer_izek.current_x = singer_izek.x_plot[index];
		singer_izek.current_y += (singer_izek.y_plot[index] - 1);
	}
}

/****** Plots points for C4 stitching ******/
void DMG_SIO::singer_izek_stitch_c4(u8 index)
{
	//Vertical Line
	if(index >= 1) { singer_izek.current_y += (singer_izek.y_plot[index] - 1); }
}		

/****** Draws a line in the stitch buffer between 2 points ******/
void DMG_SIO::singer_izek_draw_line()
{
	s32 x_dist = (singer_izek.current_x - singer_izek.last_x);
	s32 y_dist = (singer_izek.current_y - singer_izek.last_y);
	float x_inc = 0.0;
	float y_inc = 0.0;
	float x_coord = singer_izek.last_x;
	float y_coord = singer_izek.last_y;

	s32 xy_start = 0;
	s32 xy_end = 0;
	s32 xy_inc = 0;
	
	u32 buffer_pos = 0;

	if((x_dist != 0) && (y_dist != 0))
	{
		float s = (y_dist / x_dist);
		if(s < 0.0) { s *= -1.0; }

		//Steep slope, Y = 1
		if(s > 1.0)
		{
			y_inc = (y_dist > 0) ? 1.0 : -1.0;
			x_inc = float(x_dist) / float(y_dist);
					
			if((x_dist < 0) && (x_inc > 0)) { x_inc *= -1.0; }
			else if((x_dist > 0) && (x_inc < 0)) { x_inc *= -1.0; }

			xy_start = singer_izek.last_y;
			xy_end = singer_izek.current_y;
		}

		//Gentle slope, X = 1
		else
		{
			x_inc = (x_dist > 0) ? 1.0 : -1.0;
			y_inc = float(y_dist) / float(x_dist);

			if((y_dist < 0) && (y_inc > 0)) { y_inc *= -1.0; }
			else if((y_dist > 0) && (y_inc < 0)) { y_inc *= -1.0; }

			xy_start = singer_izek.last_x;
			xy_end = singer_izek.current_x;
		}
	}

	else if(x_dist == 0)
	{
		x_inc = 0.0;
		y_inc = (y_dist > 0) ? 1.0 : -1.0;

		xy_start = singer_izek.last_y;
		xy_end = singer_izek.current_y;
	}

	else if(y_dist == 0)
	{
		x_inc = (x_dist > 0) ? 1.0 : -1.0;
		y_inc = 0.0;

		xy_start = singer_izek.last_x;
		xy_end = singer_izek.current_x;
	}

	xy_inc = (xy_start < xy_end) ? 1 : -1;
	xy_end += (xy_inc > 0) ? 1 : -1;

	while(xy_start != xy_end)
	{
		//Only draw on-screen objects
		if((x_coord >= 0) && (x_coord <= 160) && (y_coord >= 0) && (y_coord <= 144))
		{
			//Convert plot points to buffer index
			buffer_pos = (round(y_coord) * 160) + round(x_coord);
			singer_izek.stitch_buffer[buffer_pos] = 0xFF000000;
		}

		x_coord += x_inc;
		y_coord += y_inc;
		xy_start += xy_inc;
	}
}
	