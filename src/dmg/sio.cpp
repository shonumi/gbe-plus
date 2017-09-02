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

#include "sio.h"
#include "common/util.h"

/****** SIO Constructor ******/
DMG_SIO::DMG_SIO()
{
	network_init = false;

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
		//Update 4 Player status
		four_player.status &= ~0xF0;
		four_player_send_byte();

		//Send disconnect byte to another system first
		u8 temp_buffer[2];
		temp_buffer[0] = 0;
		temp_buffer[1] = 0x80;
		
		SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 2);

		SDLNet_TCP_DelSocket(tcp_sockets, sender.host_socket);
		SDLNet_TCP_Close(sender.host_socket);
	}

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
	if(SDLNet_ResolveHost(&sender.host_ip, config::netplay_client_ip.c_str(), sender.port) < 0)
	{
		std::cout<<"SIO::Error - Client could not resolve hostname\n";
		return -1;
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
	sio_stat.sync_counter = 0;
	sio_stat.sync_clock = config::netplay_sync_threshold;
	sio_stat.sync = false;
	sio_stat.transfer_byte = 0;
	
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

		//Pocket Pikachu 2
		case 0x2:
			sio_stat.ir_type = GBC_POCKET_PIKACHU_2;
			break;

		//Pocket Sakura
		case 0x3:
			sio_stat.ir_type = GBC_POCKET_SAKURA;
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

	if(config::ir_device == 1)
	{
		std::string database = config::data_path + "zzh_db.bin";
		full_changer_load_db(database);
	}

	#ifdef GBE_NETPLAY

	//Close any current connections
	if(network_init)
	{
		if(server.host_socket != NULL)
		{
			SDLNet_TCP_Close(server.host_socket);
		}

		if(server.remote_socket != NULL)
		{
			SDLNet_TCP_Close(server.remote_socket);
		}

		if(sender.host_socket != NULL)
		{
			//Update 4 Player status
			four_player.status &= ~0xF0;
			four_player_send_byte();

			//Send disconnect byte to another system first
			u8 temp_buffer[2];
			temp_buffer[0] = 0;
			temp_buffer[1] = 0x80;
		
			SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 2);

			SDLNet_TCP_Close(sender.host_socket);
		}
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

	#endif

	//4 Player Adapter
	four_player.current_state = FOUR_PLAYER_INACTIVE;
	four_player.ping_count = 0;
	four_player.id = 1;
	four_player.status = 1;

	while(!four_player.data.empty()) { four_player.data.pop(); }
}

/****** Tranfers one byte to another system ******/
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

/****** Tranfers status bytes via 4 Player Adapter ******/
bool DMG_SIO::four_player_send_byte()
{
	#ifdef GBE_NETPLAY

	u8 temp_buffer[2];
	temp_buffer[0] = four_player.status;
	temp_buffer[1] = 0xFE;

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
		u8 id = (temp_buffer[0] & 0xF);
		u8 status_bit = temp_buffer[0] & (1 << (id + 3));
				
		four_player.status &= ~(1 << (id + 3));
		four_player.status |= status_bit;
	}

	#endif

	return true;
}

/****** Broadcasts data to all connected Game Boys when using the 4 Player Adapter ******/
void DMG_SIO::four_player_broadcast(u8 data_one, u8 data_two)
{
	#ifdef GBE_NETPLAY

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

/****** Tranfers one bit to another system's IR port ******/
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

			//Receive 4 Player status update
			else if(temp_buffer[1] == 0xFE)
			{
				u8 id = (temp_buffer[0] & 0xF);
				u8 status_bit = temp_buffer[0] & (1 << (id + 3));
				
				four_player.status &= ~(1 << (id + 3));
				four_player.status |= status_bit;

				temp_buffer[0] = four_player.status;
				temp_buffer[1] = 0x1;

				//Send acknowlegdement
				SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 2);

				return true;
			}

			//Prepare 4 Player network
			else if(temp_buffer[1] == 0xFD)
			{
				//Switch to real 4 player communication after 4th byte sent
				if(temp_buffer[0] == 4)
				{
					four_player.current_state = FOUR_PLAYER_PROCESS_NETWORK;
					four_player.ping_count = 0;
				}

				mem->memory_map[REG_SB] = 0xCC;
				mem->memory_map[IF_FLAG] |= 0x08;

				temp_buffer[1] = 0x1;

				//Send acknowlegdement
				SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 2);

				return true;
			}

			//Receive byte broadcast through 4-player network
			else if(temp_buffer[1] == 0xFC)
			{
				mem->memory_map[REG_SB] = temp_buffer[0];
				mem->memory_map[IF_FLAG] |= 0x08;

				temp_buffer[1] = 0x1;

				//Send acknowlegdement
				SDLNet_TCP_Send(sender.host_socket, (void*)temp_buffer, 2);

				return true;
			}

			//Respond for request for transfer byte
			else if(temp_buffer[1] == 0xFB)
			{
				temp_buffer[0] = sio_stat.transfer_byte;
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

		if((server.connected) && (sender.connected))
		{
			sio_stat.connected = true;

			//Set the emulated SIO device type
			if(sio_stat.sio_type != GB_FOUR_PLAYER_ADAPTER) { sio_stat.sio_type = GB_LINK; }

			//For 4 Player adapter, set ID based on port number
			else
			{
				four_player.id = (sender.port > server.port) ? 1 : 2;
				four_player.status = four_player.id;
			}
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

	std::string filename = "gb_print_";
	filename += util::to_str(rand() % 1024);
	filename += util::to_str(rand() % 1024);
	filename += util::to_str(rand() % 1024);

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

		//Receive tranferred data
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

	std::size_t user_match = pop_data.find("USER");
	std::size_t pass_match = pop_data.find("PASS");
	std::size_t quit_match = pop_data.find("QUIT");

	//Check for POP initiation
	if((mobile_adapter.data_length == 1) && (!mobile_adapter.pop_session_started))
	{
		mobile_adapter.pop_session_started = true;
		pop_response = "+OK\r\n";
	}

	//Check for other commands, just say everything is OK
	else if((user_match != std::string::npos) || (pass_match != std::string::npos) || (quit_match != std::string::npos))
	{
		pop_response = "+OK\r\n";
	}

	//For everything else, return an error
	else
	{
		pop_response = "-ERR\r\n";
	}

	//Start building the reply packet
	mobile_adapter.packet_buffer.clear();
	mobile_adapter.packet_buffer.resize(7 + pop_response.size(), 0x00);

	//Magic bytes
	mobile_adapter.packet_buffer[0] = (0x99);
	mobile_adapter.packet_buffer[1] = (0x66);

	//Header
	mobile_adapter.packet_buffer[2] = (0x95);
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

	//Send empty body until HTTP request is finished transmitting
	if(mobile_adapter.data_length != 1)
	{
		mobile_adapter.http_data += util::data_to_str(mobile_adapter.packet_buffer.data() + 7, mobile_adapter.packet_buffer.size() - 7);
		http_response = " ";
		response_id = 0x95;
	}

	//Process HTTP request once initial line + headers + message body have been received.
	else
	{
		//For now, just return 404 as the default
		http_response = "HTTP/1.0 404 Not Found\r\n";

		//Determine if this request is GET or POST
		std::size_t get_match = mobile_adapter.http_data.find("GET");
		std::size_t post_match = mobile_adapter.http_data.find("POST");

		//Process GET requests
		if(get_match != std::string::npos)
		{
			//See if this is the homepage for Mobile Trainer
			if(mobile_adapter.http_data.find("/01/CGB-B9AJ/index.html") != std::string::npos)
			{
				http_response = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-Length: 31\r\n<html><body>HELLO</body></html>";
			}
		}

		mobile_adapter.http_data = "";

		//Be sure to change command ID to 0x9F at the end of an HTTP response
		response_id = 0x9F;
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

			if(bardigun_scanner.barcode_pointer == bardigun_scanner.data.size()) { bardigun_scanner.current_state = BARDIGUN_INACTIVE; }

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
	//Switch to handshake mode if necessary
	if((barcode_boy.current_state != BARCODE_BOY_INACTIVE) && (sio_stat.transfer_byte == 0x10))
	{
		barcode_boy.current_state = BARCODE_BOY_INACTIVE;
		barcode_boy.counter = 0;
		barcode_boy.send_data = false;
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
				barcode_boy.current_state = BARCODE_BOY_ACTIVE;
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

/****** Processes data sent to the Game Boy via 4 Player Adapter ******/
void DMG_SIO::four_player_process()
{
	if(sio_stat.internal_clock) { four_player.current_state = FOUR_PLAYER_INACTIVE; }
	
	else
	{
		//Start preparation for real Link Cable communications
		if((sio_stat.transfer_byte == 0xAA) && (four_player.id == 1))
		{
			if(four_player.current_state != FOUR_PLAYER_PREP_NETWORK) { four_player.ping_count = 0; }
			four_player.current_state = FOUR_PLAYER_PREP_NETWORK;
		}

		//Start Link Cable ping
		else if((four_player.current_state != FOUR_PLAYER_PREP_NETWORK) && (four_player.current_state != FOUR_PLAYER_PROCESS_NETWORK))
		{
			four_player.current_state = FOUR_PLAYER_PING;
		}
	}

	switch(four_player.current_state)
	{
		case FOUR_PLAYER_INACTIVE: return;

		//Handle ping
		case FOUR_PLAYER_PING:

			//Update link status
			if((four_player.ping_count == 1) || (four_player.ping_count == 2))
			{
				if(sio_stat.transfer_byte == 0x88) { four_player.status |= (1 << (four_player.id + 3)); }
				else { four_player.status &= ~(1 << (four_player.id + 3)); }

				if(sio_stat.connected) { four_player_send_byte(); }
			}

			//Return magic byte for 1st byte
			if(four_player.ping_count == 0) { mem->memory_map[REG_SB] = 0xFE; }

			//Otherwise, return status byte
			else { mem->memory_map[REG_SB] = four_player.status; }

			mem->memory_map[IF_FLAG] |= 0x08;

			four_player.ping_count++;
			four_player.ping_count &= 0x3;

			break;

		//Prepare for real Link Cable communications
		case FOUR_PLAYER_PREP_NETWORK:
			four_player.ping_count++;

			mem->memory_map[REG_SB] = 0xCC;
			mem->memory_map[IF_FLAG] |= 0x08;

			four_player_broadcast(four_player.ping_count, 0xFD);

			if(four_player.ping_count == 4)
			{
				four_player.ping_count = 0;
				four_player.current_state = FOUR_PLAYER_PROCESS_NETWORK;

				//Set up data buffer
				while(!four_player.data.empty()) { four_player.data.pop(); }

				//Data in initial buffer is technically garbage. Set to zero for now.
				for(u32 x = 0; x < 16; x++) { four_player.data.push(0x00); }
			}

			break;

		//Process Link Cable communications
		case FOUR_PLAYER_PROCESS_NETWORK:
			if(four_player.id != 1) { return; }

			switch(four_player.ping_count / 4)
			{
				//Player 1 - Output old data - Queue new data
				case 0x0:
					{
						//Request new data from Player 1
						u8 next_data = sio_stat.transfer_byte;

						//Broadcast old data to other Game Boys
						four_player_broadcast(four_player.data.front(), 0xFC);

						mem->memory_map[REG_SB] = four_player.data.front();
						mem->memory_map[IF_FLAG] |= 0x08;

						//Queue new data
						four_player.data.pop();
						four_player.data.push(sio_stat.transfer_byte);
					}

					break;

				//Player 2 - Output old data - Queue new data
				case 0x1:
					{
						//Request new data from Player 2
						u8 next_data = four_player_request(2, 0xFB);

						//Broadcast old data to other Game Boys
						four_player_broadcast(four_player.data.front(), 0xFC);

						mem->memory_map[REG_SB] = four_player.data.front();
						mem->memory_map[IF_FLAG] |= 0x08;

						//Queue new data
						four_player.data.pop();
						four_player.data.push(next_data);
					}

					break;

				//Player 3-4 - TODO
				case 0x2:
				case 0x3:
					{
						//Request new data
						u8 next_data = 0x00;

						//Broadcast old data to other Game Boys
						four_player_broadcast(four_player.data.front(), 0xFC);

						mem->memory_map[REG_SB] = four_player.data.front();
						mem->memory_map[IF_FLAG] |= 0x08;

						//Queue new data
						four_player.data.pop();
						four_player.data.push(next_data);
					}

					break;
			}

			four_player.ping_count++;
			four_player.ping_count &= 0xF;
	}
}
