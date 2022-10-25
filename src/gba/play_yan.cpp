// GB Enhanced+ Copyright Daniel Baxter 2022
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : play_yan.cpp
// Date : August 19, 2022
// Description : Nintendo Play-Yan
//
// Handles I/O for the Nintendo Play-Yan
// Manages IRQs and firmware reads/writes

#include "mmu.h"
#include "common/util.h"

/****** Resets Play-Yan data structure ******/
void AGB_MMU::play_yan_reset()
{
	play_yan.card_data.clear();
	play_yan.card_data.resize(0x10000, 0x00);
	play_yan.card_addr = 0;

	play_yan.video_data.resize(0x12C00, 0x3C);

	play_yan.firmware.clear();
	play_yan.firmware.resize(0x100000, 0x00);

	play_yan.firmware_addr = 0;
	play_yan.firmware_status = 0x10;
	play_yan.firmware_addr_count = 0;

	play_yan.status = 0x80;
	play_yan.op_state = 0;

	play_yan.access_mode = 0;
	play_yan.access_param = 0;

	play_yan.irq_count = 0;
	play_yan.irq_repeat = 0;
	play_yan.irq_repeat_id = 0;
	play_yan.irq_delay = 0;
	play_yan.delay_reload = 60;
	play_yan.irq_data_in_use = false;
	play_yan.start_irqs = false;

	play_yan.irq_data_ptr = play_yan.sd_check_data[0];
	play_yan.irq_len = 5;

	play_yan.is_video_playing = false;
	play_yan.is_music_playing = false;

	//Read Play-Yan data files
	if(config::cart_type == AGB_PLAY_YAN)
	{
		read_play_yan_file_list((config::data_path + "play_yan/music.txt"), 0);
		read_play_yan_file_list((config::data_path + "play_yan/video.txt"), 1);

		//Set default data pulled from SD card - Index 0 for first music file
		play_yan_set_music_file(0);

		read_play_yan_thumbnails(config::data_path + "play_yan/thumbnails.txt");
	}

	for(u32 x = 0; x < 12; x++) { play_yan.cnt_data[x] = 0; }
	play_yan.cmd = 0;

	for(u32 x = 0; x < 5; x++)
	{
		for(u32 y = 0; y < 8; y++)
		{
			play_yan.sd_check_data[x][y] = 0x55555555;
		}
	}

	for(u32 x = 0; x < 2; x++)
	{
		for(u32 y = 0; y < 8; y++)
		{
			play_yan.music_check_data[x][y] = 0x0;
			play_yan.music_stop_data[x][y] = 0x0;
			play_yan.video_play_data[x][y] = 0x0;
			play_yan.video_stop_data[x][y] = 0x0;
		}
	}

	for(u32 x = 0; x < 3; x++)
	{
		for(u32 y = 0; y < 8; y++)
		{
			play_yan.music_play_data[x][y] = 0x0;
		}
	}

	for(u32 x = 0; x < 4; x++)
	{
		for(u32 y = 0; y < 8; y++)
		{
			play_yan.video_check_data[x][y] = 0x0;
		}
	}

	//Set 32-bit flags for SD Check interrupts
	play_yan.sd_check_data[0][0] = 0x80000100; play_yan.sd_check_data[0][1] = 0x00000000;
	play_yan.sd_check_data[1][0] = 0x40008000; play_yan.sd_check_data[1][1] = 0x00000005; play_yan.sd_check_data[1][2] = 0x00000000;
	play_yan.sd_check_data[2][0] = 0x40800000; play_yan.sd_check_data[2][1] = 0x00000005; play_yan.sd_check_data[2][2] = 0x00000000;
	play_yan.sd_check_data[3][0] = 0x80000100; play_yan.sd_check_data[3][1] = 0x00000000;
	play_yan.sd_check_data[4][0] = 0x40000200; play_yan.sd_check_data[4][1] = 0x00042C78;

	//Set 32-bit flags for entering/exiting music menu
	play_yan.music_check_data[0][0] = 0x80001000;
	play_yan.music_check_data[1][0] = 0x40000200;

	//Set 32-bit flags for entering/exiting video menu
	play_yan.video_check_data[0][0] = 0x40800000;
	play_yan.video_check_data[1][0] = 0x80000100;
	play_yan.video_check_data[2][0] = 0x40000200;
	play_yan.video_check_data[3][0] = 0x40000500;

	//Set 32-bit flags for playing music
	play_yan.music_play_data[0][0] = 0x40000600;
	play_yan.music_play_data[1][0] = 0x40000800;
	play_yan.music_play_data[2][0] = 0x80001000;

	//Set 32-bit flags for stopping music
	play_yan.music_stop_data[0][0] = 0x40000801;
	play_yan.music_stop_data[1][0] = 0x80001000;

	//Set 32-bit flags for playing video
	play_yan.video_play_data[0][0] = 0x40000700;
	play_yan.video_play_data[1][0] = 0x80001000; play_yan.video_play_data[1][1] = 0x31AC0; play_yan.video_play_data[1][2] = 0x12C00;

	//Set 32-bit flags for stopping video
	play_yan.video_stop_data[0][0] = 0x40000701;
	play_yan.video_stop_data[1][0] = 0x80001000;

	//Set 32-bit flags for waking from sleep
	play_yan.wake_data[0] = 0x80010001;

	for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }

	play_yan.video_progress = 0;
	play_yan.video_length = 0;

	play_yan.video_data_addr = 0;
	play_yan.thumbnail_addr = 0;
	play_yan.thumbnail_index = 0xFFFFFFFF;

	play_yan.music_file_index = 0;
	play_yan.video_file_index = 0;

	play_yan.use_bass_boost = false;
}

/****** Writes to Play-Yan I/O ******/
void AGB_MMU::write_play_yan(u32 address, u8 value)
{
	//std::cout<<"PLAY-YAN WRITE -> 0x" << address << " :: 0x" << (u32)value << "\n";

	switch(address)
	{
		//Unknown I/O
		case PY_UNK_00:
		case PY_UNK_00+1:
		case PY_UNK_02:
		case PY_UNK_02+1:
			break;

		//Access Mode (determines firmware read/write)
		case PY_ACCESS_MODE:
			play_yan.access_mode = value;
			break;

		//Firmware address
		case PY_FIRM_ADDR:
			if(play_yan.firmware_addr_count < 2)
			{
				play_yan.firmware_addr &= ~0xFF;
				play_yan.firmware_addr |= value;
			}

			else
			{
				play_yan.firmware_addr &= ~0xFF0000;
				play_yan.firmware_addr |= (value << 16);
			}

			play_yan.firmware_addr_count++;
			play_yan.firmware_addr_count &= 0x3;
			play_yan.card_addr = 0;

			break;

		//Firmware address
		case PY_FIRM_ADDR+1:
			if(play_yan.firmware_addr_count < 2)
			{
				play_yan.firmware_addr &= ~0xFF00;
				play_yan.firmware_addr |= (value << 8);
			}

			else
			{
				play_yan.firmware_addr &= ~0xFF000000;
				play_yan.firmware_addr |= (value << 24);
			}

			play_yan.firmware_addr_count++;
			play_yan.firmware_addr_count &= 0x3;

			break;

		//Parameter for reads and writes
		case PY_ACCESS_PARAM:
			play_yan.access_param &= ~0xFF;
			play_yan.access_param |= value;
			break;

		//Parameter for reads and writes
		case PY_ACCESS_PARAM+1:
			play_yan.access_param &= ~0xFF00;
			play_yan.access_param |= (value << 8);
			break;
	}

	//Write to firmware area
	if((address >= 0xB000100) && (address < 0xB000300) && ((play_yan.access_param == 0x09) || (play_yan.access_param == 0x0A)))
	{
		u32 offset = address - 0xB000100;
		
		if((play_yan.firmware_addr + offset) < 0xFF020)
		{
			play_yan.firmware[play_yan.firmware_addr + offset] = value;

			//Start IRQs automatically after first write to firmware
			if(!play_yan.start_irqs)
			{
				play_yan.start_irqs = true;
				play_yan.irq_delay = 240;
			}
		}
	}

	//Write to ... something else (control structure?)
	if((address >= 0xB000100) && (address <= 0xB00010B) && (play_yan.firmware_addr >= 0xFF020))
	{
		u32 offset = address - 0xB000100;

		if(offset <= 0x0B) { play_yan.cnt_data[offset] = value; }

		//Check for control command
		if(address == 0xB000103)
		{
			u32 prev_cmd = play_yan.cmd;
			play_yan.cmd = ((play_yan.cnt_data[3] << 24) | (play_yan.cnt_data[2] << 16) | (play_yan.cnt_data[1] << 8) | (play_yan.cnt_data[0]));

			//std::cout<<"CMD -> 0x" << play_yan.cmd << "\n";

			//Trigger Game Pak IRQ for entering/exiting music or video menu
			if((play_yan.cmd == 0x400) && (play_yan.op_state == 0xFF) && (play_yan.irq_data[0] != 0x40000500))
			{
				play_yan.op_state = 2;
				play_yan.irq_delay = 1;
				play_yan.delay_reload = 10;
				play_yan.irq_data_ptr = play_yan.music_check_data[0];
				play_yan.irq_len = 2;
			}

			//Trigger Game Pak IRQ for entering/exiting video menu
			else if((play_yan.cmd == 0x800000) && (play_yan.op_state == 0xFF))
			{
				play_yan.op_state = 3;
				play_yan.irq_delay = 1;
				play_yan.delay_reload = 10;
				play_yan.irq_data_ptr = play_yan.video_check_data[0];
				play_yan.irq_len = 3;
			}

			//Before playing a video, reset thumbnail index based on multiples of 6
			else if((play_yan.cmd == 0x400) && (prev_cmd == 0x2000))
			{
				play_yan.thumbnail_index = (6 * (play_yan.thumbnail_index / 6)) - 1;
			}

			//Trigger Game Pak IRQ for video thumbnail data
			else if(play_yan.cmd == 0x500)
			{
				play_yan.op_state = 3;
				play_yan.irq_delay = 1;
				play_yan.delay_reload = 10;
				play_yan.irq_data_ptr = play_yan.video_check_data[0];
				play_yan.irq_len = 4;
				play_yan.irq_count = 3;

				//Update video thumbnail index
				play_yan.thumbnail_index++;
				if(play_yan.thumbnail_index >= play_yan.video_thumbnails.size()) { play_yan.thumbnail_index = 0; }
				
				play_yan.thumbnail_addr = 0;
			}

			//Trigger Game Pak IRQ for playing music
			else if(play_yan.cmd == 0x600)
			{
				play_yan.op_state = 6;
				play_yan.irq_delay = 1;
				play_yan.delay_reload = 10;
				play_yan.irq_data_ptr = play_yan.music_play_data[0];
				play_yan.irq_len = 3;
				play_yan.irq_repeat = 0;
				play_yan.irq_count = 0;
				play_yan.is_music_playing = true;
			}

			//Trigger Game Pak IRQ for playing video
			else if(play_yan.cmd == 0x700)
			{
				play_yan.op_state = 9;
				play_yan.irq_delay = 1;
				play_yan.delay_reload = 10;
				play_yan.irq_data_ptr = play_yan.video_play_data[0];
				play_yan.irq_len = 1;
				play_yan.irq_repeat = 0;
				play_yan.irq_count = 0;
				play_yan.video_data_addr = 0;
				play_yan.video_progress = 0;
				play_yan.video_length = (15 * 0x20 * 30);
				play_yan.is_video_playing = true;
			}

			//Trigger Game Pak IRQ for stopping video
			else if(play_yan.cmd == 0x701)
			{
				play_yan.op_state = 10;
				play_yan.irq_delay = 1;
				play_yan.delay_reload = 10;
				play_yan.irq_data_ptr = play_yan.video_stop_data[0];
				play_yan.irq_len = 2;
				play_yan.irq_repeat = 0;
				play_yan.irq_count = 0;
				play_yan.video_data_addr = 0;
				play_yan.video_progress = 0;
				play_yan.is_video_playing = false;
			}
				
			//Trigger Game Pak IRQ for stopping music
			else if(play_yan.cmd == 0x801)
			{
				play_yan.op_state = 8;
				play_yan.irq_delay = 1;
				play_yan.delay_reload = 10;
				play_yan.irq_data_ptr = play_yan.music_stop_data[0];
				play_yan.irq_len = 2;
				play_yan.irq_repeat = 0;
				play_yan.irq_count = 0;
				play_yan.is_music_playing = false;
			}
		}

		//Check for control command parameter
		else if(address == 0xB000107)
		{
			u32 control_cmd2 = ((play_yan.cnt_data[7] << 24) | (play_yan.cnt_data[6] << 16) | (play_yan.cnt_data[5] << 8) | (play_yan.cnt_data[4]));

			//Set music file data - Index 0 for first music file
			if((play_yan.cmd == 0x200) && (control_cmd2 == 0x02) && (play_yan.op_state > 1)) { play_yan_set_music_file(0); }

			//Set video file data - Index 0 for first video file
			if((play_yan.cmd == 0x200) && (control_cmd2 == 0x01) && (play_yan.op_state > 1)) { play_yan_set_video_file(0); }

			//Adjust Play-Yan volume settings
			if(play_yan.cmd == 0xB00) { play_yan.volume = control_cmd2; }

			//Adjust Play-Yan bass boost settings
			else if(play_yan.cmd == 0xD00) { play_yan.bass_boost = control_cmd2; }

			//Turn on/off Play-Yan bass boost
			else if(play_yan.cmd == 0xD01) { play_yan.use_bass_boost = (control_cmd2 == 0x8F0F) ? false : true; }
		}
	}
}

/****** Reads from Play-Yan I/O ******/
u8 AGB_MMU::read_play_yan(u32 address)
{
	u8 result = memory_map[address];

	switch(address)
	{
		//Some kind of data stream
		case PY_INIT_DATA:
			result = 0x00;
			break;

		//Play-Yan Status
		case PY_STAT:
			result = play_yan.status;
			break;

		//Parameter for reads and writes
		case PY_ACCESS_PARAM:
			result = (play_yan.access_param & 0xFF);
			break;

		//Parameter for reads and writes
		case PY_ACCESS_PARAM+1:
			result = ((play_yan.access_param >> 8) & 0xFF);
			break;

		//Firmware Status
		case PY_FIRM_STAT:
			result = (play_yan.firmware_status & 0xFF);
			break;

		//Firmware Status
		case PY_FIRM_STAT+1:
			result = ((play_yan.firmware_status >> 8) & 0xFF);
			break;
	}

	//Read IRQ data
	if((play_yan.irq_data_in_use) && (address >= 0xB000300) && (address < 0xB000320) && (play_yan.firmware_addr == 0xFF000))
	{
		u32 offset = (address - 0xB000300) >> 2;
		u8 shift = (address & 0x3);

		//Switch back to reading firmware after all IRQ data is read
		if(address == 0xB00031C) { play_yan.irq_data_in_use = false; }

		result = (play_yan.irq_data[offset] >> (shift << 3));
	}

	//Read from firmware area
	else if((address >= 0xB000300) && (address < 0xB000500) && ((play_yan.access_param == 0x09) || (play_yan.access_param == 0x0A)))
	{
		u32 offset = address - 0xB000300;
		
		if((play_yan.firmware_addr + offset) < 0xFF020)
		{
			result = play_yan.firmware[play_yan.firmware_addr + offset];

			//Update Play-Yan firmware address if necessary
			if(offset == 0x1FE) { play_yan.firmware_addr += 0x200; }
		}
	}

	//Read from SD card data
	else if((address >= 0xB000300) && (address < 0xB000500) && (play_yan.access_param == 0x08) && (play_yan.firmware_addr != 0xFF000))
	{
		u32 offset = address - 0xB000300;
		
		if((play_yan.card_addr + offset) < 0x10000)
		{
			result = play_yan.card_data[play_yan.card_addr + offset];

			//Update Play-Yan card address if necessary
			if(offset == 0x1FE) { play_yan.card_addr += 0x200; }
		}
	}

	//Read from video thumbnail data or video data
	else if((address >= 0xB000500) && (address < 0xB000700))
	{
		u32 offset = address - 0xB000500;

		//Thumbnail data
		if(!play_yan.is_video_playing)
		{
			u32 t_index = play_yan.thumbnail_index;
			u32 t_addr = play_yan.thumbnail_addr + offset;

			if(t_index < play_yan.video_thumbnails.size())
			{
				if(t_addr < 0x12C0)
				{
					result = play_yan.video_thumbnails[t_index][t_addr];

					//Update Play-Yan thubnail address if necessary
					if(offset == 0x1FE) { play_yan.thumbnail_addr += 0x200; }
				}
			}
		}

		//Video frame data
		else
		{
			u32 v_addr = play_yan.video_data_addr + offset;
			
			if(v_addr < 0x12C00)
			{
				result = play_yan.video_data[v_addr];

				//Update Play-Yan video data address if necessary
				if(offset == 0x1FE)
				{
					play_yan.video_data_addr += 0x200;
					
					//Grab new video frame
					if(play_yan.video_data_addr == 0x12C00)
					{
						play_yan.video_data_addr = 0;
					}
				}
			}
		}	
	}

	//std::cout<<"PLAY-YAN READ -> 0x" << address << " :: 0x" << (u32)result << "\n";

	return result;
}

/****** Handles Play-Yan interrupt requests including delays and what data to respond with ******/
void AGB_MMU::process_play_yan_irq()
{
	//Wait for a certain amount of frames to pass to simulate delays in Game Pak IRQs firing
	if(play_yan.irq_delay)
	{
		play_yan.irq_delay--;
		if(play_yan.irq_delay) { return; }
	}

	//Process SD card check first and foremost after booting
	if(!play_yan.op_state)
	{
		play_yan.op_state = 1;
		play_yan.irq_count = 0;
	}

	switch(play_yan.op_state)
	{
		//SD card check
		case 0x1:

		//Enter/exit music menu
		case 0x2:

		//Enter/exit vieo menu
		case 0x3:

		//Process video thumbnails
		case 0x5:

		//Play music
		case 0x6:

		//Continue playing music
		case 0x7:

		//Stop playing music
		case 0x8:

		//Play video
		case 0x9:

		//Stop video
		case 0xA:

			//Trigger Game Pak IRQ
			memory_map[REG_IF+1] |= 0x20;

			//Wait for next IRQ condition after sending all flags
			if(play_yan.irq_count == play_yan.irq_len)
			{
				//After selecting a music file to play, fire IRQs to process audio
				if(play_yan.op_state == 0x06)
				{
					play_yan.op_state = 7;
					play_yan.irq_delay = 1;
					play_yan.delay_reload = 60;
					play_yan.irq_data_ptr = play_yan.music_play_data[0];
					play_yan.irq_count = 2;
					play_yan.irq_len = 3;
					play_yan.irq_repeat = 100;
				}

				//After selecting a video file to play, fire IRQs to process video
				else if(play_yan.op_state == 0x09)
				{
					play_yan.op_state = 8;
					play_yan.irq_delay = 1;
					play_yan.delay_reload = 2;
					play_yan.irq_data_ptr = play_yan.video_play_data[0];
					play_yan.irq_count = 1;
					play_yan.irq_len = 2;
					play_yan.irq_repeat = 10000;
				}

				//Repeat thumbnail IRQs as necessary
				else if((play_yan.op_state == 0x05) && (play_yan.irq_repeat))
				{
					play_yan.op_state = 5;
					play_yan.irq_delay = 1;
					play_yan.delay_reload = 10;
					play_yan.irq_data_ptr = play_yan.video_check_data[0];
					play_yan.irq_count = 3;
					play_yan.irq_len = 4;
					play_yan.irq_repeat--;
				}

				//Repeat music IRQs as necessary
				else if((play_yan.op_state == 0x07) && (play_yan.irq_repeat))
				{
					play_yan.op_state = 7;
					play_yan.irq_delay = 1;
					play_yan.delay_reload = 60;
					play_yan.irq_data_ptr = play_yan.music_play_data[0];
					play_yan.irq_count = 2;
					play_yan.irq_len = 3;
					play_yan.irq_repeat--;
				}

				//Repeat video IRQs as necessary
				else if((play_yan.op_state == 0x08) && (play_yan.irq_repeat))
				{
					play_yan.op_state = 8;
					play_yan.irq_delay = 1;
					play_yan.delay_reload = 1;
					play_yan.irq_data_ptr = play_yan.video_play_data[0];
					play_yan.irq_count = 1;
					play_yan.irq_len = 2;
					play_yan.irq_repeat--;

					//Update video progress via IRQ data
					play_yan.video_progress += 0x20;
					play_yan.video_play_data[1][6] = play_yan.video_progress;

					//Stop video when length is complete
					if(play_yan.video_progress >= play_yan.video_length)
					{
						play_yan.op_state = 10;
						play_yan.irq_delay = 1;
						play_yan.delay_reload = 10;
						play_yan.irq_data_ptr = play_yan.video_stop_data[0];
						play_yan.irq_len = 2;
						play_yan.irq_repeat = 0;
						play_yan.irq_count = 0;
						play_yan.video_data_addr = 0;
						play_yan.video_progress = 0;
						play_yan.is_video_playing = false;
					}
				}

				//Stop IRQs until next trigger condition
				else
				{
					play_yan.op_state = 0xFF;
					play_yan.irq_delay = 0;
					play_yan.irq_count = 0;
				}
			}

			//Send data for IRQ
			else
			{
				//Copy IRQ data from given array pointer
				//For 2D arrays, also account for multiple IRQs
				for(u32 x = 0; x < 8; x++)
				{
					play_yan.irq_data[x] = *(play_yan.irq_data_ptr + (play_yan.irq_count * 8) + x);
				}

				std::cout<<"IRQ -> 0x" << play_yan.irq_data[0] << "\n";

				play_yan.irq_count++;
				play_yan.irq_delay = play_yan.delay_reload;
				play_yan.irq_data_in_use = true;
			}

			break;
	}
}

/****** Reads a file for list of audio files to be read by the Play-Yan ******/
bool AGB_MMU::read_play_yan_file_list(std::string filename, u8 category)
{
	std::vector<std::string> *out_list = NULL;
	std::vector<u32> *out_time = NULL;

	//Grab the correct file list based on category
	switch(category)
	{
		case 0x00: out_list = &play_yan.music_files; out_time = &play_yan.music_times; break;
		case 0x01: out_list = &play_yan.video_files; out_time = &play_yan.video_times; break;
		default: std::cout<<"MMU::Error - Loading unknown category of media files for Play Yan\n"; return false;
	}

	//Clear any previosly existing contents, read in each non-blank line from the specified file
	out_list->clear();
	out_time->clear();

	std::string input_line = "";
	std::ifstream file(filename.c_str(), std::ios::in);

	if(!file.is_open())
	{
		std::cout<<"MMU::Error - Could not open list of media files from " << filename << "\n";
		return false;
	}

	//Parse line for filename, time, and any other data. Data is separated by a colon
	while(getline(file, input_line))
	{
		if(!input_line.empty())
		{
			std::size_t parse_symbol;
			s32 pos = 0;

			std::string out_str = "";
			std::string out_title = "";
			u32 out_sec = 0;

			bool end_of_string = false;

			//Grab filename
			parse_symbol = input_line.find(":", pos);
			
			if(parse_symbol == std::string::npos)
			{
				out_str = input_line;
				out_sec = 0;
				end_of_string = true;
			}

			else
			{
				out_str = input_line.substr(pos, parse_symbol);
				pos += parse_symbol;
			}

			//Grab time in seconds
			parse_symbol = input_line.find(":", pos);

			if(parse_symbol == std::string::npos)
			{
				out_sec = 0;
				end_of_string = true;
			}
			
			else
			{
				s32 end_pos = input_line.find(":", (pos + 1));

				if(end_pos == std::string::npos)
				{
					util::from_str(input_line.substr(pos + 1), out_sec);
					end_of_string = true;
				}

				else
				{
					util::from_str(input_line.substr((pos + 1), (end_pos - pos - 1)), out_sec);
					pos += (end_pos - pos);
				}

			}

			out_list->push_back(out_str);
			out_time->push_back(out_sec);
		}
	}

	std::cout<<"MMU::Loaded audio files for Play-Yan from " << filename << "\n";

	file.close();
	return true;
}


/****** Reads a file for list of video thumbnails files used for Play-Yan video ******/
bool AGB_MMU::read_play_yan_thumbnails(std::string filename)
{
	play_yan.video_thumbnails.clear();

	std::string input_line = "";
	std::ifstream file(filename.c_str(), std::ios::in);

	std::vector<std::string> list;

	if(!file.is_open())
	{
		std::cout<<"MMU::Error - Could not open list of media files from " << filename << "\n";
		return false;
	}

	//Parse line for filename
	while(getline(file, input_line))
	{
		if(!input_line.empty())
		{
			list.push_back(input_line);
		}
	}

	//Resize thumbnail vector
	play_yan.video_thumbnails.resize(list.size());

	//Grab pixel data for each thumbnail
	for(u32 x = 0; x < list.size(); x++)
	{
		//Grab pixel data of file as a BMP
		std::string t_file = config::data_path + "play_yan/" + list[x];
		SDL_Surface* source = SDL_LoadBMP(t_file.c_str());

		//Generate blank (all black) thumbnail if source not found
		if(source == NULL)
		{
			std::cout<<"MMU::Warning - Could not load thumbnail image for " << list[x] << "\n";
			play_yan.video_thumbnails[x].resize(0x12C0, 0x00);
		}

		//Otherwise grab pixel data from source
		else
		{
			play_yan.video_thumbnails[x].clear();
			u8* pixel_data = (u8*)source->pixels;

			//Convert 32-bit pixel data to RGB15 and push to vector
			for(int a = 0, b = 0; a < (source->w * source->h); a++, b+=3)
			{
				u16 raw_pixel = ((pixel_data[b] & 0xF8) << 7) | ((pixel_data[b+1] & 0xF8) << 2) | ((pixel_data[b+2] & 0xF8) >> 3);
				play_yan.video_thumbnails[x].push_back(raw_pixel & 0xFF);
				play_yan.video_thumbnails[x].push_back((raw_pixel >> 8) & 0xFF);
			}
		}
	}

	return true;
}

/****** Sets the current SD card data for a given music file via index ******/
void AGB_MMU::play_yan_set_music_file(u32 index)
{
	play_yan.card_data.clear();
	play_yan.card_data.resize(0x10000, 0x00);

	//Set data pulled from SD card
	if(!play_yan.music_files.empty())
	{
		//Set number of media files present
		play_yan.card_data[4 + (index * 268)] = play_yan.music_files.size();

		for(u32 index = 0; index < play_yan.music_files.size(); index++)
		{
			//Set number of media files present
			play_yan.card_data[4 + ((index + 1) * 268)] = 2;

			//Copy filename
			std::string sd_file = play_yan.music_files[index];

			for(u32 x = 0; x < sd_file.length(); x++)
			{
				u8 chr = sd_file[x];
				play_yan.card_data[8 + (index * 268) + x] = chr;
			}
		}
	}
}

/****** Sets the current SD card data for a given video file via index ******/
void AGB_MMU::play_yan_set_video_file(u32 index)
{
	play_yan.card_data.clear();
	play_yan.card_data.resize(0x10000, 0x00);

	//Set data pulled from SD card
	if(!play_yan.video_files.empty())
	{
		//Set number of media files present
		play_yan.card_data[4] = play_yan.video_files.size();

		for(u32 index = 0; index < play_yan.video_files.size(); index++)
		{
			//Copy filename
			std::string sd_file = play_yan.video_files[index];

			for(u32 x = 0; x < sd_file.length(); x++)
			{
				u8 chr = sd_file[x];
				play_yan.card_data[8 + (index * 268) + x] = chr;
			}
		}
	}
}

/****** Wakes Play-Yan from GBA sleep mode - Fires Game Pak IRQ ******/
void AGB_MMU::play_yan_wake()
{
	play_yan.op_state = 2;
	play_yan.irq_delay = 1;
	play_yan.delay_reload = 10;
	play_yan.irq_data_ptr = play_yan.wake_data;
	play_yan.irq_len = 1;
}
