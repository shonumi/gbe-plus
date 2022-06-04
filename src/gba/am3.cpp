// GB Enhanced+ Copyright Daniel Baxter 2022
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : am3.cpp
// Date : June 04, 2022
// Description : AM3 related functions
//
// Handles I/O and file management for AM3 Advance Movie Adapter emulation

#include "mmu.h"
#include "common/util.h" 

/****** Read AM3 firmware file into memory ******/
bool AGB_MMU::read_am3_firmware(std::string filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary);

	if(!file.is_open()) 
	{
		std::cout<<"MMU::AM3 Bootstrap file " << filename << " could not be opened. Check file path or permissions. \n";
		return false;
	}

	//Get the file size
	file.seekg(0, file.end);
	u32 file_size = file.tellg();
	file.seekg(0, file.beg);

	am3.firmware_data.clear();
	am3.firmware_data.resize(file_size, 0x00);
	
	u8* ex_mem = &am3.firmware_data[0];

	//Read data from the firmware file
	file.read((char*)ex_mem, file_size);

	file.close();

	//Copy 1st 1KB of firmware to 0x8000000
	for(u32 x = 0; x < 0x400; x++) { memory_map[0x8000000 + x] = am3.firmware_data[x]; }

	std::cout<<"MMU::AM3 firmware file " << filename << " loaded successfully. \n";

	return true;
}

/****** Read AM3 16-byte DES key into memory ******/
bool AGB_MMU::read_des_key(std::string filename)
{
	am3.des_key.clear();
	am3.des_key.resize(16, 0x00);

	std::ifstream file(filename.c_str(), std::ios::binary);

	if(!file.is_open()) 
	{
		std::cout<<"MMU::AM3 DES key file " << filename << " could not be opened. Check file path or permissions. \n";
		file.close();
		return false;
	}

	//Get the file size
	file.seekg(0, file.end);
	u32 file_size = file.tellg();
	file.seekg(0, file.beg);

	if(file_size != 16)
	{
		std::cout<<"MMU::AM3 DES key file is not 16-bytes in size\n";
		file.close();
		return false;		
	}
	
	u8* ex_mem = &am3.des_key[0];

	//Read data from the DES key file
	file.read((char*)ex_mem, file_size);

	file.close();

	std::cout<<"MMU::AM3 DES key file " << filename << " loaded successfully. \n";

	return true;
}

/****** Writes data to AM3 cartridge I/O ******/
void AGB_MMU::write_am3(u32 address, u8 value)
{
	switch(address)
	{
		case AM_BLK_SIZE:
		case AM_BLK_SIZE+1:
			am3.blk_size &= ~(0xFF << ((address & 0x1) << 3));
			am3.blk_size |= (value << ((address & 0x1) << 3));
			break;

		case AM_BLK_ADDR:
		case AM_BLK_ADDR+1:
		case AM_BLK_ADDR+2:
		case AM_BLK_ADDR+3:
			break;

		case AM_SMC_SIZE:
		case AM_SMC_SIZE+1:
			am3.smc_size &= ~(0xFF << ((address & 0x1) << 3));
			am3.smc_size |= (value << ((address & 0x1) << 3));
			break;

		case AM_SMC_OFFS:
		case AM_SMC_OFFS+1:
		case AM_SMC_OFFS+2:
		case AM_SMC_OFFS+3:
			am3.smc_offset &= ~(0xFF << ((address & 0x3) << 3));
			am3.smc_offset |= (value << ((address & 0x3) << 3));

			//Reset reading address if a new offset is passed
			if((address == (AM_SMC_OFFS+3)) && (am3.smc_offset != am3.last_offset))
			{
				am3.last_offset = am3.smc_offset;
				am3.base_addr += am3.smc_offset;

				if(am3.base_addr < am3.file_addr_list[am3.file_index]) { am3.base_addr = am3.file_addr_list[am3.file_index]; }
			}

			break;

		case AM_SMC_FILE:
		case AM_SMC_FILE+1:
			am3.file_index &= ~(0xFF << ((address & 0x1) << 3));
			am3.file_index |= (value << ((address & 0x1) << 3));

			if(address == (AM_SMC_FILE+1))
			{	
				//Special Case - If the latest file index is 0xFFFF, reset file index to zero
				if(am3.file_index == 0xFFFF) { am3.file_index = 0; }

				//Special Case - If the latest file index invalid, access the last file (INFO file)
				if(am3.file_index > am3.file_count) { am3.file_index = am3.file_count - 1; }

				am3.base_addr = am3.file_addr_list[am3.file_index];
				am3.file_size = am3.file_size_list[am3.file_index];
				am3.read_sm_card = true;
			}

			break;

		case AM_SMC_EOF:
		case AM_SMC_EOF+1:
			am3.remaining_size &= ~(0xFF << ((address & 0x1) << 3));
			am3.remaining_size |= (value << ((address & 0x1) << 3));

			break;

		case AM_BLK_STAT:
		case AM_BLK_STAT+1:
			am3.blk_stat &= ~(0xFF << ((address & 0x1) << 3));
			am3.blk_stat |= (value << ((address & 0x1) << 3));

			//Perform specific actions when settings certain bits - Operation is delayed a bit, emulated here based on CPU reads instead of actual execution time
			if(am3.blk_stat & 0x01) { am3.op_delay = 5; }

			//Update AM_BLK_STAT I/O register
			write_u16_fast(AM_BLK_STAT, am3.blk_stat);

			//Raise GamePak IRQ when AM_BLK_STAT changes to a non-zero value after writing to it
			if(am3.blk_stat) { memory_map[REG_IF+1] |= 0x20; }

			if(am3.blk_stat == 0x01) { am3.transfer_delay = 16; }
			if(am3.blk_stat == 0x03) { am3.transfer_delay = 1; }

			break;

		default:
			std::cout<<"UNKNOWN AM3 WRITE -> 0x" << address << " :: 0x" << (u32)value << "\n";
	}
}

/****** Checks whether a valid FAT exists in the AM3 SmartMedia card file - Extracts basic file data expected by AM3 hardware ******/
bool AGB_MMU::check_am3_fat()
{
	//Read Master Boot Record to determine location of Volume Boot Record (aka Volume ID)
	u32 vbr = ((am3.card_data[0x1C9] << 24) | (am3.card_data[0x1C8] << 16) | (am3.card_data[0x1C7] << 8) | am3.card_data[0x1C6]);
	vbr *= 0x200;

	std::cout<< std::hex << "AM3 -> FAT Volume Boot Record @ 0x" << vbr << "\n";

	//Read various bits of data to determine location of Data Region and Directory Table
	u16 bytes_per_sector = ((am3.card_data[vbr + 0x0C] << 8) | am3.card_data[vbr + 0x0B]);
	u16 reserved_sectors = ((am3.card_data[vbr + 0x0F] << 8) | am3.card_data[vbr + 0x0E]);
	u32 first_fat_addr = vbr + (reserved_sectors * bytes_per_sector);

	std::cout<<"AM3 -> First FAT @ 0x" << first_fat_addr << "\n";

	u8 num_of_fats = am3.card_data[vbr + 0x10];
	u16 sectors_per_fat = ((am3.card_data[vbr + 0x17] << 8) | am3.card_data[vbr + 0x16]);
	u16 max_root_dirs = ((am3.card_data[vbr + 0x12] << 8) | am3.card_data[vbr + 0x11]);
	u8 sectors_per_cluster = am3.card_data[vbr + 0x0D];

	u32 root_dir_addr = first_fat_addr + ((num_of_fats * sectors_per_fat) * bytes_per_sector);
	u32 data_region_addr = root_dir_addr + (max_root_dirs * 32);

	std::cout<<"AM3 -> Root Directory @ 0x" << root_dir_addr << "\n";
	std::cout<<"AM3 -> Data Region @ 0x" << data_region_addr << "\n";	

	//Read files from Directory Table
	u32 t_addr = data_region_addr;

	u32 info_pos = 0;
	u32 info_size = 0;

	u32 zero_pos = 0;
	u32 zero_size = 0;

	u8 fname_size = 0;

	std::vector<std::string> temp_file_list;
	std::vector<u32> temp_addr_list;
	std::vector<u32> temp_size_list;
	std::string current_file = "";

	//Grab filenames, size, and location from Root Directory
	while(t_addr < (data_region_addr + 0x200))
	{
		//Pull filename, wait until spaces
		u8 current_chr = am3.card_data[t_addr + fname_size];

		//Check for a valid filename character
		if((current_chr != 0x2E) && (current_chr >= 0x20))
		{
			//Make sure filename is not case-sensitive. Convert all lower-case ASCII to uppercase
			if((current_chr >= 0x61) && (current_chr <= 0x7A)) { current_chr -= 0x20; }

			current_file += current_chr;
			fname_size++;

			//Add filename to first list found in Root Directory, store file sizes and their addresses
			if(fname_size == 0x0B)
			{
				fname_size = 0;
				temp_file_list.push_back(current_file);
				current_file = "";

				//Grab and store file size
				u32 f_size = ((am3.card_data[t_addr + 0x1F] << 24) | (am3.card_data[t_addr + 0x1E] << 16) | (am3.card_data[t_addr + 0x1D] << 8) | am3.card_data[t_addr + 0x1C]);
				temp_size_list.push_back(f_size);
				am3.file_count++;

				//Calculate and store file offset in SmartMedia dump
				u32 f_pos = ((am3.card_data[t_addr + 0x1B] << 8) | (am3.card_data[t_addr + 0x1A]));
				f_pos = data_region_addr + ((f_pos - 2) * sectors_per_cluster * bytes_per_sector);
				temp_addr_list.push_back(f_pos);

				std::cout<<"AM3 File Found @ 0x" << f_pos << " :: Size 0x" << f_size << "\n";

				t_addr += 0x20;
			}
		}

		//For invalid filename character, skip to next file
		else
		{
			fname_size = 0;
			current_file = "";
			t_addr += 0x20;
		}
	}

	bool found_info = false;

	//Look up "INFO    AM3" and pull filenames from that table
	for(u32 x = 0; x < temp_file_list.size(); x++)
	{
		if(temp_file_list[x] == "INFO    AM3")
		{
			t_addr = temp_addr_list[x] + 0x200;
			found_info = true;
		}
	}

	//Abort if "INFO    " file does not exist
	if(!found_info)
	{
		std::cout<<"Error - AM3 FAT has no INFO.AM3 file\n";
		return false;
	}

	u32 info_table = t_addr + 0x200;

	//Grab filenames from INFO file. Compare them to the Root Directory. The order they appear in INFO is how the adapter sorts them
	while(t_addr < info_table)
	{
		//Pull filename
		u8 current_chr = am3.card_data[t_addr + fname_size + 0x08];

		//Make sure filename is not case-sensitive. Convert all lower-case ASCII to uppercase
		if((current_chr >= 0x61) && (current_chr <= 0x7A)) { current_chr -= 0x20; }

		current_file += current_chr;
		fname_size++;

		//Check current filename against previous list and add them for the emulated AM3 adapter in the correct order
		if(fname_size == 0x0B)
		{
			for(u32 x = 0; x < temp_file_list.size(); x++)
			{
				if(temp_file_list[x] == current_file)
				{
					am3.file_size_list.push_back(temp_size_list[x]);
					am3.file_addr_list.push_back(temp_addr_list[x]);
				}
			}

			fname_size = 0;
			current_file = "";
			t_addr += 0x20;
		}
	}

	if(!am3.file_count)
	{
		std::cout<<"Error - No files found in AM3 File Allocation Table \n";
		return false;
	}

	return true;
}
