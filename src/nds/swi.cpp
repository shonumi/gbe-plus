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
		//WaitByLoop
		case 0x3:
			//std::cout<<"ARM9::SWI::WaitByLoop \n";
			swi_waitbyloop();
			break;

		//IntrWait
		case 0x4:
			//std::cout<<"ARM9::SWI::IntrWait \n";
			swi_intrwait();
			break;

		//VBlankIntrWait
		case 0x5:
			//std::cout<<"ARM9::SWI::VBlankIntrWait \n";
			swi_vblankintrwait();
			break;

		//Halt
		case 0x6:
			//std::cout<<"ARM9::SWI::Halt \n";
			swi_halt();
			break;

		//IsDebugger
		case 0xF:
			//std::cout<<"ARM9::SWI::IsDebugger \n";
			swi_isdebugger();
			break;

		default:
			std::cout<<"SWI::Error - Unknown NDS9 BIOS function 0x" << std::hex << comment << "\n";
			running = false;
			break;
	}
}

/****** HLE implementation of WaitByLoop - NDS9 ******/
void NTR_ARM9::swi_waitbyloop()
{
	//Setup the initial value for swi_waitbyloop_count - R0
	swi_waitbyloop_count = get_reg(0) & 0x7FFFFFFF;
	swi_waitbyloop_count >>= 2;

	//Set CPU idle state to 2
	idle_state = 2;
}

/****** HLE implementation of Halt - NDS9 ******/
void NTR_ARM9::swi_halt()
{
	//Set CPU idle state to 1
	idle_state = 1;

	//Destroy R0
	set_reg(0, 0);
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

	//Grab old IF, set current one to zero
	mem->nds9_old_if = mem->nds9_if;
	mem->nds9_if = 0;

	//Grab old IE, set current one to R1
	mem->nds9_old_ie = mem->nds9_ie;
	mem->nds9_ie = reg.r1;

	//Set CPU idle state to 3
	idle_state = 3;
}

/****** HLE implementation of VBlankIntrWait - NDS9 ******/
void NTR_ARM9::swi_vblankintrwait()
{
	//This is basically the IntrWait SWI, but R0 and R1 are both set to 1
	reg.r0 = 1;
	reg.r1 = 1;

	//Force IME on, Force IRQ bit in CPSR
	mem->write_u32(NDS_IME, 0x1);
	reg.cpsr &= ~CPSR_IRQ;

	//Grab old IF, set current one to zero
	mem->nds9_old_if = mem->nds9_if;
	mem->nds9_if = 0;

	//Grab old IE, set current one to R1
	mem->nds9_old_ie = mem->nds9_ie;
	mem->nds9_ie = reg.r1;

	//Set CPU idle state to 3
	idle_state = 3;
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
			//std::cout<<"ARM7::SWI::WaitByLoop \n";
			swi_waitbyloop();
			break;

		//VBlankIntrWait
		case 0x5:
			//std::cout<<"ARM7::SWI::VBlankIntrWait \n";
			swi_vblankintrwait();
			break;

		//Halt
		case 0x6:
			////std::cout<<"ARM7::SWI::Halt \n";
			swi_halt();
			break;

		//GetCRC16
		case 0xE:
			//std::cout<<"ARM7::SWI::GetCRC16 \n";
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

	//Set CPU idle state to 2
	idle_state = 2;
}

/****** HLE implementation of VBlankIntrWait - NDS9 ******/
void NTR_ARM7::swi_vblankintrwait()
{
	//This is basically the IntrWait SWI, but R0 and R1 are both set to 1
	reg.r0 = 1;
	reg.r1 = 1;

	//Force IME on, Force IRQ bit in CPSR
	mem->write_u32(NDS_IME, 0x1);
	reg.cpsr &= ~CPSR_IRQ;

	//Grab old IF, set current one to zero
	mem->nds7_old_if = mem->nds7_if;
	mem->nds7_if = 0;

	//Grab old IE, set current one to R1
	mem->nds7_old_ie = mem->nds7_ie;
	mem->nds7_ie = reg.r1;

	//Set CPU idle state to 3
	idle_state = 3;
}

/****** HLE implementation of Halt - NDS7 ******/
void NTR_ARM7::swi_halt()
{
	//Set CPU idle state to 1
	idle_state = 1;
}
