// GB Enhanced+ Copyright Daniel Baxter 2017
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : timer.h
// Date : May 1, 2017
// Description : NDS Timer
//
// Defines the data structure used to emulate the 8 NDS timers
// Used as a header file here because multiple components (CPU, MMU, APU) need access to it

#ifndef NTR_TIMER
#define NTR_TIMER

#include "common.h"

struct nds_timer
{
	u16 cnt;
	u16 counter;
	u16 cycles;
	u16 reload_value;
	u16 prescalar;
	bool count_up;
	bool enable;
	bool interrupt;
};

#endif // NTR_TIMER