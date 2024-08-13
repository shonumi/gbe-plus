// GB Enhanced+ Copyright Daniel Baxter 2022
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : play_yan.cpp
// Date : August 19, 2022
// Description : Nintendo Play-Yan
//
// Handles I/O for the AGS-006 family (Play-Yan and Play-Yan Micro)
// Manages IRQs and firmware reads/writes
// Nintendo MP3 Player handled separately (see nmp.cpp)

#ifdef GBE_IMAGE_FORMATS
#include <SDL2/SDL_image.h>
#endif

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

	play_yan.video_bytes.clear();
	play_yan.video_frames.clear();

	play_yan.status = 0x80;
	play_yan.op_state = PLAY_YAN_NOP;

	play_yan.access_mode = 0;
	play_yan.access_param = 0;

	play_yan.irq_delay = 0;
	play_yan.last_delay = 0;
	play_yan.irq_data_in_use = false;
	play_yan.start_irqs = false;
	play_yan.irq_update = false;

	play_yan.cycles = 0;
	play_yan.cycle_limit = 479232;

	play_yan.is_video_playing = false;
	play_yan.is_music_playing = false;
	play_yan.is_media_playing = false;
	play_yan.is_end_of_samples = false;

	play_yan.audio_irq_active = false;
	play_yan.video_irq_active = false;

	for(u32 x = 0; x < 12; x++) { play_yan.cnt_data[x] = 0; }
	for(u32 x = 0; x < 16; x++) { play_yan.nmp_status_data[x] = 0; }
	play_yan.cmd = 0xDEADBEEF;
	play_yan.update_cmd = 0xFEEDBEEF;

	//Set 16-bit data for Nintendo MP3 Player status checking for boot?
	play_yan.nmp_boot_data[0] = 0x8001;
	play_yan.nmp_boot_data[1] = 0x8600;

	for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
	play_yan.irq_data[0] = 0x80000100;

	play_yan.music_length = 0;
	play_yan.tracker_progress = 0;
	play_yan.video_progress = 0;
	play_yan.video_length = 0;
	play_yan.video_current_fps = 0;
	play_yan.video_frame_count = 0;
	play_yan.audio_buffer_size = 0;
	play_yan.audio_frame_count = 0;
	play_yan.audio_sample_index = 0;
	play_yan.current_frame = 0;
	play_yan.update_video_frame = false;
	play_yan.update_audio_stream = false;
	play_yan.update_trackbar_timestamp = false;

	play_yan.video_data_addr = 0;
	play_yan.thumbnail_addr = 0;
	play_yan.thumbnail_index = 0;
	play_yan.video_index = 0;

	play_yan.use_bass_boost = false;
	play_yan.capture_command_stream = false;

	play_yan.type = PLAY_YAN_OG;

	play_yan.current_dir = config::data_path + "play_yan";
	play_yan.base_dir = play_yan.current_dir;
	play_yan_set_folder();

	play_yan.current_music_file = "";
	play_yan.current_video_file = "";

	play_yan.audio_channels = 0;
	play_yan.audio_sample_rate = 0;
	play_yan.l_audio_dither_error = 0;
	play_yan.r_audio_dither_error = 0;

	play_yan.nmp_data_index = 0;
	play_yan.nmp_cmd_status = 0;
	play_yan.nmp_ticks = 0;
	play_yan.nmp_entry_count = 0;
	play_yan.nmp_manual_cmd = 0;
	play_yan.nmp_init_stage = 0;
	play_yan.nmp_audio_index = 0;
	play_yan.nmp_seek_pos = 0;
	play_yan.nmp_seek_dir = 0xFF;
	play_yan.nmp_seek_count = 0;
	play_yan.nmp_valid_command = false;
	play_yan.nmp_manual_irq = false;
	play_yan.nmp_title = "";
	play_yan.nmp_artist = "";

	std::string sfx_filename = config::data_path + "play_yan/sfx.wav";
	play_yan_load_sfx(sfx_filename);
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
		play_yan.op_state = PLAY_YAN_INIT;
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
			if((play_yan.cmd == PLAY_YAN_GET_FILESYS_INFO) && (control_cmd2 == 0x02)) { play_yan_set_music_file(); }

			//Set video file data
			else if((play_yan.cmd == PLAY_YAN_GET_FILESYS_INFO) && (control_cmd2 == 0x01)) { play_yan_set_video_file(); }

			//Set file data
			else if(play_yan.cmd == PLAY_YAN_GET_FILESYS_INFO) { play_yan_set_folder(); }

			//Music position seeking
			else if((play_yan.cmd == PLAY_YAN_SEEK) && (play_yan.is_music_playing))
			{
				u32 current_sample_len = apu_stat->ext_audio.length / (apu_stat->ext_audio.channels * 2);

				//Update music progress via IRQ data
				double result = control_cmd2;
				result /= 25600;
				result *= current_sample_len;
				
				if(apu_stat->ext_audio.use_headphones) { apu_stat->ext_audio.sample_pos = result; }
				else { }

				play_yan.irq_delay = 1;
				process_play_yan_irq();
			}

			//Video position seeking
			else if((play_yan.cmd == PLAY_YAN_SEEK) && (play_yan.is_video_playing))
			{
				u16 factor = (control_cmd2 >= 0xFFFD) ? ((0xFFFF - control_cmd2) + 1) : control_cmd2;

				//Advance trackbar and timestamp
				if(control_cmd2 <= 0x03)
				{
					play_yan.video_frame_count += (4.0 * factor);
					play_yan.current_frame += (4 * factor);

					if(play_yan.video_frame_count >= 30.0)
					{
						play_yan.video_frame_count -= 30.0;
					}

					play_yan.video_progress = (play_yan.current_frame * 33.3333);

					//Update sound as well
					u32 current_sample_len = apu_stat->ext_audio.length / (apu_stat->ext_audio.channels * 2);
					double result = double(play_yan.video_progress) / play_yan.video_length;
					result *= current_sample_len;

					if(apu_stat->ext_audio.use_headphones) { apu_stat->ext_audio.sample_pos = result; }
					else { play_yan.audio_sample_index = ((1/30.0 * play_yan.current_frame) * 8192.0); }

					play_yan.irq_delay = 1;
					process_play_yan_irq();
				}

				//Rewind trackbar and timestamp
				else if(control_cmd2 >= 0xFFFD)
				{
					play_yan.video_frame_count -= (8.0 * factor);
					play_yan.current_frame -= (8 * factor);

					if(play_yan.video_frame_count < 0)
					{
						play_yan.video_frame_count += 30.0;
					}

					if(play_yan.current_frame > play_yan.video_frames.size())
					{
						play_yan.current_frame = 0;
					}

					play_yan.video_progress = (play_yan.current_frame * 33.3333);

					//Update sound as well
					u32 current_sample_len = apu_stat->ext_audio.length / (apu_stat->ext_audio.channels * 2);
					double result = double(play_yan.video_progress) / play_yan.video_length;
					result *= current_sample_len;

					if(apu_stat->ext_audio.use_headphones) { apu_stat->ext_audio.sample_pos = result; }
					else { play_yan.audio_sample_index = ((1/30.0 * play_yan.current_frame) * 8192.0); }

					play_yan.irq_delay = 1;
					process_play_yan_irq();
				}

				//Set specific timestamp
				else
				{
					play_yan.video_frame_count = 0;
					play_yan.current_frame = (30 * control_cmd2);
					play_yan.video_progress = (play_yan.current_frame * 33.3333);

					//Update sound as well
					u32 current_sample_len = apu_stat->ext_audio.length / (apu_stat->ext_audio.channels * 2);
					double result = double(play_yan.video_progress) / play_yan.video_length;
					result *= current_sample_len;

					if(apu_stat->ext_audio.use_headphones) { apu_stat->ext_audio.sample_pos = result; }
					else { }

					play_yan.irq_delay = 1;
					process_play_yan_irq();
				}

				play_yan.irq_data[6] = play_yan.video_progress;
			}

			//Adjust Play-Yan volume settings
			if(play_yan.cmd == PLAY_YAN_SET_VOLUME)
			{
				play_yan.volume = control_cmd2;
				apu_stat->ext_audio.volume = (play_yan.volume / 56.0) * 63.0;
			}

			//Adjust Play-Yan bass boost settings
			else if(play_yan.cmd == PLAY_YAN_SET_BASS_BOOST) { play_yan.bass_boost = control_cmd2; }

			//Adjust Play-Yan video brightness
			else if(play_yan.cmd == PLAY_YAN_SET_BRIGHTNESS) { play_yan.video_brightness = control_cmd2; }

			//Turn on/off Play-Yan bass boost
			else if(play_yan.cmd == PLAY_YAN_ENABLE_BASS_BOOST) { play_yan.use_bass_boost = (control_cmd2 == 0x8F0F) ? false : true; }
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
			if((play_yan.cmd == PLAY_YAN_GET_FILESYS_INFO) && (control_cmd2 == 0x02)) { play_yan_set_music_file(); }

			//Set video file data
			else if((play_yan.cmd == PLAY_YAN_GET_FILESYS_INFO) && (control_cmd2 == 0x01)) { play_yan_set_video_file(); }

			//Set file data
			else if(play_yan.cmd == PLAY_YAN_GET_FILESYS_INFO) { play_yan_set_folder(); }

			//Music position seeking
			else if((play_yan.cmd == PLAY_YAN_SEEK) && (play_yan.is_music_playing))
			{
				u32 current_sample_len = apu_stat->ext_audio.length / (apu_stat->ext_audio.channels * 2);

				//Update music progress via IRQ data
				double result = control_cmd2;
				result /= 25600;
				result *= current_sample_len;
				
				if(apu_stat->ext_audio.use_headphones) { apu_stat->ext_audio.sample_pos = result; }
				else { }

				play_yan.irq_delay = 1;
				process_play_yan_irq();
			}

			//Video position seeking
			else if((play_yan.cmd == PLAY_YAN_SEEK) && (play_yan.is_video_playing))
			{
				u16 factor = (control_cmd2 >= 0xFFFFFFFD) ? ((0xFFFFFFFF - control_cmd2) + 1) : control_cmd2;

				//Advance trackbar and timestamp
				if(control_cmd2 <= 0x03)
				{
					play_yan.video_frame_count += (4.0 * factor);
					play_yan.current_frame += (4 * factor);

					if(play_yan.video_frame_count >= 30.0)
					{
						play_yan.video_frame_count -= 30.0;
					}

					play_yan.video_progress = (play_yan.current_frame * 33.3333);

					//Update sound as well
					u32 current_sample_len = apu_stat->ext_audio.length / (apu_stat->ext_audio.channels * 2);
					double result = double(play_yan.video_progress) / play_yan.video_length;
					result *= current_sample_len;

					if(apu_stat->ext_audio.use_headphones) { apu_stat->ext_audio.sample_pos = result; }
					else { play_yan.audio_sample_index = ((1/30.0 * play_yan.current_frame) * 8192.0); }

					play_yan.irq_delay = 1;
					process_play_yan_irq();				
				}

				//Rewind trackbar and timestamp
				else if(control_cmd2 >= 0xFFFFFFFD)
				{
					play_yan.video_frame_count -= (8.0 * factor);
					play_yan.current_frame -= (8 * factor);

					if(play_yan.video_frame_count < 0)
					{
						play_yan.video_frame_count += 30.0;
					}

					if(play_yan.current_frame > play_yan.video_frames.size())
					{
						play_yan.current_frame = 0;
					}

					play_yan.video_progress = (play_yan.current_frame * 33.3333);

					//Update sound as well
					u32 current_sample_len = apu_stat->ext_audio.length / (apu_stat->ext_audio.channels * 2);
					double result = double(play_yan.video_progress) / play_yan.video_length;
					result *= current_sample_len;

					if(apu_stat->ext_audio.use_headphones) { apu_stat->ext_audio.sample_pos = result; }
					else { play_yan.audio_sample_index = ((1/30.0 * play_yan.current_frame) * 8192.0); }

					play_yan.irq_delay = 1;
					process_play_yan_irq();
				}

				//Set specific timestamp
				else
				{
					play_yan.video_frame_count = 0;
					play_yan.current_frame = (30 * control_cmd2);
					play_yan.video_progress = (play_yan.current_frame * 33.3333);

					//Update sound as well
					u32 current_sample_len = apu_stat->ext_audio.length / (apu_stat->ext_audio.channels * 2);
					double result = double(play_yan.video_progress) / play_yan.video_length;
					result *= current_sample_len;

					if(apu_stat->ext_audio.use_headphones) { apu_stat->ext_audio.sample_pos = result; }
					else { }

					play_yan.irq_delay = 1;
					process_play_yan_irq();
				}

				play_yan.irq_data[6] = play_yan.video_progress;
			}

			//Adjust Play-Yan volume settings
			if(play_yan.cmd == PLAY_YAN_SET_VOLUME)
			{
				play_yan.volume = control_cmd2;
				apu_stat->ext_audio.volume = (play_yan.volume / 56.0) * 63.0;
			}

			//Adjust Play-Yan bass boost settings
			else if(play_yan.cmd == PLAY_YAN_SET_BASS_BOOST) { play_yan.bass_boost = control_cmd2; }

			//Adjust Play-Yan video brightness
			else if(play_yan.cmd == PLAY_YAN_SET_BRIGHTNESS) { play_yan.video_brightness = control_cmd2; }

			//Turn on/off Play-Yan bass boost
			else if(play_yan.cmd == PLAY_YAN_ENABLE_BASS_BOOST) { play_yan.use_bass_boost = (control_cmd2 == 0x8F0F) ? false : true; }
		}
	}

	//Grab entire command stream
	if(((address >= 0xB000100) && (play_yan.firmware_addr >= 0xFF020) && (play_yan.capture_command_stream) && (play_yan.access_mode == 0x68))
	|| ((address >= 0xB000000) && (play_yan.firmware_addr >= 0xFF020) && (play_yan.capture_command_stream) && (play_yan.access_mode == 0x28)))
	{
		play_yan.command_stream.push_back(value);

		//Grab string of music/video filename to play
		if((value == 0x00) && ((play_yan.cmd == PLAY_YAN_SET_DIR) || (play_yan.cmd == PLAY_YAN_GET_THUMBNAIL) || (play_yan.cmd == PLAY_YAN_GET_ID3_DATA)
		|| (play_yan.cmd == PLAY_YAN_PLAY_VIDEO) || (play_yan.cmd == PLAY_YAN_PLAY_MUSIC)))
		{
			std::string temp_str = "";
			u32 offset = (play_yan.cmd == PLAY_YAN_PLAY_VIDEO) ? 9 : 1;
			if(play_yan.cmd == PLAY_YAN_GET_THUMBNAIL) { offset = 17; }

			//Don't process if not enough of the command stream for the filename is sent
			if(((play_yan.cmd == PLAY_YAN_SET_DIR) || (play_yan.cmd == PLAY_YAN_GET_ID3_DATA) 
			|| (play_yan.cmd == PLAY_YAN_PLAY_MUSIC)) && (play_yan.command_stream.size() < 4))
			{
				return;
			}

			if((play_yan.cmd == PLAY_YAN_PLAY_VIDEO) && (play_yan.command_stream.size() < 12)) { return; }
			if((play_yan.cmd == PLAY_YAN_GET_THUMBNAIL) && (play_yan.command_stream.size() < 20)) { return; }

			play_yan.capture_command_stream = false;
			play_yan.command_stream.pop_back();

			for(u32 x = offset; x < play_yan.command_stream.size(); x++)
			{
				u8 chr = play_yan.command_stream[x];
				temp_str += chr;
			}

			//Set the current directory
			if(play_yan.cmd == PLAY_YAN_SET_DIR)
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
			else if(play_yan.cmd == PLAY_YAN_GET_THUMBNAIL)
			{
				//Look up .bmp thumbnail, same name as video file, different extension
				std::string old_filename = temp_str;
				temp_str = util::get_filename_no_ext(temp_str) + ".bmp";

				//Convert backslash to forward slash
				for(u32 x = 0; x < temp_str.length(); x++)
				{
					if(temp_str[x] == 0x5C) { temp_str[x] = 0x2F; }
				}

				for(u32 x = 0; x < old_filename.length(); x++)
				{
					if(old_filename[x] == 0x5C) { old_filename[x] = 0x2F; }
				}

				read_play_yan_thumbnail(temp_str);

				//Reset video length in ms
				play_yan_check_video_header(old_filename);
			}

			//Grab ID3 data for a song
			else if(play_yan.cmd == PLAY_YAN_GET_ID3_DATA)
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
			else if(play_yan.cmd == PLAY_YAN_PLAY_MUSIC)
			{
				play_yan_load_audio(play_yan.current_music_file);
			}


			//Search for internal ID associated with video file
			else
			{
				for(u32 x = 0; x < play_yan.video_files.size(); x++)
				{
					std::string cmp_1 = util::get_filename_from_path(play_yan.video_files[x]);
					std::string cmp_2 = util::get_filename_from_path(temp_str);

					if(cmp_1 == cmp_2)
					{
						//Attempt to load video, otherwise setup dummy data
						//Dummy = blank violet screen, 10 seconds
						if(!play_yan_load_video(play_yan.video_files[x]))
						{
							play_yan.video_length = (10 * 33.3333 * 30);
							play_yan.video_current_fps = 1.0;
							play_yan.video_data.clear();
							play_yan.video_data.resize(0x12C00, 0x3C);
						}

						break;
					}
				}
			}
		}
	}	
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
		//No Headphones - Stop sound sample playback
		if(play_yan.is_end_of_samples && play_yan.is_music_playing)
		{
			for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
			play_yan.irq_data[0] = PLAY_YAN_STOP_MUSIC | 0x40000000;

			play_yan.cycles = 0;
			play_yan.irq_delay = 1;
			play_yan.is_music_playing = false;
			play_yan.is_media_playing = false;
			play_yan.is_end_of_samples = false;
			process_play_yan_irq();
		}

		//No Headphones - Update music trackbar
		else if(play_yan.update_trackbar_timestamp)
		{
			for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
			play_yan.irq_data[0] = 0x80001000;

			u32 current_sample_pos = play_yan.audio_sample_index;
			u32 current_sample_rate = 16384;

			u32 current_sample_len = apu_stat->ext_audio.length / (apu_stat->ext_audio.channels * 2);
			current_sample_len = (float(current_sample_len) / float(play_yan.audio_sample_rate)) * 16384;

			if(current_sample_rate && current_sample_len)
			{
				//Update trackbar
				play_yan.tracker_update_size = (float(current_sample_pos) / float(current_sample_len) * 0x6400);
				play_yan.irq_data[5] = play_yan.tracker_update_size;

				//Update music progress via IRQ data
				play_yan.irq_data[6] = current_sample_pos / current_sample_rate;
			}

			play_yan.irq_delay = 1;
			play_yan.update_trackbar_timestamp = false;
			process_play_yan_irq();
		}

		u32 offset = address - 0xB000300;
		
		if((play_yan.card_addr + offset) < 0x10000)
		{
			result = play_yan.card_data[play_yan.card_addr + offset];

			//Check for the end of reading audio samples from the SD card interface
			if((play_yan.audio_irq_active) && ((play_yan.card_addr + offset) == 0x23F))
			{
				play_yan.audio_frame_count++;

				if((play_yan.audio_frame_count & 0x1) == 0)
				{
					play_yan.audio_irq_active = false;

					//Once audio has been processed, check to see if any delayed video frames are present
					if(play_yan.video_irq_active && play_yan.is_media_playing)
					{
						play_yan.audio_irq_active = false;
						play_yan_set_video_pixels();
					}
				}
			}

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
			play_yan.thumbnail_index++;

			if(t_addr < 0x12C0)
			{
				result = play_yan.video_thumbnail[t_addr];

				//Update Play-Yan thubnail address if necessary
				if(play_yan.thumbnail_index == 0x200)
				{
					play_yan.thumbnail_addr += 0x200;
					play_yan.thumbnail_index = 0;
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
				if(offset == 0x1FF)
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

/****** Handles Play-Yan command processing ******/
void AGB_MMU::process_play_yan_cmd()
{
	u32 prev_cmd = play_yan.cmd;
	play_yan.cmd = ((play_yan.cnt_data[3] << 24) | (play_yan.cnt_data[2] << 16) | (play_yan.cnt_data[1] << 8) | (play_yan.cnt_data[0]));

	std::cout<<"CMD -> 0x" << play_yan.cmd << "\n";

	//Trigger Game Pak IRQ for Get Filesystem Info command
	if(play_yan.cmd == PLAY_YAN_GET_FILESYS_INFO)
	{
		play_yan.op_state = PLAY_YAN_PROCESS_CMD;

		play_yan.irq_delay = 10;

		for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
		play_yan.irq_data[0] = PLAY_YAN_GET_FILESYS_INFO | 0x40000000;
	}

	//Trigger Game Pak IRQ for Change Current Directory command
	else if(play_yan.cmd == PLAY_YAN_SET_DIR)
	{
		play_yan.op_state = PLAY_YAN_PROCESS_CMD;

		play_yan.irq_delay = 10;

		for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
		play_yan.irq_data[0] = PLAY_YAN_SET_DIR | 0x40000000;

		play_yan.capture_command_stream = true;
		play_yan.command_stream.clear();
	}

	//Trigger Game Pak IRQ for video thumbnail data
	else if(play_yan.cmd == PLAY_YAN_GET_THUMBNAIL)
	{
		play_yan.op_state = PLAY_YAN_PROCESS_VIDEO_THUMBNAILS;

		play_yan.irq_delay = 1;

		play_yan.thumbnail_addr = 0;
		play_yan.thumbnail_index = 0;

		for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
		play_yan.irq_data[0] = PLAY_YAN_GET_THUMBNAIL | 0x40000000;

		play_yan.capture_command_stream = true;
		play_yan.command_stream.clear();
	}

	//Trigger Game Pak IRQ for ID3 data retrieval
	else if(play_yan.cmd == PLAY_YAN_GET_ID3_DATA)
	{
		play_yan.op_state = PLAY_YAN_PROCESS_CMD;

		play_yan.irq_delay = 1;

		for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
		play_yan.irq_data[0] = PLAY_YAN_GET_ID3_DATA | 0x40000000;
		play_yan.irq_data[1] = 0x01;

		play_yan.capture_command_stream = true;
		play_yan.command_stream.clear();
	}

	//Trigger Game Pak IRQ for playing video
	else if(play_yan.cmd == PLAY_YAN_PLAY_VIDEO)
	{
		play_yan.op_state = PLAY_YAN_START_VIDEO;
		play_yan.irq_update = apu_stat->ext_audio.use_headphones;
		play_yan.update_cmd = PLAY_YAN_PLAY_VIDEO;

		play_yan.irq_delay = 1;

		play_yan.audio_buffer_size = 0x480;
		play_yan.audio_sample_index = 0;
		play_yan.audio_frame_count = 0;
		play_yan.cycles = 0;
		play_yan.cycle_limit = 1017856;

		play_yan.video_data_addr = 0;
		play_yan.video_progress = 0;
		play_yan.video_frame_count = 0;
		play_yan.current_frame = 0;
		play_yan.is_video_playing = true;
		play_yan.is_media_playing = true;

		play_yan.tracker_progress = 0;
		play_yan.tracker_update_size = 0;

		for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
		play_yan.irq_data[0] = PLAY_YAN_PLAY_VIDEO | 0x40000000;

		play_yan.capture_command_stream = true;
		play_yan.command_stream.clear();
	}

	//Trigger Game Pak IRQ for stopping video
	else if(play_yan.cmd == PLAY_YAN_STOP_VIDEO)
	{
		play_yan.op_state = PLAY_YAN_PROCESS_CMD;
		play_yan.irq_update = false;
		play_yan.update_cmd = 0;

		play_yan.irq_delay = 1;

		play_yan.video_data_addr = 0;
		play_yan.video_progress = 0;
		play_yan.video_frame_count = 0;
		play_yan.current_frame = 0;
		play_yan.cycles = 0;
		play_yan.is_video_playing = false;
		play_yan.is_media_playing = false;
		apu_stat->ext_audio.playing = false;

		play_yan.audio_channels = 0;
		play_yan.audio_sample_rate = 0;
		apu_stat->ext_audio.sample_pos = 0;

		for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
		play_yan.irq_data[0] = PLAY_YAN_STOP_VIDEO | 0x40000000;
	}

	//Trigger Game Pak IRQ for playing music
	else if(play_yan.cmd == PLAY_YAN_PLAY_MUSIC)
	{
		play_yan.op_state = PLAY_YAN_START_AUDIO;
		play_yan.irq_update = apu_stat->ext_audio.use_headphones;
		play_yan.update_cmd = PLAY_YAN_PLAY_MUSIC;

		play_yan.irq_delay = 1;

		play_yan.is_music_playing = true;
		play_yan.is_media_playing = true;
		play_yan.audio_buffer_size = 0x480;
		play_yan.audio_sample_index = 0;
		play_yan.cycles = 0;
		play_yan.cycle_limit = 479232;

		play_yan.tracker_progress = 0;
		play_yan.tracker_update_size = 0;

		for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
		play_yan.irq_data[0] = PLAY_YAN_PLAY_MUSIC | 0x40000000;
		play_yan.irq_data[5] = play_yan.tracker_progress;

		play_yan.capture_command_stream = true;
		play_yan.command_stream.clear();
	}

	//Trigger Game Pak IRQ for stopping music
	else if(play_yan.cmd == PLAY_YAN_STOP_MUSIC)
	{
		play_yan.op_state = PLAY_YAN_PROCESS_CMD;
		play_yan.irq_update = false;
		play_yan.update_cmd = 0;

		play_yan.irq_delay = 1;

		play_yan.is_music_playing = false;
		play_yan.is_media_playing = false;
		apu_stat->ext_audio.playing = false;

		play_yan.audio_channels = 0;
		play_yan.audio_sample_rate = 0;
		apu_stat->ext_audio.sample_pos = 0;

		for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
		play_yan.irq_data[0] = PLAY_YAN_STOP_MUSIC | 0x40000000;
	}

	//Pause Media Playback
	else if(play_yan.cmd == PLAY_YAN_PAUSE)
	{
		play_yan.is_media_playing = false;
		apu_stat->ext_audio.playing = false;
	}

	//Resume Media Playback
	else if(play_yan.cmd == PLAY_YAN_RESUME)
	{
		play_yan.is_media_playing = true;

		if(play_yan.audio_channels && play_yan.audio_sample_rate)
		{
			apu_stat->ext_audio.playing = true;
		}
	}

	//Trigger Game Pak IRQ for cartridge status
	else if(play_yan.cmd == PLAY_YAN_GET_STATUS)
	{
		play_yan.irq_delay = 60;

		for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
		play_yan.irq_data[0] = PLAY_YAN_GET_STATUS | 0x40000000;
		play_yan.irq_data[1] = 0x00000005;
	}

	//Trigger Game Pak IRQ for booting cartridge/status (firmware related?)
	else if(play_yan.cmd == PLAY_YAN_FIRMWARE)
	{
		play_yan.op_state = PLAY_YAN_PROCESS_CMD;
		play_yan.irq_update = true;
		play_yan.update_cmd = PLAY_YAN_FIRMWARE;

		play_yan.irq_delay = 10;

		for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
		play_yan.irq_data[0] = PLAY_YAN_FIRMWARE | 0x40000000;
		play_yan.irq_data[1] = 0x00000005;
	}

	//Trigger Play-Yan Micro Game Pak IRQ to open .ini file 
	else if(play_yan.cmd == PLAY_YAN_CHECK_KEY_FILE)
	{
		play_yan.op_state = PLAY_YAN_PROCESS_CMD;

		play_yan.irq_delay = 60;

		for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
		play_yan.irq_data[0] = PLAY_YAN_CHECK_KEY_FILE | 0x40000000;

		//0x3000 command is unique to Play-Yan Micro. Switch type if command detected
		play_yan.type = PLAY_YAN_MICRO;
		std::cout<<"MMU::Play-Yan Micro detected\n";
	}

	//Trigger Play-Yan Micro Game Pak IRQ to read .ini file 
	else if(play_yan.cmd == PLAY_YAN_READ_KEY_FILE)
	{
		play_yan.op_state = PLAY_YAN_PROCESS_CMD;

		play_yan.irq_delay = 60;

		for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
		play_yan.irq_data[0] = PLAY_YAN_READ_KEY_FILE | 0x40000000;

		play_yan_set_ini_file();
	}	

	//Trigger Play-Yan Micro Game Pak IRQ to close .ini file??? 
	else if(play_yan.cmd == PLAY_YAN_CLOSE_KEY_FILE)
	{
		play_yan.op_state = PLAY_YAN_PROCESS_CMD;

		play_yan.irq_delay = 60;

		for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
		play_yan.irq_data[0] = PLAY_YAN_CLOSE_KEY_FILE | 0x40000000;
	}		
}

/****** Updates Play-Yan when responding to certain IRQs ******/
void AGB_MMU::play_yan_update()
{
	switch(play_yan.update_cmd)
	{
		case PLAY_YAN_FIRMWARE:
			play_yan.op_state = PLAY_YAN_WAIT;
			play_yan.irq_update = false;
			play_yan.update_cmd = 0;

			play_yan.irq_delay = 1;

			for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
			play_yan.irq_data[0] = 0x80000100;

			break;

		case PLAY_YAN_PLAY_MUSIC:
			for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }

			if(play_yan.is_media_playing)
			{
				play_yan.irq_delay = 60;
				play_yan.irq_data[0] = 0x80001000;

				u32 current_sample_pos = (apu_stat->ext_audio.use_headphones) ? apu_stat->ext_audio.sample_pos : 0;
				u32 current_sample_rate = (apu_stat->ext_audio.use_headphones) ? play_yan.audio_sample_rate : 16384;
				u32 current_sample_len = apu_stat->ext_audio.length / (apu_stat->ext_audio.channels * 2);

				if(current_sample_rate && current_sample_len)
				{
					//Update trackbar
					play_yan.tracker_update_size = (float(current_sample_pos) / float(current_sample_len) * 0x6400);
					play_yan.irq_data[5] = play_yan.tracker_update_size;

					//Update music progress via IRQ data
					play_yan.irq_data[6] = current_sample_pos / current_sample_rate;
				}

				//Start external audio output
				if(!apu_stat->ext_audio.playing && play_yan.audio_sample_rate && play_yan.audio_channels)
				{
					apu_stat->ext_audio.channels = play_yan.audio_channels;
					apu_stat->ext_audio.frequency = play_yan.audio_sample_rate;
					apu_stat->ext_audio.sample_pos = 0;
					apu_stat->ext_audio.playing = true;
				}

				//Stop when length is complete
				if(current_sample_pos >= current_sample_len)
				{
					play_yan.op_state = PLAY_YAN_PROCESS_CMD;
					play_yan.irq_update = false;
					play_yan.update_cmd = 0;

					play_yan.irq_delay = 1;

					play_yan.is_music_playing = false;
					play_yan.is_media_playing = false;
					apu_stat->ext_audio.playing = false;

					play_yan.audio_channels = 0;
					play_yan.audio_sample_rate = 0;
					apu_stat->ext_audio.sample_pos = 0;

					for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
					play_yan.irq_data[0] = PLAY_YAN_STOP_MUSIC | 0x40000000;
				}
			}

			break;

		case PLAY_YAN_PLAY_VIDEO:
			for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
			
			if(play_yan.is_media_playing)
			{
				play_yan.irq_delay = 2;
				play_yan.irq_data[0] = 0x80001000;
				play_yan.irq_data[1] = 0x31AC0;
				play_yan.irq_data[2] = 0x12C00;

				//Update video frame counter and grab new video frame if necessary
				play_yan.video_frame_count += 1.0;
				play_yan.current_frame++;

				//Update video progress via IRQ data
				play_yan.video_progress = (play_yan.current_frame * 33.3333);
				play_yan.irq_data[6] = play_yan.video_progress;

				if(play_yan.video_frame_count >= play_yan.video_current_fps)
				{
					play_yan.video_frame_count -= play_yan.video_current_fps;
					play_yan_grab_frame_data(play_yan.current_frame);
				}

				//Start external audio output
				if(!apu_stat->ext_audio.playing && play_yan.audio_sample_rate && play_yan.audio_channels)
				{
					apu_stat->ext_audio.channels = play_yan.audio_channels;
					apu_stat->ext_audio.frequency = play_yan.audio_sample_rate;
					apu_stat->ext_audio.sample_pos = 0;
					apu_stat->ext_audio.playing = true;
				}

				//Stop video when length is complete
				if(play_yan.video_progress >= play_yan.video_length)
				{
					play_yan.op_state = PLAY_YAN_PROCESS_CMD;
					play_yan.irq_update = false;
					play_yan.update_cmd = 0;

					play_yan.irq_delay = 1;

					play_yan.video_data_addr = 0;
					play_yan.video_progress = 0;
					play_yan.is_video_playing = false;
					play_yan.is_media_playing = false;

					play_yan.audio_channels = 0;
					play_yan.audio_sample_rate = 0;
					apu_stat->ext_audio.sample_pos = 0;
					apu_stat->ext_audio.playing = false;

					for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
					play_yan.irq_data[0] = PLAY_YAN_STOP_VIDEO | 0x40000000;
				}
			}

			break;
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

	//Check for video updates (based on the LCD's operation)
	if(play_yan.update_video_frame && play_yan.is_media_playing)
	{
		play_yan.update_video_frame = false;
		play_yan.video_frame_count += 1.0;

		if(play_yan.video_frame_count >= 60.0) { play_yan.video_frame_count = 0; }

		if(int(play_yan.video_frame_count) & 0x1)
		{
			play_yan.current_frame++;

			//Draw video pixels immediately or delay if audio samples are currently being processed
			if(!play_yan.audio_irq_active) { play_yan_set_video_pixels(); }
			else { play_yan.video_irq_active = true; }
		}
	}

	if(play_yan.type == NINTENDO_MP3)
	{
		play_yan.cmd = play_yan.nmp_manual_cmd;
		play_yan.nmp_manual_cmd = 0;

		play_yan.nmp_status_data[0] = (play_yan.cmd >> 8);
		play_yan.nmp_status_data[1] = (play_yan.cmd & 0xFF);
		play_yan.nmp_data_index = 0;
		play_yan.access_param = 0;
		play_yan.op_state = PLAY_YAN_PROCESS_CMD;

		memory_map[REG_IF+1] |= 0x20;

		if(play_yan.cmd) { process_nmp_cmd(); }

		return;
	}

	if(!play_yan.cmd) { return; }

	//Process SD card check first and foremost after booting
	if(play_yan.op_state == PLAY_YAN_NOP) { play_yan.op_state = PLAY_YAN_PROCESS_CMD; }

	else if(play_yan.op_state == PLAY_YAN_WAIT) { return; }

	else if(play_yan.op_state == PLAY_YAN_IRQ_UPDATE) { play_yan_update(); }

	//Trigger Game Pak IRQ
	memory_map[REG_IF+1] |= 0x20;
	play_yan.irq_data_in_use = true;

	if(play_yan.irq_update)
	{
		play_yan.op_state = PLAY_YAN_IRQ_UPDATE;

		if(!play_yan.irq_delay) { play_yan.irq_delay = 1; }
	}

	//std::cout<<"IRQ -> 0x" << play_yan.irq_data[0] << "\n";
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

	SDL_FreeSurface(source);

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
	util::get_folders_in_dir(play_yan.current_dir, dir_listing);
	u32 folder_count = dir_listing.size();
	util::get_files_in_dir(play_yan.current_dir, ".mp3", dir_listing, false, true);

	//Write filenames and folder names to SD card data
	for(u32 x = 0; x < dir_listing.size(); x++)
	{
		//Set filetype meta data for Play-Yan
		if(x >= folder_count)
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
	util::get_files_in_dir(play_yan.current_dir, ".ASF", dir_listing, true, true);

	if(play_yan.type == PLAY_YAN_MICRO)
	{
		util::get_files_in_dir(play_yan.current_dir, ".mp4", dir_listing, true, true);
		util::get_files_in_dir(play_yan.current_dir, ".MP4", dir_listing, true, true);
	}	

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
	util::get_folders_in_dir(play_yan.current_dir, dir_listing);
	u32 folder_count = dir_listing.size();
	util::get_files_in_dir(play_yan.current_dir, "", dir_listing, false, false);

	//Write filenames and folder names to SD card data
	for(u32 x = 0; x < dir_listing.size(); x++)
	{
		//Set filetype meta data for Play-Yan
		if(x >= folder_count)
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

				//Check tag encoding type - Play-Yan and Play-Yan Micro only support 0x00 flag
				u8 encoding_type = mp3_data[x + 6];
				bool is_valid_encoding = true;

				if((encoding_type != 0x00) && (play_yan.type != NINTENDO_MP3)) { is_valid_encoding = false; }

				if(((x + 4) < file_size) && (is_valid_encoding))
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

				//Check tag encoding type - Play-Yan and Play-Yan Micro only support 0x00 flag
				u8 encoding_type = mp3_data[x + 6];
				bool is_valid_encoding = true;

				if((encoding_type != 0x00) && (play_yan.type != NINTENDO_MP3)) { is_valid_encoding = false; }

				if(((x + 4) < file_size) && (is_valid_encoding))
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

	//Save song title and artist data for later for NMP
	if(play_yan.type == NINTENDO_MP3)
	{
		play_yan.nmp_title = title;
		play_yan.nmp_artist = artist;
	}

	//Write song title and artist to SD card data - Play-Yan and Play-Yan Micro only
	else
	{
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
	play_yan.irq_data[2] = file_size;

	//Read data from file and copy to SD card data
	file.read(reinterpret_cast<char*> (&ini_data[0]), file_size);
	for(u32 x = 0; x < file_size; x++) { play_yan.card_data[x] = ini_data[x]; }
	file.close();

	std::cout<<"MMU::Reading Play-Yan Micro.ini file at " << filename << "\n";
}

/****** Sets SD card data to sound samples ******/
void AGB_MMU::play_yan_set_sound_samples()
{
	play_yan.card_data.clear();
	play_yan.card_data.resize(0x10000, 0x00);

	if(play_yan.audio_sample_rate)
	{
		float gba_sample_rate = play_yan.is_video_playing ? 8192.0 : 16384.0;
		double ratio = play_yan.audio_sample_rate / gba_sample_rate;
		s16* e_stream = (s16*)apu_stat->ext_audio.buffer;
		u32 stream_size = apu_stat->ext_audio.length / 2;

		s32 sample = 0;
		s16 error = 0;
		u32 index = 0;
		u32 index_shift = 0;
		u32 start_sample = play_yan.audio_sample_index;
		u32 offset = 0;
		u32 limit = (play_yan.audio_buffer_size / 2);

		if(play_yan.audio_channels) { index_shift = play_yan.audio_channels - 1; }

		for(u32 x = 0; x < play_yan.audio_buffer_size; x++)
		{
			if(x == limit) { play_yan.audio_sample_index -= limit; }
			bool is_left_channel = (x < limit) ? true : false; 

			index = (ratio * play_yan.audio_sample_index);
			index *= play_yan.audio_channels;
			index += (is_left_channel) ? 0 : index_shift;

			if(index >= stream_size)
			{
				index = (stream_size - 1);
				play_yan.is_end_of_samples = true;
			}

			//Perform simple Flyod-Steinberg dithering
			//Grab current sample and add 7/16 of error, quantize results, clip results 
			sample = e_stream[index];

			//Set volume manually
			sample *= (play_yan.volume / 56.0);
			if(sample > 32767) { sample = 32767; }
			else if(sample < -32768) { sample = 32768; }

			sample >>= 8;

			//Output new samples
			play_yan.card_data[offset++] = (sample & 0xFF);
			play_yan.audio_sample_index++;

			//Update music trackbar timestamp approximately, based on samples processed
			if(((play_yan.audio_sample_index % 16384) == 0) && (!play_yan.is_video_playing))
			{
				play_yan.update_trackbar_timestamp = true;
			}
		}

		play_yan.audio_irq_active = true;
	}
}

/****** Sets SD card data to video pixels ******/
void AGB_MMU::play_yan_set_video_pixels()
{
	for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
	play_yan.video_progress = (play_yan.current_frame * 33.3333);

	//Check if video has finished and stop accordingly
	if(play_yan.video_progress >= play_yan.video_length)
	{
		play_yan.op_state = PLAY_YAN_PROCESS_CMD;
		play_yan.irq_update = false;
		play_yan.update_cmd = 0;

		play_yan.irq_delay = 1;

		play_yan.video_data_addr = 0;
		play_yan.video_progress = 0;
		play_yan.is_video_playing = false;
		play_yan.is_media_playing = false;

		play_yan.audio_channels = 0;
		play_yan.audio_sample_rate = 0;
		apu_stat->ext_audio.sample_pos = 0;
		apu_stat->ext_audio.playing = false;

		play_yan.irq_data[0] = PLAY_YAN_STOP_VIDEO | 0x40000000;
	}

	//Otherwise render the most current video frame (even if some are possibly skipped)
	else
	{
		play_yan.irq_data[0] = 0x80001000;
		play_yan.irq_data[1] = 0x31AC0;
		play_yan.irq_data[2] = 0x12C00;

		//Update video progress via IRQ data
		play_yan.irq_data[6] = play_yan.video_progress;

		play_yan_grab_frame_data(play_yan.current_frame);
	}
}

/****** Wakes Play-Yan from GBA sleep mode - Fires Game Pak IRQ ******/
void AGB_MMU::play_yan_wake()
{
	play_yan.op_state = PLAY_YAN_WAKE;

	play_yan.irq_delay = 60;

	for(u32 x = 0; x < 8; x++) { play_yan.irq_data[x] = 0; }
	play_yan.irq_data[0] = PLAY_YAN_UNSLEEP | 0x80000000;
}

/****** Loads audio (.MP3) file and then converts it to .WAV for playback ******/
bool AGB_MMU::play_yan_load_audio(std::string filename)
{
	std::string sfx_filename = config::data_path + "play_yan/sfx.wav";
	bool is_sfx = (filename == sfx_filename) ? true : false;

	play_yan.music_length = 0;
	play_yan.tracker_update_size = 0;
	play_yan.audio_sample_rate = 0;
	play_yan.audio_channels = 0;

	//Clear previous buffer if necessary
	SDL_FreeWAV(apu_stat->ext_audio.buffer);
	apu_stat->ext_audio.buffer = NULL;

	SDL_AudioSpec file_spec;

	//Load music
	if(!is_sfx)
	{
		//Abort now if no audio conversion command is specified
		if(config::audio_conversion_cmd.empty())
		{
			std::cout<<"MMU::No audio conversion command specified. Cannot convert file " << filename << "\n";
			return false;
		}

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
	}

	//Load sfx
	else
	{
		if(play_yan.sfx_data.empty())
		{
			std::cout<<"MMU::Play-Yan SFX samples are empty\n";
			return false;
		}

		SDL_RWops* io_ops = SDL_AllocRW();
		io_ops = SDL_RWFromMem(play_yan.sfx_data.data(), play_yan.sfx_data.size());

		if(SDL_LoadWAV_RW(io_ops, 0, &file_spec, &apu_stat->ext_audio.buffer, &apu_stat->ext_audio.length) == NULL)
		{
			std::cout<<"MMU::Play-Yan could not load SFX samples : " << SDL_GetError() << "\n";
		}		
	}

	//Check format, must be S16 audio, LSB
	if(file_spec.format != AUDIO_S16)
	{
		std::cout<<"MMU::Play-Yan loaded file, but format is not Signed 16-bit LSB audio\n";
		return false;
	}

	//Check number of channels, max is 2
	if(file_spec.channels > 2)
	{
		std::cout<<"MMU::Play-Yan loaded file, but audio uses more than 2 channels\n";
		return false;
	}

	apu_stat->ext_audio.frequency = file_spec.freq;
	apu_stat->ext_audio.channels = file_spec.channels;

	play_yan.audio_channels = apu_stat->ext_audio.channels;
	play_yan.audio_sample_rate = apu_stat->ext_audio.frequency;

	if(!is_sfx)
	{
		//Determine song length in seconds by the amount of samples
		u32 song_samples = apu_stat->ext_audio.length / 2;
		if(file_spec.channels == 2) { song_samples /= 2; }

		u32 song_seconds = song_samples / apu_stat->ext_audio.frequency;
		if(song_samples % (u32)apu_stat->ext_audio.frequency) { song_seconds += 1; }
	
		play_yan.music_length = song_seconds;

		if(play_yan.type != NINTENDO_MP3)
		{
			play_yan.tracker_update_size = (0x6400 / play_yan.music_length);
		}

		std::cout<<"MMU::Play-Yan loaded audio file: " << filename << "\n";
	}

	return true;
}

/****** Loads video that's already been converted - MJPEG video, 16-bit PCM-LE ******/
bool AGB_MMU::play_yan_load_video(std::string filename)
{
	#ifdef GBE_IMAGE_FORMATS

	play_yan.audio_channels = 0;
	play_yan.audio_sample_rate = 0;

	//Clear video data now - Prevents leftover video data from accidentally repeating
	play_yan.video_bytes.clear();
	play_yan.video_frames.clear();

	std::vector<u8> vid_info;
	std::vector<u8> mus_info;
	std::vector<u8> tmp_info;
	std::ifstream vid_file(filename.c_str(), std::ios::binary);

	if(!vid_file.is_open()) 
	{
		play_yan.video_frames.resize(10000, 0xFFFFFFFF);
		std::cout<<"MMU::" << filename << " could not be opened. Check file path or permissions. \n";
		return false;
	}

	vid_file.seekg(0, vid_file.end);
	u32 vid_file_size = vid_file.tellg();
	vid_file.seekg(0, vid_file.beg);
	vid_info.resize(vid_file_size);

	vid_file.read(reinterpret_cast<char*> (&vid_info[0]), vid_file_size);
	vid_file.close();

	//Verify audio data format separately
	play_yan_check_audio_from_video(vid_info);

	u32 index = 0;

	//Search for movi and idx1 FOURCC as start and endpoints of usable data
	bool found_movi = false;

	for(u32 x = 0; x < (vid_file_size - 4); x++)
	{
		if((vid_info[x] == 0x6D) && (vid_info[x + 1] == 0x6F)
		&& (vid_info[x + 2] == 0x76) && (vid_info[x + 3] == 0x69))
		{
			index = x;
			found_movi = true;
		}
	} 

	if(!found_movi)
	{
		play_yan.video_frames.resize(10000, 0xFFFFFFFF);
		std::cout<<"MMU::No movi FOURCC found in " << filename << ". Check if file is valid AVI video.\n";
		return false;
	}

	for(u32 x = index; x < vid_file_size; x++)
	{
		if(x < (vid_file_size - 4))
		{

			if((vid_info[x] == 0x69) && (vid_info[x + 1] == 0x64)
			&& (vid_info[x + 2] == 0x78) && (vid_info[x + 3] == 0x31))
			{
				break;
			}
		}

		tmp_info.push_back(vid_info[x]);
	}

	//Search and parse each 00dc FOURCC - Video Frames
	for(u32 x = 0; x < (tmp_info.size() - 4); x++)
	{
		if((tmp_info[x] == 0x30) && (tmp_info[x + 1] == 0x30)
		&& (tmp_info[x + 2] == 0x64) && (tmp_info[x + 3] == 0x63))
		{
			play_yan.video_frames.push_back(play_yan.video_bytes.size());

			x += 4;
			u32 len = (tmp_info[x + 3] << 24) | (tmp_info[x + 2] << 16) | (tmp_info[x + 1] << 8) | tmp_info[x];

			x += 4;
			for(u32 y = 0; y < len; y++)
			{
				play_yan.video_bytes.push_back(tmp_info[x + y]);
			}

			x += (len - 1);
		}
	}

	if(play_yan.video_frames.empty())
	{
		play_yan.video_frames.resize(10000, 0xFFFFFFFF);
		std::cout<<"MMU::No video data found in " << filename << ". Check if file is valid AVI video.\n";
		return false;
	}

	u32 run_time = play_yan.video_frames.size() / 30;
	play_yan.video_length = play_yan.video_frames.size() * 33.3333;
	play_yan.video_current_fps = 1.0;

	std::cout<<"MMU::Loaded video file: " << filename << "\n";
	std::cout<<"MMU::Run Time: -> " << std::dec << (run_time / 60) << " : " << (run_time % 60) << std::hex << "\n";

	//Check for thumbnail. If none exists, make one for the user
	std::string thumb_file = util::get_filename_no_ext(filename) + ".bmp";
	SDL_Surface* thumb_pixels = SDL_LoadBMP(thumb_file.c_str());
	SDL_RWops* io_ops = SDL_AllocRW();

	if(thumb_pixels == NULL)
	{
		std::vector<u8> new_thumb;
		u32 mid = (play_yan.video_frames.size() / 2);
		u32 start = 0;
		u32 end = 0;
		
		if(play_yan.video_frames.size() >= 2)
		{
			start = play_yan.video_frames[mid];
			end = play_yan.video_frames[mid + 1];
		}

		else
		{
			end = play_yan.video_bytes.size();
		}

		for(u32 x = start; x < end; x++)
		{
			new_thumb.push_back(play_yan.video_bytes[x]);
		}

		io_ops = SDL_RWFromMem(new_thumb.data(), new_thumb.size());
		SDL_Surface* temp_surface = IMG_LoadTyped_RW(io_ops, 0, "JPG");
		SDL_Surface* final_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 60, 40, 32, 0, 0, 0, 0);

		SDL_Rect dest_rect;
		dest_rect.w = 60;
		dest_rect.h = 40;
		dest_rect.x = 0;
		dest_rect.y = 0;
		SDL_BlitScaled(temp_surface, NULL, final_surface, &dest_rect);
		SDL_SaveBMP(final_surface, thumb_file.c_str());

		SDL_FreeSurface(final_surface);
		SDL_FreeSurface(temp_surface);
	}

	SDL_FreeSurface(thumb_pixels);
	SDL_FreeRW(io_ops);

	//Search and parse each 01wb FOURCC - Audio Data
	for(u32 x = 0; x < (tmp_info.size() - 4); x++)
	{
		if((tmp_info[x] == 0x30) && (tmp_info[x + 1] == 0x31)
		&& (tmp_info[x + 2] == 0x77) && (tmp_info[x + 3] == 0x62))
		{
			x += 4;
			u32 len = (tmp_info[x + 3] << 24) | (tmp_info[x + 2] << 16) | (tmp_info[x + 1] << 8) | tmp_info[x];

			x += 4;
			for(u32 y = 0; y < len; y++)
			{
				mus_info.push_back(tmp_info[x + y]);
			}

			x += (len - 1);
		}
	}

	//Create .WAV file in memory, then have SDL load it for external audio
	if(play_yan.audio_sample_rate && play_yan.audio_channels)
	{
		std::vector<u8> mus_header;
		util::build_wav_header(mus_header, play_yan.audio_sample_rate, play_yan.audio_channels, mus_info.size());

		std::vector<u8> mus_file = mus_header;
		mus_file.insert(mus_file.end(), mus_info.begin(), mus_info.end());

		io_ops = SDL_AllocRW();
		io_ops = SDL_RWFromMem(mus_file.data(), mus_file.size());

		if(io_ops != NULL)
		{
			//Clear previous buffer if necessary
			SDL_FreeWAV(apu_stat->ext_audio.buffer);
			apu_stat->ext_audio.buffer = NULL;

			SDL_AudioSpec file_spec;

			if(SDL_LoadWAV_RW(io_ops, 0, &file_spec, &apu_stat->ext_audio.buffer, &apu_stat->ext_audio.length) == NULL)
			{
				std::cout<<"MMU::Play-Yan could not load audio from video : " << SDL_GetError() << "\n";
			}
		}

		SDL_FreeRW(io_ops);
	}

	return true;
	
	#endif

	play_yan.video_frames.resize(10000, 0xFFFFFFFF);

	return false;
}

/****** Loads a WAV file for SFX played by the Play-Yan ******/
bool AGB_MMU::play_yan_load_sfx(std::string filename)
{
	play_yan.sfx_data.clear();

	std::ifstream sfx_file(filename.c_str(), std::ios::binary);

	if(!sfx_file.is_open()) 
	{
		std::cout<<"MMU::SFX file " << filename << " could not be opened. Check file path or permissions. \n";
		return false;
	}

	sfx_file.seekg(0, sfx_file.end);
	u32 sfx_file_size = sfx_file.tellg();
	sfx_file.seekg(0, sfx_file.beg);
	play_yan.sfx_data.resize(sfx_file_size);

	sfx_file.read(reinterpret_cast<char*> (&play_yan.sfx_data[0]), sfx_file_size);
	sfx_file.close();

	return true;
}

/****** Verifies audio data (if any) from a video file ******/
void AGB_MMU::play_yan_check_audio_from_video(std::vector <u8> &data)
{
	u32 i = 0;

	//Search for auds FOURCC as start of usable data
	bool found_auds = false;

	for(u32 x = i; x < (data.size() - 4); x++)
	{
		if((data[x] == 0x61) && (data[x + 1] == 0x75)
		&& (data[x + 2] == 0x64) && (data[x + 3] == 0x73))
		{
			i = x + 4;
			found_auds = true;
		}
	}

	if(!found_auds) { return; }

	//Search for strf FOURCC as start of usable data
	bool found_strf = false;

	for(u32 x = i; x < (data.size() - 4); x++)
	{
		if((data[x] == 0x73) && (data[x + 1] == 0x74)
		&& (data[x + 2] == 0x72) && (data[x + 3] == 0x66))
		{
			i = x + 4;
			found_strf = true;
		}
	}

	if(!found_strf) { return; }

	//Make sure there is enough data to read - strf size (4) + strf data (16)
	if((i + 20) < data.size())
	{
		u8 audio_type = data[i + 4];

		//Verify audio type is PCM
		if(audio_type != 0x01) { return; }

		play_yan.audio_channels = data[i + 6];
		play_yan.audio_sample_rate = (data[i + 11] << 24) | (data[i + 10] << 16) | (data[i + 9] << 8) | data[i + 8];	
	}
}

/****** Grabs the data for a specific frame ******/
void AGB_MMU::play_yan_grab_frame_data(u32 frame)
{
	#ifdef GBE_IMAGE_FORMATS

	//Abort if invalid frame is being pulled from video
	if(frame >= play_yan.video_frames.size()) { return; }

	//Abort if dummy frames are present
	if(play_yan.video_frames[0] == 0xFFFFFFFF) { return; }

	u32 start = play_yan.video_frames[frame];
	u32 end = 0;

	//Check to see if this is the last frame
	//In that case, set end to be last byte of video data
	if((frame + 1) == play_yan.video_frames.size()) { end = play_yan.video_bytes.size(); }
	else { end = play_yan.video_frames[frame + 1]; }

	std::vector<u8> new_frame;

	for(u32 x = start; x < end; x++)
	{
		new_frame.push_back(play_yan.video_bytes[x]);
	}
	
	//Decode JPG data from memory into SDL_Surface
	SDL_RWops* io_ops = SDL_AllocRW();
	io_ops = SDL_RWFromMem(new_frame.data(), new_frame.size());

	SDL_Surface* temp_surface = IMG_LoadTyped_RW(io_ops, 0, "JPG");

	if(temp_surface != NULL)
	{
		//Copy and convert data into video frame buffer used by Play-Yan
		play_yan.video_data.clear();
		play_yan.video_data.resize(0x12C00, 0x00);

		u32 w = 240;
		u32 h = temp_surface->h;

		start = ((h - 160) / 2) * (w * 3);
		if(h > 160) { h = 160; }

		u8* pixel_data = (u8*)temp_surface->pixels;

		for(u32 index = 0, a = 0, i = start; a < (w * h); a++, i+=3)
		{
			u16 raw_pixel = 0;

			//Adjust (increase) brightness if necessary
			if(play_yan.video_brightness > 0x101)
			{
				u32 input_color = 0xFF000000 | (pixel_data[i] << 16) | (pixel_data[i+1] << 8) | pixel_data[i+2];
				double ratio = (play_yan.video_brightness - 0x100) / 512.0;

				util::hsl temp_color = util::rgb_to_hsl(input_color);
				temp_color.lightness += ratio;
				if(temp_color.lightness > 1.0) { temp_color.lightness = 1.0; }

				input_color = util::hsl_to_rgb(temp_color);
				u8 r = (input_color >> 16) & 0xFF;
				u8 g = (input_color >> 8) & 0xFF;
				u8 b = input_color & 0xFF;

				raw_pixel = ((b & 0xF8) << 7) | ((g & 0xF8) << 2) | ((r & 0xF8) >> 3);
			}

			else
			{
				raw_pixel = ((pixel_data[i+2] & 0xF8) << 7) | ((pixel_data[i+1] & 0xF8) << 2) | ((pixel_data[i] & 0xF8) >> 3);
			}

			play_yan.video_data[index++] = (raw_pixel & 0xFF);
			play_yan.video_data[index++] = ((raw_pixel >> 8) & 0xFF);
		}
	}

	else
	{
		std::cout<<"MMU::Warning - Could not decode video frame #" << std::dec << frame << std::hex << "\n";
	}

	SDL_FreeSurface(temp_surface);
	SDL_FreeRW(io_ops);

	#endif
}

/****** Verifies AVI header for videos ******/
void AGB_MMU::play_yan_check_video_header(std::string filename)
{
	//Set default video length of 10 seconds
	play_yan.irq_data[3] = 10000;

	#ifdef GBE_IMAGE_FORMATS

	std::vector<u8> header;
	std::ifstream vid_file(filename.c_str(), std::ios::binary);

	if(!vid_file.is_open()) 
	{
		std::cout<<"MMU::Warning - Could not read file header for " << filename << "\n";
		return;
	}

	//Only read the 1st 256 bytes of the file to find the info GBE+ needs
	vid_file.seekg(0, vid_file.end);
	u32 vid_file_size = vid_file.tellg();
	vid_file.seekg(0, vid_file.beg);

	if(vid_file_size < 0x100)
	{
		std::cout<<"MMU::Warning - Could not read file header for " << filename << "\n";
		return;
	}

	header.resize(0x100, 0x00);
	vid_file.read(reinterpret_cast<char*> (&header[0]), 0x100);
	vid_file.close();

	//Validate AVI header
	u32 id = (header[0x0B] << 24) | (header[0x0A] << 16) | (header[0x09] << 8) | (header[0x08]);

	if(id != 0x20495641)
	{
		std::cout<<"MMU::Warning - No valid AVI header for " << filename << "\n";
		return;
	}

	//Grab the number of frames and determine run-time
	u32 frame_count = (header[0x33] << 24) | (header[0x32] << 16) | (header[0x31] << 8) | (header[0x30]);
	play_yan.irq_data[3] = (frame_count * 33.3333);

	#endif
}

/****** Returns current headphone status ******/
bool AGB_MMU::play_yan_get_headphone_status()
{
	return apu_stat->ext_audio.use_headphones;
}
