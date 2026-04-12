// GB Enhanced Copyright Daniel Baxter 2026
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : info.h
// Description : GBE+ Compile-Time Information
//
// Stores information such as git commit hash, install directory, etc
// Generated dynamically when building source code

#ifndef GBE_INFO
#define GBE_INFO

#include <string>

namespace gbe_info
{
	std::string get_hash();
	std::string get_install_folder();
}

#endif // GBE_INFO

