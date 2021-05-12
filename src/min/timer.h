// GB Enhanced+ Copyright Daniel Baxter 2021
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : timer.h
// Date : March 10, 2021
// Description : Pokemon Mini Timer
//
// Defines the data structure used to emulate the Pokemon Mini timer
// Used as a header file here because multiple components (CPU, MMU, APU) need access to it

#ifndef PM_TIMER
#define PM_TIMER

#include "common.h"

struct min_timer
{
	u32 clock_lo;
	u32 clock_hi;
	u32 prescalar_lo;
	u32 prescalar_hi;
	u16 cnt;
	u16 counter;
	u16 reload_value;

	u16 pivot;
	u8 pivot_status;

	u8 osc_lo;
	u8 osc_hi;

	bool interrupt;
	bool full_mode;

	bool enable_lo;
	bool enable_hi;

	bool enable_osc;

	bool enable_scalar_hi;
	bool enable_scalar_lo;
};

#endif // PM_TIMER 
