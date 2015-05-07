// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mmu_base.h
// Date : May 7, 2015
// Description : Generic MMU interface
//
// MMU interface that can be used for different systems (GB, GBC, GBA)
// The interface is abstracted, so it can be applied to different consoles

#ifndef BASE_MMU
#define BASE_MMU

#include "./../common.h"

class MMU_base
{
	public:

	MMU_base() {};
	virtual ~MMU_base() {};

	virtual void reset() = 0;

	//Saving opertions
	virtual bool read_file(std::string filename) = 0;
	virtual bool read_bios(std::string filename) = 0;
	virtual bool save_backup(std::string filename) = 0;
	virtual bool load_backup(std::string filename) = 0;
};

#endif // BASE_MMU



