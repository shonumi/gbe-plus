// GB Enhanced+ Copyright Daniel Baxter 2022
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : am3.cpp
// Date : June 04, 2022
// Description : AM3 related functions
//
// Handles I/O and file management for AM3 Advance Movie Adapter emulation

#include <filesystem>
#include <algorithm>

#include "mmu.h"
#include "common/util.h" 

/****** Resets AM3 data structure ******/
void AGB_MMU::am3_reset()
{
	am3.read_key = true;
	am3.read_sm_card = false;
	
	am3.op_delay = 0;
	am3.transfer_delay = 0;
	am3.base_addr = 0x400;

	am3.blk_stat = 0;
	am3.blk_size = 0x400;
	am3.blk_addr = 0x8000000;

	am3.smc_offset = 0;
	am3.smc_size = 0x400;
	am3.smc_base = 0;
	am3.last_offset = 0;

	am3.file_index = 0;
	am3.file_count = 0;
	am3.file_size = 0;
	am3.file_size_list.clear();
	am3.file_addr_list.clear();

	am3.remaining_size = 0x400;

	am3.firmware_data.clear();
	am3.card_data.clear();
}

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
	u32 file_size = util::get_file_size(filename);

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

/****** Read AM3 16-byte SmartMedia ID into memory ******/
bool AGB_MMU::read_smid(std::string filename)
{
	am3.smid.clear();
	am3.smid.resize(16, 0x00);

	std::ifstream file(filename.c_str(), std::ios::binary);

	if(!file.is_open()) 
	{
		std::cout<<"MMU::AM3 SmartMedia ID file " << filename << " could not be opened. Check file path or permissions. \n";
		file.close();
		return false;
	}

	//Get the file size
	u32 file_size = util::get_file_size(filename);

	if(file_size != 16)
	{
		std::cout<<"MMU::AM3 DES key file is not 16-bytes in size\n";
		file.close();
		return false;		
	}
	
	u8* ex_mem = &am3.smid[0];

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
	u32 region_limit = 0x200;

	u32 info_pos = 0;
	u32 info_size = 0;

	u32 zero_pos = 0;
	u32 zero_size = 0;

	u8 fname_size = 0;

	std::vector<std::string> temp_file_list;
	std::vector<u32> temp_addr_list;
	std::vector<u32> temp_size_list;
	std::string current_file = "";

	bool is_frag_detected = false;

	//Grab filenames, size, and location from Root Directory
	while(t_addr < (data_region_addr + region_limit))
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

				//Calculate and store file offset in SmartMedia dump
				u32 f_pos = ((am3.card_data[t_addr + 0x1B] << 8) | (am3.card_data[t_addr + 0x1A]));
				f_pos = data_region_addr + ((f_pos - 2) * sectors_per_cluster * bytes_per_sector);
				temp_addr_list.push_back(f_pos);

				std::cout<<"AM3 File Found @ 0x" << f_pos << " :: Size 0x" << f_size << "\n";

				//Add warning if file appear fragmented
				if(temp_file_list.size() >= 2)
				{
					s32 current_addr = f_pos;
					s32 last_addr = temp_addr_list[temp_addr_list.size() - 2];
					s32 last_size = temp_size_list[temp_size_list.size() - 2];

					if((current_addr - last_addr) < last_size) { is_frag_detected = true; }
				}

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

		//Test for more data to read
		if(t_addr == (data_region_addr + region_limit))
		{
			u32 test_val = ((am3.card_data[t_addr+3] << 24) | (am3.card_data[t_addr+2] << 16) | (am3.card_data[t_addr+1] << 8) | am3.card_data[t_addr]);
			if(test_val) { region_limit += 0x20; }
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

	if(is_frag_detected)
	{
		std::cout<<"MMU::Warning - Possible file fragmentation detected in AM3 SmartMedia image\n";
		std::cout<<"MMU::Warning - Recommended to extract or mount SmartCard image to a folder\n";
	}

	u32 info_table = t_addr;
	region_limit = 0x200;

	//Grab filenames from INFO file. Compare them to the Root Directory. The order they appear in INFO is how the adapter sorts them
	while(t_addr < (info_table + region_limit))
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
					am3.file_count++;
				}
			}

			fname_size = 0;
			t_addr += 0x20;

			//Test for more data to read
			if(t_addr == (info_table + region_limit))
			{
				if((current_file[0] != 0) && (current_file[0] != 0xFF)) { region_limit += 0x20; }
			}

			current_file = "";
		}
	}

	if(!am3.file_count)
	{
		std::cout<<"Error - No files found in AM3 File Allocation Table \n";
		return false;
	}

	return true;
}

/****** Loads AM3 files from a folder ******/
bool AGB_MMU::am3_load_folder(std::string folder)
{
	std::vector<std::string> file_list;
	std::vector<std::string> find_list;
	std::vector<std::string> final_path;
	std::vector<u32> file_size;

	//Check to see if folder exists, then grab all files there (non-recursive)
	std::filesystem::path fs_path { folder };

	if(!std::filesystem::exists(fs_path))
	{
		std::cout<<"AM3::Error - AM3 Folder at " << folder << "does not exist.\n";
		return false;
	}

	//Check to see that the path points to a folder
	if(!std::filesystem::is_directory(fs_path))
	{
		std::cout<<"AM3::Error - " << folder << "is not a folder.\n";
		return false;
	}

	bool info_file_found = false;
	bool smid_file_found = false;
	u32 info_index = 0;
	u32 file_count = 0;

	//Cycle through all available files in the folder
	std::filesystem::directory_iterator fs_files;

	for(fs_files = std::filesystem::directory_iterator(fs_path); fs_files != std::filesystem::directory_iterator(); fs_files++, file_count++)
	{
		//Grab name and size
		std::string f_name = fs_files->path().string();
		std::string s_name = fs_files->path().filename().string();
		u32 f_size = util::get_file_size(f_name);

		//Convert name to uppercase for searching
		for(u32 x = 0; x < s_name.length(); x++)
		{
			u8 current_chr = s_name[x];
			if((current_chr >= 0x61) && (current_chr <= 0x7A)) { current_chr -= 0x20; }
			s_name[x] = current_chr;
		}

		file_list.push_back(f_name);
		find_list.push_back(s_name);
		file_size.push_back(f_size);

		//Check for info.am3
		if(s_name == "INFO.AM3")
		{
			info_file_found = true;
			info_index = file_count;
		}

		//Load SMID
		else if(s_name == "SMID.KEY")
		{
			smid_file_found = true;

			if(!read_smid(f_name))
			{
				std::cout<<"AM3::Error - Could not open SMID.KEY file " << f_name << "\n";
				return false;
			}
		} 
	}

	//Abort if info.am3 not found
	if(!info_file_found)
	{
		std::cout<<"AM3::Error - No INFO.AM3 file found in folder " << folder << "\n";
		return false;
	}

	//Abort if smid.key not found and smid.key is not auto-generated
	if(!smid_file_found && !config::auto_gen_am3_id)
	{
		std::cout<<"AM3::Error - No SMID.KEY file found in folder " << folder << "\n";
		return false;
	}

	//Setup dummy data for SMID if auto-generating
	else if(config::auto_gen_am3_id)
	{
		am3.smid.clear();
		am3.smid.resize(16, 0x00);
	}

	//Open up INFO.AM3 to grab list of relevant files
	std::ifstream am3_info(file_list[info_index].c_str(), std::ios::binary);
	
	if(!am3_info.is_open())
	{
		std::cout<<"AM3::Error - Could not open INFO.AM3 file in folder " << folder << "\n";
		return false;
	}

	std::vector <u8> info_data;
	info_data.resize(file_size[info_index], 0x00);

	u8* ex_mem = &info_data[0];
	am3_info.read((char*)ex_mem, file_size[info_index]);
	am3_info.close();

	u32 info_addr = 0x200;
	u32 fname_size = 0;
	std::string current_file;

	while(info_addr < file_size[info_index])
	{
		//Pull filename
		u8 current_chr = info_data[info_addr + fname_size + 0x08];

		//Make sure filename is not case-sensitive. Convert all lower-case ASCII to uppercase
		if((current_chr >= 0x61) && (current_chr <= 0x7A)) { current_chr -= 0x20; }

		if(current_chr > 0x20) { current_file += current_chr; }
		fname_size++;

		//Add period
		if(fname_size == 0x08) { current_file += "."; }

		//Check current filename against previous list and add them for the emulated AM3 adapter in the correct order
		if(fname_size == 0x0B)
		{
			for(u32 x = 0; x < find_list.size(); x++)
			{
				if(find_list[x] == current_file)
				{
					am3.file_size_list.push_back(file_size[x]);
					final_path.push_back(file_list[x]);
					am3.file_count++;
				}
			}

			fname_size = 0;
			info_addr += 0x20;

			current_file = "";
		}
	}

	u32 am3_index = 0;
	am3.card_data.clear();

	//Pull up all files, read their data, mark file locations
	for(u32 x = 0; x < am3.file_count; x++)
	{
		//Open up AM3 file
		std::ifstream am3_file(final_path[x].c_str(), std::ios::binary);

		if(!am3_file.is_open())
		{
			std::cout<<"AM3::Error - Could not open AM3 file " << final_path[x] << "\n";
			return false;
		}

		//Update current file location
		am3.file_addr_list.push_back(am3_index);

		//Grab and copy binary data
		std::vector <u8> file_data;
		file_data.resize(am3.file_size_list[x], 0x00);

		u8* ex_data = &file_data[0];
		am3_file.read((char*)ex_data, am3.file_size_list[x]);
		am3_file.close();

		for(u32 y = 0; y < file_data.size(); y++) { am3.card_data.push_back(file_data[y]); }
		am3_index += file_data.size();

		//Pad card data with zeroes to nearest 0x200 block
		u32 pad_size = (0x200 - (file_data.size() % 0x200));

		for(u32 y = 0; y < pad_size; y++) { am3.card_data.push_back(0); }
		am3_index += pad_size;
	}

	return true;
}
