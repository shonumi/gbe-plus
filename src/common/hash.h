// GB Enhanced Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : hash.h
// Date : July 23, 2015
// Description : Alphanumeric hash generation
//
// Produces alphanumeric hashes based on binary input
// Primarily used for custom graphics (though it can have any use, really)

#ifndef GBE_HASH
#define GBE_HASH

#include <string>

#include "common.h"

std::string raw_to_64(u16 input_word);

#endif // GBE_HASH
	
