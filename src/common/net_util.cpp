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

namespace net_util
{

//Sends data from server to remote client
s32 send_data(gbe_net_comm &client, void* buffer, u32 length, bool is_blocking)
{
	return SDLNet_TCP_Send(client.host_socket, buffer, length);
}

//Receives data from remote client sent to server
s32 recv_data(gbe_net_comm &server, void* buffer, u32 length, bool is_blocking)
{
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

//Receives response data on same host. Used for HTTP TCP transfers (GBMA)
s32 recv_response(gbe_net_comm &client, void* buffer, u32 length)
{
	return SDLNet_TCP_Recv(client.host_socket, buffer, length);
}

//Resolves hostname from a given IP address
s32 resolve_host(gbe_net_comm &req, std::string ip_address)
{
	s32 result = 0;
	
	if(ip_address.empty())
	{
		result = SDLNet_ResolveHost(&req.host_ip, NULL, req.port);
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
	if(req.host_socket != NULL) { result = true; }

	return result;
}

//Closes a TCP connection
void close_tcp(gbe_net_comm &req)
{

}

//Accepts a connection from a client for a server
bool accept_client(gbe_net_comm &req)
{
	bool result = false;

	req.remote_socket = SDLNet_TCP_Accept(req.host_socket);
	if(req.remote_socket != NULL) { result = true; }

	return result;
}

} //Namespace
