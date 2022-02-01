// GB Enhanced Copyright Daniel Baxter 2019
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gbma.cpp
// Date : June 29, 2019
// Description : Game Boy Mobile Adapter
//
// Emulates the Mobile Adapter GB
// Emulates an internal servers, connects to emulated servers online

#include "sio.h"
#include "common/util.h"
 
/****** Processes data sent to the GB Mobile Adapter - 8-bit Mode ******/
void AGB_SIO::mobile_adapter_process_08()
{
	switch(mobile_adapter.current_state)
	{
		//Receive packet data
		case AGB_GBMA_AWAITING_PACKET:
			
			//Push data to packet buffer	
			mobile_adapter.packet_buffer.push_back(sio_stat.transfer_data);
			mobile_adapter.packet_size++;

			//Check for magic number 0x99 0x66
			if((mobile_adapter.packet_size == 2) && (mobile_adapter.packet_buffer[0] == 0x99) && (mobile_adapter.packet_buffer[1] == 0x66))
			{
				//Move to the next state
				mobile_adapter.packet_size = 0;
				mobile_adapter.current_state = AGB_GBMA_RECEIVE_HEADER;
			}

			//If magic number not found, reset
			else if(mobile_adapter.packet_size == 2)
			{
				mobile_adapter.packet_size = 1;
				u8 temp = mobile_adapter.packet_buffer[1];
				mobile_adapter.packet_buffer.clear();
				mobile_adapter.packet_buffer.push_back(temp);
			}

			//Send data back to GBA + IRQ
			mem->memory_map[SIO_DATA_8] = 0xD2;
			if(sio_stat.cnt & 0x4000) { mem->memory_map[REG_IF] |= 0x80; }
			sio_stat.cnt &= ~0x80;
			
			break;

		//Receive header data
		case AGB_GBMA_RECEIVE_HEADER:

			//Push data to packet buffer
			mobile_adapter.packet_buffer.push_back(sio_stat.transfer_data);
			mobile_adapter.packet_size++;

			//Process header data
			if(mobile_adapter.packet_size == 4)
			{
				mobile_adapter.command = mobile_adapter.packet_buffer[2];
				mobile_adapter.data_length = mobile_adapter.packet_buffer[5];

				//Move to the next state
				mobile_adapter.packet_size = 0;
				mobile_adapter.current_state = (mobile_adapter.data_length == 0) ? AGB_GBMA_RECEIVE_CHECKSUM : AGB_GBMA_RECEIVE_DATA;

				std::cout<<"SIO::Mobile Adapter - Command ID 0x" << std::hex << (u32)mobile_adapter.command << "\n";
			}
			
			//Send data back to GBA + IRQ
			mem->memory_map[SIO_DATA_8] = 0xD2;
			if(sio_stat.cnt & 0x4000) { mem->memory_map[REG_IF] |= 0x80; }
			sio_stat.cnt &= ~0x80;
			
			break;

		//Receive transferred data
		case AGB_GBMA_RECEIVE_DATA:

			//Push data to packet buffer
			mobile_adapter.packet_buffer.push_back(sio_stat.transfer_data);
			mobile_adapter.packet_size++;

			//Move onto the next state once all data has been received
			if(mobile_adapter.packet_size == mobile_adapter.data_length)
			{
				mobile_adapter.packet_size = 0;
				mobile_adapter.current_state = AGB_GBMA_RECEIVE_CHECKSUM;
			}

			//Send data back to GBA + IRQ
			mem->memory_map[SIO_DATA_8] = 0xD2;
			if(sio_stat.cnt & 0x4000) { mem->memory_map[REG_IF] |= 0x80; }
			sio_stat.cnt &= ~0x80;

			break;

		//Receive packet checksum
		case AGB_GBMA_RECEIVE_CHECKSUM:

			//Push data to packet buffer
			mobile_adapter.packet_buffer.push_back(sio_stat.transfer_data);
			mobile_adapter.packet_size++;

			//Grab MSB of the checksum
			if(mobile_adapter.packet_size == 1)
			{
				mobile_adapter.checksum = (sio_stat.transfer_data << 8);

				//Send data back to GBA + IRQ
				mem->memory_map[SIO_DATA_8] = 0xD2;
				if(sio_stat.cnt & 0x4000) { mem->memory_map[REG_IF] |= 0x80; }
				sio_stat.cnt &= ~0x80;
			}

			//Grab LSB of the checksum, move on to next state
			else if(mobile_adapter.packet_size == 2)
			{
				mobile_adapter.checksum |= sio_stat.transfer_data;

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
					mobile_adapter.current_state = AGB_GBMA_ACKNOWLEDGE_PACKET;

					//Send data back to GBA + IRQ
					mem->memory_map[SIO_DATA_8] = 0xD2;
					if(sio_stat.cnt & 0x4000) { mem->memory_map[REG_IF] |= 0x80; }
					sio_stat.cnt &= ~0x80;
				}

				//Send and error and wait for the packet to restart
				else
				{
					mobile_adapter.packet_size = 0;
					mobile_adapter.current_state = AGB_GBMA_AWAITING_PACKET;

					//Send data back to GBA + IRQ
					mem->memory_map[SIO_DATA_8] = 0xF1;
					if(sio_stat.cnt & 0x4000) { mem->memory_map[REG_IF] |= 0x80; }
					sio_stat.cnt &= ~0x80;

					std::cout<<"SIO::Mobile Adapter - Checksum Fail \n";
				}
			}

			break;

		//Acknowledge packet
		case AGB_GBMA_ACKNOWLEDGE_PACKET:
		
			//Push data to packet buffer
			mobile_adapter.packet_buffer.push_back(sio_stat.transfer_data);
			mobile_adapter.packet_size++;

			//Mobile Adapter expects 0x81 from GBA, sends back 0x88 through 0x8F (does not matter which)
			if(mobile_adapter.packet_size == 1)
			{
				if(sio_stat.transfer_data == 0x81)
				{
					mem->memory_map[SIO_DATA_8] = 0x88;
					if(sio_stat.cnt & 0x4000) { mem->memory_map[REG_IF] |= 0x80; }
					sio_stat.cnt &= ~0x80;
				}

				else
				{
					mobile_adapter.packet_size = 0;
					mem->memory_map[SIO_DATA_8] = 0xD2;
					if(sio_stat.cnt & 0x4000) { mem->memory_map[REG_IF] |= 0x80; }
					sio_stat.cnt &= ~0x80;
				}	
			}

			//Send back 0x80 XOR current command + IRQ on 2nd byte, begin processing command
			else if(mobile_adapter.packet_size == 2)
			{
				mem->memory_map[SIO_DATA_8] = 0x80 ^ mobile_adapter.command;
				if(sio_stat.cnt & 0x4000) { mem->memory_map[REG_IF] |= 0x80; }
				sio_stat.cnt &= ~0x80;

				mobile_adapter_execute_command();
			}

			break;

		//Echo packet back to Game Boy
		case AGB_GBMA_ECHO_PACKET:

			//Check for communication errors
			switch(sio_stat.transfer_data)
			{
				case 0xEE:
				case 0xF0:
				case 0xF1:
				case 0xF2:
					std::cout<<"SIO::Error - GB Mobile Adapter communication failure code 0x" << std::hex << (sio_stat.transfer_data & 0xFF) << "\n";
					break;
			}

			//Send back the packet bytes
			if(mobile_adapter.packet_size < mobile_adapter.packet_buffer.size())
			{
				mem->memory_map[SIO_DATA_8] = mobile_adapter.packet_buffer[mobile_adapter.packet_size++];
				if(sio_stat.cnt & 0x4000) { mem->memory_map[REG_IF] |= 0x80; }
				sio_stat.cnt &= ~0x80;

				if(mobile_adapter.packet_size == mobile_adapter.packet_buffer.size())
				{
					mobile_adapter.packet_buffer.clear();
					mobile_adapter.packet_size = 0;
					mobile_adapter.current_state = AGB_GBMA_AWAITING_PACKET;
				}
			}

			break;
	}
}

/****** Processes data sent to the GB Mobile Adapter - 32bit Mode ******/
void AGB_SIO::mobile_adapter_process_32()
{
	std::cout<<"STUB\n";
}

/****** Begins execution of Mobile Adapter command ******/
void AGB_SIO::mobile_adapter_execute_command()
{
	//Process commands sent to the mobile adapter
	switch(mobile_adapter.command)
	{

		//For certain commands, the mobile adapter echoes the packet it received
		case 0x10:
		case 0x11:
			mobile_adapter.packet_size = 0;
			mobile_adapter.current_state = AGB_GBMA_ECHO_PACKET;

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
			if(mobile_adapter.command == 0x12) { mobile_adapter.packet_buffer.push_back(0x92); }
			else if(mobile_adapter.command == 0x13) { mobile_adapter.packet_buffer.push_back(0x93); }
			else { mobile_adapter.packet_buffer.push_back(0x94); }

			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0x00);

			//Checksum
			mobile_adapter.packet_buffer.push_back(0x00);

			if(mobile_adapter.command == 0x12) { mobile_adapter.packet_buffer.push_back(0x92); }
			else if(mobile_adapter.command == 0x13) { mobile_adapter.packet_buffer.push_back(0x93); }
			else { mobile_adapter.packet_buffer.push_back(0x94); }

			//Acknowledgement handshake
			mobile_adapter.packet_buffer.push_back(0x88);
			mobile_adapter.packet_buffer.push_back(0x00);

			//Send packet back
			mobile_adapter.packet_size = 0;
			mobile_adapter.current_state = AGB_GBMA_ECHO_PACKET;

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

			//SMTP data transfer
			else if(mobile_adapter.port == 25) { mobile_adapter_process_smtp(); }

			//Unknown port
			else { std::cout<<"SIO::Warning - Mobile Adapter accessing TCP transfer on unknown port " << std::dec << mobile_adapter.port << "\n"; }

			break;

		//Telephone status
		case 0x17:
			//Start building the reply packet
			mobile_adapter.packet_buffer.clear();

			//Magic bytes
			mobile_adapter.packet_buffer.push_back(0x99);
			mobile_adapter.packet_buffer.push_back(0x66);

			//Header
			mobile_adapter.packet_buffer.push_back(0x97);
			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0x01);

			//Body
			if(mobile_adapter.line_busy) { mobile_adapter.packet_buffer.push_back(0x04); }
			else { mobile_adapter.packet_buffer.push_back(0x00); }

			//Checksum
			mobile_adapter.packet_buffer.push_back(0x00);

			if(mobile_adapter.line_busy) { mobile_adapter.packet_buffer.push_back(0x9C); }
			else { mobile_adapter.packet_buffer.push_back(0x98); }

			//Acknowledgement handshake
			mobile_adapter.packet_buffer.push_back(0x88);
			mobile_adapter.packet_buffer.push_back(0x00);

			//Send packet back
			mobile_adapter.packet_size = 0;
			mobile_adapter.current_state = AGB_GBMA_ECHO_PACKET;

			break;

		//SIO32 Mode Switch
		case 0x18:
			//Switch to NORMAL32 or NORMAL8 mode depending on data received
			if(mobile_adapter.packet_buffer[6] == 0x01) { mobile_adapter.s32_mode = true; }
			else { mobile_adapter.s32_mode = false; }

			//Start building the reply packet - Empty body
			mobile_adapter.packet_buffer.clear();

			//Magic bytes
			mobile_adapter.packet_buffer.push_back(0x99);
			mobile_adapter.packet_buffer.push_back(0x66);

			//Header
			mobile_adapter.packet_buffer.push_back(0x98);
			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0x00);

			//Checksum
			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0x98);

			//Acknowledgement handshake
			mobile_adapter.packet_buffer.push_back(0x88);
			mobile_adapter.packet_buffer.push_back(0x00);

			//Send packet back
			mobile_adapter.packet_size = 0;
			mobile_adapter.current_state = AGB_GBMA_ECHO_PACKET;

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
				mobile_adapter.packet_buffer.push_back(0x99);
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
				mobile_adapter.current_state = AGB_GBMA_ECHO_PACKET;
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
				mobile_adapter.packet_buffer.push_back(0x9A);
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
				mobile_adapter.current_state = AGB_GBMA_ECHO_PACKET;
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
			mobile_adapter.packet_buffer.push_back(0xA1);
			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0x04);

			//Body - 127.0.0.1
			mobile_adapter.packet_buffer.push_back(0x7F);
			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0x01);

			//Checksum
			mobile_adapter.packet_buffer.push_back(0x01);
			mobile_adapter.packet_buffer.push_back(0x25);

			//Acknowledgement handshake
			mobile_adapter.packet_buffer.push_back(0x88);
			mobile_adapter.packet_buffer.push_back(0x00);

			//Send packet back
			mobile_adapter.packet_size = 0;
			mobile_adapter.current_state = AGB_GBMA_ECHO_PACKET;

			break;

		//ISP Logout
		case 0x22:
			//Start building the reply packet - Empty body
			mobile_adapter.packet_buffer.clear();

			//Magic bytes
			mobile_adapter.packet_buffer.push_back(0x99);
			mobile_adapter.packet_buffer.push_back(0x66);

			//Header
			mobile_adapter.packet_buffer.push_back(0xA2);
			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0x00);

			//Checksum
			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0xA2);

			//Acknowledgement handshake
			mobile_adapter.packet_buffer.push_back(0x88);
			mobile_adapter.packet_buffer.push_back(0x00);

			//Send packet back
			mobile_adapter.packet_size = 0;
			mobile_adapter.current_state = AGB_GBMA_ECHO_PACKET;

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
			mobile_adapter.current_state = AGB_GBMA_ECHO_PACKET;

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
			mobile_adapter.packet_buffer.push_back(0xA4);
			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0x01);

			//Body - Random byte
			mobile_adapter.packet_buffer.push_back(0x77);

			//Checksum
			mobile_adapter.packet_buffer.push_back(0x01);
			mobile_adapter.packet_buffer.push_back(0x1C);

			//Acknowledgement handshake
			mobile_adapter.packet_buffer.push_back(0x88);
			mobile_adapter.packet_buffer.push_back(0x00);

			//Send packet back
			mobile_adapter.packet_size = 0;
			mobile_adapter.current_state = AGB_GBMA_ECHO_PACKET;
			mobile_adapter.http_data = "";

			//Reset all sessions
			mobile_adapter.pop_session_started = false;
			mobile_adapter.http_session_started = false;
			mobile_adapter.smtp_session_started = false;
			mobile_adapter.transfer_state = 0;

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
			mobile_adapter.packet_buffer.push_back(0xA8);
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
			mobile_adapter.packet_buffer.push_back(0xAC);

			//Acknowledgement handshake
			mobile_adapter.packet_buffer.push_back(0x88);
			mobile_adapter.packet_buffer.push_back(0x00);

			//Send packet back
			mobile_adapter.packet_size = 0;
			mobile_adapter.current_state = AGB_GBMA_ECHO_PACKET;

			break;

		default:
			std::cout<<"SIO::Mobile Adapter - Unknown Command ID 0x" << std::hex << (u32)mobile_adapter.command << "\n";
			mobile_adapter.packet_buffer.clear();
			mobile_adapter.packet_size = 0;
			mobile_adapter.current_state = AGB_GBMA_AWAITING_PACKET;

			break;
	}
}

/****** Processes POP transfers from the emulated Mobile Adapter ******/
void AGB_SIO::mobile_adapter_process_pop() { }

/****** Processes HTTP transfers from the emulated Mobile Adapter ******/
void AGB_SIO::mobile_adapter_process_http() { }

/****** Processes SMTP transfers from the emulated GB Mobile Adapter ******/
void AGB_SIO::mobile_adapter_process_smtp() { }

/***** Loads a list of web addresses and points to local files in GBE+ data folder ******/
bool AGB_SIO::mobile_adapter_load_server_list()
{
	mobile_adapter.srv_list_in.clear();
	mobile_adapter.srv_list_out.clear();

	std::string input_line = "";
	std::string list_file = config::data_path + "gbma/server_list.txt";
	std::ifstream file(list_file.c_str(), std::ios::in);
	u8 line_count = 0;

	if(!file.is_open())
	{
		std::cout<<"SIO::Error - Could not open Mobile Adapter GB internal server list\n";
		return false;
	}

	while(getline(file, input_line))
	{
		if(!input_line.empty())
		{
			if(line_count & 0x1) { mobile_adapter.srv_list_out.push_back(input_line); }
			else { mobile_adapter.srv_list_in.push_back(input_line); }
			
			line_count++;
		}
	}

	if(mobile_adapter.srv_list_in.size() > mobile_adapter.srv_list_out.size()) { mobile_adapter.srv_list_in.pop_back(); }

	std::cout<<"SIO::Loaded Mobile Adapter GB internal server list\n";

	file.close();
	return true;
}