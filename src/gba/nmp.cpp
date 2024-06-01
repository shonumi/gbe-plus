// GB Enhanced+ Copyright Daniel Baxter 2023
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : nmp.cpp
// Date : October 27, 2023
// Description : Nintendo MP3 Player
//
// Handles I/O for the Nintendo MP3 Player
// Manages IRQs and firmware reads/writes
// Play-Yan and Play-Yan Micro handled separately (see play_yan.cpp)

#include "mmu.h"
#include "common/util.h" 

/****** Writes to Nintendo MP3 Player I/O ******/
void AGB_MMU::write_nmp(u32 address, u8 value)
{
	//std::cout<<"PLAY-YAN WRITE -> 0x" << address << " :: 0x" << (u32)value << "\n";

	switch(address)
	{
		//Device Control
		case PY_NMP_CNT:
			play_yan.access_mode &= ~0xFF00;
			play_yan.access_mode |= (value << 8);
			break;

		//Device Control
		case PY_NMP_CNT+1:
			play_yan.access_mode &= ~0xFF;
			play_yan.access_mode |= value;

			//After firmware is loaded, the Nintendo MP3 Player generates a Game Pak IRQ.
			//Confirm firmware is finished with this write after booting.
			if((play_yan.access_mode == 0x0808) && (play_yan.op_state == PLAY_YAN_INIT))
			{
				play_yan.irq_delay = 30;
				play_yan.op_state = PLAY_YAN_BOOT_SEQUENCE;
			}

			//Terminate command input now. Actual execution happens later.
			//Command is always first 16-bits of stream
			else if((play_yan.access_mode == 0x0404) && (play_yan.op_state == PLAY_YAN_PROCESS_CMD))
			{
				if(play_yan.command_stream.size() >= 2)
				{
					play_yan.cmd = (play_yan.command_stream[0] << 8) | play_yan.command_stream[1];
					process_nmp_cmd();
				}
			}

			break;

		//Device Parameter
		case PY_NMP_PARAMETER:
			play_yan.access_param &= ~0xFF00;
			play_yan.access_param |= (value << 8);
			break;

		//Device Parameter
		case PY_NMP_PARAMETER+1:
			play_yan.access_param &= ~0xFF;
			play_yan.access_param |= value;

			//Set high 16-bits of the param or begin processing commands now
			if(play_yan.access_mode == 0x1010) { play_yan.access_param <<= 16; }
			else if(play_yan.access_mode == 0) { access_nmp_io(); }

			break;

		//Device Data Input (firmware, commands, etc?)
		case PY_NMP_DATA_IN:
		case PY_NMP_DATA_IN+1:
			if(play_yan.firmware_addr)
			{
				play_yan.firmware[play_yan.firmware_addr++] = value;
			}

			else if(play_yan.op_state == PLAY_YAN_PROCESS_CMD)
			{
				play_yan.command_stream.push_back(value);
			}

			break;
	}	
}

/****** Reads from Nintendo MP3 Player I/O ******/
u8 AGB_MMU::read_nmp(u32 address)
{
	u8 result = 0;

	switch(address)
	{
		case PY_NMP_DATA_OUT:
		case PY_NMP_DATA_OUT+1:

			//Return SD card data
			if(play_yan.op_state == PLAY_YAN_GET_SD_DATA)
			{
				if(play_yan.nmp_data_index < play_yan.card_data.size())
				{
					result = play_yan.card_data[play_yan.nmp_data_index++];
				}
			}

			//Some kind of status data read after each Game Pak IRQ
			else if(play_yan.nmp_data_index < 16)
			{
				result = play_yan.nmp_status_data[play_yan.nmp_data_index++];
			}

			//Manually trigger secondary 0x8100 IRQs - 1st = Process Audio Samples, 2nd = Update Timestamp 
			if(play_yan.nmp_manual_irq)
			{
				play_yan.nmp_read_count++;
				u32 limit = (play_yan.audio_buffer_size / 4);

				if(play_yan.nmp_read_count == limit)
				{
					play_yan.nmp_read_count = 0;
					play_yan.nmp_manual_irq = false;
					play_yan.update_audio_stream = false;
					play_yan.update_trackbar_timestamp = true;

					play_yan.nmp_manual_cmd = 0x8100;
					play_yan.irq_delay = 0;
					process_play_yan_irq();
					play_yan.op_state = PLAY_YAN_PROCESS_CMD;
				}
			}

			break;
	}

	//std::cout<<"PLAY-YAN READ -> 0x" << address << " :: 0x" << (u32)result << "\n";

	return result;
}

/****** Handles Nintendo MP3 Player command processing ******/
void AGB_MMU::process_nmp_cmd()
{
	std::cout<<"CMD -> 0x" << play_yan.cmd << "\n";

	//Set up default status data
	for(u32 x = 0; x < 16; x++) { play_yan.nmp_status_data[x] = 0; }
	play_yan.nmp_status_data[0] = (play_yan.cmd >> 8);
	play_yan.nmp_status_data[1] = (play_yan.cmd & 0xFF);

	switch(play_yan.cmd)
	{
		//Start list of files and folders
		case 0x10:
			play_yan.nmp_cmd_status = 0x4010;
			play_yan.nmp_valid_command = true;

			play_yan.nmp_status_data[2] = 0;
			play_yan.nmp_status_data[3] = 0;

			play_yan.nmp_entry_count = 0;
			play_yan.music_files.clear();
			play_yan.folders.clear();

			//Grab all files, then folders
			util::get_files_in_dir(play_yan.current_dir, ".mp3", play_yan.music_files, false, false);
			util::get_folders_in_dir(play_yan.current_dir, play_yan.folders);

			//Stop list if done
			if(play_yan.nmp_entry_count >= (play_yan.music_files.size() + play_yan.folders.size()))
			{
				play_yan.nmp_status_data[2] = 0;
				play_yan.nmp_status_data[3] = 1;	
			}

			play_yan.nmp_entry_count++;

			break;

		//Continue list of files and folders
		case 0x11:
			play_yan.nmp_cmd_status = 0x4011;
			play_yan.nmp_valid_command = true;

			//Stop list if done
			if(play_yan.nmp_entry_count >= (play_yan.music_files.size() + play_yan.folders.size()))
			{
				play_yan.nmp_status_data[2] = 0;
				play_yan.nmp_status_data[3] = 1;
			}

			play_yan.nmp_entry_count++;

			break;

		//Change directory
		case 0x20:
			play_yan.nmp_cmd_status = 0x4020;
			play_yan.nmp_valid_command = true;

			{
				std::string new_dir = "";

				//Grab directory
				for(u32 x = 3; x < play_yan.command_stream.size(); x += 2)
				{
					u8 chr = play_yan.command_stream[x];
					if(!chr) { break; }

					new_dir += chr;
				}

				//Move one directory up
				if(new_dir == "..")
				{
					u8 chr = play_yan.current_dir.back();

					while(chr != 0x2F)
					{
						play_yan.current_dir.pop_back();
						chr = play_yan.current_dir.back();
					}

					if(chr == 0x2F) { play_yan.current_dir.pop_back(); }
				}

				//Jump down into new directory
				else if(!new_dir.empty())
				{
					play_yan.current_dir += ("/" + new_dir);
				}
			}
				
			break;

		//Get ID3 Tags
		case 0x40:
			play_yan.nmp_cmd_status = 0x4040;
			play_yan.nmp_valid_command = true;

			//Get music file
			play_yan.current_music_file = "";

			for(u32 x = 3; x < play_yan.command_stream.size(); x += 2)
			{
				u8 chr = play_yan.command_stream[x];
				if(!chr) { break; }

				play_yan.current_music_file += chr;
			}

			//This first time around, this command returns an arbitrary 16-bit value in status data
			//This indicates the 16-bit access index ID3 data can be read from
			//Here, GBE+ forces 0x0101, since the NMP uses that for subsequent ID3 reads anyway
			play_yan.nmp_status_data[6] = 0x1;
			play_yan.nmp_status_data[7] = 0x1;

			play_yan_get_id3_data(play_yan.current_dir + "/" + play_yan.current_music_file);
			play_yan.nmp_title = util::make_ascii_printable(play_yan.nmp_title);
			play_yan.nmp_artist = util::make_ascii_printable(play_yan.nmp_artist);

			break;

		//Play Music File
		case 0x50:
			play_yan.nmp_cmd_status = 0x4050;
			play_yan.nmp_valid_command = true;
			play_yan.is_music_playing = true;

			if(apu_stat->ext_audio.use_headphones)
			{
				play_yan.update_audio_stream = false;
				play_yan.update_trackbar_timestamp = true;
			}
			
			else
			{
				play_yan.update_audio_stream = true;
				play_yan.update_trackbar_timestamp = false;
			}

			//Get music file
			play_yan.current_music_file = "";

			for(u32 x = 3; x < play_yan.command_stream.size(); x += 2)
			{
				u8 chr = play_yan.command_stream[x];
				if(!chr) { break; }

				play_yan.current_music_file += chr;
			}

			if(!play_yan_load_audio(play_yan.current_dir + "/" + play_yan.current_music_file))
			{
				//If no audio could be loaded, use dummy length for song
				play_yan.music_length = 2;
			}

			//Trigger additional IRQs for processing music
			play_yan.nmp_manual_cmd = 0x8100;
			play_yan.irq_delay = 10;
			play_yan.irq_count = 0;

			break;

		//Stop Music Playback
		case 0x51:
			play_yan.nmp_cmd_status = 0x4051;
			play_yan.nmp_valid_command = true;
			play_yan.is_music_playing = false;
			apu_stat->ext_audio.playing = false;

			play_yan.audio_frame_count = 0;
			play_yan.tracker_update_size = 0;
			
			play_yan.update_audio_stream = false;
			play_yan.update_trackbar_timestamp = false;

			play_yan.nmp_manual_cmd = 0;
			play_yan.irq_delay = 0;

			break;

		//Pause Music Playback
		case 0x52:
			play_yan.nmp_cmd_status = 0x4052;
			play_yan.nmp_valid_command = true;
			play_yan.is_music_playing = false;
			apu_stat->ext_audio.playing = false;

			play_yan.nmp_manual_cmd = 0;
			play_yan.irq_delay = 0;

			break;

		//Resume Music Playback
		case 0x53:
			play_yan.nmp_cmd_status = 0x4053;
			play_yan.nmp_valid_command = true;
			play_yan.is_music_playing = true;

			if(play_yan.audio_sample_rate && play_yan.audio_channels)
			{
				apu_stat->ext_audio.playing = true;
			}

			play_yan.nmp_manual_cmd = 0x8100;
			play_yan.irq_delay = 1;
			play_yan.irq_count = 0;

			break;

		//Adjust Volume - No IRQ generated
		case 0x80:
			if(play_yan.command_stream.size() >= 4)
			{
				play_yan.volume = play_yan.command_stream[3];
				apu_stat->ext_audio.volume = (play_yan.volume / 46.0) * 63.0;
			}

			break;

		//Generate Sound (for menus) - No IRQ generated
		case 0x200:
			break;

		//Check for firmware update file
		case 0x300:
			play_yan.nmp_cmd_status = 0x4300;
			play_yan.nmp_valid_command = true;
			
			break;

		//Undocumented command (firmware update related?)
		case 0x301:
			play_yan.nmp_cmd_status = 0x4301;
			play_yan.nmp_valid_command = true;
			
			break;

		//Undocumented command (firmware update related?)
		case 0x303:
			play_yan.nmp_cmd_status = 0x4303;
			play_yan.nmp_valid_command = true;
			play_yan.cmd = 0;
			
			break;

		//Sleep Start
		case 0x500:
			play_yan.nmp_cmd_status = 0x8500;
			play_yan.nmp_valid_command = true;

			break;

		//Sleep End
		case 0x501:
			play_yan.nmp_cmd_status = 0x8501;
			play_yan.nmp_valid_command = true;

			break;

		//Init NMP Hardware
		case 0x8001:
			play_yan.nmp_cmd_status = 0x8001;
			play_yan.nmp_valid_command = true;
			
			break;

		//Continue music stream
		case 0x8100:
			play_yan.nmp_cmd_status = 0x8100;
			play_yan.nmp_valid_command = false;

			//Trigger additional IRQs for processing music
			if(play_yan.is_music_playing)
			{
				play_yan.nmp_manual_cmd = 0x8100;
				play_yan.irq_delay = 1;

				play_yan.audio_buffer_size = 32;

				//Prioritize audio stream updates
				if(play_yan.update_audio_stream && !apu_stat->ext_audio.use_headphones)
				{
					//Audio buffer size (max 0x480), *MUST* be a multiple of 16!
					play_yan.nmp_status_data[2] = (play_yan.audio_buffer_size >> 8);
					play_yan.nmp_status_data[3] = (play_yan.audio_buffer_size & 0xFF);

					//SD Card access ID - Seems arbitrary, so forced to 0x0202 here
					play_yan.nmp_status_data[4] = 0x02;
					play_yan.nmp_status_data[5] = 0x02;

					play_yan.nmp_audio_index = 0x202 + (play_yan.audio_buffer_size / 4);
					play_yan.audio_frame_count++;

					if(play_yan.audio_frame_count == 60)
					{
						play_yan.audio_frame_count = 0;
						play_yan.update_trackbar_timestamp = true;
						play_yan.tracker_update_size++;
					}
				}

				else if(play_yan.update_trackbar_timestamp)
				{
					//Trackbar position - 0 to 99

					if(play_yan.music_length - 1)
					{
						float progress = play_yan.tracker_update_size;
						progress /= play_yan.music_length - 1;
						progress *= 100.0;

						play_yan.nmp_status_data[8] = u8(progress);

						if(progress >= 100)
						{
							play_yan.nmp_manual_cmd = 0x51;
							play_yan.irq_delay = 1;
						}
					}

					//Song timestamp in seconds
					//Treated here as a 24-bit MSB value, with bytes 15, 12, and 13 (in that order)
					play_yan.nmp_status_data[15] = (play_yan.tracker_update_size >> 16) & 0xFF;
					play_yan.nmp_status_data[12] = (play_yan.tracker_update_size >> 8) & 0xFF;
					play_yan.nmp_status_data[13] = (play_yan.tracker_update_size & 0xFF);

					if(apu_stat->ext_audio.use_headphones)
					{
						play_yan.irq_delay = 60;
						play_yan.tracker_update_size++;
					}

					else
					{
						play_yan.update_audio_stream = true;
						play_yan.update_trackbar_timestamp = false;
					}
				}

				//Start external audio output
				if(!apu_stat->ext_audio.playing && play_yan.audio_sample_rate && play_yan.audio_channels)
				{
					apu_stat->ext_audio.channels = play_yan.audio_channels;
					apu_stat->ext_audio.frequency = play_yan.audio_sample_rate;
					apu_stat->ext_audio.sample_pos = 0;
					apu_stat->ext_audio.playing = true;
				}
			}

			break;

		//Headphone Status
		case 0x8600:
			play_yan.nmp_cmd_status = 0x8600;
			play_yan.nmp_valid_command = true;

			if(apu_stat->ext_audio.use_headphones)
			{
				play_yan.nmp_status_data[2] = 0;
				play_yan.nmp_status_data[3] = 1;
			}

			break;

		default:
			play_yan.nmp_valid_command = false;
			play_yan.nmp_cmd_status = 0;
			std::cout<<"Unknown Nintendo MP3 Player Command -> 0x" << play_yan.cmd << "\n";
	}
}

/****** Handles prep work for accessing Nintendo MP3 Player I/O such as writing commands, cart status, busy signal etc ******/
void AGB_MMU::access_nmp_io()
{
	play_yan.firmware_addr = 0;

	//Determine which kinds of data to access (e.g. cart status, hardware busy flag, command stuff, etc)
	if((play_yan.access_param) && (play_yan.access_param != 0x101) && (play_yan.access_param != 0x202) && (play_yan.access_param != play_yan.nmp_audio_index))
	{
		//std::cout<<"ACCESS -> 0x" << play_yan.access_param << "\n";
		play_yan.firmware_addr = (play_yan.access_param << 1);

		u16 stat_data = 0;

		//Cartridge Status
		if(play_yan.access_param == 0x100)
		{
			//Cartridge status during initial boot phase (e.g. Health and Safety screen)
			if(play_yan.nmp_init_stage < 4)
			{
				stat_data = play_yan.nmp_boot_data[play_yan.nmp_init_stage >> 1];
				play_yan.nmp_init_stage++;

				if(play_yan.nmp_init_stage == 2) { memory_map[REG_IF+1] |= 0x20; }
			}

			//Status after running a command
			else if(play_yan.nmp_cmd_status)
			{
				stat_data = play_yan.nmp_cmd_status;
			}
		}

		//Write command or wait for command to finish
		else if(play_yan.access_param == 0x10F)
		{
			play_yan.op_state = PLAY_YAN_PROCESS_CMD;
			play_yan.firmware_addr = 0;
			play_yan.command_stream.clear();

			//Finish command
			if(play_yan.nmp_valid_command)
			{
				memory_map[REG_IF+1] |= 0x20;
				play_yan.nmp_valid_command = false;
			}

			//Increment internal ticks
			//Value here is 6 ticks, a rough average of how often a real NMP updates at ~60Hz
			play_yan.nmp_ticks += 6;
			stat_data = play_yan.nmp_ticks;
		}

		//I/O Busy Flag
		//Signals the end of a command
		else if(play_yan.access_param == 0x110)
		{
			//1 = I/O Busy, 0 = I/O Ready. For now, we are never busy
			play_yan.op_state = PLAY_YAN_WAIT;
		}

		play_yan.nmp_status_data[0] = (stat_data >> 8);
		play_yan.nmp_status_data[1] = (stat_data & 0xFF);
		play_yan.nmp_data_index = 0;
		play_yan.access_param = 0;
	}

	//Access SD card data
	else
	{
		play_yan.card_data.clear();
		play_yan.op_state = PLAY_YAN_GET_SD_DATA;
		play_yan.nmp_data_index = 0;

		switch(play_yan.cmd)
		{
			//File and folder list
			case 0x10:
			case 0x11:
				play_yan.card_data.resize(528, 0x00);

				if(play_yan.nmp_entry_count)
				{
					std::string list_entry = "";
					bool is_folder = false;
					u32 file_limit = play_yan.music_files.size();
					u32 real_entry = play_yan.nmp_entry_count - 1;
					
					if(real_entry < file_limit)
					{
						list_entry = play_yan.music_files[real_entry];
					}

					else
					{
						real_entry -= file_limit;
						list_entry = play_yan.folders[real_entry];
						is_folder = true;
					}

					u32 str_len = (list_entry.length() > 255) ? 255 : list_entry.length();

					for(u32 x = 0; x < str_len; x++)
					{
						u8 chr = list_entry[x];
						play_yan.card_data[x * 2] = 0x00;
						play_yan.card_data[(x * 2) + 1] = chr;
					}

					//Set file/folder flag expected by NMP. 0x01 = Folder, 0x02 = File
					play_yan.card_data[525] = (is_folder) ? 0x01 : 0x02;
				}

				break;

			//ID3 Data
			case 0x40:
				play_yan.card_data.resize(272, 0x00);

				{
					u32 id3_pos = 4;

					u32 str_len = (play_yan.nmp_title.length() > 66) ? 66 : play_yan.nmp_title.length();

					for(u32 x = 0; x < str_len; x++)
					{
						u8 chr = play_yan.nmp_title[x];
						play_yan.card_data[id3_pos++] = 0x00;
						play_yan.card_data[id3_pos++] = chr;
					}

					id3_pos = 136;

					str_len = (play_yan.nmp_artist.length() > 68) ? 68 : play_yan.nmp_artist.length();

					for(u32 x = 0; x < str_len; x++)
					{
						u8 chr = play_yan.nmp_artist[x];
						play_yan.card_data[id3_pos++] = 0x00;
						play_yan.card_data[id3_pos++] = chr;
					}
				}

				break;

			//Music Data
			case 0x8100:
				//Manually trigger another 0x8100 to update timestamp separately from audio buffer
				if((play_yan.access_param == play_yan.nmp_audio_index) && (play_yan.update_trackbar_timestamp))
				{
					play_yan.nmp_manual_irq = true;
					play_yan.nmp_read_count = 0;
				}

				break;
		}
	}
}
