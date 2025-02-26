// GB Enhanced Copyright Daniel Baxter 2017
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : infrared.cpp
// Date : August 07, 2017
// Description : IR accessory emulation
//
// Emulates various IR accessories (Pokemon Pikachu 2, Full Changer, Pocket Sakura, TV Remote)

#include "sio.h"
#include "common/util.h" 

/****** Processes Full Changer data sent to the Game Boy ******/
void DMG_SIO::full_changer_process()
{
	//Initiate Full Changer transmission
	if(mem->ir_stat.trigger == 2)
	{
		//Validate IR database index
		if(config::ir_db_index > 0x45) { config::ir_db_index = 0; }

		mem->ir_stat.trigger = 0;
		full_changer.delay_counter = (config::ir_db_index * 0x24);
		full_changer.current_state = FULL_CHANGER_SEND_SIGNAL;
		full_changer.light_on = true;
	}

	if(full_changer.current_state != FULL_CHANGER_SEND_SIGNAL) { return; }

	//Start or stop sending light pulse to Game Boy
	if(full_changer.light_on)
	{
		mem->memory_map[REG_RP] &= ~0x2;
		full_changer.light_on = false;
	}

	else
	{
		mem->memory_map[REG_RP] |= 0x2;
		full_changer.light_on = true;
	}

	//Schedule the next on-off pulse
	if(full_changer.delay_counter != ((config::ir_db_index + 1) * 0x24) - 1)
	{
		sio_stat.shift_counter = 0;
		sio_stat.shift_clock = full_changer.data[full_changer.delay_counter];
		sio_stat.shifts_left = 1;

		//Set up next delay
		full_changer.delay_counter++;
	}

	else { full_changer.current_state = FULL_CHANGER_INACTIVE; }
}

/****** Loads a database file of Cosmic Characters for the Full Changer ******/
bool DMG_SIO::full_changer_load_db(std::string filename)
{
	std::ifstream database(filename.c_str(), std::ios::binary);

	if(!database.is_open()) 
	{ 
		std::cout<<"SIO::Full Changer database data could not be read. Check file path or permissions. \n";
		return false;
	}

	//Get file size
	database.seekg(0, database.end);
	u32 database_size = database.tellg();
	database.seekg(0, database.beg);
	database_size >>= 1;

	full_changer.data.clear();
	u16 temp_word = 0;
	
	for(u32 x = 0; x < database_size; x++)
	{
		database.read((char*)&temp_word, 2);
		u16 flip = (temp_word >> 8);
		flip |= ((temp_word & 0xFF) << 8);
		full_changer.data.push_back(flip);
	}

	database.close();

	std::cout<<"SIO::Loaded Full Changer database.\n";
	return true;
}

/****** Processes TV Remote data sent to the Game Boy ******/
void DMG_SIO::tv_remote_process()
{
	//Initiate IR transmission
	if(mem->ir_stat.trigger == 2)
	{
		mem->ir_stat.trigger = 0;
		tv_remote.current_data = 0;
		tv_remote.current_state = TV_REMOTE_SEND_SIGNAL;
		tv_remote.light_on = false;
	}

	if(tv_remote.current_state != TV_REMOTE_SEND_SIGNAL) { return; }

	//Start or stop sending light pulse to Game Boy
	if(tv_remote.light_on)
	{
		mem->memory_map[REG_RP] &= ~0x2;
		tv_remote.light_on = false;
	}

	else
	{
		mem->memory_map[REG_RP] |= 0x2;
		tv_remote.light_on = true;
	}

	//Schedule the next on-off pulse
	if(tv_remote.current_data != tv_remote.data.size())
	{
		sio_stat.shift_counter = 0;
		sio_stat.shift_clock = tv_remote.data[tv_remote.current_data];
		sio_stat.shifts_left = 1;

		//Set up next delay
		tv_remote.current_data++;
	}

	else { tv_remote.current_state = TV_REMOTE_INACTIVE; }
}

/****** Loads a database file for Pokemon Pikachu or Pocket Sakura ******/
bool DMG_SIO::pocket_ir_load_db(std::string filename)
{
	std::ifstream database(filename.c_str(), std::ios::binary);

	if(!database.is_open()) 
	{ 
		if(sio_stat.ir_type == GBC_POKEMON_PIKACHU_2) { std::cout<<"SIO::Pokemon Pikachu 2 database data could not be read. Check file path or permissions. \n"; }
		else { std::cout<<"SIO::Pocket Sakura database data could not be read. Check file path or permissions. \n"; }
		return false;
	}

	//Get file size
	database.seekg(0, database.end);
	u32 database_size = database.tellg();
	database.seekg(0, database.beg);
	database_size >>= 1;

	pocket_ir.data.clear();
	u16 temp_word = 0;
	
	for(u32 x = 0; x < database_size; x++)
	{
		database.read((char*)&temp_word, 2);
		pocket_ir.data.push_back(temp_word);
	}

	database.close();

	if(sio_stat.ir_type == GBC_POKEMON_PIKACHU_2) { std::cout<<"SIO::Loaded Pokemon Pikachu 2 database.\n"; }
	else { std::cout<<"SIO::Loaded Pocket Sakura database.\n"; }

	return true;
}

/****** Processes Pokemon Pikachu 2 or Pocket Sakura data sent to the Game Boy ******/
void DMG_SIO::pocket_ir_process()
{
	//Initiate IR device transmission
	if(mem->ir_stat.trigger == 2)
	{
		mem->ir_stat.trigger = 0;
		pocket_ir.current_data = pocket_ir.db_step * config::ir_db_index;
		pocket_ir.current_state = POCKET_IR_SEND_SIGNAL;
		pocket_ir.light_on = true;
	}

	if(pocket_ir.current_state != POCKET_IR_SEND_SIGNAL) { return; }

	//Start or stop sending light pulse to Game Boy
	if(pocket_ir.light_on)
	{
		mem->memory_map[REG_RP] &= ~0x2;
		pocket_ir.light_on = false;
	}

	else
	{
		mem->memory_map[REG_RP] |= 0x2;
		pocket_ir.light_on = true;
	}

	//Schedule the next on-off pulse
	if(pocket_ir.current_data < (pocket_ir.db_step * (config::ir_db_index + 1)))
	{
		sio_stat.shift_counter = 0;
		sio_stat.shift_clock = pocket_ir.data[pocket_ir.current_data];
		sio_stat.shifts_left = 1;

		//Set up next delay
		pocket_ir.current_data++;
	}

	else { pocket_ir.current_state = POCKET_IR_INACTIVE; }
}

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

			//std::cout<<"SENT SIGNAL -> " << std::dec << (kiss_link.output_signals.back() & 0xFFFFFFE) << " :: " << u32(cart.huc_ir_input) << "\n";

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
					if((kiss_link.cmd == 0x08) || (kiss_link.cmd == 0x02))
					{
						kiss_link.output_data.clear();
						gb_kiss_link_send_ping();

						//Some commands require receiving different lengths than they send
						//Adjust length here as needed in these cases
						if(kiss_link.cmd == 0x02) { kiss_link.len = 265; }
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
						//Receive Icon Data echo
						if((kiss_link.stage == GKL_REQUEST_ID) || (kiss_link.stage == GKL_SEND_ICON))
						{
							kiss_link.state = GKL_RECV_HANDSHAKE_AA;
							kiss_link.is_locked = false;
						}

						//Receive echo of checksum after RAM write
						else if(kiss_link.stage == GKL_WRITE_ID)
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

						std::cout<<"SENDING BYTE -> 0x" << std::hex << (u32)kiss_link.output_data.front() << "\n";

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
					//Read RAM -> Receiver ID String
					if(kiss_link.stage == GKL_INIT)
					{
						kiss_link.stage = GKL_REQUEST_ID;

						kiss_link.cmd = 0x08;
						kiss_link.local_addr = 0xCE00;
						kiss_link.remote_addr = 0xCE00;
						kiss_link.len = 0x10;
						kiss_link.param = 0;

						kiss_link.input_data.clear();
						gb_kiss_link_send_command();
					}

					//Write RAM -> Receiver ID
					else if(kiss_link.stage == GKL_WRITE_ID)
					{
						kiss_link.cmd = 0x0B;
						kiss_link.local_addr = 0xCE00;
						kiss_link.remote_addr = 0xCE00;
						kiss_link.len = 0x01;
						kiss_link.param = 0;

						//Command data
						kiss_link.input_data.clear();
						kiss_link.input_data.push_back(0x01);

						gb_kiss_link_send_command();
					}

					//Start Session
					else if(kiss_link.stage == GKL_START_SESSION)
					{
						kiss_link.cmd = 0x00;
						kiss_link.local_addr = 0xCE00;
						kiss_link.remote_addr = 0xCE00;
						kiss_link.len = 0x01;
						kiss_link.param = 0;

						kiss_link.input_data.clear();
						gb_kiss_link_send_command();
					}

					//Send Icon
					else if(kiss_link.stage == GKL_SEND_ICON)
					{
						kiss_link.cmd = 0x02;
						kiss_link.local_addr = 0xC700;
						kiss_link.remote_addr = 0xC50C;
						kiss_link.len = kiss_link.gbf_title_icon_size;
						kiss_link.param = 0;

						//Command data
						kiss_link.input_data.clear();
						u32 size = (kiss_link.gbf_title_icon_size + 5);
						
						for(u32 x = 5; x < size; x++)
						{
							kiss_link.input_data.push_back(kiss_link.gbf_data[x]);
						}

						gb_kiss_link_send_command();
					}
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
				}

				else if(kiss_link.stage == GKL_WRITE_ID)
				{
					kiss_link.stage = GKL_START_SESSION;
					gb_kiss_link_handshake(0xAA);
				}

				if(kiss_link.stage == GKL_SEND_ICON)
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

	std::cout<<"BYTE -> 0x" << std::hex << (u32)result << "\n";
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
		//Write RAM
		case 0x02:
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

	//std::cout<<"PING START\n";
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
