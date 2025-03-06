// GB Enhanced Copyright Daniel Baxter 2025
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : gb_kiss_link.cpp
// Date : March 5, 2025
// Description : GB KISS LINK emulation
//
// Emulates the GB KISS LINK (HC-749)
// Sends and receives .GBF files
// Works in conjunction with HuC-1 and HuC-3 carts

#include "sio.h"
#include "common/util.h" 

/****** Processes IR communication protocol for GB KISS LINK *****/
void DMG_MMU::gb_kiss_link_process()
{
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

					//Receive data
					if((kiss_link.cmd == 0x02) || (kiss_link.cmd == 0x03)
					|| (kiss_link.cmd == 0x04) || (kiss_link.cmd == 0x08)
					|| (kiss_link.cmd == 0x0A))
					{
						kiss_link.output_data.clear();
						gb_kiss_link_send_ping();

						//Some commands require receiving different lengths than they send
						//Adjust length here as needed in these cases
						if((kiss_link.cmd == 0x02) || (kiss_link.cmd == 0x03))
						{
							kiss_link.len = 265;
						}

						else if((kiss_link.cmd == 0x04) || (kiss_link.cmd == 0x0A))
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

						gb_kiss_link_send_ping();
					}

					//At end of ping, move onto next state, stage, or command
					else
					{
						//Receive ID data from RAM
						//Receive Icon Data echo (File Search Data)
						//Receive File Search results
						//Receive Prep Upload results
						//Receive Send History and Send File echo
						if((kiss_link.stage == GKL_REQUEST_ID) || (kiss_link.stage == GKL_SEND_ICON)
						|| (kiss_link.stage == GKL_FILE_SEARCH) || (kiss_link.stage == GKL_UNK_READ_1)
						|| (kiss_link.stage == GKL_PREP_UPLOAD) || (kiss_link.stage == GKL_SEND_HISTORY)
						|| (kiss_link.stage == GKL_SEND_FILE) || (kiss_link.stage == GKL_END_UPLOAD)
						|| (kiss_link.stage == GKL_CLOSE_FILE))
						{
							kiss_link.state = GKL_RECV_HANDSHAKE_AA;
							kiss_link.is_locked = false;
						}

						//Receive echo of checksum after RAM write
						else if((kiss_link.stage == GKL_WRITE_ID) || (kiss_link.stage == GKL_UNK_WRITE_1)
						|| (kiss_link.stage == GKL_UNK_WRITE_2))
						{
							gb_kiss_link_send_ping();
						}

						//Send icon data - Ping is super long due to client-side processing
						else if(kiss_link.stage == GKL_START_SESSION)
						{
							kiss_link.output_data.clear();
							gb_kiss_link_send_ping();
							kiss_link.output_signals[0] = 1078400;
						}

						else if(kiss_link.stage == GKL_END_SESSION)
						{
							std::cout<<"GB KISS LINK Transfer Finished\n";
						}
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

					//Receive next byte for command response
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
							kiss_link.state = GKL_RECV_DATA;
						}
					}
				}
			}

			break;

		case GKL_RECV:
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

			//Receive data from commands
			else if((kiss_link.state == GKL_RECV_DATA) && (kiss_link.input_signals.size() == 9))
			{
				gb_kiss_link_get_bytes();
				kiss_link.input_signals.clear();
				gb_kiss_link_send_ping();
			}

			//Finish receiving data from commands
			else if((kiss_link.state == GKL_RECV_DATA) && (kiss_link.input_signals.size() == 8)
			&& (kiss_link.input_data.size() == kiss_link.len))
			{
				gb_kiss_link_get_bytes();
				kiss_link.input_signals.clear();

				//Move onto next state, stage, and command
				if(kiss_link.stage == GKL_REQUEST_ID)
				{
					kiss_link.stage = GKL_WRITE_ID;
					gb_kiss_link_handshake(0xAA);

					kiss_link.slot = kiss_link.input_data[kiss_link.input_data.size() - 2];
				}

				else if(kiss_link.stage == GKL_WRITE_ID)
				{
					kiss_link.stage = GKL_START_SESSION;
					gb_kiss_link_handshake(0xAA);
				}

				else if(kiss_link.stage == GKL_SEND_ICON)
				{
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
				}

				else if(kiss_link.stage == GKL_UNK_WRITE_1)
				{
					kiss_link.stage = GKL_FILE_SEARCH;
					gb_kiss_link_handshake(0xAA);
				}

				else if(kiss_link.stage == GKL_FILE_SEARCH)
				{
					kiss_link.stage = GKL_UNK_READ_1;
					gb_kiss_link_handshake(0xAA);
				}

				else if(kiss_link.stage == GKL_UNK_READ_1)
				{
					kiss_link.stage = GKL_PREP_UPLOAD;
					gb_kiss_link_handshake(0xAA);
				}

				else if(kiss_link.stage == GKL_PREP_UPLOAD)
				{
					//TODO - This should probably be optional if GBF file does not have history
					//In that case, it would skip to the next stage? Needs more research
					kiss_link.stage = GKL_SEND_HISTORY;
					gb_kiss_link_handshake(0xAA);
				}

				else if(kiss_link.stage == GKL_SEND_HISTORY)
				{
					kiss_link.stage = GKL_SEND_FILE;
					kiss_link.is_upload_done = false;
					gb_kiss_link_handshake(0xAA);
				}

				else if(kiss_link.stage == GKL_SEND_FILE)
				{
					//Resend file in chunks until EOF is reached
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
				}

				else if(kiss_link.stage == GKL_END_UPLOAD)
				{
					kiss_link.stage = GKL_CLOSE_FILE;
					gb_kiss_link_handshake(0xAA);
				}

				else if(kiss_link.stage == GKL_CLOSE_FILE)
				{
					kiss_link.stage = GKL_UNK_WRITE_2;
					gb_kiss_link_handshake(0xAA);
				}

				else if(kiss_link.stage == GKL_UNK_WRITE_2)
				{
					kiss_link.stage = GKL_END_SESSION;
					gb_kiss_link_handshake(0xAA);
				}
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

	switch(kiss_link.stage)
	{
		//Read RAM -> Receiver ID String
		case GKL_INIT:
			kiss_link.stage = GKL_REQUEST_ID;

			kiss_link.cmd = 0x08;
			kiss_link.local_addr = 0xCE00;
			kiss_link.remote_addr = 0xCE00;
			kiss_link.len = 0x10;
			kiss_link.param = 0;

			kiss_link.input_data.clear();
			gb_kiss_link_send_command();
			
			break;

		//Write RAM -> Receiver ID
		case GKL_WRITE_ID:
			kiss_link.cmd = 0x0B;
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
			kiss_link.cmd = 0x00;
			kiss_link.local_addr = 0xCE00;
			kiss_link.remote_addr = 0xCE00;
			kiss_link.len = 0x01;
			kiss_link.param = 0;

			kiss_link.input_data.clear();
			gb_kiss_link_send_command();

			break;

		//Send Icon
		case GKL_SEND_ICON:
			kiss_link.cmd = 0x02;
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
			kiss_link.cmd = 0x0B;
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
			kiss_link.cmd = 0x03;
			kiss_link.local_addr = 0xC700;
			kiss_link.param = kiss_link.gbf_flags;

			//Command data
			kiss_link.input_data.clear();

			gb_kiss_link_send_command();
			
			break;

		//Unknown RAM Read #1
		case GKL_UNK_READ_1:
			kiss_link.cmd = 0x08;
			kiss_link.local_addr = 0xCE00;
			kiss_link.remote_addr = 0xDFFC;
			kiss_link.len = 0x02;
			kiss_link.param = 0x76;

			kiss_link.input_data.clear();
			gb_kiss_link_send_command();

			break;

		//Prep file upload from GB KISS LINK
		case GKL_PREP_UPLOAD:
			kiss_link.cmd = 0x4;
			kiss_link.local_addr = 0xC500;
			kiss_link.remote_addr = 0xFFD2;
			kiss_link.param = 0x0;

			kiss_link.input_data.clear();
			gb_kiss_link_send_command();

			break;

		//Send History
		case GKL_SEND_HISTORY:
			kiss_link.cmd = 0x0A;
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
			kiss_link.cmd = 0x0A;
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
			kiss_link.cmd = 0x04;
			kiss_link.local_addr = 0xC50B;
			kiss_link.remote_addr = 0x3 - (kiss_link.gbf_file_size - kiss_link.gbf_raw_size);
			kiss_link.len = 0;
			kiss_link.param = 0;

			kiss_link.input_data.clear();
			gb_kiss_link_send_command();

			break;

		//Close File
		case GKL_CLOSE_FILE:
			kiss_link.cmd = 0x0A;
			kiss_link.local_addr = 0xC500;
			kiss_link.remote_addr = 0xC50B;
			kiss_link.len = 0x1;
			kiss_link.param = 0;

			//Command data
			kiss_link.input_data.clear();
			kiss_link.input_data.push_back(0x00);

			gb_kiss_link_send_command();
			
			break;

		//Unknown RAM Write #2
		case GKL_UNK_WRITE_2:
			kiss_link.cmd = 0x0B;
			kiss_link.local_addr = 0xCE00;
			kiss_link.remote_addr = 0xCE00;
			kiss_link.len = 0x01;
			kiss_link.param = 0x00;

			//Command data
			kiss_link.input_data.clear();
			kiss_link.input_data.push_back(0x02);

			gb_kiss_link_send_command();
			
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
		//Start Session
		case 0x00:
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

		//File Search
		case 0x03:
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

		//Prep Upload / End Upload
		case 0x04:
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

		//Read RAM
		case 0x08:
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

			std::cout<<"Read RAM -> 0x" << kiss_link.remote_addr << "\n";

			break;

		//Send Icon
		//Send History / File Data
		//Write RAM
		case 0x02:
		case 0x0A:
		case 0x0B:
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

			if(kiss_link.cmd == 0x02) { std::cout<<"Send Icon -> 0x" << kiss_link.remote_addr << "\n"; }
			else if(kiss_link.stage == GKL_SEND_HISTORY) { std::cout<<"Send History -> 0x" << kiss_link.remote_addr << "\n"; }
			else if(kiss_link.stage == GKL_SEND_FILE) { std::cout<<"Send File -> 0x" << kiss_link.remote_addr << "\n"; }
			else { std::cout<<"Write RAM -> 0x" << kiss_link.remote_addr << "\n"; }

			break;
	}

	gb_kiss_link_send_ping();
}

/****** Sends ping signal for command bytes ******/
void DMG_MMU::gb_kiss_link_send_ping()
{
	kiss_link.output_signals.clear();
	kiss_link.cycles = 0;
	kiss_link.is_locked = true;

	kiss_link.state = GKL_SEND_PING;

	//Ping pulse
	kiss_link.output_signals.push_back(GKL_OFF_PING);
	kiss_link.output_signals.push_back(GKL_ON_PING);

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
	kiss_link.stage = GKL_INIT;
	kiss_link.is_locked = false;
	kiss_link.is_ping_delayed = false;
	kiss_link.is_upload_done = true;

	kiss_link.cmd = 0;
	kiss_link.checksum = 0;
	kiss_link.param = 0;
	kiss_link.slot = 0;
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
