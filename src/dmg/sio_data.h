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
	GB_LINK,
	GB_PRINTER,
	GB_MOBILE_ADAPTER,
	GB_BARDIGUN_SCANNER,
	GB_BARCODE_BOY,
	GB_FOUR_PLAYER_ADAPTER,
	GB_POWER_ANTENNA,
	GB_SINGER_IZEK,
	GB_ASCII_TURBO_FILE,
};

//Infrared device-type enumeration
enum ir_types
{
	GBC_IR_PORT,
	GBC_FULL_CHANGER,
	GBC_POKEMON_PIKACHU_2,
	GBC_POCKET_SAKURA,
	GBC_TV_REMOTE,
	GBC_LIGHT_SOURCE,
	GBC_IR_NOISE,
	GB_KISS_LINK,
};

enum printer_state
{
	GBP_AWAITING_PACKET,
	GBP_RECEIVE_COMMAND,
	GBP_RECEIVE_COMPRESSION_FLAG,
	GBP_RECEIVE_LENGTH,
	GBP_RECEIVE_DATA,
	GBP_RECEIVE_CHECKSUM,
	GBP_ACKNOWLEDGE_PACKET,
};

enum mobile_state
{
	GBMA_AWAITING_PACKET,
	GBMA_RECEIVE_HEADER,
	GBMA_RECEIVE_DATA,
	GBMA_RECEIVE_CHECKSUM,
	GBMA_ACKNOWLEDGE_PACKET,
	GBMA_ECHO_PACKET,
};

enum bardigun_state
{
	BARDIGUN_INACTIVE,
	BARDIGUN_SEND_BARCODE,
};

enum barcode_boy_state
{
	BARCODE_BOY_INACTIVE,
	BARCODE_BOY_ACTIVE,
	BARCODE_BOY_SEND_BARCODE,
	BARCODE_BOY_FINISH,
};

enum four_player_state
{
	FOUR_PLAYER_INACTIVE,
	FOUR_PLAYER_PING,
	FOUR_PLAYER_SYNC,
	FOUR_PLAYER_PROCESS_NETWORK,
	FOUR_PLAYER_RESTART_NETWORK,
};

enum singer_izek_state
{
	SINGER_PING,
	SINGER_SEND_HEADER,
	SINGER_SEND_DATA,
	SINGER_STATUS,
	SINGER_GET_COORDINATES,
};

enum turbo_file_state
{
	TURBO_FILE_PACKET_START,
	TURBO_FILE_PACKET_BODY,
	TURBO_FILE_PACKET_END,
	TURBO_FILE_DATA,
};

enum full_changer_state
{
	FULL_CHANGER_INACTIVE,
	FULL_CHANGER_SEND_SIGNAL,
};

enum tv_remote_state
{
	TV_REMOTE_INACTIVE,
	TV_REMOTE_SEND_SIGNAL,
};

enum pocket_ir_state
{
	POCKET_IR_INACTIVE,
	POCKET_IR_SEND_SIGNAL,
};

enum gb_kiss_link_state
{
	GKL_INACTIVE,

	GKL_RECV,
	GKL_RECV_PING,
	GKL_RECV_DATA,
	GKL_RECV_HANDSHAKE_3C,
	GKL_RECV_HANDSHAKE_55,
	GKL_RECV_HANDSHAKE_AA,
	GKL_RECV_HANDSHAKE_C3,

	GKL_SEND,
	GKL_SEND_CMD,
	GKL_SEND_PING,
	GKL_SEND_HANDSHAKE_3C,
	GKL_SEND_HANDSHAKE_55,
	GKL_SEND_HANDSHAKE_AA,
	GKL_SEND_HANDSHAKE_C3,
	
	GKL_ON = 201,
	GKL_ON_PING = 501,
	GKL_OFF_PING = 200,
	GKL_OFF_SHORT = 260,
	GKL_OFF_LONG = 520,
	GKL_OFF_STOP = 1000,
	GKL_OFF_END = 100,
};

enum gb_kiss_link_stage
{
	GKL_INIT,
	GKL_REQUEST_ID,
	GKL_WRITE_ID,
};

struct dmg_sio_data
{
	bool connected;
	bool active_transfer;
	bool double_speed;
	bool internal_clock;
	bool sync;
	bool use_hard_sync;
	bool ping_finish;
	bool send_data;
	u8 shifts_left;
	u8 transfer_byte;
	u8 last_transfer;
	u8 network_id;
	u8 ping_count;
	u32 shift_counter;
	u32 shift_clock;
	u32 sync_counter;
	u32 sync_clock;
	u8 sync_delay;
	u32 dmg07_clock;
	sio_types sio_type;
	ir_types ir_type;
};

#endif // GB_SIO_DATA 
