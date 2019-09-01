// GB Enhanced Copyright Daniel Baxter 2018
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gbma.cpp
// Date : June 29, 2018
// Description : Game Boy Mobile Adapter
//
// Emulates the GB Mobile Adapter
// Emulates an internal server for Mobile Trainer, connects to emulated servers online

#include "sio.h"
#include "common/util.h"

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
						mobile_adapter.packet_buffer.push_back(0x17);
						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0x00);
						mobile_adapter.packet_buffer.push_back(0x01);

						//Body
						if(mobile_adapter.line_busy) { mobile_adapter.packet_buffer.push_back(0x04); }
						else { mobile_adapter.packet_buffer.push_back(0x00); }

						//Checksum
						mobile_adapter.packet_buffer.push_back(0x00);

						if(mobile_adapter.line_busy) { mobile_adapter.packet_buffer.push_back(0x1C); }
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
	bool found_item = false;

	std::size_t user_match = pop_data.find("USER");
	std::size_t pass_match = pop_data.find("PASS");
	std::size_t quit_match = pop_data.find("QUIT");
	std::size_t stat_match = pop_data.find("STAT");
	std::size_t top_match = pop_data.find("TOP");
	std::size_t dele_match = pop_data.find("DELE");
	std::size_t retr_match = pop_data.find("RETR");
	std::size_t list_match = pop_data.find("LIST");

	//Check POP command
	if(user_match != std::string::npos) { pop_command = 1; }
	else if(pass_match != std::string::npos) { pop_command = 2; }
	else if(quit_match != std::string::npos) { pop_command = 3; }
	else if(stat_match != std::string::npos) { pop_command = 4; }
	else if(top_match != std::string::npos) { pop_command = 5; }
	else if(dele_match != std::string::npos) { pop_command = 6; }
	else if(retr_match != std::string::npos) { pop_command = 7; }
	else if(list_match != std::string::npos) { pop_command = 8; }

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
		pop_command = 9;
	}

	//Open up internal file for email data if available
	if(pop_command == 7)
	{
		std::string filename = "";

		//Search internal server list
		for(u32 x = 0; x < mobile_adapter.srv_list_in.size(); x++)
		{
			//Check for GBE+ mail
			if((mobile_adapter.srv_list_in[x] == "gbe_mail.txt") && (x != mobile_adapter.srv_list_out.size()))
			{
				filename = config::data_path + mobile_adapter.srv_list_out[x];
				found_item = true;
				x++;
				break;
			}

		}
				
		if(found_item)
		{
			//Open up GBE+ mails
			std::ifstream file(filename.c_str(), std::ios::binary);

			if(file.is_open()) 
			{
				//Get file size
				file.seekg(0, file.end);
				u32 file_size = file.tellg();
				file.seekg(0, file.beg);

				mobile_adapter.net_data.resize(file_size, 0x0);
				u8* ex_mem = &mobile_adapter.net_data[0];

				file.read((char*)ex_mem, file_size);
				file.seekg(0, file.beg);
				file.close();

				mobile_adapter.data_index = 0;
			}

			else { std::cout<<"SIO :: GBMA could not open " << filename << "\n"; }
		}
	}

	switch(pop_command)
	{
		//Init + USER, PASS, DELE, and QUIT commands
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
		case 0x6:
			pop_response = "+OK\r\n";
			response_id = 0x95;
			break;

		//STAT command
		case 0x4:
			//When not connecting a real server, fake an inbox with 1 message
			pop_response = "+OK 1\r\n";
			response_id = 0x95;
			break;

		//TOP command
		case 0x5:
			//When not connecting to a real server, fake a message
			pop_response = "+OK\r\n";
			response_id = 0x95;
			mobile_adapter.transfer_state = 0x1;
			break;

		//RETR
		case 0x7:
			pop_response = "+OK\r\n";
			response_id = 0x95;
			mobile_adapter.transfer_state = 0x11;
			break;

		//LIST
		case 0x8:
			pop_response = "+OK\r\n1 256\r\n";
			response_id = 0x9F;
			break;

		//End
		case 0x9:
			pop_response = "+OK\r\n";
			response_id = 0x9F;
			break;

		//Error
		default:
			pop_response = "-ERR\r\n";
			response_id = 0x95;
			break;
	}

	//Handle transfer states for various POP commands
	switch(mobile_adapter.transfer_state)
	{
		//Init TOP
		//Init RETR
		case 0x1:
		case 0x11:
			//Return email data from file
			if((mobile_adapter.transfer_state == 0x11) && (found_item)) { mobile_adapter.transfer_state = 0x20; }

			//Otherwise, continue with TOP or RETR from default email
			else { mobile_adapter.transfer_state = (mobile_adapter.transfer_state & 0x10) ? 0x12 : 0x2; }

			break;

		//Email Header - Date
		case 2:
		case 0x12:
			pop_response = "Date: Wed, 25 Jul 2018 12:00:00 -0600\r\n";
			mobile_adapter.transfer_state = (mobile_adapter.transfer_state & 0x10) ? 0x13 : 0x3;
			response_id = 0x95;
			break;

		//Email Header - Subject
		case 3:
		case 0x13:
			pop_response = "Subject: This is a test\r\n";
			mobile_adapter.transfer_state = (mobile_adapter.transfer_state & 0x10) ? 0x14 : 0x4;
			response_id = 0x95;
			break;

		//Email Header - Sender
		case 4:
		case 0x14:
			pop_response = "From: gbe_plus@test.com\r\n";
			mobile_adapter.transfer_state = (mobile_adapter.transfer_state & 0x10) ? 0x15 : 0x5;
			response_id = 0x95;
			break;

		//Email Header - Recipient
		case 5:
		case 0x15:
			pop_response = "To: user@test.com\r\n";
			mobile_adapter.transfer_state = (mobile_adapter.transfer_state & 0x10) ? 0x16 : 0x6;
			response_id = 0x95;
			break;

		//Email Header - Content Type
		case 6:
		case 0x16:
			pop_response = "Content-Type: text/plain\r\n\r\n";
			mobile_adapter.transfer_state = (mobile_adapter.transfer_state & 0x10) ? 0x17 : 0x7;
			response_id = 0x95;
			break;

		//End TOP
		case 7:
			pop_response = ".\r\n";
			mobile_adapter.transfer_state = 0x0;
			response_id = 0x95;
			break;

		//Message content
		case 0x17:
			pop_response = "Hello from GBE+\r\n\r\n";
			mobile_adapter.transfer_state = 0x18;
			response_id = 0x95;
			break;

		//Message end
		case 0x18:
			pop_response = ".\r\n";
			mobile_adapter.transfer_state = 0x0;
			response_id = 0x95;
			break;

		//RETR from file data
		case 0x20:
			pop_response = "";

			while((mobile_adapter.net_data[mobile_adapter.data_index] != 0xA) && (mobile_adapter.data_index != mobile_adapter.net_data.size()))
			{
				pop_response += std::string(1, mobile_adapter.net_data[mobile_adapter.data_index++]);
			}

			if(mobile_adapter.data_index != mobile_adapter.net_data.size())
			{
				pop_response += std::string(1, mobile_adapter.net_data[mobile_adapter.data_index++]);
				mobile_adapter.transfer_state = 0x20;
			}

			else { mobile_adapter.transfer_state = 0x0; }

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
	std::string http_header = "";
	u8 response_id = 0;
	bool not_found = true;
	bool needs_auth = false;
	bool img = mobile_adapter.http_data.find(".bmp") != std::string::npos;
	bool html = mobile_adapter.http_data.find(".html") != std::string::npos;

	//Build HTTP request
	mobile_adapter.http_data += util::data_to_str(mobile_adapter.packet_buffer.data() + 7, mobile_adapter.packet_buffer.size() - 7);

	//Send empty body until HTTP request is finished transmitting
	if(mobile_adapter.http_data.find("\r\n\r\n") == std::string::npos)
	{
		http_response = "";
		response_id = 0x95;
	}

	//Process HTTP request once initial line + headers + message body have been received.
	else if(!mobile_adapter.transfer_state)
	{
		//Update transfer status
		mobile_adapter.transfer_state = 1;

		//Determine if this request is GET or POST
		std::size_t get_match = mobile_adapter.http_data.find("GET");
		std::size_t post_match = mobile_adapter.http_data.find("POST");

		//Process GET requests
		if(get_match != std::string::npos)
		{
			//Use internal server
			if(!config::use_real_gbma_server)
			{
				std::string filename = "";

				//Search internal server list
				for(u32 x = 0; x < mobile_adapter.srv_list_in.size(); x++)
				{
					if(mobile_adapter.http_data.find(mobile_adapter.srv_list_in[x]) != std::string::npos)
					{
						not_found = false;
						filename = config::data_path + mobile_adapter.srv_list_out[x];

						//Check for GBE+ images
						if(mobile_adapter.srv_list_out[x] == "gbma/gbe_plus_mobile_header.bmp") { img = true; }

						//Check for auth status
						//This method bypasses sending an actual WWW-Authenticate: GB00 header
						if(mobile_adapter.auth_list[x] == 0x00)
						{
							http_header = "Gb-Auth-ID: authaccepted\r\n\r\n";
							needs_auth = true;
							mobile_adapter.auth_list[x] = 0x01;
						}

						break;
					}
				}

				if(!not_found)
				{
					//Open up GBE+ header image
					std::ifstream file(filename.c_str(), std::ios::binary);

					if(file.is_open()) 
					{
						//Get file size
						file.seekg(0, file.end);
						u32 file_size = file.tellg();
						file.seekg(0, file.beg);

						mobile_adapter.net_data.resize(file_size, 0x0);
						u8* ex_mem = &mobile_adapter.net_data[0];

						file.read((char*)ex_mem, file_size);
						file.seekg(0, file.beg);
						file.close();

						not_found = false;

						mobile_adapter.data_index = 0;
					}

					else { std::cout<<"SIO :: GBMA could not open " << filename << "\n"; }
				}
			}

			//Request data from real GBMA server
			else
			{
				//Isolate requested URL
				std::size_t req_start = mobile_adapter.http_data.find("GET ");
				std::string req_str = "";
				std::string rep_str = "";
				
				if(req_start != std::string::npos)
				{
					req_str = mobile_adapter.http_data.substr(req_start + 4);
					std::size_t req_end = req_str.find(" ");

					if(req_end != std::string::npos)
					{
						req_str = req_str.substr(0, req_end);
						std::string work_str = "";

						//Strip out characters that should not be in a URL
						for(u32 x = 0; x < req_str.length(); x++)
						{
							if((req_str[x] >= 0x20) && (req_str[x] <= 0x7E)) { work_str += req_str[x]; }
						}

						req_str = work_str;
					}
				}

				//Make HTTP request if a URL was parsed
				if(!req_str.empty() && sender.host_init)
				{
					req_str = "GET " + req_str + " HTTP/1.0\r\n\r\n";
					rep_str = "";
					u32 msg_size = req_str.size() + 1;
					int response_size = -1;
					u8 response_data[0x8000];
					u8 timeout = 10;

					#ifdef GBE_NETPLAY

					if(SDLNet_TCP_Send(sender.host_socket, req_str.c_str(), msg_size) < msg_size)
					{
						std::cout<<"SIO::Error - Could not transmit to GB Mobile Adapter server\n";
					}

					//Wait for response
					else
					{
						mobile_adapter.net_data.clear();

						//Get HTTP headers
						while(timeout)
						{
							response_size = -1;

							//Check for socket activity, timeout after 1 second
							if(SDLNet_CheckSockets(tcp_sockets, 100) != -1)
							{
								response_size = SDLNet_TCP_Recv(sender.host_socket, (void*)response_data, 0x8000);
							}

							//If a response was received, store net data
							if((response_size != -1) && (response_size < 0x8000))
							{
								for(u32 x = 0; x < response_size; x++)
								{
									rep_str += response_data[x];
									mobile_adapter.net_data.push_back(response_data[x]);
								}
							}

							timeout--;
						}
						
						mobile_adapter.data_index = 0;
						mobile_adapter.transfer_state = 3;
					}

					#endif
				}
			}
		}

		//Process POST requests
		else if(post_match != std::string::npos)
		{
			bool post_is_done = false;

			//Determine if POST Content-Length is done
			std::size_t post_len_match = mobile_adapter.http_data.find("Content-Length:");

			//Check to see if Content-Length has been specified in HTTP header
			if(post_len_match != std::string::npos)
			{
				//Isolate string of number of bytes
				std::string con_len = mobile_adapter.http_data.substr(post_len_match);
				std::size_t next_line = con_len.find("\r\n");
				con_len = con_len.substr(15, (next_line - 15));
				
				std::string final_str = "";
				std::string temp_chr = "";

				//Clean up string (only allow ASCII characters 0 - 9
				for(u32 x = 0; x < con_len.length(); x++)
				{
					temp_chr = con_len[x];
					if((temp_chr[0] >= 0x30) && (temp_chr[0] <= 0x39)) { final_str += temp_chr; }
				}

				//Convert string into a number
				u32 num_of_bytes = 0;
				bool found_len = util::from_str(final_str, num_of_bytes);

				//If a valid length is found, wait until the appropiate amount of data is sent
				if(found_len)
				{
					std::size_t data_start = mobile_adapter.http_data.find("\r\n\r\n");
					
					//Find the end of HTTP headers
					if(data_start != std::string::npos)
					{
						std::string post_data = mobile_adapter.http_data.substr(data_start + 4);

						//Send 200 OK response if content length has been met
						if(post_data.length() >= num_of_bytes)
						{
							http_response = "HTTP/1.0 200 OK\r\n\r\n";
							response_id = 0x95;
							mobile_adapter.transfer_state = 5;
							post_is_done = true;
						}
					}
				}
			}

			if(!post_is_done)
			{
				response_id = 0x95;
				mobile_adapter.transfer_state = 0;
			}
		}
	}
	
	//Respond to HTTP request
	switch(mobile_adapter.transfer_state)
	{
		//Status - 200 or 404. 401 if authentication needed
		case 0x1:
			if(needs_auth)
			{
				http_response = "HTTP/1.0 401 Unauthorized\r\n";
				mobile_adapter.transfer_state = 4;
			}
				
			else if(not_found)
			{
				http_response = "HTTP/1.0 404 Not Found\r\n";
				mobile_adapter.transfer_state = 2;
			}

			else
			{
				http_response = "HTTP/1.0 200 OK\r\n";
				mobile_adapter.transfer_state = 2;
			}

			http_response += http_header;
			response_id = 0x95;

			break;

		//Header or close connection if 404
		case 0x2:
			if(!not_found)
			{
				http_response = "";
				response_id = 0x9F;
				mobile_adapter.transfer_state = 0;
				mobile_adapter.http_data = "";
			}

			else if(html)
			{
				http_response = "Content-Type: text/html\r\n\r\n";
				response_id = 0x95;
				mobile_adapter.transfer_state = 3;
				mobile_adapter.http_data = "";
			}

			else if(img)
			{
				http_response = "Content-Type: image/bmp\r\n\r\n";
				response_id = 0x95;
				mobile_adapter.transfer_state = 0x13;
				mobile_adapter.http_data = "";
			}

			else
			{
				http_response = "Content-Type: text/plain\r\n\r\n";
				response_id = 0x95;
				mobile_adapter.transfer_state = 3;
				mobile_adapter.http_data = "";
			}

			break;

		//HTTP data payload
		case 0x3:
			for(int x = 0; x < 254; x++)
			{
				if(mobile_adapter.data_index < mobile_adapter.net_data.size())
				{
					http_response += mobile_adapter.net_data[mobile_adapter.data_index++];
				}

				else { x = 255; }
			}

			response_id = 0x95;
			mobile_adapter.transfer_state = (mobile_adapter.data_index < mobile_adapter.net_data.size()) ? 0x3 : 0x4;
			break;

		case 0x13:
			for(int x = 0; x < 254; x++)
			{
				if(mobile_adapter.data_index < mobile_adapter.net_data.size())
				{
					http_response += mobile_adapter.net_data[mobile_adapter.data_index++];
				}

				else { x = 255; }
			}
					
			response_id = 0x95;
			mobile_adapter.transfer_state = (mobile_adapter.data_index < mobile_adapter.net_data.size()) ? 0x13 : 0x4;
			break;

		//Close connection
		case 0x4:
			http_response = "";
			response_id = 0x9F;
			mobile_adapter.transfer_state = 0;
			mobile_adapter.http_data = "";
			break;

		//Prepare POST request finish
		case 0x5:
			mobile_adapter.transfer_state = 4;
			mobile_adapter.http_data = "";
			break;
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

/****** Processes SMTP transfers from the emulated GB Mobile Adapter ******/
void DMG_SIO::mobile_adapter_process_smtp()
{
	std::string smtp_response = "";
	std::string smtp_data = util::data_to_str(mobile_adapter.packet_buffer.data(), mobile_adapter.packet_buffer.size());
	u8 response_id = 0;
	u8 smtp_command = 0xFF;

	//Check for SMTP initiation
	if((mobile_adapter.data_length == 1) && (!mobile_adapter.transfer_state) && (!mobile_adapter.smtp_session_started))
	{
		mobile_adapter.smtp_session_started = true;
		smtp_command = 0;
	}

	std::size_t mail_match = smtp_data.find("MAIL FROM");
	std::size_t rcpt_match = smtp_data.find("RCPT TO");
	std::size_t quit_match = smtp_data.find("QUIT");
	std::size_t data_match = smtp_data.find("DATA");
	std::size_t helo_match = smtp_data.find("HELO");
	std::size_t end_match = smtp_data.find("\r\n.\r\n");

	//Check POP command
	if(mail_match != std::string::npos) { smtp_command = 1; }
	else if(rcpt_match != std::string::npos) { smtp_command = 2; }
	else if(quit_match != std::string::npos) { smtp_command = 3; }
	else if(data_match != std::string::npos) { smtp_command = 4; }
	else if(helo_match != std::string::npos) { smtp_command = 5; }
	else if(end_match != std::string::npos) { smtp_command = 6; }

	//Handle SMTP commands
	switch(smtp_command)
	{
		//Init
		case 0x0:
			smtp_response = "220 OK\r\n";
			response_id = 0x95;
			break;

		//MAIL FROM, RCPT TO, HELO, DATA-END
		case 0x1:
		case 0x2:
		case 0x5:
		case 0x6:
			smtp_response = "250 OK\r\n";
			response_id = 0x95;
			break;

		//QUIT
		case 0x3:
			smtp_response = "221\r\n";
			response_id = 0x95;
			break;

		//DATA
		case 0x4:
			smtp_response = "354\r\n";
			response_id = 0x95;
			break;

		default:
			smtp_response = "250 OK\r\n";
			response_id = 0x95;
			break;
	}

	//Start building the reply packet
	mobile_adapter.packet_buffer.clear();
	mobile_adapter.packet_buffer.resize(7 + smtp_response.size(), 0x00);

	//Magic bytes
	mobile_adapter.packet_buffer[0] = 0x99;
	mobile_adapter.packet_buffer[1] = 0x66;

	//Header
	mobile_adapter.packet_buffer[2] = response_id;
	mobile_adapter.packet_buffer[3] = 0x00;
	mobile_adapter.packet_buffer[4] = 0x00;
	mobile_adapter.packet_buffer[5] = smtp_response.size() + 1;

	//Body
	util::str_to_data(mobile_adapter.packet_buffer.data() + 7, smtp_response);

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

/***** Loads a list of web addresses and points to local files in GBE+ data folder ******/
bool DMG_SIO::mobile_adapter_load_server_list()
{
	mobile_adapter.srv_list_in.clear();
	mobile_adapter.srv_list_out.clear();

	std::string input_line = "";
	std::string input_char = "";
	std::string list_file = config::data_path + "gbma/server_list.txt";
	std::ifstream file(list_file.c_str(), std::ios::in);
	u8 line_count = 0;

	if(!file.is_open())
	{
		std::cout<<"SIO::Error - Could not open GB Mobile Adapter internal server list\n";
		return false;
	}

	while(getline(file, input_line))
	{
		if(!input_line.empty())
		{
			//Populate paths of local files
			if(line_count & 0x1) { mobile_adapter.srv_list_out.push_back(input_line); }

			//Populate paths of URIs to match (full or partial)
			else
			{
				input_char = input_line[0];

				//Determine if URI needs HTTP authentication (prepended with an *)
				if(input_char == "*")
				{
					input_line = input_line.substr(1);

					//0x00 = No auth done yet. 0x01 = Auth complete. 0xFF = No auth needed
					mobile_adapter.auth_list.push_back(0x00);
				}

				else { mobile_adapter.auth_list.push_back(0xFF); }

				mobile_adapter.srv_list_in.push_back(input_line);
			}
			
			line_count++;
		}
	}

	if(mobile_adapter.srv_list_in.size() > mobile_adapter.srv_list_out.size()) { mobile_adapter.srv_list_in.pop_back(); }

	std::cout<<"SIO::Loaded GB Mobile Adapter internal server list\n";

	file.close();
	return true;
}
