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
	u32 op_id = kiss_link.cycles / 100;

	switch(kiss_link.state)
	{
		case GKL_SEND:
		case GKL_SEND_PING:
		case GKL_SEND_HANDSHAKE_AA:
		case GKL_SEND_HANDSHAKE_C3:
			//Set HuC IR ON or OFF
			if(kiss_link.output_signals.back() & 0x01) { cart.huc_ir_input = 0x01; }
			else { cart.huc_ir_input = 0x00; }

			std::cout<<"SENT SIGNAL -> " << std::dec << (kiss_link.output_signals.back() & 0xFFFE) << " :: " << u32(cart.huc_ir_input) << "\n";

			kiss_link.output_signals.pop_back();

			if(!kiss_link.output_signals.empty())
			{
				sio_stat->shifts_left = 1;
				sio_stat->shift_counter = 0;
				sio_stat->shift_clock = (kiss_link.output_signals.back() & 0xFFFFFFFE);
			}

			else
			{
				kiss_link.cycles = 0;
				kiss_link.input_signals.clear();

				sio_stat->shifts_left = 0;
				sio_stat->shift_counter = 0;
				sio_stat->shift_clock = 0;

				if(kiss_link.state == GKL_SEND_HANDSHAKE_AA)
				{
					kiss_link.state = GKL_RECV_HANDSHAKE_55;
					kiss_link.is_locked = false;
				}

				else if(kiss_link.state == GKL_SEND_HANDSHAKE_C3)
				{
					kiss_link.state = GKL_RECV_HANDSHAKE_3C;
					kiss_link.is_locked = false;
				}

				else if(kiss_link.state == GKL_SEND_PING)
				{
					kiss_link.state = GKL_RECV_PING;
					kiss_link.is_locked = false;
				}
			}

			break;

		case GKL_RECV:
		case GKL_RECV_PING:
		case GKL_RECV_HANDSHAKE_55:
		case GKL_RECV_HANDSHAKE_3C:
			//End handshake - ~1200 cycles pulse
			if(op_id == 12)
			{
				std::cout<<"RECV SIGNAL -> " << std::dec << kiss_link.cycles << "\n";

				gb_kiss_link_get_bytes();
				kiss_link.input_signals.clear();

				u32 data_size = kiss_link.data.size();

				//Do next handshake 0xAA -> 0xC3
				if((kiss_link.data.back() == 0x55) && (data_size == 1))
				{
					gb_kiss_link_handshake(0xC3);
				}

				//End handshake, move onto next command
				else if((kiss_link.data.back() == 0x3C) && (data_size == 2))
				{
					//Read RAM -> Receiver ID
					if(kiss_link.stage == GKL_INIT)
					{
						kiss_link.stage = GKL_REQUEST_ID;
						gb_kiss_link_send_command(0x08);
						std::cout<<"CMD -> 0x8\n";
					}
				}

				//End IR transmission
				else
				{
					kiss_link.input_signals.push_back(kiss_link.cycles);
					kiss_link.cycles = 0;
					kiss_link.state = GKL_INACTIVE;

					sio_stat->shifts_left = 0;
					sio_stat->shift_counter = 0;
					sio_stat->shift_clock = 0;
				}
			}

			//End ping
			else if((op_id == 1) && (kiss_link.state == GKL_RECV_PING))
			{
				std::cout<<"PING DONE\n";
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
	u32 index = 1;

	u8 mask = 0x80;
	u8 result = 0;

	while(index < size)
	{
		op_id = kiss_link.input_signals[index++] / 100;
		if(op_id == 7) { result |= mask; }
		mask >>= 1;
	}

	kiss_link.data.push_back(result);

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
		case 0xAA: kiss_link.state = GKL_SEND_HANDSHAKE_AA; break;
		case 0xC3: kiss_link.state = GKL_SEND_HANDSHAKE_C3; break;
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
void DMG_MMU::gb_kiss_link_send_command(u8 input)
{
	kiss_link.data.clear();
	kiss_link.output_signals.clear();
	kiss_link.cycles = 0;
	kiss_link.is_locked = true;

	switch(input)
	{
		//Request ID - Stored in 0xCE00 to 0xCE10 in Game Boy's RAM
		case 0x08:
			//"Hu0" string
			kiss_link.data.push_back(0x48);
			kiss_link.data.push_back(0x75);
			kiss_link.data.push_back(0x30);

			//Command 0x8000
			kiss_link.data.push_back(0x08);
			kiss_link.data.push_back(0x00);

			//Address start + end
			kiss_link.data.push_back(0xCE);
			kiss_link.data.push_back(0x00);
			kiss_link.data.push_back(0xCE);
			kiss_link.data.push_back(0x10);

			//Unknown data
			kiss_link.data.push_back(0x00);
			kiss_link.data.push_back(0x0E);

			break;
	}

	kiss_link.state = GKL_SEND_PING;

	//Ping pulse
	kiss_link.output_signals.push_back(GKL_OFF_END);
	kiss_link.output_signals.push_back(GKL_ON);
	kiss_link.output_signals.push_back(GKL_OFF_SHORT);
	kiss_link.output_signals.push_back(GKL_ON);

	sio_stat->shifts_left = 1;
	sio_stat->shift_counter = 0;
	sio_stat->shift_clock = kiss_link.output_signals.back();
}
