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

				//Calculate correct data length for 32-bit mode
				if((mobile_adapter.s32_mode) && (mobile_adapter.data_length & 0x3))
				{
					mobile_adapter.data_length += (4 - (mobile_adapter.data_length & 0x3));
				}

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

					//Switch to 32-bit mode if necessary
					if(mobile_adapter.switch_mode)
					{
						mobile_adapter.switch_mode = false;
						mobile_adapter.s32_mode = true;
					}
				}
			}

			break;
	}
}

/****** Processes data sent to the GB Mobile Adapter - 32bit Mode ******/
void AGB_SIO::mobile_adapter_process_32()
{
	u8 original_byte = mem->memory_map[SIO_DATA_8];
	u8 input_byte = 0x00;

	u32 out_data = 0x00;
	u32 shift = 24;
	u32 temp_data = sio_stat.transfer_data;

	//Process 32-bit input the same as 8-bit input, break down byte MSB first
	for(u32 x = 0; x < 4; x++)
	{
		agb_mobile_state start_state = mobile_adapter.current_state;

		input_byte = sio_stat.transfer_data >> shift;
		sio_stat.transfer_data = input_byte;

		mobile_adapter_process_08();
		sio_stat.transfer_data = temp_data;

		out_data |= (mem->memory_map[SIO_DATA_8] << shift);
		shift -= 8;

		//Stop building 32-bit response if necessary
		if((start_state == AGB_GBMA_ACKNOWLEDGE_PACKET) && (mobile_adapter.current_state == AGB_GBMA_ECHO_PACKET))
		{
			break;
		}
	}

	mem->memory_map[SIO_DATA_8] = original_byte;
	mem->write_u32_fast(SIO_DATA_32_L, out_data);
}

/****** Begins execution of Mobile Adapter command ******/
void AGB_SIO::mobile_adapter_execute_command()
{
	//Process commands sent to the mobile adapter
	switch(mobile_adapter.command)
	{

		//For certain commands, the mobile adapter mostly echoes the packet it received
		case 0x10:
		case 0x11:
			mobile_adapter.packet_size = 0;
			mobile_adapter.current_state = AGB_GBMA_ECHO_PACKET;

			//Echo packet needs to have the proper handshake with the adapter ID and command
			mobile_adapter.packet_buffer[mobile_adapter.packet_buffer.size() - 2] = 0x88;
			mobile_adapter.packet_buffer[mobile_adapter.packet_buffer.size() - 1] = 0x00;

			//Set command byte correctly
			mobile_adapter.packet_buffer[2] ^= 0x80;

			//Set checksum correctly
			{
				u16 packet_len = mobile_adapter.packet_buffer.size() - 2;
				u16 checksum = (mobile_adapter.packet_buffer[packet_len - 2] << 8);
				
				checksum |= mobile_adapter.packet_buffer[packet_len - 1];
				checksum += 0x80;

				mobile_adapter.packet_buffer[packet_len - 2] = ((checksum >> 8) & 0xFF);
				mobile_adapter.packet_buffer[packet_len - 1] = (checksum & 0xFF);
			}

			//Line busy status
			mobile_adapter.line_busy = false;

			//Pad data acknowledgement section if necessary
			if(mobile_adapter.s32_mode)
			{
				u8 pad_length = 4 - (mobile_adapter.packet_buffer.size() & 0x3);
				for(u32 x = 0; x < pad_length; x++) { mobile_adapter.packet_buffer.push_back(0x00); }
			}
			
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

			//Pad data acknowledgement section if necessary
			if(mobile_adapter.s32_mode)
			{
				u8 pad_length = 4 - (mobile_adapter.packet_buffer.size() & 0x3);
				for(u32 x = 0; x < pad_length; x++) { mobile_adapter.packet_buffer.push_back(0x00); }
			}

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
			mobile_adapter.packet_buffer.push_back(0x03);

			//Body
			if(mobile_adapter.line_busy) { mobile_adapter.packet_buffer.push_back(0x05); }
			else { mobile_adapter.packet_buffer.push_back(0x00); }
			
			mobile_adapter.packet_buffer.push_back(0x4D);
			mobile_adapter.packet_buffer.push_back(0x00);

			//Pad data section if necessary
			if(mobile_adapter.s32_mode)
			{
				u8 pad_length = 4 - (mobile_adapter.packet_buffer[5] & 0x3);
				for(u32 x = 0; x < pad_length; x++) { mobile_adapter.packet_buffer.push_back(0x00); }
			}

			//Checksum
			mobile_adapter.packet_buffer.push_back(0x00);

			if(mobile_adapter.line_busy) { mobile_adapter.packet_buffer.push_back(0xEC); }
			else { mobile_adapter.packet_buffer.push_back(0xE7); }

			//Acknowledgement handshake
			mobile_adapter.packet_buffer.push_back(0x88);
			mobile_adapter.packet_buffer.push_back(0x00);

			//Pad data acknowledgement section if necessary
			if(mobile_adapter.s32_mode)
			{
				u8 pad_length = 4 - (mobile_adapter.packet_buffer.size() & 0x3);
				for(u32 x = 0; x < pad_length; x++) { mobile_adapter.packet_buffer.push_back(0x00); }
			}

			//Send packet back
			mobile_adapter.packet_size = 0;
			mobile_adapter.current_state = AGB_GBMA_ECHO_PACKET;

			break;

		//SIO32 Mode Switch
		case 0x18:
			//Switch to NORMAL32 or NORMAL8 mode depending on data received
			if((mobile_adapter.packet_buffer[6] == 0x01) && (!mobile_adapter.s32_mode)) { mobile_adapter.switch_mode = true; }
			else if((mobile_adapter.packet_buffer[6] == 0x00) && (mobile_adapter.s32_mode)) { mobile_adapter.switch_mode = true; }
			else { mobile_adapter.switch_mode = false; }

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

				//Pad data section if necessary
				if(mobile_adapter.s32_mode)
				{
					u8 pad_length = 4 - (mobile_adapter.packet_buffer[5] & 0x3);
					for(u32 x = 0; x < pad_length; x++) { mobile_adapter.packet_buffer.push_back(0x00); }
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

				//Pad data acknowledgement section if necessary
				if(mobile_adapter.s32_mode)
				{
					u8 pad_length = 4 - (mobile_adapter.packet_buffer.size() & 0x3);
					for(u32 x = 0; x < pad_length; x++) { mobile_adapter.packet_buffer.push_back(0x00); }
				}
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

			//Body - 0.0.0.0
			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0x00);

			//Checksum
			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0xA5);

			//Acknowledgement handshake
			mobile_adapter.packet_buffer.push_back(0x88);
			mobile_adapter.packet_buffer.push_back(0x00);

			//Pad data acknowledgement section if necessary
			if(mobile_adapter.s32_mode)
			{
				u8 pad_length = 4 - (mobile_adapter.packet_buffer.size() & 0x3);
				for(u32 x = 0; x < pad_length; x++) { mobile_adapter.packet_buffer.push_back(0x00); }
			}

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
			mobile_adapter.packet_buffer.push_back(0x00);

			//Pad data section if necessary
			if(mobile_adapter.s32_mode)
			{
				u8 pad_length = 4 - (mobile_adapter.packet_buffer[5] & 0x3);
				for(u32 x = 0; x < pad_length; x++) { mobile_adapter.packet_buffer.push_back(0x00); }
			}

			//Checksum
			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0xA4);

			//Acknowledgement handshake
			mobile_adapter.packet_buffer.push_back(0x88);
			mobile_adapter.packet_buffer.push_back(0x00);

			//Pad data acknowledgement section if necessary
			if(mobile_adapter.s32_mode)
			{
				u8 pad_length = 4 - (mobile_adapter.packet_buffer.size() & 0x3);
				for(u32 x = 0; x < pad_length; x++) { mobile_adapter.packet_buffer.push_back(0x00); }
			}

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
			mobile_adapter.packet_buffer.push_back(0x00);

			//Pad data section if necessary
			if(mobile_adapter.s32_mode)
			{
				u8 pad_length = 4 - (mobile_adapter.packet_buffer[5] & 0x3);
				for(u32 x = 0; x < pad_length; x++) { mobile_adapter.packet_buffer.push_back(0x00); }
			}

			//Checksum
			mobile_adapter.packet_buffer.push_back(0x00);
			mobile_adapter.packet_buffer.push_back(0xA5);

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

			//Pad data acknowledgement section if necessary
			if(mobile_adapter.s32_mode)
			{
				u8 pad_length = 4 - (mobile_adapter.packet_buffer.size() & 0x3);
				for(u32 x = 0; x < pad_length; x++) { mobile_adapter.packet_buffer.push_back(0x00); }
			}

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
			mobile_adapter.packet_buffer.push_back(0xCC);

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
void AGB_SIO::mobile_adapter_process_http()
{
	std::string http_response = "";
	std::string http_header = "";
	u8 response_id = 0;
	bool not_found = true;
	bool needs_auth = false;
	bool img = mobile_adapter.http_data.find(".bmp") != std::string::npos;
	bool html = mobile_adapter.http_data.find(".html") != std::string::npos;

	//Build HTTP request
	mobile_adapter.http_data += util::data_to_str(mobile_adapter.packet_buffer.data() + 7, mobile_adapter.packet_buffer.size() - 11);

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
				//Open TCP connection
				if(!mobile_adapter_open_tcp(config::gbma_server_http_port))
				{
					mobile_adapter.transfer_state = 1;
					not_found = true;
				}

				else { not_found = false; }

				#ifdef GBE_NETPLAY

				//Isolate requested URL
				std::size_t req_start = mobile_adapter.http_data.find("GET ");
				std::string req_str = "";
				std::string rep_str = "";
				
				if((req_start != std::string::npos) && (!not_found))
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
				if(!req_str.empty() && sender.host_init && !not_found)
				{
					std::cout<<"SIO::Requesting URI from GB Mobile Adapter server - " << req_str << "\n";

					req_str = "GET " + req_str + " HTTP/1.0\r\n\r\n";
					rep_str = "";
					u32 msg_size = req_str.size() + 1;
					int response_size = -1;
					u8 response_data[0x8000];
					u8 timeout = 10;

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

				}

				#endif

				//Close TCP connection
				mobile_adapter_close_tcp();
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

	//Pad data section if necessary
	if(mobile_adapter.s32_mode)
	{
		u8 pad_length = 4 - (mobile_adapter.packet_buffer[5] & 0x3);
		for(u32 x = 0; x < pad_length; x++) { mobile_adapter.packet_buffer.push_back(0x00); }
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

	//Pad data acknowledgement section if necessary
	if(mobile_adapter.s32_mode)
	{
		u8 pad_length = 4 - (mobile_adapter.packet_buffer.size() & 0x3);
		for(u32 x = 0; x < pad_length; x++) { mobile_adapter.packet_buffer.push_back(0x00); }
	}
}

/****** Processes SMTP transfers from the emulated GB Mobile Adapter ******/
void AGB_SIO::mobile_adapter_process_smtp()
{
	std::string smtp_response = "";
	std::string smtp_data = util::data_to_str(mobile_adapter.packet_buffer.data(), mobile_adapter.packet_buffer.size());
	u8 response_id = 0;
	u8 smtp_command = 0xFF;
	u8 min_data_len = (mobile_adapter.s32_mode) ? 4 : 1;

	//Check for SMTP initiation
	if((mobile_adapter.data_length == min_data_len) && (!mobile_adapter.transfer_state) && (!mobile_adapter.smtp_session_started))
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

	//Pad data section if necessary
	if(mobile_adapter.s32_mode)
	{
		u8 pad_length = 4 - (mobile_adapter.packet_buffer[5] & 0x3);
		for(u32 x = 0; x < pad_length; x++) { mobile_adapter.packet_buffer.push_back(0x00); }
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

	//Pad data acknowledgement section if necessary
	if(mobile_adapter.s32_mode)
	{
		u8 pad_length = 4 - (mobile_adapter.packet_buffer.size() & 0x3);
		for(u32 x = 0; x < pad_length; x++) { mobile_adapter.packet_buffer.push_back(0x00); }
	}
}

/***** Loads a list of web addresses and points to local files in GBE+ data folder ******/
bool AGB_SIO::mobile_adapter_load_server_list()
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
		std::cout<<"SIO::Error - Could not open Mobile Adapter GB internal server list\n";
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

	std::cout<<"SIO::Loaded Mobile Adapter GB internal server list\n";

	file.close();
	return true;
}

/****** Opens a TCP connection to a Mobile Adapter GB server ******/
bool AGB_SIO::mobile_adapter_open_tcp(u16 port)
{
	bool result = false;

	#ifdef GBE_NETPLAY

	sender.host_socket = NULL;
	sender.host_init = false;
	sender.connected = false;
	sender.port = port;

	//Resolve hostname
	if(SDLNet_ResolveHost(&sender.host_ip, config::gbma_server.c_str(), sender.port) < 0)
	{
		std::cout<<"SIO::Error - Could not resolve address of GB Mobile Adapter server\n";
		return false;
	}

	//Open a connection to the server
	sender.host_socket = SDLNet_TCP_Open(&sender.host_ip);

	if(sender.host_socket == NULL)
	{
		std::cout<<"SIO::Error - Could not connect to GB Mobile Adapter server\n";
		return false;
	}

	sender.host_init = true;
	result = true;

	#endif

	return result;
}

/****** Closes a TCP connection to a Mobile Adapter GB server ******/
void AGB_SIO::mobile_adapter_close_tcp()
{
	#ifdef GBE_NETPLAY

	SDLNet_TCP_DelSocket(tcp_sockets, sender.host_socket);
	SDLNet_TCP_Close(sender.host_socket);

	#endif
}
