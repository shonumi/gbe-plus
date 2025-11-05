// GB Enhanced Copyright Daniel Baxter 2025
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : debug_util.h
// Date : November 5, 2025
// Description : Misc. utilites for debugging
//
// Provides miscellaneous utilities for debugging each core

#ifndef GBE_DBG_UTIL
#define GBE_DBG_UTIL

#include <string>

#include "common.h"

namespace dbg_util
{
	enum debug_param_types
	{
		HEX_PARAMETER,
		INT_PARAMETER,
	};

	bool parse_debug_command(std::string full_cmd, std::string cmd, debug_param_types pt);
}

#endif // GBE_DBG_UTIL 
