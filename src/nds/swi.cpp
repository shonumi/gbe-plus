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
		//IsDebugger
		case 0xF:
			std::cout<<"ARM9::SWI::IsDebugger \n";
			swi_isdebugger();
			break;

		default:
			std::cout<<"SWI::Error - Unknown BIOS function 0x" << std::hex << comment << "\n";
			break;
	}
}

/****** HLE implementation of IsDebugger ******/
void ARM9::swi_isdebugger()
{
	//Always act as if a retail NDS, set RO to zero
	set_reg(0, 0);

	//Destroy value at 0x27FFFF8 (halfword)
	mem->write_u16(0x27FFFF8, 0x0);
}
