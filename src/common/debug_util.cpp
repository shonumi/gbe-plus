// GB Enhanced Copyright Daniel Baxter 2025
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : debug_util.cpp
// Date : November 5, 2025
// Description : Misc. utilites for debugging
//
// Provides miscellaneous utilities for debugging each core

#include "util.h"
#include "debug_util.h"

namespace dbg_util
{

//Ensure string length is valid for a given command
bool check_command_len(std::string full_cmd, std::string cmd, debug_param_types pt)
{
	bool result = false;
	u32 min_size = cmd.length();

	//Make sure command is correct, instant abort!
	if(full_cmd.substr(0, min_size) != cmd)
	{
		return false;
	}

	switch(pt)
	{
		//Size must be cmd + 4 characters (space and "0x0") at minimum
		case HEX_PARAMETER:
			min_size += 4;

			//Check that the substring "0x" is present for hex parameters
			if(full_cmd.length() >= min_size)
			{
				if((full_cmd.substr((cmd.length() + 1), 2) == "0x"))
				{
					result = true;
				}
			}

			break;

		//Size must be cmd + 2 characters (space and "0") at minimum
		case INT_PARAMETER:
			min_size += 2;

			if(full_cmd.length() >= min_size)
			{
				result = true;
			}

			break;
	}

	return result;
}

//Ensures debug string has proper parameters
bool validate_command(std::string full_cmd, std::string cmd, debug_param_types pt, u32 &param)
{
	bool result = false;
	u32 start_pos = cmd.length();

	switch(pt)
	{
		case HEX_PARAMETER:
			start_pos += 3;

			if(start_pos < full_cmd.length())
			{ 
				result = util::from_hex_str(full_cmd.substr(start_pos), param);
			}

			break;

		case INT_PARAMETER:
			start_pos += 1;

			if(start_pos < full_cmd.length())
			{
				result = util::from_str(full_cmd.substr(start_pos), param);
			}

			break;
	}

	return result;
}

}
