// GB Enhanced+ Copyright Daniel Baxter 2018
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : sio_data.h
// Date : August 13, 2018
// Description : Serial I/O data
//
// Defines the SIO data structures and enums that the MMU will update whenever values are written in memory
// Only the SIO should read values from this namespace. Only the MMU should write values to this namespace.

#ifndef GBA_SIO_DATA
#define GBA_SIO_DATA

#include "common.h"

//Serial Input-Output device-type enumeration
enum agb_sio_types
{ 
	NO_GBA_DEVICE,
	INVALID_GBA_DEVICE,
	GBA_LINK,
};

//Serial Input-Output device mode enumeration
enum agb_sio_modes
{
	NORMAL_8BIT,
	NORMAL_32BIT,
	MULTIPLAY_16BIT,
	UART,
	GENERAL_PURPOSE,
	JOY_BUS,
};

struct agb_sio_data
{
	bool connected;
	bool active_transfer;
	bool internal_clock;
	bool sync;
	u32 sync_counter;
	u32 sync_clock;
	u32 transfer_data;
	u32 shift_counter;
	u32 shift_clock;
	u32 shifts_left;
	u16 cnt;
	u16 r_cnt;
	u8 player_id;
	agb_sio_types sio_type;
	agb_sio_modes sio_mode;
};

#endif // GBA_SIO_DATA 
