// GB Enhanced Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : cp15.cpp
// Date : March 12 2016
// Description : ARM9 Coprocessor 15
//
// Handles the built in System Control Processor
// Reads and writes to coprocessor registers and performs system control based on that

#include <iostream>

#include "cp15.h"

/****** CP15 Constructor ******/
CP15::CP15()
{
	reset();
}

/****** CP15 Destructor ******/
CP15::~CP15() { }

/****** Reset CP15 ******/
void CP15::reset()
{
	//Zero out CP15 registers
	for(int x = 0; x < 34; x++) { regs[x] = 0; }

	//For NDS9 also set C0,C0,0 to 0x41059461
	regs[C0_C0_0] = 0x41059461;

	//Set cache type
	regs[C0_C0_1] = 0x0F0D2112;

	pu_enable = false;
	unified_cache = false;
	instr_cache = false;
	exception_vector = 0x0;
	cache_replacement = false;
	pre_armv5 = false;
	dtcm_enable = false;
	dtcm_read_mode = false;
	itcm_enable = false;
	itcm_read_mode = false;
}		


/****** C7,C5,0 - Invalidates the entire instruction cache ******/
void CP15::invalidate_instr_cache()
{
	//std::cout<<"CP15::C7,C5,0 - Invalidate Instruction Cache (STUBBED)\n";
}

/****** C7,C5,1 - Invalidates instruction cache line (32 bytes) ******/
void CP15::invalidate_instr_cache_line(u32 virtual_address)
{
	//std::cout<<"CP15::C7,C5,1 - Invalidate Instruction Cache Line (STUBBED)\n";
}

/****** C7,C6,0 - Invalidates the entire data cache ******/
void CP15::invalidate_data_cache()
{
	//std::cout<<"CP15::C7,C6,0 - Invalidate Data Cache (STUBBED)\n";
}

/****** C7,C6,1 - Invalidates data cache line (32 bytes) ******/
void CP15::invalidate_data_cache_line(u32 virtual_address)
{
	//std::cout<<"CP15::C7,C6,1 - Invalidate Data Cache Line (STUBBED)\n";
}

/****** C7,C10,4 - Drain write buffer ******/
void CP15::drain_write_buffer()
{
	//std::cout<<"CP15::C7,C10,4 - Drain Write Buffer (STUBBED)\n";
}

/****** C9,C0,0 - Lockdown data cache ******/
void CP15::data_cache_lockdown()
{
	//std::cout<<"CP15::C9,C0,0 - Data Cache Lockdown (STUBBED)\n";
}

/****** C9,C0,1 - Lockdown instruction cache ******/
void CP15::instr_cache_lockdown()
{
	//std::cout<<"CP15::C9,C0,1 - Instruction Cache Lockdown (STUBBED)\n";
}

/****** C9,C1,0 - Sets the Data TCM size and base address ******/
void CP15::set_dtcm_size_base()
{
	//std::cout<<"CP15::C9,C1,0\n";

	regs[CP15_TEMP] = regs[C9_C1_0] & ~0xFFF;

	//std::cout<<"DTCM BASE -> 0x" << std::hex << regs[CP15_TEMP] << "\n";
}

/****** C9,C1,1 - Sets the Instruction TCM size and base address ******/
void CP15::set_itcm_size_base()
{
	//std::cout<<"CP15::C9,C1,1 - Set Instruction TCM Size and Base (STUBBED)\n";
}