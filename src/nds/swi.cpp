// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : swi.cpp
// Date : November 05, 2015
// Description : NDS ARM7-ARM9 Software Interrupts
//
// Emulates the NDS's Software Interrupts via High Level Emulation

#include "arm9.h"
#include "arm7.h"

/****** Process Software Interrupts - NDS9 ******/
void ARM9::process_swi(u32 comment)
{
	switch(comment)
	{
		//IntrWait
		case 0x4:
			std::cout<<"ARM9::SWI::IntrWait \n";
			swi_intrwait();
			break;

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

/****** HLE implementation of IntrWait ******/
void ARM9::swi_intrwait()
{
	//NDS9 version is slightly bugged. When R0 == 0, it simply waits for any new interrupt, then leaves
	//Normally, it should return immediately if the flags in R1 are already set in IF
	//R0 == 1 will wait for the specified flags in R1

	//Force IME on, Force IRQ bit in CPSR
	mem->write_u32(NDS_IME, 0x1);
	reg.cpsr &= ~CPSR_IRQ;

	u32 if_check, ie_check, old_if, current_if = 0;
	bool fire_interrupt = false;

	//Grab old IF, set current one to zero
	old_if = mem->read_u32_fast(NDS_IF);
	mem->write_u32_fast(NDS_IF, 0x0);

	//Grab the interrupts to check from R1
	if_check = reg.r1;

	//Run controllers until an interrupt is generated
	while(!fire_interrupt)
	{
		clock();

		current_if = mem->read_u32_fast(NDS_IF);
		ie_check = mem->read_u32_fast(NDS_IE);
		
		//Match up bits in IE and IF
		for(int x = 0; x < 21; x++)
		{
			//When there is a match check to see if IntrWait can quit
			if((ie_check & (1 << x)) && (if_check & (1 << x)))
			{
				//If R0 == 0, quit on any IRQ
				if(reg.r0 == 0) { fire_interrupt = true; }
				
				//If R0 == 1, quit when the IF flags match the ones specified in R1
				else if(current_if & if_check) { fire_interrupt = true; }
			}
		}
	}

	//Restore old IF, also OR in any new flags that were set
	mem->write_u32_fast(NDS_IF, old_if | current_if);

	//Artificially hold PC at current location
	//This SWI will be fetched, decoded, and executed again until it hits VBlank 
	reg.r15 -= (arm_mode == ARM) ? 4 : 2;
}

/****** HLE implementation of IsDebugger ******/
void ARM9::swi_isdebugger()
{
	//Always act as if a retail NDS, set RO to zero
	set_reg(0, 0);

	//Destroy value at 0x27FFFF8 (halfword)
	mem->write_u16(0x27FFFF8, 0x0);
}

/****** Process Software Interrupts ******/
void NTR_ARM7::process_swi(u32 comment)
{
	switch(comment)
	{
		default:
			std::cout<<"SWI::Error - Unknown BIOS function 0x" << std::hex << comment << "\n";
			break;
	}
}
