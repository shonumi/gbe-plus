// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : swi.cpp
// Date : November 05, 2015
// Description : NDS ARM9 Software Interrupts
//
// Emulates the NDS's Software Interrupts via High Level Emulation

#include "arm9.h"

/****** Process Software Interrupts ******/
void ARM9::process_swi(u32 comment)
{
	switch(comment)
	{
		default:
			std::cout<<"SWI::Error - Unknown BIOS function 0x" << std::hex << comment << "\n";
			break;
	}
}
