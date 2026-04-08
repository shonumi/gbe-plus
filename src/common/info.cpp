// GB Enhanced Copyright Daniel Baxter 2026
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : info.cpp
// Description : GBE+ Compile-Time Information
//
// Stores information such as git commit hash, install directory, etc
// Generated dynamically when building source code

#include "info.h"

namespace gbe_info
{

std::string hash = "";

std::string get_hash() { return hash; }

}
