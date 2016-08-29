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
void NTR_ARM9::process_swi(u32 comment)
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
			std::cout<<"SWI::Error - Unknown NDS9 BIOS function 0x" << std::hex << comment << "\n";
			running = false;
			break;
	}
}

/****** HLE implementation of IntrWait - NDS9 ******/
void NTR_ARM9::swi_intrwait()
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
	old_if = mem->nds9_if;
	mem->nds9_if = 0;

	//Grab the interrupts to check from R1
	if_check = reg.r1;

	//Run controllers until an interrupt is generated
	while(!fire_interrupt)
	{
		clock();

		current_if = mem->nds9_if;
		ie_check = mem->nds9_ie;
		
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
	mem->nds9_if = (old_if | current_if);

	//Artificially hold PC at current location
	//This SWI will be fetched, decoded, and executed again until it hits VBlank 
	reg.r15 -= (arm_mode == ARM) ? 4 : 2;
}

/****** HLE implementation of IsDebugger - NDS9 ******/
void NTR_ARM9::swi_isdebugger()
{
	//Always act as if a retail NDS, set RO to zero
	set_reg(0, 0);

	//Destroy value at 0x27FFFF8 (halfword)
	mem->write_u16(0x27FFFF8, 0x0);
}

/****** Process Software Interrupts - NDS7 ******/
void NTR_ARM7::process_swi(u32 comment)
{
	switch(comment)
	{
		//WaitByLoop
		case 0x3:
			std::cout<<"ARM7::SWI::WaitByLoop \n";
			swi_waitbyloop();
			break;

		//GetCRC16
		case 0xE:
			std::cout<<"ARM7::SWI::GetCRC16 \n";
			swi_getcrc16();
			break;
			
		default:
			std::cout<<"SWI::Error - Unknown NDS7 BIOS function 0x" << std::hex << comment << "\n";
			running = false;
			break;
	}
}

/****** HLE implementation of GetCRC16 - NDS7 ******/
void NTR_ARM7::swi_getcrc16()
{
	//R0 = Initial CRC value
	//R1 = Start address of data to look at
	//R2 = Length of data to look at in bytes
	u16 crc = get_reg(0);
	u32 data_addr = get_reg(1) & ~0x1;
	u32 length = get_reg(2) & ~0x1;

	//LUT for CRC
	u16 table[] = { 0xC0C1, 0xC181, 0xC301, 0xC601, 0xCC01, 0xD801, 0xF001, 0xA001 };

	//Cycle through all the data to get the CRC16
	for(u32 x = 0; x < length; x++)
	{
		u16 data_byte = mem->memory_map[data_addr++];
		crc = crc ^ data_byte;

		for(u32 y = 0; y < 8; y++)
		{
			
			if(crc & 0x1)
			{
				crc >>= 1;
				crc = crc ^ (table[y] << (7 - y));
			}

			else { crc >>= 1; }
		}
	}

	set_reg(0, crc);
}

/****** HLE implementation of WaitByLoop - NDS7 ******/
void NTR_ARM7::swi_waitbyloop()
{
	//Setup the initial value for swi_waitbyloop_count - R0
	swi_waitbyloop_count = get_reg(0) & 0x7FFFFFFF;
}
