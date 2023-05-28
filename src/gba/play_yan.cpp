// GB Enhanced+ Copyright Daniel Baxter 2022
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : play_yan.cpp
// Date : August 19, 2022
// Description : Nintendo Play-Yan
//
// Handles I/O for the AGS-006 family (Play-Yan, Play-Yan Micro, Nintendo MP3 Player)
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
	play_yan.irq_len = 1;

	play_yan.is_video_playing = false;
	play_yan.is_music_playing = false;
	play_yan.pause_media = false;

	for(u32 x = 0; x < 12; x++) { play_yan.cnt_data[x] = 0; }
	play_yan.cmd = 0;

	for(u32 x = 0; x < 4; x++)
	{
		for(u32 y = 0; y < 8; y++)
		{
			play_yan.sd_check_data[x][y] = 0x55555555;
			play_yan.folder_ops_data[x][y] = 0x0;
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
			play_yan.micro[x][y] = 0x0;
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

	//Set 32-bit flags for entering/exiting music menu
	play_yan.music_check_data[0][0] = 0x80001000;
	play_yan.music_check_data[1][0] = 0x40000200;

	//Set 32-bit flags for entering/exiting video menu
	play_yan.video_check_data[0][0] = 0x40800000;
	play_yan.video_check_data[1][0] = 0x80000100;
	play_yan.video_check_data[2][0] = 0x40000200;
	play_yan.video_check_data[3][0] = 0x40000500;

	//Set 32-bit flags for playing music
	play_yan.music_play_data[0][0] = 0x40000600; play_yan.music_play_data[0][1] = 0x1;
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

	//Set 32-bit flags for folder operations
	play_yan.folder_ops_data[0][0] = 0x40000200;
	play_yan.folder_ops_data[1][0] = 0x80001000;
	play_yan.folder_ops_data[2][0] = 0x40000201;
	play_yan.folder_ops_data[3][0] = 0x80001000;

	//Set 32-bit flags for Play-Yan Micro .ini file handling
	play_yan.micro[0][0] = 0x40003000;
	play_yan.micro[1][0] = 0x40003001;
	play_yan.micro[2][0] = 0x40003003;

	for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }

	play_yan.music_length = 0;
	play_yan.tracker_progress = 0;
	play_yan.video_progress = 0;
	play_yan.video_length = 0;
	play_yan.video_current_fps = 0;
	play_yan.video_frame_count = 0;
	play_yan.update_video_frame = false;

	play_yan.video_data_addr = 0;
	play_yan.thumbnail_addr = 0;
	play_yan.video_index = 0;

	play_yan.use_bass_boost = false;
	play_yan.capture_command_stream = false;

	play_yan.type = PLAY_YAN_OG;

	play_yan.current_dir = config::data_path + "play_yan";
	play_yan.base_dir = play_yan.current_dir;
	play_yan_set_folder();

	play_yan.current_music_file = "";
	play_yan.current_video_file = "";
}

/****** Writes to Play-Yan I/O ******/
void AGB_MMU::write_play_yan(u32 address, u8 value)
{
	//std::cout<<"PLAY-YAN WRITE -> 0x" << address << " :: 0x" << (u32)value << "\n";

	//Detect Nintendo MP3 Player
	if(((address >> 24) == 0x0E) && (play_yan.type != NINTENDO_MP3))
	{
		std::cout<<"MMU::Nintendo MP3 detected\n";
		play_yan.type = NINTENDO_MP3;
	}

	//Handle Nintendo MP3 Player writes separately
	if(play_yan.type == NINTENDO_MP3)
	{
		write_nmp(address, value);
		return;
	}

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
			if(play_yan.access_mode == 0x28) { play_yan.serial_cmd_index = 0; }

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

	//Write command and parameters (serial version)
	if((address >= 0xB000000) && (address <= 0xB000001) && (play_yan.firmware_addr >= 0xFF020) && (play_yan.access_mode == 0x28))
	{
		if(play_yan.serial_cmd_index <= 0x0B) { play_yan.cnt_data[play_yan.serial_cmd_index++] = value; }

		//Check for control command
		if(play_yan.serial_cmd_index == 4)
		{
			process_play_yan_cmd();	
		}

		//Check for control command parameter
		else if(play_yan.serial_cmd_index == 8)
		{
			u16 control_cmd2 = (play_yan.cnt_data[5] << 8) | (play_yan.cnt_data[4]);

			//Set music file data
			if((play_yan.cmd == 0x200) && (control_cmd2 == 0x02)) { play_yan_set_music_file(); }

			//Set video file data
			else if((play_yan.cmd == 0x200) && (control_cmd2 == 0x01)) { play_yan_set_video_file(); }

			//Set file data
			else if(play_yan.cmd == 0x200) { play_yan_set_folder(); }

			//Adjust Play-Yan volume settings
			if(play_yan.cmd == 0xB00) { play_yan.volume = control_cmd2; }

			//Adjust Play-Yan bass boost settings
			else if(play_yan.cmd == 0xD00) { play_yan.bass_boost = control_cmd2; }

			//Turn on/off Play-Yan bass boost
			else if(play_yan.cmd == 0xD01) { play_yan.use_bass_boost = (control_cmd2 == 0x8F0F) ? false : true; }
		}
	}

	//Write command and parameters (non-serial version)
	if((address >= 0xB000100) && (address <= 0xB00010B) && (play_yan.firmware_addr >= 0xFF020) && (play_yan.access_mode == 0x68))
	{
		u32 offset = address - 0xB000100;

		if(offset <= 0x0B) { play_yan.cnt_data[offset] = value; }

		//Check for control command
		if(address == 0xB000103)
		{
			process_play_yan_cmd();	
		}

		//Check for control command parameter
		else if(address == 0xB000107)
		{
			u32 control_cmd2 = ((play_yan.cnt_data[7] << 24) | (play_yan.cnt_data[6] << 16) | (play_yan.cnt_data[5] << 8) | (play_yan.cnt_data[4]));

			//Set music file data
			if((play_yan.cmd == 0x200) && (control_cmd2 == 0x02)) { play_yan_set_music_file(); }

			//Set video file data
			else if((play_yan.cmd == 0x200) && (control_cmd2 == 0x01)) { play_yan_set_video_file(); }

			//Set file data
			else if(play_yan.cmd == 0x200) { play_yan_set_folder(); }

			//Music position seeking
			else if((play_yan.cmd == 0x905) && (play_yan.is_music_playing))
			{
				//Update trackbar
				play_yan.music_play_data[2][5] = control_cmd2;

				//Update music progress via IRQ data
				double result = control_cmd2;
				result /= 25600;
				result *= play_yan.music_length;

				play_yan.music_play_data[2][6] = result;
			}

			//Adjust Play-Yan volume settings
			if(play_yan.cmd == 0xB00) { play_yan.volume = control_cmd2; }

			//Adjust Play-Yan bass boost settings
			else if(play_yan.cmd == 0xD00) { play_yan.bass_boost = control_cmd2; }

			//Turn on/off Play-Yan bass boost
			else if(play_yan.cmd == 0xD01) { play_yan.use_bass_boost = (control_cmd2 == 0x8F0F) ? false : true; }
		}
	}

	//Grab entire command stream
	if(((address >= 0xB000100) && (play_yan.firmware_addr >= 0xFF020) && (play_yan.capture_command_stream) && (play_yan.access_mode == 0x68))
	|| ((address >= 0xB000000) && (play_yan.firmware_addr >= 0xFF020) && (play_yan.capture_command_stream) && (play_yan.access_mode == 0x28)))
	{
		play_yan.command_stream.push_back(value);

		//Grab string of music/video filename to play
		if((value == 0x00) && ((play_yan.cmd == 0x201) || (play_yan.cmd == 0x500) || (play_yan.cmd == 0x600) || (play_yan.cmd == 0x700) || (play_yan.cmd == 0x800)))
		{
			std::string temp_str = "";
			u32 offset = (play_yan.cmd == 0x700) ? 9 : 1;
			if(play_yan.cmd == 0x500) { offset = 17; }

			//Don't process if not enough of the command stream for the filename is sent
			if(((play_yan.cmd == 0x201) || (play_yan.cmd == 0x600) || (play_yan.cmd == 0x800)) && (play_yan.command_stream.size() < 4)) { return; }
			if((play_yan.cmd == 0x700) && (play_yan.command_stream.size() < 12)) { return; }
			if((play_yan.cmd == 0x500) && (play_yan.command_stream.size() < 20)) { return; }

			play_yan.capture_command_stream = false;
			play_yan.command_stream.pop_back();

			for(u32 x = offset; x < play_yan.command_stream.size(); x++)
			{
				u8 chr = play_yan.command_stream[x];
				temp_str += chr;
			}

			//Set the current directory
			if(play_yan.cmd == 0x201)
			{
				//Backtrack one directory up
				if(temp_str == "..")
				{
					std::size_t last_dir = play_yan.current_dir.find_last_of("/");
					play_yan.current_dir = play_yan.current_dir.substr(0, last_dir);
				}

				//Jump into new directory
				else
				{
					play_yan.current_dir = play_yan.current_dir + "/" + temp_str;
				}
			}

			//Grab thumbnail index by searching for internal ID associated with video file
			else if(play_yan.cmd == 0x500)
			{
				//Look up .bmp thumbnail, same name as video file, different extension
				temp_str = util::get_filename_no_ext(temp_str) + ".bmp";

				//Convert backslash to forward slash
				for(u32 x = 0; x < temp_str.length(); x++)
				{
					if(temp_str[x] == 0x5C) { temp_str[x] = 0x2F; }
				}

				read_play_yan_thumbnail(temp_str);

				//Reset video length in ms
				//TODO - Use external conversion program and grab video length. For demo purposes, default to 10 seconds
				u32 vid_len = 10;

				play_yan.video_check_data[3][3] = (vid_len * 1000);
			}

			//Grab ID3 data for a song
			else if(play_yan.cmd == 0x600)
			{
				//Convert backslash to forward slash
				for(u32 x = 0; x < temp_str.length(); x++)
				{
					if(temp_str[x] == 0x5C) { temp_str[x] = 0x2F; }
				}

				for(u32 x = 0; x < play_yan.music_files.size(); x++)
				{
					if(util::get_filename_from_path(play_yan.music_files[x]) == temp_str)
					{
						play_yan.current_music_file = play_yan.music_files[x];
						temp_str = play_yan.music_files[x];
						break;
					}
				}

				//Grab ID3 data
				play_yan_get_id3_data(temp_str);
			}

			//Load and convert music file
			else if(play_yan.cmd == 0x800)
			{
				play_yan_load_audio(play_yan.current_music_file);
			}

			/*
			//Search for internal ID associated with video file
			else
			{
				for(u32 x = 0; x < play_yan.video_files.size(); x++)
				{
					if(temp_str == play_yan.video_files[x])
					{
						play_yan.video_file_index = x;
						play_yan.video_length = ((play_yan.video_times[x] + 1) * 0x20 * 30);
						play_yan.video_current_fps = 30.0 / play_yan.video_fps[x];
						break;
					}
				}
			}
			*/
		}
	}	
}

/****** Writes to Nintendo MP3 Player I/O ******/
void AGB_MMU::write_nmp(u32 address, u8 value)
{

}

/****** Reads from Play-Yan I/O ******/
u8 AGB_MMU::read_play_yan(u32 address)
{
	u8 result = memory_map[address];

	//Handle Nintendo MP3 Player writes separately
	if(play_yan.type == NINTENDO_MP3)
	{
		return read_nmp(address);
	}

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
		if(address == 0xB00031C)
		{
			//Most IRQ data is only read once, however, select commands (0x200, 0x700, and 0x800) will read it twice
			if(!play_yan.is_music_playing && !play_yan.is_video_playing)
			{
				play_yan.irq_data_in_use = false;
			}
		}

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
			u32 t_addr = play_yan.thumbnail_addr + offset;

			if(t_addr < 0x12C0)
			{
				result = play_yan.video_thumbnail[t_addr];

				//Update Play-Yan thubnail address if necessary
				if(offset == 0x1FE) { play_yan.thumbnail_addr += 0x200; }
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

/****** Reads from Play-Yan I/O ******/
u8 AGB_MMU::read_nmp(u32 address)
{
	u8 result = 0;
	return result;
}

/****** Handles Play-Yan command processing ******/
void AGB_MMU::process_play_yan_cmd()
{
	u32 prev_cmd = play_yan.cmd;
	play_yan.cmd = ((play_yan.cnt_data[3] << 24) | (play_yan.cnt_data[2] << 16) | (play_yan.cnt_data[1] << 8) | (play_yan.cnt_data[0]));

	std::cout<<"CMD -> 0x" << play_yan.cmd << "\n";

	//Trigger Game Pak IRQ for Get Filesystem Info command
	if(play_yan.cmd == 0x200)
	{
		play_yan.op_state = 1;
		play_yan.irq_delay = 1;
		play_yan.irq_count = 0;
		play_yan.delay_reload = 10;
		play_yan.irq_data_ptr = play_yan.folder_ops_data[0];
		play_yan.irq_len = 2;
	}

	//Trigger Game Pak IRQ for Change Current Directory command
	if(play_yan.cmd == 0x201)
	{
		play_yan.op_state = 1;
		play_yan.irq_delay = 1;
		play_yan.irq_count = 0;
		play_yan.delay_reload = 10;
		play_yan.irq_data_ptr = play_yan.folder_ops_data[2];
		play_yan.irq_len = 2;

		play_yan.capture_command_stream = true;
		play_yan.command_stream.clear();
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
		play_yan.thumbnail_addr = 0;

		play_yan.capture_command_stream = true;
		play_yan.command_stream.clear();
	}

	//Trigger Game Pak IRQ for ID3 data retrieval
	else if(play_yan.cmd == 0x600)
	{
		play_yan.op_state = 1;
		play_yan.irq_delay = 1;
		play_yan.delay_reload = 10;
		play_yan.irq_data_ptr = play_yan.music_play_data[0];
		play_yan.irq_len = 1;
		play_yan.irq_repeat = 0;
		play_yan.irq_count = 0;

		play_yan.capture_command_stream = true;
		play_yan.command_stream.clear();
	}

	//Trigger Game Pak IRQ for playing video
	else if(play_yan.cmd == 0x700)
	{
		play_yan.op_state = 9;
		play_yan.irq_delay = 1;
		play_yan.delay_reload = 10;
		play_yan.irq_data_ptr = play_yan.video_play_data[0];
		play_yan.irq_len = 2;
		play_yan.irq_repeat = 0;
		play_yan.irq_count = 0;
		play_yan.video_data_addr = 0;
		play_yan.video_progress = 0;
		play_yan.video_play_data[1][6] = play_yan.video_progress;
		play_yan.video_frame_count = 0;
		play_yan.is_video_playing = true;

		play_yan.tracker_progress = 0;
		play_yan.tracker_update_size = 0;

		play_yan.capture_command_stream = true;
		play_yan.command_stream.clear();
	}

	//Trigger Game Pak IRQ for stopping video
	else if(play_yan.cmd == 0x701)
	{
		play_yan.op_state = 1;
		play_yan.irq_delay = 1;
		play_yan.delay_reload = 10;
		play_yan.irq_data_ptr = play_yan.video_stop_data[0];
		play_yan.irq_len = 2;
		play_yan.irq_repeat = 0;
		play_yan.irq_count = 0;
		play_yan.video_data_addr = 0;
		play_yan.video_progress = 0;
		play_yan.video_frame_count = 0;
		play_yan.is_video_playing = false;
	}

	//Trigger Game Pak IRQ for playing music
	else if(play_yan.cmd == 0x800)
	{
		play_yan.op_state = 6;
		play_yan.irq_delay = 1;
		play_yan.delay_reload = 10;
		play_yan.irq_data_ptr = play_yan.music_play_data[0];
		play_yan.irq_len = 3;
		play_yan.irq_repeat = 0;
		play_yan.irq_count = 1;
		play_yan.is_music_playing = true;

		play_yan.tracker_progress = 0;
		play_yan.tracker_update_size = 0;
		play_yan.music_play_data[2][5] = play_yan.tracker_progress;
		play_yan.music_play_data[2][6] = 0;

		play_yan.capture_command_stream = true;
		play_yan.command_stream.clear();
	}

	//Trigger Game Pak IRQ for stopping music
	else if(play_yan.cmd == 0x801)
	{
		play_yan.op_state = 1;
		play_yan.irq_delay = 1;
		play_yan.delay_reload = 10;
		play_yan.irq_data_ptr = play_yan.music_stop_data[0];
		play_yan.irq_len = 2;
		play_yan.irq_repeat = 0;
		play_yan.irq_count = 0;
		play_yan.is_music_playing = false;
	}

	//Pause Media Playback
	else if(play_yan.cmd == 0x902)
	{
		play_yan.pause_media = true;
	}

	//Resume Media Playback
	else if(play_yan.cmd == 0x904)
	{
		play_yan.pause_media = false;
	}

	//Trigger Game Pak IRQ for cartridge status
	else if(play_yan.cmd == 0x8000)
	{
		play_yan.irq_delay = 60;
		play_yan.irq_data_ptr = play_yan.sd_check_data[1];
		play_yan.irq_len = 1;
		play_yan.irq_repeat = 0;
		play_yan.irq_count = 0;
	}

	//Trigger Game Pak IRQ for booting cartridge/status
	else if(play_yan.cmd == 0x800000)
	{
		play_yan.op_state = 1;
		play_yan.irq_delay = 60;
		play_yan.irq_data_ptr = play_yan.sd_check_data[2];
		play_yan.irq_len = 2;
		play_yan.irq_repeat = 0;
		play_yan.irq_count = 0;
	}

	//Trigger Play-Yan Micro Game Pak IRQ to open .ini file 
	else if(play_yan.cmd == 0x3000)
	{
		play_yan.op_state = 1;
		play_yan.irq_delay = 60;
		play_yan.irq_data_ptr = play_yan.micro[0];
		play_yan.irq_len = 1;
		play_yan.irq_repeat = 0;
		play_yan.irq_count = 0;

		//0x3000 command is unique to Play-Yan Micro. Switch type if command detected
		play_yan.type = PLAY_YAN_MICRO;
		std::cout<<"MMU::Play-Yan Micro detected\n";
	}

	//Trigger Play-Yan Micro Game Pak IRQ to read .ini file 
	else if(play_yan.cmd == 0x3001)
	{
		play_yan.op_state = 1;
		play_yan.irq_delay = 60;
		play_yan.irq_data_ptr = play_yan.micro[0];
		play_yan.irq_len = 2;
		play_yan.irq_repeat = 0;
		play_yan.irq_count = 1;

		play_yan_set_ini_file();
	}	

	//Trigger Play-Yan Micro Game Pak IRQ to close .ini file??? 
	else if(play_yan.cmd == 0x3003)
	{
		play_yan.op_state = 1;
		play_yan.irq_delay = 60;
		play_yan.irq_data_ptr = play_yan.micro[0];
		play_yan.irq_len = 3;
		play_yan.irq_repeat = 0;
		play_yan.irq_count = 2;
	}		
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

	if(play_yan.op_state == 0xFF) { return; }

	//Process Play-Yan states
	//1 = SD card check, 2 = Enter/exit music menu, 3 = Enter/exit video menu,
	//5 = Process video thumbnails, 6 = Play music, 7 = Continue playing music
	//8 = Stop music, 9 = Play video, 10 = Stop video
	//TODO - Use an enum instead of magic numbers!

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
			play_yan.irq_repeat = play_yan.music_length + 1;
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
			play_yan.irq_repeat = (play_yan.video_length / 0x20);
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

			if(!play_yan.pause_media)
			{
				play_yan.irq_repeat--;

				//Update trackbar
				play_yan.music_play_data[2][5] += play_yan.tracker_update_size;

				//Update music progress via IRQ data
				play_yan.music_play_data[2][6]++;

				if(play_yan.music_play_data[2][6] > play_yan.music_length)
				{
					play_yan.op_state = 1;
					play_yan.irq_delay = 1;
					play_yan.delay_reload = 1;
					play_yan.irq_data_ptr = play_yan.music_stop_data[0];
					play_yan.irq_count = 0;
					play_yan.irq_len = 2;
					play_yan.irq_repeat = 0;
					play_yan.is_music_playing = false;
				}
			}
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

			if(!play_yan.pause_media)
			{
				play_yan.irq_repeat--;

				//Update video progress via IRQ data
				play_yan.video_progress += 0x20;
				play_yan.video_play_data[1][6] = play_yan.video_progress;

				//Update video frame counter and grab new video frame if necessary
				play_yan.video_frame_count += 1.0;

				if(play_yan.video_frame_count >= play_yan.video_current_fps)
				{
					play_yan.video_frame_count -= play_yan.video_current_fps;
					//TODO - Grab new frame data
				}

				//Stop video when length is complete
				if(play_yan.video_progress >= play_yan.video_length)
				{
					play_yan.op_state = 1;
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

		//std::cout<<"IRQ -> 0x" << play_yan.irq_data[0] << "\n";

		play_yan.irq_count++;
		play_yan.irq_delay = play_yan.delay_reload;
		play_yan.irq_data_in_use = true;
	}
}

/****** Reads a bitmap file for video thumbnail used for Play-Yan video ******/
bool AGB_MMU::read_play_yan_thumbnail(std::string filename)
{
	//Grab pixel data of file as a BMP
	SDL_Surface* source = SDL_LoadBMP(filename.c_str());

	//Generate blank (all black) thumbnail if source not found
	if(source == NULL)
	{
		std::cout<<"MMU::Warning - Could not load thumbnail image for " << filename << "\n";
		play_yan.video_thumbnail.resize(0x12C0, 0x00);
	}

	//Otherwise grab pixel data from source
	else
	{
		play_yan.video_thumbnail.clear();
		u8* pixel_data = (u8*)source->pixels;

		//Convert 32-bit pixel data to RGB15 and push to vector
		for(int a = 0, b = 0; a < (source->w * source->h); a++, b+=3)
		{
			u16 raw_pixel = ((pixel_data[b] & 0xF8) << 7) | ((pixel_data[b+1] & 0xF8) << 2) | ((pixel_data[b+2] & 0xF8) >> 3);
			play_yan.video_thumbnail.push_back(raw_pixel & 0xFF);
			play_yan.video_thumbnail.push_back((raw_pixel >> 8) & 0xFF);
		}
	}

	return true;
}

/****** Sets the current SD card data for a given music file via index ******/
void AGB_MMU::play_yan_set_music_file()
{
	play_yan.card_data.clear();
	play_yan.card_data.resize(0x10000, 0x00);
	play_yan.music_files.clear();

	u32 entry_size = 0;

	if(play_yan.type == PLAY_YAN_OG) { entry_size = 268; }
	else if(play_yan.type == PLAY_YAN_MICRO) { entry_size = 272; }
	else { return; }

	std::vector<std::string> dir_listing;

	//Grab files, then folders, in current directory and add them to SD card data
	util::get_files_in_dir(play_yan.current_dir, ".mp3", dir_listing, false, true);
	u32 file_count = dir_listing.size();
	util::get_folders_in_dir(play_yan.current_dir, dir_listing);

	//Write filenames and folder names to SD card data
	for(u32 x = 0; x < dir_listing.size(); x++)
	{
		//Set filetype meta data for Play-Yan
		if(x < file_count)
		{
			play_yan.card_data[4 + ((x + 1) * entry_size)] = 2;
		}

		//Copy filename
		play_yan.music_files.push_back(dir_listing[x]);
		std::string sd_file = util::get_filename_from_path(dir_listing[x]);

		for(u32 y = 0; y < sd_file.length(); y++)
		{
			u8 chr = sd_file[y];

			//Convert forward slash to backslash when writing SD card data
			if(chr == 0x2F) { chr = 0x5C; }
			play_yan.card_data[8 + (x * entry_size) + y] = chr;
		}
	}

	//Set the total number of files
	play_yan.card_data[4] = dir_listing.size();
}

/****** Sets the current SD card data for a given video file via index ******/
void AGB_MMU::play_yan_set_video_file()
{
	play_yan.card_data.clear();
	play_yan.card_data.resize(0x10000, 0x00);
	play_yan.video_files.clear();

	u32 entry_size = 0;

	if(play_yan.type == PLAY_YAN_OG) { entry_size = 268; }
	else if(play_yan.type == PLAY_YAN_MICRO) { entry_size = 272; }
	else { return; }

	std::vector<std::string> dir_listing;

	//Grab files, then folders, in current directory and add them to SD card data
	util::get_files_in_dir(play_yan.current_dir, ".asf", dir_listing, true, true);

	//Write filenames and folder names to SD card data
	for(u32 x = 0; x < dir_listing.size(); x++)
	{
		//Set filetype meta data for Play-Yan
		play_yan.card_data[4 + ((x + 1) * entry_size)] = 1;

		//Copy filename
		play_yan.video_files.push_back(dir_listing[x]);
		std::string sd_file = dir_listing[x];

		for(u32 y = 0; y < sd_file.length(); y++)
		{
			u8 chr = sd_file[y];

			//Convert forward slash to backslash when writing SD card data
			if(chr == 0x2F) { chr = 0x5C; }
			play_yan.card_data[8 + (x * entry_size) + y] = chr;
		}
	}

	//Set the total number of files
	play_yan.card_data[4] = dir_listing.size();
}

/****** Sets the current SD card data for a given folder ******/
void AGB_MMU::play_yan_set_folder()
{
	play_yan.card_data.clear();
	play_yan.card_data.resize(0x10000, 0x00);

	u32 entry_size = 0;

	if(play_yan.type == PLAY_YAN_OG) { entry_size = 268; }
	else if(play_yan.type == PLAY_YAN_MICRO) { entry_size = 272; }
	else { return; }

	std::vector<std::string> dir_listing;

	//Grab files, then folders, in current directory and add them to SD card data
	util::get_files_in_dir(play_yan.current_dir, "", dir_listing, false, false);
	u32 file_count = dir_listing.size();
	util::get_folders_in_dir(play_yan.current_dir, dir_listing);

	//Write filenames and folder names to SD card data
	for(u32 x = 0; x < dir_listing.size(); x++)
	{
		//Set filetype meta data for Play-Yan
		if(x < file_count)
		{
			play_yan.card_data[4 + ((x + 1) * entry_size)] = 2;
		}

		//Copy filename
		std::string sd_file = dir_listing[x];

		for(u32 y = 0; y < sd_file.length(); y++)
		{
			u8 chr = sd_file[y];
			play_yan.card_data[8 + (x * entry_size) + y] = chr;
		}
	}

	//Set the total number of files
	play_yan.card_data[4] = dir_listing.size();
}

/****** Gets the current ID3 data (Title + Artist) for a given MP3 file ******/
void AGB_MMU::play_yan_get_id3_data(std::string filename)
{
	play_yan.card_data.clear();
	play_yan.card_data.resize(0x10000, 0x00);

	std::ifstream file(filename.c_str(), std::ios::binary);
	std::vector<u8> mp3_data;

	if(!file.is_open()) 
	{
		std::cout<<"MMU::" << filename << " could not be opened. Check file path or permissions. \n";
		return;
	}

	//Get the file size
	file.seekg(0, file.end);
	u32 file_size = file.tellg();
	file.seekg(0, file.beg);
	mp3_data.resize(file_size);

	//Read entire MP3 file
	file.read(reinterpret_cast<char*> (&mp3_data[0]), file_size);
	file.close();

	std::string title = "";
	std::string artist = "";

	//Check for ID3v1 or ID3v2
	bool is_id3v1 = false;
	u32 tag_v1_pos = file_size - 128;

	if((file_size >= 128) && (mp3_data[tag_v1_pos] == 0x54) && (mp3_data[tag_v1_pos + 1] == 0x41) && (mp3_data[tag_v1_pos + 2] == 0x47))
	{
		is_id3v1 = true;
	}

	if(is_id3v1)
	{
		tag_v1_pos += 3;

		//Grab song title - ID3v1
		//Grab artist - ID3v1
		for(u32 x = 0; x < 30; x++) { title += mp3_data[tag_v1_pos++]; }
		for(u32 x = 0; x < 30; x++) { artist += mp3_data[tag_v1_pos++]; }

		//Prune extra spaces or null characters from ID3v1 song and artist fields
		for(u32 x = 29; x > 0; x--)
		{
			if((title[x] != 0x00) && (title[x] != 0x20)) { title = title.substr(0, (x + 1)); break; }
		}

		for(u32 x = 29; x > 0; x--)
		{
			if((artist[x] != 0x00) && (artist[x] != 0x20)) { artist = artist.substr(0, (x + 1)); break; }
		}
	}

	else
	{
		//Grab song title - ID3v2
		for(u32 x = 0; x < (file_size - 4); x++)
		{
			if((mp3_data[x] == 0x54) && (mp3_data[x + 1] == 0x49) && (mp3_data[x + 2] == 0x54) && (mp3_data[x + 3] == 0x32))
			{
				x += 4;

				if((x + 4) < file_size)
				{
					u32 title_len = (mp3_data[x] << 24) | (mp3_data[x + 1] << 16) | (mp3_data[x + 2] << 8) | mp3_data[x + 3];
					x += 7;

					if((x + title_len) < file_size)
					{
						for(u32 y = 1; y < title_len; y++, x++)
						{
							if(mp3_data[x] > 0x1F) { title += mp3_data[x]; }
						}

						break;
					}
				}
			}
		}

		//Grab artist - ID3v2
		for(u32 x = 0; x < (file_size - 4); x++)
		{
			if((mp3_data[x] == 0x54) && (mp3_data[x + 1] == 0x50) && (mp3_data[x + 2] == 0x45) && (mp3_data[x + 3] == 0x31))
			{
				x += 4;

				if((x + 4) < file_size)
				{
					u32 artist_len = (mp3_data[x] << 24) | (mp3_data[x + 1] << 16) | (mp3_data[x + 2] << 8) | mp3_data[x + 3];
					x += 7;

					if((x + artist_len) < file_size)
					{
						for(u32 y = 1; y < artist_len; y++, x++)
						{
							if(mp3_data[x] > 0x1F) { artist += mp3_data[x]; }
						}

						break;
					}
				}
			}
		}
	}

	//Write song title and artist to SD card data
	u32 t_len = (title.length() > 0x40) ? 0x40 : title.length();
	u32 a_len = (artist.length() > 0x46) ? 0x46 : artist.length();

	for(u32 x = 0; x < t_len; x++)
	{
		u8 data = title[x];
		play_yan.card_data[0x04 + x] = data;
	}

	for(u32 x = 0; x < a_len; x++)
	{
		u8 data = artist[x];
		play_yan.card_data[0x45 + x] = data;
	}
} 

/****** Sets SD card data to read the Play-Yan Micro .ini file ******/
void AGB_MMU::play_yan_set_ini_file()
{
	play_yan.card_data.clear();
	play_yan.card_data.resize(0x10000, 0x00);

	std::string filename = config::data_path + "play_yan/play_yanmicro.ini";
	std::ifstream file(filename.c_str(), std::ios::binary);
	std::vector<u8> ini_data;

	if(!file.is_open()) 
	{
		std::cout<<"MMU::" << filename << " Play-Yan Micro .ini file could not be opened. Check file path or permissions. \n";
		return;
	}

	//Get the file size
	file.seekg(0, file.end);
	u32 file_size = file.tellg();
	file.seekg(0, file.beg);
	ini_data.resize(file_size);
	
	if(file_size > 0x10000) { file_size = 0x10000; }
	
	//Set the parameter for Game Pak IRQ data
	play_yan.micro[1][2] = file_size;

	//Read data from file and copy to SD card data
	file.read(reinterpret_cast<char*> (&ini_data[0]), file_size);
	for(u32 x = 0; x < file_size; x++) { play_yan.card_data[x] = ini_data[x]; }
	file.close();

	std::cout<<"MMU::Reading Play-Yan Micro.ini file at " << filename << "\n";
}

/****** Wakes Play-Yan from GBA sleep mode - Fires Game Pak IRQ ******/
void AGB_MMU::play_yan_wake()
{
	play_yan.op_state = 2;
	play_yan.irq_delay = 60;
	play_yan.delay_reload = 10;
	play_yan.irq_data_ptr = play_yan.wake_data;
	play_yan.irq_len = 1;
}

/****** Loads audio (.MP3) file and then converts it to .WAV for playback ******/
bool AGB_MMU::play_yan_load_audio(std::string filename)
{
	//Abort now if no audio conversion command is specified
	if(config::audio_conversion_cmd.empty())
	{
		std::cout<<"MMU::No audio conversion command specified. Cannot convert file " << filename << "\n";
		return false;
	}

	//Clear previous buffer if necessary
	SDL_FreeWAV(apu_stat->ext_audio.buffer);
	apu_stat->ext_audio.buffer = NULL;

	SDL_AudioSpec file_spec;

	std::string out_file = config::temp_media_file + ".wav";
	std::string sys_cmd = config::audio_conversion_cmd;

	//Delete any existing temporary media file in case audio conversion command complains
	std::remove(out_file.c_str());

	//Replace %in and %out with proper parameters
	std::string search_str = "%in";
	std::size_t pos = sys_cmd.find(search_str);

	if(pos != std::string::npos)
	{
		std::string start = sys_cmd.substr(0, pos);
		std::string end = sys_cmd.substr(pos + 3);
		sys_cmd = start + filename + end;
	}

	search_str = "%out";
	pos = sys_cmd.find(search_str);

	if(pos != std::string::npos)
	{
		std::string start = sys_cmd.substr(0, pos);
		std::string end = sys_cmd.substr(pos + 4);
		sys_cmd = start + out_file + end;
	}
		
	//Check for a command processor on system and run audio conversion command
	if(system(NULL))
	{
		std::cout<<"MMU::Converting audio file " << filename << "\n";
		system(sys_cmd.c_str());
		std::cout<<"MMU::Conversion complete\n";
	}

	else
	{
		std::cout<<"Conversion of audio file " << filename << " failed \n";
		return false;
	}

	if(SDL_LoadWAV(out_file.c_str(), &file_spec, &apu_stat->ext_audio.buffer, &apu_stat->ext_audio.length) == NULL)
	{
		std::cout<<"MMU::Play-Yan could not load audio file: " << out_file << " :: " << SDL_GetError() << "\n";
		return false;
	}

	//Check format, must be S16 audio, LSB
	if(file_spec.format != AUDIO_S16)
	{
		std::cout<<"MMU::Jukebox loaded file, but format is not Signed 16-bit LSB audio\n";
		return false;
	}

	//Check number of channels, max is 2
	if(file_spec.channels > 2)
	{
		std::cout<<"MMU::Jukebox loaded file, but audio uses more than 2 channels\n";
		return false;
	}

	apu_stat->ext_audio.frequency = file_spec.freq;
	apu_stat->ext_audio.channels = file_spec.channels;

	//Determine song length in seconds by the amount of samples
	u32 song_samples = apu_stat->ext_audio.length / 2;
	if(file_spec.channels == 2) { song_samples /= 2; }

	u32 song_seconds = song_samples / apu_stat->ext_audio.frequency;
	if(song_samples % (u32)apu_stat->ext_audio.frequency) { song_seconds += 1; }
	
	play_yan.music_length = song_seconds;
	play_yan.tracker_update_size = (0x6400 / play_yan.music_length);

	std::cout<<"MMU::Play-Yan loaded audio file: " << filename << "\n";

	return true;
}