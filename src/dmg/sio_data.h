// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : sio_data.h
// Date : April 25, 2015
// Description : Serial I/O data
//
// Defines the SIO data structures and enums that the MMU will update whenever values are written in memory
// Only the SIO should read values from this namespace. Only the MMU should write values to this namespace.

#ifndef GB_SIO_DATA
#define GB_SIO_DATA

#include "common.h"

//Serial Input-Output device-type enumeration
enum sio_types
{ 
	NO_GB_DEVICE,
	DMG_LINK,
	DMG_07_LINK,
	GBC_LINK,
	GB_PRINTER,
};

struct dmg_sio_data
{
	bool connected;
	bool locked;
	bool active_transfer;
	bool double_speed;
	bool internal_clock;
	u8 shifts_left;
	u8 transfer_byte;
	u32 shift_counter;
	sio_types sio_type;
};

#endif // GB_SIO_DATA 
