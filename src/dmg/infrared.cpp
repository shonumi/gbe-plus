// GB Enhanced Copyright Daniel Baxter 2017
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : infrared.cpp
// Date : August 07, 2017
// Description : IR accessory emulation
//
// Emulates various IR accessories (Pocket Pikachu 2, Full Changer)

#include "sio.h"
#include "common/util.h" 

/****** Processes Full Changer data sent to the Game Boy ******/
void DMG_SIO::full_changer_process()
{
	if(full_changer.current_state != FULL_CHANGER_SEND_SIGNAL) { return; }

	//Start or stop sending light pulse to Game Boy
	if(full_changer.light_on)
	{
		mem->memory_map[REG_RP] &= ~0x2;
		full_changer.light_on = false;
		std::cout<<"ON\n";
	}

	else
	{
		mem->memory_map[REG_RP] |= 0x2;
		full_changer.light_on = true;
		std::cout<<"OFF\n";
	}

	//Schedule the next on-off pulse
	if(full_changer.delay_counter != full_changer.data.size())
	{
		sio_stat.shift_counter = 0;
		sio_stat.shift_clock = (12 * (full_changer.data[full_changer.delay_counter])) + (6 * (full_changer.data[full_changer.delay_counter] - 1)) + 20;
		sio_stat.shifts_left = 1;
		std::cout<<"DELAY -> " << std::dec << sio_stat.shift_clock << "\n";
		std::cout<<"COUNT -> " << std::dec << (u16)full_changer.delay_counter << "\n\n";

		//Set up next delay
		full_changer.delay_counter++;
	}

	else { full_changer.current_state = FULL_CHANGER_INACTIVE; std::cout<<"FINISH\n"; }
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
