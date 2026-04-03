// GB Enhanced Copyright Daniel Baxter 2026
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : net_util.h
// Date : February 20, 2026
// Description : General networking utilities
//
// Encapsulates some SDL_net functionality to reduce code footprint
// Intended to help with transition from SDL_net 2.2.0 to 3.0+

#include "net_util.h"

#ifdef GBE_NETPLAY

namespace net_util
{

//Sends data from server to remote client
s32 send_data(gbe_net_comm &client, void* buffer, u32 length, bool is_blocking)
{
	if(client.host_socket == nullptr) { return 0; }

	return SDLNet_TCP_Send(client.host_socket, buffer, length);
}

//Receives data from remote client sent to server
s32 recv_data(gbe_net_comm &server, void* buffer, u32 length, bool is_blocking)
{
	if(server.remote_socket == nullptr) { return 0; }

	s32 bytes_recv = 0;

	if(is_blocking)
	{
		bytes_recv = SDLNet_TCP_Recv(server.remote_socket, buffer, length);
	}

	else
	{
		if(SDLNet_SocketReady(server.remote_socket))
		{
			bytes_recv = SDLNet_TCP_Recv(server.remote_socket, buffer, length);
		}
	}

	return bytes_recv;
}

//Sends response data to server. Used for HTTP TCP transfer (Net Gate)
s32 send_response(gbe_net_comm &client, void* buffer, u32 length)
{
	if(client.remote_socket == nullptr) { return 0; }

	return SDLNet_TCP_Send(client.remote_socket, buffer, length);
}

//Receives response data on same host. Used for HTTP TCP transfers (Mobile Adapter GB)
s32 recv_response(gbe_net_comm &client, void* buffer, u32 length)
{
	if(client.host_socket == nullptr) { return 0; }

	return SDLNet_TCP_Recv(client.host_socket, buffer, length);
}

//Resolves hostname from a given IP address
s32 resolve_host(gbe_net_comm &req, std::string ip_address)
{
	s32 result = 0;
	
	if(ip_address.empty())
	{
		result = SDLNet_ResolveHost(&req.host_ip, nullptr, req.port);
	}

	else
	{
		result = SDLNet_ResolveHost(&req.host_ip, ip_address.c_str(), req.port);
	}

	return result;
}

//Opens a TCP connection
bool open_tcp(gbe_net_comm &req)
{
	bool result = false;

	req.host_socket = SDLNet_TCP_Open(&req.host_ip);
	if(req.host_socket != nullptr) { result = true; }

	return result;
}

//Closes a TCP connection
void close_tcp(gbe_net_comm &req, net_comm_role role)
{
	if(role == NET_COMM_SERVER)
	{
		SDLNet_TCP_Close(req.host_socket);
	}

	else
	{
		SDLNet_TCP_Close(req.remote_socket);
	}
}

//Accepts a connection from a client for a server
bool accept_client(gbe_net_comm &req)
{
	if(req.host_socket == nullptr) { return false; }

	bool result = false;

	req.remote_socket = SDLNet_TCP_Accept(req.host_socket);

	if(req.remote_socket != nullptr)
	{
		result = true;

		if(req.tcp_sockets == nullptr)
		{
			req.tcp_sockets = SDLNet_AllocSocketSet(2);
		}

		SDLNet_TCP_AddSocket(req.tcp_sockets, req.host_socket);
		SDLNet_TCP_AddSocket(req.tcp_sockets, req.remote_socket);
		req.connected = true;
		req.remote_init = true;
	}

	return result;
}

//Accepts a connection from a server for a client
bool accept_server(gbe_net_comm &req)
{
	bool result = false;

	if(net_util::open_tcp(req))
	{
		result = true;

		if(req.tcp_sockets == nullptr)
		{
			req.tcp_sockets = SDLNet_AllocSocketSet(2);
		}

		SDLNet_TCP_AddSocket(req.tcp_sockets, req.host_socket);
		req.connected = true;
		req.host_init = true;
	}

	return result;
}

//Handles setup of client or server
void setup_comm(gbe_net_comm &req, u16 port, net_comm_role role)
{
	req.host_socket = nullptr;
	req.host_init = false;
	req.remote_socket = nullptr;
	req.remote_init = false;
	req.connected = false;
	req.port = port;
	req.role = role;
	req.tcp_sockets = nullptr;
}

//Closes any active connections for client or server
void close_comm(gbe_net_comm &req)
{
	bool is_valid_socket = (req.tcp_sockets != nullptr);

	if(is_valid_socket)
	{
		if(req.host_socket != nullptr)
		{
			SDLNet_TCP_DelSocket(req.tcp_sockets, req.host_socket);
		}

		if(req.remote_socket != nullptr)
		{
			if(is_valid_socket) { SDLNet_TCP_DelSocket(req.tcp_sockets, req.remote_socket); }
		}

		SDLNet_FreeSocketSet(req.tcp_sockets);
	}

	SDLNet_TCP_Close(req.host_socket);
	SDLNet_TCP_Close(req.remote_socket);

	req.host_socket = nullptr;
	req.host_init = false;
	req.remote_socket = nullptr;
	req.remote_init = false;
	req.connected = false;
	req.tcp_sockets = nullptr;
}

} //Namespace

#endif
