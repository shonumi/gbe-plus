// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : timer.h
// Date : January 10, 2015
// Description : GBA Timer
//
// Defines the data structure used to emulate the 4 GBA timers
// Used as a header file here because multiple components (CPU, MMU, APU) need access to it

#ifndef GBA_TIMER
#define GBA_TIMER

#include "common/common.h"

struct gba_timer
{
	u16 counter;
	u16 cycles;
	u16 reload_value;
	u16 prescalar;
	bool count_up;
	bool enable;
	bool interrupt;
};

#endif // GBA_TIMER