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
	for(int x = 0; x < 33; x++) { regs[x] = 0; }
}

/****** Coprocessor Register Transfer - MCR ******/
void CP15::mcr(u32 value) { }

/****** Coprocessor Register Transfer - MRC ******/
u32 CP15::mrc(u8 reg_id) { return 0; }
