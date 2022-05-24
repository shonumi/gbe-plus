// GB Enhanced+ Copyright Daniel Baxter 2022
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : jukebox.cpp
// Date : May 22, 2022
// Description : Kemco Music Recorder / Radica Games Jukebox
//
// Handles I/O for the Jukebox
// Manages Jukebox metadata files and recording status

#include "mmu.h"
#include "common/util.h"

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

					//Play Audio File
					case 0x13:
						//Play dummy audio file for now
						jukebox.progress = 1;

						//Set Playback/Recording Status
						switch(jukebox.current_category)
						{
							case 0x00: jukebox.io_regs[0x82] = (jukebox.is_recording) ? 0x1012 : 0x1001; break;
							case 0x01: jukebox.io_regs[0x82] = (jukebox.is_recording) ? 0x1112 : 0x1101; break;
							case 0x02: jukebox.io_regs[0x82] = (jukebox.is_recording) ? 0x1211 : 0x1201; break;
						}

						break;

					//Reset Current File
					case 0x14:
						jukebox.is_recording = false;

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

						break;

					//Backward 1 File
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

							//When stopping recording for karaoke, set file info for music files
							if(jukebox.current_category == 2)
							{
								jukebox.current_category = 0;
								jukebox_set_file_info();
								jukebox.io_regs[0xA0] = jukebox.last_music_file;
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
				case 0x0088:
				case 0x008A:
				case 0x008C:
				case 0x008F:
				case 0x009A:
				case 0x009B:
					jukebox.io_regs[jukebox.io_index] &= 0xFF00;
					jukebox.io_regs[jukebox.io_index] |= value;
					break;

				//Config Register
				case 0x01C8:
					jukebox.config &= 0xFF00;
					jukebox.config |= value;
					break;
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
	std::vector<std::string> *out_list = NULL;
	std::string filename;
	u16 update_index = 0;

	//Grab the correct file list based on category, also select update file and update index
	switch(jukebox.current_category)
	{
		case 0x00:
			out_list = &jukebox.music_files;
			filename = config::data_path + "jukebox/music.txt";
			update_index = 0xAD;
			break;

		case 0x01:
			out_list = &jukebox.voice_files;
			filename = config::data_path + "jukebox/voice.txt";
			update_index = 0xAE;
			break;
		case 0x02:
			out_list = &jukebox.karaoke_files;
			filename = config::data_path + "jukebox/karaoke.txt";
			update_index = 0xAF;
			break;
		
		default:
			std::cout<<"MMU::Error - Loading unknown category of audio files for Jukebox\n";
			return false;
	}

	//Erase file from list and adjust current file position
	if(jukebox.current_file < out_list->size()) { out_list->erase(out_list->begin() + jukebox.current_file); }
	if(jukebox.current_file != 0) { jukebox.current_file--; }

	jukebox.file_limit = out_list->size();

	//Update contents
	std::ofstream file(filename.c_str(), std::ios::trunc);

	if(!file.is_open())
	{
		std::cout<<"MMU::Error - Could not open list of music files from " << filename << "\n";
		return false;
	}

	for(u32 x = 0; x < out_list->size(); x++) { file << out_list->at(x) << "\n"; }

	//Update specific indices
	jukebox.io_regs[update_index] = jukebox.file_limit;
	jukebox.io_regs[0x00A0] = jukebox.current_file;

	file.close();
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
			jukebox.progress = 0;
			jukebox.status = 0x0102;

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
			if(current_time >= jukebox.remaining_playback_time) { jukebox.io_regs[0x82] = 0x00; }

			jukebox.io_regs[0x0084] = (current_time / 60);
			jukebox.io_regs[0x0085] = (current_time % 60);
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
	out_list->push_back(converted_name);

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
