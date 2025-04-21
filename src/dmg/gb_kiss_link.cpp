// GB Enhanced Copyright Daniel Baxter 2025
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gb_kiss_link.cpp
// Date : March 5, 2025
// Description : GB KISS LINK emulation
//
// Emulates the GB KISS LINK (HC-749)
// Sends and receives .GBF and Nectaris map files
// Works in conjunction with HuC-1 and HuC-3 carts

#include "sio.h"
#include "common/util.h" 

/****** Processes IR communication protocol for GB KISS LINK *****/
void DMG_MMU::gb_kiss_link_process()
{
	if(!kiss_link.is_running) { return; }

	switch(kiss_link.state)
	{
		case GKL_SEND:
		case GKL_SEND_CMD:
		case GKL_SEND_PING:
		case GKL_SEND_HANDSHAKE_AA:
		case GKL_SEND_HANDSHAKE_C3:
		case GKL_SEND_HANDSHAKE_55:
		case GKL_SEND_HANDSHAKE_3C:
			//Set HuC IR ON or OFF
			if(kiss_link.output_signals.back() & 0x01) { cart.huc_ir_input = 0x01; }
			else { cart.huc_ir_input = 0x00; }

			kiss_link.output_signals.pop_back();

			//Continue sending signals
			if(!kiss_link.output_signals.empty())
			{
				sio_stat->shifts_left = 1;
				sio_stat->shift_counter = 0;
				sio_stat->shift_clock = (kiss_link.output_signals.back() & 0xFFFFFFFE);
			}

			//End of sent signals - Process next state
			else
			{
				kiss_link.cycles = 0;
				kiss_link.input_signals.clear();

				sio_stat->shifts_left = 0;
				sio_stat->shift_counter = 0;
				sio_stat->shift_clock = 0;

				//Move to next phase of sender handshake
				if(kiss_link.state == GKL_SEND_HANDSHAKE_AA)
				{
					kiss_link.state = GKL_RECV_HANDSHAKE_55;
					kiss_link.is_locked = false;
				}

				//Move to last phase of sender handshake
				else if(kiss_link.state == GKL_SEND_HANDSHAKE_C3)
				{
					kiss_link.state = GKL_RECV_HANDSHAKE_3C;
					kiss_link.is_locked = false;
				}

				//Move to next phase of receiver handshake
				else if(kiss_link.state == GKL_SEND_HANDSHAKE_55)
				{
					kiss_link.state = GKL_RECV_HANDSHAKE_C3;
					kiss_link.is_locked = false;
				}

				//Move to last phase of receiver handshake
				else if(kiss_link.state == GKL_SEND_HANDSHAKE_3C)
				{
					kiss_link.is_locked = false;

					//Receiver starts initial ping
					if(!kiss_link.is_sender)
					{
						gb_kiss_link_send_ping(GKL_ON_PING_RECEIVER, GKL_OFF_PING_RECEIVER);
						kiss_link.input_data.clear();
					}

					//Receive data
					else if((kiss_link.cmd == GKL_CMD_SEND_ICON) || (kiss_link.cmd == GKL_CMD_FILE_SEARCH)
					|| (kiss_link.cmd == GKL_CMD_MANAGE_UPLOAD) || (kiss_link.cmd == GKL_CMD_READ_RAM)
					|| (kiss_link.cmd == GKL_CMD_READ_SRAM) || (kiss_link.cmd == GKL_CMD_MANAGE_DATA))
					{
						kiss_link.output_data.clear();
						gb_kiss_link_send_ping(GKL_ON_PING_SENDER, GKL_OFF_PING_SENDER);

						//Some commands require receiving different lengths than they send
						//Adjust length here as needed in these cases
						if((kiss_link.cmd == GKL_CMD_SEND_ICON) || (kiss_link.cmd == GKL_CMD_FILE_SEARCH))
						{
							kiss_link.len = 265;
						}

						else if((kiss_link.cmd == GKL_CMD_MANAGE_UPLOAD) || (kiss_link.cmd == GKL_CMD_MANAGE_DATA))
						{
							kiss_link.len = 8;
						}
					}
				}

				//Send next ping
				else if(kiss_link.state == GKL_SEND_CMD)
				{
					if(!kiss_link.output_data.empty())
					{
						//When sending data bytes after a command, the ping is slightly delayed.
						if(kiss_link.output_data.size() == kiss_link.data_len)
						{
							kiss_link.is_ping_delayed = true;
						}

						gb_kiss_link_send_ping(GKL_ON_PING_SENDER, GKL_OFF_PING_SENDER);
					}

					//At end of ping, move onto next state, stage, or command
					else
					{
						gb_kiss_link_process_ping();
					}
				}

				else if(kiss_link.state == GKL_SEND_PING)
				{
					//Send next byte in command
					if(!kiss_link.output_data.empty())
					{
						kiss_link.state = GKL_SEND_CMD;
						kiss_link.output_signals.clear();
						kiss_link.cycles = 0;
						kiss_link.is_locked = true;

						//Dummy zero pulse
						kiss_link.output_signals.push_back(GKL_OFF_SHORT);
						kiss_link.output_signals.push_back(GKL_ON);

						//Data byte pulses
						gb_kiss_link_set_signal(kiss_link.output_data.front());
						kiss_link.output_data.erase(kiss_link.output_data.begin());

						sio_stat->shifts_left = 1;
						sio_stat->shift_counter = 0;
						sio_stat->shift_clock = (kiss_link.output_signals.back() & 0xFFFFFFFE);
					}

					//Receive next byte for command response (sender)
					//Begin receiving command bytes (receiver)
					else
					{
						//Send icon data after long ping
						if(kiss_link.stage == GKL_START_SESSION)
						{
							kiss_link.stage = GKL_SEND_ICON;
							gb_kiss_link_handshake(0xAA);
						}

						else
						{
							kiss_link.is_locked = false;
							kiss_link.state = (kiss_link.is_sender) ? GKL_RECV_DATA : GKL_RECV_CMD;
						}
					}
				}
			}

			break;

		case GKL_RECV:
		case GKL_RECV_CMD:
		case GKL_RECV_PING:
		case GKL_RECV_DATA:
		case GKL_RECV_HANDSHAKE_55:
		case GKL_RECV_HANDSHAKE_3C:
		case GKL_RECV_HANDSHAKE_AA:
		case GKL_RECV_HANDSHAKE_C3:
			//End handshake
			if(((kiss_link.state == GKL_RECV_HANDSHAKE_55) || (kiss_link.state == GKL_RECV_HANDSHAKE_3C)
			|| (kiss_link.state == GKL_RECV_HANDSHAKE_AA) || (kiss_link.state == GKL_RECV_HANDSHAKE_C3))
			&& (kiss_link.input_signals.size() == 9))
			{
				gb_kiss_link_get_bytes();
				kiss_link.input_signals.clear();

				//Finish first phase of sender handshake
				if(kiss_link.state == GKL_RECV_HANDSHAKE_55)
				{
					gb_kiss_link_handshake(0xC3);
				}

				//Finish first phase of receiver handshake
				else if(kiss_link.state == GKL_RECV_HANDSHAKE_AA)
				{
					gb_kiss_link_handshake(0x55);
				}

				//Finish last phase of receiver handshake
				else if(kiss_link.state == GKL_RECV_HANDSHAKE_C3)
				{
					gb_kiss_link_handshake(0x3C);
					kiss_link.input_data.clear();
				}

				//Finish last phase of sender handshake, move onto next command
				else if(kiss_link.state == GKL_RECV_HANDSHAKE_3C)
				{
					gb_kiss_link_process_command();
				}

				//Finish first phase of receiver handshake
				else if(kiss_link.state == GKL_RECV_HANDSHAKE_AA)
				{
					gb_kiss_link_handshake(0x55);
				}
			}

			//Receive command data (receiver)
			else if((kiss_link.state == GKL_RECV_CMD) && (kiss_link.input_signals.size() == 9))
			{
				gb_kiss_link_get_bytes();
				kiss_link.input_signals.clear();
				gb_kiss_link_send_ping(GKL_ON_PING_RECEIVER, GKL_OFF_PING_RECEIVER);

				//Grab command - 10th byte in stream ("Hu0" + 8 command bytes)
				if(kiss_link.input_data.size() == 10)
				{
					kiss_link.cmd = kiss_link.input_data[3];
					
					//Determine total length of bytes in command
					switch(kiss_link.cmd)
					{
						case GKL_CMD_READ_RAM:
						case GKL_CMD_MANAGE_SESSION:
						case GKL_CMD_MANAGE_UPLOAD:
							kiss_link.len = 11;
							break;

						case GKL_CMD_FILE_SEARCH:
							kiss_link.len = 268;
							break;

						case GKL_CMD_MANAGE_DATA:
							kiss_link.len = 12 + kiss_link.input_data[8];
							if(!kiss_link.input_data[8]) { kiss_link.len += 256; }
							break;

						case GKL_CMD_SEND_ICON:
						case GKL_CMD_WRITE_RAM:
							kiss_link.len = 12 + kiss_link.input_data[8];
							break;
					}

					gb_kiss_link_send_ping(41, 100);
				}
			}

			//Receive checksum after command data (receiver)
			else if((kiss_link.state == GKL_RECV_CMD) && (kiss_link.input_data.size() >= 10)
			&& ((kiss_link.input_data.size() + 1) == kiss_link.len) && (kiss_link.input_signals.size() == 8))
			{
				gb_kiss_link_get_bytes();
				kiss_link.input_signals.clear();
				gb_kiss_link_finish_command();
			}

			//Receive data from commands (sender)
			else if((kiss_link.state == GKL_RECV_DATA) && (kiss_link.input_signals.size() == 9))
			{
				gb_kiss_link_get_bytes();
				kiss_link.input_signals.clear();
				gb_kiss_link_send_ping(GKL_ON_PING_SENDER, GKL_OFF_PING_SENDER);
			}

			//Finish receiving data from commands (sender)
			else if((kiss_link.state == GKL_RECV_DATA) && (kiss_link.input_signals.size() == 8)
			&& (kiss_link.input_data.size() == kiss_link.len) && (kiss_link.is_sender))
			{
				gb_kiss_link_get_bytes();
				kiss_link.input_signals.clear();
				gb_kiss_link_finish_command();
			}

			//Continue gathering cycle counts for incoming signals
			else
			{
				sio_stat->shifts_left = 1;
				sio_stat->shift_counter = 0;
				sio_stat->shift_clock = 0;
			}

			break;
	}
}

/****** Sets up various commands to be sent via IR ******/
void DMG_MMU::gb_kiss_link_process_command()
{
	u32 size = 0;
	u32 start = 0;
	u32 end = 0;
	u8 sum = 0;
	std::string temp_str = "";

	switch(kiss_link.stage)
	{
		//Read RAM -> Receiver ID String
		case GKL_INIT_SENDER:
			kiss_link.stage = GKL_REQUEST_ID;

			kiss_link.cmd = GKL_CMD_READ_RAM;
			kiss_link.local_addr = 0xCE00;
			kiss_link.remote_addr = 0xCE00;
			kiss_link.len = 0x10;
			kiss_link.param = 0;

			kiss_link.input_data.clear();
			gb_kiss_link_send_command();
			
			break;

		//Write RAM -> Receiver ID
		case GKL_WRITE_ID:
			kiss_link.cmd = GKL_CMD_WRITE_RAM;
			kiss_link.local_addr = 0xCE00;
			kiss_link.remote_addr = 0xCE00;
			kiss_link.len = 0x01;
			kiss_link.param = 0;

			//Command data
			kiss_link.input_data.clear();
			kiss_link.input_data.push_back(0x01);

			gb_kiss_link_send_command();
			
			break;

		//Start Session
		//End Session
		case GKL_START_SESSION:
		case GKL_END_SESSION:
			kiss_link.cmd = GKL_CMD_MANAGE_SESSION;
			kiss_link.local_addr = 0xCE00;
			kiss_link.remote_addr = 0xCE00;
			kiss_link.len = 0x01;
			kiss_link.param = 0;

			kiss_link.input_data.clear();
			gb_kiss_link_send_command();

			break;

		//Send Icon
		case GKL_SEND_ICON:
			kiss_link.cmd = GKL_CMD_SEND_ICON;
			kiss_link.local_addr = 0xC700;
			kiss_link.remote_addr = 0xC50C;
			kiss_link.len = kiss_link.gbf_title_icon_size;
			kiss_link.param = 0;

			//Command data
			kiss_link.input_data.clear();
			size = (kiss_link.gbf_title_icon_size + 5);
						
			for(u32 x = 5; x < size; x++)
			{
				kiss_link.input_data.push_back(kiss_link.gbf_data[x]);
			}

			gb_kiss_link_send_command();
			
			break;

		//Unknown RAM Write #1
		case GKL_UNK_WRITE_1:
			kiss_link.cmd = GKL_CMD_WRITE_RAM;
			kiss_link.local_addr = 0xCE00;
			kiss_link.remote_addr = 0xCE00;
			kiss_link.len = 0x01;
			kiss_link.param = kiss_link.slot;

			//Command data
			kiss_link.input_data.clear();
			kiss_link.input_data.push_back(0x05);

			gb_kiss_link_send_command();
			
			break;

		//File Search
		case GKL_FILE_SEARCH:
			kiss_link.cmd = GKL_CMD_FILE_SEARCH;
			kiss_link.local_addr = 0xC700;
			kiss_link.param = kiss_link.gbf_flags;

			//Command data
			kiss_link.input_data.clear();

			gb_kiss_link_send_command();
			
			break;

		//Unknown RAM Read #1
		case GKL_UNK_READ_1:
			kiss_link.cmd = GKL_CMD_READ_RAM;
			kiss_link.local_addr = 0xCE00;
			kiss_link.remote_addr = 0xDFFC;
			kiss_link.len = 0x02;
			kiss_link.param = 0x76;

			kiss_link.input_data.clear();
			gb_kiss_link_send_command();

			break;

		//Prep file upload from GB KISS LINK
		case GKL_PREP_UPLOAD:
			kiss_link.cmd = GKL_CMD_MANAGE_UPLOAD;
			kiss_link.local_addr = 0xC500;
			kiss_link.remote_addr = 0xFFD2;
			kiss_link.param = 0x0;

			kiss_link.input_data.clear();
			gb_kiss_link_send_command();

			break;

		//Send History
		case GKL_SEND_HISTORY:
			kiss_link.cmd = GKL_CMD_MANAGE_DATA;
			kiss_link.local_addr = 0xC500;
			kiss_link.remote_addr = 0xC600;
			kiss_link.len = 0x2E;
			kiss_link.param = 0;

			//Command data
			kiss_link.input_data.clear();
			start = (kiss_link.gbf_title_icon_size + 5);
			end = start + 0x2E;
						
			for(u32 x = start; x < end; x++)
			{
				kiss_link.input_data.push_back(kiss_link.gbf_data[x]);
			}

			gb_kiss_link_send_command();
			
			break;

		//Send File
		case GKL_SEND_FILE:
			kiss_link.cmd = GKL_CMD_MANAGE_DATA;
			kiss_link.local_addr = 0xC500;
			kiss_link.remote_addr = 0xC600;
			kiss_link.len = 0;
			kiss_link.param = 1;

			//Command data
			kiss_link.input_data.clear();
			start = kiss_link.gbf_index;
			end = start + 256;

			//End of file reached on this command
			if(end > kiss_link.gbf_file_size)
			{
				end = kiss_link.gbf_file_size;
				kiss_link.len = kiss_link.gbf_file_size - start;
				kiss_link.param = 0;
				kiss_link.is_upload_done = true;
			} 
						
			for(u32 x = start; x < end; x++)
			{
				kiss_link.input_data.push_back(kiss_link.gbf_data[x]);
				kiss_link.gbf_index++;
			}

			gb_kiss_link_send_command();

			break;

		//Finish Upload
		case GKL_END_UPLOAD:
			kiss_link.cmd = GKL_CMD_MANAGE_UPLOAD;
			kiss_link.local_addr = 0xC50B;
			kiss_link.remote_addr = 0x3 - (kiss_link.gbf_file_size - kiss_link.gbf_raw_size);
			kiss_link.len = 0;
			kiss_link.param = 0;

			kiss_link.input_data.clear();
			gb_kiss_link_send_command();

			break;

		//Close File
		case GKL_CLOSE_FILE:
			kiss_link.cmd = GKL_CMD_MANAGE_DATA;
			kiss_link.local_addr = 0xC500;
			kiss_link.remote_addr = 0xC50B;
			kiss_link.len = 0x1;
			kiss_link.param = 0;

			//Command data
			kiss_link.input_data.clear();

			//Cartridge Code
			kiss_link.input_data.push_back(kiss_link.gbf_data[3]);

			gb_kiss_link_send_command();
			
			break;

		//Unknown RAM Write #2
		case GKL_UNK_WRITE_2:
			kiss_link.cmd = GKL_CMD_WRITE_RAM;
			kiss_link.local_addr = 0xCE00;
			kiss_link.remote_addr = 0xCE00;
			kiss_link.len = 0x01;
			kiss_link.param = 0x00;

			//Command data
			kiss_link.input_data.clear();
			kiss_link.input_data.push_back(0x02);

			gb_kiss_link_send_command();
			
			break;

		//Write RAM -> Send receiver ID
		case GKL_SEND_ID:
			temp_str = "GB KISS MENU ";

			//Command data
			kiss_link.input_data.clear();
			kiss_link.input_data.push_back(0x00);

			//Slot #
			kiss_link.input_data.push_back(0x00);

			for(u32 x = 0; x < 13; x++)
			{
				u8 chr = temp_str[x];
				kiss_link.input_data.push_back(chr);
			}

			kiss_link.input_data.push_back(0x00);

			gb_kiss_link_recv_command();

			break;

		//Get Icon from sender
		case GKL_GET_ICON:
			kiss_link.gbf_index = 0;
			kiss_link.gbf_file_size = 0;
			kiss_link.gbf_raw_size = 0;
			kiss_link.gbf_title_icon_size = kiss_link.input_data[0x08];
			kiss_link.gbf_flags = 0;
			kiss_link.gbf_data.clear();

			start = 11;
			end = start + kiss_link.input_data[0x08];

			kiss_link.gbf_data.push_back(kiss_link.gbf_title_icon_size);

			//Grab Title+Icon from sender data stream
			for(u32 x = start; x < end; x++)
			{
				kiss_link.gbf_data.push_back(kiss_link.input_data[x]);
			}

			//Command Data
			kiss_link.input_data.clear();

			//Transfer Status
			kiss_link.input_data.push_back(0x90);
			kiss_link.input_data.push_back(0x00);

			//Local Addr - 0xC500
			kiss_link.input_data.push_back(0x00);
			kiss_link.input_data.push_back(0xC5);

			//Length + 0xC4?
			kiss_link.input_data.push_back(kiss_link.gbf_title_icon_size);
			kiss_link.input_data.push_back(0xC4);
			kiss_link.input_data.push_back(0x00);
			kiss_link.input_data.push_back(0x00);

			//Checksum
			for(u32 x = 0; x < kiss_link.input_data.size(); x++)
			{
				sum += kiss_link.input_data[x];
			}
				
			sum = ~sum;
			sum++;
			kiss_link.input_data.push_back(sum);

			//File Search Input Data - Unknown
			kiss_link.input_data.push_back(0x00);
			kiss_link.input_data.push_back(0x00);
			kiss_link.input_data.push_back(0xFF);
			kiss_link.input_data.push_back(0x0F);
			kiss_link.input_data.push_back(0x00);
			kiss_link.input_data.push_back(0x0F);
			kiss_link.input_data.push_back(0x00);
			kiss_link.input_data.push_back(0x1E);
			kiss_link.input_data.push_back(0x00);
			kiss_link.input_data.push_back(0x06);
			kiss_link.input_data.push_back(0xFE);

			kiss_link.input_data.push_back(kiss_link.gbf_title_icon_size);

			//Echo of Title+Icon
			for(u32 x = 0; x < kiss_link.gbf_data.size(); x++)
			{
				kiss_link.input_data.push_back(kiss_link.gbf_data[x]);
			}

			//File Search Input Data - Padding
			while(kiss_link.input_data.size() < 265)
			{
				kiss_link.input_data.push_back(0x00);
			}

			//Total Data Checksum
			sum = 0;

			for(u32 x = 0; x < kiss_link.input_data.size(); x++)
			{
				sum += kiss_link.input_data[x];
			}
				
			sum = ~sum;
			sum++;
			kiss_link.input_data.push_back(sum);

			gb_kiss_link_recv_command();

			break;

		case GKL_ACK_SEARCH:
			//Command Data
			kiss_link.input_data.clear();

			//Transfer Status
			kiss_link.input_data.push_back(0x80);
			kiss_link.input_data.push_back(0x00);

			//Local Addr - 0xC500
			kiss_link.input_data.push_back(0x00);
			kiss_link.input_data.push_back(0xC5);

			//Remote Addr - 0xC50C
			kiss_link.input_data.push_back(0x0C);
			kiss_link.input_data.push_back(0xC5);

			//Unknown Parameter (used for next Read RAM command by sender), random values used here
			kiss_link.input_data.push_back(0x00);
			kiss_link.input_data.push_back(0x00);

			//Checksum
			for(u32 x = 0; x < kiss_link.input_data.size(); x++)
			{
				sum += kiss_link.input_data[x];
			}
				
			sum = ~sum;
			sum++;
			kiss_link.input_data.push_back(sum);

			//Unknown Parameter, random values used here
			kiss_link.input_data.push_back(0x00);
			kiss_link.input_data.push_back(0x00);

			//Game Data Offset
			size = 5 + kiss_link.gbf_title_icon_size;
			if(kiss_link.gbf_flags & 0x01) { size += 46; }
			size &= 0xFFFF;

			kiss_link.input_data.push_back(size >> 8);
			kiss_link.input_data.push_back(size);
			kiss_link.input_data.push_back(size >> 8);
			kiss_link.input_data.push_back(size);
			kiss_link.input_data.push_back(0x00);

			//Total File Size
			kiss_link.gbf_file_size = size + kiss_link.gbf_raw_size;
			kiss_link.input_data.push_back(kiss_link.gbf_file_size);
			kiss_link.input_data.push_back(kiss_link.gbf_file_size >> 8);

			//Flags
			kiss_link.input_data.push_back(kiss_link.gbf_flags);

			//Title+Icon Size
			kiss_link.input_data.push_back(0xFF);
			kiss_link.input_data.push_back(kiss_link.gbf_title_icon_size);

			//Echo of Title+Icon
			for(u32 x = 0; x < kiss_link.gbf_data.size(); x++)
			{
				kiss_link.input_data.push_back(kiss_link.gbf_data[x]);
			}	

			//File Search Input Data - Padding
			while(kiss_link.input_data.size() < 265)
			{
				kiss_link.input_data.push_back(0x00);
			}

			//Total Data Checksum
			sum = 0;

			for(u32 x = 0; x < kiss_link.input_data.size(); x++)
			{
				sum += kiss_link.input_data[x];
			}
				
			sum = ~sum;
			sum++;
			kiss_link.input_data.push_back(sum);

			gb_kiss_link_recv_command();

			break;

		case GKL_SEND_UNK_DATA_1:
			//Command Data
			kiss_link.input_data.clear();

			//Unknown. Seems constant?
			kiss_link.input_data.push_back(0x05);

			//Slot #
			kiss_link.input_data.push_back(0x00);

			gb_kiss_link_recv_command();

			break;

		case GKL_UPLOAD_READY:
			//Command Data
			kiss_link.input_data.clear();

			//Transfer Status
			kiss_link.input_data.push_back(0x40);
			kiss_link.input_data.push_back(0x01);

			//Remote Address
			kiss_link.input_data.push_back(0x00);
			kiss_link.input_data.push_back(0xC5);

			//Local Addr
			kiss_link.input_data.push_back(0xD2);
			kiss_link.input_data.push_back(0xFF);

			//Game Data Offset
			size = 5 + kiss_link.gbf_title_icon_size;
			if(kiss_link.gbf_flags & 0x01) { size += 46; }
			size &= 0xFFFF;

			kiss_link.input_data.push_back(size);
			kiss_link.input_data.push_back(size >> 8);

			//Checksum
			for(u32 x = 0; x < kiss_link.input_data.size(); x++)
			{
				sum += kiss_link.input_data[x];
			}
				
			sum = ~sum;
			sum++;
			kiss_link.input_data.push_back(sum);

			gb_kiss_link_recv_command();

			break;

		case GKL_GET_HISTORY:
			//Append history data to existing Title+Icon data
			start = 11;
			end = start + 46;

			for(u32 x = start; x < end; x++)
			{
				kiss_link.gbf_data.push_back(kiss_link.input_data[x]);
			}

			//Command Data
			kiss_link.input_data.clear();

			//Transfer Status
			kiss_link.input_data.push_back(0x80);
			kiss_link.input_data.push_back(0x00);

			//Remote Address
			kiss_link.input_data.push_back(0x00);
			kiss_link.input_data.push_back(0xC5);

			//History Length
			size = (kiss_link.gbf_flags & 0x01) ? 0x2E : 0x00;
			kiss_link.input_data.push_back(size);

			//Unknown - Constant?
			kiss_link.input_data.push_back(0xC4);

			//History Length again
			kiss_link.input_data.push_back(0x2E);
			kiss_link.input_data.push_back(0x00);

			//Checksum
			for(u32 x = 0; x < kiss_link.input_data.size(); x++)
			{
				sum += kiss_link.input_data[x];
			}
				
			sum = ~sum;
			sum++;
			kiss_link.input_data.push_back(sum);

			gb_kiss_link_recv_command();

			break;

		case GKL_GET_FILE:
			kiss_link.is_upload_done = (kiss_link.input_data[0x08]) ? true : false;

			//Append game data to existing GBF
			start = 11;
			
			if(kiss_link.is_upload_done) { end = start + kiss_link.input_data[0x08]; }
			else { end = start + 256; }

			for(u32 x = start; x < end; x++)
			{
				kiss_link.gbf_data.push_back(kiss_link.input_data[x]);
			}

			//Command Data
			kiss_link.input_data.clear();

			//Transfer Status
			kiss_link.input_data.push_back(0x80);
			kiss_link.input_data.push_back(0x00);

			//Remote Address
			kiss_link.input_data.push_back(0x00);
			kiss_link.input_data.push_back(0xC5);

			if(!kiss_link.is_upload_done)
			{
				kiss_link.input_data.push_back(0x00);
				kiss_link.input_data.push_back(0xC5);
				kiss_link.input_data.push_back(0x00);
				kiss_link.input_data.push_back(0x01);
			}

			else
			{
				kiss_link.input_data.push_back(0x03);
				kiss_link.input_data.push_back(0xC4);
				kiss_link.input_data.push_back(0x03);
				kiss_link.input_data.push_back(0x00);
			}

			//Checksum
			for(u32 x = 0; x < kiss_link.input_data.size(); x++)
			{
				sum += kiss_link.input_data[x];
			}
				
			sum = ~sum;
			sum++;
			kiss_link.input_data.push_back(sum);

			gb_kiss_link_recv_command();

			break;

		case GKL_UPLOAD_DONE:
			//Command Data
			kiss_link.input_data.clear();

			//Transfer Status
			kiss_link.input_data.push_back(0x40);
			kiss_link.input_data.push_back(0x01);

			//Remote Address
			kiss_link.input_data.push_back(0x00);
			kiss_link.input_data.push_back(0xC5);

			//Sizes
			size = 0x03 - (kiss_link.gbf_file_size - kiss_link.gbf_raw_size);
			size &= 0xFFFF;

			kiss_link.input_data.push_back(size);
			kiss_link.input_data.push_back(size >> 8);

			//Game Data Offset
			size = 5 + kiss_link.gbf_title_icon_size;
			if(kiss_link.gbf_flags & 0x01) { size += 46; }
			size &= 0xFFFF;

			kiss_link.input_data.push_back(size);
			kiss_link.input_data.push_back(size >> 8);

			//Checksum
			for(u32 x = 0; x < kiss_link.input_data.size(); x++)
			{
				sum += kiss_link.input_data[x];
			}
				
			sum = ~sum;
			sum++;
			kiss_link.input_data.push_back(sum);

			gb_kiss_link_recv_command();

			break;

		case GKL_ACK_FILE_CLOSE:
			//Build final GBF file and save
			kiss_link.cart_code = kiss_link.input_data[0x0B];
			gb_kiss_link_save_file();

			//Command Data
			kiss_link.input_data.clear();

			//Transfer Status
			kiss_link.input_data.push_back(0x80);
			kiss_link.input_data.push_back(0x00);

			//Remote Addr
			kiss_link.input_data.push_back(0x00);
			kiss_link.input_data.push_back(0xC5);

			//Unknown Data - Constant?
			kiss_link.input_data.push_back(0x01);
			kiss_link.input_data.push_back(0xC4);
			kiss_link.input_data.push_back(0x01);
			kiss_link.input_data.push_back(0x00);

			//Checksum (precalculated)
			kiss_link.input_data.push_back(0xF5);

			gb_kiss_link_recv_command();

			break;

		case GKL_INIT_MAP_SENDER:
		case GKL_INIT_MAP_RECEIVER:
			kiss_link.stage = GKL_REQUEST_MAP_ID;

			kiss_link.cmd = GKL_CMD_READ_RAM;
			kiss_link.local_addr = 0xD000;
			kiss_link.remote_addr = 0xD000;
			kiss_link.len = 0x10;
			kiss_link.param = 0;

			kiss_link.input_data.clear();
			gb_kiss_link_send_command();
			
			break;

		case GKL_SEND_DATA_LO:
			kiss_link.cmd = GKL_CMD_WRITE_RAM;
			kiss_link.local_addr = 0xD000;
			kiss_link.remote_addr = 0xD000;
			kiss_link.len = 0;
			kiss_link.param = 0;

			//Command data
			kiss_link.input_data.clear();

			for(u32 x = 0; x < 256; x++)
			{
				kiss_link.input_data.push_back(kiss_link.gbf_data[x]);
			}

			gb_kiss_link_send_command();
			
			break;

		case GKL_SEND_DATA_HI:
			kiss_link.cmd = GKL_CMD_WRITE_RAM;
			kiss_link.local_addr = 0xD100;
			kiss_link.remote_addr = 0xD100;
			kiss_link.len = 0;
			kiss_link.param = 0;

			//Command data
			kiss_link.input_data.clear();

			for(u32 x = 256; x < 512; x++)
			{
				kiss_link.input_data.push_back(kiss_link.gbf_data[x]);
			}

			gb_kiss_link_send_command();
			
			break;

		case GKL_GET_DATA_LO:
			kiss_link.slot = kiss_link.input_data[6];
			kiss_link.cmd = GKL_CMD_READ_SRAM;
			kiss_link.local_addr = 0x4000 + (0x200 * kiss_link.slot);
			kiss_link.remote_addr = 0x4000 + (0x200 * kiss_link.slot);
			kiss_link.len = 0;
			kiss_link.param = 0;

			kiss_link.input_data.clear();
			gb_kiss_link_send_command();
			
			break;

		case GKL_GET_DATA_HI:
			kiss_link.cmd = GKL_CMD_READ_SRAM;
			kiss_link.local_addr = 0x4100 + (0x200 * kiss_link.slot);
			kiss_link.remote_addr = 0x4100 + (0x200 * kiss_link.slot);
			kiss_link.len = 0;
			kiss_link.param = 0;

			kiss_link.gbf_data.clear();

			end = (kiss_link.input_data.size() > 256) ? 256 : kiss_link.input_data.size();

			for(u32 x = 0; x < end; x++)
			{
				kiss_link.gbf_data.push_back(kiss_link.input_data[x]);
			}

			kiss_link.input_data.clear();
			gb_kiss_link_send_command();
			
			break;

		case GKL_FINISH_MAP:
			kiss_link.cmd = GKL_CMD_MANAGE_SESSION;
			kiss_link.local_addr = 0xD100;
			kiss_link.remote_addr = 0xD100;
			kiss_link.len = 0x00;
			kiss_link.param = 0;

			end = (kiss_link.input_data.size() > 256) ? 256 : kiss_link.input_data.size();

			for(u32 x = 0; x < end; x++)
			{
				kiss_link.gbf_data.push_back(kiss_link.input_data[x]);
			}

			gb_kiss_link_save_file();

			kiss_link.input_data.clear();
			gb_kiss_link_send_command();

			break;
	}
}

/****** Moves from one command to the next for GB KISS LINK protocol ******/
void DMG_MMU::gb_kiss_link_finish_command()
{
	switch(kiss_link.stage)
	{
		case GKL_REQUEST_ID:
			kiss_link.stage = GKL_WRITE_ID;
			gb_kiss_link_handshake(0xAA);

			kiss_link.slot = kiss_link.input_data[kiss_link.input_data.size() - 2];
			
			break;

		case GKL_WRITE_ID:
			kiss_link.stage = GKL_START_SESSION;
			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_SEND_ICON:
			kiss_link.file_search_data.clear();

			//Grab input used for file search
			if(kiss_link.input_data.size() == 266)
			{
				for(u32 x = 0; x < 256; x++)
				{
					kiss_link.file_search_data.push_back(kiss_link.input_data[9 + x]);
				}

				kiss_link.stage = GKL_UNK_WRITE_1;
				gb_kiss_link_handshake(0xAA);
			}

			else
			{
				std::cout<<"MMU::Warning - GB KISS LINK File Search Data is incorrect size\n";
			}
			
			break;

		case GKL_UNK_WRITE_1:
			kiss_link.stage = GKL_FILE_SEARCH;
			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_FILE_SEARCH:
			kiss_link.stage = GKL_UNK_READ_1;
			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_UNK_READ_1:
			kiss_link.stage = GKL_PREP_UPLOAD;
			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_PREP_UPLOAD:
			//TODO - This should probably be optional if GBF file does not have history
			//In that case, it would skip to the next stage? Needs more research
			kiss_link.stage = GKL_SEND_HISTORY;
			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_SEND_HISTORY:
			kiss_link.stage = GKL_SEND_FILE;
			kiss_link.is_upload_done = false;
			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_SEND_FILE:
			//Continue sending file in chunks until EOF is reached
			if(!kiss_link.is_upload_done)
			{
				kiss_link.stage = GKL_SEND_FILE;
				gb_kiss_link_handshake(0xAA);
			}

			else
			{
				kiss_link.stage = GKL_END_UPLOAD;
				gb_kiss_link_handshake(0xAA);
			}

			break;

		case GKL_END_UPLOAD:
			kiss_link.stage = GKL_CLOSE_FILE;
			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_CLOSE_FILE:
			kiss_link.stage = GKL_UNK_WRITE_2;
			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_UNK_WRITE_2:
			kiss_link.stage = GKL_END_SESSION;
			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_INIT_RECEIVER:
			kiss_link.stage = GKL_SEND_ID;
			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_SEND_ID:
			kiss_link.stage = GKL_GET_NEW_ID;

			//Echo data checksum
			kiss_link.output_data.clear();
			kiss_link.output_data.push_back(kiss_link.input_data.back());
			gb_kiss_link_send_ping(41, 100);

			break;

		case GKL_GET_NEW_ID:
			//New sessions (for receiver) need to wait for handshakes just like init 
			kiss_link.stage = GKL_ACK_NEW_SESSION;
			kiss_link.state = GKL_INACTIVE;
			kiss_link.is_locked = false;

			break;

		case GKL_ACK_NEW_SESSION:
			kiss_link.stage = GKL_GET_ICON;
			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_GET_ICON:
			kiss_link.stage = GKL_GET_UNK_DATA_1;

			//Echo data checksum
			kiss_link.output_data.clear();
			kiss_link.output_data.push_back(kiss_link.input_data.back());
			gb_kiss_link_send_ping(41, 100);

			break;

		case GKL_GET_UNK_DATA_1:
			kiss_link.stage = GKL_ACK_SEARCH;

			kiss_link.gbf_raw_size = (kiss_link.input_data[7] << 8) | kiss_link.input_data[6];
			kiss_link.gbf_flags = kiss_link.input_data[9];

			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_ACK_SEARCH:
			kiss_link.stage = GKL_SEND_UNK_DATA_1;
			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_SEND_UNK_DATA_1:
			kiss_link.stage = GKL_UPLOAD_READY;
			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_UPLOAD_READY:
			kiss_link.stage = GKL_GET_HISTORY;
			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_GET_HISTORY:
			kiss_link.stage = GKL_GET_FILE;
			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_GET_FILE:
			if(kiss_link.is_upload_done) { kiss_link.stage = GKL_UPLOAD_DONE; }
			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_UPLOAD_DONE:
			kiss_link.stage = GKL_ACK_FILE_CLOSE;
			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_ACK_FILE_CLOSE:
			kiss_link.stage = GKL_GET_UNK_DATA_2;
			
			//Echo data checksum
			kiss_link.output_data.clear();
			kiss_link.output_data.push_back(kiss_link.input_data.back());
			gb_kiss_link_send_ping(41, 100);

			break;

		case GKL_GET_UNK_DATA_2:
			kiss_link.stage = GKL_FINISHED;
			kiss_link.is_running = false;
			sio_stat->shifts_left = 0;
			sio_stat->shift_counter = 0;
			sio_stat->shift_clock = 0;

			std::cout<<"GB KISS LINK Transfer Finished\n";

			break;

		case GKL_REQUEST_MAP_ID:
			kiss_link.stage = (kiss_link.mode == 2) ? GKL_SEND_DATA_LO : GKL_GET_DATA_LO;
			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_SEND_DATA_LO:
			kiss_link.stage = GKL_SEND_DATA_HI;
			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_SEND_DATA_HI:
			kiss_link.stage = GKL_FINISH_MAP;
			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_GET_DATA_LO:
			kiss_link.stage = GKL_GET_DATA_HI;
			gb_kiss_link_handshake(0xAA);

			break;

		case GKL_GET_DATA_HI:
			kiss_link.stage = GKL_FINISH_MAP;
			gb_kiss_link_handshake(0xAA);

			break;
	}
}

/****** Handles behavior for various commands after each ping is sent ******/
void DMG_MMU::gb_kiss_link_process_ping()
{
	switch(kiss_link.stage)
	{
		//Receive data in response to command - Start receiving handshake
		case GKL_REQUEST_ID:
		case GKL_SEND_ICON:
		case GKL_FILE_SEARCH:
		case GKL_UNK_READ_1:
		case GKL_PREP_UPLOAD:
		case GKL_SEND_HISTORY:
		case GKL_SEND_FILE:
		case GKL_END_UPLOAD:
		case GKL_CLOSE_FILE:
		case GKL_SEND_ID:
		case GKL_GET_NEW_ID:
		case GKL_GET_ICON:
		case GKL_GET_UNK_DATA_1:
		case GKL_ACK_SEARCH:
		case GKL_SEND_UNK_DATA_1:
		case GKL_UPLOAD_READY:
		case GKL_GET_HISTORY:
		case GKL_GET_FILE:
		case GKL_UPLOAD_DONE:
		case GKL_ACK_FILE_CLOSE:
		case GKL_GET_UNK_DATA_2:
		case GKL_REQUEST_MAP_ID:
		case GKL_GET_DATA_LO:
		case GKL_GET_DATA_HI:
			kiss_link.state = GKL_RECV_HANDSHAKE_AA;
			kiss_link.is_locked = false;
			
			break;

		//Receive echo of checksum after RAM writes - Continue pings
		case GKL_WRITE_ID:
		case GKL_UNK_WRITE_1:
		case GKL_UNK_WRITE_2:
		case GKL_SEND_DATA_LO:
		case GKL_SEND_DATA_HI:
			gb_kiss_link_send_ping(GKL_ON_PING_SENDER, GKL_OFF_PING_SENDER);
			
			break;

		//Start session - Ping is super long due to client-side processing
		case GKL_START_SESSION:
			kiss_link.output_data.clear();
			gb_kiss_link_send_ping(GKL_ON_PING_SENDER, GKL_OFF_PING_SENDER);
			kiss_link.output_signals[0] = 1078400;
			
			break;

		//End of transfer
		case GKL_END_SESSION:
		case GKL_FINISH_MAP:
			kiss_link.stage = GKL_FINISHED;
			kiss_link.is_running = false;
			sio_stat->shifts_left = 0;
			sio_stat->shift_counter = 0;
			sio_stat->shift_clock = 0;

			std::cout<<"GB KISS LINK Transfer Finished\n";
			
			break;
	}
}

/****** Converts incoming IR pulses into bytes for the GB KISS LINK ******/
void DMG_MMU::gb_kiss_link_get_bytes()
{
	u32 size = kiss_link.input_signals.size();
	u32 op_id = 0;
	u32 index = 0;

	u8 mask = 0x80;
	u8 result = 0;

	while(index < size)
	{
		op_id = kiss_link.input_signals[index++] / 100;
		if(op_id == 7) { result |= mask; }
		mask >>= 1;
	}

	kiss_link.input_data.push_back(result);
}

/****** Converts byte data into IR pulses for the GB KISS LINK ******/
void DMG_MMU::gb_kiss_link_set_signal(u8 input)
{
	u8 mask = 0x01;

	//Data is converted and stored as pulses little-endian first
	//Data is transmitted big-endian first using back()
	while(mask)
	{
		//Long Pulse, Bit = 1 : ~720 CPU cycles
		//200 on, 520 off
		if(input & mask)
		{
			kiss_link.output_signals.push_back(GKL_OFF_LONG);
			kiss_link.output_signals.push_back(GKL_ON);
		}

		//Short Pulse, Bit = 0 : ~460 CPU cycles
		//200 on, 260 off
		else
		{
			kiss_link.output_signals.push_back(GKL_OFF_SHORT);
			kiss_link.output_signals.push_back(GKL_ON);
		}

		mask <<= 1;
	}
}

/****** Initiates a handshake from the GB KISS LINK to the Game Boy ******/
void DMG_MMU::gb_kiss_link_handshake(u8 input)
{
	kiss_link.output_signals.clear();
	kiss_link.cycles = 0;
	kiss_link.is_locked = true;

	switch(input)
	{
		case 0x3C: kiss_link.state = GKL_SEND_HANDSHAKE_3C; break;
		case 0x55: kiss_link.state = GKL_SEND_HANDSHAKE_55; break;
		case 0xAA: kiss_link.state = GKL_SEND_HANDSHAKE_AA; break;
		case 0xC3: kiss_link.state = GKL_SEND_HANDSHAKE_C3; break;
		default: return;
	}

	//Handshake STOP
	kiss_link.output_signals.push_back(GKL_OFF_END);
	kiss_link.output_signals.push_back(GKL_ON);
	kiss_link.output_signals.push_back(GKL_OFF_STOP);
	kiss_link.output_signals.push_back(GKL_ON);

	//Handshake data
	gb_kiss_link_set_signal(input);

	//Handshake zero bit
	kiss_link.output_signals.push_back(GKL_OFF_SHORT);
	kiss_link.output_signals.push_back(GKL_ON);

	sio_stat->shifts_left = 1;
	sio_stat->shift_counter = 0;
	sio_stat->shift_clock = kiss_link.output_signals.back();
}

/****** Sends commands from the GB KISS LINK to the Game Boy ******/
void DMG_MMU::gb_kiss_link_send_command()
{
	kiss_link.output_data.clear();
	kiss_link.checksum = 0;
	kiss_link.data_len = 0;

	//"Hu0" string
	kiss_link.output_data.push_back(0x48);
	kiss_link.output_data.push_back(0x75);
	kiss_link.output_data.push_back(0x30);

	//Command
	kiss_link.output_data.push_back(kiss_link.cmd);

	switch(kiss_link.cmd)
	{
		case GKL_CMD_MANAGE_SESSION:
			//Local Addr + Remote Addr
			kiss_link.output_data.push_back(kiss_link.local_addr & 0xFF);
			kiss_link.output_data.push_back(kiss_link.local_addr >> 8);
			kiss_link.output_data.push_back(kiss_link.remote_addr & 0xFF);
			kiss_link.output_data.push_back(kiss_link.remote_addr >> 8);

			//Length
			kiss_link.output_data.push_back(kiss_link.len);

			//Unknown parameter
			kiss_link.output_data.push_back(kiss_link.param);

			//Checksum
			for(u32 x = 2; x < kiss_link.output_data.size(); x++) { kiss_link.checksum += kiss_link.output_data[x]; }
			kiss_link.checksum = ~kiss_link.checksum;
			kiss_link.checksum++;
			kiss_link.output_data.push_back(kiss_link.checksum);

			std::cout<<"Start Session -> 0x" << kiss_link.remote_addr << "\n";

			break;

		case GKL_CMD_FILE_SEARCH:
			//Local Addr
			kiss_link.output_data.push_back(kiss_link.local_addr & 0xFF);
			kiss_link.output_data.push_back(kiss_link.local_addr >> 8);

			//Raw File Size (Total GBF file size - Icon + Title + History size)
			kiss_link.gbf_raw_size = kiss_link.gbf_title_icon_size + 0x05;
			if(kiss_link.gbf_flags & 0x1) { kiss_link.gbf_raw_size += 0x2E; }
			kiss_link.gbf_raw_size = kiss_link.gbf_file_size - kiss_link.gbf_raw_size;

			kiss_link.output_data.push_back(kiss_link.gbf_raw_size & 0xFF);
			kiss_link.output_data.push_back(kiss_link.gbf_raw_size >> 8);

			kiss_link.gbf_index = kiss_link.gbf_file_size - kiss_link.gbf_raw_size;

			//GBF Destination File Slot
			kiss_link.output_data.push_back(kiss_link.slot);

			//GBF File Flags
			kiss_link.output_data.push_back(kiss_link.param);

			//Checksum
			for(u32 x = 2; x < kiss_link.output_data.size(); x++) { kiss_link.checksum += kiss_link.output_data[x]; }
			kiss_link.checksum = ~kiss_link.checksum;
			kiss_link.checksum++;
			kiss_link.output_data.push_back(kiss_link.checksum);

			//Data bytes and data checksum - Uses file_search_data
			kiss_link.checksum = 0;

			for(u32 x = 0; x < kiss_link.file_search_data.size(); x++)
			{
				kiss_link.output_data.push_back(kiss_link.file_search_data[x]);
				kiss_link.checksum += kiss_link.file_search_data[x];
				kiss_link.data_len++;
			}

			kiss_link.checksum = ~kiss_link.checksum;
			kiss_link.checksum++;
			kiss_link.output_data.push_back(kiss_link.checksum);
			kiss_link.data_len++;

			std::cout<<"File Search -> 0x" << kiss_link.remote_addr << "\n";

			break;

		case GKL_CMD_MANAGE_UPLOAD:
			//Local Addr + Remote Addr
			kiss_link.output_data.push_back(kiss_link.local_addr & 0xFF);
			kiss_link.output_data.push_back(kiss_link.local_addr >> 8);
			kiss_link.output_data.push_back(kiss_link.remote_addr & 0xFF);
			kiss_link.output_data.push_back(kiss_link.remote_addr >> 8);

			//??? - Looks like the length of history data.
			//Set to zero when ending an upload, however
			if(kiss_link.stage == GKL_PREP_UPLOAD) { kiss_link.output_data.push_back(0x2E); }
			else { kiss_link.output_data.push_back(0x00); }

			//Unknown parameter
			kiss_link.output_data.push_back(kiss_link.param);

			//Checksum
			for(u32 x = 2; x < kiss_link.output_data.size(); x++) { kiss_link.checksum += kiss_link.output_data[x]; }
			kiss_link.checksum = ~kiss_link.checksum;
			kiss_link.checksum++;
			kiss_link.output_data.push_back(kiss_link.checksum);

			//Data bytes and data checksum - Uses file_search_data
			kiss_link.checksum = 0;

			std::cout<<"Prep Upload -> 0x" << kiss_link.remote_addr << "\n";

			break;

		case GKL_CMD_READ_RAM:
		case GKL_CMD_READ_SRAM:
			//Local Addr + Remote Addr
			kiss_link.output_data.push_back(kiss_link.local_addr & 0xFF);
			kiss_link.output_data.push_back(kiss_link.local_addr >> 8);
			kiss_link.output_data.push_back(kiss_link.remote_addr & 0xFF);
			kiss_link.output_data.push_back(kiss_link.remote_addr >> 8);

			//Length
			kiss_link.output_data.push_back(kiss_link.len);

			//Unknown parameter
			kiss_link.output_data.push_back(kiss_link.param);

			//Checksum
			for(u32 x = 2; x < kiss_link.output_data.size(); x++) { kiss_link.checksum += kiss_link.output_data[x]; }
			kiss_link.checksum = ~kiss_link.checksum;
			kiss_link.checksum++;
			kiss_link.output_data.push_back(kiss_link.checksum);

			//When reading directly from RAM for Nectaris maps, the size is sometimes 0x00
			//Here, make sure that the internal length is set so GBE+ can count the correct amount of bytes sent
			if((kiss_link.mode >= 0x02) && (kiss_link.len == 0)) { kiss_link.len = 256; }

			std::cout<<"Read RAM -> 0x" << kiss_link.remote_addr << "\n";

			break;

		case GKL_CMD_SEND_ICON:
		case GKL_CMD_MANAGE_DATA:
		case GKL_CMD_WRITE_RAM:
			//Local Addr + Remote Addr
			kiss_link.output_data.push_back(kiss_link.local_addr & 0xFF);
			kiss_link.output_data.push_back(kiss_link.local_addr >> 8);
			kiss_link.output_data.push_back(kiss_link.remote_addr & 0xFF);
			kiss_link.output_data.push_back(kiss_link.remote_addr >> 8);

			//Length
			kiss_link.output_data.push_back(kiss_link.len);

			//Unknown parameter
			kiss_link.output_data.push_back(kiss_link.param);

			//Checksum
			for(u32 x = 2; x < kiss_link.output_data.size(); x++) { kiss_link.checksum += kiss_link.output_data[x]; }
			kiss_link.checksum = ~kiss_link.checksum;
			kiss_link.checksum++;
			kiss_link.output_data.push_back(kiss_link.checksum);

			//Data bytes and data checksum - Uses input_data
			kiss_link.checksum = 0;

			for(u32 x = 0; x < kiss_link.input_data.size(); x++)
			{
				kiss_link.output_data.push_back(kiss_link.input_data[x]);
				kiss_link.checksum += kiss_link.input_data[x];
				kiss_link.data_len++;
			}

			kiss_link.checksum = ~kiss_link.checksum;
			kiss_link.checksum++;
			kiss_link.output_data.push_back(kiss_link.checksum);
			kiss_link.data_len++;

			//When writing directly to RAM for Nectaris maps, the size is always 0x00
			//Here, make sure that the internal length is set so GBE+ can count the correct amount of bytes sent
			if((kiss_link.mode == 0x02) && (kiss_link.len == 0)) { kiss_link.len = 256; }

			if(kiss_link.cmd == 0x02) { std::cout<<"Send Icon -> 0x" << kiss_link.remote_addr << "\n"; }
			else if(kiss_link.stage == GKL_SEND_HISTORY) { std::cout<<"Send History -> 0x" << kiss_link.remote_addr << "\n"; }
			else if(kiss_link.stage == GKL_SEND_FILE) { std::cout<<"Send File -> 0x" << kiss_link.remote_addr << "\n"; }
			else { std::cout<<"Write RAM -> 0x" << kiss_link.remote_addr << "\n"; }

			break;
	}

	gb_kiss_link_send_ping(GKL_ON_PING_SENDER, GKL_OFF_PING_SENDER);
}

/****** Sends commands response from the GB KISS LINK to the Game Boy ******/
void DMG_MMU::gb_kiss_link_recv_command()
{
	kiss_link.output_data.clear();
	kiss_link.checksum = 0;
	kiss_link.data_len = 0;

	switch(kiss_link.cmd)
	{
		case GKL_CMD_READ_RAM:
			//Data bytes and data checksum - Uses input_data
			for(u32 x = 0; x < kiss_link.input_data.size(); x++)
			{
				kiss_link.output_data.push_back(kiss_link.input_data[x]);
				kiss_link.checksum += kiss_link.input_data[x];
				kiss_link.data_len++;
			}

			kiss_link.checksum = ~kiss_link.checksum;
			kiss_link.checksum++;
			kiss_link.output_data.push_back(kiss_link.checksum);

			break;

		default:
			for(u32 x = 0; x < kiss_link.input_data.size(); x++)
			{
				kiss_link.output_data.push_back(kiss_link.input_data[x]);
			}
	}

	gb_kiss_link_send_ping(GKL_ON_PING_SENDER, GKL_OFF_PING_SENDER);
}

/****** Sends ping signal for command bytes ******/
void DMG_MMU::gb_kiss_link_send_ping(u32 on_pulse, u32 off_pulse)
{
	kiss_link.output_signals.clear();
	kiss_link.cycles = 0;
	kiss_link.is_locked = true;

	kiss_link.state = GKL_SEND_PING;

	//Ping pulse
	kiss_link.output_signals.push_back(off_pulse);
	kiss_link.output_signals.push_back(on_pulse);

	//Ping delay - Used for data bytes that follow a command
	if(kiss_link.is_ping_delayed)
	{
		kiss_link.output_signals[1] = 741;
		kiss_link.is_ping_delayed = false;
	}

	sio_stat->shifts_left = 1;
	sio_stat->shift_counter = 0;
	sio_stat->shift_clock = kiss_link.output_signals.back();
}

/****** Loads a GBF file for the GB KISS LINK *****/
bool DMG_MMU::gb_kiss_link_load_file(std::string filename)
{
	if(filename.empty()) { return false; }

	std::ifstream file(filename.c_str(), std::ios::binary);

	if(!file.is_open()) 
	{ 
		std::cout<<"MMU::GBF file could not be read. Check file path or permissions. \n";
		return false;
	}

	//Get file size
	file.seekg(0, file.end);
	u32 file_size = file.tellg();
	file.seekg(0, file.beg);

	kiss_link.gbf_data.resize(file_size, 0x00);
	u8* ex_mem = &kiss_link.gbf_data[0];
	file.read((char*)ex_mem, file_size);

	file.close();

	if(file_size >= 5)
	{
		kiss_link.gbf_file_size = file_size;
		kiss_link.gbf_title_icon_size = kiss_link.gbf_data[0x04];
		kiss_link.gbf_flags = kiss_link.gbf_data[0x02];
	}

	std::cout<<"MMU::Loaded GBF file " << filename << "\n";
	return true;
}

/****** Saves a GBF file from GB KISS LINK *****/
bool DMG_MMU::gb_kiss_link_save_file()
{
	//Grab filename from GBF or map data, or use default
	std::string filename = "";
	std::string ext = (kiss_link.mode == 3) ? ".bin" : ".gbf";
	u8 start = (kiss_link.mode == 3) ? 8 : 2;

	for(u32 x = start; x < 14; x++)
	{
		u8 chr = kiss_link.gbf_data[x];

		if(chr == 0x00) { break; }
		else if((chr > 0x20) && (chr <= 0x7E)) { filename += chr; }
	}

	if(filename.empty()) { filename = config::data_path + "bin/infrared/default" + ext; }
	else { filename = config::data_path + "bin/infrared/" + filename + ext; }

	std::ofstream file(filename.c_str(), std::ios::binary);

	if(!file.is_open())
	{
		std::cout<<"MMU::GB KISS LINK file could not be saved. Check file path or permissions. \n";
		return false;
	}

	//Save GBF specific data
	if(kiss_link.mode == 1)
	{
		//Total File Size
		u8 hi = (kiss_link.gbf_file_size >> 8);
		u8 lo = (kiss_link.gbf_file_size & 0xFF);
		file.write((char*)&lo, 1);
		file.write((char*)&hi, 1);

		//Flags + Cart Code
		file.write((char*)&kiss_link.gbf_flags, 1);
		file.write((char*)&kiss_link.cart_code, 1);
	}

	//Rest of data
	u8* ex_mem = &kiss_link.gbf_data[0];
	u32 file_size = kiss_link.gbf_data.size();
	file.write((char*)ex_mem, file_size);

	file.close();

	std::cout<<"MMU::GB KISS LINK file saved. \n";

	return true;
}

/****** Resets all data for emulated GB KISS LINK - Optionally resets GBF data ******/
void DMG_MMU::gb_kiss_link_reset(bool reset_gbf)
{
	kiss_link.cycles = 0;
	kiss_link.input_signals.clear();
	kiss_link.output_signals.clear();
	kiss_link.input_data.clear();
	kiss_link.output_data.clear();
	kiss_link.file_search_data.clear();
	kiss_link.state = GKL_INACTIVE;
	kiss_link.is_locked = false;
	kiss_link.is_ping_delayed = false;
	kiss_link.is_upload_done = true;
	kiss_link.is_running = true;

	switch(kiss_link.mode)
	{
		case 0: kiss_link.stage = GKL_INIT_SENDER; break;
		case 1: kiss_link.stage = GKL_INIT_RECEIVER; break;
		case 2: kiss_link.stage = GKL_INIT_MAP_SENDER; break;
		case 3: kiss_link.stage = GKL_INIT_MAP_RECEIVER; break;
	}

	kiss_link.cmd = 0;
	kiss_link.checksum = 0;
	kiss_link.param = 0;
	kiss_link.slot = 0;
	kiss_link.cart_code = 0;
	kiss_link.len = 0;
	kiss_link.data_len = 0;
	kiss_link.local_addr = 0;
	kiss_link.remote_addr = 0;

	if(reset_gbf)
	{
		kiss_link.gbf_index = 0;
		kiss_link.gbf_file_size = 0;
		kiss_link.gbf_raw_size = 0;
		kiss_link.gbf_title_icon_size = 0;
		kiss_link.gbf_flags = 0;
	}
}
