// GB Enhanced Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : sio.h
// Date : April 30, 2016
// Description : Game Boy Serial Input-Output emulation
//
// Sets up SDL networking
// Emulates Gameboy-to-Gameboy data transfers

#ifndef GB_SIO
#define GB_SIO

#include "mmu.h"
#include "sio_data.h"

class DMG_SIO
{
	public:
	
	//Link to memory map
	DMG_MMU* mem;

	dmg_sio_data sio_stat;

	DMG_SIO();
	~DMG_SIO();

	bool init();
	void reset();

	bool send_bit();
	bool receive_bit();
};

#endif // GB_SIO
