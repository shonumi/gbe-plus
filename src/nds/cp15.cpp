// GB Enhanced Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : cp15.cpp
// Date : March 12 2016
// Description : ARM9 Coprocessor 15
//
// Handles the built in System Control Processor
// Reads and writes to coprocessor registers and performs system control based on that

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
	for(int x = 0; x < 33; x++) { regs[x] = 0; }

	//For NDS9 also set C0,C0,0 to 0x41059461
	regs[C0_C0_0] = 0x41059461; 
}		
