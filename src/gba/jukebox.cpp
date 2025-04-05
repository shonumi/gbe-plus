// GB Enhanced+ Copyright Daniel Baxter 2022
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : jukebox.cpp
// Date : May 22, 2022
// Description : Kemco Music Recorder / Radica Games Jukebox
//
// Handles I/O for the Jukebox
// Manages Jukebox metadata files and recording status

#include <SDL_audio.h>
#include "mmu.h"
#include "common/util.h"

/****** Reset Jukebox data structure ******/
void AGB_MMU::jukebox_reset()
{
	jukebox.io_regs.clear();
	jukebox.io_regs.resize(0x200, 0x00);
	jukebox.io_regs[0x0100] = 10;
	jukebox.io_regs[0x009A] = 1;
	jukebox.io_index = 0;
	jukebox.status = 0;
	jukebox.config = 0;
	jukebox.out_hi = 0;
	jukebox.out_lo = 0;
	jukebox.current_category = 0;
	jukebox.current_file = 0;
	jukebox.current_frame = 0;
	jukebox.last_music_file = 0;
	jukebox.last_voice_file = 0;
	jukebox.last_karaoke_file = 0;
	jukebox.progress = 0;
	jukebox.format_compact_flash = false;
	jukebox.is_recording = false;
	jukebox.enable_karaoke = false;
	jukebox.remaining_recording_time = 0;
	jukebox.remaining_playback_time = 0;
	jukebox.current_recording_time = 0;
	jukebox.recorded_file = "";
	jukebox.last_converted_file = "";
	jukebox.karaoke_track = "";

	for(u32 x = 0; x < 9; x++) { jukebox.spectrum_values[x] = 0; }

	//Read Jukebox data files
	if(config::cart_type == AGB_JUKEBOX)
	{
		read_jukebox_file_list((config::data_path + "jukebox/music.txt"), 0);
		read_jukebox_file_list((config::data_path + "jukebox/voice.txt"), 1);
		read_jukebox_file_list((config::data_path + "jukebox/karaoke.txt"), 2);

		//Set Jukebox I/O flags if files detected
		if(!jukebox.music_files.empty()) { jukebox.io_regs[0xAD] = jukebox.music_files.size(); }
		if(!jukebox.voice_files.empty()) { jukebox.io_regs[0xAE] = jukebox.voice_files.size(); }
		if(!jukebox.karaoke_files.empty()) { jukebox.io_regs[0xAF] = jukebox.karaoke_files.size(); }
	}

	//Properly calculate remaining recording time based on Jukebox data files
	u32 data_file_time = 0;
	
	for(u32 x = 0; x < jukebox.music_times.size(); x++) { data_file_time += jukebox.music_times[x]; }
	for(u32 x = 0; x < jukebox.voice_times.size(); x++) { data_file_time += jukebox.voice_times[x]; }
	for(u32 x = 0; x < jukebox.karaoke_times.size(); x++) { data_file_time += jukebox.karaoke_times[x]; }

	if(data_file_time >= config::jukebox_total_time)
	{
		jukebox.remaining_recording_time = 0;
	}

	else
	{
		jukebox.remaining_recording_time = config::jukebox_total_time - data_file_time;
	}
}

/****** Writes data to GBA Jukebox I/O ******/
void AGB_MMU::write_jukebox(u32 address, u8 value)
{
	bool process_data = false;

	switch(address)
	{
		//Index High
		case JB_REG_08:
			jukebox.io_index &= 0x00FF;
			jukebox.io_index |= (value << 8);
			break;

		//Index Low
		case JB_REG_0A:
			jukebox.io_index &= 0xFF00;
			jukebox.io_index |= value;
			process_data = true;
			break;

		//Write IO Register High
		case JB_REG_0C:
			//Write to Indices
			switch(jukebox.io_index)
			{
				//Status Register
				case 0x0080:
					jukebox.status &= 0x00FF;
					jukebox.status |= (value << 8);
					break;

				//Misc Audio Registers
				case 0x0088:
				case 0x008A:
				case 0x008C:
				case 0x008F:
				case 0x009A:
				case 0x009B:
					jukebox.io_regs[jukebox.io_index] &= 0x00FF;
					jukebox.io_regs[jukebox.io_index] |= (value << 8);
					break;

				//Config Register
				case 0x01C8:
					jukebox.config &= 0x00FF;
					jukebox.config |= (value << 8);
					break;
			}

			//Music File metadata
			if((jukebox.io_index >= 0xB0) && (jukebox.io_index <= 0xCD))
			{
				jukebox.io_regs[jukebox.io_index] &= 0x00FF;
				jukebox.io_regs[jukebox.io_index] |= (value << 8);
			}

			break;

		//Write IO Register Low
		case JB_REG_0E:
			//Status Register - Send Command
			if(jukebox.io_index == 0x0080)
			{
				jukebox.status &= 0xFF00;
				jukebox.status |= value;

				bool restore = false;
				bool was_recording = jukebox.is_recording;

				//Process various commands now
				switch(jukebox.status)
				{
					//Format Compact Flash Card
					case 0x02:
						jukebox.progress = 0x10;
						jukebox.status = 0;
						jukebox.format_compact_flash = true;
						break;

					//Select Music Files
					case 0x08:
						//Set current file
						jukebox.current_file = jukebox.last_music_file;
						jukebox.io_regs[0xA0] = jukebox.current_file;

						jukebox.current_category = 0;
						jukebox.file_limit = jukebox.music_files.size();
						jukebox_set_file_info();

						//Update the number of songs in a given category
						jukebox.io_regs[0xAD] = jukebox.file_limit;

						//Setup remaining playback time if not recording
						jukebox.io_regs[0x0084] = 0;
						jukebox.io_regs[0x0085] = 0;

						break;

					//Record Music Files
					case 0x09:
						//Turn on microphone if possible
						if(config::use_microphone && apu_stat->mic_init)
						{
							apu_stat->is_mic_on = true;
							apu_stat->save_recording = false;
							SDL_PauseAudioDevice(apu_stat->mic_id, 0);
						}

						jukebox.current_category = 0;
						jukebox.file_limit = jukebox.music_files.size();
						jukebox_set_file_info();

						jukebox.is_recording = true;
						jukebox.current_recording_time = 0;
						jukebox.io_regs[0x82] = 0x1010;
						jukebox.io_regs[0xA0] = jukebox.file_limit;
						jukebox.progress = 1;

						//Set remaining recording time
						jukebox.io_regs[0x0086] = (jukebox.remaining_recording_time / 60);
						jukebox.io_regs[0x0087] = (jukebox.remaining_recording_time % 60);
						break;

					//Select Voice Files
					case 0x0A:
						//Set current file
						jukebox.current_file = jukebox.last_voice_file;
						jukebox.io_regs[0xA0] = jukebox.current_file;

						jukebox.current_category = 1;
						jukebox.file_limit = jukebox.voice_files.size();
						jukebox_set_file_info();

						//Update the number of songs in a given category
						jukebox.io_regs[0xAE] = jukebox.file_limit;

						//Setup remaining playback time if not recording
						jukebox.io_regs[0x0084] = 0;
						jukebox.io_regs[0x0085] = 0;

						break;

					//Record Voice Files
					case 0x0B:
						//Turn on microphone if possible
						if(config::use_microphone && apu_stat->mic_init)
						{
							apu_stat->is_mic_on = true;
							apu_stat->save_recording = false;
							SDL_PauseAudioDevice(apu_stat->mic_id, 0);
						}

						jukebox.current_category = 1;
						jukebox.file_limit = jukebox.voice_files.size();
						jukebox_set_file_info();

						jukebox.is_recording = true;
						jukebox.current_recording_time = 0;
						jukebox.io_regs[0xA0] = jukebox.file_limit;
						jukebox.progress = 1;

						//Set remaining recording time
						jukebox.io_regs[0x0086] = (jukebox.remaining_recording_time / 60);
						jukebox.io_regs[0x0087] = (jukebox.remaining_recording_time % 60);
						break;

					//Select Karaoke Files
					case 0x0C:
						//Set current file
						jukebox.current_file = jukebox.last_karaoke_file;
						jukebox.io_regs[0xA0] = jukebox.current_file;

						jukebox.current_category = 2;
						jukebox.file_limit = jukebox.karaoke_files.size();
						jukebox_set_file_info();

						//Update the number of songs in a given category
						jukebox.io_regs[0xAF] = jukebox.file_limit;

						//Setup remaining playback time if not recording
						jukebox.io_regs[0x0084] = 0;
						jukebox.io_regs[0x0085] = 0;

						break;

					//Record Karaoke Files
					case 0x0D:
						//Turn on microphone if possible
						if(config::use_microphone && apu_stat->mic_init)
						{
							apu_stat->is_mic_on = true;
							apu_stat->save_recording = false;
							SDL_PauseAudioDevice(apu_stat->mic_id, 0);
						}

						jukebox.current_category = 0;
						jukebox.current_recording_time = 0;
						jukebox.current_file = jukebox.last_music_file;
						jukebox.io_regs[0xA0] = jukebox.current_file;
						jukebox.file_limit = jukebox.music_files.size();
						jukebox_set_file_info();
						jukebox.current_category = 2;

						jukebox.is_recording = true;
						jukebox.progress = 1;

						//Set current file for Karaoke recording, uses indices 0x102 and 0x103
						//0x102 = Hundreds value (000 - 900), 0x103 = Tens value (00 - 99), each has unusual offsets too
						jukebox.io_regs[0x102] = (((jukebox.karaoke_files.size() + 1) / 100) + 0xA6) & 0xFF;
						jukebox.io_regs[0x103] = (((jukebox.karaoke_files.size() + 1) % 100) + 0xA8) & 0xFF;

						//Set remaining recording time
						jukebox.io_regs[0x0086] = (jukebox.remaining_recording_time / 60);
						jukebox.io_regs[0x0087] = (jukebox.remaining_recording_time % 60);

						break;

					//Commit Artist/Title Changes - Only applicable for Music Files
					case 0x11:
						if(!jukebox.current_category)
						{
							jukebox_update_metadata();
							jukebox_set_file_info();
						}

						break;

					//Play Audio File
					case 0x13:
						//Play dummy audio file for now
						jukebox.progress = 1;

						//Set audio output to GBA speaker or headphones via configuration index
						apu_stat->ext_audio.use_headphones = (jukebox.config & 0x200) ? 0 : 1;
						apu_stat->ext_audio.sample_pos = 0;

						//Set/Reset volume depending on the audio category
						if(jukebox.current_category == 0x01)
						{
							apu_stat->ext_audio.volume = 0x3F - (jukebox.io_regs[0x8A] & 0x3F);
						}

						else
						{
							apu_stat->ext_audio.volume = 0x3F - (jukebox.io_regs[0x88] & 0x3F);
						}
						
						//Set Playback/Recording Status
						//Load data from file if possible
						switch(jukebox.current_category)
						{
							case 0x00:
								jukebox.io_regs[0x82] = (jukebox.is_recording) ? 0x1012 : 0x1001;

								if(!jukebox.music_files.empty() && (!jukebox.is_recording))
								{
									apu_stat->ext_audio.playing = jukebox_load_audio(config::data_path + "jukebox/" + jukebox.music_files[jukebox.current_file]);
								}

								break;

							case 0x01:
								jukebox.io_regs[0x82] = (jukebox.is_recording) ? 0x1112 : 0x1101;

								if(!jukebox.voice_files.empty() && (!jukebox.is_recording))
								{
									apu_stat->ext_audio.playing = jukebox_load_audio(config::data_path + "jukebox/" + jukebox.voice_files[jukebox.current_file]);
								}
								
								break;

							case 0x02:
								jukebox.io_regs[0x82] = (jukebox.is_recording) ? 0x1211 : 0x1201;

								if(!jukebox.music_files.empty() && (jukebox.is_recording))
								{
									apu_stat->ext_audio.playing = jukebox_load_audio(config::data_path + "jukebox/" + jukebox.music_files[jukebox.current_file]);
								}

								else if(!jukebox.karaoke_files.empty())
								{
									apu_stat->ext_audio.playing = jukebox_load_audio(config::data_path + "jukebox/" + jukebox.karaoke_files[jukebox.current_file]);
								}

								break;
						}

						//Set External Audio ID for Jukebox
						apu_stat->ext_audio.id = 1;
						apu_stat->ext_audio.set_count = 0;
						apu_stat->ext_audio.current_set = 0;

						//Spectrum settings for Music Files
						if((!jukebox.current_category) && (jukebox.io_regs[0x9A]))
						{
							for(u32 x = 0; x < 9; x++) { jukebox.io_regs[0x90 + x] = 0; }
							jukebox.io_regs[0x9A] = 0x01;
						}

						//Start recording via microphone if possible
						if(config::use_microphone && apu_stat->mic_init && jukebox.is_recording)
						{
							apu_stat->is_recording = true;
							apu_stat->save_recording = false;
							SDL_PauseAudioDevice(apu_stat->mic_id, 0);
						}

						break;

					//Reset Current File
					case 0x14:
						jukebox.is_recording = false;

						//Turn off microphone if possible
						if(config::use_microphone && apu_stat->mic_init)
						{
							apu_stat->is_mic_on = true;
							SDL_PauseAudioDevice(apu_stat->mic_id, 1);
						}

						//Setup remaining playback time if not recording
						jukebox.io_regs[0x0084] = 0;
						jukebox.io_regs[0x0085] = 0;

						break;

					//Move Forward 1 File
					case 0x15:
						//When recording Karaoke files, pull files to play from Music list
						if((jukebox.current_category == 2) && (jukebox.is_recording))
						{
							jukebox.current_category = 0;
							restore = true;
						}

						if(jukebox.file_limit)
						{
							jukebox.current_file++;

							//Wrap around if current file is past the number of files in the list
							if(jukebox.current_file >= jukebox.file_limit) { jukebox.current_file = 0; }

							jukebox_set_file_info();
							jukebox.io_regs[0xA0] = jukebox.current_file;

							//Setup remaining playback time if not recording
							jukebox.io_regs[0x0084] = 0;
							jukebox.io_regs[0x0085] = 0;
						}

						//Restore current category to Karaoke for all other operations
						if(restore)
						{
							jukebox.current_category = 2;
							restore = false;
						}

						//Make sure external audio channel stops playing
						apu_stat->ext_audio.playing = false;

						break;

					//Move Backward 1 File
					case 0x16:
						//When recording Karaoke files, pull files to play from Music list
						if((jukebox.current_category == 2) && (jukebox.is_recording))
						{
							jukebox.current_category = 0;
							restore = true;
						}

						if(jukebox.file_limit)
						{
							//Wrap around if current file is at the beginning of the list in the list
							if(jukebox.current_file == 0) { jukebox.current_file = jukebox.file_limit; }

							jukebox.current_file--;

							jukebox_set_file_info();
							jukebox.io_regs[0xA0] = jukebox.current_file;

							//Setup remaining playback time if not recording
							jukebox.io_regs[0x0084] = 0;
							jukebox.io_regs[0x0085] = 0;
						}

						//Restore current category to Karaoke for all other operations
						if(restore)
						{
							jukebox.current_category = 2;
							restore = false;
						}

						break;

					//Move 1 File Forward in Playlist
					case 0x17:
						if((jukebox.current_category == 0) && ((jukebox.current_file + 1) < jukebox.file_limit))
						{
							u32 index = jukebox.current_file;

							//Swap data for current file and current file + 1
							std::swap(jukebox.music_files[index], jukebox.music_files[index + 1]);
							std::swap(jukebox.music_titles[index], jukebox.music_titles[index + 1]);
							std::swap(jukebox.music_artists[index], jukebox.music_artists[index + 1]);
							std::swap(jukebox.music_times[index], jukebox.music_times[index + 1]);

							jukebox.current_file++;
							jukebox_set_file_info();
							jukebox.io_regs[0xA0] = jukebox.current_file;

							jukebox_update_music_file_list();
						}

						break;

					//Move 1 File Backward in Playlist
					case 0x18:
						if((jukebox.current_category == 0) && (jukebox.current_file != 0))
						{
							u32 index = jukebox.current_file;

							//Swap data for current file and current file + 1
							std::swap(jukebox.music_files[index], jukebox.music_files[index - 1]);
							std::swap(jukebox.music_titles[index], jukebox.music_titles[index - 1]);
							std::swap(jukebox.music_artists[index], jukebox.music_artists[index - 1]);
							std::swap(jukebox.music_times[index], jukebox.music_times[index - 1]);

							jukebox.current_file--;
							jukebox_set_file_info();
							jukebox.io_regs[0xA0] = jukebox.current_file;

							jukebox_update_music_file_list();
						}

						break;

					//Delete File
					case 0x19:
						if(jukebox.file_limit)
						{
							jukebox_delete_file();
							jukebox_set_file_info();
						}

						break;
							

					//Stop Playing Audio File
					case 0x20:
						//Reset audio input/output progress
						jukebox.progress = 0;
						jukebox.io_regs[0x82] = 0x0;

						//Save recorded file
						if(jukebox.is_recording)
						{
							jukebox_save_recording();
							jukebox.current_recording_time = 0;

							//When stopping recording for karaoke, set file info for music files
							if(jukebox.current_category == 2)
							{
								jukebox.current_category = 0;
								jukebox_set_file_info();
								jukebox.io_regs[0xA0] = jukebox.last_music_file;
								jukebox.current_category = 2;
							}

							else
							{
								jukebox_set_file_info();
								jukebox.io_regs[0xA0] = jukebox.file_limit;
							}
						}

						//Setup remaining playback time if not recording
						jukebox.io_regs[0x0084] = 0;
						jukebox.io_regs[0x0085] = 0;

						apu_stat->ext_audio.playing = false;

						//Stop recording via microphone if possible
						if(config::use_microphone && apu_stat->mic_init)
						{
							apu_stat->save_recording = true;
						}

						break;
				}

				//When recording Karaoke files, pull files to play from Music list
				if((jukebox.current_category == 2) && (jukebox.is_recording || was_recording))
				{
					jukebox.current_category = 0;
					restore = true;
				}

				//Update last file for each category
				switch(jukebox.current_category)
				{
					case 0x00: jukebox.last_music_file = jukebox.current_file; break;
					case 0x01: jukebox.last_voice_file = jukebox.current_file; break;
					case 0x02: jukebox.last_karaoke_file = jukebox.current_file; break;
				}

				//Restore current category to Karaoke for all other operations
				if(restore)
				{
					jukebox.current_category = 2;
					restore = false;
				}
			}

			//Write to Indices
			switch(jukebox.io_index)
			{
				//Misc Audio Registers
				case 0x008C:
				case 0x008F:
				case 0x009A:
				case 0x009B:
					jukebox.io_regs[jukebox.io_index] &= 0xFF00;
					jukebox.io_regs[jukebox.io_index] |= value;
					break;

				//Music Volume
				//Voice Volume
				case 0x0088:
				case 0x008A:
					jukebox.io_regs[jukebox.io_index] &= 0xFF00;
					jukebox.io_regs[jukebox.io_index] |= value;
					apu_stat->ext_audio.volume = 0x3F - (value & 0x3F);
					break;

				//Config Register
				case 0x01C8:
					jukebox.config &= 0xFF00;
					jukebox.config |= value;
					break;
			}

			//Music File metadata
			if((jukebox.io_index >= 0xB0) && (jukebox.io_index <= 0xCD))
			{
				jukebox.io_regs[jukebox.io_index] &= 0xFF00;
				jukebox.io_regs[jukebox.io_index] |= value;
			}

			break;

		//Reset Status
		case JB_REG_12:
			if(value == 1) { jukebox.status |= 0x100; }
			break;
	}

	//Process data from Jukebox once a specific index has been selected
	if((process_data) && (jukebox.io_index < 0x200))
	{
		switch(jukebox.io_index)
		{
			//Read Status
			case 0x0081:
				jukebox.out_hi = (jukebox.status >> 8) & 0xFF;
				jukebox.out_lo = (jukebox.status & 0xFF);
				break;

			//Spectrum Analyzer
			case 0x009A:
				if((jukebox.io_regs[0x9A]) && (jukebox.io_regs[0x9A] < 0xFFFF)) { jukebox.io_regs[0x9A]++; }
				jukebox.out_hi = (jukebox.io_regs[0x9A] >> 8) & 0xFF;
				jukebox.out_lo = (jukebox.io_regs[0x9A] & 0xFF);
				break;

			//Read Compact Flash Access Progress
			case 0x0101:
				jukebox.out_hi = ((jukebox.progress >> 1) >> 8) & 0xFF;
				jukebox.out_lo = ((jukebox.progress >> 1) & 0xFF);
				break;

			//Read User Config
			case 0x01C8:
				jukebox.out_hi = (jukebox.config >> 8) & 0xFF;
				jukebox.out_lo = (jukebox.config & 0xFF);
				break;

			//Default -> Read whatever data at unimplemented I/O
			default:
				jukebox.out_hi = (jukebox.io_regs[jukebox.io_index] >> 8) & 0xFF;
				jukebox.out_lo = (jukebox.io_regs[jukebox.io_index] & 0xFF);
		}
	}
}

/****** Reads a file for list of other audio files to be read by the GBA Music Recorder/Jukebox ******/
bool AGB_MMU::read_jukebox_file_list(std::string filename, u8 category)
{
	std::vector<std::string> *out_list = NULL;
	std::vector<u16> *out_time = NULL;

	//Grab the correct file list based on category
	switch(category)
	{
		case 0x00: out_list = &jukebox.music_files; out_time = &jukebox.music_times; break;
		case 0x01: out_list = &jukebox.voice_files; out_time = &jukebox.voice_times; break;
		case 0x02: out_list = &jukebox.karaoke_files; out_time = &jukebox.karaoke_times; break;
		default: std::cout<<"MMU::Error - Loading unknown category of audio files for Jukebox\n"; return false;
	}

	//Clear any previosly existing contents, read in each non-blank line from the specified file
	out_list->clear();
	out_time->clear();

	if(category == 0)
	{
		jukebox.music_titles.clear();
		jukebox.music_artists.clear();
	}

	std::string input_line = "";
	std::ifstream file(filename.c_str(), std::ios::in);

	if(!file.is_open())
	{
		std::cout<<"MMU::Error - Could not open list of music files from " << filename << "\n";
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
			std::string out_artist = "";
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

			//Grab song title - Music Files Only!
			if(category == 0)
			{
				if(!end_of_string)
				{
					s32 end_pos = input_line.find(":", (pos + 1));

					if(end_pos == std::string::npos)
					{
						out_title = input_line.substr(pos + 1);
						end_of_string = true;
					}

					else
					{
						out_title = input_line.substr((pos + 1), (end_pos - pos - 1));
						pos += (end_pos - pos);
					}
				}

				jukebox.music_titles.push_back(out_title);
			}

			//Grab song artist - Music Files Only!
			if(category == 0)
			{
				if(((pos + 1) < input_line.length()) && (!end_of_string)) { out_artist = input_line.substr(pos + 1); }
				jukebox.music_artists.push_back(out_artist);
			}

			out_list->push_back(out_str);
			out_time->push_back(out_sec);
		}
	}

	std::cout<<"MMU::Loaded audio files for Jukebox from " << filename << "\n";

	file.close();
	return true;
}

/****** Sets the file info when GBA Music Recorder/Jukebox tries to read it ******/
void AGB_MMU::jukebox_set_file_info()
{
	//Clear contents from indices 0xA1 - 0xA6 and 0xB0 - 0xCF
	for(u32 x = 0; x < 7; x++) { jukebox.io_regs[0xA1 + x] = 0x0000; }
	for(u32 x = 0; x < 32; x++) { jukebox.io_regs[0xB0 + x] = 0x0000; }

	std::vector<std::string> file_list;
	std::vector<u16> time_list;

	//Grab the correct file list based on category
	switch(jukebox.current_category)
	{
		case 0x00: file_list = jukebox.music_files; time_list = jukebox.music_times; break;
		case 0x01: file_list = jukebox.voice_files; time_list = jukebox.voice_times; break;
		case 0x02: file_list = jukebox.karaoke_files; time_list = jukebox.karaoke_times; break;
	}

	//Nothing to do if list is empty
	if(file_list.empty()) { return; }

	std::string temp_str = file_list[jukebox.current_file];

	//Convert filename from list to 8.3 DOS format if longer than 12 characters 
	if(temp_str.length() > 12)
	{
		std::string front = temp_str.substr(0, 8);
		std::string back = temp_str.substr((temp_str.length() - 4), temp_str.length());
		temp_str = front + back;
	}

	while(temp_str.length() < 12) { temp_str = " " + temp_str; }

	for(u32 x = 0, y = 0; y < 7; y++, x += 2)
	{
		u16 ascii_chrs = ((temp_str[x] << 8) | temp_str[x + 1]);
		jukebox.io_regs[0xA1 + y] = ascii_chrs;
	}

	jukebox.remaining_playback_time = time_list[jukebox.current_file];

	//Music - Set song title and artist
	if(jukebox.current_category == 0)
	{
		u8 len = 0;
		u32 index = 0xB0;
		u16 val = 0;

		//Set title
		temp_str = jukebox.music_titles[jukebox.current_file];
		len = (temp_str.length() < 30) ? temp_str.length() : 30;

		for(u32 x = 0; x < len; x++)
		{
			if(x & 0x01)
			{
				val |= temp_str[x];
				jukebox.io_regs[index++] = val;
			}

			else { val = (temp_str[x] << 8); }
		}

		if(len & 0x1) { jukebox.io_regs[index++] = val; }

		//Set artist
		index = 0xBF;
		temp_str = jukebox.music_artists[jukebox.current_file];
		len = (temp_str.length() < 30) ? temp_str.length() : 30;

		for(u32 x = 0; x < len; x++)
		{
			if(x & 0x01)
			{
				val |= temp_str[x];
				jukebox.io_regs[index++] = val;
			}

			else { val = (temp_str[x] << 8); }
		}

		if(len & 0x1) { jukebox.io_regs[index++] = val; }
	}
}

/****** Deletes a specific file from the Music Recorder/Jukebox ******/
bool AGB_MMU::jukebox_delete_file()
{
	std::vector<std::string> final_list;
	std::string filename;
	std::string delete_target;
	u16 update_index = 0;
	u16 update_time = 0;

	//Grab the correct file list based on category, also select update file and update index
	switch(jukebox.current_category)
	{
		case 0x00:
			filename = config::data_path + "jukebox/music.txt";
			update_time = jukebox.music_times[jukebox.current_file];
			update_index = 0xAD;
			delete_target = config::data_path + "jukebox/" + jukebox.music_files[jukebox.current_file];
			break;

		case 0x01:
			filename = config::data_path + "jukebox/voice.txt";
			update_time = jukebox.voice_times[jukebox.current_file];
			update_index = 0xAE;
			delete_target = config::data_path + "jukebox/" + jukebox.voice_files[jukebox.current_file];
			break;

		case 0x02:
			filename = config::data_path + "jukebox/karaoke.txt";
			update_time = jukebox.karaoke_times[jukebox.current_file];
			update_index = 0xAF;
			delete_target = config::data_path + "jukebox/" + jukebox.karaoke_files[jukebox.current_file];
			break;
		
		default:
			std::cout<<"MMU::Error - Loading unknown category of audio files for Jukebox\n";
			return false;
	}

	//Build new file without current file
	std::string input_line = "";
	std::ifstream ifile(filename.c_str(), std::ios::in);
	u32 line_count = 0;

	if(!ifile.is_open())
	{
		std::cout<<"MMU::Error - Could not open list of music files from " << filename << "\n";
		return false;
	}

	//Grab all non-empty lines from file
	while(getline(ifile, input_line))
	{
		//Don't grab
		if((!input_line.empty()) && (input_line != "\n"))
		{

			if(line_count != jukebox.current_file)
			{
				final_list.push_back(input_line);
			}

			line_count++;
		}
	}

	ifile.close();

	//Adjust current file position if necessary
	if(final_list.size())
	{
		if(jukebox.current_file == final_list.size()) { jukebox.current_file--; }
		jukebox.file_limit = final_list.size();
	}

	//When deleting last file in a category, make sure to zero-out all file positions
	else
	{
		jukebox.file_limit = 0;
		jukebox.current_file = 0;		

		switch(jukebox.current_category)
		{
			case 0: jukebox.last_music_file = 0; break;
			case 1: jukebox.last_voice_file = 0; break;
			case 2: jukebox.last_karaoke_file = 0; break;
		}
	}

	//Update contents
	std::ofstream ofile(filename.c_str(), std::ios::trunc);

	if(!ofile.is_open())
	{
		std::cout<<"MMU::Error - Could not open list of music files from " << filename << "\n";
		return false;
	}

	for(u32 x = 0; x < final_list.size(); x++) { ofile << final_list[x] << "\n"; }

	ofile.close();

	//Update internal Jukebox data with new file
	read_jukebox_file_list(filename, jukebox.current_category);

	//Update remaining Jukebox time
	jukebox.remaining_recording_time += update_time;

	//Update specific indices
	jukebox.io_regs[update_index] = jukebox.file_limit;
	jukebox.io_regs[0x00A0] = jukebox.current_file;

	//Delete file from data folder
	if(std::remove(delete_target.c_str()) == 0)
	{
		std::cout<<"MMU::Deleting audio recording file at " << delete_target << "\n";
	}

	else
	{
		std::cout<<"MMU::Error - Could not delete audio recording file at " << delete_target << "\n";
	}

	return true;
}

/****** Updates Music Recorder/Jukebox periodically ******/
void AGB_MMU::process_jukebox()
{
	//Update 0x0101 index while formatting the Compact Flash Card
	if(jukebox.format_compact_flash)
	{
		jukebox.progress += 0x80;

		//Finish formatting
		if(jukebox.progress >= 0xFF80)
		{
			std::string filename = "";
			std::string delete_target = "";
			jukebox.progress = 0;
			jukebox.status = 0x0102;

			//Delete all Jukebox recordings - Music, Voice Memos, and Karaoke
			for(u32 x = 0; x < jukebox.music_files.size(); x++)
			{
				delete_target = config::data_path + "jukebox/" + jukebox.music_files[x];

				if(std::remove(delete_target.c_str()) != 0)
				{
					std::cout<<"MMU::Error - Could not delete audio recording file at " << delete_target << "\n";
				}
			}

			for(u32 x = 0; x < jukebox.voice_files.size(); x++)
			{
				delete_target = config::data_path + "jukebox/" + jukebox.voice_files[x];

				if(std::remove(delete_target.c_str()) != 0)
				{
					std::cout<<"MMU::Error - Could not delete audio recording file at " << delete_target << "\n";
				}
			}

			for(u32 x = 0; x < jukebox.karaoke_files.size(); x++)
			{
				delete_target = config::data_path + "jukebox/" + jukebox.karaoke_files[x];

				if(std::remove(delete_target.c_str()) != 0)
				{
					std::cout<<"MMU::Error - Could not delete audio recording file at " << delete_target << "\n";
				}
			}

			//Truncate lists and update file info
			filename = config::data_path + "jukebox/music.txt";
			std::ofstream file_1(filename.c_str(), std::ios::trunc);
			jukebox.music_files.clear();
			jukebox.music_times.clear();
			jukebox.music_titles.clear();
			jukebox.music_artists.clear();

			filename = config::data_path + "jukebox/voice.txt";
			std::ofstream file_2(filename.c_str(), std::ios::trunc);
			jukebox.voice_files.clear();
			jukebox.voice_times.clear();

			filename = config::data_path + "jukebox/karaoke.txt";
			std::ofstream file_3(filename.c_str(), std::ios::trunc);
			jukebox.karaoke_files.clear();
			jukebox.karaoke_times.clear();

			jukebox.io_regs[0xAD] = 0;
			jukebox.io_regs[0xAE] = 0; 
			jukebox.io_regs[0xAF] = 0;

			//Reset remaining recording time to the maximum
			jukebox.remaining_recording_time = config::jukebox_total_time;
		}
	}

	//Update 0x0101 index while playing music or recording music
	else if(jukebox.progress)
	{
		jukebox.progress++;

		//When recording music, make sure to decrement remaining time left to record audio
		if((jukebox.is_recording) && ((jukebox.progress % 60) == 0) && ((jukebox.status == 0x113) || (jukebox.status == 0x121)) && (jukebox.remaining_recording_time))
		{
			jukebox.remaining_recording_time--;
			jukebox.current_recording_time++;

			if(!jukebox.remaining_recording_time) { jukebox.io_regs[0x82] = 0x00; }

			jukebox.io_regs[0x0086] = (jukebox.remaining_recording_time / 60);
			jukebox.io_regs[0x0087] = (jukebox.remaining_recording_time % 60);
		}

		//When playing music, make sure to increment playback time
		else if((!jukebox.is_recording) && ((jukebox.progress % 60) == 0) && (jukebox.status == 0x113))
		{
			u32 current_time = (jukebox.io_regs[0x0084] * 60) + jukebox.io_regs[0x0085] + 1;

			jukebox.io_regs[0x0084] = (current_time / 60);
			jukebox.io_regs[0x0085] = (current_time % 60);

			if(current_time >= jukebox.remaining_playback_time)
			{
				jukebox.io_regs[0x0082] = 0x00;
				jukebox.io_regs[0x0084] = 0x00;
				jukebox.io_regs[0x0085] = 0x00;

				//Make sure external audio channel stops playing
				apu_stat->ext_audio.playing = false;
			}
		}
	}
}

/****** Saves recording from the Music Recorder/Jukebox ******/
bool AGB_MMU::jukebox_save_recording()
{
	std::vector<std::string> *out_list = NULL;
	std::vector<u16> *out_time = NULL;
	std::string filename;
	u16 update_index = 0;
	u16 update_time = jukebox.current_recording_time;

	//Grab the correct file list based on category, also select update file and update index
	switch(jukebox.current_category)
	{
		case 0x00:
			out_list = &jukebox.music_files;
			out_time = &jukebox.music_times;
			filename = config::data_path + "jukebox/music.txt";
			update_index = 0xAD;
			break;

		case 0x01:
			out_list = &jukebox.voice_files;
			out_time = &jukebox.voice_times;
			filename = config::data_path + "jukebox/voice.txt";
			update_index = 0xAE;
			break;

		case 0x02:
			out_list = &jukebox.karaoke_files;
			out_time = &jukebox.karaoke_times;
			filename = config::data_path + "jukebox/karaoke.txt";
			update_index = 0xAF;
			break;
		
		default:
			std::cout<<"MMU::Error - Loading unknown category of audio files for Jukebox\n";
			return false;
	}

	//Generate new name for recording file
	std::string converted_name = util::to_str(jukebox.io_regs[update_index] + 1);
	while(converted_name.length() != 4) { converted_name = "0" + converted_name; }

	converted_name += (jukebox.current_category) ? ".WAV" : ".GB3";

	//Prepend each file with "M", "V", or "K" depending on the audio category
	switch(jukebox.current_category)
	{
		case 0x00: converted_name = "M" + converted_name; break;
		case 0x01: converted_name = "V" + converted_name; break;
		case 0x02: converted_name = "K" + converted_name; break;
	}

	out_list->push_back(converted_name);

	jukebox.recorded_file = converted_name;

	//Update GBE+'s Jukebox metadata
	out_time->push_back(update_time);
	converted_name += (":" + util::to_str(update_time));

	if(!jukebox.current_category)
	{
		jukebox.music_titles.push_back("");
		jukebox.music_artists.push_back("");
	}

	//Update contents
	std::ofstream file(filename.c_str(), std::ios::app);

	if(!file.is_open())
	{
		std::cout<<"MMU::Error - Could not open list of music files from " << filename << "\n";
		return false;
	}

	file << "\n" << converted_name << "\n";

	for(u32 x = 0; x < out_list->size(); x++)
	{
		std::string ts = out_list->at(x);
		if((ts.empty()) || (ts == "\n")) { out_list->erase(out_list->begin() + x); }
	}

	//Update currently selected file
	//Note, after recording karaoke files, switch to browsing music files, so update with music file info
	jukebox.io_regs[update_index] = out_list->size();
	jukebox.file_limit = (jukebox.current_category == 2) ? jukebox.music_files.size() : out_list->size();

	file.close();
	return true;
} 

/****** Updates metadata for Music Files ******/
void AGB_MMU::jukebox_update_metadata()
{
	std::string song_title = "";
	std::string artist_name = "";

	//Build strings for song title
	for(u32 x = 0; x < 15; x++)
	{
		u16 str_bytes = jukebox.io_regs[0xB0 + x];
		char tmp_1 = (str_bytes >> 8) & 0xFF;
		char tmp_2 = (str_bytes & 0xFF);

		if(!tmp_1) { break; }
		song_title += tmp_1;

		if(!tmp_2) { break; }
		song_title += tmp_2;
	}

	//Build strings for artist name
	for(u32 x = 0; x < 15; x++)
	{
		u16 str_bytes = jukebox.io_regs[0xBF + x];
		char tmp_1 = (str_bytes >> 8) & 0xFF;
		char tmp_2 = (str_bytes & 0xFF);

		if(!tmp_1) { break; }
		artist_name += tmp_1;

		if(!tmp_2) { break; }
		artist_name += tmp_2;
	}

	jukebox.music_titles[jukebox.current_file] = song_title;
	jukebox.music_artists[jukebox.current_file] = artist_name;

	//Save data new metadata to file
	std::string filename = config::data_path + "jukebox/music.txt";
	std::ofstream file(filename.c_str(), std::ios::trunc);

	if(!file.is_open())
	{
		std::cout<<"MMU::Error - Could not open list of music files from " << filename << "\n";
		return;
	}

	//Build strings to write to file
	for(u32 x = 0; x < jukebox.music_files.size(); x++)
	{
		bool finish_str = false;
		std::string out_str = "";

		//Add filename
		out_str += jukebox.music_files[x];

		//Add duration
		if(jukebox.music_times[x]) { out_str += (":" + util::to_str(jukebox.music_times[x])); }
		else { finish_str = true; }

		//Add name
		if(!jukebox.music_titles[x].empty() && !finish_str) { out_str += (":" + jukebox.music_titles[x]); }
		else { finish_str = true; }

		//Add artist
		if(!jukebox.music_artists[x].empty() && !finish_str) { out_str += (":" + jukebox.music_artists[x]); }

		file << out_str  << "\n";
	}
	
	file.close();
}

/****** Loads audio (.WAV) file for playback on Jukebox (GBA speakers or headphones) ******/
bool AGB_MMU::jukebox_load_audio(std::string filename)
{
	//Reset current karaoke track
	jukebox.karaoke_track = "";

	//Clear previous buffer if necessary
	SDL_FreeWAV(apu_stat->ext_audio.buffer);
	apu_stat->ext_audio.buffer = NULL;

	SDL_AudioSpec file_spec;

	//If specified audio file is not a PCM S16 LE WAV file, attempt to convert it through a temporary audio file
	//Grab last 4 characters of filename to determine if the conversion should happen
	std::string ext = filename;

	if(ext.size() >= 4)
	{
		ext = ext.substr(ext.size() - 4);
	}

	if((ext != ".wav") && (ext != ".WAV") && (ext != ".gb3") && (ext != ".GB3"))
	{
		std::string out_file = config::temp_media_file + ".wav";
		std::string sys_cmd = config::audio_conversion_cmd;

		//Skip conversion if current file has already been converted
		if(jukebox.last_converted_file == filename)
		{
			filename = out_file;
			jukebox.karaoke_track = out_file;
		}
			
		else
		{
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

				jukebox.last_converted_file = filename;
				filename = out_file;
				jukebox.karaoke_track = out_file;
			}

			else
			{
				std::cout<<"Conversion of audio file " << filename << " failed \n";
				return false;
			}
		}
	}

	else { jukebox.karaoke_track = filename; }

	if(SDL_LoadWAV(filename.c_str(), &file_spec, &apu_stat->ext_audio.buffer, &apu_stat->ext_audio.length) == NULL)
	{
		std::cout<<"MMU::Jukebox could not load audio file: " << filename << " :: " << SDL_GetError() << "\n";
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

	std::cout<<"MMU::Jukebox loaded audio file: " << filename << "\n";

	//Attempt to remove vocals from the audio file if using Music or Karaoke modes
	if((jukebox.current_category == 0) || (jukebox.current_category == 2))
	{
		jukebox.enable_karaoke = jukebox_load_karaoke_audio();
	}

	return true;
}

/****** Loads a karaoke track (.WAV) file for playback on Jukebox (GBA speakers or headphones) ******/
bool AGB_MMU::jukebox_load_karaoke_audio()
{
	//Clear previous buffer if necessary
	SDL_FreeWAV(apu_stat->ext_audio.karaoke_buffer);
	apu_stat->ext_audio.karaoke_buffer = NULL;

	SDL_AudioSpec file_spec;

	//This function MUST be called IMMEDIATELY AFTER the audio format conversion was successful in jukebox_load_audio()
	//This ensures that the file being loaded is a .WAV file in PCM S16 LE
	std::string in_file = jukebox.karaoke_track;
	std::string out_file = config::data_path + "gbe_temp_karaoke.wav";
	std::string sys_cmd = config::remove_vocals_cmd;

	//Delete any existing temporary media file in case audio conversion command complains
	std::remove(out_file.c_str());

	//Replace %in and %out with proper parameters
	std::string search_str = "%in";
	std::size_t pos = sys_cmd.find(search_str);

	if(pos != std::string::npos)
	{
		std::string start = sys_cmd.substr(0, pos);
		std::string end = sys_cmd.substr(pos + 3);
		sys_cmd = start + in_file + end;
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
		std::cout<<"MMU::Removing vocals from audio file " << in_file << "\n";
		system(sys_cmd.c_str());
		std::cout<<"MMU::Removal complete\n";
	}

	else
	{
		std::cout<<"Conversion of audio file " << in_file << " failed \n";
		return false;
	}

	if(SDL_LoadWAV(out_file.c_str(), &file_spec, &apu_stat->ext_audio.karaoke_buffer, &apu_stat->ext_audio.karaoke_length) == NULL)
	{
		std::cout<<"MMU::Jukebox could not load audio file: " << out_file << " :: " << SDL_GetError() << "\n";
		return false;
	}

	//Check format, must be S16 audio, LSB
	if(file_spec.format != AUDIO_S16)
	{
		std::cout<<"MMU::Jukebox loaded file, but format is not Signed 16-bit LSB audio\n";
		return false;
	}

	//Make sure the number of channels matches the original audio
	if(file_spec.channels != apu_stat->ext_audio.channels)
	{
		std::cout<<"MMU::Karaoke track does not have the same amount of channels as the original\n";
		return false;
	}

	//Make sure the frequency matches the original audio
	if(file_spec.freq != apu_stat->ext_audio.frequency)
	{
		std::cout<<"MMU::Karaoke track does not have the same frequency as the original\n";
		return false;
	}

	std::cout<<"MMU::Jukebox loaded audio file: " << out_file << "\n";
	return true;
}

/****** Updates the music file list after rearranging the current playlist ******/
bool AGB_MMU::jukebox_update_music_file_list()
{
	//Build new file without current file
	std::string output_line = "";
	std::string filename = config::data_path + "jukebox/music.txt";
	std::ofstream out_file(filename.c_str(), std::ios::trunc);

	if(!out_file.is_open())
	{
		std::cout<<"MMU::Error - Could not open list of music files from " << filename << "\n";
		return false;
	}

	//Output data for each music file on a separate line
	for(u32 x = 0; x < jukebox.music_files.size(); x++)
	{
		output_line = jukebox.music_files[x] + ":" + util::to_str(jukebox.music_times[x]);		
		if(!jukebox.music_titles[x].empty()) { output_line = output_line + ":" + jukebox.music_titles[x]; }
		if(!jukebox.music_artists[x].empty()) { output_line = output_line + ":" + jukebox.music_artists[x]; }
		output_line += "\n";

		out_file << output_line;
	}

	out_file.close();

	return true;
}

