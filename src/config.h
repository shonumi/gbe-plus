// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : config.h
// Date : September 20, 2014
// Description : GBE+ configuration options
//
// Parses command-line arguments to configure GBE options

#ifndef GBA_CONFIG
#define GBA_CONFIG

#include <vector>
#include <string>

#include "common.h"

bool parse_cli_args();

namespace config
{ 
	extern std::string rom_file;
	extern std::vector <std::string> cli_args;
	extern bool use_debugger;
}

#endif // GBA_CONFIG