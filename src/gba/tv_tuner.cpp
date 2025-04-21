// GB Enhanced+ Copyright Daniel Baxter 2024
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : tv_tuner.cpp
// Date : April 3, 2024
// Description : Agatsuma TV Tuner
//
// Handles I/O for the Agatsuma TV Tuner (ATVT)

#ifdef GBE_IMAGE_FORMATS
#include <SDL_image.h>
#endif

#include <ctime>
#include <filesystem>

#include "mmu.h"
#include "common/util.h"

/****** Resets ATVT data structure ******/
void AGB_MMU::tv_tuner_reset()
{
	tv_tuner.index = 0;
	tv_tuner.data = 0;
	tv_tuner.transfer_count = 0;
	tv_tuner.current_channel = 1;
	tv_tuner.next_channel = 0;
	tv_tuner.current_file = 0;
	tv_tuner.channel_freq = 91.25;
	tv_tuner.signal_delay = 0;
	tv_tuner.state = TV_TUNER_STOP_DATA;
	tv_tuner.data_stream.clear();
	tv_tuner.cmd_stream.clear();
	tv_tuner.video_stream.clear();
	tv_tuner.video_frames.clear();
	tv_tuner.video_bytes.clear();
	tv_tuner.channel_file_list.clear();
	tv_tuner.channel_runtime.clear();
	tv_tuner.read_request = false;
	tv_tuner.is_av_input_on = false;
	tv_tuner.is_av_input_changed = false;
	tv_tuner.is_av_connected = false;
	tv_tuner.is_channel_changed = false;
	tv_tuner.is_channel_scheduled = false;
	tv_tuner.is_scheduled_video_loaded = false;

	tv_tuner.video_brightness = 0;
	tv_tuner.video_contrast = 0;
	tv_tuner.video_hue = 0;

	tv_tuner.current_frame = 0;
	tv_tuner.start_ticks = SDL_GetTicks();
	tv_tuner.scheduled_start = TV_TUNER_MAX_SECS;
	tv_tuner.scheduled_end = TV_TUNER_MAX_SECS;

	tv_tuner.flash_cmd = 0;
	tv_tuner.flash_cmd_status = 0;
	tv_tuner.flash_data.clear();
	tv_tuner.flash_data.resize(0x100, 0xFF);

	tv_tuner.cnt_a = 0;
	tv_tuner.cnt_b = 0;
	tv_tuner.read_data = 0;

	//Only search for active TV channels if the ATVT is actually being emulated
	if(config::cart_type != AGB_TV_TUNER) { return; }

	u16 temp_channel_list[63] =
	{
		0x0890, 0x08F0, 0x0950, 0x0D90, 0x0DF0, 0x0E50, 0x0EB0, 0x0EF0, 0x0F50, 0x0FB0,
		0x1010, 0x1070, 0x2050, 0x20B0, 0x2110, 0x2170, 0x21D0, 0x2230, 0x2290, 0x22F0,
		0x2350, 0x23B0, 0x2410, 0x2470, 0x24D0, 0x2530, 0x2590, 0x25F0, 0x2650, 0x26B0,
		0x2710, 0x2770, 0x27D0, 0x2830, 0x2890, 0x28F0, 0x2950, 0x29B0, 0x2A10, 0x2A70,
		0x2AD0, 0x2B30, 0x2B90, 0x2BF0, 0x2C50, 0x2CB0, 0x2D10, 0x2D70, 0x2DD0, 0x2E30,
		0x2E90, 0x2EF0, 0x2F50, 0x2FB0, 0x3010, 0x3070, 0x30D0, 0x3130, 0x3190, 0x31F0,
		0x3250, 0x32B0, 0x0000,
	};

	for(u32 x = 0; x < 63; x++)
	{
		tv_tuner.is_channel_on[x] = false;
		tv_tuner.channel_id_list[x] = temp_channel_list[x];

		//Check for data/tv/XX, where XX is a folder representing video files for a given channel
		//Channel 63 is a special case in GBE+, points to data/tv/av for A/V input
		std::string channel_path = "";

		if(x == TV_TUNER_AV_STREAM) { channel_path = config::data_path + "tv/av/"; }
		else { channel_path = config::data_path + "tv/" + util::to_str(x + 1) + "/"; }

		std::filesystem::path fs_path { channel_path };

		if(std::filesystem::exists(fs_path) && std::filesystem::is_directory(fs_path))
		{
			std::vector<std::string> channel_files;
			util::get_files_in_dir(channel_path, ".avi", channel_files, true, true);
			util::get_files_in_dir(channel_path, ".AVI", channel_files, true, true);

			if(!channel_files.empty())
			{
				if(x != TV_TUNER_AV_STREAM)
				{
					tv_tuner.is_channel_on[x] = true;
					std::cout<<"MMU::TV Tuner Channel # " << std::dec << (x + 1) << std::hex << " is live\n";
				}

				else
				{
					tv_tuner.is_av_connected = true;
					std::cout<<"MMU::TV Tuner A/V Input is live\n";
				}
			}
		}
	}
}

/****** Writes to ATVT I/O ******/
void AGB_MMU::write_tv_tuner(u32 address, u8 value)
{
	//std::cout<<"TV TUNER WRITE -> 0x" << address << " :: 0x" << (u32)value << " :: " << (u32)tv_tuner.state <<"\n";

	switch(address)
	{
		case TV_CNT_A:
		case TV_CNT_B:
			tv_tuner.data_stream.push_back(value);
			tv_tuner.transfer_count++;

			//Reset transfer count
			if((value == 0x67) || (value == 0xE7))
			{
				tv_tuner.transfer_count = 0;
				tv_tuner.data_stream.clear();
			}

			//Start data transfer
			else if((tv_tuner.state == TV_TUNER_STOP_DATA) && (tv_tuner.transfer_count == 3))
			{
				if((tv_tuner.data_stream[1] & 0x03) == 0x03)
				{
					if(tv_tuner.data_stream[2] & 0x01)
					{
						tv_tuner.state = TV_TUNER_DELAY_DATA;
					}

					else
					{
						tv_tuner.state = TV_TUNER_START_DATA;
					}

					tv_tuner.data_stream.clear();
					tv_tuner.transfer_count = 0;
				}
			}

			//Delayed start of data transfer
			else if((tv_tuner.state == TV_TUNER_DELAY_DATA) && (tv_tuner.transfer_count == 1))
			{
				if((value & 0x1) == 0)
				{
					tv_tuner.state = TV_TUNER_START_DATA;
				}

				tv_tuner.data_stream.clear();
				tv_tuner.transfer_count = 0;
			}

			//Stop data transfer - After Write Request
			else if((tv_tuner.state == TV_TUNER_NEXT_DATA) && (tv_tuner.transfer_count == 3) && (!tv_tuner.read_request))
			{
				if((tv_tuner.data_stream[1] & 0x03) == 0x02)
				{
					tv_tuner.state = TV_TUNER_STOP_DATA;

					//Check for specific commands
					process_tv_tuner_cmd();

					tv_tuner.data_stream.clear();
					tv_tuner.cmd_stream.clear();
					tv_tuner.transfer_count = 0;
					tv_tuner.read_request = false;
				}
			}

			//Stop data transfer - After Read Request
			else if((tv_tuner.state == TV_TUNER_NEXT_DATA) && (tv_tuner.transfer_count == 4) && (tv_tuner.read_request))
			{
				if(((tv_tuner.data_stream[0] & 0x03) == 0x01) && ((tv_tuner.data_stream[1] & 0x03) == 0x00)
				&& ((tv_tuner.data_stream[2] & 0x03) == 0x02) && ((tv_tuner.data_stream[3] & 0x03) == 0x03))
				{
					tv_tuner.state = TV_TUNER_STOP_DATA;
					tv_tuner.data_stream.clear();
					tv_tuner.cmd_stream.clear();
					tv_tuner.transfer_count = 0;
					tv_tuner.read_request = false;
				}
			}

			//Grab 8-bit data from stream;
			else if(((tv_tuner.state == TV_TUNER_START_DATA) || (tv_tuner.state == TV_TUNER_NEXT_DATA))
			&& (tv_tuner.transfer_count == 32))
			{
				tv_tuner.state = TV_TUNER_ACK_DATA;
				tv_tuner.data_stream.clear();
				tv_tuner.transfer_count = 0;
				
				tv_tuner.data = 0;
				u8 mask = 0x80;

				for(u32 x = 0; x < 8; x++)
				{
					if(tv_tuner.data_stream[(x * 4) + 1] & 0x01)
					{
						tv_tuner.data |= mask;
					}

					mask >>= 1;
				}

				tv_tuner.cmd_stream.push_back(tv_tuner.data);
			}

			//Read data into stream
			if(tv_tuner.state == TV_TUNER_READ_DATA)
			{
				//TODO - Output bits every 3 transfers

				if(tv_tuner.transfer_count == 24)
				{
					tv_tuner.state = TV_TUNER_ACK_DATA;
					tv_tuner.data_stream.clear();
					tv_tuner.cmd_stream.clear();
					tv_tuner.transfer_count = 0;
					tv_tuner.cnt_a |= 0x40;
				}
			}

			//Wait for acknowledgement
			else if((tv_tuner.state == TV_TUNER_ACK_DATA) && (tv_tuner.transfer_count == 4))
			{
				tv_tuner.state = TV_TUNER_NEXT_DATA;
				tv_tuner.data_stream.clear();
				tv_tuner.transfer_count = 0;

				//Check for specific commands
				process_tv_tuner_cmd();
			}
			
			break;

		case TV_CNT_C:
			tv_tuner.cnt_a = value;
			break;

		case TV_CNT_D:
			tv_tuner.cnt_b = value;
			break;	
	}

	//Handle Flash commands and data
	if((address >= 0x8000000) && (address < 0x9000000))
	{
		switch(address)
		{
			case TV_FLASH_INIT:
				tv_tuner.flash_cmd = value;
				tv_tuner.flash_cmd_status = 0;
				tv_tuner.flash_cmd = 0;
				break;

			case TV_FLASH_CMD0:
				if(value == 0xAA)
				{
					tv_tuner.flash_cmd_status = 1;
				}
				
				else if(tv_tuner.flash_cmd_status == 2)
				{
					tv_tuner.flash_cmd = value;
					tv_tuner.flash_cmd_status = 0;
				}

				break;

			case TV_FLASH_CMD1:
				if(value == 0x55)
				{
					tv_tuner.flash_cmd_status++;
				}

				break;
		}

		//Flash data
		if((address >= 0x8020000) && (address < 0x8020100))
		{
			//Write bytes to Flash ROM
			if(tv_tuner.flash_cmd == 0xA0)
			{
				u32 index = (address - 0x8020000);
				tv_tuner.flash_data[index] = value;
			}

			//Erase command
			else if((address == 0x8020000) && (tv_tuner.flash_cmd == 0x80) && (value == 0x30))
			{
				tv_tuner.flash_data.clear();
				tv_tuner.flash_data.resize(0x100, 0xFF);
				tv_tuner.flash_cmd_status = 0;
			}
		}
	}
}

/****** Reads from ATVT I/O ******/
u8 AGB_MMU::read_tv_tuner(u32 address)
{
	u8 result = memory_map[address];

	switch(address)
	{
		case TV_CNT_A:
			if(tv_tuner.read_request)
			{
				result = (tv_tuner.read_data & 0x80) ? 1 : 0;
				tv_tuner.read_data <<= 1;
			}

			else
			{
				result = tv_tuner.cnt_a;
			}

			break;

		case TV_CNT_B:
			if(tv_tuner.read_request)
			{
				result = (tv_tuner.read_data & 0x80) ? 1 : 0;
				tv_tuner.read_data <<= 1;
			}

			else
			{
				result = tv_tuner.cnt_b;
			}

			break;
	}

	//Video data
	if((address >= 0xA800000) && (address < 0xA812C00))
	{
		u32 index = (address - 0xA800000);
		result = tv_tuner.video_stream[index];
	}

	//Flash data
	else if((address >= 0x8020000) && (address < 0x8020100))
	{
		u32 index = (address - 0x8020000);
		result = tv_tuner.flash_data[index];
	}
	
	else
	{
		//std::cout<<"TV TUNER READ -> 0x" << address << " :: 0x" << (u32)result << "\n";
	}

	return result;
}

/****** Handles ATVT commands ******/
void AGB_MMU::process_tv_tuner_cmd()
{
	//Change channel only if 0x87 command has not happened within 20 frames
	//Delays playing videos for TV channels until searching is complete
	if(tv_tuner.signal_delay)
	{
		tv_tuner.signal_delay--;

		if(!tv_tuner.signal_delay)
		{
			//Stop video playback
			apu_stat->ext_audio.playing = false;
			apu_stat->ext_audio.sample_pos = 0;
			apu_stat->ext_audio.volume = 0;
			tv_tuner.current_frame = 0;

			if(tv_tuner.is_channel_changed)
			{
				//Grab list of video files associated with this channel
				tv_tuner.current_channel = tv_tuner.next_channel;
				tv_tuner.channel_file_list.clear();

				std::cout<<"CHANNEL CHANGE -> " << std::dec << ((u32)tv_tuner.current_channel + 1) << std::hex << "\n";

				std::string channel_path = config::data_path + "tv/" + util::to_str(tv_tuner.current_channel + 1) + "/";
				util::get_files_in_dir(channel_path, ".avi", tv_tuner.channel_file_list, true, true);
				util::get_files_in_dir(channel_path, ".AVI", tv_tuner.channel_file_list, true, true);

				//Check if channel is scheduled, otherwise play live TV
				std::string channel_schedule = channel_path + "schedule.txt";
				std::filesystem::path schedule_path { channel_schedule };

				if(std::filesystem::exists(schedule_path) && !std::filesystem::is_directory(schedule_path))
				{
					tv_tuner.is_channel_scheduled = true;
					tv_tuner.is_scheduled_video_loaded = tv_tuner_play_schedule(channel_schedule);
				}

				else
				{
					tv_tuner.is_channel_scheduled = false;
					tv_tuner.is_channel_on[tv_tuner.current_channel] = tv_tuner_play_live();
				}
			}

			tv_tuner.is_channel_changed = false;
			tv_tuner.next_channel = 0;
		}
	}

	//D8 command -> Writes 16-bit values
	if((tv_tuner.cmd_stream.size() == 3) && (tv_tuner.cmd_stream[0] == 0xD8) && (tv_tuner.state == TV_TUNER_STOP_DATA))
	{
		u8 param_1 = tv_tuner.cmd_stream[1];
		u8 param_2 = tv_tuner.cmd_stream[2];

		//std::cout<<"CMD 0xD8 -> 0x" << (u32)param_1 << " :: 0x" << (u32)param_2 << "\n";

		//Render video frame
		if((param_1 == 0x0D) && (param_2 == 0x00))
		{
			tv_tuner_render_frame();
		}

		//Switch between TV channels and AV input
		else if(param_1 == 0x02)
		{
			tv_tuner.is_av_input_on = (param_2 == 0xE1) ? true : false;
			tv_tuner.is_av_input_changed = ~tv_tuner.is_av_input_on;

			if((tv_tuner.is_av_input_on) && (!tv_tuner.is_channel_on[TV_TUNER_AV_STREAM]))
			{
				//Stop video playback
				apu_stat->ext_audio.playing = false;
				apu_stat->ext_audio.sample_pos = 0;
				apu_stat->ext_audio.volume = 0;
				tv_tuner.current_frame = 0;

				tv_tuner.is_channel_on[TV_TUNER_AV_STREAM] = true;
				tv_tuner.is_channel_scheduled = false;

				u32 temp_channel = tv_tuner.current_channel;
				tv_tuner.current_channel = TV_TUNER_AV_STREAM;

				tv_tuner.channel_file_list.clear();				
				std::string channel_path = config::data_path + "tv/av/";
				util::get_files_in_dir(channel_path, ".avi", tv_tuner.channel_file_list, true, true);
				util::get_files_in_dir(channel_path, ".AVI", tv_tuner.channel_file_list, true, true);

				tv_tuner_play_live();
				tv_tuner.current_channel = temp_channel;				

				std::cout<<"AV INPUT ON\n";
			}

			else
			{
				tv_tuner.is_channel_on[TV_TUNER_AV_STREAM] = false;
				std::cout<<"TV INPUT ON\n";
			}
		}

		//Various TV image manipulation commands
		else if(param_1 == TV_TUNER_SET_CONTRAST)
		{
			if(param_2 & 0x80)
			{
				param_2--;
				param_2 = ~param_2;
				tv_tuner.video_contrast = -param_2;
			}

			else
			{
				tv_tuner.video_contrast = param_2;
			}

			std::cout<<"VIDEO CONTRAST -> " << std::dec << (s32)tv_tuner.video_contrast << std::hex << "\n";
		}

		else if(param_1 == TV_TUNER_SET_BRIGHTNESS)
		{
			if(param_2 & 0x80)
			{
				param_2--;
				param_2 = ~param_2;
				tv_tuner.video_brightness = -param_2;
			}

			else
			{
				tv_tuner.video_brightness = param_2;
			}

			std::cout<<"VIDEO BRIGHTNESS -> " << std::dec << (s32)tv_tuner.video_brightness << std::hex << "\n";
		}

		else if(param_1 == TV_TUNER_SET_HUE)
		{
			if(param_2 & 0x80)
			{
				param_2--;
				param_2 = ~param_2;
				tv_tuner.video_hue = -param_2;
			}

			else
			{
				tv_tuner.video_hue = param_2;
			}

			std::cout<<"VIDEO HUE -> " << std::dec << (s32)tv_tuner.video_hue << std::hex << "\n";
		}

	}

	//D8 command -> Writes 8-bit values
	else if((tv_tuner.cmd_stream.size() == 2) && (tv_tuner.cmd_stream[0] == 0xD8) && (tv_tuner.state == TV_TUNER_STOP_DATA))
	{
		u8 param_1 = tv_tuner.cmd_stream[1];

		//std::cout<<"CMD 0xD8 -> 0x" << (u32)param_1 << "\n";
	}

	//D9 command -> Reads a single 8-bit value
	else if((tv_tuner.cmd_stream.size() == 1) && (tv_tuner.cmd_stream[0] == 0xD9) && (tv_tuner.state == TV_TUNER_NEXT_DATA))
	{
		tv_tuner.state = TV_TUNER_READ_DATA;
		tv_tuner.read_request = true;
		tv_tuner.read_data = 0;

		//std::cout<<"CMD 0xD9\n";
	}

	//87 command -> Reads a single 8-bit value
	//Appears to indicate if a channel is active. Used during the Channel Search feature
	else if((tv_tuner.cmd_stream.size() == 1) && (tv_tuner.cmd_stream[0] == 0x87) && (tv_tuner.state == TV_TUNER_NEXT_DATA))
	{
		tv_tuner.state = TV_TUNER_READ_DATA;
		tv_tuner.read_request = true;

		if(tv_tuner.is_channel_on[tv_tuner.next_channel])
		{
			tv_tuner.read_data = 0xC0;
		}

		std::cout<<"CHANNEL PROBE -> " << std::dec << ((u32)tv_tuner.next_channel + 1) << std::hex << "\n";

		//Extend delay for scheduled channel changes when probing active channels
		if(tv_tuner.signal_delay) { tv_tuner.signal_delay = 60; }

		//Stop video playback
		apu_stat->ext_audio.playing = false;
		apu_stat->ext_audio.sample_pos = 0;
		apu_stat->ext_audio.volume = 0;
		tv_tuner.current_frame = 0;
	}

	//C0 Command -> Reads a 16-bit value
	else if((tv_tuner.cmd_stream.size() == 3) && (tv_tuner.cmd_stream[0] == 0xC0))
	{
		u8 param_1 = tv_tuner.cmd_stream[1];
		u8 param_2 = tv_tuner.cmd_stream[2];

		//Get 16-bit channel ID - Used to change channel
		if(param_1 != 0x86)
		{
			u16 raw_freq = ((param_1 << 8) | param_2);

			for(u32 x = 0; x < 62; x++)
			{
				u16 freq_start = tv_tuner.channel_id_list[x];
				u16 freq_end = (x == 61) ? 0xFFFF : tv_tuner.channel_id_list[x + 1];

				//Detect channel change within the total frequency range for a given channel
				if((raw_freq >= freq_start) && (raw_freq < freq_end))
				{
					u8 new_channel = x;
					tv_tuner.next_channel = new_channel;

					if((new_channel != tv_tuner.current_channel) || (tv_tuner.is_av_input_changed))
					{
						tv_tuner.is_channel_changed = true;
						tv_tuner.signal_delay = 20;
					}

					break;
				}
			}

			//Calculate current frequency based on distance from lowest possible frequency
			//Minumum = 55.25MHz, hex value 0x650
			tv_tuner.channel_freq = (float(raw_freq - 0x650) * 0.0625) + 55.25;
			std::cout<<"CHANNEL FREQUENCY -> " << tv_tuner.channel_freq << "\n";
			std::cout<<"CHANNEL FREQUENCY RAW -> " << raw_freq << "\n\n";
		}

		else { std::cout<<"UKNOWN CMD -> 0x86 :: 0x" << (u32)param_1 << " :: 0x" << (u32)param_2 << "\n"; }
	}

	//Unknown command
	else if(tv_tuner.state == TV_TUNER_STOP_DATA)
	{
		std::cout<<"UNKOWN CMD -> ";
		
		for(u32 x = 0; x < tv_tuner.cmd_stream.size(); x++)
		{
			std::cout<<"0x" << (u32)tv_tuner.cmd_stream[x] << " :: ";
		}

		std::cout<<"\n";
	}
}

/****** Renders video data for a frame for the AVTV ******/
void AGB_MMU::tv_tuner_render_frame()
{
	tv_tuner.video_stream.clear();

	//Render composite A/V input, if connected
	if(tv_tuner.is_av_input_on)
	{
		if(tv_tuner.is_av_connected)
		{
			//Forcibly sync audio periodically
			if(config::force_cart_audio_sync)
			{
				apu_stat->ext_audio.sample_pos = ((1/30.0 * tv_tuner.current_frame) * apu_stat->ext_audio.frequency);
			}

			bool render_result = tv_tuner_grab_frame_data(tv_tuner.current_frame);
			tv_tuner.current_frame++;

			//Move onto next video in A/V stream
			if((tv_tuner.current_frame >= tv_tuner.video_frames.size()) && !tv_tuner.is_channel_scheduled)
			{
				u32 next_video = tv_tuner.current_file + 1;
				tv_tuner.current_frame -= tv_tuner.video_frames.size();
			
				if(next_video < tv_tuner.channel_file_list.size())
				{
					tv_tuner.current_file++;
					tv_tuner_load_video(tv_tuner.channel_file_list[tv_tuner.current_file]);

					apu_stat->ext_audio.playing = true;
					apu_stat->ext_audio.volume = g_pad->ext_volume;
					apu_stat->ext_audio.sample_pos = ((1/30.0 * tv_tuner.current_frame) * apu_stat->ext_audio.frequency); 
				}

				//End of A/V stream
				else
				{
					tv_tuner.video_frames.clear();
					tv_tuner.is_av_connected = false;
				}
			}
		}

		else
		{
			tv_tuner.video_stream.resize(0x12C00, 0x00);
		}
	}

	//Render TV channel video
	else if(tv_tuner.is_channel_on[tv_tuner.current_channel])
	{
		//Forcibly sync audio periodically
		if(config::force_cart_audio_sync)
		{
			apu_stat->ext_audio.sample_pos = ((1/30.0 * tv_tuner.current_frame) * apu_stat->ext_audio.frequency);
		}

		bool render_result = tv_tuner_grab_frame_data(tv_tuner.current_frame);
		tv_tuner.current_frame++;

		//Move onto next video if no schedule file exists
		if((tv_tuner.current_frame >= tv_tuner.video_frames.size()) && !tv_tuner.is_channel_scheduled)
		{
			u32 next_video = tv_tuner.current_file + 1;
			tv_tuner.current_frame -= tv_tuner.video_frames.size();
			
			if(next_video < tv_tuner.channel_file_list.size())
			{
				tv_tuner.current_file++;
				tv_tuner_load_video(tv_tuner.channel_file_list[tv_tuner.current_file]);

				apu_stat->ext_audio.playing = true;
				apu_stat->ext_audio.volume = g_pad->ext_volume;
				apu_stat->ext_audio.sample_pos = ((1/30.0 * tv_tuner.current_frame) * apu_stat->ext_audio.frequency); 
			}

			//End of unscheduled stream, so disable channel signal
			else
			{
				tv_tuner.video_frames.clear();
				tv_tuner.is_channel_on[tv_tuner.current_channel] = false;
			}
		}

		//Signal end of scheduled video
		else if((tv_tuner.current_frame >= tv_tuner.video_frames.size()) && tv_tuner.is_channel_scheduled)
		{
			tv_tuner.is_scheduled_video_loaded = false;
			tv_tuner.video_frames.clear();
			tv_tuner.is_channel_on[tv_tuner.current_channel] = false;
		}
	}

	//Render static noise (grayscale) if channel has no signal
	else if(!tv_tuner.is_channel_on[tv_tuner.current_channel])
	{
		srand(SDL_GetTicks());
		u8 num = 0;
		u16 color = 0;
		u8 bit = 0;

		for(u32 x = 0; x < 0x12C00; x += 2)
		{
			num = (rand() % 0x1F); 

			u16 color = (num << 10) | (num << 5) | num;
			
			tv_tuner.video_stream.push_back(color & 0xFF);
			tv_tuner.video_stream.push_back(color >> 8);
		}
	}

	//Check for start of scheduled video
	if(tv_tuner.is_channel_scheduled)
	{
		u32 seconds_now = tv_tuner_get_seconds();
		u32 start_time = tv_tuner.scheduled_start;
		u32 end_time = tv_tuner.scheduled_end;
		u32 test_time = end_time - TV_TUNER_MAX_SECS;
		bool is_video_playing = false;

		//Check to see if video starts and finishes before midnight
		if((end_time < TV_TUNER_MAX_SECS) && (seconds_now >= start_time) && (start_time < end_time))
		{
			is_video_playing = true;
		}

		//Check to see if video started before midnight but ends on the next day
		else if((end_time >= TV_TUNER_MAX_SECS) && ((seconds_now >= start_time) || (seconds_now < test_time)))
		{
			is_video_playing = true;
		}

		if(is_video_playing && !tv_tuner.is_scheduled_video_loaded)
		{
			std::string channel_path = config::data_path + "tv/" + util::to_str(tv_tuner.current_channel + 1) + "/";
			std::string channel_schedule = channel_path + "schedule.txt";
			tv_tuner.is_channel_on[tv_tuner.current_channel] = tv_tuner_play_schedule(channel_schedule);
			tv_tuner.is_scheduled_video_loaded = tv_tuner.is_channel_on[tv_tuner.current_channel];
		}
	}

	//Update volume (based on context input from the Game Pad)
	if(apu_stat->ext_audio.playing)
	{
		apu_stat->ext_audio.volume = g_pad->ext_volume;
	}
}

/****** Loads video that's already been converted - MJPEG video, 16-bit PCM-LE ******/
bool AGB_MMU::tv_tuner_load_video(std::string filename)
{
	#ifdef GBE_IMAGE_FORMATS

	apu_stat->ext_audio.channels = 0;
	apu_stat->ext_audio.frequency = 0;

	tv_tuner.video_bytes.clear();
	tv_tuner.video_frames.clear();

	std::vector<u8> vid_info;
	std::vector<u8> mus_info;
	std::vector<u8> tmp_info;
	std::ifstream vid_file(filename.c_str(), std::ios::binary);

	if(!vid_file.is_open()) 
	{
		tv_tuner.video_frames.resize(10000, 0xFFFFFFFF);
		std::cout<<"MMU::" << filename << " could not be opened. Check file path or permissions. \n";
		return false;
	}

	vid_file.seekg(0, vid_file.end);
	u32 vid_file_size = vid_file.tellg();
	vid_file.seekg(0, vid_file.beg);
	vid_info.resize(vid_file_size);

	vid_file.read(reinterpret_cast<char*> (&vid_info[0]), vid_file_size);
	vid_file.close();

	if(vid_file_size < 4) 
	{
		tv_tuner.video_frames.resize(10000, 0xFFFFFFFF);
		std::cout<<"MMU::" << filename << " file size is too small. \n";
		return false;
	}

	//Verify audio data format separately
	tv_tuner_check_audio_from_video(vid_info);

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
		tv_tuner.video_frames.resize(10000, 0xFFFFFFFF);
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
			tv_tuner.video_frames.push_back(tv_tuner.video_bytes.size());

			x += 4;
			u32 len = (tmp_info[x + 3] << 24) | (tmp_info[x + 2] << 16) | (tmp_info[x + 1] << 8) | tmp_info[x];

			x += 4;
			for(u32 y = 0; y < len; y++)
			{
				tv_tuner.video_bytes.push_back(tmp_info[x + y]);
			}

			x += (len - 1);
		}
	}

	if(tv_tuner.video_frames.empty())
	{
		tv_tuner.video_frames.resize(10000, 0xFFFFFFFF);
		std::cout<<"MMU::No video data found in " << filename << ". Check if file is valid AVI video.\n";
		return false;
	}

	u32 run_time = tv_tuner.video_frames.size() / 30;

	std::cout<<"MMU::Loaded video file: " << filename << "\n";
	std::cout<<"MMU::Run Time: -> " << std::dec << (run_time / 60) << " : " << (run_time % 60) << std::hex << "\n";

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
	if(apu_stat->ext_audio.frequency && apu_stat->ext_audio.channels)
	{
		std::vector<u8> mus_header;
		util::build_wav_header(mus_header, apu_stat->ext_audio.frequency, apu_stat->ext_audio.channels, mus_info.size());

		std::vector<u8> mus_file = mus_header;
		mus_file.insert(mus_file.end(), mus_info.begin(), mus_info.end());

		SDL_RWops* io_ops = SDL_AllocRW();
		io_ops = SDL_RWFromMem(mus_file.data(), mus_file.size());

		if(io_ops != NULL)
		{
			//Clear previous buffer if necessary
			SDL_FreeWAV(apu_stat->ext_audio.buffer);
			apu_stat->ext_audio.buffer = NULL;

			SDL_AudioSpec file_spec;

			if(SDL_LoadWAV_RW(io_ops, 0, &file_spec, &apu_stat->ext_audio.buffer, &apu_stat->ext_audio.length) == NULL)
			{
				std::cout<<"MMU::TV Tuner could not load audio from video : " << SDL_GetError() << "\n";
			}
		}

		SDL_FreeRW(io_ops);
	}

	return true;
	
	#endif

	tv_tuner.video_frames.resize(10000, 0xFFFFFFFF);

	return false;
}

/****** Verifies audio data (if any) from a video file ******/
void AGB_MMU::tv_tuner_check_audio_from_video(std::vector <u8> &data)
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

		apu_stat->ext_audio.channels = data[i + 6];
		apu_stat->ext_audio.frequency = (data[i + 11] << 24) | (data[i + 10] << 16) | (data[i + 9] << 8) | data[i + 8];	
	}
}

/****** Grabs the data for a specific frame ******/
bool AGB_MMU::tv_tuner_grab_frame_data(u32 frame)
{
	#ifdef GBE_IMAGE_FORMATS

	//Abort if invalid frame is being pulled from video
	if(frame >= tv_tuner.video_frames.size()) { return false; }

	//Abort if dummy frames are present
	if(tv_tuner.video_frames[0] == 0xFFFFFFFF) { return false; }

	u32 start = tv_tuner.video_frames[frame];
	u32 end = 0;

	//Check to see if this is the last frame
	//In that case, set end to be last byte of video data
	if((frame + 1) == tv_tuner.video_frames.size()) { end = tv_tuner.video_bytes.size(); }
	else { end = tv_tuner.video_frames[frame + 1]; }

	std::vector<u8> new_frame;

	for(u32 x = start; x < end; x++)
	{
		new_frame.push_back(tv_tuner.video_bytes[x]);
	}
	
	//Decode JPG data from memory into SDL_Surface
	SDL_RWops* io_ops = SDL_AllocRW();
	io_ops = SDL_RWFromMem(new_frame.data(), new_frame.size());

	SDL_Surface* temp_surface = IMG_LoadTyped_RW(io_ops, 0, "JPG");

	if(temp_surface != NULL)
	{
		//Calculate ratios used to change Brightness and Hue. Contrast value can be used as is.
		//Note that "Hue" is translated from the ATVT menu. It actually changes saturation!
		float bright_ratio = 0.0;
		u8 bright_max = (tv_tuner.video_brightness > 0) ? 127 : 128;
		if(tv_tuner.video_brightness) { bright_ratio = (0.5 / bright_max) * tv_tuner.video_brightness; }

		float hue_ratio = 0.0;
		u8 hue_max = (tv_tuner.video_hue > 0) ? 127 : 128;
		if(tv_tuner.video_hue) { hue_ratio = (0.5 / hue_max) * tv_tuner.video_hue; }

		//Copy and convert data into video frame buffer used by ATVT
		tv_tuner.video_stream.clear();
		tv_tuner.video_stream.resize(0x12C00, 0x00);

		u32 w = 240;
		u32 h = 160;

		u8* pixel_data = (u8*)temp_surface->pixels;

		for(u32 index = 0, a = 0, i = 0; a < (w * h); a++, i += 3)
		{
			u8 r = (pixel_data[i] >> 3);
			u8 g = (pixel_data[i + 1] >> 3);
			u8 b = (pixel_data[i + 2] >> 3);

			u16 raw_pixel = 0;
			u32 input_color = 0xFF000000 | (pixel_data[i] << 16) | (pixel_data[i + 1] << 8) | pixel_data[i + 2];

			if(tv_tuner.video_brightness)
			{
				util::hsl temp_color = util::rgb_to_hsl(input_color);
				temp_color.lightness += bright_ratio;

				if(temp_color.lightness > 1.0) { temp_color.lightness = 1.0; }
				if(temp_color.lightness < 0.0) { temp_color.lightness = 0.0; }

				input_color = util::hsl_to_rgb(temp_color);

				r = (input_color >> 19) & 0x1F;
				g = (input_color >> 11) & 0x1F;
				b = (input_color >> 3) & 0x1F;
			}

			if(tv_tuner.video_hue)
			{
				util::hsl temp_color = util::rgb_to_hsl(input_color);
				temp_color.saturation += hue_ratio;

				if(temp_color.saturation > 1.0) { temp_color.saturation = 1.0; }
				if(temp_color.saturation < 0.0) { temp_color.saturation = 0.0; }

				input_color = util::hsl_to_rgb(temp_color);

				r = (input_color >> 19) & 0x1F;
				g = (input_color >> 11) & 0x1F;
				b = (input_color >> 3) & 0x1F;
			}

			if(tv_tuner.video_contrast)
			{
				input_color = util::adjust_contrast(input_color, tv_tuner.video_contrast);

				r = (input_color >> 19) & 0x1F;
				g = (input_color >> 11) & 0x1F;
				b = (input_color >> 3) & 0x1F;
			}

			raw_pixel = ((b << 10) | (g << 5) | r);

			tv_tuner.video_stream[index++] = (raw_pixel & 0xFF);
			tv_tuner.video_stream[index++] = ((raw_pixel >> 8) & 0xFF);
		}
	}

	else
	{
		std::cout<<"MMU::Warning - Could not decode video frame #" << std::dec << frame << std::hex << "\n";
		return false;
	}

	SDL_FreeSurface(temp_surface);
	SDL_FreeRW(io_ops);

	#endif

	return true;
}

/****** Returns the total length of a video (in seconds) ******/
u32 AGB_MMU::tv_tuner_get_video_length(std::string filename)
{
	std::ifstream vid_file(filename.c_str(), std::ios::binary);
	if(!vid_file.is_open()) { return 0; }

	std::filesystem::path fs_path { filename };
	u32 vid_file_size = std::filesystem::file_size(fs_path);

	//This is a quick 'n' dirty way of reading standard AVI header for total frames
	//Only the first 52 bytes are needed here
	//Total Frame offset = 48th byte
	if(vid_file_size < 52) { return 0; }

	std::vector<u8> temp_data;
	temp_data.resize(52);

	vid_file.read(reinterpret_cast<char*> (&temp_data[0]), 52);
	vid_file.close();

	u32 total_frames = (temp_data[51] << 24) | (temp_data[50] << 16) | (temp_data[49] << 8) | temp_data[48];
	u32 result = total_frames / 30;
	
	if(total_frames % 30) { result++; }

	return result;
}

/****** Reads TV schedule file for a given channel and plays the video ******/
bool AGB_MMU::tv_tuner_play_schedule(std::string filename)
{
	std::string input_line = "";
	std::ifstream file(filename.c_str(), std::ios::in);

	if(!file.is_open())
	{
		std::cout<<"MMU::Error - Could not open TV schedule " << filename << "\n";
		return false;
	}

	bool file_found = false;
	bool video_playing = false;

	tv_tuner.scheduled_start = TV_TUNER_MAX_SECS;
	tv_tuner.scheduled_end = TV_TUNER_MAX_SECS;
	std::vector<u32> time_list;
	std::vector<u32> end_list;

	//Parse line for filename and start time. Data is separated by a colon
	while(getline(file, input_line))
	{
		if(!input_line.empty())
		{
			std::size_t parse_symbol;
			s32 pos = 0;

			std::string out_file = "";
			std::string out_time = "";

			bool end_of_string = true;

			//Grab filename
			parse_symbol = input_line.find(":", pos);
			
			if(parse_symbol != std::string::npos)
			{
				out_file = input_line.substr(pos, parse_symbol);
				pos += (parse_symbol + 1);
				end_of_string = false;
			}

			//Grab start time
			if((pos < input_line.length()) && (!end_of_string))
			{
				out_time = input_line.substr(pos);
			}

			//Parse start time for given video file
			if(!out_file.empty() && !out_time.empty())
			{
				for(u32 x = 0; x < tv_tuner.channel_file_list.size(); x++)
				{
					if(out_file == util::get_filename_from_path(tv_tuner.channel_file_list[x]))
					{
						file_found = true;

						u32 start_time = 0;
						util::from_str(out_time, start_time);
						u32 end_time = start_time + tv_tuner_get_video_length(tv_tuner.channel_file_list[x]);
						u32 test_time = end_time - TV_TUNER_MAX_SECS;

						time_list.push_back(start_time);
						end_list.push_back(end_time);

						if(!video_playing)
						{
							u32 seconds_now = tv_tuner_get_seconds();

							bool is_valid_time = false;

							//Check to see if video started and finishes before midnight
							if((end_time < TV_TUNER_MAX_SECS) && (seconds_now >= start_time) && (seconds_now < end_time))
							{
								is_valid_time = true;
							}

							//Check to see if video started before midnight but ends on the next day
							else if((end_time >= TV_TUNER_MAX_SECS) && ((seconds_now >= start_time) || (seconds_now < test_time)))
							{
								is_valid_time = true;
								if(seconds_now < test_time) { seconds_now += TV_TUNER_MAX_SECS; }
							}

							if(is_valid_time)
							{
								video_playing = true;
								
								//Load new video and start playback
								if(tv_tuner_load_video(tv_tuner.channel_file_list[x]))
								{
									apu_stat->ext_audio.playing = true;
									apu_stat->ext_audio.volume = g_pad->ext_volume;
								}

								tv_tuner.current_frame = ((seconds_now - start_time) * 30);
								apu_stat->ext_audio.sample_pos = ((1/30.0 * tv_tuner.current_frame) * apu_stat->ext_audio.frequency); 
							}
						}
					}
				}
			}
		}
	}

	//Find next scheduled video start time
	for(u32 x = 0; x < time_list.size(); x++)
	{
		u32 seconds_now = tv_tuner_get_seconds();
		
		if((seconds_now < time_list[x]) && (tv_tuner.scheduled_start > time_list[x]))
		{
			tv_tuner.scheduled_start = time_list[x];
			tv_tuner.scheduled_end = end_list[x];
		}
	}

	//Report failure if schedule exists, but no entries matched files for this channel
	if(!file_found)
	{
		std::cout<<"MMU::No scheduled files found in " << filename << "\n";
		tv_tuner.is_channel_on[tv_tuner.current_channel] = false;
		return false;
	}

	//Report failure if schedule exists, entries found, but nothing playing atm
	else if(!video_playing)
	{
		std::cout<<"MMU::No scheduled files are playing for " << filename << "\n";
		tv_tuner.is_channel_on[tv_tuner.current_channel] = false;
		return false;
	}

	std::cout<<"MMU::Reading Channel Schedule File  " << filename << "\n";
	tv_tuner.is_channel_on[tv_tuner.current_channel] = true;
	return true;
}

/****** Get current time of day in seconds ******/
u32 AGB_MMU::tv_tuner_get_seconds()
{
	time_t system_time = time(0);
	tm* current_time = localtime(&system_time);
	return (current_time->tm_hour * 3600) + (current_time->tm_min * 60) + (current_time->tm_sec);
}

/****** Plays "live" or unscheduled TV, starts as soon as GBE+ boots ******/
bool AGB_MMU::tv_tuner_play_live()
{
	//Calculate playback position based on ticks since boot + channel loop start time
	//Mirrors live TV broadcasts
	u32 global_ticks = ((SDL_GetTicks() - tv_tuner.start_ticks) / 1000);
	u32 local_ticks = 0;

	//Get total channel video length
	tv_tuner.channel_runtime.clear();
	tv_tuner.current_file = 0;

	bool file_found = false;

	for(u32 x = 0; x < tv_tuner.channel_file_list.size(); x++)
	{
		std::string tv_file = tv_tuner.channel_file_list[x];
		u32 start_time = (!x) ? 0 : tv_tuner.channel_runtime[x - 1];
		u32 end_time = start_time + tv_tuner_get_video_length(tv_file);
		tv_tuner.channel_runtime.push_back(end_time);

		//Find out which video is playing based on current ticks
		if((global_ticks >= start_time) && (global_ticks < end_time))
		{
			tv_tuner.current_file = x;
			local_ticks = start_time;
			file_found = true;
		}
	}

	//Abort loading video if no unscheduled video is currently playing
	if(!file_found)
	{
		std::cout<<"No unscheduled files are playing for Channel " << std::dec << u32(tv_tuner.current_channel + 1) << std::hex << "\n";
		return false;
	}

	//Load new video and restart playback
	if(!tv_tuner.channel_file_list.empty() && tv_tuner_load_video(tv_tuner.channel_file_list[tv_tuner.current_file]))
	{
		apu_stat->ext_audio.playing = true;
		apu_stat->ext_audio.volume = g_pad->ext_volume;
	}

	tv_tuner.current_frame = ((global_ticks - local_ticks) * 30);
	apu_stat->ext_audio.sample_pos = ((1/30.0 * tv_tuner.current_frame) * apu_stat->ext_audio.frequency);

	return true;
}
