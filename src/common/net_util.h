// GB Enhanced Copyright Daniel Baxter 2026
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : net_util.h
// Date : February 19, 2026
// Description : General networking utilities
//
// Encapsulates some SDL_net functionality to reduce code footprint
// Intended to help with transition from SDL_net 2.2.0 to 3.0+

#ifndef GBE_NET_UTIL
#define GBE_NET_UTIL

#include <string>

#include <SDL.h>
#include <SDL_net.h>

#include "common.h"

#ifdef GBE_NETPLAY

enum net_comm_role
{
	NET_COMM_SERVER = true,
	NET_COMM_CLIENT = false,
};

enum net_comm_misc
{
	NET_COMM_IS_BLOCKING,
	NET_COMM_IS_NONBLOCKING,
};

//Acts as both server/client, depending on usage
struct gbe_net_comm
{
	TCPsocket host_socket, remote_socket;
	SDLNet_SocketSet tcp_sockets;
	IPaddress host_ip;
	bool connected;
	bool host_init;
	bool remote_init;
	u16 port;
	net_comm_role role;
};

namespace net_util
{
	s32 send_data(gbe_net_comm &client, void* buffer, u32 length, bool is_blocking = false);
	s32 recv_data(gbe_net_comm &server, void* buffer, u32 length, bool is_blocking = false);
	s32 recv_response(gbe_net_comm &client, void* buffer, u32 length);
	s32 resolve_host(gbe_net_comm &req, std::string ip_address);

	bool accept_client(gbe_net_comm &req);
	bool accept_server(gbe_net_comm &req);

	bool open_tcp(gbe_net_comm &req);
	void close_tcp(gbe_net_comm &req);

	void setup_comm(gbe_net_comm &req, u16 port, net_comm_role role);
	void close_comm(gbe_net_comm &req);
};

#endif

#endif // GBE_NET_UTIL
