// GB Enhanced Copyright Daniel Baxter 2025
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : debug_util.cpp
// Date : November 5, 2025
// Description : Misc. utilites for debugging
//
// Provides miscellaneous utilities for debugging each core

#include "debug_util.h"

namespace dbg_util
{

//Ensure string length is valid for a given command
bool check_command_len(std::string full_cmd, std::string cmd, debug_param_types pt)
{
	bool result = false;
	u32 min_size = cmd.length();

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

}
